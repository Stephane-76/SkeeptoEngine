//==============================================================================
// TestSkExcel
//==============================================================================
#include "../include/TestSkExcel.hpp"
#include <SkPrintParameters.hpp>
#include <SkTools.hpp>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>
#include <cstring>
#include <rapidjson/document.h>
#include <SkFormatCssApi.hpp>
#include <SkFormatRoot.hpp>
#include <SkInterfaceWeb.hpp>
#include <SkMessage.hpp>
#include <SkSpreadSheet.hpp>

using namespace SkFormat;

namespace {

// Directory containing AmortBis.sker / Budget.sker for Excel round-trip tests.
// Native: SKER_EXCEL_TEST_DIR, then <repo>/File/ (SKER_FILE_DIR), then ~/Projects/Excel.
// Emscripten: no host FS — CMake preloads *.sker from File/ into
// /home/web_user/Projects/Excel/.
tString ExcelTestDataDir() {
    const char* wDir = std::getenv("SKER_EXCEL_TEST_DIR");
    if (wDir != nullptr && wDir[0] != '\0') {
        tString p(wDir);
        while (!p.empty() && (p.back() == '/' || p.back() == '\\'))
            p.pop_back();
        if (!p.empty())
            p += '/';
        return p;
    }
    const char* wDisk = std::getenv("SKER_DISK_PATH");
    if (wDisk != nullptr && wDisk[0] != '\0') {
        tString wFull(wDisk);
        const tSize wPos = wFull.find_last_of("/\\");
        if (wPos != tString::npos)
            return wFull.substr(0, wPos + 1);
    }
#if defined(SKER_FILE_DIR) && !defined(__EMSCRIPTEN__)
    {
        tString p(SKER_FILE_DIR);
        while (!p.empty() && (p.back() == '/' || p.back() == '\\'))
            p.pop_back();
        if (!p.empty()) {
            p += '/';
            return p;
        }
    }
#endif
    const char* wHome = std::getenv("HOME");
#if defined(_WIN32) || defined(_WIN64)
    if (wHome == nullptr || wHome[0] == '\0')
        wHome = std::getenv("USERPROFILE");
#endif
    if (wHome != nullptr && wHome[0] != '\0') {
        tString p(wHome);
        if (!p.empty() && p.back() != '/' && p.back() != '\\')
            p += '/';
        p += "Projects/Excel/";
        return p;
    }
#if defined(__EMSCRIPTEN__)
    // Default MEMFS layout when HOME is unset (matches typical --preload-file ...@/home/web_user/Projects/Excel/)
    return "/home/web_user/Projects/Excel/";
#endif
    return {};
}

tString WorkBookUriFromSkerJson(const tString& sJson) {
    rapidjson::Document wDocument;
    if (wDocument.Parse(sJson.c_str()).HasParseError() || !wDocument.HasMember("uri")) {
        return {};
    }
    return wDocument["uri"].GetString();
}

/**
 * PDFMetrics needs any workbook with realistic print params + columns; prefer Budget-familial for
 * BottomRight parity with Node. WASM link preload often ships only Amort.sker into MEMFS.
 */
tString PdfMetricsSkerPathOrEmpty() {
    const tString wDir = ExcelTestDataDir();
    static const char* const kNames[] = {"Budget-familial.sker", "Budget.sker", "Amort.sker"};
    for (const char* wName : kNames) {
        const tString wPath = wDir + wName;
        if (tFile(wPath).Exist())
            return wPath;
    }
    return {};
}

/** OOXML paperSize portrait width × height (mm); subset aligned with sker-app printParametersToPdfOptions.mjs. */
static bool PaperPortraitMm(tInt code, tDouble* outW, tDouble* outH) {
    switch (code) {
        case 1:
        case 2:
        case 14:
        case 15:
            *outW = 215.9;
            *outH = 279.4;
            return true;
        case 5:
            *outW = 215.9;
            *outH = 355.6;
            return true;
        case 8:
            *outW = 297.0;
            *outH = 420.0;
            return true;
        case 9:
        case 10:
            *outW = 210.0;
            *outH = 297.0;
            return true;
        case 11:
            *outW = 148.0;
            *outH = 210.0;
            return true;
        default:
            *outW = 215.9;
            *outH = 279.4;
            return false;
    }
}

// Drawable content box in CSS px (@96 dpi): paper minus left/right/top/bottom margins (inches → mm).
// Matches sker-app `drawablePageCssPxFromPrintParameters()` — header/footer (`marginHeader` / `marginFooter`)
// are OOXML bands for header/footer *content*, not subtracted again here (Excel grid often uses the full
// margin box; subtracting mh/mf made tiles shorter than Node PDF helpers and typical Print Preview rows).
static void DrawablePxFromPrintParameters(const tPrintParameters& p, tDouble kMmPerInch, tDouble kCssPxPerInch,
                                            tDouble* outDrawableWPx, tDouble* outDrawableHPx, bool* outKnownPaper) {
    tDouble wPortraitW = 215.9;
    tDouble wPortraitH = 279.4;
    *outKnownPaper = PaperPortraitMm(p.m_PaperSize, &wPortraitW, &wPortraitH);
    tDouble wPageWMm = wPortraitW;
    tDouble wPageHMm = wPortraitH;
    if (p.m_Orientation == tPrintOrientation::Landscape) {
        std::swap(wPageWMm, wPageHMm);
    }
    const tDouble ml = p.m_MarginLeft * kMmPerInch;
    const tDouble mr = p.m_MarginRight * kMmPerInch;
    const tDouble mt = p.m_MarginTop * kMmPerInch;
    const tDouble mb = p.m_MarginBottom * kMmPerInch;
    const tDouble wDrawableWMm = std::max(0.0, wPageWMm - ml - mr);
    const tDouble wDrawableHMm = std::max(0.0, wPageHMm - mt - mb);
    *outDrawableWPx = wDrawableWMm * kCssPxPerInch / kMmPerInch;
    *outDrawableHPx = wDrawableHMm * kCssPxPerInch / kMmPerInch;
}

// Horizontal band from leftCol: always include at least one column (even if wider than max WPx) so pagination advances.
static bool ComputeColBandPx(tColRowCellRange* range, tIndex leftCol, tIndex lastCol, tDouble maxWPx,
                             tDouble* outWidthPx, tIndex* outRightCol) {
    if (leftCol > lastCol)
        return false;
    *outWidthPx = 0;
    *outRightCol = leftCol - 1;
    for (tIndex c = leftCol; c <= lastCol; ++c) {
        const tDouble wColW = range->SizeCol(c, tUnitMetrics::pixels);
        if (*outWidthPx > 1e-12 && *outWidthPx + wColW > maxWPx + 1e-6)
            break;
        *outWidthPx += wColW;
        *outRightCol = c;
    }
    return *outRightCol >= leftCol;
}

// Vertical band from topRow: always include at least one row so pagination advances.
static bool ComputeRowBandPx(tColRowCellRange* range, tIndex topRow, tIndex lastRow, tDouble maxHPx,
                             tDouble* outHeightPx, tIndex* outBottomRow) {
    if (topRow > lastRow)
        return false;
    *outHeightPx = 0;
    *outBottomRow = topRow - 1;
    for (tIndex r = topRow; r <= lastRow; ++r) {
        const tDouble wRowH = range->SizeRow(r, tUnitMetrics::pixels);
        if (*outHeightPx > 1e-12 && *outHeightPx + wRowH > maxHPx + 1e-6)
            break;
        *outHeightPx += wRowH;
        *outBottomRow = r;
    }
    return *outBottomRow >= topRow;
}

// JsonView payload: rows[].cells[] with c_r, c_c, optional c_f, c_v (see SkJsonView / tVariant::JsonJavaScript).
static bool TryJsonMemberInt(const rapidjson::Value& o, const char* k, int& out) {
    if (!o.HasMember(k))
        return false;
    const rapidjson::Value& v = o[k];
    if (v.IsInt()) {
        out = v.GetInt();
        return true;
    }
    if (v.IsUint()) {
        out = static_cast<int>(v.GetUint());
        return true;
    }
    return false;
}

#if 0
static tIndex JsonToCellIndex(const rapidjson::Value& v) {
    if (v.IsInt() && v.GetInt() >= 0)
        return static_cast<tIndex>(v.GetInt());
    if (v.IsUint())
        return static_cast<tIndex>(v.GetUint());
    if (v.IsInt64() && v.GetInt64() >= 0)
        return static_cast<tIndex>(v.GetInt64());
    if (v.IsUint64())
        return static_cast<tIndex>(v.GetUint64());
    return 0;
}

static tString JsonViewValueToDisplayString(const rapidjson::Value& v) {
    if (v.IsNull())
        return "(null)";
    if (v.IsString())
        return tString(v.GetString(), v.GetStringLength());
    if (v.IsBool())
        return v.GetBool() ? tString("true") : tString("false");
    if (v.IsInt())
        return std::to_string(v.GetInt());
    if (v.IsUint())
        return std::to_string(v.GetUint());
    if (v.IsInt64())
        return std::to_string(v.GetInt64());
    if (v.IsUint64())
        return std::to_string(v.GetUint64());
    if (v.IsDouble()) {
        std::ostringstream wOs;
        wOs << std::setprecision(15) << v.GetDouble();
        return wOs.str();
    }
    if (v.IsObject())
        return tString("(object)");
    if (v.IsArray())
        return tString("(array)");
    return tString("(?)");
}

struct JsonViewCellsDumpOptions {
    std::optional<tIndex> row;
    std::optional<tIndex> col;
    bool includeFormula = false;
};

static tString DumpJsonViewCellsListing(const tString& sJsonView,
                                        JsonViewCellsDumpOptions sOpts = {}) {
    tStringStream wStream;
    rapidjson::Document wDoc;
    wDoc.Parse(sJsonView.c_str());
    if (wDoc.HasParseError() || !wDoc.IsObject()) {
        wStream << "DumpJsonViewCellsListing: invalid JSON\n";
        return (wStream.str());
    }
    if (sOpts.includeFormula)
        wStream << "ref\tformula\tvalue\n";
    else
        wStream << "ref\tvalue\n";
    const rapidjson::Value& wRows = wDoc["rows"];
    for (rapidjson::SizeType wRi = 0; wRi < wRows.Size(); ++wRi) {
        const rapidjson::Value& wRow = wRows[wRi];
        if (!wRow.IsObject() || !wRow.HasMember("cells") || !wRow["cells"].IsArray())
            continue;
        const rapidjson::Value& wCells = wRow["cells"];
        for (rapidjson::SizeType wCi = 0; wCi < wCells.Size(); ++wCi) {
            const rapidjson::Value& wC = wCells[wCi];
            if (!wC.IsObject() || !wC.HasMember("c_r") || !wC.HasMember("c_c"))
                continue;
            const tIndex wR = JsonToCellIndex(wC["c_r"]);
            const tIndex wCol = JsonToCellIndex(wC["c_c"]);
            if (sOpts.row.has_value() && wR != *sOpts.row)
                continue;
            if (sOpts.col.has_value() && wCol != *sOpts.col)
                continue;
            tTempoPoint wPt(wR, wCol);
            const tString wRef = wPt.StrRef();
            tString wFormula;
            if (wC.HasMember("c_f") && wC["c_f"].IsString())
                wFormula.assign(wC["c_f"].GetString(), wC["c_f"].GetStringLength());
            tString wVal = "(no c_v)";
            if (wC.HasMember("c_v"))
                wVal = JsonViewValueToDisplayString(wC["c_v"]);
            wStream << wRef << '\t';
            if (sOpts.includeFormula)
                wStream << (wFormula.empty() ? tString("(no formula)") : wFormula) << '\t';
            wStream << wVal << '\n';
        }
    }
    return(wStream.str());
}
#endif

std::vector<tString> SplitLinesForDiff(const tString& s) {
    std::vector<tString> lines;
    tSize beg = 0;
    for (tSize i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '\n') {
            tString line = s.substr(beg, i - beg);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(std::move(line));
            beg = i + 1;
        }
    }
    return lines;
}

