//=============================================================================
// SkExcel2SpreadSheet.cpp
//=============================================================================
#include "SkExcel2SpreadSheet.hpp"
#include "SkExcelChartImport.hpp"
#include "SkExcelProgress.hpp"
#include <SkModelClass.hpp>
#include <SkTypesClass.hpp>
#include <SkRangeRefTransform.hpp>
#include <SkTableStyle.hpp>
#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace SkSpreadSheet;


namespace SkExcel {

#ifdef __EMSCRIPTEN__
// Single definition of the progress hook (see SkExcelProgress.hpp). Calls the
// host-provided JS callback synchronously; the worker relays it to the UI.
EM_JS(void, SkExcelReportProgressImpl, (int sPercent), {
    if (typeof globalThis !== 'undefined' &&
        typeof globalThis.__skExcelProgress === 'function') {
        globalThis.__skExcelProgress(sPercent);
    }
});
void SkExcelReportProgress(int sPercent) {
    if (sPercent < 0) sPercent = 0;
    if (sPercent > 100) sPercent = 100;
    SkExcelReportProgressImpl(sPercent);
}
#else
void SkExcelReportProgress(int) {}
#endif

// Bounds for workbook.xml definedName text (WASM / libc++: avoid pathological copies, length_error, OOB reads).
static constexpr tSize kMaxDefinedNameText = 65536;
static constexpr tSize kMaxSheetNameInDefinedName = 512;
static constexpr tSize kMaxRefInDefinedName = 32768;
static constexpr tSize kMaxDefinedNameAttrChars = 256; // Excel name length limit is 255

static tString BoundedXmlAttr(const char* sP, tSize sMaxChars) {
    tString wOut;
    if (sP == nullptr) {
        return wOut;
    }
    while (wOut.size() < sMaxChars && *sP != '\0') {
        wOut.push_back(*sP++);
    }
    return wOut;
}

// Append at most one PCDATA/CDATA value, stopping at '\0' or cap — never bulk-memcpy from pugi with a guessed length
// (a missing terminator would read past the buffer and trap in wasm).
static void AppendPcdataCapped(tString& ioOut, const char* sV) {
    if (sV == nullptr) {
        return;
    }
    while (ioOut.size() < kMaxDefinedNameText && *sV != '\0') {
        ioOut.push_back(*sV++);
    }
}

// Build definedName body from PCDATA/CDATA children (OOXML text is usually a single child).
// OOXML calculatedColumnFormula / totalsRowFormula: concatenate all PCDATA/CDATA (Excel may split lines);
// cap length like defined names to avoid pathological std::string growth on WASM.
static tString TableFormulaTextFromNode(pugi::xml_node sFormulaNode) {
    tString wOut;
    wOut.reserve(std::min(static_cast<tSize>(512), kMaxDefinedNameText));
    for (pugi::xml_node wCh = sFormulaNode.first_child(); wCh; wCh = wCh.next_sibling()) {
        if (wCh.type() == pugi::node_pcdata || wCh.type() == pugi::node_cdata) {
            AppendPcdataCapped(wOut, wCh.value());
            if (wOut.size() >= kMaxDefinedNameText) {
                break;
            }
        }
    }
    if (wOut.empty()) {
        AppendPcdataCapped(wOut, sFormulaNode.text().get());
    }
    return wOut;
}

static tString DefinedNameTextFromNode(pugi::xml_node sDn) {
    tString wOut;
    wOut.reserve(std::min(static_cast<tSize>(512), kMaxDefinedNameText));
    for (pugi::xml_node wCh = sDn.first_child(); wCh; wCh = wCh.next_sibling()) {
        if (wCh.type() == pugi::node_pcdata || wCh.type() == pugi::node_cdata) {
            AppendPcdataCapped(wOut, wCh.value());
            if (wOut.size() >= kMaxDefinedNameText) {
                break;
            }
        }
    }
    if (wOut.empty()) {
        AppendPcdataCapped(wOut, sDn.text().get());
    }
    return wOut;
}

static void StripOuterQuotes(tString& s) {
    while (!s.empty() && (s.front() == '\'' || s.front() == '"')) {
        s.erase(s.begin());
    }
    while (!s.empty() && (s.back() == '\'' || s.back() == '"')) {
        s.pop_back();
    }
}

// workbook.xml definedName text is often "Sheet!A1" or "Sheet!A1:B2" without a leading "=". Those must become
// EnsureRangeNamed, not CreateFormulaNamedRange — otherwise _xlnm.Print_Area is compiled as a bogus formula on _$$
// and can break named matrix / spill behavior.
// The first '!' may be inside #REF!, #NAME?, etc.; try every '!' so "OFFSET(#REF!,,,A5)" does not pick the wrong split.
static tBool TryWorkbookDefinedNameAsSheetReference(const tString& sText, SkExcel::tRangeNamed& ioRangeNamed) {
    if (sText.empty() || sText[0] == '=') {
        return false;
    }
    if (sText.size() > kMaxDefinedNameText) {
        return false;
    }
    // Formula / array literal names (e.g. JoursEtSemaines = {0,1,...}+...)
    if (sText.find('{') != tString::npos) {
        return false;
    }
    for (tSize wBang = sText.find('!'); wBang != tString::npos; wBang = sText.find('!', wBang + 1)) {
        if (wBang + 1 >= sText.size()) {
            continue;
        }
        tString wSheetName = sText.substr(0, wBang);
        tString wRef = sText.substr(wBang + 1);
        if (wSheetName.size() > kMaxSheetNameInDefinedName || wRef.size() > kMaxRefInDefinedName) {
            continue;
        }
        StripOuterQuotes(wSheetName);
        if (wSheetName.size() > kMaxSheetNameInDefinedName) {
            continue;
        }
        tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
        if (!ParseRange(wRef, wTop, wLeft, wBottom, wRight)) {
            continue;
        }
        ioRangeNamed.m_SheetName = wSheetName;
        ioRangeNamed.m_Ref = wRef;
        ioRangeNamed.m_Formula.clear();
        return true;
    }
    return false;
}

// Excel workbook defined names can list union areas as comma-separated sheet!refs (not formulas).
static void TrimWs(tString& s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.erase(s.begin());
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
}

static tBool IsExcelDefinedNameRefList(const tString& sText) {
    if (sText.empty() || sText.front() == '=') {
        return false;
    }
    if (sText.find('!') == tString::npos) {
        return false;
    }
    if (sText.find('(') != tString::npos || sText.find('{') != tString::npos) {
        return false;
    }
    return sText.find(',') != tString::npos;
}

static std::vector<tString> SplitExcelDefinedNameAreas(const tString& sText) {
    std::vector<tString> wAreas;
    tString wCurrent;
    tBool wInQuotes = false;
    for (tSize i = 0; i < sText.size(); ++i) {
        const char wCh = sText[i];
        if (wCh == '\'') {
            wInQuotes = !wInQuotes;
            wCurrent += wCh;
        } else if (wCh == ',' && !wInQuotes) {
            TrimWs(wCurrent);
            if (!wCurrent.empty()) {
                wAreas.push_back(wCurrent);
            }
            wCurrent.clear();
        } else {
            wCurrent += wCh;
        }
    }
    TrimWs(wCurrent);
    if (!wCurrent.empty()) {
        wAreas.push_back(wCurrent);
    }
    return wAreas;
}

static void ApplyExcelDefinedNameRefList(tApi& sApi, const tString& sName, const tString& sText) {
#ifdef debugformula
    cout << "Apply Excel defined-name ref list -> " << sName << "=" << sText << endl;
#endif
    for (const tString& wArea : SplitExcelDefinedNameAreas(sText)) {
        tRangeNamed wRangeNamed;
        if (TryWorkbookDefinedNameAsSheetReference(wArea, wRangeNamed)) {
            tSheet* wSheet = sApi.Sheet(wRangeNamed.m_SheetName);
            sApi.EnsureRangeNamed(sName, wRangeNamed.m_Ref, "", wSheet);
        }
    }
}

namespace {
// Sheet used extent: never materialize OOXML col/row width or format past real content.
struct tSheetUsedExtent {
    tInt m_RightCol = 0;
    tInt m_BottomRow = 0;
};

static void MergeUsedExtent(tSheetUsedExtent& ioExtent, tInt sRightCol, tInt sBottomRow) {
    if (sRightCol > ioExtent.m_RightCol) {
        ioExtent.m_RightCol = sRightCol;
    }
    if (sBottomRow > ioExtent.m_BottomRow) {
        ioExtent.m_BottomRow = sBottomRow;
    }
}

// OOXML ECMA-376: row/@r is optional. Omitted index is previous row + 1.
// Fall back to the first cell ref (e.g. "B12") when @r is missing so ht/hidden
// still land on the correct Sker row.
static tInt ResolveSheetRowIndex(pugi::xml_node sRow, tInt& ioNextImplied) {
    tInt wRIdx = sRow.attribute("r").as_int(0);
    if (wRIdx <= 0) {
        if (pugi::xml_node wCell = sRow.child("c")) {
            const char* wRef = wCell.attribute("r").value();
            if (wRef != nullptr && *wRef != '\0') {
                wRIdx = RefToRowCol(tString(wRef)).first;
            }
        }
    }
    if (wRIdx <= 0) {
        wRIdx = ioNextImplied;
    }
    if (wRIdx > 0) {
        ioNextImplied = wRIdx + 1;
    }
    return wRIdx;
}

static const char* LocalXmlName(const char* sName) {
    if (sName == nullptr || *sName == '\0') {
        return "";
    }
    const char* wColon = std::strrchr(sName, ':');
    return (wColon != nullptr) ? (wColon + 1) : sName;
}

static pugi::xml_node ChildByLocalName(pugi::xml_node sParent, const char* sLocal) {
    if (!sParent || sLocal == nullptr || *sLocal == '\0') {
        return {};
    }
    if (pugi::xml_node wExact = sParent.child(sLocal)) {
        return wExact;
    }
    for (pugi::xml_node wN = sParent.first_child(); wN; wN = wN.next_sibling()) {
        if (std::strcmp(LocalXmlName(wN.name()), sLocal) == 0) {
            return wN;
        }
    }
    return {};
}

// Apply Excel row ht / hidden onto Sker.
//
// `ht` is honored whenever present and > 0, regardless of `customHeight`:
// current Excel versions ignore that flag and fall back to defaultRowHeight
// only when `ht` is missing or invalid. Empty spacer rows often carry `ht`
// with no <c> at all, so they must go through this path too.
static void ApplyExcelRowSize(tApi& sApi, tInt sRow, pugi::xml_node sRowNode) {
    if (sRow <= 0 || !sRowNode) {
        return;
    }
    if (sRowNode.attribute("hidden").as_bool(false)) {
        sApi.UndoSizeRow(sRow, sRow, 0.0);
        return;
    }
    if (pugi::xml_attribute wHtAttr = sRowNode.attribute("ht")) {
        const tDouble wHtPt = wHtAttr.as_double(0.0);
        if (wHtPt > 0.0) {
            sApi.UndoSizeRow(sRow, sRow, ExcelRowHeightPtToSkMm(wHtPt));
        }
    }
}

// Rows that omit `ht` inherit sheet defaultRowHeight (Excel). Fill only
// unset slots (m_Size == -1 / no tColRow). Never overwrite an explicit ht
// or a hidden row (Size == 0).
static void FillUnsetExcelRowHeights(tApi& sApi, tInt sBottomRow, tDouble sDefaultMm) {
    if (sBottomRow <= 0 || sDefaultMm <= 0.0) {
        return;
    }
    tSheet* wSheet = sApi.ActiveSheet();
    if (wSheet == nullptr) {
        return;
    }
    for (tInt wRow = 1; wRow <= sBottomRow; ++wRow) {
        tColRow* wColRow = wSheet->Row(wRow);
        if (wColRow == nullptr || wColRow->Size() < 0.0) {
            sApi.UndoSizeRow(wRow, wRow, sDefaultMm);
        }
    }
}

static tSheetUsedExtent ScanSheetDataUsedExtent(const pugi::xml_document& sDoc) {
    tSheetUsedExtent wOut;
    const pugi::xml_node wSheetData = sDoc.child("worksheet").child("sheetData");
    if (!wSheetData) {
        return wOut;
    }
    tInt wNextImpliedRow = 1;
    for (pugi::xml_node wRow : wSheetData.children("row")) {
        const tInt wRIdx = ResolveSheetRowIndex(wRow, wNextImpliedRow);
        if (wRIdx > wOut.m_BottomRow) {
            wOut.m_BottomRow = wRIdx;
        }
        for (pugi::xml_node wCell : wRow.children("c")) {
            const char* wRef = wCell.attribute("r").value();
            if (wRef == nullptr || *wRef == '\0') {
                continue;
            }
            const std::pair<tInt, tInt> wRc = RefToRowCol(tString(wRef));
            if (wRc.first > wOut.m_BottomRow) {
                wOut.m_BottomRow = wRc.first;
            }
            if (wRc.second > wOut.m_RightCol) {
                wOut.m_RightCol = wRc.second;
            }
        }
    }
    return wOut;
}

// Cap `<col min="…" max="…">` width/style application at the sheet used extent.
static tInt ColStyleApplyMax(tInt sCmin, tInt sCmax, tInt sSheetUsedCol) {
    if (sCmin <= 0 || sCmax < sCmin) {
        return sCmax;
    }
    if (sSheetUsedCol > 0) {
        if (sSheetUsedCol < sCmin) {
            return sCmin - 1;
        }
        return std::min(sCmax, sSheetUsedCol);
    }
    const tInt wSpan = sCmax - sCmin + 1;
    if (wSpan <= 256) {
        return sCmax;
    }
    // Whole-sheet default col without known extent: seed one column only.
    return sCmin;
}

} // namespace



// ---------------------------------------------------------------------------
// Date format detection / Excel serial -> tDate (epoch) conversion.
// Excel stores dates as a serial number of days since 1899-12-30 (with the
// well-known 1900 leap-year bug: serials 1..59 map to Jan 1, 1900 .. Feb 28,
// 1900; serial 60 is the bogus Feb 29, 1900; serials >= 61 map to Mar 1, 1900
// onwards). When a cell value is numeric AND the cell style references a
// date/time format, we convert the serial to a tDate (time_t since UNIX epoch)
// so SkSpreadSheet renders it through the date format pipeline rather than as
// a raw number (e.g. 41587 -> 09/11/2013).
// ---------------------------------------------------------------------------

// Built-in OOXML numFmtIds that are date/time formats (per ECMA-376 Part 1).
// We treat 14..22, 27..36, 45..47, 50..58, 71..81 as date/time.
// IMPORTANT: this is independent of the format code string and lets us
// recognize locale-specific built-ins that have no formatCode in styles.xml.
static tBool IsBuiltinDateNumFmtId(tInt sNumFmtId) {
    if (sNumFmtId >= 14 && sNumFmtId <= 22) return true;
    if (sNumFmtId >= 27 && sNumFmtId <= 36) return true;
    if (sNumFmtId >= 45 && sNumFmtId <= 47) return true;
    if (sNumFmtId >= 50 && sNumFmtId <= 58) return true;
    if (sNumFmtId >= 71 && sNumFmtId <= 81) return true;
    return false;
}

// Detect a date/time format from its (possibly custom) format code.
// Strategy: scan outside of quoted strings and bracket directives ([Red], [$-409], ...)
// for any unescaped d/m/y/h/s token. We deliberately accept lone 'm' and 's'
// because Excel uses 'm' for both month and minute and 's' for seconds.
// Pure numeric/currency masks like "0.00", "#,##0.00", "0%" don't contain
// these tokens so they correctly return false.
static tBool FormatCodeLooksLikeDate(const tString& sFormatCode) {
    if (sFormatCode.empty()) return false;
    tBool wInQuote = false;
    tBool wInBracket = false;
    for (tSize wI = 0; wI < sFormatCode.size(); ++wI) {
        char wC = sFormatCode[wI];
        if (wC == '\\' && wI + 1 < sFormatCode.size()) { ++wI; continue; }
        if (wC == '"') { wInQuote = !wInQuote; continue; }
        if (!wInQuote) {
            if (wC == '[') { wInBracket = true; continue; }
            if (wC == ']') { wInBracket = false; continue; }
            if (wInBracket) continue;
            char wLc = (char)std::tolower((unsigned char)wC);
            if (wLc == 'd' || wLc == 'y' || wLc == 'm' || wLc == 'h' || wLc == 's') {
                return true;
            }
        }
    }
    return false;
}

// Convert an Excel serial date to a tDate (time_t).
//
// We delegate to tClassDate so an imported date shares the EXACT same time_t as
// dates produced by DATE()/EDATE()/TODAY() and every other serial->date path.
// This matters because equality (=A1=B1), VLOOKUP/MATCH exact match and date
// arithmetic compare the raw time_t, not the rendered calendar day: previously
// imported dates were anchored at noon UTC while formula dates used local
// midnight, so an imported date could never equal a computed one for the same
// day. tClassDate honors the 1900 leap-year bug and preserves any fractional
// time-of-day.
static tDate ExcelSerialToTDate(tDouble sSerial) {
    if (sSerial >= 0.0 && sSerial < 1.0) {
        return SkExcel::ExcelTimeOnlySerialToSkDate(sSerial);
    }
    return tClassDate(tVariant(sSerial)).Value();
}

// Parse a strict-OOXML ISO 8601 date/date-time cell value (cells with t="d") into a tDate.
// Strict-conformance workbooks (workbook@conformance="strict") store dates as ISO text in
// <v>, e.g. "2017-05-31" or "2017-09-01T00:00:01[.fffffff][Z]", instead of an Excel serial.
// Generic: accepts date-only and date+time; the optional fractional seconds and trailing 'Z'
// are ignored. Uses tClassDate so the result shares the same time_t anchor as DATE()/serial
// dates. Returns false when the text is not an ISO date so the caller can fall back.
static tBool IsoDateTimeToTDate(const tString& sIso, tDate& oDate) {
    tInt wY = 0, wMo = 0, wD = 0, wH = 0, wMi = 0, wS = 0;
    tSize wPos = 0;
    const tSize wLen = sIso.size();
    auto wReadInt = [&](tInt& oVal, tSize sDigits) -> tBool {
        if (wPos + sDigits > wLen) {
            return false;
        }
        tInt wValue = 0;
        for (tSize wI = 0; wI < sDigits; ++wI) {
            const char wC = sIso[wPos + wI];
            if (wC < '0' || wC > '9') {
                return false;
            }
            wValue = wValue * 10 + (wC - '0');
        }
        oVal = wValue;
        wPos += sDigits;
        return true;
    };
    auto wExpect = [&](char sC) -> tBool {
        if (wPos < wLen && sIso[wPos] == sC) {
            ++wPos;
            return true;
        }
        return false;
    };
    // Mandatory date part: YYYY-MM-DD.
    if (!wReadInt(wY, 4) || !wExpect('-') || !wReadInt(wMo, 2) || !wExpect('-') || !wReadInt(wD, 2)) {
        return false;
    }
    // Optional time part: 'T'HH:MM:SS (fractional seconds / trailing 'Z' are ignored).
    if (wPos < wLen && (sIso[wPos] == 'T' || sIso[wPos] == ' ')) {
        ++wPos;
        if (!wReadInt(wH, 2) || !wExpect(':') || !wReadInt(wMi, 2) || !wExpect(':') || !wReadInt(wS, 2)) {
            return false;
        }
    }
    if (wY <= 0 || wMo < 1 || wMo > 12 || wD < 1 || wD > 31) {
        return false;
    }
    tClassDate wDate;
    wDate.SetDateHour(wY, wMo, wD, wH, wMi, wS);
    oDate = wDate.Value();
    return true;
}

namespace {

tString TrimAsciiSpacesCopy(const tString& sText) {
    tSize wBegin = 0;
    while (wBegin < sText.size() && std::isspace(static_cast<unsigned char>(sText[wBegin]))) {
        ++wBegin;
    }
    tSize wEnd = sText.size();
    while (wEnd > wBegin && std::isspace(static_cast<unsigned char>(sText[wEnd - 1]))) {
        --wEnd;
    }
    return sText.substr(wBegin, wEnd - wBegin);
}

tString ToUpperAsciiCopy(const tString& sText) {
    tString wOut;
    wOut.reserve(sText.size());
    for (const char wChar : sText) {
        wOut += static_cast<char>(std::toupper(static_cast<unsigned char>(wChar)));
    }
    return wOut;
}

void SplitSheetPrefixFromRef(const tString& sRef, tString& oSheetPrefix, tString& oBody) {
    oSheetPrefix.clear();
    oBody = sRef;
    if (sRef.empty()) {
        return;
    }
    if (sRef.front() == '\'') {
        for (tSize wIndex = 1; wIndex < sRef.size(); ++wIndex) {
            if (sRef[wIndex] != '\'') {
                continue;
            }
            if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '\'') {
                ++wIndex;
                continue;
            }
            if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '!') {
                oSheetPrefix = sRef.substr(0, wIndex + 2);
                oBody = sRef.substr(wIndex + 2);
                return;
            }
            break;
        }
        return;
    }
    const tSize wBang = sRef.find('!');
    if (wBang != tString::npos) {
        oSheetPrefix = sRef.substr(0, wBang + 1);
        oBody = sRef.substr(wBang + 1);
    }
}

void AppendFrenchL1C1AxisSpec(const tString& sBody, tSize& ioPos, tString& oOut, tBool sAllowImplicitZero) {
    const tSize n = sBody.size();
    if (ioPos >= n) {
        if (sAllowImplicitZero) {
            oOut += "[0]";
        }
        return;
    }
    if (sBody[ioPos] == '[') {
        while (ioPos < n) {
            oOut += sBody[ioPos];
            if (sBody[ioPos] == ']') {
                ++ioPos;
                break;
            }
            ++ioPos;
        }
        return;
    }
    if (sBody[ioPos] == '(') {
        ++ioPos;
        tBool wNegative = false;
        if (ioPos < n && sBody[ioPos] == '+') {
            ++ioPos;
        } else if (ioPos < n && sBody[ioPos] == '-') {
            wNegative = true;
            ++ioPos;
        }
        tInt wOffset = 0;
        tBool wHasDigits = false;
        while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
            wHasDigits = true;
            wOffset = wOffset * 10 + static_cast<tInt>(sBody[ioPos] - '0');
            ++ioPos;
        }
        if (!wHasDigits || ioPos >= n || sBody[ioPos] != ')') {
            return;
        }
        ++ioPos;
        if (wNegative) {
            wOffset = -wOffset;
        }
        oOut += '[';
        oOut += std::to_string(wOffset);
        oOut += ']';
        return;
    }
    if (std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
        while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
            oOut += sBody[ioPos++];
        }
        return;
    }
    if (sAllowImplicitZero) {
        const char wNext = sBody[ioPos];
        const char wNextUpper = static_cast<char>(std::toupper(static_cast<unsigned char>(wNext)));
        if (wNextUpper == 'L' || wNextUpper == 'R' || wNextUpper == 'C' || wNext == ':') {
            oOut += "[0]";
        }
    }
}

