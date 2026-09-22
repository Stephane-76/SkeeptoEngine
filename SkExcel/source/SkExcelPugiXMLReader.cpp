//===============================================
// SkExcelPugiXMLReader.cpp
// Read Excel file and parse it using PugiXML
//===============================================

#include "SkExcelPugiXMLReader.hpp"
#include "SkExcelProgress.hpp"
#include <SkApplication.hpp>
#include <SkFormatNumber.hpp>
#include <SkFormatString.hpp>
#include <SkBase64.hpp>
#include <SkFile.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <ctime>
#include <chrono>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#else
#include <unistd.h>
#endif

// debuginfo left undefined: verbose unzip/parse logs make large-sheet import unusable.

namespace {

#ifdef _WIN32
static int SkMkdir(const char* sPath) { return _mkdir(sPath); }
static int SkRmdir(const char* sPath) { return _rmdir(sPath); }
#else
static int SkMkdir(const char* sPath) { return mkdir(sPath, 0755); }
static int SkRmdir(const char* sPath) { return rmdir(sPath); }
#endif

// UTC tm from Unix seconds. Avoid gmtime_r (missing on MSVC) and gmtime_s
// (rejects time_t < 1970, which Excel serials before 1970-01-01 produce).
static std::tm UnixSecondsToUtcTm(long sSecs) {
    using namespace std::chrono;
    const sys_seconds wTp{seconds{sSecs}};
    const auto wDay = floor<days>(wTp);
    const year_month_day wYmd{wDay};
    const hh_mm_ss wHms{wTp - wDay};
    std::tm wTm{};
    wTm.tm_year = static_cast<int>(wYmd.year()) - 1900;
    wTm.tm_mon = static_cast<int>(static_cast<unsigned>(wYmd.month())) - 1;
    wTm.tm_mday = static_cast<int>(static_cast<unsigned>(wYmd.day()));
    wTm.tm_hour = static_cast<int>(wHms.hours().count());
    wTm.tm_min = static_cast<int>(wHms.minutes().count());
    wTm.tm_sec = static_cast<int>(wHms.seconds().count());
    return wTm;
}

// OOXML built-in date/time numFmtIds — keep in sync with SkExcel2SpreadSheet.cpp (ECMA-376).
static tBool ExcelBuiltinNumFmtIdIsDateTime(tInt sNumFmtId) {
    if (sNumFmtId >= 14 && sNumFmtId <= 22) return true;
    if (sNumFmtId >= 27 && sNumFmtId <= 36) return true;
    if (sNumFmtId >= 45 && sNumFmtId <= 47) return true;
    if (sNumFmtId >= 50 && sNumFmtId <= 58) return true;
    if (sNumFmtId >= 71 && sNumFmtId <= 81) return true;
    return false;
}

static void TrimAsciiWs(tString& s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

// american english: OOXML nodes often use namespace prefixes (e.g. main:b). Match by local name only.
static const char* XmlOoXmlLocalName(const char* sFullName) {
    if (!sFullName) return "";
    const char* wColon = std::strchr(sFullName, ':');
    return wColon ? (wColon + 1) : sFullName;
}

static pugi::xml_node XmlOoXmlChild(const pugi::xml_node& parent, const char* sLocalName) {
    if (!parent) return {};
    for (pugi::xml_node wCh = parent.first_child(); wCh; wCh = wCh.next_sibling()) {
        if (wCh.type() != pugi::node_element) continue;
        if (std::strcmp(XmlOoXmlLocalName(wCh.name()), sLocalName) == 0) return wCh;
    }
    return {};
}

static pugi::xml_attribute XmlOoXmlAttr(const pugi::xml_node& node, const char* sLocalName) {
    if (!node) return {};
    for (pugi::xml_attribute wA = node.first_attribute(); wA; wA = wA.next_attribute()) {
        if (std::strcmp(XmlOoXmlLocalName(wA.name()), sLocalName) == 0) return wA;
    }
    return {};
}

static tBool XmlOoXmlElementIs(const pugi::xml_node& node, const char* sLocalName) {
    return node && node.type() == pugi::node_element &&
           std::strcmp(XmlOoXmlLocalName(node.name()), sLocalName) == 0;
}

// american english: OOXML boolean elements (<b>, <i>, <strike>): absent val means true; val="0"/false disables.
static tBool XmlBoolChildTrue(const pugi::xml_node& sNode) {
    if (!sNode) return false;
    pugi::xml_attribute wV = XmlOoXmlAttr(sNode, "val");
    if (!wV) return true;
    return wV.as_bool(true);
}

static tBool AsciiEqualsI(const char* s, const char* lit) {
    if (!s || !lit) return false;
    while (*s && *lit) {
        if (std::tolower(static_cast<unsigned char>(*s)) != std::tolower(static_cast<unsigned char>(*lit)))
            return false;
        ++s;
        ++lit;
    }
    return *s == *lit;
}

// american english: <u val="none"/> must not enable underline; val="single"/etc. do.
static tBool XmlUnderlineTrue(const pugi::xml_node& sNode) {
    if (!sNode) return false;
    pugi::xml_attribute wV = XmlOoXmlAttr(sNode, "val");
    if (!wV || !wV.value() || !*wV.value()) return true;
    const char* wS = wV.as_string("");
    if (AsciiEqualsI(wS, "none")) return false;
    if (AsciiEqualsI(wS, "false")) return false;
    if (wS[0] == '0' && wS[1] == '\0') return false;
    return wV.as_bool(true);
}

static tString CssFragmentFromRPr(const pugi::xml_node& rPr) {
    if (!rPr) return {};
    std::ostringstream wCss;
    if (XmlBoolChildTrue(XmlOoXmlChild(rPr, "b"))) wCss << "font-weight:bold;";
    if (XmlBoolChildTrue(XmlOoXmlChild(rPr, "i"))) wCss << "font-style:italic;";
    const tBool wU = XmlUnderlineTrue(XmlOoXmlChild(rPr, "u"));
    const tBool wSt = XmlBoolChildTrue(XmlOoXmlChild(rPr, "strike"));
    if (wU || wSt) {
        wCss << "text-decoration-line:";
        if (wU) wCss << " underline";
        if (wSt) wCss << " line-through";
        wCss << ";";
    }
    return wCss.str();
}

// american english: Shared-string item (<si>) or inline cell string (<is>): extra CSS when every <r> agrees.
static tString RichTextUniformCssFromParent(const pugi::xml_node& parent) {
    if (!parent) return {};
    const tBool wHasDirectT = static_cast<bool>(XmlOoXmlChild(parent, "t"));
    std::vector<pugi::xml_node> wRuns;
    for (pugi::xml_node wCh = parent.first_child(); wCh; wCh = wCh.next_sibling()) {
        if (!XmlOoXmlElementIs(wCh, "r")) continue;
        if (XmlOoXmlChild(wCh, "t")) wRuns.push_back(wCh);
    }
    if (wRuns.empty()) return {};
    if (wHasDirectT) return {};
    tString wFirst = CssFragmentFromRPr(XmlOoXmlChild(wRuns[0], "rPr"));
    for (tSize i = 1; i < wRuns.size(); ++i) {
        if (CssFragmentFromRPr(XmlOoXmlChild(wRuns[i], "rPr")) != wFirst) return {};
    }
    return wFirst;
}

static tString RemoveExcelBracketTokens(const tString& s) {
    tString out = s;
    for (;;) {
        tSize lb = out.find('[');
        if (lb == tString::npos) break;
        tSize rb = out.find(']', lb);
        if (rb == tString::npos) break;
        out.erase(lb, rb - lb + 1);
    }
    return out;
}

static tString StripQuotedLiteralsExcelMask(const tString& s) {
    tString stripped;
    stripped.reserve(s.size());
    for (tSize i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            ++i;
            while (i < s.size() && s[i] != '"') {
                if (s[i] == '\\') ++i;
                ++i;
            }
            continue;
        }
        stripped.push_back(s[i]);
    }
    return stripped;
}

static int MaxDigitSlotsAfterDot(const tString& s) {
    tSize dot = s.find('.');
    if (dot == tString::npos) return 0;
    int n = 0;
    for (tSize i = dot + 1; i < s.size(); ++i) {
        char c = s[i];
        if (c == '0' || c == '#') ++n;
        else break;
    }
    return n;
}

static tBool MaskLooksLikeFraction(const tString& s) {
    return s.find('/') != tString::npos && s.find('?') != tString::npos;
}

static int MantissaDecimalsScientific(const tString& s) {
    tSize ePos = s.size();
    for (tSize i = 0; i < s.size(); ++i) {
        if (s[i] == 'e' || s[i] == 'E') {
            ePos = i;
            break;
        }
    }
    return MaxDigitSlotsAfterDot(s.substr(0, ePos));
}

// Map Excel OOXML built-in number numFmtIds (non-date) to SkRoot predefined CSS keys (locale-aware).
static tBool BuiltinNumFmtIdToSkRootNumberKey(tInt sNumFmtId, const char** outKey, tShort* outPrec) {
    *outPrec = -1;
    switch (sNumFmtId) {
        case 1: *outKey = "0"; return true;
        case 2: *outKey = "0.00"; return true;
        case 3: *outKey = "#,##0"; return true;
        case 4: *outKey = "#,##0.00"; return true;
        case 5:
        case 6:
            *outKey = "#,##0 $;(#,##0) $"; // accountingP — negatives in parentheses (closest to Excel "$"#,##0_)
            return true;
        case 7:
        case 8:
            *outKey = "#,##0.00 $;(#,##0.00) $"; // accounting0P
            return true;
        case 9: *outKey = "0%"; return true;
        case 10: *outKey = "0.00%"; return true;
        case 11: *outKey = "0.00E+00"; return true;
        case 48: *outKey = "0.000E+00"; return true; // ##0.0E+0 in Excel
        default: return false;
    }
}

// Best-effort mapping from an Excel number mask to a SkRoot predefined format-string KEY so that
// numeric formatting uses tLocale thousand/decimal separators (excelnumber literals stay US-style).
static tBool ClosestSkRootKeyFromExcelNumberMask(const tString& sMaskNorm, const char** outKey, tShort* outPrec) {
    *outPrec = -1;
    SkRoot::tFormatStringRoot* wRoot = nullptr;
    if (SkRoot::tApplication::Instance() != nullptr) {
        wRoot = SkRoot::tApplication::Instance()->FormatStringRoot();
    }
    if (wRoot == nullptr) return false;
    if (wRoot->IsValidExcelDate(sMaskNorm)) return false;

    tString work = sMaskNorm;
    tSize semi = work.find(';');
    if (semi != tString::npos) work = work.substr(0, semi);
    work = SkRoot::NormalizeExcelIntlCurrencyBracketsInMask(work);
    work = RemoveExcelBracketTokens(work);
    work = StripQuotedLiteralsExcelMask(work);
    TrimAsciiWs(work);
    if (work.empty()) return false;
    if (work == "@") return false;
    {
        tString wGen = work;
        for (char& c : wGen) {
            if (static_cast<unsigned char>(c) <= 127) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (wGen == "general") return false;
    }
    if (MaskLooksLikeFraction(work)) return false;

    if (work.find('%') != tString::npos) {
        int dz = MaxDigitSlotsAfterDot(work);
        if (dz >= 2) *outKey = "0.00%";
        else *outKey = "0%";
        return true;
    }

    tBool wSci = false;
    for (char c : work) {
        if (c == 'e' || c == 'E') {
            wSci = true;
            break;
        }
    }
    if (wSci) {
        int md = MantissaDecimalsScientific(work);
        if (md <= 2) *outKey = "0.00E+00";
        else if (md == 3) *outKey = "0.000E+00";
        else *outKey = "0.0000E+00";
        return true;
    }

    const tBool wDollar = work.find('$') != tString::npos;
    const tBool wEuro = work.find("\xE2\x82\xAC") != tString::npos;
    const tBool wPound = work.find("\xC2\xA3") != tString::npos;              // UTF-8 £
    const tBool wYen = work.find("\xC2\xA5") != tString::npos || work.find("\xEF\xBF\xA5") != tString::npos;
    const tBool wHasCurr = wDollar || wEuro || wPound || wYen;

    const tBool wGrouping =
        work.find(',') != tString::npos && (work.find('#') != tString::npos || work.find('0') != tString::npos);

    const int decPl = MaxDigitSlotsAfterDot(work);

    if (wHasCurr) {
        // Predefined SkRoot accounting KEYS literally contain "$". Mapping €/£/¥ masks to them
        // makes the renderer show dollars. Fall through so rebuildSuffix emits excelnumber with
        // the real glyph (see BuildCssForStyle below).
        if (wEuro || wPound || wYen)
            return false;
        if (!wDollar)
            return false;
        if (decPl >= 2) *outKey = "#,##0.00 $;(#,##0.00) $";
        else *outKey = "#,##0 $;(#,##0) $";
        return true;
    }

    if (wGrouping) {
        if (decPl >= 2) {
            *outKey = "#,##0.00";
            if (decPl != 2) *outPrec = static_cast<tShort>(decPl);
        } else if (decPl == 1) {
            *outKey = "#,##0.00";
            *outPrec = 1;
        } else {
            *outKey = "#,##0";
        }
        return true;
    }

    if (decPl >= 2) {
        *outKey = "0.00";
        if (decPl != 2) *outPrec = static_cast<tShort>(decPl);
        return true;
    }
    if (decPl == 1) {
        *outKey = "0";
        *outPrec = 1;
        return true;
    }
    *outKey = "0";
    return true;
}

// SkFormat text-align accepts left|center|right|justify only — skip Excel-only values.
static tString HorizontalAlignForCss(const tString& sExcelAlign) {
    if (sExcelAlign.empty() || sExcelAlign == "general"
        || sExcelAlign == "centerContinuous" || sExcelAlign == "fill"
        || sExcelAlign == "distributed") {
        return "";
    }
    return sExcelAlign;
}

// OOXML rgb / indexedColors: AARRGGBB or RRGGBB → 6-char sRGB for CSS.
static tString NormalizeOoXmlRgbHex(const tString& sRaw) {
    tString wHex = sRaw;
    if (wHex.empty()) return {};
    std::transform(wHex.begin(), wHex.end(), wHex.begin(), ::toupper);
    if (wHex.size() == 8) wHex = wHex.substr(2);
    if (wHex.size() > 6) wHex = wHex.substr(wHex.size() - 6);
    return wHex;
}

} // namespace

tString tExcelPugiXMLReader::BuildCssForStyle(tInt sStyleIdx) const {
    if (sStyleIdx < 0) return "";
    std::ostringstream wCss;
    auto wSafe = [](tInt wIdx, tInt wSize){ return wIdx>=0 && wIdx<wSize; };

    if (wSafe(sStyleIdx, (tInt)m_CellXfsFontId.size())) {
        tInt wFid = m_CellXfsFontId[sStyleIdx];
        if (wSafe(wFid, (tInt)m_Fonts.size())) {
            const auto& wF = m_Fonts[wFid];
            if (!wF.name.empty()) wCss << "font-family:'" << wF.name << "',serif;";
            if (wF.size>0) wCss << "font-size:" << wF.size << "pt;";
            if (wF.bold) wCss << "font-weight:bold;";
            if (wF.italic) wCss << "font-style:italic;";
            if (wF.underline || wF.strike) {
                wCss << "text-decoration-line:";
                tBool wFirst=true;
                if (wF.underline) { wCss << " underline"; wFirst=false; }
                if (wF.strike) { wCss << (wFirst?" ":" ") << " line-through"; }
                wCss << ";";
            }
            tString wFc = ResolveColor(wF.color);
            if (!wFc.empty() && wFc!="transparent") wCss << "color:#" << wFc << ";";
        }
    }
    if (wSafe(sStyleIdx, (tInt)m_CellXfsFillId.size())) {
        tInt wFillId = m_CellXfsFillId[sStyleIdx];
        if (wSafe(wFillId, (tInt)m_Fills.size())) {
            const auto& wFill = m_Fills[wFillId];
            tString wBg = ResolvePatternFillBackground(wFill);
            if (!wBg.empty() && wBg != "transparent") wCss << "background-color:#" << wBg << ";";
        }
    }
    if (wSafe(sStyleIdx, (tInt)m_CellXfsBorderId.size())) {
        tInt wBid = m_CellXfsBorderId[sStyleIdx];
        if (wSafe(wBid, (tInt)m_Borders.size())) {
            const auto& wB = m_Borders[wBid];
            EmitBorderEdgeCss(wCss, "border-left",   wB.left);
            EmitBorderEdgeCss(wCss, "border-top",    wB.top);
            EmitBorderEdgeCss(wCss, "border-right",  wB.right);
            EmitBorderEdgeCss(wCss, "border-bottom", wB.bottom);
        }
    }
    // OOXML wrapText="1" -> SkFormat text-wrap (see SkText::Str / tTextWrap::wrap).
    if (wSafe(sStyleIdx, (tInt)m_CellXfsWrapText.size()) && m_CellXfsWrapText[sStyleIdx]) {
        wCss << "text-wrap:wrap;";
    }
    if (wSafe(sStyleIdx, (tInt)m_CellXfsAlignH.size())) {
        const tString& wAh = m_CellXfsAlignH[sStyleIdx];
        const tString& wAv = m_CellXfsAlignV[sStyleIdx];
        const tString wCssAlignH = HorizontalAlignForCss(wAh);
        if (!wCssAlignH.empty()) wCss << "text-align:" << wCssAlignH << ";";
        if (!wAv.empty()) wCss << "vertical-align:" << (wAv=="center"?"middle":wAv) << ";";
        //if (m_CellXfsShrinkToFit[sStyleIdx]) wCss << "font-size:smaller;";
        if (m_CellXfsIndent[sStyleIdx]>0) wCss << "padding-left:" << (m_CellXfsIndent[sStyleIdx]*8) << "px;";
        tInt wR = m_CellXfsTextRotation[sStyleIdx];
        if (wR >= 0) {
            // OOXML textRotation convention:
            //   0..90  : counter-clockwise angle in degrees
            //   91..180: clockwise, OOXML value = 90 + |negative angle|
            //            -> internal angle = 90 - wR (e.g. 135 -> -45, 180 -> -90)
            //   255    : vertical stacked text (sentinel, passed through)
            if (wR == 255) {
                wCss << "text-rotate:255;";
            } else if (wR >= 0 && wR <= 90) {
                if (wR > 0) wCss << "text-rotate:" << wR << ";";
            } else if (wR > 90 && wR <= 180) {
                wCss << "text-rotate:" << (90 - wR) << ";";
            }
        }
    }
    // Append Excel number/date format as custom CSS properties (use CSS custom props to avoid parser drops)
        // Built-in Excel numFmtIds 14/15/16/17/22 are LOCALE-DEPENDENT in Excel itself
        // (Excel resolves them to mm/dd/yyyy in en-US, dd/mm/yyyy in fr-FR, etc.).
        // Emit the matching SkRoot locale-aware predefined key instead of a baked Excel literal,
        // so the renderer resolves the actual format at display time using the active tLocale.
        tInt wRawNumFmtId = GetNumFmtIdForStyle(sStyleIdx);
        const tChar* wLocaleAwareKey = nullptr;
        switch (wRawNumFmtId) {
            case 14: wLocaleAwareKey = "mm-dd-yyyy"; break;            // datelong
            case 17: wLocaleAwareKey = "mmmm yyyy"; break;             // datemmmyy
            case 18: wLocaleAwareKey = "h:mm AM/PM"; break;           // datehmmap
            case 19: wLocaleAwareKey = "h:mm:ss AM/PM"; break;        // datehmmssap
            case 20: wLocaleAwareKey = "h:mm"; break;                 // datehmm
            case 21: wLocaleAwareKey = "h:mm:ss"; break;              // datehmmss
            case 22: wLocaleAwareKey = "mm-dd-yyyy h:mm"; break;       // dateddmmyyyyhmm
            default: break;
        }
        if (wLocaleAwareKey != nullptr) {
            wCss << "format-string:\"" << wLocaleAwareKey << "\";";
            return wCss.str();
        }
        tString wFmt = GetFormatCodeForStyle(sStyleIdx);
        if (!wFmt.empty() && wFmt != "General") {
            
            // Pre-process: unescape already escaped sequences to avoid double-escaping
            tString wUnescaped = wFmt;
            // Replace \\ with \ and \" with " but keep \ " as is
            tSize pos = 0;
            while ((pos = wUnescaped.find("\\\\", pos)) != tString::npos) {
                wUnescaped.replace(pos, 2, "\\");
                pos += 1;
            }
            pos = 0;
            while ((pos = wUnescaped.find("\\\"", pos)) != tString::npos) {
                wUnescaped.replace(pos, 2, "\"");
                pos += 1;
            }
            // Normalize Excel localized quotes around currency: replace \ "€" or "€" with space+€
            {
                auto normalizeEuro = [](tString s) {
                    // Replace sequences: \ "€" ->  €   and   "€" ->  €
                    // 1) remove backslash-space-quote before euro
                    tSize p = 0;
                    while ((p = s.find("\\ \"€\"", p)) != tString::npos) {
                        s.replace(p, 5, " €");
                    }
                    // 2) remove simple quoted euro
                    p = 0;
                    while ((p = s.find("\"€\"", p)) != tString::npos) {
                        s.replace(p, 3, " €");
                    }
                    // Trim duplicate spaces
                    while (s.find("  ") != tString::npos) s.erase(s.find("  "), 1);
                    return s;
                };
                wUnescaped = normalizeEuro(wUnescaped);
            }

            // Locale-aware NUMBER formats: map OOXML built-in ids and simple masks to SkRoot
            // predefined keys so tLocale separators apply (comma/dot grouping differs FR vs US).
            if (!ExcelBuiltinNumFmtIdIsDateTime(wRawNumFmtId)) {
                const tBool wCustomNumFmt =
                    (m_CustomNumFmt.find(wRawNumFmtId) != m_CustomNumFmt.end());
                if (!wCustomNumFmt) {
                    const char* nk = nullptr;
                    tShort np = -1;
                    if (BuiltinNumFmtIdToSkRootNumberKey(wRawNumFmtId, &nk, &np)) {
                        wCss << "format-string:\"" << nk << "\"";
                        if (np >= 0) wCss << " " << np;
                        wCss << ";";
                        return wCss.str();
                    }
                }
            }
            {
                const char* nk = nullptr;
                tShort np = -1;
                if (ClosestSkRootKeyFromExcelNumberMask(wUnescaped, &nk, &np)) {
                    wCss << "format-string:\"" << nk << "\"";
                    if (np >= 0) wCss << " " << np;
                    wCss << ";";
                    return wCss.str();
                }
            }

            {
                // If the mask contains currency/percent, rebuild a clean suffix form: "<numeric> <symbol>"
                auto hasChar = [](const tString& s, const char* utf8) { return s.find(utf8) != tString::npos; };
                auto rebuildSuffix = [&](const tString& s, const char* utf8Symbol) -> tString {
                    tString base = s;
                    // Normalize non-breaking spaces to regular spaces
                    tSize np = 0;
                    while ((np = base.find("\xC2\xA0", np)) != tString::npos) { // U+00A0
                        base.replace(np, 2, " ");
                    }
                    np = 0;
                    while ((np = base.find("\xE2\x80\xAF", np)) != tString::npos) { // U+202F
                        base.replace(np, 3, " ");
                    }
                    // Remove quotes and backslashes
                    base.erase(std::remove(base.begin(), base.end(), '\\'), base.end());
                    base.erase(std::remove(base.begin(), base.end(), '"'), base.end());
                    // Remove all occurrences of the symbol to isolate numeric part
                    tSize pos = 0; tString symbol(utf8Symbol);
                    while ((pos = base.find(symbol, pos)) != tString::npos) {
                        base.erase(pos, symbol.size());
                    }
                    // Trim spaces
                    auto ltrim=[&](tString& x){ x.erase(x.begin(), std::find_if(x.begin(), x.end(), [](unsigned char ch){return !std::isspace(ch);})); };
                    auto rtrim=[&](tString& x){ x.erase(std::find_if(x.rbegin(), x.rend(), [](unsigned char ch){return !std::isspace(ch);} ).base(), x.end()); };
                    ltrim(base); rtrim(base);
                    // Collapse internal spaces and clean non-ASCII characters
                    tString compact; compact.reserve(base.size()); tBool prevSpace=false;
                    for (unsigned char ch : base) {
                        tBool isSpace = std::isspace(ch);
                        if (isSpace) { if (!prevSpace) { compact.push_back(' '); prevSpace=true; } }
                        else if (ch >= 0x20 && ch <= 0x7E) { // Only keep printable ASCII
                            compact.push_back((char)ch); prevSpace=false; 
                        }
                        // Skip non-ASCII characters to avoid "Bad character" errors
                    }
                    // Special case for %: no space before the symbol
                    if (symbol == "%") {
                        return compact + symbol;
                    }
                    return compact + " " + symbol;
                };

                // rebuildSuffix produces parse-safe ASCII (it strips backslashes and quotes that
                // would otherwise break the CSS lexer / Excel validation and drop the whole
                // format). Apply it PER SECTION so a multi-section accounting mask keeps its
                // currency glyph on EVERY section. For e.g. "_-* #,##0.00\ [$€-1]_-;...":
                //  - passing the raw mask through breaks validation (General fallback -> "5"),
                //  - a single flattened rebuildSuffix call appended one trailing "€" that landed
                //    only in the last (zero) section, so positive values lost the symbol ("5,00").
                // Splitting on ';' and rebuilding each section restores "5,00 €" on positives.
                auto rebuildMask = [&](const tString& s, const char* utf8Symbol) -> tString {
                    tString out;
                    tSize start = 0;
                    for (tSize i = 0; i <= s.size(); ++i) {
                        if (i == s.size() || s[i] == ';') {
                            if (!out.empty()) out += ";";
                            out += rebuildSuffix(s.substr(start, i - start), utf8Symbol);
                            start = i + 1;
                        }
                    }
                    return out;
                };

                if (hasChar(wUnescaped, "€")) {
                    wCss << "format-string:\"" << rebuildMask(wUnescaped, "€") << "\";";
                } else if (hasChar(wUnescaped, "%")) {
                    wCss << "format-string:\"" << rebuildMask(wUnescaped, "%") << "\";";
                } else if (hasChar(wUnescaped, "$")) {
                    wCss << "format-string:\"" << rebuildMask(wUnescaped, "$") << "\";";
                } else if (hasChar(wUnescaped, "£")) {
                    wCss << "format-string:\"" << rebuildMask(wUnescaped, "£") << "\";";
                } else if (hasChar(wUnescaped, "¥") || hasChar(wUnescaped, "\xEF\xBF\xA5")) { // Yen or fullwidth Yen
                    wCss << "format-string:\"" << rebuildMask(wUnescaped, "¥") << "\";";
                } else {
                auto escapeCss = [](const tString& in) -> tString {
                tString out;
                out.reserve(in.size()*2);
                const unsigned char* p = reinterpret_cast<const unsigned char*>(in.data());
                tSize i = 0, n = in.size();
                auto appendHex = [&](tUInt cp){
                    // Always ensCode as 6-digit uppercase hex per CSS escapes
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "%06X", cp & 0xFFFFFFu);
                    out += '\\';
                    out += buf; // e.g. \000020AC
                    // no trailing space to avoid injecting characters
                };
                while (i < n) {
                    unsigned char c = p[i];
                    if (c < 0x20) { // control
                        appendHex(c);
                        ++i;
                    } else if (c < 0x80) { // ASCII
                        if (c == '"') { out += "\\\""; }
                        else if (c == '\\') { out += "\\\\"; }
                        else { out += (char)c; }
                        ++i;
                    } else {
                        // UTF-8 desCode
                        tUInt cp = 0; tSize len = 0;
                        if ((c & 0xE0) == 0xC0 && i+1 < n) { cp = ((c & 0x1F) << 6) | (p[i+1] & 0x3F); len = 2; }
                        else if ((c & 0xF0) == 0xE0 && i+2 < n) { cp = ((c & 0x0F) << 12) | ((p[i+1] & 0x3F) << 6) | (p[i+2] & 0x3F); len = 3; }
                        else if ((c & 0xF8) == 0xF0 && i+3 < n) { cp = ((c & 0x07) << 18) | ((p[i+1] & 0x3F) << 12) | ((p[i+2] & 0x3F) << 6) | (p[i+3] & 0x3F); len = 4; }
                        else { // invalid, escape byte
                            appendHex(c);
                            ++i; continue;
                        }
                        if (cp == 0x20AC) {
                            // Euro sign: keep literal UTF-8 "€" (don't escape)
                            out += "\xE2\x82\xAC";
                        } else if (cp == 0x00A0 || cp == 0x202F) {
                            // non-breaking space or narrow no-break space -> regular space
                            out += ' ';
                        } else {
                            appendHex(cp);
                        }
                        i += len;
                    }
                }
                return out;
                };
                tString wEscaped = escapeCss(wUnescaped);
                wCss << "format-string:\"" << wEscaped << "\";";
                }
            }
        }
    return wCss.str();
}