tString EscapeForDiffDisplay(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    if (c == '\n')
        return "\\n";
    if (c == '\r')
        return "\\r";
    if (c == '\t')
        return "\\t";
    if (u < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\x%02x", u);
        return tString(buf);
    }
    return tString(1, c);
}

// Unified diff from LCS on lines (GitHub / git diff style). Returns false if skipped (too large).
bool TryPrintLineUnifiedDiff(const tString& sLabelLeft, const std::vector<tString>& a,
                             const tString& sLabelRight, const std::vector<tString>& b, std::ostream& os) {
    const tSize n = a.size();
    const tSize m = b.size();
    const tSize kMaxProduct = 2000000; // ~8MB dp table of int32
    if (n == 0 && m == 0) {
        os << "=== diff: both empty ===\n";
        return true;
    }
    if (n * m > kMaxProduct) {
        return false;
    }
    std::vector<std::vector<tInt>> dp(n + 1, std::vector<tInt>(m + 1, 0));
    for (tSize i = 1; i <= n; ++i) {
        for (tSize j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1])
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
        }
    }
    struct Step {
        char prefix; // ' ' context, '-' removed, '+' added
        tString line;
    };
    std::vector<Step> steps;
    tSize i = n, j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i - 1] == b[j - 1] && dp[i][j] == dp[i - 1][j - 1] + 1) {
            steps.push_back({' ', a[i - 1]});
            --i;
            --j;
        } else if (i > 0 && (j == 0 || dp[i - 1][j] >= dp[i][j - 1])) {
            steps.push_back({'-', a[i - 1]});
            --i;
        } else {
            steps.push_back({'+', b[j - 1]});
            --j;
        }
    }
    std::reverse(steps.begin(), steps.end());

    os << "=== diff: " << sLabelLeft << " vs " << sLabelRight << " ===\n";
    os << "--- " << sLabelLeft << " (" << n << " lines)\n";
    os << "+++ " << sLabelRight << " (" << m << " lines)\n";
    os << " (prefix: space=context, -=only left, +=only right)\n";

    for (const Step& st : steps)
        os << st.prefix << st.line << "\n";
    return true;
}

void PrintDiffPreamble(const tString& sLabelLeft, const tString& sLeft, const tString& sLabelRight,
                       const tString& sRight, const std::vector<tString>& la, const std::vector<tString>& lb,
                       std::ostream& os) {
    os << "\n*** STRINGS DIFFER (no assertion => test can still report OK) ***\n";
    os << "  " << sLabelLeft << ": " << sLeft.size() << " bytes\n";
    os << "  " << sLabelRight << ": " << sRight.size() << " bytes\n";
    tSize fd = 0;
    const tSize nmin = std::min(sLeft.size(), sRight.size());
    while (fd < nmin && sLeft[fd] == sRight[fd])
        ++fd;
    os << "  first differing byte index: " << fd;
    if (fd < nmin)
        os << " (" << EscapeForDiffDisplay(sLeft[fd]) << " vs " << EscapeForDiffDisplay(sRight[fd]) << ")";
    else if (sLeft.size() != sRight.size())
        os << " (one string ended; lengths differ)";
    os << "\n";
    if (la.size() == 1 && lb.size() == 1) {
        os << "  note: single-line JSON on both sides — '-' and '+' are the full documents.\n";
        os << "  common cause: different iteration order (e.g. namedranges) while data is equivalent.\n";
    }
}

void PrintCharLevelFirstMismatch(const tString& sLabelLeft, const tString& left,
                                 const tString& sLabelRight, const tString& right, std::ostream& os) {
    os << "=== diff (character scope): " << sLabelLeft << " vs " << sLabelRight << " ===\n";
    os << "length " << sLabelLeft << "=" << left.size() << "  " << sLabelRight << "=" << right.size() << "\n";
    tSize k = 0;
    const tSize n = std::min(left.size(), right.size());
    while (k < n && left[k] == right[k])
        ++k;
    if (k == n && left.size() == right.size()) {
        os << "(strings are identical)\n";
        return;
    }
    const tSize window = 72;
    tSize start = (k > window) ? k - window : 0;
    auto dumpWindow = [&](const tString& s, const char* tag) {
        os << "--- " << tag << " (escaped window around byte " << k << ") ---\n";
        tString vis;
        vis.reserve(window * 4);
        tSize end = std::min(s.size(), start + window * 2);
        for (tSize i = start; i < end; ++i)
            vis += EscapeForDiffDisplay(s[i]);
        os << vis << "\n";
    };
    dumpWindow(left, sLabelLeft.c_str());
    dumpWindow(right, sLabelRight.c_str());
    if (k < left.size() && k < right.size()) {
        os << "first mismatch: '" << EscapeForDiffDisplay(left[k]) << "' (0x"
           << std::hex << (static_cast<unsigned>(static_cast<unsigned char>(left[k])) & 0xff) << std::dec
           << ") vs '" << EscapeForDiffDisplay(right[k]) << "' (0x"
           << std::hex << (static_cast<unsigned>(static_cast<unsigned char>(right[k])) & 0xff) << std::dec
           << ")\n";
    } else if (k >= left.size()) {
        os << "(end of left string; right has " << (right.size() - k) << " more bytes)\n";
    } else {
        os << "(end of right string; left has " << (left.size() - k) << " more bytes)\n";
    }
}

void RegisterExcelWorkBookCellClasses(tApi* sApi) {
    if (tClassFactory::Instance()->Get("SkCellClassSparkline") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassSparkline", "Sparkline", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassImage") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassImage", "Image", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassTextBox") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassTextBox", "TextBox", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassLineChart") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassLineChart", "LineChart", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassPieChart") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassPieChart", "PieChart", "Javascript");
    }
}

// Release cell/sheet css refs while FormatApi is still attached (WorkBook::DeleteCellFormat).
static void ClearAllWorkBooksForFormatTeardown() {
    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    tVectorWorkBookClass wWorkBooks;
    wContainer->WorkBooksList(wWorkBooks);
    for (tWorkBook* wWorkBook : wWorkBooks) {
        if (wWorkBook != nullptr) {
            wWorkBook->Clear();
        }
    }
}