tString ConvertFrenchL1C1AddressToR1C1(const tString& sAddr) {
    tString wSheetPrefix;
    tString wBody;
    SplitSheetPrefixFromRef(sAddr, wSheetPrefix, wBody);
    if (wBody.empty()) {
        return sAddr;
    }

    tString wOut;
    wOut.reserve(sAddr.size() + 16);
    wOut += wSheetPrefix;

    for (tSize wIndex = 0; wIndex < wBody.size(); ) {
        const char wCharUpper =
            static_cast<char>(std::toupper(static_cast<unsigned char>(wBody[wIndex])));
        if (wCharUpper == 'L' || wCharUpper == 'R') {
            wOut += 'R';
            ++wIndex;
            AppendFrenchL1C1AxisSpec(wBody, wIndex, wOut, true);
        } else if (wCharUpper == 'C') {
            wOut += 'C';
            ++wIndex;
            AppendFrenchL1C1AxisSpec(wBody, wIndex, wOut, true);
        } else if (wCharUpper == ':') {
            wOut += ':';
            ++wIndex;
        } else {
            return sAddr;
        }
    }
    return wOut;
}

tBool IsFrenchColumnAxisBody(const tString& sBody) {
    if (sBody.empty() || std::toupper(static_cast<unsigned char>(sBody[0])) != 'C') {
        return false;
    }
    const tString wConverted = ConvertFrenchL1C1AddressToR1C1(sBody);
    if (wConverted.empty() || std::toupper(static_cast<unsigned char>(wConverted[0])) != 'C') {
        return false;
    }
    for (tSize wIndex = 1; wIndex < wConverted.size(); ++wIndex) {
        const char wChar = wConverted[wIndex];
        if (wChar == '[' || wChar == ']' || wChar == '-' || wChar == '+' ||
            std::isdigit(static_cast<unsigned char>(wChar))) {
            continue;
        }
        return false;
    }
    return true;
}

tBool MatchesIndirectKeywordAt(const tString& sFormula, tSize sIndex, tSize& oKeywordLen) {
    static const char* const kIndirect = "INDIRECT";
    if (sIndex > 0) {
        const unsigned char wPrev = static_cast<unsigned char>(sFormula[sIndex - 1]);
        if (std::isalnum(wPrev) || wPrev == '_') {
            return false;
        }
    }
    for (tSize wOffset = 0; kIndirect[wOffset] != '\0'; ++wOffset) {
        if (sIndex + wOffset >= sFormula.size()) {
            return false;
        }
        if (std::toupper(static_cast<unsigned char>(sFormula[sIndex + wOffset])) !=
            static_cast<unsigned char>(kIndirect[wOffset])) {
            return false;
        }
    }
    oKeywordLen = 8;
    return true;
}

tBool IsR1C1StyleSecondArg(const tString& sArg) {
    const tString wTrimmed = ToUpperAsciiCopy(TrimAsciiSpacesCopy(sArg));
    return wTrimmed == "0" || wTrimmed == "FALSE" || wTrimmed == "FAUX";
}

tBool ParseIndirectCall(const tString& sFormula,
                        tSize sKeywordIndex,
                        tSize& oCallEnd,
                        tString& oRefText,
                        tBool& oHasSecondArg,
                        tString& oSecondArg) {
    oRefText.clear();
    oSecondArg.clear();
    oHasSecondArg = false;
    oCallEnd = sKeywordIndex;

    tSize wIndex = sKeywordIndex + 8;
    while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
        ++wIndex;
    }
    if (wIndex >= sFormula.size() || sFormula[wIndex] != '(') {
        return false;
    }
    ++wIndex;
    while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
        ++wIndex;
    }
    if (wIndex >= sFormula.size() || sFormula[wIndex] != '"') {
        return false;
    }
    ++wIndex;
    while (wIndex < sFormula.size()) {
        if (sFormula[wIndex] == '"') {
            if (wIndex + 1 < sFormula.size() && sFormula[wIndex + 1] == '"') {
                oRefText += '"';
                wIndex += 2;
                continue;
            }
            ++wIndex;
            break;
        }
        oRefText += sFormula[wIndex++];
    }
    while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
        ++wIndex;
    }
    if (wIndex < sFormula.size() && (sFormula[wIndex] == ',' || sFormula[wIndex] == ';')) {
        ++wIndex;
        oHasSecondArg = true;
        while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
            ++wIndex;
        }
        const tSize wArgStart = wIndex;
        tInt wDepth = 0;
        while (wIndex < sFormula.size()) {
            const char wChar = sFormula[wIndex];
            if (wChar == '(') {
                ++wDepth;
            } else if (wChar == ')') {
                if (wDepth == 0) {
                    break;
                }
                --wDepth;
            } else if ((wChar == ',' || wChar == ';') && wDepth == 0) {
                break;
            }
            ++wIndex;
        }
        oSecondArg = sFormula.substr(wArgStart, wIndex - wArgStart);
    }
    while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
        ++wIndex;
    }
    if (wIndex >= sFormula.size() || sFormula[wIndex] != ')') {
        return false;
    }
    ++wIndex;
    oCallEnd = wIndex;
    return true;
}

// Replace INDIRECT("LC(-1)",0) with R[0]C[-1] (shared-formula safe; optional import path).
void MaterializeIndirectFrenchL1C1CellRefsInFormula(tString& sFormula) {
    if (sFormula.empty() || !kSkExcelTransformIndirectEnabled) {
        return;
    }

    tString wResult;
    wResult.reserve(sFormula.size() + 32);
    tSize wIndex = 0;
    while (wIndex < sFormula.size()) {
        tSize wKeywordLen = 0;
        if (MatchesIndirectKeywordAt(sFormula, wIndex, wKeywordLen)) {
            tSize wCallEnd = 0;
            tString wRefText;
            tBool wHasSecondArg = false;
            tString wSecondArg;
            if (ParseIndirectCall(sFormula, wIndex, wCallEnd, wRefText, wHasSecondArg, wSecondArg)) {
                const tBool wR1C1 = !wHasSecondArg || IsR1C1StyleSecondArg(wSecondArg);
                tString wReplacement;
                if (wR1C1) {
                    tString wSheetPrefix;
                    tString wBody;
                    SplitSheetPrefixFromRef(wRefText, wSheetPrefix, wBody);
                    if (!wBody.empty() && !IsFrenchColumnAxisBody(wBody)) {
                        const tString wConverted = ConvertFrenchL1C1AddressToR1C1(wRefText);
                        if (wConverted != wRefText) {
                            wReplacement = wConverted;
                        }
                    }
                }
                if (!wReplacement.empty()) {
                    wResult += wReplacement;
                    wIndex = wCallEnd;
                    continue;
                }
            }
        }
        wResult += sFormula[wIndex++];
    }
    sFormula.swap(wResult);
}

} // namespace

tExcel2SpreadSheet::tExcel2SpreadSheet() : tClass() {
    m_RowFormulaNamdedRange = 1;
}

tExcel2SpreadSheet::~tExcel2SpreadSheet() {
}

/// @brief Create formula named range on specialize Sheet
void tExcel2SpreadSheet::CreateFormulaNamedRange(tApi& sApi, const tString& sName, const tString& sFormula) {
    tWorkBook* wWorkBook = sApi.ActiveWorkBook();
    if (wWorkBook != nullptr) {
        // workbook.xml can break structured refs across lines (e.g. Table[Ending LF balance]); align with cell formulas.
        tString wFormula = sFormula;
        TransFormFormulaSyntax(wFormula);
        if (FormulaContainsIndirect(wFormula) && !kSkExcelTransformIndirectEnabled) {
#ifdef debugformula
            cout << "Skip FormulaNamed (INDIRECT as text): " << sName << endl;
#endif
            return;
        }
#ifdef debugformula
        cout << "Apply FormulaNamed -> " << sName << "=" << wFormula << endl;
#endif
        wWorkBook->InsertNamedFormula(sName, wFormula);
        tFormulaNamed* wFormulaNamed = wWorkBook->RangeNamedContainer()->FormulaNamed(sName);
        wFormulaNamed->SetJsonFormulaValue(wFormula);
        wFormulaNamed->Compil(wFormula, false);
    }
}


/// @brief Load and parse Excel file
tBool tExcel2SpreadSheet::LoadAndParseExcelFile(const tString& sXlsxPath, tExcelPugiXMLReader& sReader) {
    if (!sReader.LoadExcelFile(sXlsxPath)) {
        std::cerr << "Failed to load: " << sXlsxPath << std::endl;
        return false;
    }
    sReader.ParseTheme();
    sReader.ParseStyles();
    sReader.ParseSharedStrings();
    sReader.ParseWorkbook();

#ifdef debuginfo
    sReader.DisplayStyles();
    // Display all worksheet names
    sReader.DisplayWorksheetNames();
    // Display conditional formatting formulas
    sReader.DisplayConditionalFormattingFormulas();
    // Display visual conditional formats
    sReader.DisplayConditionalFormattingVisuals();
    // Display sparklines
    sReader.DisplaySparklines();
    // Display comments/notes
    sReader.DisplayComments();
    // Display charts (type + series)
    sReader.DisplayCharts();
    // Display Excel tables (name, range, columns)
    sReader.DisplayTables();
    // Display Excel images (name, size, position)
    sReader.DisplayImages();
#endif
    
    return true;
}

/// @brief Create sheets in the API from Excel reader
void tExcel2SpreadSheet::CreateSheetsInApi(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    for (const auto& wName : sReader.GetWorksheetNames()) {
        sApi.AddSheet(wName.c_str());
    }
}

/// @brief Process defined names (named ranges) from workbook.xml
void tExcel2SpreadSheet::GetDefinedNames(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    // Named cells/ranges (definedNames in workbook.xml) -> EnsureNamedRange on the proper sheet
    if (const UnzippedFile* wWb = sReader.GetUnzippedFile("xl/workbook.xml")) {
        pugi::xml_document wWbDoc;
        if (wWbDoc.load_buffer(wWb->content.data(), wWb->content.size())) {
            pugi::xml_node wDefNames = wWbDoc.child("workbook").child("definedNames");
            if (!wDefNames) wDefNames = wWbDoc.child("definedNames");
            if (wDefNames) {
                // Composite key to skip duplicate XML nodes (same name + same localSheetId)
                std::set<tString> wSeenKeys;
                // Loop and defined name ======================================
                for (pugi::xml_node wDn : wDefNames.children("definedName")) {
                    // Name, optional localSheetId, text contains refs (space-separated)
                    const char* wNameRaw = wDn.attribute("name").as_string("");
                    const tString wName = BoundedXmlAttr(wNameRaw, kMaxDefinedNameAttrChars);
                    if (wName.empty()) {
                        continue;
                    }
                    tInt wLocalSheetId = wDn.attribute("localSheetId").as_int(-1);
                    tString wText = DefinedNameTextFromNode(wDn);
                    if (wText.empty()) continue;
                    tString wKey = wName;
                    if (wLocalSheetId >= 0) wKey += "@" + std::to_string(wLocalSheetId);
                    if (!wSeenKeys.insert(wKey).second) continue;  // already processed this (name, localSheetId)
#ifdef debugrangenamed
                    cout << "Name : " << wName.c_str() << "-->";
#endif
                    
                    tLexer wLex(wText.c_str());
                    tLexerToken wLexerToken = wLex.next();
                    tString wSheetName = "";
                    tString wRef = "";
                    tInt wCount = 0;
                    while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
#ifdef debugrangenamed
                        //cout << "  -> " << wLexerToken << endl;
#endif
                        switch (wLexerToken.Kind()) {
                            case tKind::Sheet:
                                if (wCount == 0) {
                                    wSheetName = wLexerToken.Lexeme();
                                    // strip trailing '!' from sheet reference (e.g. Clair! -> Clair)
                                    if (!wSheetName.empty() && wSheetName[wSheetName.size() - 1] == '!')
                                        wSheetName.resize(wSheetName.size() - 1);
                                
                                    tClassString wClassString(wSheetName);
                                    wSheetName = wClassString.Unquote();
                                }
                                break;
                            case tKind::Cell:
                                if (wCount == 1)
                                    wRef = wLexerToken.Lexeme();
                                break;
                            default:
                                break;
                        }
                        wCount++;
                        if (wCount > 2) break;
                        wLexerToken = wLex.next();
                    }
                    // Named range: either a reference (Sheet + Cell, 2 tokens) or a formula.
                    // Excel union areas use commas ('Sheet'!$A$1,'Sheet'!$B$1); the lexer path below
                    // only keeps the first area when wCount > 2.
                    const tBool wLooksLikeUnionSheetRefs =
                        (wText.find(',') != tString::npos && wText.find('!') != tString::npos);
                    tRangeNamed wRangeNamed;
                    if (!wLooksLikeUnionSheetRefs
                        && wCount == 2
                        && !wSheetName.empty()
                        && !wRef.empty()) {
#ifdef debugrangenamed
                        cout <<  wSheetName << "!"<< wRef << endl;
#endif
                        wRangeNamed.m_SheetName = wSheetName;
                        wRangeNamed.m_Ref = wRef;
                    } else if (wLooksLikeUnionSheetRefs) {
                        wRangeNamed.m_Formula = wText;
                    } else {
                        if (!TryWorkbookDefinedNameAsSheetReference(wText, wRangeNamed)) {
                            wRangeNamed.m_Formula = wText;
                        }
                    }
                    m_MapRangeNamed[wName] = std::move(wRangeNamed);
                }
            }
        }
    }
}

/// @brief Process structured tables from Excel
void tExcel2SpreadSheet::ProcessStructuredTables(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    // Read Excel structured tables (xl/tables/tableN.xml) and create named ranges
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = sReader.GetUnzippedFile(wSheetFile);
        if (!wItSheet || !wItSheet->HasPayload()) break;
        
        tString wSheetName = (wS-1) >= 0 && (wS-1) < (tInt)sReader.GetWorksheetNames().size() 
            ? sReader.GetWorksheetNames()[wS-1] 
            : ("sheet" + std::to_string(wS));
        
        // Find table relationships from sheet rels
        tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = sReader.GetUnzippedFile(wRelsPath);
        if (!wItRels) continue;
        
        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->content.data(), wItRels->content.size())) continue;
        
        // Helper to resolve relative paths
        auto resolveTarget = [](const tString& basePath, const tString& target) -> tString {
            if (target.rfind("../", 0) == 0) {
                return tString("xl/") + target.substr(3);
            }
            if (target.rfind("xl/", 0) == 0) {
                return target;
            }
            return basePath + target;
        };
        
        // Find all table relationships
        std::vector<tString> wTableTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/table") != tString::npos) {
                tString wTarget = wRel.attribute("Target").as_string("");
                wTarget = resolveTarget("xl/worksheets/_rels/", wTarget);
                if (wTarget.find("xl/tables/") == tString::npos) {
                    tSize p = wTarget.find("tables/");
                    if (p != tString::npos) {
                        wTarget = tString("xl/") + wTarget.substr(p);
                    }
                }
#ifdef debugdata
                cout << "Table ->" << wTarget << endl;