#include <pugixml.hpp>
#include <zip.h>
#include <SkExcelPugiXMLReader.hpp>

using namespace std;

namespace {
// Excel cell text limit is ~32k; WASM libc++ can still length_error on large contiguous allocations — stay conservative.
constexpr tSize kMaxSharedStringSiChars = 8192;
constexpr tSize kMaxWorksheetNameChars = 256;
constexpr tSize kMaxZipEntryPathChars = 1024;
constexpr tSize kMaxFormatCodeChars = 8192;
constexpr tSize kMaxFontNameChars = 256;
constexpr tSize kMaxPatternTypeChars = 128;
constexpr tSize kMaxBorderStyleChars = 64;
constexpr tSize kMaxAlignAttrChars = 64;
constexpr tSize kMaxThemeColorAttrChars = 16;

static tString BoundedCStr(const char* sP, tSize sMaxChars) {
	tString wOut;
	if (sP == nullptr) {
		return wOut;
	}
	while (wOut.size() < sMaxChars && *sP != '\0') {
		wOut.push_back(*sP++);
	}
	return wOut;
}

static tString BoundedAttr(const pugi::xml_attribute& sAttr, tSize sMaxChars) {
	if (!sAttr) {
		return {};
	}
	return BoundedCStr(sAttr.as_string(""), sMaxChars);
}

static void AppendPugiTextCapped(tString& ioAcc, pugi::xml_text sTx) {
	const char* wP = sTx.get();
	if (wP == nullptr) {
		return;
	}
	while (ioAcc.size() < kMaxSharedStringSiChars && *wP != '\0') {
		ioAcc.push_back(*wP++);
	}
}

// Large zip parts (sharedStrings.xml, big worksheets) are spilled to disk and
// leave content empty. Always load from diskPath when set, otherwise memory.
static tBool LoadUnzippedXmlDocument(const UnzippedFile& sFile, pugi::xml_document& sOut) {
	if (!sFile.diskPath.empty()) {
		const pugi::xml_parse_result wFromDisk = sOut.load_file(sFile.diskPath.c_str());
		if (wFromDisk) {
			return true;
		}
		std::cerr << "SkExcel: failed to parse " << sFile.filename
		          << " from disk (" << sFile.diskPath << "): "
		          << wFromDisk.description() << std::endl;
		return false;
	}
	if (sFile.content.empty()) {
		return false;
	}
	const pugi::xml_parse_result wFromMem =
	    sOut.load_buffer(sFile.content.data(), sFile.content.size());
	if (wFromMem) {
		return true;
	}
	std::cerr << "SkExcel: failed to parse " << sFile.filename
	          << " from memory: " << wFromMem.description() << std::endl;
	return false;
}

static tString SheetNameFromAttribute(pugi::xml_attribute sAttr) {
	const char* wP = sAttr.value();
	if (wP == nullptr) {
		return {};
	}
	tString wOut;
	while (wOut.size() < kMaxWorksheetNameChars && *wP != '\0') {
		wOut.push_back(*wP++);
	}
	return wOut;
}
} // namespace

// Constructor
tExcelPugiXMLReader::tExcelPugiXMLReader() : m_WorksheetCount(0) {
    m_WorksheetNames.clear();
}

// Destructor
tExcelPugiXMLReader::~tExcelPugiXMLReader() {
    CleanupUnzipSpill();
}

// Load and parse Excel file
tBool tExcelPugiXMLReader::LoadExcelFile(const tString& sFilePath) {
#ifdef debuginfo
    cout << "\n=== Loading Excel File with PugiXML ===" << std::endl;
    cout << "File: " << sFilePath << std::endl;
#endif
    
    CleanupUnzipSpill();
    m_UnzipTempDir = sFilePath + ".skexcel-parts";
    m_UnzippedFiles = UnzipExcelInMemory(sFilePath);
    if (m_UnzippedFiles.empty()) {
        std::cerr << "Failed to unzip Excel file or file is empty." << std::endl;
        return false;
    }
    
#ifdef debuginfo
    std::cout << "Successfully loaded " << m_UnzippedFiles.size() << " files from Excel archive" << std::endl;
#endif
    InitializeBuiltinFormats();
    // Initialize indexed palette (Excel default 0..63)
    m_IndexedColors = {
        "000000","FFFFFF","FF0000","00FF00","0000FF","FFFF00","FF00FF","00FFFF",
        "000000","FFFFFF","FF0000","00FF00","0000FF","FFFF00","FF00FF","00FFFF",
        "800000","008000","000080","808000","800080","008080","C0C0C0","808080",
        "9999FF","993366","FFFFCC","CCFFFF","660066","FF8080","0066CC","CCCCFF",
        "000080","FF00FF","FFFF00","00FFFF","800080","800000","008080","0000FF",
        "00CCFF","CCFFFF","CCFFCC","FFFF99","99CCFF","FF99CC","CC99FF","FFCC99",
        "3366FF","33CCCC","99CC00","FFCC00","FF9900","FF6600","666699","969696",
        "003366","339966","003300","333300","993300","993366","333399","333333"
    };
    return true;
}
void tExcelPugiXMLReader::ParseTheme() {
    m_ThemeColors.clear();
    auto wIt = m_UnzippedFiles.find("xl/theme/theme1.xml");
    if (wIt == m_UnzippedFiles.end()) return;
    pugi::xml_document wDoc;
    if (!wDoc.load_buffer(wIt->second.content.data(), wIt->second.content.size())) return;
    // themeClrScheme: a:clrScheme -> a:lt1,a:dk1,a:lt2,a:dk2,a:accent1..6,a:hlink,a:folHlink
    pugi::xml_node wScheme = wDoc.select_node("//a:clrScheme").node();
    if (!wScheme) return;
    auto wReadSrgb = [](const pugi::xml_node& wN) -> tString {
        if (!wN) return {};
        if (auto wS = wN.child("a:srgbClr")) return BoundedAttr(wS.attribute("val"), kMaxThemeColorAttrChars);
        if (auto wS2 = wN.child("a:sysClr")) return BoundedAttr(wS2.attribute("lastClr"), kMaxThemeColorAttrChars);
        return {};
    };
    // Excel mapping for theme index:
    // 0: lt1, 1: dk1, 2: lt2, 3: dk2, 4..9: accent1..6, 10: hlink, 11: folHlink
    std::vector<const char*> wOrder = {"a:lt1","a:dk1","a:lt2","a:dk2","a:accent1","a:accent2","a:accent3","a:accent4","a:accent5","a:accent6","a:hlink","a:folHlink"};
    for (auto wKey : wOrder) {
        pugi::xml_node wN = wScheme.child(wKey);
        tString wV = wReadSrgb(wN);
        std::transform(wV.begin(), wV.end(), wV.begin(), ::toupper);
        m_ThemeColors.push_back(wV);
    }
}

tString tExcelPugiXMLReader::ApplyTintToSrgb(const tString& srgb, tDouble sTint) {
    if (srgb.size() != 6) return srgb;
    auto hex = [](char c){ if ('0'<=c&&c<='9') return c-'0'; if ('A'<=c&&c<='F') return c-'A'+10; return 0; };
    tInt r = (hex(srgb[0])<<4) + hex(srgb[1]);
    tInt g = (hex(srgb[2])<<4) + hex(srgb[3]);
    tInt b = (hex(srgb[4])<<4) + hex(srgb[5]);
    auto apply=[&](tInt c){ tDouble v=c/255.0; if (sTint>0) v = v*(1.0-sTint)+1.0*sTint; else v = v*(1.0+sTint); tInt out=(tInt)std::round(std::clamp(v,0.0,1.0)*255.0); return std::clamp(out,0,255); };
    r=apply(r); g=apply(g); b=apply(b);
    char buf[7]; std::snprintf(buf,sizeof(buf),"%02X%02X%02X",r,g,b); return tString(buf);
}

tString tExcelPugiXMLReader::ResolveColor(const ExcelColor& sColor) const {
    // priority: rgb -> theme -> indexed
    tString rgb = sColor.rgb;
    if (!rgb.empty()) {
        tString v = NormalizeOoXmlRgbHex(rgb);
        if (!sColor.tint.empty()) {
            tDouble wTint = std::atof(sColor.tint.c_str());
            v = ApplyTintToSrgb(v, wTint);
        }
        return v;
    }
    if (!sColor.theme.empty()) {
        tInt idx = std::atoi(sColor.theme.c_str());
        if (idx>=0 && idx<(tInt)m_ThemeColors.size()) {
            tString wV = m_ThemeColors[idx];
            if (!sColor.tint.empty()) wV = ApplyTintToSrgb(wV, std::atof(sColor.tint.c_str()));
            return wV;
        }
    }
    if (!sColor.indexed.empty()) {
        tInt idx = std::atoi(sColor.indexed.c_str());
        // Excel automatic color (not in indexedColors list).
        if (idx == 64) return "000000";
        if (idx>=0 && idx<(tInt)m_IndexedColors.size()) return m_IndexedColors[idx];
    }
    return "";
}
// Parse styles.xml
void tExcelPugiXMLReader::ParseStyles() {
    auto it = m_UnzippedFiles.find("xl/styles.xml");
    if (it == m_UnzippedFiles.end()) {
#ifdef debugerror
        std::cerr << "No styles.xml found" << std::endl;
#endif
        return;
    }

    const auto& styles = it->second;
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(styles.content.data(), styles.content.size());
    if (!result) {
#ifdef debugerror
        std::cerr << "Error parsing styles.xml: " << result.description() << std::endl;
#endif
        return;
    }

    m_CustomNumFmt.clear();
    m_CellXfsNumFmtId.clear();
    m_Fonts.clear();
    m_Fills.clear();
    m_Borders.clear();
    m_DxFills.clear();
    m_DxFontColors.clear();
    m_DxFonts.clear();
    m_DxBorders.clear();
    m_TableStyles.clear();
    m_CellXfsFontId.clear();
    m_CellXfsFillId.clear();
    m_CellXfsBorderId.clear();
    m_CellXfsAlignH.clear();
    m_CellXfsAlignV.clear();
    m_CellXfsWrapText.clear();
    m_CellXfsShrinkToFit.clear();
    m_CellXfsIndent.clear();
    m_CellXfsTextRotation.clear();
    m_CellXfsReadingOrder.clear();

    pugi::xml_node root = doc.child("styleSheet");
    if (!root) {
        for (pugi::xml_node wCh = doc.first_child(); wCh; wCh = wCh.next_sibling()) {
            if (XmlOoXmlElementIs(wCh, "styleSheet")) {
                root = wCh;
                break;
            }
        }
    }
    if (!root) return;

    // Workbook may override the default Excel indexed palette (0..63).
    if (pugi::xml_node wColors = XmlOoXmlChild(root, "colors")) {
        if (pugi::xml_node wIndexed = XmlOoXmlChild(wColors, "indexedColors")) {
            tSize wIdx = 0;
            for (pugi::xml_node wRgb = wIndexed.first_child(); wRgb; wRgb = wRgb.next_sibling()) {
                if (!XmlOoXmlElementIs(wRgb, "rgbColor")) continue;
                const tString wHex = NormalizeOoXmlRgbHex(
                    BoundedAttr(XmlOoXmlAttr(wRgb, "rgb"), 16));
                if (wHex.empty()) continue;
                if (wIdx >= m_IndexedColors.size()) {
                    m_IndexedColors.resize(wIdx + 1, "000000");
                }
                m_IndexedColors[wIdx] = wHex;
                ++wIdx;
            }
        }
    }

    // custom numFmts
    if (pugi::xml_node numFmts = XmlOoXmlChild(root, "numFmts")) {
        for (pugi::xml_node numFmt = numFmts.first_child(); numFmt; numFmt = numFmt.next_sibling()) {
            if (!XmlOoXmlElementIs(numFmt, "numFmt")) continue;
            tInt id = XmlOoXmlAttr(numFmt, "numFmtId").as_int(-1);
            tString sCode = BoundedAttr(XmlOoXmlAttr(numFmt, "formatCode"), kMaxFormatCodeChars);
            if (id >= 0) m_CustomNumFmt[id] = std::move(sCode);
        }
    }
    // fonts
    if (pugi::xml_node fonts = XmlOoXmlChild(root, "fonts")) {
        for (pugi::xml_node f = fonts.first_child(); f; f = f.next_sibling()) {
            if (!XmlOoXmlElementIs(f, "font")) continue;
            ExcelFont ef;
            if (auto n = XmlOoXmlChild(f, "name")) ef.name = BoundedAttr(XmlOoXmlAttr(n, "val"), kMaxFontNameChars);
            if (auto n = XmlOoXmlChild(f, "sz")) ef.size = XmlOoXmlAttr(n, "val").as_double(0.0);
            ef.bold = XmlBoolChildTrue(XmlOoXmlChild(f, "b"));
            ef.italic = XmlBoolChildTrue(XmlOoXmlChild(f, "i"));
            ef.underline = XmlUnderlineTrue(XmlOoXmlChild(f, "u"));
            ef.strike = XmlBoolChildTrue(XmlOoXmlChild(f, "strike"));
            if (auto n = XmlOoXmlChild(f, "color")) ef.color = ReadColorNode(n);
            m_Fonts.push_back(ef);
        }
    }
    // fills
    if (pugi::xml_node fills = XmlOoXmlChild(root, "fills")) {
        for (pugi::xml_node fl = fills.first_child(); fl; fl = fl.next_sibling()) {
            if (!XmlOoXmlElementIs(fl, "fill")) continue;
            ExcelFill efl;
            if (auto pat = XmlOoXmlChild(fl, "patternFill")) {
                efl.patternType = BoundedAttr(XmlOoXmlAttr(pat, "patternType"), kMaxPatternTypeChars);
                if (auto fg = XmlOoXmlChild(pat, "fgColor")) efl.fgColor = ReadColorNode(fg);
                if (auto bg = XmlOoXmlChild(pat, "bgColor")) efl.bgColor = ReadColorNode(bg);
            }
            m_Fills.push_back(efl);
        }
    }
    // borders
    if (pugi::xml_node borders = XmlOoXmlChild(root, "borders")) {
        for (pugi::xml_node b = borders.first_child(); b; b = b.next_sibling()) {
            if (!XmlOoXmlElementIs(b, "border")) continue;
            auto readPr = [](const pugi::xml_node& pr) {
                ExcelBorderPr r;
                r.style = BoundedAttr(XmlOoXmlAttr(pr, "style"), kMaxBorderStyleChars);
                if (auto c = XmlOoXmlChild(pr, "color")) {
                    r.color.rgb = BoundedAttr(XmlOoXmlAttr(c, "rgb"), 16);
                    r.color.theme = BoundedAttr(XmlOoXmlAttr(c, "theme"), 16);
                    r.color.indexed = BoundedAttr(XmlOoXmlAttr(c, "indexed"), 16);
                    r.color.tint = BoundedAttr(XmlOoXmlAttr(c, "tint"), 32);
                }
                return r;
            };
            ExcelBorder eb;
            if (auto n = XmlOoXmlChild(b, "left")) eb.left = readPr(n);
            if (auto n = XmlOoXmlChild(b, "right")) eb.right = readPr(n);
            if (auto n = XmlOoXmlChild(b, "top")) eb.top = readPr(n);
            if (auto n = XmlOoXmlChild(b, "bottom")) eb.bottom = readPr(n);
            if (auto n = XmlOoXmlChild(b, "diagonal")) eb.diagonal = readPr(n);
            m_Borders.push_back(eb);
        }
    }
    // dxfs (differential formats — used by conditional formatting AND
    // table styles). Each dxf can carry a <fill>, a <font> (bold, italic,
    // strike, underline, color, name, size) and a <border>. We mirror the
    // four sub-records in parallel vectors, all keyed by dxfId, so the
    // resolver helpers can stay O(1) while supporting both the existing
    // conditional format pipeline (fill+fontColor) and the new table style
    // pipeline (fill+font+border).
    if (pugi::xml_node dxfs = XmlOoXmlChild(root, "dxfs")) {
        auto wReadBorderPr = [](const pugi::xml_node& sPr) {
            ExcelBorderPr wRes;
            wRes.style = BoundedAttr(XmlOoXmlAttr(sPr, "style"), kMaxBorderStyleChars);
            if (auto wC = XmlOoXmlChild(sPr, "color")) {
                wRes.color.rgb = BoundedAttr(XmlOoXmlAttr(wC, "rgb"), 16);
                wRes.color.theme = BoundedAttr(XmlOoXmlAttr(wC, "theme"), 16);
                wRes.color.indexed = BoundedAttr(XmlOoXmlAttr(wC, "indexed"), 16);
                wRes.color.tint = BoundedAttr(XmlOoXmlAttr(wC, "tint"), 32);
            }
            return wRes;
        };
        for (pugi::xml_node dxf = dxfs.first_child(); dxf; dxf = dxf.next_sibling()) {
            if (!XmlOoXmlElementIs(dxf, "dxf")) continue;
            ExcelFill efl;
            if (auto fl = XmlOoXmlChild(dxf, "fill")) {
                if (auto pat = XmlOoXmlChild(fl, "patternFill")) {
                    efl.patternType = BoundedAttr(XmlOoXmlAttr(pat, "patternType"), kMaxPatternTypeChars);
                    if (auto fg = XmlOoXmlChild(pat, "fgColor")) efl.fgColor = ReadColorNode(fg);
                    if (auto bg = XmlOoXmlChild(pat, "bgColor")) efl.bgColor = ReadColorNode(bg);
                }
            }
            m_DxFills.push_back(efl);
            ExcelColor wFontColor;
            ExcelFont   wFont;
            // dxf <font>: same semantics as <fonts>/<font> for boolean children.
            if (auto wF = XmlOoXmlChild(dxf, "font")) {
                if (auto wN = XmlOoXmlChild(wF, "name")) wFont.name = BoundedAttr(XmlOoXmlAttr(wN, "val"), kMaxFontNameChars);
                if (auto wS = XmlOoXmlChild(wF, "sz"))   wFont.size = XmlOoXmlAttr(wS, "val").as_double(0.0);
                wFont.bold      = XmlBoolChildTrue(XmlOoXmlChild(wF, "b"));
                wFont.italic    = XmlBoolChildTrue(XmlOoXmlChild(wF, "i"));
                wFont.underline = XmlUnderlineTrue(XmlOoXmlChild(wF, "u"));
                wFont.strike    = XmlBoolChildTrue(XmlOoXmlChild(wF, "strike"));
                if (auto wC = XmlOoXmlChild(wF, "color")) {
                    wFontColor = ReadColorNode(wC);
                    wFont.color = wFontColor;
                }
            }
            m_DxFontColors.push_back(wFontColor);
            m_DxFonts.push_back(wFont);
            ExcelBorder wBorder;
            if (auto wB = XmlOoXmlChild(dxf, "border")) {
                if (auto wN = XmlOoXmlChild(wB, "left"))   wBorder.left   = wReadBorderPr(wN);
                if (auto wN = XmlOoXmlChild(wB, "right"))  wBorder.right  = wReadBorderPr(wN);
                if (auto wN = XmlOoXmlChild(wB, "top"))    wBorder.top    = wReadBorderPr(wN);
                if (auto wN = XmlOoXmlChild(wB, "bottom")) wBorder.bottom = wReadBorderPr(wN);
            }
            m_DxBorders.push_back(wBorder);
        }
    }
    // tableStyles (custom). Each <tableStyle> entry maps an element type
    // (wholeTable, headerRow, totalsRow, firstRowStripe, secondRowStripe,
    // firstColumnStripe, secondColumnStripe, firstHeaderCell,
    // lastHeaderCell, firstTotalCell, lastTotalCell, firstColumn,
    // lastColumn, firstColumnSubheading, secondColumnSubheading,
    // firstRowSubheading, secondRowSubheading, blankRow, pageFieldLabels,
    // pageFieldValues) to a dxfId. Built-in styles like TableStyleMedium2
    // are NOT stored here — they live in the application and are handled
    // by BuildBuiltinTableStyleElementCss instead.
    if (pugi::xml_node wTs = XmlOoXmlChild(root, "tableStyles")) {
        for (pugi::xml_node wStyle = wTs.first_child(); wStyle; wStyle = wStyle.next_sibling()) {
            if (!XmlOoXmlElementIs(wStyle, "tableStyle")) continue;
            tString wName = BoundedAttr(XmlOoXmlAttr(wStyle, "name"), 128);
            if (wName.empty()) continue;
            TableStyleDef wDef;
            for (pugi::xml_node wEl = wStyle.first_child(); wEl; wEl = wEl.next_sibling()) {
                if (!XmlOoXmlElementIs(wEl, "tableStyleElement")) continue;
                tString wType = BoundedAttr(XmlOoXmlAttr(wEl, "type"), 64);
                tInt    wDxf  = XmlOoXmlAttr(wEl, "dxfId").as_int(-1);
                if (!wType.empty() && wDxf >= 0) {
                    wDef.elementToDxfId[wType] = wDxf;
                }
            }
            if (!wDef.elementToDxfId.empty()) {
                m_TableStyles[wName] = std::move(wDef);
            }
        }
    }
    // cellXfs with numFmtId per xf and fk to parts
    if (pugi::xml_node cellXfs = XmlOoXmlChild(root, "cellXfs")) {
        for (pugi::xml_node xf = cellXfs.first_child(); xf; xf = xf.next_sibling()) {
            if (!XmlOoXmlElementIs(xf, "xf")) continue;
            tInt numFmtId = XmlOoXmlAttr(xf, "numFmtId").as_int(-1);
            m_CellXfsNumFmtId.push_back(numFmtId);
            m_CellXfsFontId.push_back(XmlOoXmlAttr(xf, "fontId").as_int(-1));
            m_CellXfsFillId.push_back(XmlOoXmlAttr(xf, "fillId").as_int(-1));
            m_CellXfsBorderId.push_back(XmlOoXmlAttr(xf, "borderId").as_int(-1));
            tString halign = ""; tString valign = ""; tBool wrap=false; tBool shrink=false; tInt indent=-1; tInt textRot=-1; tInt readOrder=-1;
            if (auto al = XmlOoXmlChild(xf, "alignment")) {
                halign = BoundedAttr(XmlOoXmlAttr(al, "horizontal"), kMaxAlignAttrChars);
                valign = BoundedAttr(XmlOoXmlAttr(al, "vertical"), kMaxAlignAttrChars);
                wrap = XmlOoXmlAttr(al, "wrapText").as_bool(false);
                shrink = XmlOoXmlAttr(al, "shrinkToFit").as_bool(false);
                indent = XmlOoXmlAttr(al, "indent").as_int(-1);
                textRot = XmlOoXmlAttr(al, "textRotation").as_int(-1);
                readOrder = XmlOoXmlAttr(al, "readingOrder").as_int(-1);
            }
            m_CellXfsAlignH.push_back(halign);
            m_CellXfsAlignV.push_back(valign);
            m_CellXfsWrapText.push_back(wrap);
            m_CellXfsShrinkToFit.push_back(shrink);
            m_CellXfsIndent.push_back(indent);
            m_CellXfsTextRotation.push_back(textRot);
            m_CellXfsReadingOrder.push_back(readOrder);
        }
    }
}