static void ShutdownFormatApi(tFormatApi* sFormatApi) {
    if (sFormatApi == nullptr) {
        return;
    }
    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    if (wContainer->FormatApi() != sFormatApi) {
        wContainer->FormatApi(sFormatApi);
    }
    // Undo SaveCells hold ColRowCellRange* — clear undo before WorkBook::Clear().
    tApplication::Instance()->ClearUndoRedo();
    ClearAllWorkBooksForFormatTeardown();
    wContainer->FormatApi(nullptr);
    delete(sFormatApi);
}

// Budget "Rapport financier": delete rows 16–22 (7 rows) then undo — regression for empty/duplicated rows.
static tString SnapshotCellForDeleteRowTest(tApi& sApi, const tString& sRef) {
    tCell* wCell = sApi.Cell(sRef);
    if (wCell == nullptr) {
        return tString("<null>");
    }
    tStringStream wStream;
    wStream << wCell->FormulaStr() << "|" << wCell->Value();
    return wStream.str();
}

static tString SnapshotRowForDeleteRowTest(tApi& sApi, tIndex sRow) {
    static const char* const kCols[] = {"B", "C", "D", "E", "F", "H"};
    tStringStream wStream;
    for (const char* wCol : kCols) {
        tString wRef = tString(wCol) + std::to_string(static_cast<long long>(sRow));
        wStream << wRef << "=" << SnapshotCellForDeleteRowTest(sApi, wRef) << ";";
    }
    return wStream.str();
}

static tString SnapshotRowDisplayValues(tApi& sApi, tIndex sRow) {
    static const char* const kCols[] = {"B", "C", "D", "E", "F", "H"};
    tStringStream wStream;
    for (const char* wCol : kCols) {
        tString wRef = tString(wCol) + std::to_string(static_cast<long long>(sRow));
        tCell* wCell = sApi.Cell(wRef);
        if (wCell == nullptr) {
            wStream << "<null>;";
        } else {
            wStream << wCell->Value() << ';';
        }
    }
    return wStream.str();
}

static tBool CellHasContent(const tString& sSnapshot) {
    if (sSnapshot == "<null>") {
        return false;
    }
    const tSize wSep = sSnapshot.find('|');
    if (wSep == tString::npos) {
        return false;
    }
    const tString wFormula = sSnapshot.substr(0, wSep);
    const tString wValue = sSnapshot.substr(wSep + 1);
    if (!wFormula.empty()) {
        return true;
    }
    return !wValue.empty() && wValue != "0" && wValue != "0.0";
}

static void AssertBudgetDeleteRowUndoScenarioSized(tApi& sApi, const tString& sPrefix,
    tIndex sDeleteRow, tIndex sDeleteSize) {
    const tIndex sShiftedRow = sDeleteRow + sDeleteSize;

    sApi.ActiveSheet("Rapport financier");

    tStringStream wBeforeDeleteBlock;
    for (tIndex wRow = sDeleteRow; wRow < sDeleteRow + sDeleteSize; ++wRow) {
        wBeforeDeleteBlock << SnapshotRowForDeleteRowTest(sApi, wRow) << '\n';
    }
    const tString wBeforeDeleteBlockStr = wBeforeDeleteBlock.str();
    const tString wBeforeShiftedRowSnapshot = SnapshotRowForDeleteRowTest(sApi, sShiftedRow);
    const tString wBeforeShiftedRowValues = SnapshotRowDisplayValues(sApi, sShiftedRow);
    const tString wBeforeB16 = SnapshotCellForDeleteRowTest(sApi, "B16");
    const tString wBeforeB17 = SnapshotCellForDeleteRowTest(sApi, "B17");
    const tString wBeforeD16 = SnapshotCellForDeleteRowTest(sApi, "D16");

    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": D16 must have content before delete", CellHasContent(wBeforeD16));

    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": UndoDeleteRow must succeed",
        sApi.UndoDeleteRow(sDeleteRow, sDeleteSize));

    const tString wAfterDeleteValues = SnapshotRowDisplayValues(sApi, sDeleteRow);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": after delete row " + std::to_string(sDeleteRow) + " values must match former row " +
            std::to_string(sShiftedRow),
        wAfterDeleteValues == wBeforeShiftedRowValues);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": after delete D16 must differ from snapshot before delete",
        SnapshotCellForDeleteRowTest(sApi, "D16") != wBeforeD16);

    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": Undo() must succeed", sApi.Undo());

    tStringStream wAfterUndoBlock;
    for (tIndex wRow = sDeleteRow; wRow < sDeleteRow + sDeleteSize; ++wRow) {
        wAfterUndoBlock << SnapshotRowForDeleteRowTest(sApi, wRow) << '\n';
    }
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": deleted rows must match snapshot after undo",
        wAfterUndoBlock.str() == wBeforeDeleteBlockStr);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": shifted row must match snapshot after undo",
        SnapshotRowForDeleteRowTest(sApi, sShiftedRow) == wBeforeShiftedRowSnapshot);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": D16 must have content after undo (not empty rows)",
        CellHasContent(SnapshotCellForDeleteRowTest(sApi, "D16")));
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": B16/B17 labels must not be duplicated after undo",
        SnapshotCellForDeleteRowTest(sApi, "B16") == wBeforeB16 &&
        SnapshotCellForDeleteRowTest(sApi, "B17") == wBeforeB17 &&
        wBeforeB16 != wBeforeB17);

    if (tRange* wCfTail = sApi.FindRange("B19:I40")) {
        CPPUNIT_ASSERT_MESSAGE(
            sPrefix + ": B19:I40 CF tail must not be merged after undo",
            !wCfTail->IsMerged());
    }
    for (tIndex wRow = sDeleteRow; wRow < sDeleteRow + sDeleteSize; ++wRow) {
        for (const char* wCol : {"B", "D", "H"}) {
            tString wRef = tString(wCol) + std::to_string(static_cast<long long>(wRow));
            tCell* wCell = sApi.Cell(wRef);
            if (wCell != nullptr) {
                const tString wFormula = wCell->FormulaStr();
                CPPUNIT_ASSERT_MESSAGE(
                    sPrefix + ": " + wRef + " must not contain #REF! after undo",
                    wFormula.find("#REF!") == tString::npos);
            }
        }
    }
}

static void AssertBudgetDeleteRowUndoScenario(tApi& sApi, const tString& sPrefix) {
    AssertBudgetDeleteRowUndoScenarioSized(sApi, sPrefix, 16, 7);
}

static void AssertBudgetDeleteRowUndoViaGetMessage(tInterfaceWeb& sInterface, const tString& sPrefix) {
    static constexpr tIndex kDeleteRow = 16;
    static constexpr tIndex kDeleteSize = 5;

    sInterface.ActiveSheet("Rapport financier");
    sInterface.ActiveWorkBook()->RecalculateAll();

    tStringStream wBeforeBlock;
    for (tIndex wRow = kDeleteRow; wRow < kDeleteRow + kDeleteSize; ++wRow) {
        wBeforeBlock << SnapshotRowForDeleteRowTest(sInterface, wRow) << '\n';
    }
    const tString wBeforeBlockStr = wBeforeBlock.str();
    const tString wBeforeD16 = SnapshotCellForDeleteRowTest(sInterface, "D16");

    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": UndoDeleteRow must succeed", sInterface.UndoDeleteRow(kDeleteRow, kDeleteSize));

    tUndo* wUndo = sInterface.LastUndo();
    auto* wUndoSpreadSheet = dynamic_cast<tUndoSpreadSheet*>(wUndo);
    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": delete row undo must be on stack", wUndoSpreadSheet != nullptr);
    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": rebase before JSON", wUndoSpreadSheet->Rebase());

#ifdef TestMultiUser
    tMessage wMessage("u@test.fr", sInterface.ActiveWorkBook()->Uri(), "Undo");
#else
    tMessage wMessage("u@test.fr", "Test", "User", sInterface.ActiveWorkBook()->Uri(), "Undo");
#endif
    wUndoSpreadSheet->IsUndo(true);
    wUndoSpreadSheet->IsJson(true);
    wMessage.WriteJson(wUndoSpreadSheet);
    wUndoSpreadSheet->IsJson(false);

    // Server / collaborative path: post-delete state, apply undo from JSON only.
    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": GetMessage undo must succeed", sInterface.GetMessage(wMessage.Json()));
#ifdef checksp
    sInterface.Check();
#endif

    tStringStream wAfterBlock;
    for (tIndex wRow = kDeleteRow; wRow < kDeleteRow + kDeleteSize; ++wRow) {
        wAfterBlock << SnapshotRowForDeleteRowTest(sInterface, wRow) << '\n';
    }
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": rows must match snapshot after GetMessage undo",
        wAfterBlock.str() == wBeforeBlockStr);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": D16 must have content after GetMessage undo",
        CellHasContent(SnapshotCellForDeleteRowTest(sInterface, "D16")));
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": D16 must match before delete",
        SnapshotCellForDeleteRowTest(sInterface, "D16") == wBeforeD16);
}