#endif
                wTableTargets.push_back(wTarget);
            }
        }
        
        // Parse each table XML file and create named range
        tSheet* wSheet = sApi.Sheet(wSheetName);
        for (const tString& wTablePath : wTableTargets) {
            auto wItTable = sReader.GetUnzippedFile(wTablePath);
            if (!wItTable) continue;

            pugi::xml_document wTableDoc;
            if (!wTableDoc.load_buffer(wItTable->content.data(), wItTable->content.size())) continue;

            pugi::xml_node wTableNode = wTableDoc.child("table");
            if (!wTableNode) continue;
            
            // Extract table attributes (copy ref to tString so we have a stable value;
            // OOXML ref can include or exclude the totals row depending on Excel version/export)
            const char* wTableName = wTableNode.attribute("name").as_string("");
            const char* wDisplayName = wTableNode.attribute("displayName").as_string("");
            const char* wRefAttr = wTableNode.attribute("ref").as_string("");
            tString wRef = (wRefAttr && *wRefAttr) ? tString(wRefAttr) : tString("");
#ifdef debugdata
            cout << "Ref " << wTableName << "-->" << wRef << endl;
#endif
            // Read totalsRowCount: use string then parse; fallback iterate attributes if namespaced.
            tInt wTotalsRowCount = 0;
            const char* wTotalsAttr = wTableNode.attribute("totalsRowCount").as_string("");
            if (wTotalsAttr && *wTotalsAttr) {
                wTotalsRowCount = std::atoi(wTotalsAttr);
                if (wTotalsRowCount < 0) wTotalsRowCount = 0;
            }
            if (wTotalsRowCount == 0) {
                for (pugi::xml_attribute a : wTableNode.attributes()) {
                    if (std::strstr(a.name(), "totalsRowCount") != nullptr) {
                        tInt v = std::atoi(a.value());
                        if (v > 0) wTotalsRowCount = v;
                        break;
                    }
                }
            }
            // Per OOXML (ISO 29500), when totalsRowCount > 0 the ref encompasses the entire table including the totals row.
            
            // Create named range for the table
            if (wTableName && *wTableName && !wRef.empty() && wSheet) {
                // Parse table reference to get coordinates
                tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
                if (ParseRange(wRef, wTop, wLeft, wBottom, wRight)) {
                    // Same behavior for all tables: OOXML says ref includes totals row when totalsRowCount > 0.
                    // Exclude totals row from Data so SUBTOTAL in totals row does not create a circular dependency.
                    tInt wLastDataRow = (wTotalsRowCount > 0) ? (wBottom - wTotalsRowCount) : wBottom;
                    if (wLastDataRow < wTop) wLastDataRow = wBottom;
                    // Extract column names, calculatedColumnFormula, and totalsRowFormula
                    tVectorString wColumnNames;
                    std::map<tInt, tString> wCalculatedColumnFormulas; // Column index -> formula
                    std::map<tInt, tString> wTotalsRowFormulas;        // Column index -> totals row formula
                    std::map<tInt, tString> wTotalsRowLabels;          // Column index -> totals row label
                    std::map<tInt, tString> wTotalsRowFunctions;       // Column index -> totals row function
                    pugi::xml_node wTableColumns = wTableNode.child("tableColumns");
                    if (wTableColumns) {
                        tInt wColIndex = 0;
#ifdef debugdata
                        cout << "ProcessStructuredTables: All columns in table '" << wTableName << "':" << endl;
#endif
                        for (pugi::xml_node wCol : wTableColumns.children("tableColumn")) {
                            const tChar* wCharColName = wCol.attribute("name").as_string("");
                            if (wCharColName && *wCharColName) {
                                tString wColName = tString(wCharColName);
                                // OOXML may use _x000a_ / real LF in tableColumn@name; must match normalized refs in formulas.
                                TrimSpacesBeforeNewline(wColName);
                                wColumnNames.push_back(wColName);
#ifdef debugdata
                                cout << "  Column[" << wColIndex << "]: '" << wColName << "'" << endl;
#endif
                            }
                            const char* wTotalsLabelAttr = wCol.attribute("totalsRowLabel").as_string("");
                            if (wTotalsLabelAttr && *wTotalsLabelAttr) {
                                wTotalsRowLabels[wColIndex] = tString(wTotalsLabelAttr);
                            }
                            const char* wTotalsFuncAttr = wCol.attribute("totalsRowFunction").as_string("");
                            if (wTotalsFuncAttr && *wTotalsFuncAttr) {
                                wTotalsRowFunctions[wColIndex] = tString(wTotalsFuncAttr);
                            }
                            // Check for calculatedColumnFormula
                            pugi::xml_node wCalculatedColumnFormula = wCol.child("calculatedColumnFormula");
                            if (wCalculatedColumnFormula) {
                                tString wFormulaStr = TableFormulaTextFromNode(wCalculatedColumnFormula);
                                if (!wFormulaStr.empty()) {
                                    wCalculatedColumnFormulas[wColIndex] = std::move(wFormulaStr);
#ifdef debugdata
                                    if (wColIndex < (tInt)wColumnNames.size()) {
                                        auto wItF = wCalculatedColumnFormulas.find(wColIndex);
                                        if (wItF != wCalculatedColumnFormulas.end()) {
                                            cout << "   -->" << wColumnNames[(tSize)wColIndex] << "=" << wItF->second << endl;
                                        }
                                    }
#endif
                                }
                            }
                            // Check for totalsRowFormula (formula for the totals row cell in this column)
                            pugi::xml_node wTotalsRowFormulaNode = wCol.child("totalsRowFormula");
                            if (wTotalsRowFormulaNode) {
                                tString wTotalsStr = TableFormulaTextFromNode(wTotalsRowFormulaNode);
                                if (!wTotalsStr.empty()) {
                                    wTotalsRowFormulas[wColIndex] = std::move(wTotalsStr);
                                }
                            }
                            wColIndex++;
                        }
                    }
                    
                    // Store table metadata (use stable wRef string)
                    tTableMetadata wMetadata;
                    wMetadata.m_Name = wTableName;
                    wMetadata.m_DisplayName = (wDisplayName && *wDisplayName) ? wDisplayName : wTableName;
                    wMetadata.mRef = wRef;
                    wMetadata.mSheetName = wSheetName;
                    wMetadata.columnNames = wColumnNames;
                    // Capture <tableStyleInfo> + headerRowCount/totalsRowCount.
                    // We default headerRowCount to 1 (OOXML default) when
                    // omitted; an explicit "0" disables the header overlay.
                    {
                        const char* wHdrAttr = wTableNode.attribute("headerRowCount").as_string("");
                        if (wHdrAttr && *wHdrAttr) {
                            wMetadata.m_HeaderRowCount = std::atoi(wHdrAttr);
                            if (wMetadata.m_HeaderRowCount < 0) wMetadata.m_HeaderRowCount = 0;
                        }
                        wMetadata.m_TotalsRowCount = wTotalsRowCount;
                        if (pugi::xml_node wInfo = wTableNode.child("tableStyleInfo")) {
                            const char* wStyleAttr = wInfo.attribute("name").as_string("");
                            if (wStyleAttr) wMetadata.m_StyleName = tString(wStyleAttr);
                            wMetadata.m_ShowFirstColumn  = wInfo.attribute("showFirstColumn").as_bool(false);
                            wMetadata.m_ShowLastColumn   = wInfo.attribute("showLastColumn").as_bool(false);
                            wMetadata.m_ShowRowStripes   = wInfo.attribute("showRowStripes").as_bool(false);
                            wMetadata.m_ShowColumnStripes= wInfo.attribute("showColumnStripes").as_bool(false);
                        }
                        wMetadata.m_HasAutoFilter = (wTableNode.child("autoFilter") != nullptr);
                    }
                    m_TableMetadata[wTableName] = wMetadata;
                    
                     tSheet* wSheet=sApi.Sheet(wSheetName);
                    
                    // If we have column names, create RangeData with columns.
                    // Only set firstrow: true when the first row of the table actually contains header labels
                    // (e.g. "Période 0", "Articles"); Encaissements has no header row in the sheet, Décaissements does.
                    if (!wColumnNames.empty()) {
                        tBool wUseFirstRowAsHeader = FirstRowMatchesColumnNames(sReader, wS, wTop, wLeft, wColumnNames);
                        std::ostringstream wJsonStream;
                        wJsonStream << "{";
                        wJsonStream << "\"" << kJsonKeyUseFirstRowAsHeader << "\":" << (wUseFirstRowAsHeader ? "true" : "false");
                        if (wTotalsRowCount > 0) {
                            wJsonStream << ",\"" << kJsonKeyTotalsRowCount << "\":" << wTotalsRowCount;
                        }
                        if (!wMetadata.m_DisplayName.empty()) {
                            wJsonStream << ",\"" << kJsonKeyTableDisplayName << "\":\""
                                        << EscapeJsonString(wMetadata.m_DisplayName) << "\"";
                        }
                        if (!wMetadata.m_StyleName.empty()) {
                            wJsonStream << ",\"" << kJsonKeyTableStyleName << "\":\""
                                        << EscapeJsonString(wMetadata.m_StyleName) << "\"";
                        }
                        if (wMetadata.m_ShowRowStripes) {
                            wJsonStream << ",\"" << kJsonKeyTableShowRowStripes << "\":true";
                        }
                        if (wMetadata.m_ShowColumnStripes) {
                            wJsonStream << ",\"" << kJsonKeyTableShowColumnStripes << "\":true";
                        }
                        if (wMetadata.m_ShowFirstColumn) {
                            wJsonStream << ",\"" << kJsonKeyTableShowFirstColumn << "\":true";
                        }
                        if (wMetadata.m_ShowLastColumn) {
                            wJsonStream << ",\"" << kJsonKeyTableShowLastColumn << "\":true";
                        }
                        if (wMetadata.m_HasAutoFilter) {
                            wJsonStream << ",\"" << kJsonKeyTableAutoFilter << "\":true";
                        }
                        std::map<tInt, tBool> wHiddenFilterButtonByColId;
                        if (pugi::xml_node wAutoFilterNode = wTableNode.child("autoFilter")) {
                            for (pugi::xml_node wFilterCol : wAutoFilterNode.children("filterColumn")) {
                                const tInt wColId = wFilterCol.attribute("colId").as_int(-1);
                                if (wColId < 0) {
                                    continue;
                                }
                                wHiddenFilterButtonByColId[wColId] =
                                    wFilterCol.attribute("hiddenButton").as_bool(false);
                            }
                        }
                        wJsonStream << ",\"" << kJsonKeyColumnDataVector << "\":[";
                        // RangeData column "index" matches 0-based offset within the table (same as SkRangeData / WASM).
                        for (tSize wI = 0; wI < wColumnNames.size(); ++wI) {
                            if (wI > 0) wJsonStream << ",";
                            tString wEscapedName = EscapeJsonString(wColumnNames[wI]);
                            wJsonStream << "{";
                            wJsonStream << "\"" << kJsonKeyColumnCol << "\":" << (wLeft + static_cast<tInt>(wI)) << ",";
                            wJsonStream << "\"" << kJsonKeyNameData << "\":\"" << wEscapedName << "\",";
                            wJsonStream << "\"" << kJsonKeyTypeData << "\":\"Text\",";
                            wJsonStream << "\"" << kJsonKeyFilterOperator << "\":\"None\",";
                            // Omit filtervalue when filter operator is None (it's optional)
                            wJsonStream << "\"" << kJsonKeySortOrder << "\":\"None\"";
                            auto wHiddenIt = wHiddenFilterButtonByColId.find(static_cast<tInt>(wI));
                            if (wHiddenIt != wHiddenFilterButtonByColId.end() && wHiddenIt->second) {
                                wJsonStream << ",\"" << kJsonKeyFilterButtonHidden << "\":true";
                            }
                            auto wCalcIt = wCalculatedColumnFormulas.find(static_cast<tInt>(wI));
                            if (wCalcIt != wCalculatedColumnFormulas.end() && !wCalcIt->second.empty()) {
                                wJsonStream << ",\"" << kJsonKeyCalculatedColumnFormula << "\":\""
                                            << EscapeJsonString(wCalcIt->second) << "\"";
                            }
                            auto wTotalsLabelIt = wTotalsRowLabels.find(static_cast<tInt>(wI));
                            if (wTotalsLabelIt != wTotalsRowLabels.end()
                                && !wTotalsLabelIt->second.empty()) {
                                wJsonStream << ",\"" << kJsonKeyTotalsRowLabel << "\":\""
                                            << EscapeJsonString(wTotalsLabelIt->second) << "\"";
                            }
                            auto wTotalsFuncIt = wTotalsRowFunctions.find(static_cast<tInt>(wI));
                            if (wTotalsFuncIt != wTotalsRowFunctions.end()
                                && !wTotalsFuncIt->second.empty()) {
                                wJsonStream << ",\"" << kJsonKeyTotalsRowFunction << "\":\""
                                            << EscapeJsonString(wTotalsFuncIt->second) << "\"";
                            }
                            auto wTotalsFormulaIt = wTotalsRowFormulas.find(static_cast<tInt>(wI));
                            if (wTotalsFormulaIt != wTotalsRowFormulas.end()
                                && !wTotalsFormulaIt->second.empty()) {
                                wJsonStream << ",\"" << kJsonKeyTotalsRowFormula << "\":\""
                                            << EscapeJsonString(wTotalsFormulaIt->second) << "\"";
                            }
                            wJsonStream << "}";
                        }
                        wJsonStream << "]";
                        wJsonStream << "}";
                        
                        tString wJsonData = wJsonStream.str().c_str();
                       
#ifdef debugdata
                        cout << "Data " << wTableName << "(" << Base10ToAlpha(wLeft) << wTop << ":" << Base10ToAlpha(wRight) << wLastDataRow << ")" << endl;
#endif
                        // Create RangeData with column names (header + data only, no totals row)
                        sApi.EnsureRangeData(wTableName, wJsonData, wTop, wLeft, wLastDataRow, wRight, wSheet);
                        
                        // Apply calculatedColumnFormula to data rows that have no sheet formula yet.
                        // Excel writes the template in first-data-row A1 (COUNTIFS(...,E13), PTS=(G13*3)+…).
                        // Overwriting ProcessSheet formulas with that raw A1 text made every row compute
                        // row 13 (League-Table Part A: all PTS equal → all RANK=1 → MATCH(2) #N/A).
                        if (!wCalculatedColumnFormulas.empty() && wTop < wLastDataRow) {
                            tInt wFirstDataRow = wUseFirstRowAsHeader ? (wTop + 1) : wTop;
                            for (const auto& wFormulaPair : wCalculatedColumnFormulas) {
                                tInt wColIndex = wFormulaPair.first;
                                tString wTemplate = wFormulaPair.second;
                                
                                tInt wCol = wLeft + wColIndex;
#ifdef debugdata
                                cout << "DEBUG ProcessStructuredTables: Found calculatedColumnFormula for column index " << wColIndex
                                     << " (absolute column " << Base10ToAlpha(wCol) << "), formula: " << wTemplate << endl;
#endif
                                TransFormFormulaSyntax(wTemplate, sApi, wSheet);
                                tString wFormulaForRows = wTemplate;
                                if (!FormulaContainsStructuredTableRef(wTemplate)
                                    && !FormulaUsesR1C1CellRefs(wTemplate)) {
                                    wFormulaForRows = ConvertA1CellRefsToR1C1(
                                        wTemplate, wFirstDataRow, wCol);
                                }
                                if (wFormulaForRows.empty()) {
                                    continue;
                                }
                                if (wFormulaForRows.front() != '=') {
                                    wFormulaForRows = "=" + wFormulaForRows;
                                }
                                for (tInt wRow = wFirstDataRow; wRow <= wLastDataRow; wRow++) {
                                    tCellAddr wCellAddr = std::make_tuple(wSheetName, Base10ToAlpha(wCol) + std::to_string(wRow));
                                    if (m_MapFormula.find(wCellAddr) != m_MapFormula.end()) {
                                        continue;
                                    }
                                    if (m_CellsWithCalculatedColumn.find(wCellAddr)
                                        != m_CellsWithCalculatedColumn.end()) {
                                        continue;
                                    }
                                    m_MapFormula[wCellAddr] = wFormulaForRows;
#ifdef debugdata
                                    cout << " Applied calculatedColumnFormula to "
                                         << Base10ToAlpha(wCol) << wRow << ":" << wFormulaForRows << endl;
#endif
                                }
                            }
                        }
                        // Apply totalsRowFormula to the totals row (wTotalsRowIndex: ref includes totals -> wBottom, else wBottom+1).
                        if (wTotalsRowCount > 0 && !wTotalsRowFormulas.empty()) {
                            for (const auto& wTotalsPair : wTotalsRowFormulas) {
                                tInt wColIndex = wTotalsPair.first;
                                tString wFormula = wTotalsPair.second;
                                tInt wCol = wLeft + wColIndex;
                                tString wCellRef = Base10ToAlpha(wCol) + std::to_string(wBottom);
                                tCellAddr wCellAddr = std::make_tuple(wSheetName, wCellRef);
                                TransFormFormulaSyntax(wFormula, sApi, wSheet);
                                m_MapFormula[wCellAddr] = wFormula;
                            }
                        }
                    } else {
                        // No column names, just create named range
                        sApi.EnsureRangeNamed(wTableName, wRef);
                    }
                } else {
                    // Failed to parse reference, fallback to simple named range
                    sApi.EnsureRangeNamed(wTableName,"", wRef, wSheet);
                    
                    // Store table metadata without column names
                    tTableMetadata wMetadata;
                    wMetadata.m_Name = wTableName;
                    wMetadata.m_DisplayName = (wDisplayName && *wDisplayName) ? wDisplayName : wTableName;
                    wMetadata.mRef = wRef;
                    wMetadata.mSheetName = wSheetName;
                    m_TableMetadata[wTableName] = wMetadata;
                }
            }
        }
    }
}

namespace {

static tBool RectsOverlap(tIndex sTop1, tIndex sLeft1, tIndex sBottom1, tIndex sRight1,
                          tIndex sTop2, tIndex sLeft2, tIndex sBottom2, tIndex sRight2) {
    return !(sBottom1 < sTop2 || sTop1 > sBottom2 || sRight1 < sLeft2 || sLeft1 > sRight2);
}

static tBool AutoFilterOverlapsStructuredTable(
    const std::map<tString, tTableMetadata>& sTables,
    const tString& sSheet,
    tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
    for (const auto& wEntry : sTables) {
        const tTableMetadata& wMeta = wEntry.second;
        if (wMeta.mSheetName != sSheet || wMeta.mRef.empty()) {
            continue;
        }
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!ParseRange(wMeta.mRef, wTop, wLeft, wBottom, wRight)) {
            continue;
        }
        if (RectsOverlap(sTop, sLeft, sBottom, sRight, wTop, wLeft, wBottom, wRight)) {
            return true;
        }
    }
    return false;
}

static tInt LastRowInWorksheetColumns(const pugi::xml_node& sWorksheet, tInt sLeft, tInt sRight) {
    tInt wMax = 0;
    if (pugi::xml_node wDim = sWorksheet.child("dimension")) {
        const char* wRef = wDim.attribute("ref").as_string("");
        if (wRef && *wRef) {
            tIndex wTop = 0;
            tIndex wLeft = 0;
            tIndex wBottom = 0;
            tIndex wRight = 0;
            if (ParseRange(tString(wRef), wTop, wLeft, wBottom, wRight)) {
                if (wRight >= (tIndex)sLeft && wLeft <= (tIndex)sRight) {
                    wMax = std::max(wMax, (tInt)wBottom);
                }
            }
        }
    }
    if (pugi::xml_node wSheetData = sWorksheet.child("sheetData")) {
        tInt wNextImpliedRow = 1;
        for (pugi::xml_node wRow : wSheetData.children("row")) {
            const tInt wRowNum = ResolveSheetRowIndex(wRow, wNextImpliedRow);
            if (wRowNum <= 0) {
                continue;
            }
            for (pugi::xml_node wCell : wRow.children("c")) {
                const char* wCellRef = wCell.attribute("r").as_string("");
                if (!wCellRef || !*wCellRef) {
                    continue;
                }
                const std::pair<tInt, tInt> wRc = RefToRowCol(tString(wCellRef));
                const tInt wCol = wRc.second;
                if (wCol >= sLeft && wCol <= sRight) {
                    wMax = std::max(wMax, wRowNum);
                }
            }
        }
    }
    return wMax;
}

static tString BuildRangeRef(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
    tStringStream wStream;
    wStream << Base10ToAlpha(sLeft) << sTop << ":" << Base10ToAlpha(sRight) << sBottom;
    return wStream.str();
}

static tBool ParseOoxmlCellValueToVariant(
    const tString& sValueStr,
    const char* sStyleAttr,
    const char* sTypeAttr,
    const tExcelPugiXMLReader& sReader,
    tVariant& sOut) {
    if (sValueStr.empty()) {
        return false;
    }
    // Strict-OOXML ISO date cells (t="d"): value is ISO 8601 text, not an Excel serial.
    // Handle first so a real date is stored (not the raw "YYYY-MM-DD" string), which would
    // otherwise break date comparisons, VLOOKUP/MATCH and date arithmetic downstream.
    if (sTypeAttr && *sTypeAttr && tString(sTypeAttr) == "d") {
        tDate wIsoDate = 0;
        if (IsoDateTimeToTDate(sValueStr, wIsoDate)) {
            sOut.SetDate(wIsoDate);
            return true;
        }
        // Not a valid ISO date: fall through to the generic handling below.
    }
    tClassString wNumberStr = tClassString(sValueStr);
    if (wNumberStr.IsNumber()) {
        // NOTE: std::stod throws std::out_of_range on out-of-range magnitudes (e.g. 1e400),
        // which under Wasm (libc++ -fno-exceptions) becomes a hard abort() during import.
        // IsNumber() already validated the token via strtod, so parse with strtod (no throw),
        // matching tClassString::ToDouble()'s Wasm-safe pattern.
        const tDouble wD = std::strtod(sValueStr.c_str(), nullptr);
        tBool wIsDate = false;
        if (sStyleAttr && *sStyleAttr) {
            tInt wStyleIdxLocal = std::atoi(sStyleAttr);
            tInt wNumFmtId = sReader.GetNumFmtIdForStyle(wStyleIdxLocal);
            if (IsBuiltinDateNumFmtId(wNumFmtId)) {
                wIsDate = true;
            } else {
                tString wFmtCode = sReader.GetFormatCodeForStyle(wStyleIdxLocal);
                if (FormatCodeLooksLikeDate(wFmtCode)) {
                    wIsDate = true;
                }
            }
        }
        if (wIsDate) {
            tInt wNumFmtId = -1;
            tString wFmtCode;
            if (sStyleAttr && *sStyleAttr) {
                const tInt wStyleIdxLocal = std::atoi(sStyleAttr);
                wNumFmtId = sReader.GetNumFmtIdForStyle(wStyleIdxLocal);
                wFmtCode = sReader.GetFormatCodeForStyle(wStyleIdxLocal);
            }
            if (ShouldImportExcelSerialAsTimeOnly(wD, wNumFmtId, wFmtCode)) {
                sOut.SetDate(ExcelTimeOnlySerialToSkDate(wD));
            } else {
                sOut.SetDate(ExcelSerialToTDate(wD));
            }
        } else {
            sOut = tVariant(wD);
        }
        return true;
    }
    sOut = tVariant(sValueStr.c_str());
    return true;
}

} // namespace

namespace {
    // Restore the caller's undo flag even if the import helper returns early.
    class tUndoActifScope {
        tApi& m_Api;
        tBool m_Previous;
    public:
        tUndoActifScope(tApi& sApi, tBool sActive)
            : m_Api(sApi), m_Previous(sApi.IsUndoActif()) {
            m_Api.IsUndoActif(sActive);
        }
        ~tUndoActifScope() { m_Api.IsUndoActif(m_Previous); }
    };
} // namespace

/// @brief Promote worksheet autoFilter / _FilterDatabase to engine RangeData.
void tExcel2SpreadSheet::ProcessWorksheetAutoFilters(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    static const tString kFilterDatabaseName = "_xlnm._FilterDatabase";

    tUndoActifScope wUndoOff(sApi, false);
    for (tInt wS = 1;; ++wS) {
        const tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = sReader.GetUnzippedFile(wSheetFile);
        if (!wItSheet || !wItSheet->HasPayload()) {
            break;
        }

        pugi::xml_document wDoc;
        const tBool wLoaded = wItSheet->ShouldStreamRows()
            ? sReader.LoadWorksheetShell(wSheetFile, wDoc)
            : wDoc.load_buffer(wItSheet->content.data(), wItSheet->content.size());
        if (!wLoaded) {
            continue;
        }

        pugi::xml_node wWorksheet = wDoc.child("worksheet");
        if (!wWorksheet) {
            continue;
        }

        const tString wSheetName =
            (wS - 1) >= 0 && (wS - 1) < (tInt)sReader.GetWorksheetNames().size()
                ? sReader.GetWorksheetNames()[wS - 1]
                : ("sheet" + std::to_string(wS));

        tString wFilterRef;
        if (pugi::xml_node wAutoFilter = wWorksheet.child("autoFilter")) {
            const char* wRefAttr = wAutoFilter.attribute("ref").as_string("");
            if (wRefAttr && *wRefAttr) {
                wFilterRef = tString(wRefAttr);
            }
        }

        auto wDbIt = m_MapRangeNamed.find(kFilterDatabaseName);
        if (wFilterRef.empty() && wDbIt != m_MapRangeNamed.end()
            && wDbIt->second.m_SheetName == wSheetName && !wDbIt->second.m_Ref.empty()) {
            wFilterRef = wDbIt->second.m_Ref;
        }
        if (wFilterRef.empty()) {
            continue;
        }

        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!ParseRange(wFilterRef, wTop, wLeft, wBottom, wRight)) {
            continue;
        }

        if (wBottom <= wTop) {
            const tInt wLastRow =
                LastRowInWorksheetColumns(wWorksheet, (tInt)wLeft, (tInt)wRight);
            if (wLastRow > (tInt)wTop) {
                wBottom = (tIndex)wLastRow;
            }
        }

        if (wBottom <= wTop) {
            continue;
        }
        if (AutoFilterOverlapsStructuredTable(m_TableMetadata, wSheetName, wTop, wLeft, wBottom, wRight)) {
            continue;
        }

        tSheet* wSheet = sApi.Sheet(wSheetName);
        if (wSheet == nullptr) {
            continue;
        }

        const tString wRangeRef = BuildRangeRef(wTop, wLeft, wBottom, wRight);
        tString wRangeName = kFilterDatabaseName;
        if (wDbIt == m_MapRangeNamed.end() || wDbIt->second.m_SheetName != wSheetName) {
            wRangeName = "AutoFilter_" + wSheetName;
        }

#ifdef debugdata
        cout << "ProcessWorksheetAutoFilters: " << wSheetName << " " << wRangeName
             << " -> " << wRangeRef << endl;
#endif
        // Build RangeData JSON from the autofilter header row (0-based column indices).
        std::ostringstream wJsonStream;
        wJsonStream << "{";
        wJsonStream << "\"" << kJsonKeyUseFirstRowAsHeader << "\":true";
        wJsonStream << ",\"" << kJsonKeyColumnDataVector << "\":[";
        tBool wFirstCol = true;
        for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
            if (!wFirstCol) {
                wJsonStream << ",";
            }
            wFirstCol = false;
            const tIndex wColIndex = wCol - wLeft;
            tString wHeaderLabel;
            if (tCell* wHeaderCell = wSheet->Cell(wTop, wCol)) {
                wHeaderLabel = wSheet->WorkBook()->CellFormatString(wHeaderCell);
                if (wHeaderLabel.empty()) {
                    wHeaderLabel = wHeaderCell->Value().Str();
                }
            }
            if (wHeaderLabel.empty()) {
                tStringStream wFallback;
                wFallback << "Column" << (wColIndex + 1);
                wHeaderLabel = wFallback.str();
            }
            const tString wEscapedName = EscapeJsonString(wHeaderLabel);
            wJsonStream << "{";
            wJsonStream << "\"" << kJsonKeyColumnCol << "\":" << wCol << ",";
            wJsonStream << "\"" << kJsonKeyNameData << "\":\"" << wEscapedName << "\",";
            wJsonStream << "\"" << kJsonKeyTypeData << "\":\"Text\",";
            wJsonStream << "\"" << kJsonKeyFilterOperator << "\":\"None\",";
            wJsonStream << "\"" << kJsonKeySortOrder << "\":\"None\"";
            wJsonStream << "}";
        }
        wJsonStream << "]}";
        sApi.EnsureRangeData(wRangeName, wJsonStream.str(), wTop, wLeft, wBottom, wRight, wSheet);
        m_MapRangeNamed.erase(kFilterDatabaseName);
    }
}

/// @brief Project table style formatting onto cell-level CSS.
static tString ExtractAndRemoveCssProp(tString& ioCss, const tString& sKey);