tString tExcelPugiXMLReader::ResolvePatternFillBackground(const ExcelFill& sFill) const {
    // Only explicit none means no fill; omitted patternType defaults to solid (OOXML).
    if (sFill.patternType == "none") {
        return "";
    }
    auto wResolveFillComponent = [this](const ExcelColor& sColor) -> tString {
        // indexed 64 = automatic fill color -> transparent, not black.
        if (!sColor.indexed.empty() && std::atoi(sColor.indexed.c_str()) == 64) {
            return "";
        }
        return ResolveColor(sColor);
    };
    tString c = wResolveFillComponent(sFill.fgColor);
    if (c.empty() || c == "transparent") {
        c = wResolveFillComponent(sFill.bgColor);
    }
    return c;
}

tString tExcelPugiXMLReader::ResolveDxfFillColor(tInt sDxfId) const {
    if (sDxfId < 0 || sDxfId >= (tInt)m_DxFills.size()) return "";
    return ResolvePatternFillBackground(m_DxFills[sDxfId]);
}

tString tExcelPugiXMLReader::ResolveDxfFontColor(tInt sDxfId) const {
    if (sDxfId < 0 || sDxfId >= (tInt)m_DxFontColors.size()) return "";
    return ResolveColor(m_DxFontColors[sDxfId]);
}

void tExcelPugiXMLReader::MapBorderStyle(const tString& sStyle, const char*& sCssStyle, tInt& sWidth) {
    sCssStyle = "solid";
    sWidth = 1;
    if      (sStyle == "thin")             { sCssStyle = "solid";  sWidth = 1; }
    else if (sStyle == "medium")           { sCssStyle = "solid";  sWidth = 2; }
    else if (sStyle == "thick")            { sCssStyle = "solid";  sWidth = 3; }
    else if (sStyle == "hair")             { sCssStyle = "dotted"; sWidth = 1; }
    else if (sStyle == "dotted")           { sCssStyle = "dotted"; sWidth = 1; }
    else if (sStyle == "dashed")           { sCssStyle = "dashed"; sWidth = 1; }
    else if (sStyle == "mediumDashed")     { sCssStyle = "dashed"; sWidth = 2; }
    else if (sStyle == "dashDot")          { sCssStyle = "dashed"; sWidth = 1; }
    else if (sStyle == "mediumDashDot")    { sCssStyle = "dashed"; sWidth = 2; }
    else if (sStyle == "dashDotDot")       { sCssStyle = "dashed"; sWidth = 1; }
    else if (sStyle == "mediumDashDotDot") { sCssStyle = "dashed"; sWidth = 2; }
    else if (sStyle == "slantDashDot")     { sCssStyle = "dashed"; sWidth = 1; }
    else if (sStyle == "double")           { sCssStyle = "double"; sWidth = 3; }
}

void tExcelPugiXMLReader::EmitBorderEdgeCss(std::ostringstream& sCss, const char* sProperty, const ExcelBorderPr& sEdge) const {
    if (sEdge.style.empty()) return;
    const char* wCssStyle = "solid";
    tInt wWidth = 1;
    MapBorderStyle(sEdge.style, wCssStyle, wWidth);
    tString wCol = ResolveColor(sEdge.color);
    // Excel "automatic" color (indexed=64) is not in the indexed palette
    // so ResolveColor returns empty. Default to black to match Excel's
    // rendering of unspecified border colors.
    if (wCol.empty()) wCol = "000000";
    sCss << sProperty << ":" << wCssStyle << " " << wWidth << "px"
         << " #" << wCol << ";";
}

tString tExcelPugiXMLReader::BuildDxfBackgroundCss(tInt sDxfId) const {
    tString wColor = ResolveDxfFillColor(sDxfId);
    if (wColor.empty() || wColor == "transparent") return "";
    return tString("background-color:#") + wColor + ";";
}

tString tExcelPugiXMLReader::BuildDxfFontCss(tInt sDxfId) const {
    if (sDxfId < 0 || sDxfId >= (tInt)m_DxFonts.size()) return "";
    const ExcelFont& wF = m_DxFonts[sDxfId];
    std::ostringstream wCss;
    // We only emit weight/style/decoration when the dxf actively requests
    // them. dxfs commonly carry <b val="0"/> as "no override"; we already
    // filtered those out at parse time so any true here is intentional.
    if (wF.bold)   wCss << "font-weight:bold;";
    if (wF.italic) wCss << "font-style:italic;";
    if (wF.underline || wF.strike) {
        wCss << "text-decoration-line:";
        if (wF.underline) wCss << " underline";
        if (wF.strike)    wCss << " line-through";
        wCss << ";";
    }
    tString wColor = ResolveDxfFontColor(sDxfId);
    if (!wColor.empty() && wColor != "transparent") {
        wCss << "color:#" << wColor << ";";
    }
    return wCss.str().c_str();
}

tString tExcelPugiXMLReader::BuildDxfBordersCss(tInt sDxfId) const {
    if (sDxfId < 0 || sDxfId >= (tInt)m_DxBorders.size()) return "";
    const ExcelBorder& wB = m_DxBorders[sDxfId];
    std::ostringstream wCss;
    EmitBorderEdgeCss(wCss, "border-left",   wB.left);
    EmitBorderEdgeCss(wCss, "border-top",    wB.top);
    EmitBorderEdgeCss(wCss, "border-right",  wB.right);
    EmitBorderEdgeCss(wCss, "border-bottom", wB.bottom);
    return wCss.str().c_str();
}

tInt tExcelPugiXMLReader::GetTableStyleElementDxfId(const tString& sStyleName, const tString& sElementType) const {
    auto wIt = m_TableStyles.find(sStyleName);
    if (wIt == m_TableStyles.end()) return -1;
    auto wEl = wIt->second.elementToDxfId.find(sElementType);
    if (wEl == wIt->second.elementToDxfId.end()) return -1;
    return wEl->second;
}

tString tExcelPugiXMLReader::BuildBuiltinTableStyleElementCss(const tString& sStyleName, const tString& sElementType) const {
    // Generic resolver for Excel built-in table styles. Excel ships
    // ~60 named styles split in three families with a regular structure:
    //
    //   TableStyleLight  1 .. 21  (3 banks of 7: family + 6 accents)
    //   TableStyleMedium 1 .. 28  (4 banks of 7)
    //   TableStyleDark   1 .. 11  (1 bank of 7 + 4 leftovers)
    //
    // Within each family the bank-relative offset (1 = neutral grey,
    // 2..7 = accent1..accent6) drives the color. The bank index changes
    // border weight and stripe intensity but not the hue. We collapse
    // those visual variations to a sensible default per family:
    //   - Light  : bottom-border underline only (no header fill)
    //   - Medium : full header fill + white text + light stripes
    //   - Dark   : full header fill + white text + medium stripes
    // This matches Excel's general rendering well enough to remove the
    // "all white / no theme" import bug, without trying to replicate the
    // 60 distinct visual deltas (which would be costly and brittle).
    auto wAccent = [&](tInt sIdx, tDouble sTint) -> tString {
        // theme indices in clrScheme order are:
        //   0:lt1, 1:dk1, 2:lt2, 3:dk2, 4:accent1 ... 9:accent6
        // sIdx is 1-based on accents (1 -> accent1). sIdx == 0 means
        // "neutral grey" — we use dk1 (typically black/grey) as base.
        if (sIdx <= 0) {
            if (m_ThemeColors.size() < 2) return "";
            tString wRgb = m_ThemeColors[1]; // dk1
            if (sTint != 0.0) wRgb = ApplyTintToSrgb(wRgb, sTint);
            return wRgb;
        }
        tInt wThemeIdx = 3 + sIdx;
        if (wThemeIdx < 0 || wThemeIdx >= (tInt)m_ThemeColors.size()) return "";
        tString wRgb = m_ThemeColors[wThemeIdx];
        if (sTint != 0.0) wRgb = ApplyTintToSrgb(wRgb, sTint);
        return wRgb;
    };

    enum class Family { None, Light, Medium, Dark };
    auto wParseName = [](const tString& sName, Family& sFamily, tInt& sNumber) {
        sFamily = Family::None;
        sNumber = 0;
        const char* wPrefix = "TableStyle";
        const tSize wPrefixLen = std::strlen(wPrefix);
        if (sName.size() <= wPrefixLen || sName.compare(0, wPrefixLen, wPrefix) != 0) return;
        tString wRest = sName.substr(wPrefixLen);
        if      (wRest.compare(0, 5, "Light")  == 0) { sFamily = Family::Light;  wRest = wRest.substr(5); }
        else if (wRest.compare(0, 6, "Medium") == 0) { sFamily = Family::Medium; wRest = wRest.substr(6); }
        else if (wRest.compare(0, 4, "Dark")   == 0) { sFamily = Family::Dark;   wRest = wRest.substr(4); }
        else return;
        if (wRest.empty()) return;
        // Defensive parse: ignore trailing characters Excel does not produce
        // (the spec only allows decimal digits here).
        for (tChar wC : wRest) {
            if (wC < '0' || wC > '9') { sNumber = 0; return; }
        }
        sNumber = std::atoi(wRest.c_str());
    };

    Family wFamily = Family::None;
    tInt   wNumber = 0;
    wParseName(sStyleName, wFamily, wNumber);
    if (wFamily == Family::None || wNumber <= 0) return "";

    // Map the style number to (bank, offset) where offset is 1..7 with
    // 1 = neutral, 2..7 = accent1..accent6. Some bank widths differ
    // slightly between families but the modulo-7 cycle holds for the
    // ranges Excel actually ships.
    auto wOffsetToAccent = [](tInt sOffset) -> tInt {
        // offset 1 -> 0 (neutral), 2..7 -> 1..6 (accent indices)
        return sOffset - 1;
    };
    tInt wOffset = ((wNumber - 1) % 7) + 1;
    tInt wAccentIdx = wOffsetToAccent(wOffset);

    if (wFamily == Family::Light) {
        if (sElementType == "headerRow") {
            tString wRgb = wAccent(wAccentIdx, 0.0);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "border-bottom:solid 2px #" << wRgb << ";";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "totalsRow") {
            tString wRgb = wAccent(wAccentIdx, 0.0);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "border-top:solid 2px #" << wRgb << ";";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "secondRowStripe") {
            // Light family: stripes only on banks 8..21 (offset >= 8 in
            // raw number). Keep the visual subtle to avoid adding noise
            // on banks 1..7.
            if (wNumber < 8) return "";
            tString wRgb = wAccent(wAccentIdx, 0.8);
            if (wRgb.empty()) return "";
            return tString("background-color:#") + wRgb + ";";
        }
        return "";
    }

    if (wFamily == Family::Medium) {
        if (sElementType == "headerRow") {
            tString wRgb = wAccent(wAccentIdx, 0.0);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "background-color:#" << wRgb << ";";
            wCss << "color:#FFFFFF;";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "totalsRow") {
            tString wRgb = wAccent(wAccentIdx, 0.0);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "border-top:solid 2px #" << wRgb << ";";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "secondRowStripe") {
            // Excel paints Medium stripes at ~60% lighten of the accent
            // (tint 0.6 in the OOXML spec). For the neutral bank we use
            // a flat light grey so the table still has visible bands.
            tString wRgb = wAccentIdx <= 0 ? tString("F2F2F2") : wAccent(wAccentIdx, 0.6);
            if (wRgb.empty()) return "";
            return tString("background-color:#") + wRgb + ";";
        }
        if (sElementType == "wholeTable") {
            tString wRgb = wAccent(wAccentIdx, 0.0);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "border-left:solid 1px #"   << wRgb << ";";
            wCss << "border-right:solid 1px #"  << wRgb << ";";
            wCss << "border-top:solid 1px #"    << wRgb << ";";
            wCss << "border-bottom:solid 1px #" << wRgb << ";";
            return wCss.str().c_str();
        }
        return "";
    }

    if (wFamily == Family::Dark) {
        if (sElementType == "headerRow") {
            // Dark variant darkens the accent (tint -0.5) before fill.
            tString wRgb = wAccent(wAccentIdx, -0.5);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "background-color:#" << wRgb << ";";
            wCss << "color:#FFFFFF;";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "secondRowStripe") {
            tString wRgb = wAccent(wAccentIdx, 0.4);
            if (wRgb.empty()) return "";
            return tString("background-color:#") + wRgb + ";";
        }
        if (sElementType == "firstColumn" || sElementType == "lastColumn") {
            // Dark fill required: white text alone is invisible on white/light stripes.
            tString wRgb = wAccent(wAccentIdx, -0.5);
            if (wRgb.empty()) return "font-weight:bold;";
            std::ostringstream wCss;
            wCss << "background-color:#" << wRgb << ";";
            wCss << "color:#FFFFFF;";
            wCss << "font-weight:bold;";
            return wCss.str().c_str();
        }
        if (sElementType == "wholeTable") {
            tString wRgb = wAccent(wAccentIdx, -0.5);
            if (wRgb.empty()) return "";
            std::ostringstream wCss;
            wCss << "border-left:solid 1px #"   << wRgb << ";";
            wCss << "border-right:solid 1px #"  << wRgb << ";";
            wCss << "border-top:solid 1px #"    << wRgb << ";";
            wCss << "border-bottom:solid 1px #" << wRgb << ";";
            return wCss.str().c_str();
        }
        return "";
    }

    return "";
}

tString tExcelPugiXMLReader::ResolveColorFromXmlNode(const pugi::xml_node& sColorNode) const {
    if (!sColorNode) return "";
    ExcelColor ec = ReadColorNode(sColorNode);
    return ResolveColor(ec);
}

void tExcelPugiXMLReader::DisplayStyles() {
    std::cout << "\n=== Styles (xl/styles.xml) ===" << std::endl;
    if (m_CellXfsNumFmtId.empty() && m_CustomNumFmt.empty()) {
#ifdef debugerror
        std::cerr << "No styles loaded. Call ParseStyles() first." << std::endl;
#endif
        return;
    }
    std::cout << "Custom numFmts:" << std::endl;
    for (const auto& wKv : m_CustomNumFmt) {
        std::cout << "  numFmtId=" << wKv.first << " sCode=\"" << wKv.second << "\"" << std::endl;
    }
    std::cout << "Fonts (" << m_Fonts.size() << ")" << std::endl;
    for (tSize wI=0;wI<m_Fonts.size();++wI) {
        const auto& wF = m_Fonts[wI];
        std::cout << "  font["<<wI<<"] name="<<wF.name<<" size="<<wF.size
                  << (wF.bold?" bold":"") << (wF.italic?" italic":"")
                  << (wF.underline?" underline":"") << (wF.strike?" strike":"")
                  << " color(rgb="<<wF.color.rgb<<",theme="<<wF.color.theme<<")" << std::endl;
    }
    std::cout << "Fills (" << m_Fills.size() << ")" << std::endl;
    for (tSize wI=0;wI<m_Fills.size();++wI) {
        const auto& wFl = m_Fills[wI];
        std::cout << "  fill["<<wI<<"] pattern="<<wFl.patternType
                  << " fg(rgb="<<wFl.fgColor.rgb<<") bg(rgb="<<wFl.bgColor.rgb<<")" << std::endl;
    }
    std::cout << "Borders (" << m_Borders.size() << ")" << std::endl;
    for (tSize wI=0;wI<m_Borders.size();++wI) {
        const auto& wB = m_Borders[wI];
        auto wPr = [](const char* wN, const ExcelBorderPr& wP){ std::cout << " "<<wN<<"(style="<<wP.style<<",rgb="<<wP.color.rgb<<")"; };
        std::cout << "  border["<<wI<<"]"; wPr("L",wB.left); wPr("R",wB.right); wPr("T",wB.top); wPr("B",wB.bottom); std::cout<<std::endl;
    }
    std::cout << "cellXfs (style index -> numFmtId, fontId, fillId, borderId):" << std::endl;
    for (tSize wI = 0; wI < m_CellXfsNumFmtId.size(); ++wI) {
        tInt wId = m_CellXfsNumFmtId[wI];
        tString wSCode;
        auto wItC = m_CustomNumFmt.find(wId);
        if (wItC != m_CustomNumFmt.end()) wSCode = wItC->second; else {
            auto wItB = m_BuiltinNumFmt.find(wId);
            if (wItB != m_BuiltinNumFmt.end()) wSCode = wItB->second; else wSCode = "(builtin/unknown)";
        }
        std::cout << "  s=" << wI << " -> numFmtId=" << wId << " sCode=\"" << wSCode
                  << "\" fontId="<< m_CellXfsFontId[wI]
                  << " fillId="<< m_CellXfsFillId[wI]
                  << " borderId="<< m_CellXfsBorderId[wI]
                  << std::endl;
    }
}

// helper
tExcelPugiXMLReader::ExcelColor tExcelPugiXMLReader::ReadColorNode(const pugi::xml_node& node) {
    ExcelColor c;
    c.rgb = BoundedAttr(node.attribute("rgb"), 16);
    c.theme = BoundedAttr(node.attribute("theme"), 16);
    c.indexed = BoundedAttr(node.attribute("indexed"), 16);
    c.tint = BoundedAttr(node.attribute("tint"), 32);
    return c;
}

// Very simple formatting for demo: handle a few built-in ids and dates
tString tExcelPugiXMLReader::FormatNumericWithStyle(tDouble value, tInt sStyleIndex) {
    tInt numFmtId = -1;
    if (sStyleIndex >= 0 && sStyleIndex < (tInt)m_CellXfsNumFmtId.size()) {
        numFmtId = m_CellXfsNumFmtId[sStyleIndex];
    }
    // Custom format
    auto itCustom = m_CustomNumFmt.find(numFmtId);
    if (itCustom != m_CustomNumFmt.end()) {
        return ApplyFormatCode(value, itCustom->second);
    }
    // Built-in
    auto itBuilt = m_BuiltinNumFmt.find(numFmtId);
    if (itBuilt != m_BuiltinNumFmt.end()) {
        return ApplyFormatCode(value, itBuilt->second);
    }
    // Fallback
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%g", value);
    return tString(buf);
}

void tExcelPugiXMLReader::InitializeBuiltinFormats() {
    m_BuiltinNumFmt.clear();
    // Built-in numFmtIds 14/15/16/17/22 are LOCALE-DEPENDENT in Excel itself. We deliberately keep
    // generic mm/dd/yyyy literals here so callers that read raw format codes still see plausible
    // strings, BUT BuildCssForStyle short-circuits these ids to a SkRoot locale-aware predefined
    // key (e.g. datelong) so the renderer resolves the actual format at display time. See
    // BuildCssForStyle in this file for the locale-aware mapping.
    m_BuiltinNumFmt[0] = "General";
    m_BuiltinNumFmt[1] = "0";
    m_BuiltinNumFmt[2] = "0.00";
    m_BuiltinNumFmt[3] = "#,##0";
    m_BuiltinNumFmt[4] = "#,##0.00";
    m_BuiltinNumFmt[9] = "0%";
    m_BuiltinNumFmt[10] = "0.00%";
    m_BuiltinNumFmt[14] = "mm/dd/yyyy";
    m_BuiltinNumFmt[15] = "d-mmm-yy";
    m_BuiltinNumFmt[16] = "d-mmm";
    m_BuiltinNumFmt[17] = "mmm-yy";
    m_BuiltinNumFmt[22] = "mm/dd/yyyy hh:mm";
    // Accounting (simplify to two decimals)
    m_BuiltinNumFmt[37] = "#,##0;(#,##0)";
    m_BuiltinNumFmt[38] = "#,##0;(#,##0)";
    m_BuiltinNumFmt[39] = "#,##0.00;(#,##0.00)";
    m_BuiltinNumFmt[40] = "#,##0.00;(#,##0.00)";
}

// Very small mask interpreter for common patterns
tString tExcelPugiXMLReader::ApplyFormatCode(tDouble value, const tString& sCode) {
    // Percent
    if (sCode.find('%') != tString::npos) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f%%", value * 100.0);
        return tString(buf);
    }
    // Dates (basic): treat numeric as Excel serial date
    if (sCode.find('y') != tString::npos || sCode.find('m') != tString::npos || sCode.find('d') != tString::npos) {
        long days = static_cast<long>(value);
        long unixDays = days - 25569;
        long secs = unixDays * 86400L;
        std::tm tmv = UnixSecondsToUtcTm(secs);
        char buf[64];
        if (sCode.find('h') != tString::npos) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmv);
        } else {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmv);
        }
        return tString(buf);
    }
    // Thousands and decimals
    tInt decimals = 0;
    auto pos = sCode.find('.');
    if (pos != tString::npos) {
        decimals = (tInt)std::count(sCode.begin() + pos + 1, sCode.end(), '0');
    }
    tBool thousand = (sCode.find("#,#") != tString::npos) || (sCode.find(",") != tString::npos && sCode.find("#") != tString::npos);
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(decimals);
    oss << value;
    tString s = oss.str();
    if (thousand) {
        // insert thousand separators
        auto insertSep = [](tString str) {
            tSize dot = str.find('.');
            tSize end = (dot == tString::npos) ? str.size() : dot;
            tInt count = 0;
            for (tSize i = end; i-- > 0;) {
                if (++count == 3 && i != 0) {
                    str.insert(i, ",");
                    count = 0;
                }
            }
            return str;
        };
        s = insertSep(s);
    }
    return s;
}