static void AssertBudgetDeleteRowByRectUndoViaGetMessage(tInterfaceWeb& sInterface, const tString& sPrefix,
    const tRect& sDeleteRect, tIndex sSnapshotTopRow, tIndex sSnapshotBottomRow) {
    const tIndex kTopRow = sSnapshotTopRow;
    const tIndex kBottomRow = sSnapshotBottomRow;
    const tRect wDeleteRect = sDeleteRect;

    sInterface.ActiveSheet("Rapport financier");
    sInterface.ActiveWorkBook()->RecalculateAll();

    tStringStream wBeforeBlock;
    for (tIndex wRow = kTopRow; wRow <= kBottomRow; ++wRow) {
        wBeforeBlock << SnapshotRowForDeleteRowTest(sInterface, wRow) << '\n';
    }
    const tString wBeforeBlockStr = wBeforeBlock.str();
    const tString wBeforeD15 = SnapshotCellForDeleteRowTest(sInterface, "D15");
    const tString wBeforeD16 = SnapshotCellForDeleteRowTest(sInterface, "D16");
    const tString wBeforeB16 = SnapshotCellForDeleteRowTest(sInterface, "B16");
    const tString wBeforeB17 = SnapshotCellForDeleteRowTest(sInterface, "B17");

    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": UndoDeleteRowByRect must succeed",
        sInterface.UndoDeleteRowByRect(wDeleteRect));

    tUndo* wUndo = sInterface.LastUndo();
    auto* wUndoSpreadSheet = dynamic_cast<tUndoSpreadSheet*>(wUndo);
    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": delete row by rect undo must be on stack", wUndoSpreadSheet != nullptr);
    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": rebase before JSON", wUndoSpreadSheet->Rebase());

#ifdef TestMultiUser
    tMessage wMessage("u@test.fr", sInterface.ActiveWorkBook()->Uri(), "Undo");
#else
    tMessage wMessage("u@test.fr", "Test", "User", sInterface.ActiveWorkBook()->Uri(), "Undo");
#endif
    wUndoSpreadSheet->IsUndo(true);
    wUndoSpreadSheet->IsJson(true);
    wMessage.WriteJson(wUndoSpreadSheet);
    wUndoSpreadSheet->IsJson(false);

    CPPUNIT_ASSERT_MESSAGE(sPrefix + ": GetMessage undo must succeed", sInterface.GetMessage(wMessage.Json()));
#ifdef checksp
    sInterface.Check();
#endif

    tStringStream wAfterBlock;
    for (tIndex wRow = kTopRow; wRow <= kBottomRow; ++wRow) {
        wAfterBlock << SnapshotRowForDeleteRowTest(sInterface, wRow) << '\n';
    }
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": rows must match snapshot after GetMessage undo",
        wAfterBlock.str() == wBeforeBlockStr);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": D15 must match before delete",
        SnapshotCellForDeleteRowTest(sInterface, "D15") == wBeforeD15);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": D16 must match before delete",
        SnapshotCellForDeleteRowTest(sInterface, "D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE(
        sPrefix + ": B16/B17 labels must not be duplicated after undo",
        SnapshotCellForDeleteRowTest(sInterface, "B16") == wBeforeB16 &&
        SnapshotCellForDeleteRowTest(sInterface, "B17") == wBeforeB17);
    for (tIndex wRow = kTopRow; wRow <= kBottomRow; ++wRow) {
        for (const char* wCol : {"B", "D", "H"}) {
            tString wRef = tString(wCol) + std::to_string(static_cast<long long>(wRow));
            tCell* wCell = sInterface.Cell(wRef);
            if (wCell != nullptr) {
                CPPUNIT_ASSERT_MESSAGE(
                    sPrefix + ": " + wRef + " must not contain #REF! after undo",
                    wCell->FormulaStr().find("#REF!") == tString::npos);
            }
        }
    }
}

} // namespace

// We can send it to the API of a feature
TestSkExcel::TestSkExcel() :CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr), m_FormatApi(nullptr) {
}


void  TestSkExcel::DebugCell(tString sRef) {
    return; //Drop
    tCell* wCell=m_Api->Cell(sRef);
    cout <<  sRef;
    if (wCell!=nullptr) {
        if (wCell->FormulaStr()!="") cout << ":(" << wCell->FormulaStr() << ")";
        cout << "=" << wCell->Value();
    }
    cout << endl;
}

void TestSkExcel::DumpStringDiffGitHubStyle(const tString& sLabelLeft, const tString& sLeft,
                                            const tString& sLabelRight, const tString& sRight) const {
    if (sLeft == sRight) {
        std::cout << "=== diff: strings are identical (" << sLeft.size() << " bytes) ===\n";
        return;
    }
    const std::vector<tString> la = SplitLinesForDiff(sLeft);
    const std::vector<tString> lb = SplitLinesForDiff(sRight);
    PrintDiffPreamble(sLabelLeft, sLeft, sLabelRight, sRight, la, lb, std::cout);

    tSize maxLine = 0;
    for (const tString& x : la)
        maxLine = std::max(maxLine, x.size());
    for (const tString& x : lb)
        maxLine = std::max(maxLine, x.size());
    // Very long single lines: avoid printing megabytes as two '-' / '+' rows
    static constexpr tSize kMaxFullSingleLinePrint = 12000;
    if (la.size() == 1 && lb.size() == 1 &&
        (sLeft.size() > kMaxFullSingleLinePrint || sRight.size() > kMaxFullSingleLinePrint)) {
        std::cout << "\n(full line diff omitted — use pretty-printed JSON or a canonical compare)\n\n";
        PrintCharLevelFirstMismatch(sLabelLeft, sLeft, sLabelRight, sRight, std::cout);
        return;
    }
    // Very long single line for LCS memory: still bounded by product below
    static constexpr tSize kMaxLineCharsForLineDiff = 256000;
    const tBool wCanLineDiff =
        maxLine <= kMaxLineCharsForLineDiff && la.size() * lb.size() <= 2000000;
    if (wCanLineDiff && TryPrintLineUnifiedDiff(sLabelLeft, la, sLabelRight, lb, std::cout))
        return;
    PrintCharLevelFirstMismatch(sLabelLeft, sLeft, sLabelRight, sRight, std::cout);
}