void tExcel2SpreadSheet::ApplyTableStyleOverlays(const tExcelPugiXMLReader& sReader, tApi& sApi) {
#ifdef debugtablestyle
    std::cout << "[TableStyleOverlay] table count=" << m_TableMetadata.size() << std::endl;
#endif
    if (m_TableMetadata.empty()) return;

    tWorkBook* wWorkBook = sApi.ActiveWorkBook();
    tTableStyleContainer* wTableStyles =
        wWorkBook != nullptr ? wWorkBook->TableStyleContainer() : nullptr;
    if (wTableStyles != nullptr) {
        wTableStyles->EnsureBuiltins();
    }

    static const char* const kTableStyleElementTypes[] = {
        TableStyleElement::kWholeTable,
        TableStyleElement::kHeaderRow,
        TableStyleElement::kTotalRow,
        TableStyleElement::kFirstRowStripe,
        TableStyleElement::kSecondRowStripe,
        TableStyleElement::kFirstColumnStripe,
        TableStyleElement::kSecondColumnStripe,
        TableStyleElement::kFirstColumn,
        TableStyleElement::kLastColumn,
    };

    // OOXML <tableStyles> dxfs first; built-ins via tTableStyleContainer.
    auto wResolveElementCss = [&](const tString& sStyleName, const tString& sElementType) -> tString {
        if (sStyleName.empty()) return "";
        tInt wDxfId = sReader.GetTableStyleElementDxfId(sStyleName, sElementType);
        if (wDxfId >= 0) {
            tString wCss;
            wCss += sReader.BuildDxfBackgroundCss(wDxfId);
            wCss += sReader.BuildDxfFontCss(wDxfId);
            wCss += sReader.BuildDxfBordersCss(wDxfId);
            return wCss;
        }
        tString wBuiltinCss = sReader.BuildBuiltinTableStyleElementCss(sStyleName, sElementType);
        if (!wBuiltinCss.empty()) {
            return wBuiltinCss;
        }
        if (wTableStyles != nullptr) {
            return wTableStyles->ElementCssString(sStyleName, sElementType);
        }
        return "";
    };

    auto wStyleHasCustomDxfs = [&](const tString& sStyleName) -> tBool {
        for (const char* wType : kTableStyleElementTypes) {
            if (sReader.GetTableStyleElementDxfId(sStyleName, wType) >= 0) {
                return true;
            }
        }
        return false;
    };

    // Append overlay CSS to a target cell, but only for properties the
    // cell does not already declare. Excel's precedence rule is "direct
    // cell formatting beats table style"; ProcessSheet (which writes
    // cellXfs CSS) runs *after* this function, so if we append properties
    // that cellXfs will also write, the later concatenation would still
    // win in CSS terms. The CssHasProp guard is defensive: ProcessSheet
    // can short-circuit on an unstyled cell, in which case the overlay
    // we wrote here is the only style and must remain visible.
    auto wAppendNonOverriding = [&](tCellAddr sAddr, const tString& sOverlay) {
        if (sOverlay.empty()) return;
        m_MapCss[sAddr] += sOverlay;
    };

    for (const auto& wEntry : m_TableMetadata) {
        const tTableMetadata& wMeta = wEntry.second;
#ifdef debugtablestyle
        std::cout << "[TableStyleOverlay] table=" << wMeta.m_Name
                  << " sheet=" << wMeta.mSheetName
                  << " ref=" << wMeta.mRef
                  << " styleName='" << wMeta.m_StyleName << "'"
                  << " stripes=" << (wMeta.m_ShowRowStripes ? 1 : 0)
                  << " hdrCount=" << wMeta.m_HeaderRowCount
                  << " totCount=" << wMeta.m_TotalsRowCount << std::endl;
#endif
        if (wMeta.m_StyleName.empty()) continue;
        if (wMeta.mRef.empty() || wMeta.mSheetName.empty()) continue;

        tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
        if (!ParseRange(wMeta.mRef, wTop, wLeft, wBottom, wRight)) continue;

        // OOXML: when totalsRowCount > 0, the ref already includes the totals row.
        tInt wHeaderRow = (wMeta.m_HeaderRowCount > 0) ? wTop : -1;
        tInt wTotalsRow = (wMeta.m_TotalsRowCount > 0) ? wBottom : -1;
        tInt wDataTop   = (wHeaderRow >= 0) ? (wTop + wMeta.m_HeaderRowCount) : wTop;
        tInt wDataBot   = (wTotalsRow >= 0) ? (wBottom - wMeta.m_TotalsRowCount) : wBottom;

        // Pre-resolve CSS fragments once per table to avoid the per-cell
        // dxf lookup overhead.
        tString wCssWhole       = wResolveElementCss(wMeta.m_StyleName, "wholeTable");
        tString wCssHeader      = wResolveElementCss(wMeta.m_StyleName, "headerRow");
        tString wCssTotals      = wResolveElementCss(wMeta.m_StyleName, "totalsRow");
        tString wCssStripeFirst = wResolveElementCss(wMeta.m_StyleName, "firstRowStripe");
        tString wCssStripeSecond= wResolveElementCss(wMeta.m_StyleName, "secondRowStripe");
        tString wCssFirstCol    = wResolveElementCss(wMeta.m_StyleName, "firstColumn");
        tString wCssLastCol     = wResolveElementCss(wMeta.m_StyleName, "lastColumn");
#ifdef debugtablestyle
        std::cout << "[TableStyleOverlay]   header='"  << wCssHeader << "'\n"
                  << "[TableStyleOverlay]   stripe2='" << wCssStripeSecond << "'\n"
                  << "[TableStyleOverlay]   whole='"   << wCssWhole << "'" << std::endl;
#endif

        // OOXML stripe convention: data rows alternate first/second
        // starting with "first" on the topmost data row. We also expose
        // the convention through showRowStripes — when false, no stripe
        // overlay is applied even if the style defines one.
        for (tIndex wR = wTop; wR <= wBottom; ++wR) {
            for (tIndex wC = wLeft; wC <= wRight; ++wC) {
                tString wRef = Base10ToAlpha(wC) + std::to_string(wR);
                tCellAddr wAddr = std::make_tuple(wMeta.mSheetName, wRef);
                tString wOverlay;
                if (wR == wHeaderRow) {
                    // wholeTable often sets a white fill; keep its borders but let headerRow own bg/font.
                    tString wWholeForHeader = wCssWhole;
                    ExtractAndRemoveCssProp(wWholeForHeader, "background-color");
                    ExtractAndRemoveCssProp(wWholeForHeader, "color");
                    wOverlay += wWholeForHeader;
                    wOverlay += wCssHeader;
                } else if (wR == wTotalsRow) {
                    tString wWholeForTotals = wCssWhole;
                    ExtractAndRemoveCssProp(wWholeForTotals, "background-color");
                    ExtractAndRemoveCssProp(wWholeForTotals, "color");
                    wOverlay += wWholeForTotals;
                    wOverlay += wCssTotals;
                } else {
                    wOverlay += wCssWhole;
                }
                if (wR != wHeaderRow && wR != wTotalsRow) {
                    if (wR >= wDataTop && wR <= wDataBot) {
                        if (wMeta.m_ShowRowStripes) {
                            // Distance from first data row decides which stripe applies.
                            tInt wStripeIdx = (wR - wDataTop);
                            if ((wStripeIdx % 2) == 0) {
                                wOverlay += wCssStripeFirst;
                            } else {
                                wOverlay += wCssStripeSecond;
                            }
                        }
                        if (wMeta.m_ShowColumnStripes) {
                            // Symmetric column striping; rare in practice but
                            // cheap to emit when the workbook asks for it.
                            tInt wColStripeIdx = (wC - wLeft);
                            if ((wColStripeIdx % 2) == 0) {
                                wOverlay += wResolveElementCss(wMeta.m_StyleName, "firstColumnStripe");
                            } else {
                                wOverlay += wResolveElementCss(wMeta.m_StyleName, "secondColumnStripe");
                            }
                        }
                        if (wMeta.m_ShowFirstColumn && wC == wLeft) {
                            wOverlay += wCssFirstCol;
                        }
                        if (wMeta.m_ShowLastColumn && wC == wRight) {
                            wOverlay += wCssLastCol;
                        }
                    }
                }
                wAppendNonOverriding(wAddr, wOverlay);
            }
        }

        if (wWorkBook != nullptr) {
            tRangeData* wRangeData = wWorkBook->RangeData(wMeta.m_Name);
            if (wRangeData != nullptr) {
                wRangeData->TableStyleName(wMeta.m_StyleName);
                wRangeData->TableShowRowStripes(wMeta.m_ShowRowStripes);
                wRangeData->TableShowColumnStripes(wMeta.m_ShowColumnStripes);
                wRangeData->TableShowFirstColumn(wMeta.m_ShowFirstColumn);
                wRangeData->TableShowLastColumn(wMeta.m_ShowLastColumn);

                std::map<tString, tString> wElements;
                auto wPersistElement = [&](const tString& sType, const tString& sCss) {
                    if (sCss.empty()) {
                        return;
                    }
                    wElements[sType] = sCss;
                    wRangeData->SetTableStyleElementCss(sType, sCss);
                };
                for (const char* wType : kTableStyleElementTypes) {
                    wPersistElement(wType, wResolveElementCss(wMeta.m_StyleName, wType));
                }
                // Custom styles (e.g. Calculateur de prêt immobilier) often omit headerRow
                // in <tableStyles>; capture the header row's resolved cell xfs instead.
                if (wHeaderRow >= 0
                    && sReader.GetTableStyleElementDxfId(wMeta.m_StyleName, TableStyleElement::kHeaderRow) < 0
                    && wElements.find(TableStyleElement::kHeaderRow) == wElements.end()) {
                    const tString wHeaderRef =
                        Base10ToAlpha(wLeft) + std::to_string(wHeaderRow);
                    const tCellAddr wHeaderAddr = std::make_tuple(wMeta.mSheetName, wHeaderRef);
                    auto wHeaderIt = m_MapCss.find(wHeaderAddr);
                    if (wHeaderIt != m_MapCss.end() && !wHeaderIt->second.empty()) {
                        tString wSrc = wHeaderIt->second;
                        tString wHeaderCss;
                        auto wTake = [&](const tString& sKey) {
                            const tString wValue = ExtractAndRemoveCssProp(wSrc, sKey);
                            if (!wValue.empty()) {
                                wHeaderCss += sKey + ":" + wValue + ";";
                            }
                        };
                        wTake("background-color");
                        wTake("color");
                        const tString wWeight = ExtractAndRemoveCssProp(wSrc, "font-weight");
                        if (wWeight == "bold") {
                            wHeaderCss += "font-weight:bold;";
                        }
                        wPersistElement(TableStyleElement::kHeaderRow, wHeaderCss);
                    }
                }
                if (wTableStyles != nullptr && wStyleHasCustomDxfs(wMeta.m_StyleName)
                    && !wElements.empty()) {
                    wTableStyles->RegisterFromElementCss(wMeta.m_StyleName, wElements);
                }
            }
        }
    }
}

void tExcel2SpreadSheet::ImportSheetPrintSettings(const pugi::xml_node& sWorksheet, tSheet* sSheet)
{
    if (!sSheet || !sWorksheet) {
        return;
    }
    tPrintParameters pp;

    if (pugi::xml_node wSpr = sWorksheet.child("sheetPr")) {
        if (pugi::xml_node wPsu = wSpr.child("pageSetUpPr")) {
            pp.m_FitToPage = wPsu.attribute("fitToPage").as_bool(false);
            if (wPsu.attribute("autoPageBreaks")) {
                pp.m_AutoPageBreaks = wPsu.attribute("autoPageBreaks").as_bool(true);
            }
        }
    }

    if (pugi::xml_node wPs = sWorksheet.child("pageSetup")) {
        if (wPs.attribute("paperSize")) {
            pp.m_PaperSize = wPs.attribute("paperSize").as_int(pp.m_PaperSize);
        }
        const char* wOrient = wPs.attribute("orientation").value();
        if (wOrient != nullptr && wOrient[0] != '\0') {
            if (std::strcmp(wOrient, "landscape") == 0) {
                pp.m_Orientation = tPrintOrientation::Landscape;
            } else {
                pp.m_Orientation = tPrintOrientation::Portrait;
            }
        }
        if (wPs.attribute("scale")) {
            pp.m_Scale = wPs.attribute("scale").as_int(pp.m_Scale);
        }
        if (wPs.attribute("fitToWidth")) {
            pp.m_FitToWidthPages = wPs.attribute("fitToWidth").as_int(pp.m_FitToWidthPages);
        }
        if (wPs.attribute("fitToHeight")) {
            pp.m_FitToHeightPages = wPs.attribute("fitToHeight").as_int(pp.m_FitToHeightPages);
        }
        const char* wOrder = wPs.attribute("pageOrder").value();
        if (wOrder != nullptr && wOrder[0] != '\0') {
            if (std::strcmp(wOrder, "overThenDown") == 0) {
                pp.m_PageOrder = tPrintPageOrder::OverThenDown;
            } else {
                pp.m_PageOrder = tPrintPageOrder::DownThenOver;
            }
        }
        if (wPs.attribute("horizontalDpi")) {
            pp.m_HorizontalDpi = wPs.attribute("horizontalDpi").as_int(pp.m_HorizontalDpi);
        }
        if (wPs.attribute("verticalDpi")) {
            pp.m_VerticalDpi = wPs.attribute("verticalDpi").as_int(pp.m_VerticalDpi);
        }
        {
            auto wClampDpi = [](tInt& io) {
                constexpr tInt kDefault = 600;
                constexpr tInt kMin = 72;
                constexpr tInt kMax = 9600;
                if (io < kMin || io > kMax) {
                    io = kDefault;
                }
            };
            wClampDpi(pp.m_HorizontalDpi);
            wClampDpi(pp.m_VerticalDpi);
        }
        if (wPs.attribute("copies")) {
            pp.m_Copies = wPs.attribute("copies").as_int(pp.m_Copies);
        }
        if (wPs.attribute("blackAndWhite")) {
            pp.m_BlackAndWhite = wPs.attribute("blackAndWhite").as_bool(false);
        }
        if (wPs.attribute("draft")) {
            pp.m_Draft = wPs.attribute("draft").as_bool(false);
        }
        if (wPs.attribute("firstPageNumber")) {
            pp.m_FirstPageNumber = wPs.attribute("firstPageNumber").as_int(pp.m_FirstPageNumber);
        }
        if (wPs.attribute("usePrinterDefaults")) {
            pp.m_UsePrinterDefaults = wPs.attribute("usePrinterDefaults").as_bool(pp.m_UsePrinterDefaults);
        }
    }

    if (pugi::xml_node wPm = sWorksheet.child("pageMargins")) {
        if (wPm.attribute("left")) {
            pp.m_MarginLeft = wPm.attribute("left").as_double(pp.m_MarginLeft);
        }
        if (wPm.attribute("right")) {
            pp.m_MarginRight = wPm.attribute("right").as_double(pp.m_MarginRight);
        }
        if (wPm.attribute("top")) {
            pp.m_MarginTop = wPm.attribute("top").as_double(pp.m_MarginTop);
        }
        if (wPm.attribute("bottom")) {
            pp.m_MarginBottom = wPm.attribute("bottom").as_double(pp.m_MarginBottom);
        }
        if (wPm.attribute("header")) {
            pp.m_MarginHeader = wPm.attribute("header").as_double(pp.m_MarginHeader);
        }
        if (wPm.attribute("footer")) {
            pp.m_MarginFooter = wPm.attribute("footer").as_double(pp.m_MarginFooter);
        }
    }

    if (pugi::xml_node wPrtOpts = sWorksheet.child("printOptions")) {
        if (wPrtOpts.attribute("gridLines")) {
            pp.m_PrintGridLines = wPrtOpts.attribute("gridLines").as_bool(false);
        }
        if (wPrtOpts.attribute("headings")) {
            pp.m_PrintHeadings = wPrtOpts.attribute("headings").as_bool(false);
        }
        if (wPrtOpts.attribute("horizontalCentered")) {
            pp.m_HorizontalCentered = wPrtOpts.attribute("horizontalCentered").as_bool(false);
        }
        if (wPrtOpts.attribute("verticalCentered")) {
            pp.m_VerticalCentered = wPrtOpts.attribute("verticalCentered").as_bool(false);
        }
    }

    sSheet->PrintParametersImport(pp);
}