// american english: returns the Excel formatCode string for a style index
tString tExcelPugiXMLReader::GetFormatCodeForStyle(tInt sStyleIdx) const {
    if (sStyleIdx < 0 || sStyleIdx >= (tInt)m_CellXfsNumFmtId.size()) return "";
    tInt numFmtId = m_CellXfsNumFmtId[sStyleIdx];
    auto itC = m_CustomNumFmt.find(numFmtId);
    if (itC != m_CustomNumFmt.end()) return itC->second;
    auto itB = m_BuiltinNumFmt.find(numFmtId);
    if (itB != m_BuiltinNumFmt.end()) return itB->second;
    return ""; // General or unknown
}

// american english: returns the raw numFmtId for a style index, or -1 if unknown.
tInt tExcelPugiXMLReader::GetNumFmtIdForStyle(tInt sStyleIdx) const {
    if (sStyleIdx < 0 || sStyleIdx >= (tInt)m_CellXfsNumFmtId.size()) return -1;
    return m_CellXfsNumFmtId[sStyleIdx];
}

// Parse workbook.xml to get worksheet information
void tExcelPugiXMLReader::ParseWorkbook() {
#ifdef debuginfo
    std::cout << "\n=== Parsing Workbook XML with PugiXML ===" << std::endl;
#endif
    auto wIt = m_UnzippedFiles.find("xl/workbook.xml");
    if (wIt == m_UnzippedFiles.end()) {
        cerr << "Workbook.xml not found" << std::endl;
        return;
    }
    
    const auto& wWorkbook = wIt->second;
    
    // Parse XML directly from memory using PugiXML
    pugi::xml_document wDoc;
    pugi::xml_parse_result wResult = wDoc.load_buffer(wWorkbook.content.data(), wWorkbook.content.size());
    
    if (!wResult) {
        cerr << "Error parsing workbook XML: " << wResult.description() << std::endl;
        return;
    }
    
    // Find the sheets element
    pugi::xml_node wSheets = wDoc.child("workbook").child("sheets");
    if (!wSheets) {
        cerr << "No sheets element found in workbook" << std::endl;
        return;
    }
    
    // Clear previous data
    m_WorksheetNames.clear();
    m_WorksheetCount = 0;
    
    // Extract sheet information
    for (pugi::xml_node wSheet : wSheets.children("sheet")) {
        pugi::xml_attribute wNameAttr = wSheet.attribute("name");
        if (wNameAttr) {
            m_WorksheetNames.push_back(SheetNameFromAttribute(wNameAttr));
            m_WorksheetCount++;
#ifdef debuginfo
            std::cout << "Found sheet: \"" << wNameAttr.value() << "\"" << std::endl;
#endif
        }
    }
#ifdef debuginfo    
    std::cout << "✓ Workbook XML parsed successfully with PugiXML" << std::endl;
#endif
}

const tString& tExcelPugiXMLReader::GetSharedStringRichCss(tSize idx) const {
    static const tString wEmpty;
    if (idx >= m_SharedStringRichCss.size()) return wEmpty;
    return m_SharedStringRichCss[idx];
}

tString tExcelPugiXMLReader::BuildRichTextUniformCss(const pugi::xml_node& siOrIs) const {
    return RichTextUniformCssFromParent(siOrIs);
}

// Parse sharedStrings.xml to get all shared strings
void tExcelPugiXMLReader::ParseSharedStrings() {
#ifdef debuginfo
    std::cout << "\n=== Parsing Shared Strings XML with PugiXML ===" << std::endl;
#endif

    auto wIt = m_UnzippedFiles.find("xl/sharedStrings.xml");
    if (wIt == m_UnzippedFiles.end()) {
#ifdef debugerror
        std::cerr << "No shared strings found (file not present)" << std::endl;
#endif
        return;
    }

    const auto& wSharedStrings = wIt->second;

    // Build m_SharedStrings strictly per <si> item
    m_SharedStrings.clear();
    m_SharedStringRichCss.clear();

    pugi::xml_document wDoc;
    if (!LoadUnzippedXmlDocument(wSharedStrings, wDoc)) {
#ifdef debugerror
        std::cerr << "✗ Failed to parse sharedStrings.xml" << std::endl;
#endif
        return;
    }

    pugi::xml_node wSst = wDoc.child("sst");
    if (!wSst) {
        for (pugi::xml_node wCh = wDoc.first_child(); wCh; wCh = wCh.next_sibling()) {
            if (XmlOoXmlElementIs(wCh, "sst")) {
                wSst = wCh;
                break;
            }
        }
    }
    if (!wSst) {
#ifdef debugerror
        std::cerr << "sharedStrings: <sst> node not found" << std::endl;
#endif
        return;
    }

    for (pugi::xml_node wChSi = wSst.first_child(); wChSi; wChSi = wChSi.next_sibling()) {
        if (!XmlOoXmlElementIs(wChSi, "si")) continue;
        pugi::xml_node wSi = wChSi;
        tString wAcc;
        // Two shapes: direct <t> or runs <r><t> — cap length for WASM / std::string limits
        if (pugi::xml_node wT = XmlOoXmlChild(wSi, "t")) {
            AppendPugiTextCapped(wAcc, wT.text());
        }
        for (pugi::xml_node wR = wSi.first_child(); wR; wR = wR.next_sibling()) {
            if (!XmlOoXmlElementIs(wR, "r")) continue;
            if (pugi::xml_node wRt = XmlOoXmlChild(wR, "t")) {
                AppendPugiTextCapped(wAcc, wRt.text());
            }
        }
        m_SharedStrings.push_back(std::move(wAcc));
        m_SharedStringRichCss.push_back(RichTextUniformCssFromParent(wSi));
    }
    if (!wSharedStrings.diskPath.empty()) {
        std::cerr << "SkExcel: sharedStrings loaded " << m_SharedStrings.size()
                  << " entries from disk" << std::endl;
    }
#ifdef debuginfo
    std::cout << "✓ Shared Strings loaded: " << m_SharedStrings.size() << std::endl;
#endif
}

// Parse a specific worksheet
void tExcelPugiXMLReader::ParseWorksheet(tInt sSheetNumber) {
    std::cout << "\n=== Parsing Worksheet " << sSheetNumber << " with PugiXML ===" << std::endl;
    
    tString sheetFile = "xl/worksheets/sheet" + std::to_string(sSheetNumber) + ".xml";
    auto it = m_UnzippedFiles.find(sheetFile);
    if (it == m_UnzippedFiles.end()) {
        std::cerr << "Worksheet " << sSheetNumber << " not found" << std::endl;
        return;
    }
    
    const auto& worksheet = it->second;
    
    // Parse XML directly from memory using PugiXML
    if (ParseXMLFromMemory(worksheet.content, "worksheet" + std::to_string(sSheetNumber))) {
        std::cout << "✓ Worksheet " << sSheetNumber << " XML parsed successfully with PugiXML" << std::endl;
        ExtractWorksheetData(sSheetNumber);
    } else {
        std::cerr << "✗ Failed to parse worksheet " << sSheetNumber << " with PugiXML" << std::endl;
    }
}

// Display Excel file structure
void tExcelPugiXMLReader::DisplayStructure() {
    std::cout << "\n=== Excel File Structure ===" << std::endl;
    
    // Key files to highlight
    std::vector<tString> keyFiles = {
        "[Content_Types].xml",
        "_rels/.rels", 
        "xl/workbook.xml",
        "xl/worksheets/",
        "xl/styles.xml",
        "xl/sharedStrings.xml"
    };
    
    for (const auto& file : m_UnzippedFiles) {
        tBool isKeyFile = false;
        for (const auto& key : keyFiles) {
            if (file.first.find(key) != tString::npos) {
                isKeyFile = true;
                break;
            }
        }
        
        if (isKeyFile) {
            std::cout << "✓ " << file.first << " (" << file.second.size << " bytes)" << std::endl;
        }
    }
}


// Display worksheet names
void tExcelPugiXMLReader::DisplayWorksheetNames() {
    std::cout << "\n=== Feuilles Excel trouvées ===" << std::endl;
    if (m_WorksheetNames.empty()) {
        std::cout << "Aucune feuille trouvée." << std::endl;
        return;
    }
    
    for (tSize wI = 0; wI < m_WorksheetNames.size(); wI++) {
        std::cout << "Feuille " << (wI + 1) << ": \"" << m_WorksheetNames[wI] << "\"" << std::endl;
    }
    std::cout << "Total: " << m_WorksheetNames.size() << " feuille(s)" << std::endl;
}


// Extract worksheet data
void tExcelPugiXMLReader::ExtractWorksheetData(tInt sSheetNumber) {
    std::cout << "Worksheet " << sSheetNumber << " data extraction completed" << std::endl;
    // Here you could extract cell data, formulas, etsColor.
}

// List all cells of a worksheet (addresses and values)
void tExcelPugiXMLReader::ListWorksheetCells(tInt sSheetNumber) {
    tString sheetFile = "xl/worksheets/sheet" + std::to_string(sSheetNumber) + ".xml";
    auto it = m_UnzippedFiles.find(sheetFile);
    if (it == m_UnzippedFiles.end()) {
        std::cerr << "Worksheet " << sSheetNumber << " not found" << std::endl;
        return;
    }

    const auto& worksheet = it->second;

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(worksheet.content.data(), worksheet.content.size());
    if (!result) {
        std::cerr << "Error parsing worksheet XML: " << result.description() << std::endl;
        return;
    }

    std::cout << "\n=== Cells in sheet " << sSheetNumber << " ===" << std::endl;

    // Iterate rows and cells: <sheetData><row r="i"><c r="A1" t="s|inlineStr|.."><v>..</v></c></row>
    pugi::xml_node sheetData = doc.child("worksheet").child("sheetData");
    for (pugi::xml_node row : sheetData.children("row")) {
        for (pugi::xml_node cell : row.children("c")) {
            const char* addr = cell.attribute("r").value();
            const char* type = cell.attribute("t").value();
            const char* sAttr = cell.attribute("s").value(); // style index (optional)

            // Value priority: inlineStr/t or v
            tString displayValue;
            if (tString(type) == "inlineStr") {
                pugi::xml_node is = cell.child("is");
                pugi::xml_node t = is.child("t");
                displayValue = t.text().as_string();
            } else {
                pugi::xml_node v = cell.child("v");
                displayValue = v.text().as_string();
            }

            // Shared strings handling (t="s" with index into sharedStrings)
            if (tString(type) == "s") {
                const char* wIdxStr = displayValue.empty() ? "0" : displayValue.c_str();
                char* wEnd = nullptr;
                errno = 0;
                unsigned long wIdx = std::strtoul(wIdxStr, &wEnd, 10);
                if (wEnd != wIdxStr && errno == 0) {
                    while (*wEnd == ' ' || *wEnd == '\t') {
                        ++wEnd;
                    }
                    if (*wEnd == '\0' && wIdx < m_SharedStrings.size()) {
                        displayValue = m_SharedStrings[static_cast<tSize>(wIdx)];
                    }
                }
                // On parse failure keep raw displayValue (same as stoul + catch)
            } else if (tString(type) == "b") {
                // Boolean
                displayValue = (displayValue == "1") ? "TRUE" : "FALSE";
            } else if (tString(type).empty()) {
                // Numeric - apply style-based formatting when available (strtod: no exceptions for WASM)
                if (!displayValue.empty()) {
                    char* wEnd = nullptr;
                    errno = 0;
                    tDouble num = std::strtod(displayValue.c_str(), &wEnd);
                    if (wEnd != displayValue.c_str() && errno == 0) {
                        while (*wEnd == ' ' || *wEnd == '\t') {
                            ++wEnd;
                        }
                        if (*wEnd == '\0') {
                            if (tString(sAttr).size() > 0) {
                                tInt sStyleIdx = std::atoi(sAttr);
                                displayValue = FormatNumericWithStyle(num, sStyleIdx);
                            } else {
                                char buf[64];
                                std::snprintf(buf, sizeof(buf), "%g", num);
                                displayValue = tString(buf);
                            }
                        }
                    }
                }
            }

            // Show formula if present
            tString formula;
            if (pugi::xml_node f = cell.child("f")) {
                formula = f.text().as_string();
            }

            // Print with metadata
            if (!formula.empty()) {
                std::cout << addr << " = " << displayValue << " (f=" << formula << ")";
            } else {
                std::cout << addr << " = " << displayValue;
            }
            if (tString(type).size() > 0) {
                std::cout << " [t=" << type << "]";
            }
            if (tString(sAttr).size() > 0) {
                std::cout << " [s=" << sAttr;
                tInt sStyleIdx = std::atoi(sAttr);
                // Fill colors (background/foreground)
                if (sStyleIdx >= 0 && sStyleIdx < (tInt)m_CellXfsFillId.size()) {
                    tInt fillId = m_CellXfsFillId[sStyleIdx];
                    if (fillId >= 0 && fillId < (tInt)m_Fills.size()) {
                        const auto& fill = m_Fills[fillId];
                        tString fg = ResolveColor(fill.fgColor);
                        tString bg = ResolveColor(fill.bgColor);
                        std::cout << ", fill=" << fillId
                                  << ", fg=" << (fg.empty()?"-":fg)
                                  << ", bg=" << (bg.empty()?"-":bg)
                                  << ", pattern=" << (fill.patternType.empty()?"-":fill.patternType);
                    }
                }
                // Font info
                if (sStyleIdx >= 0 && sStyleIdx < (tInt)m_CellXfsFontId.size()) {
                    tInt fontId = m_CellXfsFontId[sStyleIdx];
                    if (fontId >= 0 && fontId < (tInt)m_Fonts.size()) {
                        const auto& f = m_Fonts[fontId];
                        tString fcol = ResolveColor(f.color);
                        std::cout << ", font=" << fontId
                                  << "(name=" << (f.name.empty()?"-":f.name)
                                  << ", sz=" << (f.size>0?f.size:0)
                                  << ", color=" << (fcol.empty()?"-":fcol)
                                  << (f.bold?",bold":"") << (f.italic?",italic":"")
                                  << (f.underline?",underline":"") << (f.strike?",strike":"")
                                  << ")";
                    }
                }
                // Border info (print styles of sides if any)
                if (sStyleIdx >= 0 && sStyleIdx < (tInt)m_CellXfsBorderId.size()) {
                    tInt borderId = m_CellXfsBorderId[sStyleIdx];
                    if (borderId >= 0 && borderId < (tInt)m_Borders.size()) {
                        const auto& b = m_Borders[borderId];
                        auto side = [](const ExcelBorderPr& p){
                            if (p.style.empty()) return tString("-");
                            return p.style + (p.color.rgb.empty()?"":"/"+p.color.rgb);
                        };
                        std::cout << ", border=" << borderId
                                  << "(L=" << side(b.left)
                                  << ",T=" << side(b.top)
                                  << ",R=" << side(b.right)
                                  << ",B=" << side(b.bottom) << ")";
                    }
                }
                // Alignment if any
                if (sStyleIdx >= 0 && sStyleIdx < (tInt)m_CellXfsAlignH.size()) {
                    const tString& ah = m_CellXfsAlignH[sStyleIdx];
                    const tString& av = m_CellXfsAlignV[sStyleIdx];
                    tBool wrap = m_CellXfsWrapText[sStyleIdx];
                    tBool shrink = m_CellXfsShrinkToFit[sStyleIdx];
                    tInt indent = m_CellXfsIndent[sStyleIdx];
                    if (!ah.empty() || !av.empty() || wrap || shrink || indent>=0) {
                        std::cout << ", align=" << (ah.empty()?"-":ah) << "/" << (av.empty()?"-":av);
                        if (wrap) std::cout << ", wrap";
                        if (shrink) std::cout << ", shrink";
                        if (indent>=0) std::cout << ", indent=" << indent;
                        tInt rot = m_CellXfsTextRotation[sStyleIdx];
                        if (rot>=0) {
                            // Excel normalization: 255 => vertical; 0..90 => deg; 91..180 => -(rot-90)
                            if (rot == 255) {
                                std::cout << ", rotate=vertical";
                            } else if (rot <= 90) {
                                std::cout << ", rotate=" << rot << "deg";
                            } else if (rot <= 180) {
                                tInt norm = -(rot - 90); // e.g. 180 => -90, 135 => -45
                                std::cout << ", rotate=" << norm << "deg";
                            } else {
                                std::cout << ", rotate=" << rot; // fallback
                            }
                        }
                        tInt ro = m_CellXfsReadingOrder[sStyleIdx];
                        if (ro>=0) std::cout << ", readingOrder=" << ro;
                    }
                }
                std::cout << "]";
            }
            std::cout << std::endl;
        }
    }
}

// Display conditional formatting rules and formulas across all worksheets
void tExcelPugiXMLReader::DisplayConditionalFormattingFormulas() {
    if (m_UnzippedFiles.empty()) {
        std::cout << "No workbook loaded." << std::endl;
        return;
    }
    std::cout << "\n=== Conditional Formatting (per sheet) ===" << std::endl;
    for (tInt wS = 1; ; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wIt = m_UnzippedFiles.find(wSheetFile);
        if (wIt == m_UnzippedFiles.end()) {
            // stop when next sheet is not found
            break;
        }
        const auto& wWorksheet = wIt->second;
        pugi::xml_document wDoc;
        pugi::xml_parse_result wResult = wDoc.load_buffer(wWorksheet.content.data(), wWorksheet.content.size());
        if (!wResult) continue;

        tString wSheetName = (wS-1)>=0 && (wS-1)<(tInt)m_WorksheetNames.size() ? m_WorksheetNames[wS-1] : ("sheet"+std::to_string(wS));
        std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;

        pugi::xml_node wWs = wDoc.child("worksheet");
        for (pugi::xml_node wCf : wWs.children("conditionalFormatting")) {
            tString wSqref = wCf.attribute("sqref").as_string("");
            for (pugi::xml_node wRule : wCf.children("cfRule")) {
                tString wType = wRule.attribute("type").as_string("");
                tInt wDxfId = wRule.attribute("dxfId").as_int(-1);
                tString wPriority = wRule.attribute("priority").as_string("");
                // collect all <formula> nodes
                std::vector<tString> wFormulas;
                for (pugi::xml_node wF : wRule.children("formula")) {
                    wFormulas.push_back(wF.text().as_string(""));
                }
                std::cout << "sqref=" << (wSqref.empty()?"-":wSqref)
                          << ", type=" << (wType.empty()?"-":wType)
                          << ", priority=" << (wPriority.empty()?"-":wPriority);
                if (wDxfId>=0) {
                    tString wColor = ResolveDxfFillColor(wDxfId);
                    if (!wColor.empty()) std::cout << ", fillColor=#" << wColor;
                    tString wFontColor = ResolveDxfFontColor(wDxfId);
                    if (!wFontColor.empty()) std::cout << ", fontColor=#" << wFontColor;
                }
                if (!wFormulas.empty()) {
                    std::cout << ", formulas=[";
                    for (tSize i=0;i<wFormulas.size();++i) {
                        if (i) std::cout << "; ";
                        std::cout << wFormulas[i];
                    }
                    std::cout << "]";
                }
                std::cout << std::endl;
            }
        }
    }
}

// Display visual conditional formats (dataBars, colorScales, iconSets)
void tExcelPugiXMLReader::DisplayConditionalFormattingVisuals() {
    if (m_UnzippedFiles.empty()) { std::cout << "No workbook loaded." << std::endl; return; }
    std::cout << "\n=== Conditional Formatting Visuals (per sheet) ===" << std::endl;
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wIt = m_UnzippedFiles.find(wSheetFile);
        if (wIt == m_UnzippedFiles.end()) break;
        const auto& wWorksheet = wIt->second;
        pugi::xml_document wDoc;
        pugi::xml_parse_result wResult = wDoc.load_buffer(wWorksheet.content.data(), wWorksheet.content.size());
        if (!wResult) continue;
        tString wSheetName = (wS-1)>=0 && (wS-1)<(tInt)m_WorksheetNames.size() ? m_WorksheetNames[wS-1] : ("sheet"+std::to_string(wS));
        std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;
        pugi::xml_node wWs = wDoc.child("worksheet");
        tBool wAny=false;
        for (pugi::xml_node wCf : wWs.children("conditionalFormatting")) {
            tString wSqref = wCf.attribute("sqref").as_string("");
            for (pugi::xml_node wRule : wCf.children("cfRule")) {
                tString wType = wRule.attribute("type").as_string("");
                if (wType=="dataBar") {
                    tString wAxisPos = wRule.child("dataBar").attribute("axisPosition").as_string("");
                    std::cout << "sqref=" << wSqref << ", dataBar (axis=" << (wAxisPos.empty()?"auto":wAxisPos) << ")" << std::endl; wAny=true;
                } else if (wType=="colorScale") {
                    tInt wStops=0; for (auto n : wRule.child("colorScale").children()) { (void)n; ++wStops; }
                    std::cout << "sqref=" << wSqref << ", colorScale (stops=" << wStops << ")" << std::endl; wAny=true;
                } else if (wType=="iconSet") {
                    tString wSet = wRule.child("iconSet").attribute("iconSet").as_string("");
                    std::cout << "sqref=" << wSqref << ", iconSet (set=" << (wSet.empty()?"default":wSet) << ")" << std::endl; wAny=true;
                }
            }
        }
        if (!wAny) std::cout << "(no visual conditional formats)" << std::endl;
    }
}

// Display legacy comments and threaded comments, resolved per sheet via worksheet rels
void tExcelPugiXMLReader::DisplayComments() {
    if (m_UnzippedFiles.empty()) { std::cout << "No workbook loaded." << std::endl; return; }
    std::cout << "\n=== Comments / Notes (per sheet) ===" << std::endl;
    for (tInt wS = 1;; ++wS) {
        tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wIt = m_UnzippedFiles.find(wRelsPath);
        if (wIt == m_UnzippedFiles.end()) {
            // If no rels for this sheet, try next; stop when also no sheet exists
            tString wSheetPath = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
            if (m_UnzippedFiles.find(wSheetPath) == m_UnzippedFiles.end()) break;
            else continue;
        }
        tString wSheetName = (wS-1)>=0 && (wS-1)<(tInt)m_WorksheetNames.size() ? m_WorksheetNames[wS-1] : ("sheet"+std::to_string(wS));
        std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;

        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wIt->second.content.data(), wIt->second.content.size())) continue;

        auto resolveTarget = [](const tString& baseDir, const char* target) -> tString {
            if (!target || !*target) return tString();
            tString t = target;
            if (t.rfind("../", 0) == 0) {
                // base: xl/worksheets/_rels/ -> up one to xl/worksheets/, up again to xl/
                // For simplicity: map ../X to xl/X
                return tString("xl/") + t.substr(3);
            }
            return baseDir + t;
        };

        // Legacy comments
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            const char* wTargetAttr = wRel.attribute("Target").as_string("");
            if (wType.find("/comments") != tString::npos && wType.find("threaded") == tString::npos) {
                tString wTarget = resolveTarget("xl/worksheets/_rels/", wTargetAttr);
                auto wCmtIt = m_UnzippedFiles.find(wTarget);
                if (wCmtIt != m_UnzippedFiles.end()) {
                    pugi::xml_document wDoc;
                    if (wDoc.load_buffer(wCmtIt->second.content.data(), wCmtIt->second.content.size())) {
                        pugi::xml_node wList = wDoc.child("comments").child("commentList");
                        for (pugi::xml_node wCom : wList.children("comment")) {
                            const char* wRef = wCom.attribute("ref").as_string("");
                            const char* wAuthorId = wCom.attribute("authorId").as_string("");
                            tString wText;
                            for (pugi::xml_node wT : wCom.child("text").children("t")) {
                                wText += wT.text().as_string("");
                            }
                            std::cout << "cell=" << (wRef[0]?wRef:"-")
                                      << ", authorId=" << (wAuthorId[0]?wAuthorId:"-")
                                      << ", text=\"" << wText << "\"" << std::endl;
                        }
                    }
                }
            }
        }

        // Threaded comments
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            const char* wTargetAttr = wRel.attribute("Target").as_string("");
            if (wType.find("threadedComments") != tString::npos) {
                tString wTarget = resolveTarget("xl/worksheets/_rels/", wTargetAttr);
                auto wTcIt = m_UnzippedFiles.find(wTarget);
                if (wTcIt != m_UnzippedFiles.end()) {
                    pugi::xml_document wDoc;
                    if (wDoc.load_buffer(wTcIt->second.content.data(), wTcIt->second.content.size())) {
                        for (pugi::xml_node wTc : wDoc.child("threadedComments").children("threadedComment")) {
                            const char* wRef = wTc.attribute("ref").as_string("");
                            const char* wDtid = wTc.attribute("dT").as_string("");
                            tString wText = wTc.child("text").text().as_string("");
                            std::cout << "cell=" << (wRef[0]?wRef:"-")
                                      << ", text=\"" << wText << "\"";
                            if (wDtid && *wDtid) std::cout << ", dateTime=" << wDtid;
                            std::cout << std::endl;
                        }
                    }
                }
            }
        }
    }
}