void TestSkExcel::DrawCell(tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow;
            //cout << "=" << wFormula;
            cout << ":" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkExcel::UndoOperation() {
	return; // Drop
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}

void TestSkExcel::TestFormula() {
    tString wTitle="TestFormula";
    tString wSubTitle="function IF(cond,ok,not ok)";
    
    tString wFormula="=IF(A1=123.45,\"ok\",\"not ok\")";
    if (!m_Api->UndoCellValue("A2", wFormula)) {
        wSubTitle="Error Compil "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    //DebugCell("A2");
 
    tCell* wCellA2=m_Api->Cell("A2");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, "="+wCellA2->FormulaStr() == wFormula);

    m_Api->AddSheet("Entrées des données financières");
    m_Api->ActiveSheet("Entrées des données financières");
    
    m_Api->UndoCellValue("B14", "COUCOU");
    
    m_Api->ActiveSheet("Sheet1");
                                
    m_Api->UndoCellValue("B14", "=IF('Entrées des données financières'!B14='COUCOU','OK','Not Ok')");
    //DebugCell("B14");
  
}


void TestSkExcel::TestFunction() {
    tString wTitle="TestFormula";
    tString wSubTitle="xlfn.FORMULATEXT()";
    // Bug ??? change in futur
    tString wFormula="=xfn.FORMULATEXT(A1)";
    //tString wFormula="=FORMULATEXT(A1)";
    if (m_Api->UndoCellValue("A2", wFormula)) {
        wSubTitle="Error accept  Compil "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    //DebugCell("A2");
    
}

#ifndef __EMSCRIPTEN__
void TestSkExcel::LoadPret() {
    //return;
    // Just xcode
    tString wFileName = ExcelTestDataDir() + "PretBis.sker";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
     
        m_Api->ReadJson(wJson);
        tWorkBook* wWorkBook=m_Api->ActiveWorkBook();
        //cout << wWorkBook->Debug() << endl;
        tRange* wRange=wWorkBook->FindRangeNamed("ValeursEntrées");
        //cout << wRange->StrRef(true) << endl;
        DrawCell("Read Excel", 1, 1, 23, 10);
        
        tString TestWrite=m_Api->ActiveWorkBook()->WriteJson();
        
        tFile wFile(ExcelTestDataDir() + "PretBis.Json");
        wFile.SaveString(wJson);
#ifdef checksp
        m_Api->Check();
#endif
    } else {
        //cout << wFileName << "Dont't exist" << endl;
    }
}


void TestSkExcel::LoadJsonResult() {
   tFile wFile( ExcelTestDataDir() + "PretBis.json");
   tString wJsonStr=wFile.LoadString();
    //cout << wJsonStr << endl;
    m_Api->ReadJson(wJsonStr);
#ifdef checksp
        m_Api->Check();
#endif
    DrawCell("Read Json Excel", 1, 1, 7, 10);
}


#endif

void TestSkExcel::LoadAmort() {
   // Just xcode
    tString wFileName = ExcelTestDataDir() + "AmortBis.sker";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
     
     
        m_Api->ReadJson(wJson);
        tWorkBook* wWorkBook=m_Api->ActiveWorkBook();
        //cout << wWorkBook->Debug() << endl;
        tString wUri=wWorkBook->Uri();
        tString wJsonWrite=m_Api->ActiveWorkBook()->WriteJson();
        
        tFile wFile( ExcelTestDataDir() + "AmortBis.json");
        wFile.SaveString(wJsonWrite);
#ifdef checksp
        m_Api->Check();
#endif
        DrawCell("Read Amort ", 1, 1, 23, 10);
        
        tApplication::Instance()->Locale("fr");
        tearDown();
        setUp();
        tApplication::Instance()->Locale("fr");
        
        // Same name as SaveString (case-sensitive FS / Linux / some CI)
        wFileName =  ExcelTestDataDir()+"AmortBis.json";
        tFile wFileJson(wFileName);
        tString wJsonRead=wFileJson.LoadString();
        //cout << wJsonStr << endl;
        m_Api->ReadJson(wJsonRead);
        
        tString wJsonResult=m_Api->WriteJson(wUri);
        
        wWorkBook=m_Api->ActiveWorkBook();
        //cout << wWorkBook->Debug() << endl;
        
        DumpStringDiffGitHubStyle("WriteJson (first pass)", wJsonWrite, "WriteJson (after re-read)", wJsonResult);
        CPPUNIT_ASSERT_MESSAGE(
            tString("LoadAmort: WriteJson round-trip must be byte-identical (first WriteJson vs second after ReadJson(AmortBis.json); see diff above if it fails)"),
            wJsonWrite == wJsonResult);
            
        
        // Test JsSon View
        tString wJsonView=  m_Api->JsonView(13, 1, tUnitMetrics::pixels, 300, 600,0,0, true);
        //cout << wJsonView << endl;
        //cout << DumpJsonViewCellsListing(wJsonView, { .col = 4, .includeFormula = true });
        // Examples: one sheet row, with formulas — DumpJsonViewCellsListing(wJsonView, { .row = 14, .includeFormula = true });
        //            one column only — DumpJsonViewCellsListing(wJsonView, { .col = 3, .includeFormula = true });
        //            row + column — DumpJsonViewCellsListing(wJsonView, { .row = 14, .col = 3, .includeFormula = true });
        //for(tInt wRow=1;wRow<34;wRow++) {
        //    for(tInt wCol=1;wCol<=10;wCol++)
        //        wJsonView = m_Api->JsonView(wRow, wCol, tUnitMetrics::pixels, 1200, 600,0,0, true);
        //}
        DrawCell("Read Amort ", 1, 1, 23, 10);
        
    } else {
        cout << "\nLoadAmort: file not found:\n  " << wFileName << "\n";
#if defined(__EMSCRIPTEN__)
        cout << "  Emscripten: the .sker must be bundled into the virtual FS. Configure CMake with:\n"
             << "    -DSKER_WASM_PRELOAD_EXCEL_DIR=/absolute/path/to/folder/containing/AmortBis.sker\n"
             << "  (adds --preload-file ...@/home/web_user/Projects/Excel/AmortBis.sker at link time).\n";
#else
        cout << "  Set SKER_EXCEL_TEST_DIR to that directory, or SKER_DISK_PATH to a .sker file inside it"
             << " (same idea as Node SkTestSpreadSheet.mjs).\n";
#endif
    }
    
   
}

void TestSkExcel::LoadBudget() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudget: file not found, skipping:\n  " << wFileName << endl;
        cout << "  Native: place Budget.sker in " << ExcelTestDataDir()
             << " or set SKER_EXCEL_TEST_DIR.\n";
#if defined(__EMSCRIPTEN__)
        cout << "  WASM: rebuild SkSpreadSheet_test with SKER_WASM_PRELOAD_EXCEL_DIR containing Budget.sker.\n";
#endif
        return;
    }
    const tString wJson = wFile.LoadString();
    m_Api->ReadJson(wJson);

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    CPPUNIT_ASSERT_MESSAGE("LoadBudget: workbook must exist after ReadJson", wWorkBook != nullptr);

    m_Api->ActiveSheet("Rapport financier");

    auto wSnapshotCell = [&](const tString& sRef) -> tString {
        tCell* wCell = m_Api->Cell(sRef);
        if (wCell == nullptr) {
            return tString("<null>");
        }
        tStringStream wStream;
        wStream << wCell->FormulaStr() << "|" << wCell->Value();
        return wStream.str();
    };

    const tString wBeforeD15 = wSnapshotCell("D15");
    const tString wBeforeD16 = wSnapshotCell("D16");
    const tString wBeforeH16 = wSnapshotCell("H16");

    CPPUNIT_ASSERT(m_Api->Copy("B16:F20"));
    CPPUNIT_ASSERT(m_Api->UndoPaste("B42"));

    const tString wAfterPasteD15 = wSnapshotCell("D15");
    CPPUNIT_ASSERT_MESSAGE(
        tString("LoadBudget: D15 unchanged after copy/paste elsewhere: ") + wAfterPasteD15 + " != " + wBeforeD15,
        wAfterPasteD15 == wBeforeD15);
    CPPUNIT_ASSERT_MESSAGE("LoadBudget: D16 unchanged after copy/paste elsewhere",
        wSnapshotCell("D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE("LoadBudget: H16 unchanged after copy/paste elsewhere",
        wSnapshotCell("H16") == wBeforeH16);

    CPPUNIT_ASSERT(m_Api->UndoMove("D16", "D17"));
    CPPUNIT_ASSERT(m_Api->Undo());
    const tString wAfterMoveUndoD15 = wSnapshotCell("D15");
    CPPUNIT_ASSERT_MESSAGE(
        tString("LoadBudget: D15 unchanged after move undo: ") + wAfterMoveUndoD15 + " != " + wBeforeD15,
        wAfterMoveUndoD15 == wBeforeD15);
    CPPUNIT_ASSERT_MESSAGE("LoadBudget: D16 unchanged after move undo",
        wSnapshotCell("D16") == wBeforeD16);
}

void TestSkExcel::LoadBudgetInterfaceWeb() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetInterfaceWeb: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wJson = wFile.LoadString();
    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWeb: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveWorkBook()->RecalculateAll();

    wInterface.ActiveSheet("Rapport financier");
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWeb: D16 must exist after load",
        wInterface.Cell("D16") != nullptr);

    auto wSnapshotCell = [&](const tString& sRef) -> tString {
        tCell* wCell = wInterface.Cell(sRef);
        if (wCell == nullptr) {
            return tString("<null>");
        }
        tStringStream wStream;
        wStream << wCell->FormulaStr() << "|" << wCell->Value();
        return wStream.str();
    };

    const tString wBeforeD16 = wSnapshotCell("D16");
    const tString wBeforeH16 = wSnapshotCell("H16");

    CPPUNIT_ASSERT(wInterface.Copy("B16:F20"));
    CPPUNIT_ASSERT(wInterface.UndoPaste("B42"));

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWeb: D16 unchanged after copy/paste elsewhere",
        wSnapshotCell("D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWeb: H16 unchanged after copy/paste elsewhere",
        wSnapshotCell("H16") == wBeforeH16);

    CPPUNIT_ASSERT(wInterface.UndoMove("D16", "D17"));
    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWeb: D16 unchanged after move undo",
        wSnapshotCell("D16") == wBeforeD16);

    // Client() keeps undo on m_UndoRedoContainer; clear it before workbook teardown.
    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRowUndo() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowUndo: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();

    // Direct tApi path (in-memory SaveSelect on Undo).
    m_Api->ReadJson(wJson);
    m_Api->ActiveWorkBook()->RecalculateAll();
    AssertBudgetDeleteRowUndoScenario(*m_Api, "LoadBudgetDeleteRowUndo/Api");

    // tInterfaceWeb path (Client + Rebase, same as browser WASM when solo).
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowUndo: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveWorkBook()->RecalculateAll();
    AssertBudgetDeleteRowUndoScenario(wInterface, "LoadBudgetDeleteRowUndo/InterfaceWeb");

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRowUndo16_19() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowUndo16_19: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();

    m_Api->ReadJson(wJson);
    m_Api->ActiveWorkBook()->RecalculateAll();
    AssertBudgetDeleteRowUndoScenarioSized(*m_Api, "LoadBudgetDeleteRowUndo16_19/Api", 16, 4);

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowUndo16_19: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveWorkBook()->RecalculateAll();
    AssertBudgetDeleteRowUndoScenarioSized(wInterface, "LoadBudgetDeleteRowUndo16_19/InterfaceWeb", 16, 4);

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

static tString MergedRangeRefAtCell(tApi& sApi, const tString& sRef) {
    tIndex wRow = 0;
    tIndex wCol = 0;
    if (!ParseCell(sRef, wRow, wCol)) {
        return tString("<bad-ref>");
    }
    tSheet* wSheet = sApi.ActiveSheet();
    if (wSheet == nullptr) {
        return tString("<no-sheet>");
    }
    tRange* wRange = wSheet->MergedRange(wRow, wCol);
    if (wRange == nullptr || !wRange->IsMerged()) {
        return tString("<none>");
    }
    return wRange->StrRef();
}