/// @brief Process a single sheet (merged cells, column widths, row heights, conditional formatting, cells)
void tExcel2SpreadSheet::ProcessSheet(const tExcelPugiXMLReader& sReader, tInt sSheetIndex, tApi& sApi) {
    tString wSheetName="";
    // Active Sheet By Name
    if (sSheetIndex-1 < (tInt)sReader.GetWorksheetNames().size()) {
        wSheetName=sReader.GetWorksheetNames()[sSheetIndex-1];
        if (!sApi.ActiveSheet(wSheetName)) {
            tStringStream wStream;
            wStream << "SetActiveSheet(" << wSheetName << ") Error !'";
            cerr << wStream.str() << endl;
       }
    }
    // Load the sheet XML (full DOM for small sheets; shell + streamed rows for spilled ones).
    tString wSheetFile = "xl/worksheets/sheet" + std::to_string(sSheetIndex) + ".xml";
    auto wUnzippedFile = sReader.GetUnzippedFile(wSheetFile);
    if (!wUnzippedFile || !wUnzippedFile->HasPayload()) {
        std::cerr << "SkExcel: missing " << wSheetFile << " after unzip (sheet will stay empty)" << std::endl;
        return;
    }

    pugi::xml_document wDoc;
    const tBool wStreamRows = wUnzippedFile->ShouldStreamRows();
    if (wStreamRows) {
        if (!sReader.LoadWorksheetShell(wSheetFile, wDoc)) {
            std::cerr << "SkExcel: failed to load worksheet shell for " << wSheetFile
                      << " (" << wUnzippedFile->size << " bytes)" << std::endl;
            return;
        }
        std::cerr << "SkExcel: streaming rows for " << wSheetFile
                  << " (" << wUnzippedFile->size << " bytes)" << std::endl;
    } else if (!wDoc.load_buffer(wUnzippedFile->content.data(), wUnzippedFile->content.size())) {
        std::cerr << "SkExcel: failed to parse " << wSheetFile
                  << " (" << wUnzippedFile->content.size() << " bytes)" << std::endl;
        return;
    }
    const auto forEachRow = [&](const std::function<void(pugi::xml_node)>& sFn) {
        if (wStreamRows) {
            sReader.ForEachWorksheetRow(wSheetFile, sFn);
        } else {
            for (pugi::xml_node wRow : wDoc.child("worksheet").child("sheetData").children("row")) {
                sFn(wRow);
            }
        }
    };

    if (pugi::xml_node wWsRoot = wDoc.child("worksheet")) {
        if (tSheet* wTargetSheet = sApi.ActiveSheet()) {
            ImportSheetPrintSettings(wWsRoot, wTargetSheet);
            if (pugi::xml_node wSheetViews = wWsRoot.child("sheetViews")) {
                for (pugi::xml_node wSheetView : wSheetViews.children("sheetView")) {
                    if (wSheetView.attribute("zoomScaleNormal")) {
                        wTargetSheet->ViewZoomScaleNormal(
                            wSheetView.attribute("zoomScaleNormal").as_int(100));
                    }
                    if (wSheetView.attribute("showGridLines")) {
                        wTargetSheet->ShowGridLines(
                            wSheetView.attribute("showGridLines").as_bool(true));
                    }
                    break;
                }
            }
        }
    }

    // Merged cells
#ifdef debugmerge
    tInt wMergeCount = 0;
#endif
    if (pugi::xml_node wMerges = wDoc.child("worksheet").child("mergeCells")) {
        for (pugi::xml_node wMc : wMerges.children("mergeCell")) {
            const char* wRange = wMc.attribute("ref").value();
            if (wRange && *wRange) {
                tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
                if (ParseRange(tString(wRange), wTop, wLeft, wBottom, wRight)) {
                    sApi.EnsureMergedRange(wTop, wLeft, wBottom, wRight);
                    // Stash the range for the later border adjacency pass
                    // (Excel stores the merge perimeter on the anchor; our
                    // rendering model needs border-right / border-bottom
                    // pushed onto the neighbor column / row).
                    m_MergedRanges.push_back(std::make_tuple(wSheetName, wTop, wLeft, wBottom, wRight));
#ifdef debugmerge
                    ++wMergeCount;
#endif
                }
            }
        }
    }
#ifdef debugmerge
    if (wMergeCount > 0) {
        std::cout << "Sheet " << sSheetIndex << ": merged ranges = " << wMergeCount << std::endl;
    }
#endif
    // Used extent for col/row metadata: dimension, merges, and sheetData cells.
    tSheetUsedExtent wUsedExtent;
    if (pugi::xml_node wDim = wDoc.child("worksheet").child("dimension")) {
        const char* wRef = wDim.attribute("ref").value();
        if (wRef && *wRef) {
            tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
            if (ParseRange(tString(wRef), wTop, wLeft, wBottom, wRight)) {
                MergeUsedExtent(wUsedExtent, wRight, wBottom);
            }
        }
    }
    for (const auto& wMerge : m_MergedRanges) {
        if (std::get<0>(wMerge) == wSheetName) {
            MergeUsedExtent(wUsedExtent, std::get<4>(wMerge), std::get<3>(wMerge));
        }
    }
    if (!wStreamRows) {
        const tSheetUsedExtent wDataExtent = ScanSheetDataUsedExtent(wDoc);
        MergeUsedExtent(wUsedExtent, wDataExtent.m_RightCol, wDataExtent.m_BottomRow);
    } else if (wUsedExtent.m_RightCol > 1024 || wUsedExtent.m_BottomRow >= 1048576) {
        // Full-grid Excel dimension would make UndoSizeCol/Row explode; grow from cells instead.
        wUsedExtent.m_RightCol = 0;
        wUsedExtent.m_BottomRow = 0;
    }
    const tInt wSheetRightCol = wUsedExtent.m_RightCol;

    // Empirical calibration knob for column widths on the SkSpreadSheet canvas.
    // Column widths: ExcelColWidthKScale() / ExcelColWidthCharsToSkMm() in SkExcelTools (1.0 = 1:1 px).
    tInt wColMdw = EstimateMaxDigitWidthPx(sReader);

    // Excel default row height is 15 pt (Calibri 11). The sheet may override
    // it via <sheetFormatPr defaultRowHeight="..."> with optional customHeight=1.
    // We remember it so that any row that does NOT carry an explicit "ht"
    // attribute later inherits the sheet default (otherwise SkSpreadSheet would
    // fall back to its own internal default and rows look cropped).
    tDouble wSheetDefaultRowHtPt = 0.0;
    // Default column width in CHARACTERS, resolved from <sheetFormatPr> using
    // the OOXML precedence:
    //   1. `defaultColWidth` if present (already in chars).
    //   2. otherwise `baseColWidth` (default 10 if absent), which Excel uses
    //      to compute the per-column default width. We approximate that with
    //      `baseColWidth + 1` to mirror the small padding Excel adds (the
    //      Microsoft formula is more involved but the approximation is close
    //      enough for our visual calibration). Without this, sheets that
    //      omit `defaultColWidth` fall back to SkSpreadSheet's internal
    //      static default and look completely out of proportion vs. the
    //      explicitly-sized columns from <cols>.
    tDouble wSheetDefaultColWidthChars = 0.0;
    pugi::xml_node wWs = ChildByLocalName(wDoc, "worksheet");
    if (!wWs) {
        wWs = wDoc.child("worksheet");
    }
    if (pugi::xml_node wSfp = ChildByLocalName(wWs, "sheetFormatPr")) {
        tDouble wDefaultColWidthChars = wSfp.attribute("defaultColWidth").as_double(0.0);
        if (wDefaultColWidthChars > 0.0) {
            wSheetDefaultColWidthChars = wDefaultColWidthChars;
        } else {
            tDouble wBase = wSfp.attribute("baseColWidth").as_double(10.0);
            if (wBase > 0.0) {
                wSheetDefaultColWidthChars = wBase + 1.0;
            }
        }
        // Capture the sheet-wide default row height when the workbook explicitly
        // departs from the 15 pt baseline. We honor `defaultRowHeight` whether
        // or not `customHeight` is set: Excel itself displays the value as soon
        // as it differs from the standard.
        tDouble wDrh = wSfp.attribute("defaultRowHeight").as_double(0.0);
        if (wDrh > 0.0) {
            wSheetDefaultRowHtPt = wDrh;
        }
    }
    if (tWorkBook* wWorkBook = sApi.ActiveWorkBook()) {
        if (wSheetDefaultRowHtPt > 0.0) {
            wWorkBook->DefaultSizeRow(ExcelRowHeightPtToSkMm(wSheetDefaultRowHtPt));
        }
        if (wSheetDefaultColWidthChars > 0.0) {
            wWorkBook->DefaultSizeCol(ExcelColWidthCharsToSkMm(wSheetDefaultColWidthChars, wColMdw));
        }
    }
    // Apply the resolved default column width to the whole used range so
    // that columns without an explicit <col> entry render at the same scale
    // as those that have one.
    if (wSheetDefaultColWidthChars > 0.0 && wSheetRightCol > 0) {
        sApi.UndoSizeCol(1, wSheetRightCol, ExcelColWidthCharsToSkMm(wSheetDefaultColWidthChars, wColMdw));
    }
    if (wSheetDefaultRowHtPt <= 0.0) {
        wSheetDefaultRowHtPt = 15.0;
        if (tWorkBook* wWorkBook = sApi.ActiveWorkBook()) {
            wWorkBook->DefaultSizeRow(ExcelRowHeightPtToSkMm(wSheetDefaultRowHtPt));
        }
    }

    // Non-standard column widths: Excel OOXML stores `width` in character
    // units. As with row heights, the `customWidth` attribute is sometimes
    // absent even though Excel does honor the `width` value at display time
    // (e.g. <col style="3" width="14.4"/> for an auto-fit column carrying a
    // style). We therefore apply `width` whenever it is present, just like
    // we now do for `ht` on rows.
    if (pugi::xml_node wCols = wDoc.child("worksheet").child("cols")) {
        for (pugi::xml_node wCol : wCols.children("col")) {
            tInt wCmin = wCol.attribute("min").as_int(0);
            tInt wCmax = wCol.attribute("max").as_int(0);
            pugi::xml_attribute wWidthAttr = wCol.attribute("width");
            const tBool wColHidden = wCol.attribute("hidden").as_bool(false);
            if (wWidthAttr && wCmin > 0 && wCmax >= wCmin) {
                tDouble wW = wWidthAttr.as_double(0.0);  // Excel: character units
                if (wW > 0.0) {
                    const tInt wApplyMax = ColStyleApplyMax(wCmin, wCmax, wSheetRightCol);
                    if (wApplyMax >= wCmin) {
                        sApi.UndoSizeCol(wCmin, wApplyMax, ExcelColWidthCharsToSkMm(wW, wColMdw));
                    }
                }
            }
            // Excel hidden="1": Sker hides a col/row when Size==0 (tColRow::IsVisible).
            if (wColHidden && wCmin > 0 && wCmax >= wCmin) {
                const tInt wApplyMax = ColStyleApplyMax(wCmin, wCmax, wSheetRightCol);
                if (wApplyMax >= wCmin) {
                    sApi.UndoSizeCol(wCmin, wApplyMax, 0.0);
                }
            }
            // Recover column default format (Excel OOXML: <col style="index"/>); one call per column via ColSelect(sIndex)
            const char* wStyleAttr = wCol.attribute("style").value();
            if (wStyleAttr && *wStyleAttr && wCmin > 0 && wCmax >= wCmin) {
                tInt wStyleIdx = std::atoi(wStyleAttr);
                tString wCss = sReader.BuildCssForStyle(wStyleIdx);
                if (!wCss.empty()) {
                    const tInt wApplyMax = ColStyleApplyMax(wCmin, wCmax, wSheetRightCol);
                    if (wApplyMax >= wCmin) {
                        for (tInt wColIdx = wCmin; wColIdx <= wApplyMax; ++wColIdx) {
                            tRect wRect;
                            wRect.ColSelect(wColIdx);
                            tString wColRef = wRect.StrRef();
                            tCellAddr wAddr = std::make_tuple(wSheetName, wColRef);
                            m_MapCss[wAddr] = wCss;
                        }
                    }
                }
            }
        }
    }

    // Non-standard row heights: Excel OOXML stores "ht" in points; we convert
    // to mm for the API. Applied in the single sheetData pass below, alongside
    // the cells (ApplyExcelRowSize). Rows without "ht" are filled afterwards
    // with the sheet defaultRowHeight (FillUnsetExcelRowHeights).

    // Collect all conditional formatting rules (generalized for export to SkSpreadSheet)
    struct ExcelCfRule {
        tString type; tString sqref; tInt dxfId; tInt priority; tString formula; tString formula2; tString operator_;
        // dataBar
        tString dataBarMinVal, dataBarMaxVal, dataBarColor;
        // colorScale (2 or 3 stops)
        std::vector<tString> colorScaleVals, colorScaleColors;
        // iconSet
        tString iconSetName;
        std::vector<tString> iconSetVals;
        std::vector<tString> iconSetCfvoTypes;
    };

    auto splitSqrefAreas = [](const tString& sSqref) -> std::vector<tString> {
        std::vector<tString> wAreas;
        tString wToken;
        for (tSize i = 0; i <= sSqref.size(); ++i) {
            const char wCh = (i < sSqref.size()) ? sSqref[i] : ';';
            if (wCh == ';' || wCh == ' ') {
                if (!wToken.empty()) {
                    wAreas.push_back(wToken);
                    wToken.clear();
                }
            } else {
                wToken += wCh;
            }
        }
        return wAreas;
    };

    auto joinCommaList = [](const std::vector<tString>& sParts) -> tString {
        tString wOut;
        for (tSize i = 0; i < sParts.size(); ++i) {
            if (i > 0) {
                wOut += ",";
            }
            wOut += sParts[i];
        }
        return wOut;
    };

    auto mapExcelIconSetNameToSkIconType = [](const tString& sIconSetName) -> tString {
        // Prefer exact OOXML names — "3Signs" must not be lumped with "3Symbols2".
        if (sIconSetName == "3Symbols" || sIconSetName == "3Symbols2"
            || sIconSetName == "5Quarters" || sIconSetName == "4RedToBlack") {
            return "Shapes";
        }
        if (sIconSetName.find("Symbol") != tString::npos && sIconSetName.find("Signs") == tString::npos) {
            return "Shapes";
        }
        if (sIconSetName == "3Signs" || sIconSetName.find("TrafficLight") != tString::npos) {
            return "Indicators";
        }
        if (sIconSetName.find("Arrows") != tString::npos) {
            return "Arrows";
        }
        if (sIconSetName.find("Flags") != tString::npos) {
            return "Flags";
        }
        if (sIconSetName.find("Rating") != tString::npos || sIconSetName == "5Boxes") {
            return "Ratings";
        }
        return "Arrows";
    };
    std::vector<ExcelCfRule> wExcelCfRules;
    auto collectCfRules = [&](pugi::xml_node wCfsNode) {
        tString wSqref = wCfsNode.attribute("sqref").as_string("");
        if (wSqref.empty()) return;
#ifdef debugcondformat
        std::cerr << "[CF] conditionalFormatting sqref=\"" << wSqref << "\"" << std::endl;
#endif
        for (pugi::xml_node wR : wCfsNode.children("cfRule")) {
            ExcelCfRule wRule;
            wRule.type = wR.attribute("type").as_string("");
            wRule.sqref = wSqref;
            wRule.priority = wR.attribute("priority").as_int(0);
#ifdef debugcondformat
            std::cerr << "[CF]   cfRule type(attr)=\"" << wRule.type << "\"";
            for (pugi::xml_node c : wR.children()) {
                const char* n = c.name();
                std::cerr << " child=\"" << (n ? n : "(null)") << "\"";
            }
            std::cerr << std::endl;
#endif
            wRule.dxfId = wR.attribute("dxfId").as_int(-1);
            wRule.operator_ = wR.attribute("operator").as_string("");
            wRule.formula.clear();
            wRule.formula2.clear();
            wRule.dataBarMinVal.clear();
            wRule.dataBarMaxVal.clear();
            wRule.dataBarColor.clear();
            wRule.colorScaleVals.clear();
            wRule.colorScaleColors.clear();
            wRule.iconSetName.clear();
            wRule.iconSetVals.clear();
            wRule.iconSetCfvoTypes.clear();
            // Infer type from child node when type attribute is wrong (e.g. "expression" with <iconSet> child in Tableau_des_flux)
            auto childNameContains = [](pugi::xml_node parent, const char* name) {
                tSize nameLen = std::strlen(name);
                if (nameLen == 0) return false;
                auto toLower = [](char ch) { return (char)std::tolower((unsigned char)ch); };
                for (pugi::xml_node c : parent.children()) {
                    const char* n = c.name();
                    if (!n) continue;
                    // Match local part (after '}' in Clark notation) or full name, case-insensitive
                    const char* local = std::strchr(n, '}');
                    const char* search = local ? local + 1 : n;
                    for (tSize i = 0; search[i] && (std::strlen(search + i) >= nameLen); ++i) {
                        tBool match = true;
                        for (tSize k = 0; k < nameLen; ++k)
                            if (toLower(search[i + k]) != toLower(name[k])) { match = false; break; }
                        if (match) return true;
                    }
                }
                return false;
            };
            if (wR.child("iconSet") || childNameContains(wR, "iconSet")) {
                wRule.type = "iconSet";
            } else if ((wR.child("dataBar") || childNameContains(wR, "dataBar")) && wRule.type.empty()) {
                wRule.type = "dataBar";
            } else if ((wR.child("colorScale") || childNameContains(wR, "colorScale")) && wRule.type.empty()) {
                wRule.type = "colorScale";
            }
#ifdef debugcondformat
            std::cerr << "[CF]     -> inferred type=\"" << wRule.type << "\"" << std::endl;
#endif
            tInt wFi = 0;
            for (pugi::xml_node wF : wR.children("formula")) {
                tString wVal = wF.child_value();
                if (wFi == 0) wRule.formula = wVal; else if (wFi == 1) wRule.formula2 = wVal;
                ++wFi;
            }
            // Parse dataBar
            if (wRule.type == "dataBar") {
                pugi::xml_node wDb = wR.child("dataBar");
                if (wDb) {
                    tInt wCi = 0;
                    for (pugi::xml_node wCfvo : wDb.children("cfvo")) {
                        // OOXML cfvo with type="min" / type="max" represents Excel's
                        // "automatic" bound: no 'val' attribute, the bar should scale
                        // against the actual data range. Storing "0" here would be
                        // forwarded to tConditionalFormat::CallBackCell as Param4/Param5,
                        // making both bounds 0 and tripping drawDataBars()'s
                        // minValue===maxValue early-return. Leave the field empty so
                        // CallBackCell falls back to m_CalculatedMinValue/MaxValue.
                        tString wType = wCfvo.attribute("type").as_string("");
                        tString wVal;
                        if (wType != "min" && wType != "max") {
                            wVal = wCfvo.attribute("val").as_string("");
                        }
                        if (wCi == 0) wRule.dataBarMinVal = wVal; else if (wCi == 1) wRule.dataBarMaxVal = wVal;
                        ++wCi;
                    }
                    if (pugi::xml_node wColor = wDb.child("color")) {
                        tString wHex = sReader.ResolveColorFromXmlNode(wColor);
                        if (!wHex.empty()) wRule.dataBarColor = wHex;
                    }
                }
            }
            // Parse colorScale
            else if (wRule.type == "colorScale") {
                pugi::xml_node wCs = wR.child("colorScale");
                if (wCs) {
                    for (pugi::xml_node wCfvo : wCs.children("cfvo")) {
                        wRule.colorScaleVals.push_back(wCfvo.attribute("val").as_string("0"));
                    }
                    for (pugi::xml_node wColor : wCs.children("color")) {
                        tString wHex = sReader.ResolveColorFromXmlNode(wColor);
                        wRule.colorScaleColors.push_back(wHex.empty() ? tString("FFFFFF") : wHex);
                    }
                }
            }
            // Parse iconSet (either type was "iconSet" or inferred from child <iconSet>)
            if (wRule.type == "iconSet") {
                pugi::xml_node wIs = wR.child("iconSet");
                if (!wIs) {
                    auto nodeNameMatches = [](const char* n, const char* name) {
                        if (!n || !name) return false;
                        const char* local = std::strchr(n, '}');
                        const char* s = local ? local + 1 : n;
                        tSize len = std::strlen(name);
                        if (std::strlen(s) != len) return false;
                        for (tSize i = 0; i < len; ++i)
                            if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)name[i])) return false;
                        return true;
                    };
                    for (pugi::xml_node c : wR.children()) {
                        if (nodeNameMatches(c.name(), "iconSet")) { wIs = c; break; }
                    }
                }
                if (wIs) {
                    // ECMA-376 §18.3.1.49: when the iconSet attribute is omitted,
                    // the default icon set is "3TrafficLights1" (mapped to "Indicators"),
                    // not "3Arrows". Excel's "Conditional Formatting > Icon Sets" wizard
                    // emits a bare <iconSet> for traffic lights, so we must honor that.
                    wRule.iconSetName = wIs.attribute("iconSet").as_string("3TrafficLights1");
                    if (wRule.iconSetName.empty()) wRule.iconSetName = wIs.attribute("iconSetValue").as_string("3TrafficLights1");
                    for (pugi::xml_node wCfvo : wIs.children("cfvo")) {
                        wRule.iconSetVals.push_back(wCfvo.attribute("val").as_string("0"));
                        wRule.iconSetCfvoTypes.push_back(wCfvo.attribute("type").as_string("percent"));
                    }
#ifdef debugcondformat
                    std::cerr << "[CF]     iconSet parsed: iconSetName=\"" << wRule.iconSetName << "\" cfvoCount=" << wRule.iconSetVals.size() << std::endl;
#endif
                } else {
#ifdef debugcondformat
                    if (wRule.type == "iconSet") std::cerr << "[CF]     iconSet type but no iconSet node found" << std::endl;
#endif
                }
            }
            wExcelCfRules.push_back(wRule);
        }
    };
    for (pugi::xml_node wCfsN : wDoc.child("worksheet").children("conditionalFormatting")) {
        collectCfRules(wCfsN);
    }

    // Collect x14 extension conditional formatting (e.g. IconSets in extLst/ext/x14:conditionalFormattings)
    pugi::xml_node wExtLst = wDoc.child("worksheet").child("extLst");
    if (wExtLst) {
        // Get local part: after '}' (Clark) or after last ':' (prefix:local), else full name
        auto getLocalName = [](const char* nodeName) -> const char* {
            if (!nodeName) return nodeName;
            const char* clark = std::strchr(nodeName, '}');
            if (clark) return clark + 1;
            const char* colon = std::strrchr(nodeName, ':');
            return colon ? colon + 1 : nodeName;
        };
        auto localNameContains = [&getLocalName](const char* nodeName, const char* sub) {
            if (!nodeName || !sub) return false;
            const char* s = getLocalName(nodeName);
            tSize subLen = std::strlen(sub);
            for (tSize i = 0; s[i]; ++i) {
                if (std::strlen(s + i) < subLen) break;
                tBool match = true;
                for (tSize k = 0; k < subLen; ++k)
                    if (std::tolower((unsigned char)s[i + k]) != std::tolower((unsigned char)sub[k])) { match = false; break; }
                if (match) return true;
            }
            return false;
        };
        auto localNameEquals = [&getLocalName](const char* nodeName, const char* exact) {
            if (!nodeName || !exact) return false;
            const char* s = getLocalName(nodeName);
            tSize len = std::strlen(exact);
            if (std::strlen(s) != len) return false;
            for (tSize i = 0; i < len; ++i)
                if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)exact[i])) return false;
            return true;
        };
        for (pugi::xml_node wExt : wExtLst.children("ext")) {
            for (pugi::xml_node wCfsContainer : wExt.children()) {
                const char* containerName = wCfsContainer.name();
                if (!localNameContains(containerName, "conditionalFormattings")) continue;
                // x14:conditionalFormattings -> iterate x14:conditionalFormatting
                for (pugi::xml_node wX14Cf : wCfsContainer.children()) {
                    const char* cfName = wX14Cf.name();
                    if (!cfName || !localNameEquals(cfName, "conditionalFormatting")) continue;
                    // x14: sqref is often in a child element <xm:sqref>, not an attribute
                    tString wSqref = wX14Cf.attribute("sqref").as_string("");
                    if (wSqref.empty()) {
                        for (pugi::xml_node wChild : wX14Cf.children()) {
                            if (wChild.name() && localNameEquals(wChild.name(), "sqref")) {
                                wSqref = wChild.child_value();
                                break;
                            }
                        }
                    }
                    if (wSqref.empty()) continue;
                    for (pugi::xml_node wCr : wX14Cf.children()) {
                        const char* crName = wCr.name();
                        if (!localNameContains(crName, "cfRule")) continue;
                        pugi::xml_node wIconSet;
                        for (pugi::xml_node c : wCr.children()) {
                            if (localNameContains(c.name(), "iconSet")) { wIconSet = c; break; }
                        }
                        if (!wIconSet) continue;
                        ExcelCfRule wRule;
                        wRule.type = "iconSet";
                        wRule.sqref = wSqref;
                        wRule.priority = wCr.attribute("priority").as_int(0);
                        wRule.dxfId = -1;
                        wRule.operator_.clear();
                        wRule.formula.clear();
                        wRule.formula2.clear();
                        wRule.dataBarMinVal.clear();
                        wRule.dataBarMaxVal.clear();
                        wRule.dataBarColor.clear();
                        wRule.colorScaleVals.clear();
                        wRule.colorScaleColors.clear();
                        // Same OOXML default as the inline <iconSet> path above.
                        wRule.iconSetName = wIconSet.attribute("iconSet").as_string("3TrafficLights1");
                        if (wRule.iconSetName.empty()) wRule.iconSetName = wIconSet.attribute("iconSetValue").as_string("3TrafficLights1");
                        wRule.iconSetVals.clear();
                        wRule.iconSetCfvoTypes.clear();
                        for (pugi::xml_node wCfvo : wIconSet.children()) {
                            if (!localNameContains(wCfvo.name(), "cfvo")) continue;
                            tString wVal = wCfvo.attribute("val").as_string("");
                            if (wVal.empty()) {
                                for (pugi::xml_node wF : wCfvo.children()) {
                                    if (wF.name() && localNameEquals(wF.name(), "f")) {
                                        wVal = wF.child_value();
                                        break;
                                    }
                                }
                            }
                            wRule.iconSetVals.push_back(wVal.empty() ? "0" : wVal);
                            wRule.iconSetCfvoTypes.push_back(wCfvo.attribute("type").as_string("percent"));
                        }
#ifdef debugcondformat
                        std::cerr << "[CF]   x14 iconSet collected: sqref=\"" << wSqref << "\" iconSetName=\"" << wRule.iconSetName << "\" cfvoCount=" << wRule.iconSetVals.size() << std::endl;
#endif
                        wExcelCfRules.push_back(wRule);
                    }
                }
                break; // one conditionalFormattings per ext
            }
        }
    }

    // Apply conditional formats via SkSpreadSheet API (CustomFormulas / HighlightCellsRules)
    tSheet* wSheet = sApi.Sheet(wSheetName);
#ifdef debugcondformat
    std::cerr << "[CF] Applying " << wExcelCfRules.size() << " rule(s) on sheet \"" << wSheetName << "\"" << std::endl;