// Display charts (per sheet): detects drawings relationships to chart parts and prints chart type and series
void tExcelPugiXMLReader::DisplayCharts() {
    if (m_UnzippedFiles.empty()) { std::cout << "No workbook loaded." << std::endl; return; }
    std::cout << "\n=== Charts (per sheet) ===" << std::endl;
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = m_UnzippedFiles.find(wSheetFile);
        if (wItSheet == m_UnzippedFiles.end()) break;

        tString wSheetName = (wS-1)>=0 && (wS-1)<(tInt)m_WorksheetNames.size() ? m_WorksheetNames[wS-1] : ("sheet"+std::to_string(wS));
        std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;

        // Find drawing rels from sheet rels
        tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = m_UnzippedFiles.find(wRelsPath);
        if (wItRels == m_UnzippedFiles.end()) { std::cout << "(no drawings)" << std::endl; continue; }
        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->second.content.data(), wItRels->second.content.size())) continue;

        std::vector<tString> wDrawingTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/drawing") != tString::npos) {
                tString wTarget = wRel.attribute("Target").as_string("");
                if (wTarget.rfind("../",0)==0) wTarget = tString("xl/") + wTarget.substr(3);
                else wTarget = tString("xl/worksheets/_rels/") + wTarget; // fallback
                // Normalize to drawings/drawingN.xml
                if (wTarget.find("xl/drawings/") == tString::npos) {
                    tSize p = wTarget.find("drawings/");
                    if (p != tString::npos) wTarget = tString("xl/") + wTarget.substr(p);
                }
                wDrawingTargets.push_back(wTarget);
            }
        }

        if (wDrawingTargets.empty()) { std::cout << "(no drawings)" << std::endl; continue; }

        // For each drawing, open its XML, build r:id -> position map, then open rels and find chart parts
        for (const tString& wDrawPath : wDrawingTargets) {
            // Load drawing XML to capture anchors and map r:id to positions
            std::map<tString, tString> wRelIdToPos;
            std::map<tString, std::pair<long long,long long>> wRelIdToExt; // width/height in EMUs when available
            auto wItDrawXml = m_UnzippedFiles.find(wDrawPath);
            if (wItDrawXml != m_UnzippedFiles.end()) {
                pugi::xml_document wDrawDoc;
                if (wDrawDoc.load_buffer(wItDrawXml->second.content.data(), wItDrawXml->second.content.size())) {
                    auto wEmuToPx = [](long long wEmu, tDouble wDpi = 96.0) -> tDouble {
                        // american english: 1 inch = 914400 EMUs. pixels = inches * DPI
                        const tDouble wEmusPerInch = 914400.0;
                        return (tDouble)wEmu / wEmusPerInch * wDpi;
                    };
                    auto wColLetters = [](tInt wCol) {
                        // zero-based col -> letters
                        tString w; tInt c = wCol;
                        do { tInt r = c % 26; w.insert(w.begin(), char('A' + r)); c = c / 26 - 1; } while (c >= 0);
                        return w;
                    };
                    auto wCellA1 = [&](tInt wRow, tInt wCol) {
                        return wColLetters(wCol) + std::to_string(wRow + 1);
                    };
                    auto wReadPos = [&](const pugi::xml_node& wFrom, const pugi::xml_node& wTo) {
                        tInt wFromCol = wFrom.child("xdr:col").text().as_int(wFrom.child("col").text().as_int(-1));
                        tInt wFromRow = wFrom.child("xdr:row").text().as_int(wFrom.child("row").text().as_int(-1));
                        long long wFromColOff = 0; long long wFromRowOff = 0;
                        if (auto n = wFrom.child("xdr:colOff")) { wFromColOff = atoll(n.text().as_string("0")); }
                        else if (auto n2 = wFrom.child("colOff")) { wFromColOff = atoll(n2.text().as_string("0")); }
                        if (auto n = wFrom.child("xdr:rowOff")) { wFromRowOff = atoll(n.text().as_string("0")); }
                        else if (auto n2 = wFrom.child("rowOff")) { wFromRowOff = atoll(n2.text().as_string("0")); }

                        tInt wToCol = -1, wToRow = -1; long long wToColOff = 0, wToRowOff = 0; tBool wHasTo=false;
                        if (wTo) {
                            wHasTo=true;
                            wToCol = wTo.child("xdr:col").text().as_int(wTo.child("col").text().as_int(-1));
                            wToRow = wTo.child("xdr:row").text().as_int(wTo.child("row").text().as_int(-1));
                            if (auto n = wTo.child("xdr:colOff")) { wToColOff = atoll(n.text().as_string("0")); }
                            else if (auto n2 = wTo.child("colOff")) { wToColOff = atoll(n2.text().as_string("0")); }
                            if (auto n = wTo.child("xdr:rowOff")) { wToRowOff = atoll(n.text().as_string("0")); }
                            else if (auto n2 = wTo.child("rowOff")) { wToRowOff = atoll(n2.text().as_string("0")); }
                        }
                        std::ostringstream w;
                        if (wFromRow>=0 && wFromCol>=0) {
                            w << wCellA1(wFromRow, wFromCol)
                              << " + (colOff=" << wFromColOff << "emu/" << (tInt)std::round(wEmuToPx(wFromColOff)) << "px"
                              << ", rowOff=" << wFromRowOff << "emu/" << (tInt)std::round(wEmuToPx(wFromRowOff)) << "px)";
                            if (wHasTo && wToRow>=0 && wToCol>=0) {
                                w << " -> " << wCellA1(wToRow, wToCol)
                                  << " + (colOff=" << wToColOff << "emu/" << (tInt)std::round(wEmuToPx(wToColOff)) << "px"
                                  << ", rowOff=" << wToRowOff << "emu/" << (tInt)std::round(wEmuToPx(wToRowOff)) << "px)";
                                // If both offsets present, show delta offsets in EMUs and px
                                long long wDx = (wToColOff - wFromColOff);
                                long long wDy = (wToRowOff - wFromRowOff);
                                w << " [dColOff=" << wDx << "emu/" << (tInt)std::round(wEmuToPx(wDx)) << "px"
                                  << ", dRowOff=" << wDy << "emu/" << (tInt)std::round(wEmuToPx(wDy)) << "px]";
                            }
                        } else {
                            w << "(unknown)";
                        }
                        return w.str();
                    };
                    auto wBind = [&](const pugi::xml_node& wAnchor){
                        // Find graphicFrame -> c:chart r:id
                        pugi::xml_node wFrame = wAnchor.child("xdr:graphicFrame"); if (!wFrame) wFrame = wAnchor.child("graphicFrame");
                        if (!wFrame) return; 
                        pugi::xml_node wGraphic = wFrame.child("a:graphic"); if (!wGraphic) wGraphic = wFrame.child("graphic");
                        if (!wGraphic) return;
                        pugi::xml_node wGd = wGraphic.child("a:graphicData"); if (!wGd) wGd = wGraphic.child("graphicData");
                        if (!wGd) return;
                        pugi::xml_node wC = wGd.child("c:chart"); if (!wC) wC = wGd.child("chart");
                        if (!wC) return;
                        const char* wRid = wC.attribute("r:id").as_string("");
                        if (!wRid || !*wRid) return;
                        tString wPos;
                        if (tString(wAnchor.name()) == "xdr:twoCellAnchor" || tString(wAnchor.name()) == "twoCellAnchor") {
                            wPos = wReadPos(wAnchor.child("xdr:from") ? wAnchor.child("xdr:from") : wAnchor.child("from"),
                                            wAnchor.child("xdr:to") ? wAnchor.child("xdr:to") : wAnchor.child("to"));
                        } else if (tString(wAnchor.name()) == "xdr:oneCellAnchor" || tString(wAnchor.name()) == "oneCellAnchor") {
                            wPos = wReadPos(wAnchor.child("xdr:from") ? wAnchor.child("xdr:from") : wAnchor.child("from"), pugi::xml_node());
                            long long wCx = wAnchor.child("xdr:ext").attribute("cx").as_llong(wAnchor.child("ext").attribute("cx").as_llong(0));
                            long long wCy = wAnchor.child("xdr:ext").attribute("cy").as_llong(wAnchor.child("ext").attribute("cy").as_llong(0));
                            if (wCx>0 && wCy>0) wRelIdToExt[wRid] = {wCx, wCy};
                        } else if (tString(wAnchor.name()) == "xdr:absoluteAnchor" || tString(wAnchor.name()) == "absoluteAnchor") {
                            // absolute position/size in EMUs
                            long long wX = wAnchor.child("xdr:pos").attribute("x").as_llong(wAnchor.child("pos").attribute("x").as_llong(-1));
                            long long wY = wAnchor.child("xdr:pos").attribute("y").as_llong(wAnchor.child("pos").attribute("y").as_llong(-1));
                            long long wCx = wAnchor.child("xdr:ext").attribute("cx").as_llong(wAnchor.child("ext").attribute("cx").as_llong(0));
                            long long wCy = wAnchor.child("xdr:ext").attribute("cy").as_llong(wAnchor.child("ext").attribute("cy").as_llong(0));
                            std::ostringstream w; w << "absolute(x=" << wX << "emu/" << (tInt)std::round(wEmuToPx(wX)) << "px"
                                                     << ", y=" << wY << "emu/" << (tInt)std::round(wEmuToPx(wY)) << "px"
                                                     << ", cx=" << wCx << "emu/" << (tInt)std::round(wEmuToPx(wCx)) << "px"
                                                     << ", cy=" << wCy << "emu/" << (tInt)std::round(wEmuToPx(wCy)) << "px)"; wPos = w.str();
                            if (wCx>0 && wCy>0) wRelIdToExt[wRid] = {wCx, wCy};
                        }
                        if (!wPos.empty()) wRelIdToPos[wRid] = wPos;
                    };
                    pugi::xml_node wRoot = wDrawDoc.child("xdr:wsDr"); if (!wRoot) wRoot = wDrawDoc.child("wsDr");
                    if (wRoot) {
                        for (pugi::xml_node wA : wRoot.children()) {
                            tString wN = wA.name();
                            if (wN == "xdr:twoCellAnchor" || wN == "twoCellAnchor" || wN == "xdr:oneCellAnchor" || wN == "oneCellAnchor" || wN == "xdr:absoluteAnchor" || wN == "absoluteAnchor") {
                                wBind(wA);
                            }
                        }
                    }
                }
            }
            tString wRels = wDrawPath + ".rels";
            // drawings rels live under xl/drawings/_rels/drawingN.xml.rels
            if (wRels.find("xl/drawings/") != tString::npos) {
                wRels.insert(wRels.find("xl/drawings/") + tString("xl/drawings/").size(), "_rels/");
            }
            auto wItDrawRels = m_UnzippedFiles.find(wRels);
            if (wItDrawRels == m_UnzippedFiles.end()) continue;

            pugi::xml_document wDrawRelDoc;
            if (!wDrawRelDoc.load_buffer(wItDrawRels->second.content.data(), wItDrawRels->second.content.size())) continue;

            for (pugi::xml_node wRel : wDrawRelDoc.child("Relationships").children("Relationship")) {
                tString wType = wRel.attribute("Type").as_string("");
                if (wType.find("/chart") != tString::npos) {
                    tString wTarget = wRel.attribute("Target").as_string("");
                    if (wTarget.rfind("../",0)==0) wTarget = tString("xl/") + wTarget.substr(3); else wTarget = tString("xl/drawings/") + wTarget;
                    // Normalize to xl/charts/chartN.xml
                    if (wTarget.find("xl/charts/") == tString::npos) {
                        tSize p = wTarget.find("charts/");
                        if (p != tString::npos) wTarget = tString("xl/") + wTarget.substr(p);
                    }
                    tString wRelId = wRel.attribute("Id").as_string("");
                    auto wItChart = m_UnzippedFiles.find(wTarget);
                    if (wItChart == m_UnzippedFiles.end()) continue;
                    pugi::xml_document wChartDoc;
                    if (!wChartDoc.load_buffer(wItChart->second.content.data(), wItChart->second.content.size())) continue;

                    pugi::xml_node wChartSpace = wChartDoc.child("c:chartSpace");
                    if (!wChartSpace) wChartSpace = wChartDoc.child("chartSpace");
                    pugi::xml_node wChart = wChartSpace.child("c:chart");
                    if (!wChart) wChart = wChartSpace.child("chart");
                    pugi::xml_node wPlot = wChart.child("c:plotArea");
                    if (!wPlot) wPlot = wChart.child("plotArea");

                    // Extract chart title: prefer string ref, else rich text
                    auto readChartTitle = [&](pugi::xml_node wChartNode) -> tString {
                        if (!wChartNode) return tString("");
                        pugi::xml_node wTitle = wChartNode.child("c:title"); if (!wTitle) wTitle = wChartNode.child("title");
                        if (!wTitle) return tString("");
                        // c:title/c:tx/c:strRef/c:f -> formula points to cell containing title
                        if (auto f = wTitle.child("c:tx").child("c:strRef").child("c:f")) {
                            return tString("cell:") + f.text().as_string("");
                        }
                        // c:title/c:tx/c:rich/a:p/a:r/a:t -> concatenate runs
                        pugi::xml_node wRich = wTitle.child("c:tx").child("c:rich"); if (!wRich) wRich = wTitle.child("tx").child("rich");
                        if (wRich) {
                            tString wText;
                            for (pugi::xml_node wP = wRich.child("a:p"); wP; wP = wP.next_sibling("a:p")) {
                                for (pugi::xml_node wR = wP.child("a:r"); wR; wR = wR.next_sibling("a:r")) {
                                    if (auto t = wR.child("a:t")) { wText += t.text().as_string(""); }
                                }
                                // line break between paragraphs
                                if (wP.next_sibling("a:p")) wText += "\n";
                            }
                            return wText;
                        }
                        return tString("");
                    };
                    tString wTitle = readChartTitle(wChart);

                    // Detect type: look for known series containers under plotArea
                    auto printSeries = [&](const char* wTypeName, const char* wSerTag){
                        tBool wPrintedHeader=false;
                        for (pugi::xml_node wSerContainer : wPlot.children(wTypeName)) {
                            for (pugi::xml_node wSer : wSerContainer.children(wSerTag)) {
                                tString wTx;
                                if (auto c = wSer.child("c:tx").child("c:strRef").child("c:f")) wTx = c.text().as_string("");
                                else if (auto c2 = wSer.child("tx").child("strRef").child("f")) wTx = c2.text().as_string("");
                                tString wX;
                                if (auto x = wSer.child("c:cat").child("c:strRef").child("c:f")) wX = x.text().as_string("");
                                else if (auto x2 = wSer.child("cat").child("strRef").child("f")) wX = x2.text().as_string("");
                                tString wY;
                                if (auto y = wSer.child("c:val").child("c:numRef").child("c:f")) wY = y.text().as_string("");
                                else if (auto y2 = wSer.child("val").child("numRef").child("f")) wY = y2.text().as_string("");
                                // Try to get series color from spPr/solidFill
                                auto readSerColor = [&](const pugi::xml_node& wSerNode) -> tString {
                                    pugi::xml_node wSpPr = wSerNode.child("c:spPr"); if (!wSpPr) wSpPr = wSerNode.child("spPr");
                                    if (!wSpPr) return tString("");
                                    pugi::xml_node wFill = wSpPr.child("a:solidFill"); if (!wFill) wFill = wSpPr.child("solidFill");
                                    if (!wFill) return tString("");
                                    auto applyLum = [&](const tString& srgb, tInt lumMod, tInt lumOff) -> tString {
                                        // lumMod/lumOff are in 0..100000. result = off + mod*in
                                        if (srgb.size() != 6) return srgb;
                                        auto hex = [](char c){ if ('0'<=c&&c<='9') return c-'0'; if ('A'<=c&&c<='F') return c-'A'+10; if ('a'<=c&&c<='f') return c-'a'+10; return 0; };
                                        auto clamp255 = [&](tDouble x){ if (x<0) x=0; if (x>255) x=255; return (tInt)std::round(x); };
                                        tDouble mod = std::max(0, std::min(100000, lumMod)) / 100000.0;
                                        tDouble off = std::max(0, std::min(100000, lumOff)) / 100000.0 * 255.0;
                                        tInt r = (hex(srgb[0])<<4) + hex(srgb[1]);
                                        tInt g = (hex(srgb[2])<<4) + hex(srgb[3]);
                                        tInt b = (hex(srgb[4])<<4) + hex(srgb[5]);
                                        tInt R = clamp255(off + mod * r);
                                        tInt G = clamp255(off + mod * g);
                                        tInt B = clamp255(off + mod * b);
                                        char buf[7]; std::snprintf(buf,sizeof(buf),"%02X%02X%02X",R,G,B); return tString(buf);
                                    };
                                    if (auto wSrgb = wFill.child("a:srgbClr")) {
                                        tString v = wSrgb.attribute("val").as_string("");
                                        // optional adjustments
                                        tInt lumMod = -1, lumOff = -1;
                                        if (auto n = wSrgb.child("a:lumMod")) lumMod = n.attribute("val").as_int(-1);
                                        if (auto n = wSrgb.child("a:lumOff")) lumOff = n.attribute("val").as_int(-1);
                                        if (!v.empty()) {
                                            tString base = v; 
                                            if (lumMod>=0 || lumOff>=0) {
                                                if (lumMod<0) lumMod = 100000; if (lumOff<0) lumOff = 0;
                                                base = applyLum(base, lumMod, lumOff);
                                            }
                                            return tString("#") + base;
                                        }
                                    }
                                    if (auto wScheme = wFill.child("a:schemeClr")) {
                                        tString name = wScheme.attribute("val").as_string("");
                                        // Map scheme name to theme index
                                        auto idxOf = [&](const tString& n)->tInt{
                                            if (n=="lt1") return 0; if (n=="dk1") return 1; if (n=="lt2") return 2; if (n=="dk2") return 3;
                                            if (n=="accent1") return 4; if (n=="accent2") return 5; if (n=="accent3") return 6;
                                            if (n=="accent4") return 7; if (n=="accent5") return 8; if (n=="accent6") return 9;
                                            if (n=="hlink") return 10; if (n=="folHlink") return 11; return -1;
                                        };
                                        tInt tidx = idxOf(name);
                                        if (tidx>=0 && tidx < (tInt)m_ThemeColors.size()) {
                                            tString base = m_ThemeColors[tidx];
                                            tInt lumMod = -1, lumOff = -1;
                                            if (auto n = wScheme.child("a:lumMod")) lumMod = n.attribute("val").as_int(-1);
                                            if (auto n = wScheme.child("a:lumOff")) lumOff = n.attribute("val").as_int(-1);
                                            if (lumMod>=0 || lumOff>=0) {
                                                if (lumMod<0) lumMod = 100000; if (lumOff<0) lumOff = 0;
                                                base = applyLum(base, lumMod, lumOff);
                                            }
                                            return tString("#") + base;
                                        }
                                        // fallback without theme resolution
                                        if (!name.empty()) return tString("scheme:") + name;
                                    }
                                    if (auto wSys = wFill.child("a:sysClr")) {
                                        const char* last = wSys.attribute("lastClr").as_string("");
                                        if (last && *last) return tString("#") + last;
                                    }
                                    return tString("");
                                };
                                tString wColor = readSerColor(wSer);
                                if (!wPrintedHeader) {
                                    std::cout << "chartType=" << wTypeName << std::endl;
                                    wPrintedHeader=true;
                                }
                                std::cout << "  series: name=" << (wTx.empty()?"-":wTx)
                                          << ", x=" << (wX.empty()?"-":wX)
                                          << ", y=" << (wY.empty()?"-":wY);
                                if (!wColor.empty()) std::cout << ", color=" << wColor;
                                std::cout << std::endl;
                            }
                        }
                        return wPrintedHeader;
                    };

                    tBool wAny=false;
                    wAny |= printSeries("c:barChart", "c:ser");
                    wAny |= printSeries("barChart", "ser");
                    wAny |= printSeries("c:lineChart", "c:ser");
                    wAny |= printSeries("lineChart", "ser");
                    wAny |= printSeries("c:pieChart", "c:ser");
                    wAny |= printSeries("pieChart", "ser");
                    wAny |= printSeries("c:areaChart", "c:ser");
                    wAny |= printSeries("areaChart", "ser");
                    wAny |= printSeries("c:scatterChart", "c:ser");
                    wAny |= printSeries("scatterChart", "ser");
                    wAny |= printSeries("c:radarChart", "c:ser");
                    wAny |= printSeries("radarChart", "ser");
                    if (!wAny) std::cout << "(chart type not recognized or no series)" << std::endl;
                    // Print title, position and blocks
                    auto wPosIt = wRelIdToPos.find(wRelId);
                    if (!wTitle.empty() || wPosIt != wRelIdToPos.end()) {
                        std::cout << "  ";
                        if (!wTitle.empty()) std::cout << "title=" << wTitle;
                        if (!wTitle.empty() && wPosIt != wRelIdToPos.end()) std::cout << ", ";
                        if (wPosIt != wRelIdToPos.end()) std::cout << "position=" << wPosIt->second;
                        std::cout << std::endl;
                    }
                    // Try to print four blocks positions within the chart (plot area + legend + title + data table if any)
                    // Note: Chart-space blocks are not positioned in drawing anchors; they are inside chart XML with layout.
                    auto readBlock = [&](const char* label, pugi::xml_node node){
                        if (!node) return;
                        pugi::xml_node layout = node.child("c:layout"); if (!layout) layout = node.child("layout");
                        pugi::xml_node manual = layout.child("c:manualLayout"); if (!manual) manual = layout.child("manualLayout");
                        if (!manual) { std::cout << "  " << label << "=(auto layout)" << std::endl; return; }
                        auto get = [&](const char* n) {
                            pugi::xml_node x = manual.child(n);
                            if (!x) {
                                // child() takes const char*. A temporary tString does not convert.
                                tString wName = tString("c:") + n;
                                x = manual.child(wName.c_str());
                            }
                            return x;
                        };
                        tString xMode = get("layoutTarget").attribute("val").as_string(""); // inner/outer
                        tDouble x = get("x").attribute("val").as_double(std::numeric_limits<tDouble>::quiet_NaN());
                        tDouble y = get("y").attribute("val").as_double(std::numeric_limits<tDouble>::quiet_NaN());
                        tDouble w = get("w").attribute("val").as_double(std::numeric_limits<tDouble>::quiet_NaN());
                        tDouble h = get("h").attribute("val").as_double(std::numeric_limits<tDouble>::quiet_NaN());
                        std::cout.setf(std::ios::fixed); std::cout.precision(3);
                        std::cout << "  " << label << "=";
                        if (!xMode.empty()) std::cout << xMode << ":";
                        std::cout << "x=" << (std::isnan(x)?-1.0:x) << ", y=" << (std::isnan(y)?-1.0:y)
                                  << ", w=" << (std::isnan(w)?-1.0:w) << ", h=" << (std::isnan(h)?-1.0:h) << std::endl;
                        std::cout.unsetf(std::ios::fixed);
                    };
                    readBlock("plotArea", wPlot);
                    readBlock("legend", wChart.child("c:legend") ? wChart.child("c:legend") : wChart.child("legend"));
                    readBlock("title", wChart.child("c:title") ? wChart.child("c:title") : wChart.child("title"));
                    readBlock("dTable", wChart.child("c:dTable") ? wChart.child("c:dTable") : wChart.child("dTable"));
                }
            }
        }
    }
}