void TestSkExcel::LoadBudgetDeleteRowByRectMergeD16() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowByRectMergeD16: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowByRectMergeD16: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveSheet("Rapport financier");
    wInterface.ActiveWorkBook()->RecalculateAll();

    const tString wExpectedMerge = "I16:L16";
    const tString wBeforeMerge = MergedRangeRefAtCell(wInterface, "I16");
    CPPUNIT_ASSERT_MESSAGE(
        "LoadBudgetDeleteRowByRectMergeD16: I16 must be merged before delete",
        wBeforeMerge == wExpectedMerge);

    const tRect wDeleteRect(16, 4, 16, 4); // D16 only — shift up inside column D
    CPPUNIT_ASSERT_MESSAGE(
        "LoadBudgetDeleteRowByRectMergeD16: UndoDeleteRowByRect(D16) must succeed",
        wInterface.UndoDeleteRowByRect(wDeleteRect));
    CPPUNIT_ASSERT_MESSAGE(
        "LoadBudgetDeleteRowByRectMergeD16: I16:L16 must stay after delete outside column D",
        MergedRangeRefAtCell(wInterface, "I16") == wExpectedMerge);

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowByRectMergeD16: Undo() must succeed", wInterface.Undo());
    const tString wAfterUndoMerge = MergedRangeRefAtCell(wInterface, "I16");
    CPPUNIT_ASSERT_MESSAGE(
        "LoadBudgetDeleteRowByRectMergeD16: I16:L16 must be restored after undo (got "
            + wAfterUndoMerge + ")",
        wAfterUndoMerge == wBeforeMerge);
#ifdef checksp
    wInterface.Check();
#endif

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRowByRectUndoGetMessage() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowByRectUndoGetMessage: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowByRectUndoGetMessage: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    AssertBudgetDeleteRowByRectUndoViaGetMessage(
        wInterface,
        "LoadBudgetDeleteRowByRectUndoGetMessage",
        tRect(15, 4, 17, 6),
        15,
        17);

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRowByRectD16UndoGetMessage() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowByRectD16UndoGetMessage: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowByRectD16UndoGetMessage: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    AssertBudgetDeleteRowByRectUndoViaGetMessage(
        wInterface,
        "LoadBudgetDeleteRowByRectD16UndoGetMessage",
        tRect(16, 4, 16, 4),
        15,
        17);

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRowUndoGetMessage() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRowUndoGetMessage: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRowUndoGetMessage: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    AssertBudgetDeleteRowUndoViaGetMessage(wInterface, "LoadBudgetDeleteRowUndoGetMessage");

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteRow7_10() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteRow7_10: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    static constexpr tIndex kDeleteRow = 7;
    static constexpr tIndex kDeleteSize = 4; // rows 7 through 10

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRow7_10: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveSheet("Rapport financier");
    wInterface.ActiveWorkBook()->RecalculateAll();

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRow7_10: UndoDeleteRow(7:10) must succeed",
        wInterface.UndoDeleteRow(kDeleteRow, kDeleteSize));
#ifdef checksp
    wInterface.Check();
#endif

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteRow7_10: Undo() must succeed", wInterface.Undo());
#ifdef checksp
    wInterface.Check();
#endif

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteColF() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteColF: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    static constexpr tIndex kColF = 6;
    static constexpr tIndex kDeleteSize = 1;

    const tString wJson = wFile.LoadString();

    // Browser-like path (tInterfaceWeb + Client).
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColF: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveSheet("Rapport financier");
    wInterface.ActiveWorkBook()->RecalculateAll();

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColF: UndoDeleteCol(F) must succeed",
        wInterface.UndoDeleteCol(kColF, kDeleteSize));
#ifdef checksp
    wInterface.Check();
#endif

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColF: Undo() must succeed", wInterface.Undo());
#ifdef checksp
    wInterface.Check();
#endif

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteColHToL() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteColHToL: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    static constexpr tIndex kColH = 8;
    static constexpr tIndex kDeleteSize = 5; // H through L

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColHToL: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveSheet("Rapport financier");
    wInterface.ActiveWorkBook()->RecalculateAll();

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColHToL: UndoDeleteCol(H:L) must succeed",
        wInterface.UndoDeleteCol(kColH, kDeleteSize));
#ifdef checksp
    wInterface.Check();
#endif

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColHToL: Undo() must succeed", wInterface.Undo());
#ifdef checksp
    wInterface.Check();
#endif

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

void TestSkExcel::LoadBudgetDeleteColJToM() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetDeleteColJToM: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    static constexpr tIndex kColJ = 10;
    static constexpr tIndex kDeleteSize = 4; // J through M

    const tString wJson = wFile.LoadString();

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColJToM: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveSheet("Rapport financier");
    wInterface.ActiveWorkBook()->RecalculateAll();

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColJToM: UndoDeleteCol(J:M) must succeed",
        wInterface.UndoDeleteCol(kColJ, kDeleteSize));
#ifdef checksp
    wInterface.Check();
#endif

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetDeleteColJToM: Undo() must succeed", wInterface.Undo());
#ifdef checksp
    wInterface.Check();
#endif

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}

#ifdef TestMultiUser
void TestSkExcel::LoadBudgetInterfaceWebMultiUser() {
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetInterfaceWebMultiUser: file not found, skipping:\n  " << wFileName << endl;
        return;
    }

    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(true);
    wInterface.User("u@test.fr", "Test", "User");
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);

    const tString wJson = wFile.LoadString();
    const tString wUri = WorkBookUriFromSkerJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: Budget.sker must contain uri", !wUri.empty());
    wInterface.UriWorkBook(wUri);
    RegisterExcelWorkBookCellClasses(&wInterface);
    CPPUNIT_ASSERT(wInterface.ReadJson(wJson));
    wInterface.WorkBook(wUri);
    wInterface.ActiveWorkBook()->RecalculateAll();
    wInterface.ActiveSheet("Rapport financier");
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: D16 must exist after load",
        wInterface.Cell("D16") != nullptr);

    auto wSnapshotCell = [&](const tString& sRef) -> tString {
        tCell* wCell = wInterface.Cell(sRef);
        if (wCell == nullptr) {
            return tString("<null>");
        }
        tStringStream wStream;
        wStream << wCell->FormulaStr() << "|" << wCell->Value();
        return wStream.str();
    };

    const tString wBeforeD16 = wSnapshotCell("D16");
    const tString wBeforeH16 = wSnapshotCell("H16");

    CPPUNIT_ASSERT(wInterface.Copy("B16:F20"));
    CPPUNIT_ASSERT(wInterface.UndoPaste("B42"));

    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: D16 unchanged after copy/paste elsewhere",
        wSnapshotCell("D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: H16 unchanged after copy/paste elsewhere",
        wSnapshotCell("H16") == wBeforeH16);

    // Multi-user paste undo: PopUndoToRedoWithoutExecute + GetMessage (same as collaborative browser).
    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: D16 unchanged after paste undo",
        wSnapshotCell("D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: H16 unchanged after paste undo",
        wSnapshotCell("H16") == wBeforeH16);

    CPPUNIT_ASSERT(wInterface.UndoMove("D16", "D17"));
    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: D16 unchanged after move undo",
        wSnapshotCell("D16") == wBeforeD16);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetInterfaceWebMultiUser: D16 formula must reference Calculs",
        wSnapshotCell("D16").find("Calculs!") != tString::npos);

    wInterface.FormatApi(nullptr);
    wInterface.Clear();
    ShutdownFormatApi(wFormatApi);
}
#endif