#endif
    // Keep OOXML file order for stacked icon sets; do not reorder by priority here.
    for (const ExcelCfRule& wRule : wExcelCfRules) {
        tString wSqrefNorm = wRule.sqref;
        for (tSize i = 0; i < wSqrefNorm.size(); ++i) {
            if (wSqrefNorm[i] == ' ') wSqrefNorm[i] = ';';
        }
#ifdef debugcondformat
        std::cerr << "[CF]   Apply rule type=\"" << wRule.type << "\" sqref=\"" << wSqrefNorm << "\"" << std::endl;
#endif
        if (wRule.type == "expression" || wRule.type == "formula") {
            // Formula-based rule: Param1 = formula (no leading =), Param2 = CSS when true (from dxfId)
            if (wRule.formula.empty()) continue;
            tString wFormula = wRule.formula;
            if (!wFormula.empty() && wFormula[0] == '=') wFormula.erase(0, 1);
            tString wCssWhenTrue;
            if (wRule.dxfId >= 0) {
                tString wFill = sReader.ResolveDxfFillColor(wRule.dxfId);
                if (!wFill.empty()) wCssWhenTrue += tString("background-color:#") + wFill + ";";
                // Pull the dxf font color too: Excel's named styles
                // ("Light Red Fill with Dark Red Text", etc.) carry both.
                tString wFont = sReader.ResolveDxfFontColor(wRule.dxfId);
                if (!wFont.empty()) wCssWhenTrue += tString("color:#") + wFont + ";";
            }
            sApi.UndoConditionalFormat("CustomFormulas", wSqrefNorm, wFormula, wCssWhenTrue,
                "", "", "", "", "", "", "", "", wSheet);
        } else if (wRule.type == "cellIs") {
            // Cell-is rule: the SkSpreadSheet conditional-format grammar uses
            // a "%" prefix to denote the current cell (see SkLemonSpreadSheet.y
            // production "state ::= percent_prefix conditionalformat"), with a
            // single comparison operator. R1C1-style "RC" is NOT a token here
            // and would fail parsing silently, leaving the rule unregistered.
            //
            // Compound operators (between/notBetween) are not expressible in
            // that single-comparison form, so we approximate with the lower
            // bound. Bumping this to two registered rules would need extra
            // plumbing on the API side.
            tString wFormula;
            if (wRule.operator_ == "between" && !wRule.formula2.empty()) {
                wFormula = tString("%>=") + wRule.formula;
            } else if (wRule.operator_ == "notBetween" && !wRule.formula2.empty()) {
                wFormula = tString("%<") + wRule.formula;
            } else if (wRule.operator_ == "equal") {
                wFormula = tString("%=") + wRule.formula;
            } else if (wRule.operator_ == "notEqual") {
                wFormula = tString("%<>") + wRule.formula;
            } else if (wRule.operator_ == "greaterThan") {
                wFormula = tString("%>") + wRule.formula;
            } else if (wRule.operator_ == "greaterThanOrEqual") {
                wFormula = tString("%>=") + wRule.formula;
            } else if (wRule.operator_ == "lessThan") {
                wFormula = tString("%<") + wRule.formula;
            } else if (wRule.operator_ == "lessThanOrEqual") {
                wFormula = tString("%<=") + wRule.formula;
            } else {
                continue;
            }
            tString wCssWhenTrue;
            if (wRule.dxfId >= 0) {
                tString wFill = sReader.ResolveDxfFillColor(wRule.dxfId);
                if (!wFill.empty()) wCssWhenTrue += tString("background-color:#") + wFill + ";";
                tString wFont = sReader.ResolveDxfFontColor(wRule.dxfId);
                if (!wFont.empty()) wCssWhenTrue += tString("color:#") + wFont + ";";
            }
            sApi.UndoConditionalFormat("HighlightCellsRules", wSqrefNorm, wFormula, wCssWhenTrue,
                "", "", "", "", "", "", "", "", wSheet);
        } else if (wRule.type == "dataBar") {
            // DataBars param contract is set by tConditionalFormat::CallBackCell
            // (SkConditionalFormat.cpp): Param1=color, Param3=style, Param4=min_value,
            // Param5=max_value, Param6=negative_color. The previous mapping put min/max
            // into Param1/Param2 and the bar color into Param3, which made
            // CallBackCell store the literal "0" string in tItemCF::m_Color; the
            // frontend drawDataBars() (SkUtility.js) then ran
            // normalizeHexColor("0"), got back "0", and the canvas fillStyle fell back
            // to black — visible regression on Conditional.sker B4:B18.
            //
            // For min/max: keep them empty when the OOXML rule omitted explicit cfvo
            // values so CallBackCell falls back to m_CalculatedMinValue /
            // m_CalculatedMaxValue (the actual data range), matching Excel's "auto"
            // dataBar behaviour. Forcing "0" here would collapse the bar to zero
            // width and trip drawDataBars()'s minValue===maxValue early-return.
            tString wColor = wRule.dataBarColor.empty() ? "638EC6" : wRule.dataBarColor;
            sApi.UndoConditionalFormat("DataBars", wSqrefNorm,
                wColor,                  // Param1: bar color (hex without # — normalizeHexColor adds it)
                "",                      // Param2: unused
                "gradient",              // Param3: style
                wRule.dataBarMinVal,     // Param4: min_value (empty => auto)
                wRule.dataBarMaxVal,     // Param5: max_value (empty => auto)
                "",                      // Param6: negative color
                "", "", "", "", wSheet);
        } else if (wRule.type == "colorScale") {
            // ColorScales: Param1=min_color, Param2=mid_color, Param3=max_color, Param4=min_value, Param5=mid_value, Param6=max_value
            tSize wN = std::min(wRule.colorScaleVals.size(), wRule.colorScaleColors.size());
            if (wN >= 2) {
                tString wMinC = wRule.colorScaleColors.size() > 0 ? wRule.colorScaleColors[0] : "FFFFFF";
                tString wMidC = (wN >= 3 && wRule.colorScaleColors.size() > 1) ? wRule.colorScaleColors[1] : wMinC;
                tString wMaxC = wRule.colorScaleColors.size() > (wN - 1) ? wRule.colorScaleColors[wN - 1] : "FFFFFF";
                tString wMinV = wRule.colorScaleVals[0];
                tString wMidV = wN >= 3 ? wRule.colorScaleVals[1] : wMinV;
                tString wMaxV = wRule.colorScaleVals[wN - 1];
                sApi.UndoConditionalFormat("ColorScales", wSqrefNorm, wMinC, wMidC, wMaxC, wMinV, wMidV, wMaxV,
                    "", "", "", "", wSheet);
            }
        } else if (wRule.type == "iconSet") {
            // IconSets: 3-icon Param1-3=icons, Param4-6=thresholds, Param7=IconType, Param8=OOXML iconSet name, Param9=cfvo types
            //           5-icon Param1-5=icons, Param6-9=thresholds, Param10=IconType
            const tString wIconType = mapExcelIconSetNameToSkIconType(wRule.iconSetName);
            const tString wCfvoTypes = joinCommaList(wRule.iconSetCfvoTypes);
            const tBool wFiveIcons = (wRule.iconSetVals.size() >= 5) || (wRule.iconSetName.find("5") != tString::npos);
            const std::vector<tString> wAreas = splitSqrefAreas(wSqrefNorm);
            for (const tString& wArea : wAreas) {
                if (wFiveIcons) {
                    // 5-icon: Param1-5 = icons, Param6-9 = 4 thresholds, Param10 = IconType
                    tString wI1, wI2, wI3, wI4, wI5;
                    if (wIconType == "Arrows") {
                        wI1 = "\xE2\x86\x93"; wI2 = "\xE2\x86\x98"; wI3 = "\xE2\x86\x92"; wI4 = "\xE2\x86\x97"; wI5 = "\xE2\x86\x91"; // ↓ ↘ → ↗ ↑
                    } else if (wIconType == "Flags") {
                        wI1 = "\xF0\x9F\x98\x9E"; wI2 = "\xF0\x9F\x98\x9E"; wI3 = "\xF0\x9F\x98\x90"; wI4 = "\xF0\x9F\x98\x8A"; wI5 = "\xF0\x9F\x98\x8A"; // 😞😞😐😊😊
                    } else if (wIconType == "Shapes") {
                        wI1 = "\xE2\x97\xAF"; wI2 = "\xE2\x96\xB2"; wI3 = "\xE2\x97\x86"; wI4 = "\xE2\x96\xB2"; wI5 = "\xE2\x96\xA0"; // ○ △ ◆ △ ■
                    } else if (wIconType == "Indicators") {
                        wI1 = "\xE2\xAC\xA4"; wI2 = "\xE2\xAC\xA4"; wI3 = "\xE2\xAC\xA4"; wI4 = "\xE2\xAC\xA4"; wI5 = "\xE2\xAC\xA4"; // ●
                    } else {
                        wI1 = "\xE2\x98\x86"; wI2 = "\xE2\x98\x86"; wI3 = "\xE2\x98\x86"; wI4 = "\xE2\x98\x86"; wI5 = "\xE2\x98\x85"; // ☆☆☆☆★
                    }
                    tString wT1 = wRule.iconSetVals.size() > 1 ? wRule.iconSetVals[1] : "20";
                    tString wT2 = wRule.iconSetVals.size() > 2 ? wRule.iconSetVals[2] : "40";
                    tString wT3 = wRule.iconSetVals.size() > 3 ? wRule.iconSetVals[3] : "60";
                    tString wT4 = wRule.iconSetVals.size() > 4 ? wRule.iconSetVals[4] : "80";
                    sApi.UndoConditionalFormat("IconSets", wArea,
                        wI1, wI2, wI3, wI4, wI5, wT1, wT2, wT3, wT4, wIconType, wSheet);
                } else {
                    // 3-icon: Param1-3 = icons, Param4-6 = thresholds, Param7 = IconType
                    tString wI1, wI2, wI3;
                    if (wIconType == "Arrows") {
                        wI1 = "\xE2\x86\x93"; wI2 = "\xE2\x86\x92"; wI3 = "\xE2\x86\x91"; // UTF-8: ↓ → ↑
                    } else if (wIconType == "Flags") {
                        wI1 = "\xF0\x9F\x98\x9E"; wI2 = "\xF0\x9F\x98\x90"; wI3 = "\xF0\x9F\x98\x8A"; // 😞 😐 😊
                    } else if (wIconType == "Shapes") {
                        wI1 = "\xE2\x97\xAF"; wI2 = "\xE2\x96\xB2"; wI3 = "\xE2\x97\x86"; // ○ △ ◆
                    } else if (wIconType == "Indicators") {
                        wI1 = "\xE2\xAC\xA4"; wI2 = "\xE2\xAC\xA4"; wI3 = "\xE2\xAC\xA4"; // ●
                    } else {
                        wI1 = "\xE2\x98\x86"; wI2 = "\xE2\x98\x86"; wI3 = "\xE2\x98\x85"; // ☆ ☆ ★ (Ratings)
                    }
                    tString wT1 = wRule.iconSetVals.size() > 0 ? wRule.iconSetVals[0] : "0";
                    tString wT2 = wRule.iconSetVals.size() > 1 ? wRule.iconSetVals[1] : "33";
                    tString wT3 = wRule.iconSetVals.size() > 2 ? wRule.iconSetVals[2] : "67";
                    tString wRuleKey = wArea + "@" + wRule.iconSetName;
                    tString wPri = std::to_string(wRule.priority);
                    sApi.UndoConditionalFormat("IconSets", wRuleKey,
                        wI1, wI2, wI3, wT1, wT2, wT3, wIconType,
                        wRule.iconSetName, wCfvoTypes, wPri, wSheet);
                }
            }
        }
    }

    // Shared-formula masters are registered in the same pass as cells (Excel usually
    // writes the master first). Slaves seen before their master are queued.
    struct tMasterSharedFormula {
        tInt m_si;
        tInt m_row;
        tInt m_col;
        tString m_formulaShared;
        tString m_formulaR1C1;
    };
    std::unordered_map<tInt, tMasterSharedFormula> wSharedMasterBySi;
    struct tPendingSharedSlave {
        tInt m_si;
        tString m_ref;
    };
    std::vector<tPendingSharedSlave> wPendingSharedSlaves;

    tInt wNextImpliedRow = 1;
    forEachRow([&](pugi::xml_node wRow) {
        tInt wRIdx = ResolveSheetRowIndex(wRow, wNextImpliedRow);
        ApplyExcelRowSize(sApi, wRIdx, wRow);
        tBool wCustomFormat = wRow.attribute("customFormat").as_bool(false);
        const char* wRowStyleAttr = wRow.attribute("s").value();
        if (wCustomFormat && wRowStyleAttr && *wRowStyleAttr && wRIdx > 0) {
            tInt wStyleIdx = std::atoi(wRowStyleAttr);
            tString wCss = sReader.BuildCssForStyle(wStyleIdx);
            if (!wCss.empty()) {
                tRect wRect;
                wRect.RowSelect(wRIdx);
                tString wRowRef = wRect.StrRef();
                tCellAddr wAddr = std::make_tuple(wSheetName, wRowRef);
                m_MapCss[wAddr] = wCss;
            }
        }
        if (wRIdx > wUsedExtent.m_BottomRow) {
            wUsedExtent.m_BottomRow = wRIdx;
        }

        for (pugi::xml_node wNodeCell : wRow.children("c")) {
            const char* wR = wNodeCell.attribute("r").value();
            if (!wR || !*wR) continue;
            auto wRc = RefToRowCol(wR);
            tInt wRowIdx = wRc.first;
            tInt wColIdx = wRc.second;
            
            const char* wTAttr = wNodeCell.attribute("t").value();
            const char* wSAttr = wNodeCell.attribute("s").value();
            tString wValueStr;
            tInt wSharedStrIdx = -1;
            if (tString(wTAttr) == "inlineStr") {
                if (auto wIs = wNodeCell.child("is"); wIs) {
                    if (pugi::xml_node wT = wIs.child("t"))
                        wValueStr = wT.child_value();
                    for (pugi::xml_node wR : wIs.children("r")) {
                        if (pugi::xml_node wRt = wR.child("t"))
                            wValueStr += wRt.child_value();
                    }
                }
            } else if (auto wV = wNodeCell.child("v")) {
                wValueStr = wV.child_value();
            }
            
            // Shared strings ==============================================
            if (tString(wTAttr) == "s") {
                if (!wValueStr.empty()) {
                    tInt wIdx = std::atoi(wValueStr.c_str());
                    const auto& wSst = sReader.GetSharedStrings();
                    if (wIdx >= 0 && wIdx < (tInt)wSst.size()) {
                        wSharedStrIdx = wIdx;
                        wValueStr = wSst[wIdx];
                    }
                }
            }
        
            // Formula ====================================================
            tString wFormula;
            if (auto wF = wNodeCell.child("f")) {
                tString wFText = wF.child_value();
                const char* wFType = wF.attribute("t").value();
                tInt wSi = wF.attribute("si").as_int(-1);
                
                // Store cells with ca="1" attribute (calculated column formulas)
                const char* wCaAttr = wF.attribute("ca").value();
                if (wCaAttr && *wCaAttr && tString(wCaAttr) == "1") {
                    tCellAddr wCellAddr = std::make_tuple(wSheetName, wR);
                    m_CellsWithCalculatedColumn.insert(wCellAddr);
                }
                if (wFType && tString(wFType) == "shared") {
                    if (!wFText.empty() && wSharedMasterBySi.find(wSi) == wSharedMasterBySi.end()) {
                        tString wFormulaMasterA1 = TransFormFormulaSyntax(wFText);
                        if (wSheet != nullptr) {
                            TransformFormulaIndirectNotation(wFormulaMasterA1, sApi, wSheet);
                        }
                        tString wFormulaMasterR1C1 =
                            ConvertA1CellRefsToR1C1(wFormulaMasterA1, wRowIdx, wColIdx);
                        tCellAddr wCellAddr = std::make_tuple(wSheetName, wR);
                        m_MapFormula[wCellAddr] = "=" + wFormulaMasterR1C1;
                        wSharedMasterBySi[wSi] = {
                            wSi, wRowIdx, wColIdx, wFormulaMasterA1, wFormulaMasterR1C1
                        };
                    }
                    auto itM = wSharedMasterBySi.find(wSi);
                    if (!wFText.empty()) {
                        wFormula = (itM != wSharedMasterBySi.end()) ? itM->second.m_formulaR1C1 : wFText;
                    } else if (itM != wSharedMasterBySi.end()) {
                        wFormula = itM->second.m_formulaR1C1;
                    } else {
                        wPendingSharedSlaves.push_back({ wSi, tString(wR) });
                        wFormula.clear();
                    }
                } else {
                    wFormula = wFText;
                    // Dynamic array (t="array" ref="..."): Excel stores one <f> on the spill origin (top-left of ref).
                    // SkSpreadSheet spills from that cell; copying the same formula into every cell in ref breaks matrix
                    // results (e.g. JoursEtSemaines+DATE(...) would evaluate the full matrix from each column as origin).
                    const char* wRefAttr = wF.attribute("ref").value();
                    if (wFType && !wFormula.empty() && wRefAttr && *wRefAttr &&
                        (tString(wFType) == "array" || tString(wFType) == "dynamic")) {
                        tIndex wTop = 0, wLeft = 0, wBottom = 0, wRight = 0;
                        if (ParseRange(tString(wRefAttr), wTop, wLeft, wBottom, wRight)) {
                            tString wFormulaNorm = wFormula;
                            if (wSheet != nullptr) {
                                TransFormFormulaSyntax(wFormulaNorm, sApi, wSheet);
                            } else {
                                TransFormFormulaSyntax(wFormulaNorm);
                            }
                            // Use OOXML cell "r" as key (same as other m_MapFormula entries).
                            tCellAddr wAddrArr = std::make_tuple(wSheetName, tString(wR));
                            m_MapFormula[wAddrArr] = "=" + wFormulaNorm;
                            const auto wTupleRect = std::make_tuple(wTop, wLeft, wBottom, wRight);
                            m_MapArrayFormulaOutput[wAddrArr] = wTupleRect;
                            const auto wRcOrigin = RefToRowCol(tString(wR));
                            m_MapArrayFormulaOutputByOrigin[std::make_tuple(wSheetName, wRcOrigin.first, wRcOrigin.second)] =
                                wTupleRect;
                            wFormula.clear(); // origin recorded in m_MapFormula; skip duplicate assign at block below
                        }
                    }
                }
            }
           
           
            // Formula or Text =============================================
            if (!wFormula.empty()) {
                // Ensure the cell exists, but build the address key from the known-good locals
                // (sheet name + OOXML "r" ref) instead of dereferencing wCell->Sheet(): EnsureCell
                // may return null, or the cell's Sheet() back-pointer may be unset here, and copying
                // that sheet's corrupted m_Name throws std::length_error (hard abort under Wasm).
                // This also matches the canonical key used elsewhere, e.g. make_tuple(wSheetName, wR).
                sApi.EnsureCell(wRowIdx, wColIdx);
                tCellAddr wAddr = std::make_tuple(wSheetName, tString(wR));
                tString wStoredFormula = wFormula;
                if (wSheet != nullptr) {
                    TransFormFormulaSyntax(wStoredFormula, sApi, wSheet);
                } else {
                    TransFormFormulaSyntax(wStoredFormula);
                }
                m_MapFormula[wAddr] = "=" + wStoredFormula;
                // Cache Excel computed value for formula cells (skip RecalculateAll at import).
                tVariant wCachedValue;
                if (ParseOoxmlCellValueToVariant(wValueStr, wSAttr, wTAttr, sReader, wCachedValue)) {
                    m_MapFormulaCachedValue[wAddr] = wCachedValue;
                }
            } else {
                // OOXML spill slaves: <f ca="1"/> with no formula text; Excel still stores <v> (cached spill output).
                // Do not import <v>: SkMatrix::SpillDestinationIsClear treats any non-null value as blocking spill
                // until MatExtend is set; leaving these cells empty lets matrix spill fill them on recalc.
                tBool wOoxmlSpillSlaveNoFormulaText = false;
                if (auto wFNode = wNodeCell.child("f")) {
                    const char* wCaSkip = wFNode.attribute("ca").value();
                    const char* wFTextSkip = wFNode.child_value();
                    if (wCaSkip && *wCaSkip && tString(wCaSkip) == "1" &&
                        (!wFTextSkip || !*wFTextSkip)) {
                        wOoxmlSpillSlaveNoFormulaText = true;
                    }
                }
                if (!wOoxmlSpillSlaveNoFormulaText && !wValueStr.empty()) {
                    tVariant wVariant;
                    if (ParseOoxmlCellValueToVariant(wValueStr, wSAttr, wTAttr, sReader, wVariant)) {
                        sApi.CellValue(wRowIdx, wColIdx, wVariant);
                    }
#ifdef debugvalue
                    // Debug
                    tCell* wCell=sApi.Cell(wRowIdx,wColIdx);
                    cout << wCell->StrRef(true) << "=" << wCell->Value() << endl;
#endif
                }

            }

            // Apply CSS: cellXfs plus uniform rich-text bold/italic from SST or inlineStr.
            tString wCss;
            if (wSAttr && *wSAttr) {
                const tInt wStyleIdx = std::atoi(wSAttr);
                wCss = sReader.BuildCssForStyle(wStyleIdx);
            }
            if (wSharedStrIdx >= 0) {
                const tString& wRich = sReader.GetSharedStringRichCss(static_cast<tSize>(wSharedStrIdx));
                if (!wRich.empty()) wCss += wRich;
            } else if (tString(wTAttr) == "inlineStr") {
                if (auto wIs = wNodeCell.child("is"))
                    wCss += sReader.BuildRichTextUniformCss(wIs);
            }
            if (!wCss.empty()) {
                // Same rationale as above: use the local sheet name + "r" ref rather than
                // wCell->Sheet()->Name(), which can deref an invalid Sheet() back-pointer.
                if (sApi.EnsureCell(wRowIdx, wColIdx)) {
                    tCellAddr wAddr = std::make_tuple(wSheetName, tString(wR));
                    m_MapCss[wAddr] = wCss;
                }
            }
            
            // Conditional formatting is applied by SkSpreadSheet engine from rules registered above (UndoConditionalFormat)
            if (wColIdx > wUsedExtent.m_RightCol) {
                wUsedExtent.m_RightCol = wColIdx;
            }
        }
    });

    for (const auto& wPending : wPendingSharedSlaves) {
        auto itM = wSharedMasterBySi.find(wPending.m_si);
        if (itM == wSharedMasterBySi.end()) {
            continue;
        }
        m_MapFormula[std::make_tuple(wSheetName, wPending.m_ref)] = "=" + itM->second.m_formulaR1C1;
    }

    if (wStreamRows && wSheetRightCol == 0 && wUsedExtent.m_RightCol > 0) {
        if (wSheetDefaultColWidthChars > 0.0) {
            sApi.UndoSizeCol(1, wUsedExtent.m_RightCol, ExcelColWidthCharsToSkMm(wSheetDefaultColWidthChars, wColMdw));
        }
    }
    FillUnsetExcelRowHeights(sApi, wUsedExtent.m_BottomRow,
        ExcelRowHeightPtToSkMm(wSheetDefaultRowHtPt));
}