// Parse XML directly from memory using PugiXML
tBool tExcelPugiXMLReader::ParseXMLFromMemory(const std::vector<char>& sXmlData, const tString& sContext) {
    // load_buffer does not throw; no try/catch (WASM / -fno-exceptions)
    pugi::xml_document wDoc;
    pugi::xml_parse_result wResult = wDoc.load_buffer(sXmlData.data(), sXmlData.size());

    if (!wResult) {
        std::cerr << "  → Error parsing " << sContext << " XML: " << wResult.description() << std::endl;
        return false;
    }

    ParsePugiXMLDirectly(wDoc.root(), 0, sContext);

    std::cout << "  → Parsed " << sContext << " XML from memory (" << sXmlData.size() << " bytes)" << std::endl;
    return true;
}

// Parse PugiXML directly and display structure
void tExcelPugiXMLReader::ParsePugiXMLDirectly(const pugi::xml_node& node, tInt sDepth, const tString& sContext) {
    if (!node) return;
    
    tString indent(sDepth * 2, ' ');
    
    // Handle different node types
    switch (node.type()) {
        case pugi::node_element: {
            std::cout << indent << "  → Element: " << node.name();
            
            // Show attributes
            for (const pugi::xml_attribute& attr : node.attributes()) {
                std::cout << " " << attr.name() << "=\"" << attr.value() << "\"";
            }
            std::cout << std::endl;
            
            // Count worksheets in workbook and extract names
            if (tString(node.name()) == "sheet") {
                m_WorksheetCount++;
                // Extract sheet name from 'name' attribute
                pugi::xml_attribute nameAttr = node.attribute("name");
                if (nameAttr) {
                    m_WorksheetNames.push_back(SheetNameFromAttribute(nameAttr));
                }
            }
            
            // Extract shared strings (debug tree walk; cap like ParseSharedStrings)
            if (tString(node.name()) == "t") {
                tString value;
                AppendPugiTextCapped(value, node.text());
                if (!value.empty()) {
                    m_SharedStrings.push_back(std::move(value));
                }
            }
            
            // Process children
            for (const pugi::xml_node& child : node.children()) {
                ParsePugiXMLDirectly(child, sDepth + 1, sContext);
            }
            break;
        }
        case pugi::node_pcdata: {
            tString value = node.value();
            if (!value.empty() && value.find_first_not_of(" \t\n\r") != tString::npos) {
                std::cout << indent << "  → Text: \"" << value << "\"" << std::endl;
            }
            break;
        }
        default:
            break;
    }
}

namespace {

constexpr zip_uint64_t kUnzipSpillBytes = 8ull * 1024ull * 1024ull;
constexpr size_t kUnzipIoChunk = 1024 * 1024;
constexpr size_t kMaxWorksheetShellBytes = 32ull * 1024ull * 1024ull;
constexpr size_t kMaxWorksheetRowBytes = 32ull * 1024ull * 1024ull;

static tBool EnsureDirPath(const tString& sPath) {
    tString wAccum;
    for (tSize i = 0; i < sPath.size(); ++i) {
        wAccum.push_back(sPath[i]);
        if (sPath[i] == '/' || i + 1 == sPath.size()) {
            tString wDir = wAccum;
            if (!wDir.empty() && wDir.back() == '/') {
                wDir.pop_back();
            }
            if (wDir.empty() || wDir == ".") {
                continue;
            }
            if (SkMkdir(wDir.c_str()) != 0 && errno != EEXIST) {
                return false;
            }
        }
    }
    return true;
}

static tString ParentDir(const tString& sPath) {
    const tSize wSlash = sPath.find_last_of('/');
    if (wSlash == tString::npos) {
        return ".";
    }
    if (wSlash == 0) {
        return "/";
    }
    return sPath.substr(0, wSlash);
}

struct tUnzipByteReader {
    const char* mem = nullptr;
    size_t memSize = 0;
    size_t memOff = 0;
    FILE* fp = nullptr;

    tBool Open(const UnzippedFile& sFile) {
        if (!sFile.diskPath.empty()) {
            fp = std::fopen(sFile.diskPath.c_str(), "rb");
            return fp != nullptr;
        }
        mem = sFile.content.empty() ? nullptr : sFile.content.data();
        memSize = sFile.content.size();
        return true;
    }
    void Close() {
        if (fp) {
            std::fclose(fp);
            fp = nullptr;
        }
    }
    size_t Read(char* sDst, size_t sN) {
        if (fp) {
            return std::fread(sDst, 1, sN, fp);
        }
        if (memOff >= memSize) {
            return 0;
        }
        const size_t wTake = std::min(sN, memSize - memOff);
        if (wTake && mem) {
            std::memcpy(sDst, mem + memOff, wTake);
        }
        memOff += wTake;
        return wTake;
    }
};

static tBool IsRowOpenTag(const std::string& s, size_t sPos) {
    if (sPos + 4 > s.size() || s.compare(sPos, 4, "<row") != 0) {
        return false;
    }
    if (sPos + 4 == s.size()) {
        return false;
    }
    const char wC = s[sPos + 4];
    return wC == ' ' || wC == '\t' || wC == '\n' || wC == '\r' || wC == '>' || wC == '/';
}

static size_t FindRowOpen(const std::string& s, size_t sFrom) {
    size_t wPos = sFrom;
    while ((wPos = s.find("<row", wPos)) != std::string::npos) {
        if (IsRowOpenTag(s, wPos)) {
            return wPos;
        }
        wPos += 4;
    }
    return std::string::npos;
}

static tBool LoadWorksheetShellFromFile(const UnzippedFile& sFile, pugi::xml_document& sOut) {
    tUnzipByteReader wReader;
    if (!wReader.Open(sFile)) {
        return false;
    }
    std::string wHead;
    std::string wTail;
    std::string wCarry;
    enum { kHead, kSkip, kTail } wState = kHead;
    std::vector<char> wChunk(kUnzipIoChunk);
    tBool wOk = true;
    for (;;) {
        const size_t wN = wReader.Read(wChunk.data(), wChunk.size());
        if (wN == 0) {
            break;
        }
        if (wState == kHead) {
            wHead.append(wChunk.data(), wN);
            const size_t wData = wHead.find("<sheetData");
            if (wData != std::string::npos) {
                const size_t wTagEnd = wHead.find('>', wData);
                if (wTagEnd == std::string::npos) {
                    if (wHead.size() > kMaxWorksheetShellBytes) {
                        wOk = false;
                        break;
                    }
                    continue;
                }
                const tBool wEmpty = (wTagEnd > 0 && wHead[wTagEnd - 1] == '/');
                std::string wPrefix = wHead.substr(0, wData);
                if (wEmpty) {
                    wTail = wHead.substr(wTagEnd + 1);
                    wHead.swap(wPrefix);
                    wState = kTail;
                } else {
                    const size_t wClose = wHead.find("</sheetData>", wTagEnd);
                    if (wClose != std::string::npos) {
                        wTail = wHead.substr(wClose + 12);
                        wHead.swap(wPrefix);
                        wState = kTail;
                    } else {
                        wCarry = wHead.substr(std::max(wTagEnd + 1, wHead.size() - 16));
                        wHead.swap(wPrefix);
                        wState = kSkip;
                    }
                }
            } else if (wHead.size() > kMaxWorksheetShellBytes) {
                wOk = false;
                break;
            }
        } else if (wState == kSkip) {
            wCarry.append(wChunk.data(), wN);
            const size_t wClose = wCarry.find("</sheetData>");
            if (wClose != std::string::npos) {
                wTail = wCarry.substr(wClose + 12);
                wCarry.clear();
                wState = kTail;
            } else if (wCarry.size() > 16) {
                wCarry.erase(0, wCarry.size() - 16);
            }
        } else {
            wTail.append(wChunk.data(), wN);
            if (wTail.size() > kMaxWorksheetShellBytes) {
                wOk = false;
                break;
            }
        }
    }
    wReader.Close();
    if (!wOk) {
        std::cerr << "SkExcel: worksheet shell too large for " << sFile.filename << std::endl;
        return false;
    }
    const std::string wXml = wHead + "<sheetData/>" + wTail;
    return sOut.load_buffer(wXml.data(), wXml.size());
}

static tBool ForEachWorksheetRowFromFile(
    const UnzippedFile& sFile,
    const std::function<void(const pugi::xml_node&)>& sFn
) {
    tUnzipByteReader wReader;
    if (!wReader.Open(sFile)) {
        return false;
    }
    std::string wBuf;
    std::vector<char> wChunk(kUnzipIoChunk);
    tBool wInData = false;
    tBool wOk = true;
    size_t wBytesRead = 0;
    size_t wLastLog = 0;
    const size_t wTotal = sFile.size > 0 ? static_cast<size_t>(sFile.size) : 0;
    for (;;) {
        const size_t wN = wReader.Read(wChunk.data(), wChunk.size());
        if (wN == 0) {
            break;
        }
        wBytesRead += wN;
        if (wTotal > 0) {
            const int wPct = static_cast<int>((wBytesRead * 85ull) / wTotal);
            SkExcel::SkExcelReportProgress(std::min(85, std::max(5, wPct)));
            if (wBytesRead - wLastLog >= 32ull * 1024ull * 1024ull) {
                wLastLog = wBytesRead;
                std::cerr << "SkExcel: streaming " << sFile.filename << " "
                          << (wBytesRead / (1024 * 1024)) << " / "
                          << (wTotal / (1024 * 1024)) << " MB" << std::endl;
            }
        }
        wBuf.append(wChunk.data(), wN);
        if (!wInData) {
            const size_t wData = wBuf.find("<sheetData");
            if (wData == std::string::npos) {
                if (wBuf.size() > 64) {
                    wBuf.erase(0, wBuf.size() - 64);
                }
                continue;
            }
            const size_t wTagEnd = wBuf.find('>', wData);
            if (wTagEnd == std::string::npos) {
                continue;
            }
            if (wTagEnd > 0 && wBuf[wTagEnd - 1] == '/') {
                break;
            }
            wBuf.erase(0, wTagEnd + 1);
            wInData = true;
        }
        while (wInData) {
            const size_t wCloseData = wBuf.find("</sheetData>");
            const size_t wRow = FindRowOpen(wBuf, 0);
            if (wRow == std::string::npos) {
                if (wCloseData != std::string::npos) {
                    wOk = true;
                    wReader.Close();
                    return true;
                }
                if (wBuf.size() > 64) {
                    wBuf.erase(0, wBuf.size() - 64);
                }
                break;
            }
            if (wCloseData != std::string::npos && wCloseData < wRow) {
                wReader.Close();
                return true;
            }
            const size_t wOpenEnd = wBuf.find('>', wRow);
            if (wOpenEnd == std::string::npos) {
                if (wRow > 0) {
                    wBuf.erase(0, wRow);
                }
                break;
            }
            if (wBuf[wOpenEnd - 1] == '/') {
                const std::string wRowXml = wBuf.substr(wRow, wOpenEnd + 1 - wRow);
                pugi::xml_document wRowDoc;
                if (wRowDoc.load_buffer(wRowXml.data(), wRowXml.size())) {
                    if (pugi::xml_node wRowNode = wRowDoc.child("row")) {
                        sFn(wRowNode);
                    }
                }
                wBuf.erase(0, wOpenEnd + 1);
                continue;
            }
            const size_t wRowEnd = wBuf.find("</row>", wOpenEnd);
            if (wRowEnd == std::string::npos) {
                if (wRow > 0) {
                    wBuf.erase(0, wRow);
                }
                if (wBuf.size() > kMaxWorksheetRowBytes) {
                    std::cerr << "SkExcel: worksheet row exceeds " << kMaxWorksheetRowBytes
                              << " bytes in " << sFile.filename << std::endl;
                    wOk = false;
                    wReader.Close();
                    return false;
                }
                break;
            }
            const std::string wRowXml = wBuf.substr(wRow, wRowEnd + 6 - wRow);
            pugi::xml_document wRowDoc;
            if (wRowDoc.load_buffer(wRowXml.data(), wRowXml.size())) {
                if (pugi::xml_node wRowNode = wRowDoc.child("row")) {
                    sFn(wRowNode);
                }
            }
            wBuf.erase(0, wRowEnd + 6);
        }
    }
    wReader.Close();
    return wOk;
}

} // namespace

void tExcelPugiXMLReader::CleanupUnzipSpill() {
    for (auto& wPair : m_UnzippedFiles) {
        if (!wPair.second.diskPath.empty()) {
            std::remove(wPair.second.diskPath.c_str());
            wPair.second.diskPath.clear();
        }
    }
    if (!m_UnzipTempDir.empty()) {
        const tString wWs = m_UnzipTempDir + "/xl/worksheets";
        const tString wXl = m_UnzipTempDir + "/xl";
        SkRmdir(wWs.c_str());
        SkRmdir(wXl.c_str());
        SkRmdir(m_UnzipTempDir.c_str());
        m_UnzipTempDir.clear();
    }
}

tBool tExcelPugiXMLReader::LoadWorksheetShell(const tString& sSheetFile, pugi::xml_document& sOut) const {
    const UnzippedFile* wFile = GetUnzippedFile(sSheetFile);
    if (!wFile || !wFile->HasPayload()) {
        return false;
    }
    return LoadWorksheetShellFromFile(*wFile, sOut);
}

tBool tExcelPugiXMLReader::ForEachWorksheetRow(
    const tString& sSheetFile,
    const std::function<void(const pugi::xml_node&)>& sFn
) const {
    const UnzippedFile* wFile = GetUnzippedFile(sSheetFile);
    if (!wFile || !wFile->HasPayload()) {
        return false;
    }
    return ForEachWorksheetRowFromFile(*wFile, sFn);
}

// Unzip Excel file in memory (large worksheet XML is spilled to disk).
std::map<tString, UnzippedFile> tExcelPugiXMLReader::UnzipExcelInMemory(const tString& sFilePath) {
    std::map<tString, UnzippedFile> unzippedFiles;
    
    // Check if file exists and is readable
    struct stat fileStat;
    if (stat(sFilePath.c_str(), &fileStat) != 0) {
        std::cerr << "Error: File does not exist or cannot be accessed: " << sFilePath << std::endl;
        std::cerr << "  errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        return unzippedFiles;
    }
    
    if (S_ISDIR(fileStat.st_mode)) {
        std::cerr << "Error: Path is a directory, not a file: " << sFilePath << std::endl;
        return unzippedFiles;
    }
    
#ifdef debuginfo
    std::cout << "File size: " << fileStat.st_size << " bytes" << std::endl;
#endif
    
    // Try to open the ZIP archive
    tInt error = 0;
    zip_t* archive = zip_open(sFilePath.c_str(), ZIP_RDONLY, &error);
    
    if (!archive) {
        std::cerr << "Error opening ZIP file: " << sFilePath << std::endl;
        std::cerr << "  Error code: " << error << std::endl;
        
        // Get detailed error message from libzip
        zip_error_t zipError;
        zip_error_init(&zipError);
        zip_error_set(&zipError, error, errno);
        std::cerr << "  libzip error: " << zip_error_strerror(&zipError) << std::endl;
        zip_error_fini(&zipError);
        
        // Also show system error if available
        if (errno != 0) {
            std::cerr << "  System error: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
        }
        
        // Common error codes and their meanings
        switch (error) {
            case ZIP_ER_OPEN:
                std::cerr << "  Meaning: Failed to open file (check permissions or file format)" << std::endl;
                break;
            case ZIP_ER_READ:
                std::cerr << "  Meaning: Read error (file may be corrupted)" << std::endl;
                break;
            case ZIP_ER_NOZIP:
                std::cerr << "  Meaning: File is not a valid ZIP archive" << std::endl;
                break;
            case ZIP_ER_INCONS:
                std::cerr << "  Meaning: ZIP archive is inconsistent or corrupted" << std::endl;
                break;
            case ZIP_ER_NOENT:
                std::cerr << "  Meaning: File not found" << std::endl;
                break;
            default:
                std::cerr << "  Meaning: Unknown error (code " << error << ")" << std::endl;
                break;
        }
        
        return unzippedFiles;
    }
    
    zip_int64_t numEntries = zip_get_num_entries(archive, 0);
#ifdef debuginfo
    std::cout << "Found " << numEntries << " files in the Excel archive" << std::endl;
#endif
    for (zip_int64_t i = 0; i < numEntries; i++) {
        zip_stat_t stat;
        if (zip_stat_index(archive, i, 0, &stat) == 0) {
            tString filename = BoundedCStr(stat.name, kMaxZipEntryPathChars);
            if (filename.empty()) {
                continue;
            }
            // Skip directories
            if (filename.back() == '/') {
                continue;
            }
            
            // calcChain is unused on import and can be hundreds of MB.
            if (filename == "xl/calcChain.xml" || filename.rfind("xl/printerSettings/", 0) == 0) {
                continue;
            }

            zip_file_t* file = zip_fopen_index(archive, i, 0);
            if (file) {
                const zip_uint64_t wSize = stat.size;
                UnzippedFile unzippedFile;
                unzippedFile.filename = filename;
                unzippedFile.size = static_cast<tSize>(wSize);

                tBool wExtracted = false;
                tBool wTriedSpillIo = false;
                if (wSize > kUnzipSpillBytes && !m_UnzipTempDir.empty()) {
                    const tString wDiskPath = m_UnzipTempDir + "/" + filename;
                    if (EnsureDirPath(ParentDir(wDiskPath))) {
                        FILE* wOut = std::fopen(wDiskPath.c_str(), "wb");
                        if (wOut) {
                            wTriedSpillIo = true;
                            std::vector<char> wChunk(kUnzipIoChunk);
                            zip_uint64_t wLeft = wSize;
                            tBool wWriteOk = true;
                            while (wLeft > 0) {
                                const zip_uint64_t wWant = std::min<zip_uint64_t>(wLeft, kUnzipIoChunk);
                                const zip_int64_t n = zip_fread(file, wChunk.data(), wWant);
                                if (n <= 0) {
                                    wWriteOk = false;
                                    break;
                                }
                                if (std::fwrite(wChunk.data(), 1, static_cast<size_t>(n), wOut) != static_cast<size_t>(n)) {
                                    wWriteOk = false;
                                    break;
                                }
                                wLeft -= static_cast<zip_uint64_t>(n);
                            }
                            std::fclose(wOut);
                            if (wWriteOk && wLeft == 0) {
                                unzippedFile.diskPath = wDiskPath;
                                wExtracted = true;
                                std::cerr << "SkExcel: spilled " << filename << " (" << wSize
                                          << " bytes) to disk" << std::endl;
                            } else {
                                std::remove(wDiskPath.c_str());
                                std::cerr << "SkExcel: failed to spill " << filename
                                          << " to disk (read leftover " << wLeft << ")" << std::endl;
                            }
                        }
                    }
                }
                if (!wExtracted) {
                    // Spill failed or file is small: keep the part in memory.
                    // Reopen if a failed spill already consumed this zip entry.
                    if (wTriedSpillIo) {
                        zip_fclose(file);
                        file = zip_fopen_index(archive, i, 0);
                        if (!file) {
                            continue;
                        }
                    }
                    unzippedFile.content.resize(static_cast<size_t>(wSize));
                    zip_int64_t totalRead = 0;
                    while (static_cast<zip_uint64_t>(totalRead) < wSize) {
                        const zip_int64_t n = zip_fread(
                            file,
                            unzippedFile.content.data() + totalRead,
                            wSize - static_cast<zip_uint64_t>(totalRead));
                        if (n < 0) {
                            totalRead = -1;
                            break;
                        }
                        if (n == 0) {
                            break;
                        }
                        totalRead += n;
                    }
                    if (totalRead >= 0 && static_cast<zip_uint64_t>(totalRead) == wSize) {
                        wExtracted = true;
                    } else {
                        unzippedFile.content.clear();
                        std::cerr << "SkExcel: incomplete zip extract for " << filename
                                  << " (read " << totalRead << " of " << wSize << " bytes)" << std::endl;
                    }
                }

                if (wExtracted) {
                    unzippedFiles[filename] = std::move(unzippedFile);
#ifdef debuginfo
                    std::cout << "  - Extracted: " << filename << " (" << wSize << " bytes)" << std::endl;
#endif
                }

                zip_fclose(file);
            }
        }
    }
    
    zip_close(archive);
    return unzippedFiles;
}

const UnzippedFile* tExcelPugiXMLReader::GetUnzippedFileAppXml() const {
    const UnzippedFile* a = GetUnzippedFile("docProps/app.xml");
    const UnzippedFile* b = GetUnzippedFile("DocProps/app.xml");
    if (!a) return b;
    if (!b) return a;
    return (a->content.size() >= b->content.size()) ? a : b;
}

// Save unzipped files to a directory, preserving relative paths
tBool tExcelPugiXMLReader::SaveAllXmlToDirectory(const tString& sTargetDirectory) const {
    auto ensureDir = [](const tString& path) {
        // Best-effort recursive creation for nested paths
        tString accum;
        for (tSize i = 0; i < path.size(); ++i) {
            char c = path[i];
            accum.push_back(c);
            if (c == '/' || i == path.size()-1) {
                if (!accum.empty() && accum != "/") {
                    SkMkdir(accum.c_str());
                }
            }
        }
    };

    // Normalize base dir (no trailing slash)
    tString base = sTargetDirectory;
    if (!base.empty() && base.back() == '/') base.pop_back();
    ensureDir(base);

    for (const auto& kv : m_UnzippedFiles) {
        const tString& rel = kv.first; // e.g. xl/workbook.xml
        // Compose output path
        tString outPath = base + "/" + rel;

        // Ensure directory exists for this file
        tSize slashPos = outPath.find_last_of('/');
        if (slashPos != tString::npos) {
            tString dir = outPath.substr(0, slashPos+1);
            ensureDir(dir);
        }

        // Write file
        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) {
            std::cerr << "Failed to write: " << outPath << std::endl;
            return false;
        }
        ofs.write(kv.second.content.data(), (std::streamsize)kv.second.content.size());
    }
    std::cout << "Saved XML files to: " << base << std::endl;
    return true;
}