void TestSkExcel::LoadBudgetNamedFormulas() {
    // Diagnostic: Budget.sker is auto-generated by SkExcel from Budget.xlsx and exercises two dynamic
    // named formulas — lstAnnées (OFFSET row) and lstMesures (OFFSET column with COUNTA on a sparse
    // column). The original Budget.xlsx never references them directly; instead Calculs!D3, D4 and
    // C6:G6 wrap them in MATCH(...,lstAnnées,0)+1. The user reported (Apr 2026) that loading the XLS
    // produces wrong results in those cells. This test loads the Budget.sker payload (skipped silently
    // if absent) and reports each scalar that depends on lstAnnées so we can see exactly which cell
    // misbehaves; only failures that match a known-bad signature trigger CPPUNIT_FAIL so the test is
    // safe to keep enabled when Budget.sker is missing on a developer machine.
    const tString wFileName = ExcelTestDataDir() + "Budget.sker";
    tFile wFile(wFileName);
    if (!wFile.Exist()) {
        cout << "\nLoadBudgetNamedFormulas: file not found, skipping:\n  " << wFileName << "\n";
        return;
    }
    const tString wJson = wFile.LoadString();
    m_Api->ReadJson(wJson);

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetNamedFormulas: workbook must exist after ReadJson", wWorkBook != nullptr);

    // Both names must be registered after the load; without them every probe is meaningless.
    tFormulaNamed* wFnAnnees = wWorkBook->FindFormulaNamed("lstAnnées");
    tFormulaNamed* wFnMesures = wWorkBook->FindFormulaNamed("lstMesures");
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetNamedFormulas: lstAnnées must be defined as a FormulaNamed", wFnAnnees != nullptr);
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetNamedFormulas: lstMesures must be defined as a FormulaNamed", wFnMesures != nullptr);

    // Switch to the Calculs sheet (where MATCH(...,lstAnnées,0)+1 lives).
    m_Api->ActiveSheet("Calculs");
    tSheet* wSheet = wWorkBook->ActiveSheet();
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetNamedFormulas: 'Calculs' sheet must exist", wSheet != nullptr);

    // Helper: dump the cell so a developer immediately sees value type and any formula error.
    auto wDescribe = [&](const tString& sRef) -> tString {
        tCell* wCell = m_Api->Cell(sRef);
        tStringStream wOut;
        wOut << "Calculs!" << sRef << " = ";
        if (wCell == nullptr) {
            wOut << "<no cell>";
            return wOut.str();
        }
        if (wCell->FormulaStr() != "") wOut << "(" << wCell->FormulaStr() << ") -> ";
        wOut << wCell->Value();
        wOut << " [type=" << static_cast<tInt>(wCell->Value().Type()) << "]";
        return wOut.str();
    };

    // Treat any t_error variant as a hard failure — that's what shows up as #ARG / #REF in the UI.
    auto wAssertNoError = [&](const tString& sRef) {
        tCell* wCell = m_Api->Cell(sRef);
        CPPUNIT_ASSERT_MESSAGE(tString("LoadBudgetNamedFormulas: missing cell ") + sRef, wCell != nullptr);
        const tBool wIsError = wCell->Value().Type() == tVariantType::t_error;
        if (wIsError) {
            cout << "\nLoadBudgetNamedFormulas: error in " << wDescribe(sRef) << "\n";
        }
        CPPUNIT_ASSERT_MESSAGE(tString("LoadBudgetNamedFormulas: ") + wDescribe(sRef) + " must not be an error",
                               !wIsError);
    };

    wAssertNoError("C6");
    wAssertNoError("G6");
    wAssertNoError("C15");

    // Copy must not trigger JsonCompil on named formulas (regression: Cmd+C broke lstAnnées / #NA).
    CPPUNIT_ASSERT_MESSAGE("LoadBudgetNamedFormulas: Copy Calculs!D3",
        m_Api->Copy("D3"));
    wAssertNoError("C6");
    wAssertNoError("G6");
}