/// @brief Strip Excel color/condition tags [...] and mangled remnants (e.g. "-40C]") so SkRoot sees only the numeric/date part.
#if 0 // reserved for format-string cleanup during import
static tString StripExcelFormatColorTags(const tString& s) {
    tString out;
    out.reserve(s.size());
    tSize i = 0;
    while (i < s.size()) {
        if (s[i] == '[') {
            while (i < s.size() && s[i] != ']') ++i;
            if (i < s.size()) ++i; // skip ]
            continue;
        }
        out += s[i];
        ++i;
    }
    // Remove trailing garbage like "-40C]" (mangled "[Color 40]"): strip trailing ']' then suffix starting with '-' when that suffix contains a letter (so we keep "€#,##0" and "0.00" intact)
    if (!out.empty() && out.back() == ']')
        out.pop_back();
    tSize lastHyphen = tString::npos;
    for (tSize k = 0; k < out.size(); ++k)
        if (out[k] == '-') lastHyphen = k;
    if (lastHyphen != tString::npos) {
        tString suffix = out.substr(lastHyphen);
        tBool hasLetter = false;
        tBool onlyTagChars = true;
        for (char c : suffix) {
            if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
            if (c != '-' && c != ' ' && c != '\t' && !std::isalnum(static_cast<unsigned char>(c))) onlyTagChars = false;
        }
        if (hasLetter && onlyTagChars)
            out.resize(lastHyphen);
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
    return out;
}
#endif


/// @brief Apply CSS styles to cells
// Extract a CSS property segment ("name:value;") from a CSS string and return
// its value (without the trailing semicolon). Modifies sCss in place. Returns
// an empty string if the property is not present. The search requires the
// property to start at offset 0 or right after a ';' so we do not match
// "border-right-color" when looking for "border-right".
static tString ExtractAndRemoveCssProp(tString& ioCss, const tString& sKey) {
    tString wNeedle = sKey + ":";
    tSize wPos = 0;
    while (wPos < ioCss.size()) {
        tSize wFound = ioCss.find(wNeedle, wPos);
        if (wFound == tString::npos) return "";
        tBool wOk = (wFound == 0) || (ioCss[wFound - 1] == ';');
        if (!wOk) {
            wPos = wFound + 1;
            continue;
        }
        tSize wValStart = wFound + wNeedle.size();
        tSize wEnd = ioCss.find(';', wValStart);
        tString wVal = (wEnd == tString::npos)
                        ? ioCss.substr(wValStart)
                        : ioCss.substr(wValStart, wEnd - wValStart);
        tSize wRemoveEnd = (wEnd == tString::npos) ? ioCss.size() : wEnd + 1;
        ioCss.erase(wFound, wRemoveEnd - wFound);
        return wVal;
    }
    return "";
}

// Returns true if the CSS block already declares the given property (matched
// with the same "start-of-block" anchoring as ExtractAndRemoveCssProp).
static tBool CssHasProp(const tString& sCss, const tString& sKey) {
    tString wNeedle = sKey + ":";
    tSize wPos = 0;
    while (wPos < sCss.size()) {
        tSize wFound = sCss.find(wNeedle, wPos);
        if (wFound == tString::npos) return false;
        if (wFound == 0 || sCss[wFound - 1] == ';') return true;
        wPos = wFound + 1;
    }
    return false;
}

void tExcel2SpreadSheet::AdjacencyConvertMergedBorders() {
    // Excel keeps the 4 perimeter borders of a merged range on the anchor
    // (top-left) cell. Our adjacency model (see SkJsonView / EmitMergedBorders)
    // expects:
    //   - the merge right edge  -> border-left on each cell of column right+1
    //   - the merge bottom edge -> border-top  on each cell of row bottom+1
    // Without this rewrite, importing a .xlsx file with a bordered merged
    // range leaves the right and bottom edges invisible because the neighbors
    // carry nothing, and EmitMergedBorders only projects from neighbors.
    //
    // The rewrite is skipped (left on the anchor) when the corresponding
    // neighbor already declares its own border-left / border-top, so a
    // caller-provided adjacent border wins over the merge's projection.
    for (const auto& wMerge : m_MergedRanges) {
        const tString& wSheetName = std::get<0>(wMerge);
        tIndex wTop    = std::get<1>(wMerge);
        tIndex wLeft   = std::get<2>(wMerge);
        tIndex wBottom = std::get<3>(wMerge);
        tIndex wRight  = std::get<4>(wMerge);

        tString wAnchorRef = Base10ToAlpha(wLeft) + std::to_string(wTop);
        tCellAddr wAnchorAddr = std::make_tuple(wSheetName, wAnchorRef);
        auto wIt = m_MapCss.find(wAnchorAddr);
        if (wIt == m_MapCss.end()) continue;

        tString& wAnchorCss = wIt->second;
        tString wRightVal  = ExtractAndRemoveCssProp(wAnchorCss, "border-right");
        tString wBottomVal = ExtractAndRemoveCssProp(wAnchorCss, "border-bottom");
        if (wRightVal.empty() && wBottomVal.empty()) continue;

        // Project right edge onto the full neighbor column (all rows of the merge).
        if (!wRightVal.empty() && wRight + 1 <= Cst_MaxCol) {
            tIndex wNbCol = wRight + 1;
            for (tIndex wR = wTop; wR <= wBottom; ++wR) {
                tString wNbRef = Base10ToAlpha(wNbCol) + std::to_string(wR);
                tCellAddr wNbAddr = std::make_tuple(wSheetName, wNbRef);
                tString& wNbCss = m_MapCss[wNbAddr];
                if (!CssHasProp(wNbCss, "border-left")) {
                    wNbCss += "border-left:" + wRightVal + ";";
                }
            }
        }

        // Project bottom edge onto the full neighbor row (all cols of the merge).
        if (!wBottomVal.empty() && wBottom + 1 <= Cst_MaxRow) {
            tIndex wNbRow = wBottom + 1;
            for (tIndex wC = wLeft; wC <= wRight; ++wC) {
                tString wNbRef = Base10ToAlpha(wC) + std::to_string(wNbRow);
                tCellAddr wNbAddr = std::make_tuple(wSheetName, wNbRef);
                tString& wNbCss = m_MapCss[wNbAddr];
                if (!CssHasProp(wNbCss, "border-top")) {
                    wNbCss += "border-top:" + wBottomVal + ";";
                }
            }
        }
    }
    m_MergedRanges.clear();
}

void tExcel2SpreadSheet::AdjacencyConvertCellBorders() {
    // Excel stores all four sides on each cell. Our renderer (SkSpCellCanvas
    // + SkJsonView) reads each cell's own border-top / border-left and
    // expects right and bottom to live on the *next* cell as the neighbor's
    // left / top. Without this rewrite, importing a .xlsx where A1 has
    // border-right keeps that property on A1 where the painter ignores it
    // for the inter-cell edge, and B1 receives nothing.
    //
    // We collect the projections in a separate map and only commit them
    // afterwards to keep iteration on m_MapCss simple and to avoid
    // double-processing freshly inserted neighbor entries.
    struct tProjection {
        tString css; // CSS fragment to append (e.g. "border-left:solid 2px #000000;")
    };
    std::map<tCellAddr, tProjection> wAddLeft;
    std::map<tCellAddr, tProjection> wAddTop;

    for (auto& wEntry : m_MapCss) {
        const tCellAddr& wAddr = wEntry.first;
        tString& wCss = wEntry.second;

        tString wRight  = ExtractAndRemoveCssProp(wCss, "border-right");
        tString wBottom = ExtractAndRemoveCssProp(wCss, "border-bottom");
        if (wRight.empty() && wBottom.empty()) continue;

        const tString& wSheetName = std::get<0>(wAddr);
        const tString& wRef       = std::get<1>(wAddr);
        tIndex wTop = 0, wLeft = 0, wBot = 0, wRgt = 0;
        if (!ParseRange(wRef, wTop, wLeft, wBot, wRgt)) continue;
        // ParseRange handles single cell refs: top==bottom, left==right.

        if (!wRight.empty() && wLeft + 1 <= Cst_MaxCol) {
            tString wNbRef = Base10ToAlpha(wLeft + 1) + std::to_string(wTop);
            tCellAddr wNbAddr = std::make_tuple(wSheetName, wNbRef);
            wAddLeft[wNbAddr] = { "border-left:" + wRight + ";" };
        }
        if (!wBottom.empty() && wTop + 1 <= Cst_MaxRow) {
            tString wNbRef = Base10ToAlpha(wLeft) + std::to_string(wTop + 1);
            tCellAddr wNbAddr = std::make_tuple(wSheetName, wNbRef);
            wAddTop[wNbAddr] = { "border-top:" + wBottom + ";" };
        }
    }

    // Apply projections; an existing border-left / border-top on the
    // neighbor wins, mirroring AdjacencyConvertMergedBorders.
    for (const auto& wIt : wAddLeft) {
        tString& wNbCss = m_MapCss[wIt.first];
        if (!CssHasProp(wNbCss, "border-left")) {
            wNbCss += wIt.second.css;
        }
    }
    for (const auto& wIt : wAddTop) {
        tString& wNbCss = m_MapCss[wIt.first];
        if (!CssHasProp(wNbCss, "border-top")) {
            wNbCss += wIt.second.css;
        }
    }
}

void tExcel2SpreadSheet::ApplyCssStyles(tApi& sApi) {
    tUndoActifScope wUndoOff(sApi, false);
    typedef map<tString,tString> tMapDebug;
    tMapDebug wMapDebug;

    // Apply CSS styles (SkSpreadSheet handles format override/priority)
    for (const auto& wCellItem : m_MapCss) {
        const tExcel2SpreadSheet::tCellAddr& wCellAdr = wCellItem.first;
        const tString& wSheetName = std::get<0>(wCellAdr);
        tSheet* wRectSheet = sApi.Sheet(wSheetName);
        if (wRectSheet == nullptr) {
#ifdef debugerror
            cerr << "Error: Sheet not found: " << wSheetName << endl;
#endif
            continue;
        }
        const tString& wRef = std::get<1>(wCellAdr);
        tRect wRect(wRef);
        tString wCssStr = wCellItem.second;
#ifdef debugformat
        cout << "Format Sheet :" << wSheetName << ":" << wRef << wCssStr << endl;
#endif
#if defined(__EXCEPTIONS) && __EXCEPTIONS
        try {
#endif
            if (!sApi.UndoCellFormat(wRect.StrRef(), wCssStr, wRectSheet)) {
#ifdef debugerror
                cerr << "Error applying format to " << wRect.StrRef() << ": " << wCellItem.second << endl;
                cerr << sApi.ErrorWithDetail() << endl;
#endif
            }
#if defined(__EXCEPTIONS) && __EXCEPTIONS
        } catch (const SkRoot::tExceptionInternalError& wE) {
#ifdef debugerror
            cerr << "Exception applying format to " << wRect.StrRef() << " on sheet " << wSheetName << ": " << wE.what() << endl;
#endif
        } catch (...) {
#ifdef debugerror
            cerr << "Unknown exception applying format to " << wRect.StrRef() << " on sheet " << wSheetName << endl;
#endif
        }
#endif
        wMapDebug[wCellItem.second] = wCellItem.second;
    }

#ifdef debugformat
    tInt wInd=1;
    for(auto MapDebug : wMapDebug ) {
        cout << "wFormat[" << wInd++  << "]="<<  MapDebug.first << endl;
    }
#endif
}

/// @brief Apply named ranges to cells
void tExcel2SpreadSheet::ApplyNamedRanges(tApi& sApi) {
    tUndoActifScope wUndoOff(sApi, false);
    // First create Named Range
    for(auto wRangeNamedItem : m_MapRangeNamed) {
        const tString& wName = wRangeNamedItem.first;
         const tRangeNamed& wRangeNamed = wRangeNamedItem.second;
         if (wRangeNamed.m_Formula=="") {
          tSheet* wSheet=sApi.Sheet(wRangeNamedItem.second.m_SheetName);
          sApi.EnsureRangeNamed(wName, wRangeNamedItem.second.m_Ref,"",wSheet);
        } else if (IsExcelDefinedNameRefList(wRangeNamed.m_Formula)) {
            ApplyExcelDefinedNameRefList(sApi, wName, wRangeNamed.m_Formula);
        } else {
            CreateFormulaNamedRange(sApi, wName, wRangeNamed.m_Formula);
        }
    }
}

void tExcel2SpreadSheet::ApplyFormulas(tApi& sApi) {
#ifdef debuginfo
    cout << "ApplyFormulas()" << endl;
#endif
    tUndoActifScope wUndoOff(sApi, false);
    tSize wCompiledCount = 0;
    // Apply named-formula sheet (_$$) first: std::map order puts "JANVIER" before "_$$" ('J' < '_'); sheet formulas
    // that reference FormulaNamed names need the _$$ definition cells compiled first (e.g. JoursEtSemaines matrix).
    auto wApplyOne = [&](const std::pair<const tCellAddr, tString>& wCellItem) {
        const tString& wSheetName = std::get<0>(wCellItem.first);

        tSheet* wSheet = sApi.Sheet(wSheetName);
        sApi.ActiveSheet(wSheetName);
        const tString& wRef = std::get<1>(wCellItem.first);
        const tString& wFormulaStr = wCellItem.second;

#ifdef debugformula
        cout << wSheetName << "!" << wRef << wFormulaStr << endl;
#endif
        // Bound A:A / $B:$B here (all sheets imported, LastRow is real). ProcessSheet
        // must not bound: a later sheet still has LastRow==0 and became $A$1:$A$1.
        tString wFormulaToApply = wFormulaStr;
        if (!wFormulaToApply.empty() && wFormulaToApply.front() == '=') {
            wFormulaToApply.erase(0, 1);
        }
        if (wSheet != nullptr) {
            BoundWholeColumnRefsInFormula(wFormulaToApply, sApi, wSheet);
        }
        const std::pair<tInt, tInt> wCellRc = RefToRowCol(wRef);
        // Structured table refs (PaymentSchedule3[[#Headers],...]) must stay in A1 form; per-row
        // R1C1 conversion corrupts them and yields invalid Excel refs like [[#Headers],...].
        if (!FormulaUsesR1C1CellRefs(wFormulaToApply)
            && !FormulaContainsStructuredTableRef(wFormulaToApply)) {
            wFormulaToApply =
                ConvertA1CellRefsToR1C1(wFormulaToApply, wCellRc.first, wCellRc.second);
        }
        if (wSheet != nullptr) {
            MaterializeIndirectR1C1RefsToA1(
                wFormulaToApply, wCellRc.first, wCellRc.second, sApi, wSheet);
        }
        if (m_MaterializeIndirectFrenchL1C1CellRefs) {
            MaterializeIndirectFrenchL1C1CellRefsInFormula(wFormulaToApply);
        }
        if (FormulaContainsIndirect(wFormulaToApply) && !kSkExcelTransformIndirectEnabled) {
            tString wFormulaAsText = wFormulaStr;
            if (!wFormulaAsText.empty() && wFormulaAsText.front() != '=') {
                wFormulaAsText = "=" + wFormulaAsText;
            }
            if (tCell* wCell = sApi.EnsureCell(wRef, wSheet)) {
                SetCellFormulaAsText(wCell, wFormulaAsText);
            }
            return;
        }
        // OOXML array ref (t="array" ref="B8:H8"): register spill rect before CompilCell. SpillDestinationIsClear runs
        // during first compile; if IsSpillRange() is still false, slaves with cached [] or <v> block the spill.
        tBool wHaveRect = false;
        {
            std::tuple<tIndex, tIndex, tIndex, tIndex> wt{};
            auto wItArr = m_MapArrayFormulaOutput.find(wCellItem.first);
            if (wItArr != m_MapArrayFormulaOutput.end()) {
                wt = wItArr->second;
                wHaveRect = true;
            } else {
                auto wItOrig = m_MapArrayFormulaOutputByOrigin.find(
                    std::make_tuple(wSheetName, wCellRc.first, wCellRc.second));
                if (wItOrig != m_MapArrayFormulaOutputByOrigin.end()) {
                    wt = wItOrig->second;
                    wHaveRect = true;
                }
            }
            if (wHaveRect) {
                if (tCell* wCellPre = sApi.EnsureCell(wRef, wSheet)) {
                    wCellPre->SetSpillRect(
                        std::get<0>(wt), std::get<1>(wt), std::get<2>(wt), std::get<3>(wt));
                }
            }
        }
        // Always CompilCell per host cell: SharedFormulaPool dedups opcodes but each cell needs its own VectorRef.
        const tString wFormulaForCell = wFormulaToApply.empty() ? wFormulaStr : ("=" + wFormulaToApply);
        if (!sApi.CellValue(wRef, wFormulaForCell, wSheet)) {
#ifdef debugerror
            cerr << " Error " << wSheetName << ":" << wRef << endl;
            cerr << sApi.ErrorWithDetail() << endl;
#endif
        } else {
            ++wCompiledCount;
        }

#ifdef debugformula
        if (tCell* wCell = sApi.Cell(wRef)) {
            tString wExcelFormulaForCompare = wFormulaStr;
            if (!wExcelFormulaForCompare.empty() && wExcelFormulaForCompare.front() != '=') {
                wExcelFormulaForCompare = "=" + wExcelFormulaForCompare;
            }
            tString wFormulaCtrl = "=" + wCell->FormulaStr();
            cout << " -> " << wCell->StrRef(true) << ":" << wCell->FormulaStr() << endl;
            if (wFormulaCtrl != wExcelFormulaForCompare && wFormulaCtrl != wFormulaToApply) {
                cout << wRef << " Difference between Excel and SkSpreadSheet " << wCell->Sheet()->Name() << endl;
                cout << "Excel : " << wExcelFormulaForCompare << endl;
                cout << "SkSp  : " << wFormulaCtrl << endl;
            }
        }
#endif
    };

    for (const auto& wCellItem : m_MapFormula) {
        if (std::get<0>(wCellItem.first) != CstSheetNamed) {
            continue;
        }
        wApplyOne(wCellItem);
    }
    // 2 Pass for 
    for (const auto& wCellItem : m_MapFormula) {
        if (std::get<0>(wCellItem.first) == CstSheetNamed) {
            continue;
        }
        wApplyOne(wCellItem);
    }
#ifdef debuginfo
    cout << "ApplyFormulas: compiled=" << wCompiledCount << endl;
#endif
}

void tExcel2SpreadSheet::ApplyCachedFormulaValues(tApi& sApi) {
    for (const auto& wItem : m_MapFormulaCachedValue) {
        const tString& wSheetName = std::get<0>(wItem.first);
        const tString& wRef = std::get<1>(wItem.first);
        tSheet* wSheet = sApi.Sheet(wSheetName);
        if (wSheet == nullptr) {
            continue;
        }
        if (tCell* wCell = sApi.EnsureCell(wRef, wSheet)) {
            wCell->Value(wItem.second);
        }
    }
}

void tExcel2SpreadSheet::EnsureDefaultFontOnCssMap(tApi& sApi) {
    tWorkBook* wWorkBook = sApi.ActiveWorkBook();
    if (wWorkBook == nullptr) {
        return;
    }
    const tString wFontName = wWorkBook->DefaultFontName();
    const tDouble wFontSize = wWorkBook->DefaultFontSize();
    if (wFontName.empty()) {
        return;
    }
    for (auto& wPair : m_MapCss) {
        wPair.second = PrependDefaultFontCss(wPair.second, wFontName, wFontSize);
    }
}

/// @brief Save output file
tBool tExcel2SpreadSheet::SaveOutputFile(const tString& sXlsxPath, tApi& sApi) {
    // Replace extension by .sker
    tString wOutPath = sXlsxPath;
    tSize wSlash = wOutPath.find_last_of("/\\");
    tSize wDot = wOutPath.find_last_of('.');
    if (wDot != tString::npos && (wSlash == tString::npos || wDot > wSlash)) {
        wOutPath = wOutPath.substr(0, wDot) + ".sker";
    } else {
        wOutPath += ".sker";
    }

    tString wUri=sApi.ActiveWorkBook()->Uri();
    
    tString wJson = sApi.WriteJson(wUri);
    tFile wFile = tFile(wOutPath);
#if defined(__EXCEPTIONS) && __EXCEPTIONS
    try {
        wFile.SaveString(wJson);
    } catch (const std::exception& wE) {
        cerr << "Error: Failed to save output file: " << wE.what() << endl;
        return false;
    }
#else
    wFile.SaveString(wJson);
#endif
    if (!wFile.Exist() || wFile.Length() == 0) {
        cerr << "Error: output file not written: " << wOutPath << endl;
        return false;
    }
#ifdef debuginfo
    cout << "Output file saved successfully" << endl;
#endif
    return true;
}

/// @brief Import an .xlsx file into a SkSpreadSheet::tApi instance (creates sheets, sets values/formulas)
tBool tExcel2SpreadSheet::ImportXlsxToApi(const tString& sXlsxPath, tApi& sApi) {
    m_MapFormula.clear();
    m_MapFormulaCachedValue.clear();
    m_MapCss.clear();
    m_MapRangeNamed.clear();
    m_MapArrayFormulaOutput.clear();
    m_MapArrayFormulaOutputByOrigin.clear();
    m_TableMetadata.clear();
    m_CellsWithCalculatedColumn.clear();
    m_NamedRangesToCheck.clear();

    tExcelPugiXMLReader wReader;
    if (!LoadAndParseExcelFile(sXlsxPath, wReader)) {
        return false;
    }

    // Create sheets in the API
    CreateSheetsInApi(wReader, sApi);

    const tInt wSheetCountForExtract = wReader.GetWorksheetCount();
    for (tInt wS = 1; wS <= wSheetCountForExtract; ++wS) {
        const tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        const UnzippedFile* wSheetXml = wReader.GetUnzippedFile(wSheetFile);
        if (!wSheetXml || !wSheetXml->HasPayload()) {
            std::cerr << "SkExcel: worksheet XML was not extracted: " << wSheetFile << std::endl;
            return false;
        }
    }

    if (tWorkBook* wWorkBook = sApi.ActiveWorkBook()) {
        const auto& wFonts = wReader.GetFonts();
        if (!wFonts.empty() && !wFonts[0].name.empty()) {
            wWorkBook->DefaultFontName(wFonts[0].name);
            if (wFonts[0].size > 0.0) {
                wWorkBook->DefaultFontSize(wFonts[0].size);
            }
        }
    }

    // Process defined names (named ranges)
    GetDefinedNames(wReader, sApi);
    
    // Classic worksheet autoFilter -> RangeData (Wine-style lists, not ListObjects)
    // ProcessSheet must run first so header cell labels are available.
    // ca="1" cells are collected during ProcessSheet; tables run after so they skip those cells.
    tInt wSheetCount = wReader.GetWorksheetCount();
    for (tInt wS = 1; wS <= wSheetCount; ++wS) {
        // Reserve 0..90% for the per-sheet pass (the bulk of the work); the
        // remaining passes (autofilters, named ranges, styles) finish to 100%.
        if (wSheetCount > 0) {
            SkExcelReportProgress((wS - 1) * 90 / wSheetCount);
        }
        ProcessSheet(wReader, wS, sApi);
    }
    SkExcelReportProgress(90);

    ProcessStructuredTables(wReader, sApi);
    ProcessWorksheetAutoFilters(wReader, sApi);

    // Apply Named Range
    ApplyNamedRanges(sApi);

    // Project <tableStyles> formatting onto m_MapCss AFTER ProcessSheet so
    // overlay declarations land at the end of each cell's CSS string.
    // The CSS engine downstream deduplicates by keeping the last value,
    // so this ordering makes the table style win on the visual properties
    // it actually defines (header background+color, stripe background,
    // table borders) while leaving everything else (font name, format
    // string, text-align, padding, totals row fill from cellXfs[fillId])
    // untouched — those are not redefined by the overlay so they are not
    // overwritten.
    ApplyTableStyleOverlays(wReader, sApi);
#ifdef checksp
    sApi.Check();
#endif
  

    // Rewrite Excel anchor-style merged borders into our adjacency model
    // (border-right -> next col border-left, border-bottom -> next row border-top)
    // BEFORE the CSS is fed to the spreadsheet engine.
    AdjacencyConvertMergedBorders();
    // Same projection but for non-merged single cells: required because the
    // renderer reads each cell's own border-top / border-left and expects
    // the right / bottom edges on the cells of column+1 / row+1.
    AdjacencyConvertCellBorders();

    EnsureDefaultFontOnCssMap(sApi);

    // Apply CSS styles
    ApplyCssStyles(sApi);
    tWorkBook* wWorkBook = sApi.ActiveWorkBook();
#ifdef debugworkbook
    // Debug WorkBook
    cout << wWorkBook->Debug() << endl;
#endif
    // Apply formulas
    ApplyFormulas(sApi);
    if (m_RecalculateAtImport && wWorkBook != nullptr) {
#ifdef debuginfo
        cout << "RecalculateAll()" << endl;
#endif
        wWorkBook->RecalculateAll();
    } else {
        ApplyCachedFormulaValues(sApi);
#ifdef debuginfo
        cout << "RecalculateAll() skipped (Excel cached values applied)" << endl;
#endif
    }
#ifdef debuginfo
    cout << "->End()" << endl;
#endif

#ifdef debuginfo
    cout <<  "ApplySpaklines()" << endl;
#endif
    ApplySparklines(wReader, sApi);

#ifdef debuginfo
    cout <<  "ApplyImages()" << endl;
#endif
    ApplyImages(wReader, sApi);

#ifdef debuginfo
    cout <<  "ApplyTextBases()" << endl;
#endif
    ApplyTextBoxes(wReader, sApi);
    
#ifdef debuginfo
    cout <<  "ApplyCharts()" << endl;
#endif
    ApplyCharts(wReader, sApi);

#ifdef debuginfo
    cout <<  "SaveOuputFile(" << sXlsxPath << "))" << endl;
#endif
    if (!m_WriteSkerOnImport) {
        SkExcelReportProgress(100);
        return true;
    }
    // Save output file
    tBool wSaved = SaveOutputFile(sXlsxPath, sApi);
    SkExcelReportProgress(100);
    return wSaved;
}

void tExcel2SpreadSheet::RegisterJavascriptCellClasses(tApi& sApi) {
    // ~tApi() clears tClassFactory — re-register on every fresh Api (e.g. CalculateAll ReadJson).
    if (tClassFactory::Instance()->Get("SkCellClassSparkline") == nullptr) {
        sApi.RegisterClassAttribute("SkCellClassSparkline", "Sparkline", "Javascript");
        sApi.AddProperty("DataRange", "string", "Values range", 0, "", "range");
        sApi.AddProperty("showMarkers", "string", "Show markers", 1, "true");
    }
    if (tClassFactory::Instance()->Get("SkCellClassImage") == nullptr) {
        sApi.RegisterClassAttribute("SkCellClassImage", "Image", "Javascript");
        sApi.AddProperty("dataUrl", "string", "Image data URL", 0, "");
        sApi.AddProperty("altText", "string", "Alt text", 1, "");
        sApi.AddProperty("opacity", "string", "Opacity (0-1)", 2, "1");
        sApi.AddProperty("rotation", "string", "Rotation (degrees)", 3, "0");
    }
    if (tClassFactory::Instance()->Get("SkCellClassTextBox") == nullptr) {
        sApi.RegisterClassAttribute("SkCellClassTextBox", "TextBox", "Javascript");
        sApi.AddProperty("text", "string", "Text content", 0, "");
        sApi.AddProperty("color", "string", "Text color", 1, "#000000");
        sApi.AddProperty("fontSize", "string", "Font size (pt)", 2, "12");
        sApi.AddProperty("fontFamily", "string", "Font family", 3, "");
        sApi.AddProperty("textAlign", "string", "Text alignment", 4, "left");
        sApi.AddProperty("bold", "string", "Bold text", 5, "false");
    }
    if (tClassFactory::Instance()->Get("SkCellClassLineChart") == nullptr) {
        sApi.RegisterClassAttribute("SkCellClassLineChart", "LineChart", "Javascript");
        sApi.AddProperty("Title", "string", "Title", 0, "");
        sApi.AddProperty("chartData", "string", "Labels range (or A:B combined)", 1, "", "range");
        sApi.AddProperty("DataRange", "string", "Values range (or A:B combined)", 2, "", "range");
        sApi.AddProperty("seriesLabels", "string", "Series labels range", 3, "", "range");
        sApi.AddProperty("chartType", "string", "Chart type", 4, "line", "enum:line,bar,area");
        sApi.AddProperty("barDirection", "string", "Bar direction", 5, "vertical", "enum:vertical,horizontal");
    }
    if (tClassFactory::Instance()->Get("SkCellClassPieChart") == nullptr) {
        sApi.RegisterClassAttribute("SkCellClassPieChart", "PieChart", "Javascript");
        sApi.AddProperty("Title", "string", "Title", 0, "");
        sApi.AddProperty("chartData", "string", "Labels range (or A:B combined)", 1, "", "range");
        sApi.AddProperty("DataRange", "string", "Values range (or A:B combined)", 2, "", "range");
    }
}

namespace {

tString UniqueImageFloatingName(const tExcelImageEntry& sEntry, tSize sIndex, std::set<tString>& sUsed) {
    tString wStem = "Img";
    for (tChar wCh : sEntry.Name) {
        if (std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_') {
            wStem.push_back(wCh);
        } else if (wCh == '.' || wCh == '-') {
            wStem.push_back('_');
        }
    }
    if (wStem.size() <= 3) {
        wStem += std::to_string(sIndex + 1);
    }
    tString wCandidate = wStem;
    tSize wSuffix = 1;
    while (sUsed.find(wCandidate) != sUsed.end()) {
        wCandidate = wStem + "_" + std::to_string(wSuffix++);
    }
    sUsed.insert(wCandidate);
    return wCandidate;
}

tString UniqueTextBoxFloatingName(const tExcelTextBoxEntry& sEntry, tSize sIndex, std::set<tString>& sUsed) {
    tString wStem = "Txt";
    for (tChar wCh : sEntry.Name) {
        if (std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_') {
            wStem.push_back(wCh);
        } else if (wCh == '.' || wCh == '-') {
            wStem.push_back('_');
        }
    }
    if (wStem.size() <= 3) {
        wStem += std::to_string(sIndex + 1);
    }
    tString wCandidate = wStem;
    tSize wSuffix = 1;
    while (sUsed.find(wCandidate) != sUsed.end()) {
        wCandidate = wStem + "_" + std::to_string(wSuffix++);
    }
    sUsed.insert(wCandidate);
    return wCandidate;
}

tBool IsBrowserImageType(const tExcelImageEntry& sEntry) {
    if (sEntry.DataUrl.empty() || sEntry.Base64.empty()) {
        return false;
    }
    const tString wType = sEntry.ImageType;
    if (wType == "EMF" || wType == "WMF") {
        return false;
    }
    const tString wMime = sEntry.MimeType;
    return wMime.find("image/") == 0;
}

tString UniqueChartFloatingName(const tExcelChartEntry& sEntry, tSize sIndex, std::set<tString>& sUsed) {
    tString wStem = "Chart";
    for (tChar wCh : sEntry.Name) {
        if (std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_') {
            wStem.push_back(wCh);
        } else if (wCh == '.' || wCh == '-' || wCh == ' ') {
            wStem.push_back('_');
        }
    }
    if (wStem.size() <= 5) {
        wStem += std::to_string(sIndex + 1);
    }
    tString wCandidate = wStem;
    tSize wSuffix = 1;
    while (sUsed.find(wCandidate) != sUsed.end()) {
        wCandidate = wStem + "_" + std::to_string(wSuffix++);
    }
    sUsed.insert(wCandidate);
    return wCandidate;
}

} // namespace

void tExcel2SpreadSheet::ApplyImages(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    RegisterJavascriptCellClasses(sApi);
    const std::vector<tExcelImageEntry> wEntries = sReader.CollectImages();
    if (wEntries.empty()) {
        return;
    }

    tSheet* wPrevSheet = sApi.ActiveSheet();
    std::set<tString> wUsedNames;
    tSize wIndex = 0;

    for (const tExcelImageEntry& wEntry : wEntries) {
        if (!IsBrowserImageType(wEntry)) {
            continue;
        }
        if (wEntry.SheetName.empty()) {
            continue;
        }
        tSheet* wSheet = sApi.ActiveWorkBook()->Sheet(wEntry.SheetName);
        if (wSheet == nullptr) {
            continue;
        }

        tInt wRow = wEntry.AnchorRow;
        tInt wCol = wEntry.AnchorCol;
        if (wRow <= 0 || wCol <= 0) {
            if (!wEntry.Position.empty() && wEntry.Position != "unknown") {
                const std::pair<tInt, tInt> wRc = RefToRowCol(wEntry.Position);
                wRow = wRc.first;
                wCol = wRc.second;
            }
        }
        if (wRow <= 0) {
            wRow = 1;
        }
        if (wCol <= 0) {
            wCol = 1;
        }

        tString wCellRef = wEntry.Position;
        if (wCellRef.empty() || wCellRef == "unknown") {
            wCellRef = Base10ToAlpha(wCol) + std::to_string(wRow);
        }
        const tString wAnchorRef = wEntry.SheetName + "!" + wCellRef;

        const tDouble wWidth = wEntry.WidthPx > 0 ? static_cast<tDouble>(wEntry.WidthPx) : 120.0;
        const tDouble wHeight = wEntry.HeightPx > 0 ? static_cast<tDouble>(wEntry.HeightPx) : 80.0;
        const tString wObjectName = UniqueImageFloatingName(wEntry, wIndex++, wUsedNames);

        sApi.ActiveSheet(wEntry.SheetName);
        if (!sApi.UndoInsertFloatingObject(
                wObjectName,
                "SkCellClassImage",
                wEntry.SheetName,
                wSheet,
                wEntry.DiffX,
                wEntry.DiffY,
                wWidth,
                wHeight,
                1.0,
                wAnchorRef)) {
            continue;
        }
        sApi.UndoFloatingObjectAttribute(wObjectName, "dataUrl", tVariant(wEntry.DataUrl));
        if (!wEntry.Name.empty()) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "altText", tVariant(wEntry.Name));
        }
    }

    if (wPrevSheet != nullptr) {
        sApi.ActiveSheet(wPrevSheet->Name());
    }
}