// Display all sparklines (cell + source range) for all worksheets
void tExcelPugiXMLReader::DisplaySparklines() {
    if (m_UnzippedFiles.empty()) {
        std::cout << "No workbook loaded." << std::endl;
        return;
    }
    std::cout << "\n=== Sparklines (per sheet) ===" << std::endl;
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wIt = m_UnzippedFiles.find(wSheetFile);
        if (wIt == m_UnzippedFiles.end()) break;
        const auto& wWorksheet = wIt->second;
        pugi::xml_document wDoc;
        pugi::xml_parse_result wResult = wDoc.load_buffer(wWorksheet.content.data(), wWorksheet.content.size());
        if (!wResult) continue;
        tString wSheetName = (wS-1)>=0 && (wS-1)<(tInt)m_WorksheetNames.size() ? m_WorksheetNames[wS-1] : ("sheet"+std::to_string(wS));
        std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;
        // x14:sparklineGroups live under extLst/ext[x14]
        pugi::xml_node wExtLst = wDoc.child("worksheet").child("extLst");
        if (!wExtLst) { std::cout << "(no sparklines)" << std::endl; continue; }
        tBool wAny=false;
        for (pugi::xml_node wExt : wExtLst.children()) {
            pugi::xml_node wGroups = wExt.child("x14:sparklineGroups");
            if (!wGroups) continue;
            for (pugi::xml_node wGroup : wGroups.children("x14:sparklineGroup")) {
                // american english: group-level style and options
                auto readColor = [&](pugi::xml_node c) -> tString {
                    if (!c) return tString("");
                    const char* rgb = c.attribute("rgb").as_string("");
                    if (rgb && *rgb) {
                        tString v(rgb);
                        if (v.size()==8 && v.rfind("FF",0)==0) v = v.substr(2);
                        std::transform(v.begin(), v.end(), v.begin(), ::toupper);
                        return tString("#") + v;
                    }
                    const char* themeAttr = c.attribute("theme").as_string("");
                    if (themeAttr && *themeAttr) {
                        tInt idx = std::atoi(themeAttr);
                        if (idx>=0 && idx<(tInt)m_ThemeColors.size()) {
                            tString base = m_ThemeColors[idx];
                            // tint is tDouble in [-1,1]; positive lighten, negative darken
                            tDouble tint = 0.0; tBool hasTint=false;
                            if (c.attribute("tint")) { tint = c.attribute("tint").as_double(0.0); hasTint=true; }
                            std::transform(base.begin(), base.end(), base.begin(), ::toupper);
                            if (hasTint && !base.empty()) {
                                base = ApplyTintToSrgb(base, tint);
                            }
                            return base.empty()?tString(""):tString("#")+base;
                        }
                    }
                    return tString("");
                };

                tString wType = wGroup.attribute("type").as_string(""); // line/column/winLoss
                tBool wDateAxis = wGroup.child("x14:dateAxis");
                tBool wMarkers = wGroup.child("x14:markers");
                tBool wHigh    = wGroup.child("x14:high");
                tBool wLow     = wGroup.child("x14:low");
                tBool wFirst   = wGroup.child("x14:first");
                tBool wLast    = wGroup.child("x14:last");
                tBool wNegative= wGroup.child("x14:negative");
                tDouble wLineWeight = wGroup.attribute("lineWeight").as_double(0.0);
                tString wEmpty = wGroup.attribute("displayEmptyCellsAs").as_string(""); // gaps/zero/span

                tString cSeries   = readColor(wGroup.child("x14:colorSeries"));
                tString cNegative = readColor(wGroup.child("x14:colorNegative"));
                tString cAxis     = readColor(wGroup.child("x14:colorAxis"));
                tString cMarkers  = readColor(wGroup.child("x14:colorMarkers"));
                tString cFirst    = readColor(wGroup.child("x14:colorFirst"));
                tString cLast     = readColor(wGroup.child("x14:colorLast"));
                tString cHigh     = readColor(wGroup.child("x14:colorHigh"));
                tString cLow      = readColor(wGroup.child("x14:colorLow"));

                // Print group header
                std::cout << "group: type=" << (wType.empty()?"-":wType)
                          << ", dateAxis=" << (wDateAxis?"1":"0")
                          << ", markers=" << (wMarkers?"1":"0")
                          << ", high/low=" << (wHigh?"1":"0") << "/" << (wLow?"1":"0")
                          << ", first/last=" << (wFirst?"1":"0") << "/" << (wLast?"1":"0")
                          << ", negative=" << (wNegative?"1":"0");
                if (!wEmpty.empty()) std::cout << ", empty=" << wEmpty;
                if (wLineWeight>0) std::cout << ", lineWeight=" << wLineWeight;
                if (!cSeries.empty())   std::cout << ", colorSeries=" << cSeries;
                if (!cNegative.empty()) std::cout << ", colorNegative=" << cNegative;
                if (!cAxis.empty())     std::cout << ", colorAxis=" << cAxis;
                if (!cMarkers.empty())  std::cout << ", colorMarkers=" << cMarkers;
                if (!cFirst.empty())    std::cout << ", colorFirst=" << cFirst;
                if (!cLast.empty())     std::cout << ", colorLast=" << cLast;
                if (!cHigh.empty())     std::cout << ", colorHigh=" << cHigh;
                if (!cLow.empty())      std::cout << ", colorLow=" << cLow;
                std::cout << std::endl;

                for (pugi::xml_node wSparks = wGroup.child("x14:sparklines"); wSparks; wSparks = pugi::xml_node()) {
                    for (pugi::xml_node wSpark : wSparks.children("x14:sparkline")) {
                        const char* wF = wSpark.child("xm:f").text().as_string("");
                        const char* wSqref = wSpark.child("xm:sqref").text().as_string("");
                        std::cout << "cell=" << (wSqref[0]?wSqref:"-") << ", source=" << (wF[0]?wF:"-") << std::endl;
                        wAny=true;
                    }
                }
            }
        }
        if (!wAny) std::cout << "(no sparklines)" << std::endl;
    }
}

std::vector<tSparklineEntry> tExcelPugiXMLReader::CollectSparklines() const {
    std::vector<tSparklineEntry> wOut;
    if (m_UnzippedFiles.empty()) {
        return wOut;
    }
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wIt = m_UnzippedFiles.find(wSheetFile);
        if (wIt == m_UnzippedFiles.end()) {
            break;
        }
        const auto& wWorksheet = wIt->second;
        pugi::xml_document wDoc;
        pugi::xml_parse_result wResult =
            wDoc.load_buffer(wWorksheet.content.data(), wWorksheet.content.size());
        if (!wResult) {
            continue;
        }
        tString wSheetName =
            (wS - 1) >= 0 && (wS - 1) < (tInt)m_WorksheetNames.size()
                ? m_WorksheetNames[wS - 1]
                : ("sheet" + std::to_string(wS));
        pugi::xml_node wExtLst = wDoc.child("worksheet").child("extLst");
        if (!wExtLst) {
            continue;
        }
        for (pugi::xml_node wExt : wExtLst.children()) {
            pugi::xml_node wGroups = wExt.child("x14:sparklineGroups");
            if (!wGroups) {
                continue;
            }
            for (pugi::xml_node wGroup : wGroups.children("x14:sparklineGroup")) {
                const tBool wMarkers = wGroup.attribute("markers") ? true : false;
                for (pugi::xml_node wSparks = wGroup.child("x14:sparklines"); wSparks;
                     wSparks = pugi::xml_node()) {
                    for (pugi::xml_node wSpark : wSparks.children("x14:sparkline")) {
                        const char* wF = wSpark.child("xm:f").text().as_string("");
                        const char* wSqref = wSpark.child("xm:sqref").text().as_string("");
                        if (!wF || !*wF || !wSqref || !*wSqref) {
                            continue;
                        }
                        tSparklineEntry wEntry;
                        wEntry.SheetName = wSheetName;
                        wEntry.CellRef = wSqref;
                        wEntry.SourceRange = wF;
                        wEntry.Markers = wMarkers;
                        wOut.push_back(wEntry);
                    }
                }
            }
        }
    }
    return wOut;
}

// Display all Excel tables (name, range, columns) across all worksheets
void tExcelPugiXMLReader::DisplayTables() {
    if (m_UnzippedFiles.empty()) { 
        std::cout << "No workbook loaded." << std::endl; 
        return; 
    }
    
    std::cout << "\n=== Excel Tables ===" << std::endl;
    
    tBool wAnyTable = false;
    
    // Iterate through all worksheets
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = m_UnzippedFiles.find(wSheetFile);
        if (wItSheet == m_UnzippedFiles.end()) break;
        
        tString wSheetName = (wS-1) >= 0 && (wS-1) < (tInt)m_WorksheetNames.size() 
            ? m_WorksheetNames[wS-1] 
            : ("sheet" + std::to_string(wS));
        
        // Find table relationships from sheet rels
        tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = m_UnzippedFiles.find(wRelsPath);
        if (wItRels == m_UnzippedFiles.end()) continue;
        
        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->second.content.data(), wItRels->second.content.size())) continue;
        
        // Helper to resolve relative paths
        auto resolveTarget = [](const tString& basePath, const tString& target) -> tString {
            if (target.rfind("../", 0) == 0) {
                // Remove "../" prefix and prepend "xl/"
                return tString("xl/") + target.substr(3);
            }
            // If already absolute from xl/, use as is
            if (target.rfind("xl/", 0) == 0) {
                return target;
            }
            // Otherwise, assume relative to basePath
            return basePath + target;
        };
        
        // Find all table relationships
        std::vector<tString> wTableTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/table") != tString::npos) {
                tString wTarget = wRel.attribute("Target").as_string("");
                wTarget = resolveTarget("xl/worksheets/_rels/", wTarget);
                // Normalize to tables/tableN.xml
                if (wTarget.find("xl/tables/") == tString::npos) {
                    tSize p = wTarget.find("tables/");
                    if (p != tString::npos) {
                        wTarget = tString("xl/") + wTarget.substr(p);
                    }
                }
                wTableTargets.push_back(wTarget);
            }
        }
        
        // Parse each table XML file
        for (const tString& wTablePath : wTableTargets) {
            auto wItTable = m_UnzippedFiles.find(wTablePath);
            if (wItTable == m_UnzippedFiles.end()) continue;
            
            pugi::xml_document wTableDoc;
            if (!wTableDoc.load_buffer(wItTable->second.content.data(), wItTable->second.content.size())) continue;
            
            pugi::xml_node wTableNode = wTableDoc.child("table");
            if (!wTableNode) continue;
            
            wAnyTable = true;
            
            // Extract table attributes
            const char* wName = wTableNode.attribute("name").as_string("");
            const char* wDisplayName = wTableNode.attribute("displayName").as_string("");
            const char* wRef = wTableNode.attribute("ref").as_string("");
            const char* wId = wTableNode.attribute("id").as_string("");
            const char* wTotalsRowShown = wTableNode.attribute("totalsRowShown").as_string("0");
            
            std::cout << "\n-- Sheet " << wS << " (" << wSheetName << ") --" << std::endl;
            std::cout << "Table ID: " << (wId[0] ? wId : "-") << std::endl;
            std::cout << "Name: " << (wName[0] ? wName : "-") << std::endl;
            std::cout << "Display Name: " << (wDisplayName[0] ? wDisplayName : "-") << std::endl;
            std::cout << "Range: " << (wRef[0] ? wRef : "-") << std::endl;
            std::cout << "Totals Row Shown: " << (wTotalsRowShown[0] ? wTotalsRowShown : "0") << std::endl;
            
            // Extract autoFilter if present
            pugi::xml_node wAutoFilter = wTableNode.child("autoFilter");
            if (wAutoFilter) {
                const char* wFilterRef = wAutoFilter.attribute("ref").as_string("");
                if (wFilterRef[0]) {
                    std::cout << "AutoFilter Range: " << wFilterRef << std::endl;
                }
            }
            
            // Extract table columns
            pugi::xml_node wTableColumns = wTableNode.child("tableColumns");
            if (wTableColumns) {
                tInt wColCount = wTableColumns.attribute("count").as_int(0);
                std::cout << "Columns (" << wColCount << "):" << std::endl;
                
                tInt wColIndex = 1;
                for (pugi::xml_node wCol : wTableColumns.children("tableColumn")) {
                    const char* wColId = wCol.attribute("id").as_string("");
                    const char* wColName = wCol.attribute("name").as_string("");
                    std::cout << "  [" << wColIndex << "] ID=" << (wColId[0] ? wColId : "-")
                              << ", Name=\"" << (wColName[0] ? wColName : "-") << "\"" << std::endl;
                    wColIndex++;
                }
            }
            
            // Extract table style info
            pugi::xml_node wStyleInfo = wTableNode.child("tableStyleInfo");
            if (wStyleInfo) {
                const char* wStyleName = wStyleInfo.attribute("name").as_string("");
                const char* wShowFirstCol = wStyleInfo.attribute("showFirstColumn").as_string("0");
                const char* wShowLastCol = wStyleInfo.attribute("showLastColumn").as_string("0");
                const char* wShowRowStripes = wStyleInfo.attribute("showRowStripes").as_string("0");
                const char* wShowColStripes = wStyleInfo.attribute("showColumnStripes").as_string("0");
                
                std::cout << "Style: " << (wStyleName[0] ? wStyleName : "-") << std::endl;
                std::cout << "  Show First Column: " << wShowFirstCol << std::endl;
                std::cout << "  Show Last Column: " << wShowLastCol << std::endl;
                std::cout << "  Show Row Stripes: " << wShowRowStripes << std::endl;
                std::cout << "  Show Column Stripes: " << wShowColStripes << std::endl;
            }
        }
    }
    
    if (!wAnyTable) {
        std::cout << "(no tables found)" << std::endl;
    }
}