void TestSkExcel::LoadConditionalHighlightCells() {
    // Disabled here because rendering CFs requires a tFormatApi (lives in libSkFormat) which
    // SkSpreadSheet_test doesn't link. The equivalent rendering test is
    // TestSkConditionalFormat::TestHighlightCellsRulesJsonViewRendering (uses Undo path) and
    // TestSkExcel::TestSkInterfaceBudget already exercises the .sker JSON load path for
    // DataBars / ColorScales (would crash with null FormatApi if either of the bugs fixed
    // alongside this stub regressed).
}
void TestSkExcel::PDFMetrics() {
    const tString wFileName = PdfMetricsSkerPathOrEmpty();
    if (wFileName.empty()) {
        cout << "\nPDFMetrics: no candidate workbook under Excel test dir, skipping:\n  "
             << ExcelTestDataDir() << "{Budget-familial,Budget,Amort}.sker\n";
        return;
    }
    tFile wFile(wFileName);
    cout << "\nPDFMetrics: workbook file:\n  " << wFileName << "\n";
    const tString wJson = wFile.LoadString();
    m_Api->ReadJson(wJson);
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();

    cout << "\n--- PDFMetrics (workbook → paper-sized JsonView bands; pixels @96/in; no whole-sheet viewport) ---\n";

    static constexpr tDouble kMmPerInch = 25.4;
    static constexpr tDouble kCssPxPerInch = 96.0;
    static constexpr tDouble kPdfPtPerInch = 72.0;
    static constexpr int kMaxTileJsonFilesPerScenario = 8;

    auto wDescribePaper = [](tInt code) -> const char* {
        switch (code) {
            case 9:
            case 10:
                return "A4";
            case 8:
                return "A3";
            case 11:
                return "A5";
            case 1:
            case 2:
                return "Letter";
            default:
                return "(unknown→Letter mm fallback)";
        }
    };

    auto wSlugSheetName = [](const tString& sName) -> tString {
        tStringStream wSlug;
        for (tChar ch : sName) {
            if (ch == ' ' || ch == '\t' || ch == '/') {
                wSlug << '_';
            } else {
                wSlug << ch;
            }
        }
        return wSlug.str();
    };

    auto wRunScenario = [&](const char* sScenarioTitle, const char* sDumpTag) {
        int wScenarioPages = 0;
        int wJsonFilesLeft = kMaxTileJsonFilesPerScenario;
        bool wScenarioPreviewShown = false;
        cout << sScenarioTitle << "\n";

        const tVectorSheet& wSheets = wWorkBook->VectorPtSheet();
        for (tSheet* wSh : wSheets) {
            if (wSh == nullptr || wSh->Name() == "_$$")
                continue;

            m_Api->ActiveSheet(wSh->Name());
            tColRowCellRange* wRange = wSh->ColRowCellRange();
            if (wRange == nullptr)
                continue;

            const tIndex wLastCol = wSh->LastCol();
            const tIndex wLastRow = wSh->LastRow();

            tPrintParameters p;
            {
                const tString wPJ = wSh->JsonPrintParameters();
                if (!wPJ.empty() && !p.JsonParse(wPJ)) {
                    p = tPrintParameters();
                }
            }

            bool wKnownPaper = false;
            tDouble wDrawableWPx = 0;
            tDouble wDrawableHPx = 0;
            DrawablePxFromPrintParameters(p, kMmPerInch, kCssPxPerInch, &wDrawableWPx, &wDrawableHPx, &wKnownPaper);

            tDouble wSumColPx = 0;
            for (tIndex c = 1; c <= wLastCol; ++c) {
                wSumColPx += wRange->SizeCol(c, tUnitMetrics::pixels);
            }
            tDouble wSumRowPx = 0;
            for (tIndex r = 1; r <= wLastRow; ++r) {
                wSumRowPx += wRange->SizeRow(r, tUnitMetrics::pixels);
            }

            cout << "\n  Sheet \"" << wSh->Name() << "\"  LastCol=" << wLastCol << " (" << Base10ToAlpha(wLastCol)
                 << ")  LastRow=" << wLastRow << "\n";
            // Same JSON as UISpreadSheet::JsonBottomRight(sheet) after SetSheet — compare Node/WASM logs.
            const tPoint wJsonBottomRight = m_Api->BottomRight(wSh);
            cout << "    JsonBottomRight parity: {\"r\":" << wJsonBottomRight.Row()
                 << ",\"c\":" << wJsonBottomRight.Col() << "}\n";
            cout << std::fixed << std::setprecision(3);
            cout << "    Sum SizeCol (loop): " << wSumColPx << " px\n";
            cout << "    Sum SizeRow (loop): " << wSumRowPx << " px\n";
            cout << "    printParameters: paperSize=" << p.m_PaperSize << " (" << wDescribePaper(p.m_PaperSize) << ")"
                 << "  orientation="
                 << (p.m_Orientation == tPrintOrientation::Landscape ? "Landscape" : "Portrait")
                 << "  pageOrder="
                 << (p.m_PageOrder == tPrintPageOrder::OverThenDown ? "OverThenDown" : "DownThenOver")
                 << "  fitToPage=" << (p.m_FitToPage ? "true" : "false") << "  scale=" << p.m_Scale << "%"
                 << "  usePrinterDefaults=" << (p.m_UsePrinterDefaults ? "true" : "false") << "\n";
            if (!wKnownPaper) {
                cout << "    (warning: paperSize not in PDFMetrics lookup table — using Letter portrait mm fallback)\n";
            }
            cout << "    Drawable CSS px (paper minus L/R/T/B margins; matches Node drawablePageCssPxFromPrintParameters): "
                 << wDrawableWPx << " x " << wDrawableHPx << "\n";

            const tDouble wScaleWpx = wDrawableWPx > 0 ? wDrawableWPx / wSumColPx : 0;
            const tDouble wScaleHpx = wDrawableHPx > 0 ? wDrawableHPx / wSumRowPx : 0;
            const tDouble wUniformFit = std::min(std::min(wScaleWpx, wScaleHpx), 1.0);
            cout << "    Uniform fit scale min(drawable/gridSum, 1): " << wUniformFit << "  pt/px@scale "
                 << (kPdfPtPerInch / kCssPxPerInch) * wUniformFit << "\n";

            // Sanity vs Excel Print Preview: raw JsonView column widths are CSS px; they often exceed drawable width at scale 100%.
            {
                constexpr tIndex kDiagCols = 5;
                constexpr tIndex kDiagRows = 35;
                tDouble wDiagWPx = 0;
                const tIndex wLimC = std::min(kDiagCols, wLastCol);
                for (tIndex c = 1; c <= wLimC; ++c) {
                    wDiagWPx += wRange->SizeCol(c, tUnitMetrics::pixels);
                }
                tDouble wDiagHPx = 0;
                const tIndex wLimR = std::min(kDiagRows, wLastRow);
                for (tIndex r = 1; r <= wLimR; ++r) {
                    wDiagHPx += wRange->SizeRow(r, tUnitMetrics::pixels);
                }
                cout << "    Diagnostic raw px through col " << wLimC << " (" << Base10ToAlpha(wLimC)
                     << "): width cumul=" << wDiagWPx << " vs drawable W=" << wDrawableWPx;
                if (wDiagWPx > wDrawableWPx + 1e-6) {
                    cout << "  → implicit shrink ~" << std::setprecision(1) << std::fixed
                         << (100.0 * wDrawableWPx / wDiagWPx) << "% for A.." << Base10ToAlpha(wLimC)
                         << " at native JsonView px (Excel preview often applies this)";
                }
                cout << "\n";
                cout << std::fixed << std::setprecision(3);
                cout << "    Diagnostic raw px through row " << wLimR << ": height cumul=" << wDiagHPx
                     << " vs drawable H=" << wDrawableHPx;
                if (wDiagHPx > wDrawableHPx + 1e-6) {
                    cout << "  → implicit shrink ~" << std::setprecision(1) << std::fixed
                         << (100.0 * wDrawableHPx / wDiagHPx) << "% for rows 1.." << wLimR << " at native px";
                }
                cout << "\n";
                cout << std::fixed << std::setprecision(3);
                cout << "    JsonView lastcol/lastrow are strict viewport-fit (may sit inside Excel's printed block).\n";
            }

            if (wDrawableWPx <= 1e-12 || wDrawableHPx <= 1e-12) {
                cout << "    Skip tiles: zero drawable area.\n";
                continue;
            }

            tIndex wTileSeq = 0;
            auto wEmitTile = [&](tIndex topRow, tIndex leftCol, tDouble viewHPx, tDouble viewWPx, tIndex bottomRow,
                                 tIndex rightCol) {
                ++wTileSeq;
                ++wScenarioPages;
                const tString wJv =
                    m_Api->JsonView(topRow, leftCol, tUnitMetrics::pixels, viewHPx, viewWPx, 0, 0, true, wSh);

                rapidjson::Document wJvDoc;
                wJvDoc.Parse(wJv.c_str(), static_cast<rapidjson::SizeType>(wJv.size()));
                int wJvLastCol = 0;
                int wJvLastRow = 0;
                const bool wJvBoundsOk =
                    !wJvDoc.HasParseError() && wJvDoc.IsObject() && TryJsonMemberInt(wJvDoc, "lastcol", wJvLastCol)
                    && TryJsonMemberInt(wJvDoc, "lastrow", wJvLastRow);

                cout << "    Page #" << wScenarioPages << " (sheet \"" << wSh->Name() << "\" tile #" << wTileSeq
                     << ") origin " << Base10ToAlpha(leftCol) << topRow << "  viewport px " << viewWPx << " x "
                     << viewHPx << "  bands cols " << leftCol << ".." << rightCol << " rows " << topRow << ".."
                     << bottomRow << "  JSON bytes=" << wJv.size();
                if (wJvBoundsOk) {
                    cout << "  lastcol=" << wJvLastCol << " (" << Base10ToAlpha(static_cast<tIndex>(wJvLastCol))
                         << ")  lastrow=" << wJvLastRow << "\n";
                    cout << "      bottom-right (viewport-fit from JsonView): "
                         << Base10ToAlpha(static_cast<tIndex>(wJvLastCol)) << wJvLastRow << "\n";
                } else {
                    cout << "  (lastcol/lastrow unavailable)\n";
                    cout << "      bottom-right (band extent fallback): " << Base10ToAlpha(rightCol) << bottomRow
                         << "\n";
                }

                if (wJsonFilesLeft > 0) {
                    --wJsonFilesLeft;
                    tStringStream wDumpPath;
                    wDumpPath << ExcelTestDataDir() << "PDFMetrics_JsonView_" << wSlugSheetName(wSh->Name()) << '_'
                              << sDumpTag << "_t" << wTileSeq << '_' << Base10ToAlpha(leftCol) << topRow << '_'
                              << static_cast<tInt>(viewWPx + 0.5) << 'x' << static_cast<tInt>(viewHPx + 0.5) << ".json";
                    const tString wDumpPathStr = wDumpPath.str();
                    tFile wDumpFile(wDumpPathStr);
                    wDumpFile.SaveString(wJv);
                    cout << "      dump: " << wDumpPathStr << "\n";

                    static constexpr size_t kPreviewChars = 800;
                    if (!wScenarioPreviewShown) {
                        wScenarioPreviewShown = true;
                        cout << "      JsonView preview (" << kPreviewChars << " chars, first tile of scenario):\n";
                        if (wJv.size() <= kPreviewChars) {
                            cout << wJv << "\n";
                        } else {
                            cout << wJv.substr(0, kPreviewChars) << "…\n";
                        }
                    }
                }
            };

            // Tile the used range: each JsonView viewport matches drawable paper px (no accumulation across lastCol×lastRow).
            if (p.m_PageOrder == tPrintPageOrder::OverThenDown) {
                for (tIndex tr = 1; tr <= wLastRow;) {
                    tDouble vh = 0;
                    tIndex br = tr;
                    if (!ComputeRowBandPx(wRange, tr, wLastRow, wDrawableHPx, &vh, &br))
                        break;
                    for (tIndex lc = 1; lc <= wLastCol;) {
                        tDouble vw = 0;
                        tIndex rc = lc;
                        if (!ComputeColBandPx(wRange, lc, wLastCol, wDrawableWPx, &vw, &rc))
                            break;
                        wEmitTile(tr, lc, vh, vw, br, rc);
                        lc = rc + 1;
                    }
                    tr = br + 1;
                }
            } else {
                for (tIndex lc = 1; lc <= wLastCol;) {
                    tDouble vw = 0;
                    tIndex rc = lc;
                    if (!ComputeColBandPx(wRange, lc, wLastCol, wDrawableWPx, &vw, &rc))
                        break;
                    for (tIndex tr = 1; tr <= wLastRow;) {
                        tDouble vh = 0;
                        tIndex br = tr;
                        if (!ComputeRowBandPx(wRange, tr, wLastRow, wDrawableHPx, &vh, &br))
                            break;
                        wEmitTile(tr, lc, vh, vw, br, rc);
                        tr = br + 1;
                    }
                    lc = rc + 1;
                }
            }

            cout << "    Pages (tiles) emitted for this sheet: " << wTileSeq << "\n";
        }

        cout << "  Scenario total pages (tiles, all sheets): " << wScenarioPages << "\n";
    };

    std::vector<std::pair<tSheet*, tPrintParameters>> wRestorePrint;
    for (tSheet* wSh : wWorkBook->VectorPtSheet()) {
        if (wSh == nullptr || wSh->Name() == "_$$")
            continue;
        tPrintParameters snap;
        const tString wPJ = wSh->JsonPrintParameters();
        if (!wPJ.empty() && !snap.JsonParse(wPJ)) {
            snap = tPrintParameters();
        }
        wRestorePrint.push_back({wSh, snap});
    }

    wRunScenario("--- Scenario: loaded printParameters (all sheets) ---", "loaded_sheet");

    tPrintParameters wAlt =
        wRestorePrint.empty() ? tPrintParameters() : wRestorePrint.front().second;
    wAlt.m_PaperSize = 9;
    wAlt.m_Orientation = tPrintOrientation::Landscape;
    for (auto& pr : wRestorePrint) {
        pr.first->PrintParametersAssign(wAlt);
    }

    wRunScenario("--- Scenario: A4 Landscape demo (all sheets) ---", "A4_landscape_demo");

    for (const auto& pr : wRestorePrint) {
        pr.first->PrintParametersAssign(pr.second);
    }

    cout << "--- end PDFMetrics ---\n\n";
}

void TestSkExcel::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
    
	m_Application = tApplication::Instance();
    m_Application->Locale("us");
    tFormatRoot::Instance();
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
    RegisterExcelWorkBookCellClasses(m_Api);
    m_FormatApi = new tFormatCssApi();
    m_Api->FormatApi(m_FormatApi);
};

void TestSkExcel::tearDown() {
    tApplication::Instance()->Locale("fr");
    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    tFormatApi* wContainerFormatApi = wContainer->FormatApi();
    if (wContainerFormatApi != nullptr && wContainerFormatApi != m_FormatApi) {
        ShutdownFormatApi(wContainerFormatApi);
    } else if (m_FormatApi != nullptr) {
        if (wContainerFormatApi == nullptr) {
            wContainer->FormatApi(m_FormatApi);
        }
        tApplication::Instance()->ClearUndoRedo();
        ClearAllWorkBooksForFormatTeardown();
    }
    if (m_Api != nullptr) {
        delete(m_Api);
        m_Api = nullptr;
    }
    if (m_FormatApi != nullptr) {
        wContainer->FormatApi(nullptr);
        delete(m_FormatApi);
        m_FormatApi = nullptr;
    }
    DoneFormatRoot();
}