void tExcel2SpreadSheet::ApplyTextBoxes(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    RegisterJavascriptCellClasses(sApi);
    const std::vector<tExcelTextBoxEntry> wEntries = sReader.CollectTextBoxes();
    if (wEntries.empty()) {
        return;
    }

    tSheet* wPrevSheet = sApi.ActiveSheet();
    std::set<tString> wUsedNames;
    tSize wIndex = 0;

    for (const tExcelTextBoxEntry& wEntry : wEntries) {
        if (wEntry.SheetName.empty() || wEntry.Text.empty()) {
            continue;
        }
        tSheet* wSheet = sApi.ActiveWorkBook()->Sheet(wEntry.SheetName);
        if (wSheet == nullptr) {
            continue;
        }

        tInt wRow = wEntry.AnchorRow;
        tInt wCol = wEntry.AnchorCol;
        if (wRow <= 0 || wCol <= 0) {
            if (!wEntry.Position.empty() && wEntry.Position != "unknown") {
                const std::pair<tInt, tInt> wRc = RefToRowCol(wEntry.Position);
                wRow = wRc.first;
                wCol = wRc.second;
            }
        }
        if (wRow <= 0) {
            wRow = 1;
        }
        if (wCol <= 0) {
            wCol = 1;
        }

        tString wCellRef = wEntry.Position;
        if (wCellRef.empty() || wCellRef == "unknown") {
            wCellRef = Base10ToAlpha(wCol) + std::to_string(wRow);
        }
        const tString wAnchorRef = wEntry.SheetName + "!" + wCellRef;

        const tDouble wWidth = wEntry.WidthPx > 0 ? static_cast<tDouble>(wEntry.WidthPx) : 200.0;
        const tDouble wHeight = wEntry.HeightPx > 0 ? static_cast<tDouble>(wEntry.HeightPx) : 60.0;
        const tString wObjectName = UniqueTextBoxFloatingName(wEntry, wIndex++, wUsedNames);

        sApi.ActiveSheet(wEntry.SheetName);
        if (!sApi.UndoInsertFloatingObject(
                wObjectName,
                "SkCellClassTextBox",
                wEntry.SheetName,
                wSheet,
                wEntry.DiffX,
                wEntry.DiffY,
                wWidth,
                wHeight,
                1.0,
                wAnchorRef)) {
            continue;
        }
        sApi.UndoFloatingObjectAttribute(wObjectName, "text", tVariant(wEntry.Text));
        if (!wEntry.Color.empty()) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "color", tVariant(wEntry.Color));
        }
        if (wEntry.FontSize > 0.0) {
            sApi.UndoFloatingObjectAttribute(
                wObjectName,
                "fontSize",
                tVariant(std::to_string(static_cast<tInt>(std::round(wEntry.FontSize)))));
        }
        if (!wEntry.FontFamily.empty()) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "fontFamily", tVariant(wEntry.FontFamily));
        }
        if (!wEntry.TextAlign.empty()) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "textAlign", tVariant(wEntry.TextAlign));
        }
        if (wEntry.Bold) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "bold", tVariant("true"));
        }
    }

    if (wPrevSheet != nullptr) {
        sApi.ActiveSheet(wPrevSheet->Name());
    }
}

namespace {

constexpr tDouble kMinHorizontalBarChartWidthPx = 720.0;

void ResolveChartLayoutPx(tApi& sApi, tSheet* sSheet, const tExcelChartEntry& sEntry, tDouble& oWidth, tDouble& oHeight) {
    oWidth = sEntry.WidthPx > 0 ? static_cast<tDouble>(sEntry.WidthPx) : 0.0;
    oHeight = sEntry.HeightPx > 0 ? static_cast<tDouble>(sEntry.HeightPx) : 0.0;

    if (sSheet == nullptr) {
        if (oWidth <= 0.0) {
            oWidth = 420.0;
        }
        if (oHeight <= 0.0) {
            oHeight = 260.0;
        }
        return;
    }

    if (oWidth <= 0.0 || oHeight <= 0.0) {
        if (sEntry.AnchorCol > 0 && sEntry.ToAnchorCol > 0 && sEntry.AnchorRow > 0 && sEntry.ToAnchorRow > 0) {
            sApi.ActiveSheet(sSheet->Name());
            // ToDiffX/ToDiffY are offsets from the *leading* edge of the closing
            // cell, so the span stops at that cell rather than covering it. Both
            // SumPixel* helpers are inclusive on their end bound, hence the -1:
            // summing through ToAnchorCol would add a whole extra column on top
            // of the offset and let the chart bleed past its Excel footprint.
            if (oWidth <= 0.0) {
                const tDouble wColSpan =
                    sApi.SumPixelWidth(sEntry.AnchorCol, sEntry.ToAnchorCol - 1, sSheet);
                oWidth = wColSpan + sEntry.ToDiffX - sEntry.DiffX;
            }
            if (oHeight <= 0.0) {
                const tDouble wRowSpan =
                    sApi.SumPixelHeight(sEntry.AnchorRow, sEntry.ToAnchorRow - 1, sSheet);
                oHeight = wRowSpan + sEntry.ToDiffY - sEntry.DiffY;
            }
        }
    }

    if (oWidth <= 0.0) {
        oWidth = 420.0;
    }
    if (oHeight <= 0.0) {
        oHeight = 260.0;
    }
    if (sEntry.BarDirection == "horizontal" && oWidth < kMinHorizontalBarChartWidthPx) {
        oWidth = kMinHorizontalBarChartWidthPx;
    }
}

} // namespace

void tExcel2SpreadSheet::ApplyCharts(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    RegisterJavascriptCellClasses(sApi);
    const std::vector<tExcelChartEntry> wEntries = sReader.CollectCharts();
    if (wEntries.empty()) {
        return;
    }

    tSheet* wPrevSheet = sApi.ActiveSheet();
    std::set<tString> wUsedNames;
    tSize wIndex = 0;

    for (const tExcelChartEntry& wEntry : wEntries) {
        if (wEntry.SheetName.empty() || wEntry.Series.empty()) {
            continue;
        }
        tSheet* wSheet = sApi.ActiveWorkBook()->Sheet(wEntry.SheetName);
        if (wSheet == nullptr) {
            continue;
        }

        tString wChartData;
        tString wDataRange;
        tString wSeriesLabels;
        if (!ChartImport::BuildSkerChartRanges(wEntry, wChartData, wDataRange, wSeriesLabels)) {
            continue;
        }

        tInt wRow = wEntry.AnchorRow;
        tInt wCol = wEntry.AnchorCol;
        if (wRow <= 0 || wCol <= 0) {
            if (!wEntry.Position.empty() && wEntry.Position != "unknown") {
                const std::pair<tInt, tInt> wRc = RefToRowCol(wEntry.Position);
                wRow = wRc.first;
                wCol = wRc.second;
            }
        }
        if (wRow <= 0) {
            wRow = 1;
        }
        if (wCol <= 0) {
            wCol = 1;
        }

        tString wCellRef = wEntry.Position;
        if (wCellRef.empty() || wCellRef == "unknown") {
            wCellRef = Base10ToAlpha(wCol) + std::to_string(wRow);
        }
        const tString wAnchorRef = wEntry.SheetName + "!" + wCellRef;

        tDouble wWidth = 420.0;
        tDouble wHeight = 260.0;
        ResolveChartLayoutPx(sApi, wSheet, wEntry, wWidth, wHeight);

        const tString wObjectName = UniqueChartFloatingName(wEntry, wIndex++, wUsedNames);

        const tBool wIsPie = (wEntry.SkerChartType == "pie");
        const tString wClassName = wIsPie ? "SkCellClassPieChart" : "SkCellClassLineChart";

        sApi.ActiveSheet(wEntry.SheetName);
        if (!sApi.UndoInsertFloatingObject(
                wObjectName,
                wClassName,
                wEntry.SheetName,
                wSheet,
                wEntry.DiffX,
                wEntry.DiffY,
                wWidth,
                wHeight,
                1.0,
                wAnchorRef)) {
            continue;
        }

        if (!wEntry.Title.empty() && wEntry.Title.find('!') == tString::npos && wEntry.Title.find('=') == tString::npos) {
            sApi.UndoFloatingObjectAttribute(wObjectName, "Title", tVariant(wEntry.Title));
        }

        if (!sApi.UndoFloatingObjectAttribute(wObjectName, "chartData", tVariant(wChartData))) {
            continue;
        }
        if (!sApi.UndoFloatingObjectAttribute(wObjectName, "DataRange", tVariant(wDataRange))) {
            continue;
        }

        if (!wIsPie) {
            if (!wSeriesLabels.empty() && wEntry.Series.size() > 1) {
                sApi.UndoFloatingObjectAttribute(wObjectName, "seriesLabels", tVariant(wSeriesLabels));
            }
            tString wChartType = wEntry.SkerChartType;
            if (wChartType != "line" && wChartType != "bar" && wChartType != "area") {
                wChartType = "bar";
            }
            sApi.UndoFloatingObjectAttribute(wObjectName, "chartType", tVariant(wChartType));
            if (wEntry.BarDirection == "horizontal") {
                sApi.UndoFloatingObjectAttribute(wObjectName, "barDirection", tVariant("horizontal"));
            }
        }
    }

    if (wPrevSheet != nullptr) {
        sApi.ActiveSheet(wPrevSheet->Name());
    }
}

void tExcel2SpreadSheet::ApplySparklines(const tExcelPugiXMLReader& sReader, tApi& sApi) {
    RegisterJavascriptCellClasses(sApi);
    const std::vector<tSparklineEntry> wEntries = sReader.CollectSparklines();
    if (wEntries.empty()) {
        return;
    }
    tSheet* wPrevSheet = sApi.ActiveSheet();
    for (const tSparklineEntry& wEntry : wEntries) {
        if (wEntry.SheetName.empty() || wEntry.CellRef.empty() || wEntry.SourceRange.empty()) {
            continue;
        }
        tSheet* wSheet = sApi.ActiveWorkBook()->Sheet(wEntry.SheetName);
        if (wSheet == nullptr) {
            continue;
        }
        sApi.ActiveSheet(wEntry.SheetName);
        if (!sApi.UndoCellClass(wEntry.CellRef, "SkCellClassSparkline", wSheet)) {
            continue;
        }
        tString wFormula = "=DATARANGE(" + wEntry.SourceRange + ")";
        wFormula = QualifyRefsForSheet(wEntry.SheetName, wFormula);
        sApi.UndoCellAttribute(wEntry.CellRef, "DataRange", tVariant(wFormula), wSheet);
        if (wEntry.Markers) {
            sApi.UndoCellAttribute(wEntry.CellRef, "showMarkers", tVariant("true"), wSheet);
        }
    }
    if (wPrevSheet != nullptr) {
        sApi.ActiveSheet(wPrevSheet->Name());
    }
}

const tTableMetadata* tExcel2SpreadSheet::GetTableMetadata(const tString& sTableName) const {
    auto it = m_TableMetadata.find(sTableName);
    if (it != m_TableMetadata.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<tString> tExcel2SpreadSheet::GetTableNames() const {
    std::vector<tString> wNames;
    for (const auto& pair : m_TableMetadata) {
        wNames.push_back(pair.first);
    }
    return wNames;
}

}