// Display all images (name, size, position) across all worksheets
std::vector<tExcelImageEntry> tExcelPugiXMLReader::CollectImages() const {
    std::vector<tExcelImageEntry> wOut;
    if (m_UnzippedFiles.empty()) {
        return wOut;
    }
    
    // Iterate through all worksheets
    for (tInt wS = 1;; ++wS) {
        tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = m_UnzippedFiles.find(wSheetFile);
        if (wItSheet == m_UnzippedFiles.end()) break;
        
        tString wSheetName = (wS-1) >= 0 && (wS-1) < (tInt)m_WorksheetNames.size() 
            ? m_WorksheetNames[wS-1] 
            : ("sheet" + std::to_string(wS));
        
        // Find drawing relationships from sheet rels
        tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = m_UnzippedFiles.find(wRelsPath);
        if (wItRels == m_UnzippedFiles.end()) continue;
        
        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->second.content.data(), wItRels->second.content.size())) continue;
        
        // Helper to resolve relative paths
        auto resolveTarget = [](const tString& basePath, const tString& target) -> tString {
            if (target.rfind("../", 0) == 0) {
                // Remove "../" prefix and prepend "xl/"
                return tString("xl/") + target.substr(3);
            }
            // If already absolute from xl/, use as is
            if (target.rfind("xl/", 0) == 0) {
                return target;
            }
            // Otherwise, assume relative to basePath
            return basePath + target;
        };
        
        // Find all drawing relationships
        std::vector<tString> wDrawingTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/drawing") != tString::npos) {
                tString wTarget = wRel.attribute("Target").as_string("");
                wTarget = resolveTarget("xl/worksheets/_rels/", wTarget);
                // Normalize to drawings/drawingN.xml
                if (wTarget.find("xl/drawings/") == tString::npos) {
                    tSize p = wTarget.find("drawings/");
                    if (p != tString::npos) {
                        wTarget = tString("xl/") + wTarget.substr(p);
                    }
                }
                wDrawingTargets.push_back(wTarget);
            }
        }
        
        // For each drawing, find its relationships to images
        for (const tString& wDrawPath : wDrawingTargets) {
            // Find drawing relationships file
            tString wDrawRelsPath = wDrawPath + ".rels";
            // drawings rels live under xl/drawings/_rels/drawingN.xml.rels
            if (wDrawRelsPath.find("xl/drawings/") != tString::npos) {
                wDrawRelsPath.insert(wDrawRelsPath.find("xl/drawings/") + tString("xl/drawings/").size(), "_rels/");
            }
            
            auto wItDrawRels = m_UnzippedFiles.find(wDrawRelsPath);
            if (wItDrawRels == m_UnzippedFiles.end()) continue;
            
            pugi::xml_document wDrawRelDoc;
            if (!wDrawRelDoc.load_buffer(wItDrawRels->second.content.data(), wItDrawRels->second.content.size())) continue;
            
            // Find image relationships
            for (pugi::xml_node wRel : wDrawRelDoc.child("Relationships").children("Relationship")) {
                tString wType = wRel.attribute("Type").as_string("");
                if (wType.find("/image") != tString::npos) {
                    tString wTarget = wRel.attribute("Target").as_string("");
                    tString wRelId = wRel.attribute("Id").as_string("");
                    
                    // Resolve image path
                    if (wTarget.rfind("../", 0) == 0) {
                        wTarget = tString("xl/") + wTarget.substr(3);
                    } else if (wTarget.rfind("xl/", 0) != 0) {
                        wTarget = tString("xl/drawings/") + wTarget;
                    }
                    
                    // Normalize to xl/media/imageN.ext
                    if (wTarget.find("xl/media/") == tString::npos) {
                        tSize p = wTarget.find("media/");
                        if (p != tString::npos) {
                            wTarget = tString("xl/") + wTarget.substr(p);
                        }
                    }
                    
                    // Find the image file
                    auto wItImage = m_UnzippedFiles.find(wTarget);
                    if (wItImage == m_UnzippedFiles.end()) continue;
                    
                    // Extract image info
                    const UnzippedFile& wImageFile = wItImage->second;
                    tString wImageName = wImageFile.filename;
                    tSize wSlashPos = wImageName.find_last_of('/');
                    if (wSlashPos != tString::npos) {
                        wImageName = wImageName.substr(wSlashPos + 1);
                    }
                    
                    // Determine image type from extension
                    tString wImageType = "unknown";
                    tSize wDotPos = wImageName.find_last_of('.');
                    if (wDotPos != tString::npos) {
                        tString wExt = wImageName.substr(wDotPos + 1);
                        if (wExt == "png" || wExt == "PNG") wImageType = "PNG";
                        else if (wExt == "jpg" || wExt == "JPG" || wExt == "jpeg" || wExt == "JPEG") wImageType = "JPEG";
                        else if (wExt == "gif" || wExt == "GIF") wImageType = "GIF";
                        else if (wExt == "bmp" || wExt == "BMP") wImageType = "BMP";
                        else if (wExt == "emf" || wExt == "EMF") wImageType = "EMF";
                        else if (wExt == "wmf" || wExt == "WMF") wImageType = "WMF";
                        else wImageType = wExt;
                    }
                    
                    // Try to find image position and dimensions in drawing XML
                    tString wPosition = "unknown";
                    tString wAnchorType = "unknown";
                    tString wPositionDetails = "";
                    tInt wWidthPx = 0, wHeightPx = 0;
                    tInt wAnchorRow = 0;
                    tInt wAnchorCol = 0;
                    tDouble wDiffX = 0.0;
                    tDouble wDiffY = 0.0;
                    
                    auto wItDrawXml = m_UnzippedFiles.find(wDrawPath);
                    if (wItDrawXml != m_UnzippedFiles.end()) {
                        pugi::xml_document wDrawDoc;
                        if (wDrawDoc.load_buffer(wItDrawXml->second.content.data(), wItDrawXml->second.content.size())) {
                            // Helper function to convert column number to letters
                            auto wColLetters = [](tInt wC) {
                                tString w; tInt c = wC;
                                do { 
                                    tInt r = c % 26; 
                                    w.insert(w.begin(), char('A' + r)); 
                                    c = c / 26 - 1; 
                                } while (c >= 0);
                                return w;
                            };
                            
                            // Helper to convert EMUs to pixels
                            auto wEmuToPx = [](long long wEmu, tDouble wDpi = 96.0) -> tInt {
                                const tDouble wEmusPerInch = 914400.0;
                                return (tInt)std::round((tDouble)wEmu / wEmusPerInch * wDpi);
                            };

                            auto wApplyFromNode = [&](const pugi::xml_node& wFrom) {
                                if (!wFrom) {
                                    return;
                                }
                                tInt wColFrom = wFrom.child("xdr:col").text().as_int(-1);
                                if (wColFrom < 0) {
                                    wColFrom = wFrom.child("col").text().as_int(-1);
                                }
                                tInt wRowFrom = wFrom.child("xdr:row").text().as_int(-1);
                                if (wRowFrom < 0) {
                                    wRowFrom = wFrom.child("row").text().as_int(-1);
                                }
                                if (wColFrom < 0 || wRowFrom < 0) {
                                    return;
                                }
                                wPosition = wColLetters(wColFrom) + std::to_string(wRowFrom + 1);
                                wAnchorRow = wRowFrom + 1;
                                wAnchorCol = wColFrom + 1;
                                long long wColOff = wFrom.child("xdr:colOff").text().as_llong(0);
                                if (wColOff == 0) {
                                    wColOff = wFrom.child("colOff").text().as_llong(0);
                                }
                                long long wRowOff = wFrom.child("xdr:rowOff").text().as_llong(0);
                                if (wRowOff == 0) {
                                    wRowOff = wFrom.child("rowOff").text().as_llong(0);
                                }
                                wDiffX = (tDouble)wEmuToPx(wColOff);
                                wDiffY = (tDouble)wEmuToPx(wRowOff);
                            };
                            
                            // Recursive function to search for picture elements that reference this image
                            std::function<void(const pugi::xml_node&, tInt, tString&, tString&, tString&, tInt&, tInt&)> wSearchForPicture =
                                [&](const pugi::xml_node& wNode, tInt wDepth, tString& wPos, tString& wAncType, tString& wDetails, tInt& wW, tInt& wH) {
                                if (wDepth > 10) return; // Limit recursion
                                
                                tString wNodeName = wNode.name();
                                if (wNodeName == "xdr:pic" || wNodeName == "pic") {
                                    // Check if this picture references our image
                                    pugi::xml_node wBlip = wNode.child("xdr:blipFill").child("a:blip");
                                    if (!wBlip) wBlip = wNode.child("blipFill").child("blip");
                                    if (wBlip) {
                                        const char* wEmbed = wBlip.attribute("r:embed").as_string("");
                                        if (wEmbed == wRelId || wBlip.attribute("embed").as_string("") == wRelId) {
                                            // Found the picture, try to get position and dimensions
                                            pugi::xml_node wAnchor = wNode;
                                            while (wAnchor && wAnchor.name() != tString("xdr:twoCellAnchor") && 
                                                   wAnchor.name() != tString("twoCellAnchor") &&
                                                   wAnchor.name() != tString("xdr:oneCellAnchor") &&
                                                   wAnchor.name() != tString("oneCellAnchor") &&
                                                   wAnchor.name() != tString("xdr:absoluteAnchor") &&
                                                   wAnchor.name() != tString("absoluteAnchor")) {
                                                wAnchor = wAnchor.parent();
                                            }
                                            if (wAnchor) {
                                                tString wAncName = wAnchor.name();
                                                wAncType = wAncName;
                                                
                                                // Extract dimensions from xfrm/ext
                                                pugi::xml_node wSpPr = wNode.child("xdr:spPr");
                                                if (!wSpPr) wSpPr = wNode.child("spPr");
                                                if (wSpPr) {
                                                    pugi::xml_node wXfrm = wSpPr.child("a:xfrm");
                                                    if (!wXfrm) wXfrm = wSpPr.child("xfrm");
                                                    if (wXfrm) {
                                                        pugi::xml_node wExt = wXfrm.child("a:ext");
                                                        if (!wExt) wExt = wXfrm.child("ext");
                                                        if (wExt) {
                                                            long long wCx = wExt.attribute("cx").as_llong(0);
                                                            long long wCy = wExt.attribute("cy").as_llong(0);
                                                            wW = wEmuToPx(wCx);
                                                            wH = wEmuToPx(wCy);
                                                        }
                                                    }
                                                }
                                                
                                                // Extract position based on anchor type
                                                if (wAncName == "xdr:twoCellAnchor" || wAncName == "twoCellAnchor") {
                                                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                                                    if (!wFrom) wFrom = wAnchor.child("from");
                                                    pugi::xml_node wTo = wAnchor.child("xdr:to");
                                                    if (!wTo) wTo = wAnchor.child("to");
                                                    
                                                    if (wFrom) {
                                                        wApplyFromNode(wFrom);
                                                        wPos = wPosition;
                                                        if (wTo) {
                                                            tInt wColTo = wTo.child("xdr:col").text().as_int(-1);
                                                            if (wColTo < 0) wColTo = wTo.child("col").text().as_int(-1);
                                                            tInt wRowTo = wTo.child("xdr:row").text().as_int(-1);
                                                            if (wRowTo < 0) wRowTo = wTo.child("row").text().as_int(-1);
                                                            
                                                            if (wColTo >= 0 && wRowTo >= 0 && !wPos.empty() && wPos != "unknown") {
                                                                wDetails = "From: " + wPos + " To: " + wColLetters(wColTo) + std::to_string(wRowTo + 1);
                                                            }
                                                        }
                                                    }
                                                } else if (wAncName == "xdr:oneCellAnchor" || wAncName == "oneCellAnchor") {
                                                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                                                    if (!wFrom) wFrom = wAnchor.child("from");
                                                    if (wFrom) {
                                                        wApplyFromNode(wFrom);
                                                        wPos = wPosition;
                                                        if (!wPos.empty() && wPos != "unknown") {
                                                            wDetails = "Anchor: " + wPos;
                                                        }
                                                    }
                                                } else if (wAncName == "xdr:absoluteAnchor" || wAncName == "absoluteAnchor") {
                                                    pugi::xml_node wPosNode = wAnchor.child("xdr:pos");
                                                    if (!wPosNode) wPosNode = wAnchor.child("pos");
                                                    if (wPosNode) {
                                                        long long wX = wPosNode.attribute("x").as_llong(0);
                                                        long long wY = wPosNode.attribute("y").as_llong(0);
                                                        wAnchorRow = 1;
                                                        wAnchorCol = 1;
                                                        wPosition = "A1";
                                                        wPos = wPosition;
                                                        wDiffX = (tDouble)wEmuToPx(wX);
                                                        wDiffY = (tDouble)wEmuToPx(wY);
                                                        wDetails = "Absolute position: x=" + std::to_string((tInt)wDiffX) + "px, y=" + std::to_string((tInt)wDiffY) + "px";
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                for (pugi::xml_node wChild : wNode.children()) {
                                    wSearchForPicture(wChild, wDepth + 1, wPos, wAncType, wDetails, wW, wH);
                                }
                            };
                            
                            pugi::xml_node wRoot = wDrawDoc.root();
                            wSearchForPicture(wRoot, 0, wPosition, wAnchorType, wPositionDetails, wWidthPx, wHeightPx);
                        }
                    }
                    
                    tExcelImageEntry wEntry;
                    wEntry.SheetName = wSheetName;
                    wEntry.Name = wImageName;
                    wEntry.MediaPath = wTarget;
                    wEntry.ImageType = wImageType;
                    wEntry.MimeType = SkRoot::SkBase64::MimeTypeFromImagePath(wTarget);
                    wEntry.Position = wPosition;
                    wEntry.AnchorType = wAnchorType;
                    wEntry.PositionDetails = wPositionDetails;
                    wEntry.WidthPx = wWidthPx;
                    wEntry.HeightPx = wHeightPx;
                    wEntry.AnchorRow = wAnchorRow;
                    wEntry.AnchorCol = wAnchorCol;
                    wEntry.DiffX = wDiffX;
                    wEntry.DiffY = wDiffY;
                    wEntry.FileSize = wImageFile.size;
                    wEntry.Base64 = SkRoot::SkBase64::Encode(
                        wImageFile.content.data(), wImageFile.content.size());
                    wEntry.DataUrl = SkRoot::SkBase64::DataUrl(
                        wEntry.MimeType,
                        wImageFile.content.data(),
                        wImageFile.content.size());
                    wOut.push_back(std::move(wEntry));
                }
            }
        }
    }
    
    return wOut;
}

namespace {

tString DrawingSchemeNameToThemeKey(const tString& sName) {
    if (sName == "bg1") return "lt1";
    if (sName == "tx1") return "dk1";
    if (sName == "bg2") return "lt2";
    if (sName == "tx2") return "dk2";
    return sName;
}

tString ApplyDrawingLumModOff(const tString& sRgb, tInt sLumMod, tInt sLumOff) {
    if (sRgb.size() != 6) {
        return sRgb;
    }
    auto wHex = [](char c) -> tInt {
        if ('0' <= c && c <= '9') return c - '0';
        if ('A' <= c && c <= 'F') return c - 'A' + 10;
        if ('a' <= c && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    auto wClamp255 = [](tDouble x) -> tInt {
        if (x < 0.0) x = 0.0;
        if (x > 255.0) x = 255.0;
        return static_cast<tInt>(std::round(x));
    };
    const tDouble wMod = static_cast<tDouble>(std::max(0, std::min(100000, sLumMod))) / 100000.0;
    const tDouble wOff = static_cast<tDouble>(std::max(0, std::min(100000, sLumOff))) / 100000.0 * 255.0;
    tInt wR = (wHex(sRgb[0]) << 4) + wHex(sRgb[1]);
    tInt wG = (wHex(sRgb[2]) << 4) + wHex(sRgb[3]);
    tInt wB = (wHex(sRgb[4]) << 4) + wHex(sRgb[5]);
    wR = wClamp255(wOff + wMod * wR);
    wG = wClamp255(wOff + wMod * wG);
    wB = wClamp255(wOff + wMod * wB);
    char wBuf[8];
    std::snprintf(wBuf, sizeof(wBuf), "%02X%02X%02X", wR, wG, wB);
    return tString(wBuf);
}

tInt DrawingSchemeNameToThemeIndex(const tString& sName) {
    const tString wKey = DrawingSchemeNameToThemeKey(sName);
    if (wKey == "lt1") return 0;
    if (wKey == "dk1") return 1;
    if (wKey == "lt2") return 2;
    if (wKey == "dk2") return 3;
    if (wKey == "accent1") return 4;
    if (wKey == "accent2") return 5;
    if (wKey == "accent3") return 6;
    if (wKey == "accent4") return 7;
    if (wKey == "accent5") return 8;
    if (wKey == "accent6") return 9;
    if (wKey == "hlink") return 10;
    if (wKey == "folHlink") return 11;
    return -1;
}

tString ResolveDrawingSolidFill(const pugi::xml_node& sFillNode, const std::vector<tString>& sThemeColors) {
    if (!sFillNode) {
        return {};
    }
    if (auto wSrgb = sFillNode.child("a:srgbClr")) {
        tString wVal = wSrgb.attribute("val").as_string("");
        if (wVal.empty()) {
            return {};
        }
        tInt wLumMod = -1;
        tInt wLumOff = -1;
        if (auto wN = wSrgb.child("a:lumMod")) wLumMod = wN.attribute("val").as_int(-1);
        if (auto wN = wSrgb.child("a:lumOff")) wLumOff = wN.attribute("val").as_int(-1);
        if (wLumMod >= 0 || wLumOff >= 0) {
            if (wLumMod < 0) wLumMod = 100000;
            if (wLumOff < 0) wLumOff = 0;
            wVal = ApplyDrawingLumModOff(wVal, wLumMod, wLumOff);
        }
        return tString("#") + wVal;
    }
    if (auto wScheme = sFillNode.child("a:schemeClr")) {
        const tString wName = wScheme.attribute("val").as_string("");
        tInt wIdx = DrawingSchemeNameToThemeIndex(wName);
        if (wIdx >= 0 && wIdx < static_cast<tInt>(sThemeColors.size())) {
            tString wBase = sThemeColors[static_cast<tSize>(wIdx)];
            tInt wLumMod = -1;
            tInt wLumOff = -1;
            if (auto wN = wScheme.child("a:lumMod")) wLumMod = wN.attribute("val").as_int(-1);
            if (auto wN = wScheme.child("a:lumOff")) wLumOff = wN.attribute("val").as_int(-1);
            if (wLumMod >= 0 || wLumOff >= 0) {
                if (wLumMod < 0) wLumMod = 100000;
                if (wLumOff < 0) wLumOff = 0;
                wBase = ApplyDrawingLumModOff(wBase, wLumMod, wLumOff);
            }
            return tString("#") + wBase;
        }
    }
    if (auto wSys = sFillNode.child("a:sysClr")) {
        const char* wLast = wSys.attribute("lastClr").as_string("");
        if (wLast && *wLast) {
            return tString("#") + wLast;
        }
    }
    return {};
}

tString DrawingParagraphAlign(const pugi::xml_node& sParagraph) {
    pugi::xml_node wPPr = sParagraph.child("a:pPr");
    if (!wPPr) {
        wPPr = sParagraph.child("pPr");
    }
    const tString wAlgn = wPPr ? wPPr.attribute("algn").as_string("") : tString();
    if (wAlgn == "ctr") return "center";
    if (wAlgn == "r") return "right";
    if (wAlgn == "just") return "justify";
    return "left";
}

tString ExtractDrawingText(const pugi::xml_node& sTxBody) {
    tString wOut;
    for (pugi::xml_node wP = sTxBody.child("a:p"); wP; wP = wP.next_sibling("a:p")) {
        tString wLine;
        for (pugi::xml_node wR = wP.child("a:r"); wR; wR = wR.next_sibling("a:r")) {
            pugi::xml_node wT = wR.child("a:t");
            if (!wT) {
                wT = wR.child("t");
            }
            if (wT) {
                wLine += wT.text().as_string("");
            }
        }
        if (wLine.empty()) {
            pugi::xml_node wT = wP.child("a:t");
            if (!wT) {
                wT = wP.child("t");
            }
            if (wT) {
                wLine = wT.text().as_string("");
            }
        }
        if (!wOut.empty()) {
            wOut.push_back('\n');
        }
        wOut += wLine;
    }
    return wOut;
}

void ApplyFirstRunStyle(const pugi::xml_node& sTxBody, tExcelTextBoxEntry& sEntry) {
    for (pugi::xml_node wP = sTxBody.child("a:p"); wP; wP = wP.next_sibling("a:p")) {
        if (sEntry.TextAlign.empty()) {
            sEntry.TextAlign = DrawingParagraphAlign(wP);
        }
        pugi::xml_node wR = wP.child("a:r");
        if (!wR) {
            wR = wP.child("r");
        }
        if (!wR) {
            continue;
        }
        pugi::xml_node wRPr = wR.child("a:rPr");
        if (!wRPr) {
            wRPr = wR.child("rPr");
        }
        if (!wRPr) {
            continue;
        }
        const tInt wSz = wRPr.attribute("sz").as_int(0);
        if (wSz > 0) {
            sEntry.FontSize = static_cast<tDouble>(wSz) / 100.0;
        }
        if (wRPr.attribute("b").as_bool(false)) {
            sEntry.Bold = true;
        }
        pugi::xml_node wLatin = wRPr.child("a:latin");
        if (!wLatin) {
            wLatin = wRPr.child("latin");
        }
        if (wLatin) {
            const char* wFace = wLatin.attribute("typeface").as_string("");
            if (wFace && *wFace) {
                sEntry.FontFamily = wFace;
            }
        }
        pugi::xml_node wFill = wRPr.child("a:solidFill");
        if (!wFill) {
            wFill = wRPr.child("solidFill");
        }
        if (wFill) {
            // color resolved later with theme colors
        }
        break;
    }
}

tString ResolveFirstRunColor(const pugi::xml_node& sTxBody, const std::vector<tString>& sThemeColors) {
    for (pugi::xml_node wP = sTxBody.child("a:p"); wP; wP = wP.next_sibling("a:p")) {
        pugi::xml_node wR = wP.child("a:r");
        if (!wR) {
            wR = wP.child("r");
        }
        if (!wR) {
            continue;
        }
        pugi::xml_node wRPr = wR.child("a:rPr");
        if (!wRPr) {
            wRPr = wR.child("rPr");
        }
        if (!wRPr) {
            continue;
        }
        pugi::xml_node wFill = wRPr.child("a:solidFill");
        if (!wFill) {
            wFill = wRPr.child("solidFill");
        }
        return ResolveDrawingSolidFill(wFill, sThemeColors);
    }
    return {};
}

pugi::xml_node FindShapeChild(const pugi::xml_node& sAnchor) {
    pugi::xml_node wSp = sAnchor.child("xdr:sp");
    if (!wSp) {
        wSp = sAnchor.child("sp");
    }
    return wSp;
}

pugi::xml_node FindTxBody(const pugi::xml_node& sShape) {
    pugi::xml_node wTx = sShape.child("xdr:txBody");
    if (!wTx) {
        wTx = sShape.child("txBody");
    }
    return wTx;
}

tBool IsAnchorNodeName(const tString& sName) {
    return sName == "xdr:twoCellAnchor" || sName == "twoCellAnchor"
        || sName == "xdr:oneCellAnchor" || sName == "oneCellAnchor"
        || sName == "xdr:absoluteAnchor" || sName == "absoluteAnchor";
}

} // namespace

std::vector<tExcelTextBoxEntry> tExcelPugiXMLReader::CollectTextBoxes() const {
    std::vector<tExcelTextBoxEntry> wOut;
    if (m_UnzippedFiles.empty()) {
        return wOut;
    }

    auto wColLetters = [](tInt wC) {
        tString w;
        tInt c = wC;
        do {
            tInt r = c % 26;
            w.insert(w.begin(), static_cast<char>('A' + r));
            c = c / 26 - 1;
        } while (c >= 0);
        return w;
    };

    auto wEmuToPx = [](long long wEmu, tDouble wDpi = 96.0) -> tInt {
        const tDouble wEmusPerInch = 914400.0;
        return static_cast<tInt>(std::round(static_cast<tDouble>(wEmu) / wEmusPerInch * wDpi));
    };

    auto wResolveTarget = [](const tString& target) -> tString {
        if (target.rfind("../", 0) == 0) {
            return tString("xl/") + target.substr(3);
        }
        if (target.rfind("xl/", 0) == 0) {
            return target;
        }
        return target;
    };

    for (tInt wS = 1;; ++wS) {
        const tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = m_UnzippedFiles.find(wSheetFile);
        if (wItSheet == m_UnzippedFiles.end()) {
            break;
        }

        const tString wSheetName = (wS - 1) >= 0 && (wS - 1) < static_cast<tInt>(m_WorksheetNames.size())
            ? m_WorksheetNames[static_cast<tSize>(wS - 1)]
            : ("sheet" + std::to_string(wS));

        const tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = m_UnzippedFiles.find(wRelsPath);
        if (wItRels == m_UnzippedFiles.end()) {
            continue;
        }

        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->second.content.data(), wItRels->second.content.size())) {
            continue;
        }

        std::vector<tString> wDrawingTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            const tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/drawing") == tString::npos) {
                continue;
            }
            tString wTarget = wResolveTarget(wRel.attribute("Target").as_string(""));
            if (wTarget.find("xl/drawings/") == tString::npos) {
                const tSize p = wTarget.find("drawings/");
                if (p != tString::npos) {
                    wTarget = tString("xl/") + wTarget.substr(p);
                }
            }
            wDrawingTargets.push_back(wTarget);
        }

        for (const tString& wDrawPath : wDrawingTargets) {
            auto wItDrawXml = m_UnzippedFiles.find(wDrawPath);
            if (wItDrawXml == m_UnzippedFiles.end()) {
                continue;
            }

            pugi::xml_document wDrawDoc;
            if (!wDrawDoc.load_buffer(wItDrawXml->second.content.data(), wItDrawXml->second.content.size())) {
                continue;
            }

            pugi::xml_node wDrawRoot = wDrawDoc.child("xdr:wsDr");
            if (!wDrawRoot) {
                wDrawRoot = wDrawDoc.child("wsDr");
            }
            if (!wDrawRoot) {
                wDrawRoot = wDrawDoc.document_element();
            }

            for (pugi::xml_node wAnchor : wDrawRoot.children()) {
                const tString wAnchorName = wAnchor.name();
                if (!IsAnchorNodeName(wAnchorName)) {
                    continue;
                }

                pugi::xml_node wSp = FindShapeChild(wAnchor);
                if (!wSp) {
                    continue;
                }

                pugi::xml_node wTxBody = FindTxBody(wSp);
                if (!wTxBody) {
                    continue;
                }

                const tString wText = ExtractDrawingText(wTxBody);
                if (wText.empty()) {
                    continue;
                }

                tExcelTextBoxEntry wEntry;
                wEntry.SheetName = wSheetName;
                wEntry.Text = wText;
                wEntry.AnchorType = wAnchorName;

                pugi::xml_node wNvSpPr = wSp.child("xdr:nvSpPr");
                if (!wNvSpPr) {
                    wNvSpPr = wSp.child("nvSpPr");
                }
                if (wNvSpPr) {
                    pugi::xml_node wCNvPr = wNvSpPr.child("xdr:cNvPr");
                    if (!wCNvPr) {
                        wCNvPr = wNvSpPr.child("cNvPr");
                    }
                    if (wCNvPr) {
                        wEntry.Name = wCNvPr.attribute("name").as_string("");
                    }
                }

                tString wPosition = "unknown";
                tInt wWidthPx = 0;
                tInt wHeightPx = 0;
                tInt wAnchorRow = 0;
                tInt wAnchorCol = 0;
                tDouble wDiffX = 0.0;
                tDouble wDiffY = 0.0;

                auto wApplyFromNode = [&](const pugi::xml_node& wFrom) {
                    if (!wFrom) {
                        return;
                    }
                    tInt wColFrom = wFrom.child("xdr:col").text().as_int(-1);
                    if (wColFrom < 0) {
                        wColFrom = wFrom.child("col").text().as_int(-1);
                    }
                    tInt wRowFrom = wFrom.child("xdr:row").text().as_int(-1);
                    if (wRowFrom < 0) {
                        wRowFrom = wFrom.child("row").text().as_int(-1);
                    }
                    if (wColFrom < 0 || wRowFrom < 0) {
                        return;
                    }
                    wPosition = wColLetters(wColFrom) + std::to_string(wRowFrom + 1);
                    wAnchorRow = wRowFrom + 1;
                    wAnchorCol = wColFrom + 1;
                    long long wColOff = wFrom.child("xdr:colOff").text().as_llong(0);
                    if (wColOff == 0) {
                        wColOff = wFrom.child("colOff").text().as_llong(0);
                    }
                    long long wRowOff = wFrom.child("xdr:rowOff").text().as_llong(0);
                    if (wRowOff == 0) {
                        wRowOff = wFrom.child("rowOff").text().as_llong(0);
                    }
                    wDiffX = static_cast<tDouble>(wEmuToPx(wColOff));
                    wDiffY = static_cast<tDouble>(wEmuToPx(wRowOff));
                };

                pugi::xml_node wSpPr = wSp.child("xdr:spPr");
                if (!wSpPr) {
                    wSpPr = wSp.child("spPr");
                }
                if (wSpPr) {
                    pugi::xml_node wXfrm = wSpPr.child("a:xfrm");
                    if (!wXfrm) {
                        wXfrm = wSpPr.child("xfrm");
                    }
                    if (wXfrm) {
                        pugi::xml_node wExt = wXfrm.child("a:ext");
                        if (!wExt) {
                            wExt = wXfrm.child("ext");
                        }
                        if (wExt) {
                            const long long wCx = wExt.attribute("cx").as_llong(0);
                            const long long wCy = wExt.attribute("cy").as_llong(0);
                            wWidthPx = wEmuToPx(wCx);
                            wHeightPx = wEmuToPx(wCy);
                        }
                    }
                }

                if (wAnchorName == "xdr:twoCellAnchor" || wAnchorName == "twoCellAnchor") {
                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                    if (!wFrom) {
                        wFrom = wAnchor.child("from");
                    }
                    wApplyFromNode(wFrom);
                } else if (wAnchorName == "xdr:oneCellAnchor" || wAnchorName == "oneCellAnchor") {
                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                    if (!wFrom) {
                        wFrom = wAnchor.child("from");
                    }
                    wApplyFromNode(wFrom);
                } else if (wAnchorName == "xdr:absoluteAnchor" || wAnchorName == "absoluteAnchor") {
                    pugi::xml_node wPosNode = wAnchor.child("xdr:pos");
                    if (!wPosNode) {
                        wPosNode = wAnchor.child("pos");
                    }
                    if (wPosNode) {
                        const long long wX = wPosNode.attribute("x").as_llong(0);
                        const long long wY = wPosNode.attribute("y").as_llong(0);
                        wAnchorRow = 1;
                        wAnchorCol = 1;
                        wPosition = "A1";
                        wDiffX = static_cast<tDouble>(wEmuToPx(wX));
                        wDiffY = static_cast<tDouble>(wEmuToPx(wY));
                    }
                }

                wEntry.Position = wPosition;
                wEntry.WidthPx = wWidthPx;
                wEntry.HeightPx = wHeightPx;
                wEntry.AnchorRow = wAnchorRow;
                wEntry.AnchorCol = wAnchorCol;
                wEntry.DiffX = wDiffX;
                wEntry.DiffY = wDiffY;

                ApplyFirstRunStyle(wTxBody, wEntry);
                wEntry.Color = ResolveFirstRunColor(wTxBody, m_ThemeColors);
                if (wEntry.TextAlign.empty()) {
                    wEntry.TextAlign = "left";
                }

                wOut.push_back(std::move(wEntry));
            }
        }
    }

    return wOut;
}

void tExcelPugiXMLReader::DisplayImages() {
    if (m_UnzippedFiles.empty()) {
        std::cout << "No workbook loaded." << std::endl;
        return;
    }
    std::cout << "\n=== Excel Images ===" << std::endl;
    const std::vector<tExcelImageEntry> wImages = CollectImages();
    if (wImages.empty()) {
        std::cout << "(no images found)" << std::endl;
        return;
    }
    for (const tExcelImageEntry& wImg : wImages) {
        std::cout << "\n-- Sheet (" << wImg.SheetName << ") --" << std::endl;
        std::cout << "Image: " << wImg.Name << std::endl;
        std::cout << "Type: " << wImg.ImageType << std::endl;
        std::cout << "File Size: " << wImg.FileSize << " bytes" << std::endl;
        std::cout << "Path: " << wImg.MediaPath << std::endl;
        if (wImg.Position != "unknown") {
            std::cout << "Position: " << wImg.Position << std::endl;
            if (!wImg.AnchorType.empty() && wImg.AnchorType != "unknown") {
                std::cout << "Anchor Type: " << wImg.AnchorType << std::endl;
            }
            if (!wImg.PositionDetails.empty()) {
                std::cout << "Details: " << wImg.PositionDetails << std::endl;
            }
            if (wImg.WidthPx > 0 && wImg.HeightPx > 0) {
                std::cout << "Dimensions: " << wImg.WidthPx << " x " << wImg.HeightPx
                          << " pixels" << std::endl;
            }
        }
        std::cout << "Base64 length: " << wImg.Base64.size() << std::endl;
    }
}

namespace {

tString JsonEscapeString(const tString& sText) {
    tString wOut;
    wOut.reserve(sText.size() + 8);
    for (tChar wCh : sText) {
        switch (wCh) {
            case '\\': wOut += "\\\\"; break;
            case '"': wOut += "\\\""; break;
            case '\b': wOut += "\\b"; break;
            case '\f': wOut += "\\f"; break;
            case '\n': wOut += "\\n"; break;
            case '\r': wOut += "\\r"; break;
            case '\t': wOut += "\\t"; break;
            default:
                if (static_cast<unsigned char>(wCh) < 0x20) {
                    char wBuf[8];
                    std::snprintf(wBuf, sizeof(wBuf), "\\u%04x", static_cast<unsigned char>(wCh));
                    wOut += wBuf;
                } else {
                    wOut.push_back(wCh);
                }
                break;
        }
    }
    return wOut;
}

tString JsonStringField(const tString& sName, const tString& sValue, tBool sTrailingComma) {
    tString wLine = "      \"" + sName + "\": \"" + JsonEscapeString(sValue) + "\"";
    if (sTrailingComma) {
        wLine += ",";
    }
    return wLine;
}

} // namespace

tInt tExcelPugiXMLReader::SaveImagesJsonToFile(const tString& sOutputPath) const {
    if (sOutputPath.empty()) {
        return -1;
    }
    const std::vector<tExcelImageEntry> wImages = CollectImages();
    std::ostringstream wJson;
    wJson << "{\n  \"images\": [\n";
    for (tSize wIdx = 0; wIdx < wImages.size(); ++wIdx) {
        const tExcelImageEntry& wImg = wImages[wIdx];
        const tBool wLast = (wIdx + 1 >= wImages.size());
        wJson << "    {\n";
        wJson << JsonStringField("sheet", wImg.SheetName, true) << "\n";
        wJson << JsonStringField("name", wImg.Name, true) << "\n";
        wJson << JsonStringField("path", wImg.MediaPath, true) << "\n";
        wJson << JsonStringField("imageType", wImg.ImageType, true) << "\n";
        wJson << JsonStringField("mimeType", wImg.MimeType, true) << "\n";
        wJson << JsonStringField("position", wImg.Position, true) << "\n";
        wJson << JsonStringField("anchorType", wImg.AnchorType, true) << "\n";
        wJson << JsonStringField("positionDetails", wImg.PositionDetails, true) << "\n";
        wJson << "      \"widthPx\": " << wImg.WidthPx << ",\n";
        wJson << "      \"heightPx\": " << wImg.HeightPx << ",\n";
        wJson << "      \"size\": " << wImg.FileSize << ",\n";
        wJson << JsonStringField("base64", wImg.Base64, true) << "\n";
        wJson << JsonStringField("dataUrl", wImg.DataUrl, false) << "\n";
        wJson << "    }";
        if (!wLast) {
            wJson << ",";
        }
        wJson << "\n";
    }
    wJson << "  ]\n}\n";

    SkRoot::tFile wFile(sOutputPath);
    wFile.SaveString(wJson.str());
    return static_cast<tInt>(wImages.size());
}
