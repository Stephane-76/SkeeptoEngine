//=============================================================================
// SkFormatNumber - Excel Format Parser Implementation
//=============================================================================

#include "../include/SkFormatNumber.hpp"
#include "../include/SkFormatDate.hpp"
#include "../include/SkApplication.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cctype>



namespace SkRoot {

// Replace Excel international currency/locale brackets [$€-40C], [$£-en-GB], [$-409], etc.
// Keeps the currency glyph (or $) and drops locale id so tokenization is not split on '$'.
static SkRoot::tString _NormalizeExcelIntlCurrencyBrackets(const SkRoot::tString& s) {
    using SkRoot::tString;
    using SkRoot::tSize;
    static const char kEuro[] = "\xE2\x82\xAC";
    static const char kPound[] = "\xC2\xA3";
    static const char kYen[] = "\xC2\xA5";
    tString out;
    out.reserve(s.size());
    for (tSize i = 0; i < s.size(); ) {
        if (s[i] != '[' || i + 1 >= s.size() || s[i + 1] != '$') {
            out.push_back(s[i]);
            ++i;
            continue;
        }
        tSize j = i + 2;
        while (j < s.size() && s[j] != ']')
            ++j;
        if (j >= s.size()) {
            out.push_back(s[i]);
            ++i;
            continue;
        }
        const tString inner = s.substr(i + 2, j - (i + 2));
        tString replacement;
        tSize p = 0;
        if (inner.size() >= 3 && inner[0] == (char)0xE2 && inner[1] == (char)0x82 && inner[2] == (char)0xAC) {
            replacement = kEuro;
            p = 3;
        } else if (inner.size() >= 2 && inner[0] == (char)0xC2 && inner[1] == (char)0xA3) {
            replacement = kPound;
            p = 2;
        } else if (inner.size() >= 2 && inner[0] == (char)0xC2 && inner[1] == (char)0xA5) {
            replacement = kYen;
            p = 2;
        } else if (p < inner.size() && inner[p] == '$') {
            replacement = "$";
            p += 1;
        } else if (p < inner.size() && inner[p] == '-') {
            // Locale-only [$-409]: strip bracket entirely
            p = inner.size();
        } else {
            // Unknown [$…] body — drop bracket to avoid bogus '$' splits
            i = j + 1;
            continue;
        }
        if (p < inner.size() && inner[p] == '-') {
            ++p;
            while (p < inner.size()) {
                const unsigned char uc = static_cast<unsigned char>(inner[p]);
                if (std::isalnum(uc) || inner[p] == '-' || inner[p] == '_')
                    ++p;
                else
                    break;
            }
        }
        out += replacement;
        i = j + 1;
    }
    return out;
}

tString NormalizeExcelIntlCurrencyBracketsInMask(const tString& sFormatString) {
    return _NormalizeExcelIntlCurrencyBrackets(sFormatString);
}

// True when [...] body is Excel intl currency/locale (not a color/conditional tag).
static bool _ExcelIntlCurrencyBracketInner(const SkRoot::tString& inner) {
    using SkRoot::tString;
    if (inner.empty())
        return false;
    if (inner[0] == '$')
        return true;
    if (inner[0] == '-')
        return true;
    if (inner.size() >= 3 && inner[0] == (char)0xE2 && inner[1] == (char)0x82 && inner[2] == (char)0xAC)
        return true;
    if (inner.size() >= 2 && inner[0] == (char)0xC2 &&
        (inner[1] == (char)0xA3 || inner[1] == (char)0xA5))
        return true;
    return false;
}

// Helper: extract color tag from a section string, returns color name or empty
static SkRoot::tString _ExtractColorTag(const SkRoot::tString& section) {
    using SkRoot::tString;
    using SkRoot::tSize;
    for (tSize i = 0; i < section.size(); ++i) {
        if (section[i] != '[')
            continue;
        tSize j = i + 1;
        while (j < section.size() && section[j] != ']')
            ++j;
        if (j >= section.size())
            break;
        const tString inner = section.substr(i + 1, j - i - 1);
        i = j;
        if (_ExcelIntlCurrencyBracketInner(inner))
            continue;
        return inner;
    }
    return tString();
}

namespace {

// Scan comma/dot roles outside quotes and top-level brackets (same boundary rules as SplitExcelFormatSections).
static void CollectCommaDotSeparators(const tString& sec, std::vector<std::pair<tSize, char>>& oSepPositions) {
    bool inQuotes = false;
    tInt bracketDepth = 0;
    for (tSize i = 0; i < sec.size(); ++i) {
        const tChar c = sec[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes) {
            if (c == '[') {
                ++bracketDepth;
            } else if (c == ']' && bracketDepth > 0) {
                --bracketDepth;
            }
            if (bracketDepth == 0 && (c == ',' || c == '.')) {
                oSepPositions.emplace_back(i, static_cast<char>(c));
            }
        }
    }
}

static void InferCommaDotRoles(const tString& sec, tString& oDecimalSep, tString& oThousandsSep) {
    oDecimalSep.clear();
    oThousandsSep.clear();
    std::vector<std::pair<tSize, char>> seps;
    CollectCommaDotSeparators(sec, seps);
    const tInt separatorCount = static_cast<tInt>(seps.size());
    if (separatorCount == 0) {
        return;
    }

    const char lastSeparator = seps.back().second;
    const tSize sepPos = seps.back().first;

    if (separatorCount > 1) {
        oDecimalSep = tString(1, static_cast<tChar>(lastSeparator));
        oThousandsSep = (lastSeparator == '.') ? tString(",") : tString(".");
        return;
    }

    const tBool hasPercentSymbol = sec.find('%') != tString::npos;
    tBool hasDigitsAfterSep = false;
    if (sepPos + 1 < sec.size()) {
        for (tSize k = sepPos + 1; k < sec.size(); ++k) {
            const tChar ch = sec[k];
            if (ch == '0' || ch == '#') {
                hasDigitsAfterSep = true;
                break;
            }
            if (ch == ';' || ch == '%' || ch == '$') {
                break;
            }
            if (ch == (char)0xE2 && k + 2 < sec.size() &&
                sec[k + 1] == (char)0x82 && sec[k + 2] == (char)0xAC) {
                break;
            }
            if (ch == (char)0xC2 && k + 1 < sec.size() &&
                (sec[k + 1] == (char)0xA3 || sec[k + 1] == (char)0xA5)) {
                break;
            }
        }
    }
    tBool looksLikeGrouping = false;
    if (sepPos > 0 && sepPos + 1 < sec.size()) {
        looksLikeGrouping = (sec[sepPos - 1] == '#' && sec[sepPos + 1] == '#');
    }

    if (!looksLikeGrouping && (hasPercentSymbol || hasDigitsAfterSep)) {
        oDecimalSep = tString(1, static_cast<tChar>(lastSeparator));
    } else {
        oThousandsSep = tString(1, static_cast<tChar>(lastSeparator));
    }
}

static void SwapCommaDotProtected(tString& s) {
    bool inQuotes = false;
    tInt bracketDepth = 0;
    for (tSize i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes) {
            if (s[i] == '[') {
                ++bracketDepth;
            } else if (s[i] == ']' && bracketDepth > 0) {
                --bracketDepth;
            }
            if (bracketDepth == 0) {
                if (s[i] == ',') {
                    s[i] = '\x01';
                } else if (s[i] == '.') {
                    s[i] = ',';
                }
            }
        }
    }
    for (tSize i = 0; i < s.size(); ++i) {
        if (s[i] == '\x01') {
            s[i] = '.';
        }
    }
}

static void ReplaceCharProtected(tString& s, tChar fromCh, tChar toCh) {
    bool inQuotes = false;
    tInt bracketDepth = 0;
    for (tSize i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes) {
            if (s[i] == '[') {
                ++bracketDepth;
            } else if (s[i] == ']' && bracketDepth > 0) {
                --bracketDepth;
            }
            if (bracketDepth == 0 && s[i] == fromCh) {
                s[i] = toCh;
            }
        }
    }
}

static tString NormalizeOneExcelSectionCommaDotToUs(const tString& sec) {
    tString decimalSep;
    tString thousandsSep;
    InferCommaDotRoles(sec, decimalSep, thousandsSep);

    if (decimalSep == "." && thousandsSep == ",") {
        return sec;
    }
    if (decimalSep == "," && thousandsSep == ".") {
        tString s = sec;
        SwapCommaDotProtected(s);
        return s;
    }
    if (decimalSep == "," && thousandsSep.empty()) {
        tString s = sec;
        ReplaceCharProtected(s, ',', '.');
        return s;
    }
    if (decimalSep.empty() && thousandsSep == ".") {
        tString s = sec;
        ReplaceCharProtected(s, '.', ',');
        return s;
    }
    return sec;
}

static tString JoinExcelFormatSections(const std::vector<tString>& secs) {
    tString out;
    for (tSize i = 0; i < secs.size(); ++i) {
        if (i != 0) {
            out += ';';
        }
        out += secs[i];
    }
    return out;
}

// Intermediate numeric strings use US convention: ',' thousands, '.' decimal (before locale remap).
static tString ApplyLocaleSeparatorsToFixedNumber(const tString& r, tChar localeDec, tChar localeThous) {
    if (localeDec == '.' && localeThous == ',') {
        return r;
    }

    tSize decPos = tString::npos;
    for (tSize i = 0; i < r.size(); ++i) {
        if (r[i] != '.') {
            continue;
        }
        if (i > 0 && std::isdigit(static_cast<unsigned char>(r[i - 1])) && i + 1 < r.size() &&
            std::isdigit(static_cast<unsigned char>(r[i + 1]))) {
            decPos = i;
        }
    }

    if (decPos == tString::npos) {
        tString out = r;
        if (localeThous != ',') {
            for (tSize i = 0; i < out.size(); ++i) {
                if (out[i] == ',') {
                    out[i] = localeThous;
                }
            }
        }
        return out;
    }

    tString intPart = r.substr(0, decPos);
    tString fracPart = r.substr(decPos + 1);
    if (localeThous != ',') {
        for (tSize i = 0; i < intPart.size(); ++i) {
            if (intPart[i] == ',') {
                intPart[i] = localeThous;
            }
        }
    }
    const tString locDec(1, localeDec);
    return intPart + locDec + fracPart;
}

static tString ApplyLocaleSeparatorsToFormattedNumber(const tString& usFormatted, tChar localeDec, tChar localeThous,
                                                      tBool isScientific) {
    if (localeDec == '.' && localeThous == ',') {
        return usFormatted;
    }

    if (isScientific) {
        const tSize ePos = usFormatted.find('E');
        if (ePos != tString::npos) {
            const tString mantissa = usFormatted.substr(0, ePos);
            const tString expo = usFormatted.substr(ePos);
            return ApplyLocaleSeparatorsToFixedNumber(mantissa, localeDec, localeThous) + expo;
        }
    }
    return ApplyLocaleSeparatorsToFixedNumber(usFormatted, localeDec, localeThous);
}

} // namespace

    tString NormalizeExcelNumberFormatToUs(const tString& sFormatString) {
        if (sFormatString.empty()) {
            return sFormatString;
        }
        const tString currencyNorm = _NormalizeExcelIntlCurrencyBrackets(sFormatString);
        const std::vector<tString> secs = SplitExcelFormatSections(currencyNorm);
        std::vector<tString> normalized;
        normalized.reserve(secs.size());
        for (const auto& sec : secs) {
            normalized.push_back(NormalizeOneExcelSectionCommaDotToUs(sec));
        }
        return JoinExcelFormatSections(normalized);
    }

    // Constructor =============================================================
    tExcelFormatParser::tExcelFormatParser(const tString& sFormatString) 
        : m_FormatString(NormalizeExcelNumberFormatToUs(sFormatString)),
          m_ParseNormalized(),
          m_IntegerPart(),                     // Integer part pattern
          m_DecimalPart(),                   // Decimal part pattern
          m_ThousandsSeparator(),              // Thousands separator
          m_DecimalSeparator(),            // Decimal separator
          m_CurrencySymbol(),                  // Currency symbol if present
          m_HasPercent(false),
          m_IsScientific(false),
          m_ExponentDigits(0),
          m_DecimalPlaces(0), m_IntegerDigits(0), m_IsValid(false) {
        if (!m_FormatString.empty()) {
            ParseFormatString();
        }
    }

    // Destructor ==============================================================
    tExcelFormatParser::~tExcelFormatParser() {
        Clear();
    }

    // Set Format String ======================================================
    void tExcelFormatParser::SetFormatString(const tString& sFormatString) {
        m_FormatString = NormalizeExcelNumberFormatToUs(sFormatString);
        Clear();
        if (!m_FormatString.empty()) {
            ParseFormatString();
        }
    }

    // Get Format String ======================================================
    tString tExcelFormatParser::GetFormatString() const {
        return m_FormatString;
    }

    // Get Elements ============================================================
    const tVectorExcelFormatElement& tExcelFormatParser::GetElements() const {
        return m_Elements;
    }

    // Get Integer Part =======================================================
    tString tExcelFormatParser::GetIntegerPart() const {
        return m_IntegerPart;
    }

    // Get Decimal Part =======================================================
    tString tExcelFormatParser::GetDecimalPart() const {
        return m_DecimalPart;
    }

    // Get Thousands Separator ================================================
    tString tExcelFormatParser::GetThousandsSeparator() const {
        return m_ThousandsSeparator;
    }

    // Get Decimal Separator ==================================================
    tString tExcelFormatParser::GetDecimalSeparator() const {
        return m_DecimalSeparator;
    }

    // Get Currency Symbol ====================================================
    tString tExcelFormatParser::GetCurrencySymbol() const {
        return m_CurrencySymbol;
    }

    // Has Percent =============================================================
    tBool tExcelFormatParser::HasPercent() const {
        return m_HasPercent;
    }

    // Get Decimal Places =====================================================
    tInt tExcelFormatParser::GetDecimalPlaces() const {
        return m_DecimalPlaces;
    }

    // Get Integer Digits =====================================================
    tInt tExcelFormatParser::GetIntegerDigits() const {
        return m_IntegerDigits;
    }

    // Is Valid ================================================================
    tBool tExcelFormatParser::IsValid() const {
        return m_IsValid;
    }

    // Clear ===================================================================
    void tExcelFormatParser::Clear() {
        m_Elements.clear();
        m_IntegerPart.clear();
        m_DecimalPart.clear();
        m_ThousandsSeparator.clear();
        m_DecimalSeparator.clear();
        m_CurrencySymbol.clear();
        m_HasPercent = false;
        m_IsScientific = false;
        m_ExponentDigits = 0;
        m_DecimalPlaces = 0;
        m_IntegerDigits = 0;
        m_IsValid = false;
        m_ParseNormalized.clear();
    }

    // Parse Format String ====================================================
    void tExcelFormatParser::ParseFormatString() {
        if (m_FormatString.empty()) {
            m_ParseNormalized.clear();
            m_IsValid = false;
            return;
        }

        m_ParseNormalized = _NormalizeExcelIntlCurrencyBrackets(m_FormatString);

        // Pre-allocate elements vector for better performance
        // Estimate capacity based on format string length (roughly 1 element per 2-3 chars)
        m_Elements.reserve(m_ParseNormalized.length() / 2);

        // Split format string into elements
        tString currentElement;
        currentElement.reserve(16); // Pre-allocate for typical element size
        tInt position = 0;
        
        for (tSize i = 0; i < m_ParseNormalized.length(); ++i) {
            tChar c = m_ParseNormalized[i];
            
            // Check for separators that split elements
            if (c == '.' || c == ',' || c == ' ' || c == '%' || c == '$') {
                if (!currentElement.empty()) {
                    ParseElement(currentElement, position);
                    position += static_cast<SkRoot::tInt>(currentElement.length());
                    currentElement.clear();
                }
                
                // Handle the separator itself
                tString separator(1, c);
                ParseElement(separator, position);
                position++;
            } else if (c == (char)0xE2 && i + 2 < m_ParseNormalized.length() && 
                       m_ParseNormalized[i+1] == (char)0x82 && m_ParseNormalized[i+2] == (char)0xAC) {
                // Handle Euro symbol (€) - UTF-8 encoded as E2 82 AC
                if (!currentElement.empty()) {
                    ParseElement(currentElement, position);
                    position += static_cast<SkRoot::tInt>(currentElement.length());
                    currentElement.clear();
                }
                
                // Handle the Euro symbol
                tString separator("€");
                ParseElement(separator, position);
                position += 3; // Skip the 3 UTF-8 bytes
                i += 2; // Skip the next 2 characters
            } else if (c == (char)0xC2 && i + 1 < m_ParseNormalized.length() && m_ParseNormalized[i+1] == (char)0xA5) {
                // Handle Yen symbol (¥) - UTF-8 encoded as C2 A5
                if (!currentElement.empty()) {
                    ParseElement(currentElement, position);
                    position += static_cast<SkRoot::tInt>(currentElement.length());
                    currentElement.clear();
                }

                tString separator("¥");
                ParseElement(separator, position);
                position += 2; // Skip 2 UTF-8 bytes
                i += 1; // Skip the next character byte
            } else if (c == (char)0xC2 && i + 1 < m_ParseNormalized.length() && m_ParseNormalized[i+1] == (char)0xA3) {
                // Handle Pound symbol (£) - UTF-8 encoded as C2 A3
                if (!currentElement.empty()) {
                    ParseElement(currentElement, position);
                    position += static_cast<SkRoot::tInt>(currentElement.length());
                    currentElement.clear();
                }

                tString separator("£");
                ParseElement(separator, position);
                position += 2; // Skip 2 UTF-8 bytes
                i += 1; // Skip the next character byte
            } else {
                currentElement += c;
            }
        }
        
        // Handle last element
        if (!currentElement.empty()) {
            ParseElement(currentElement, position);
        }
        
        // Analyze all elements
        AnalyzeElements();
        
        // Only consider valid if we have at least one numeric element
        tBool hasNumericElement = false;
        for (const auto& element : m_Elements) {
            if (element.m_Type == "numeric") {
                hasNumericElement = true;
                break;
            }
        }
        m_IsValid = hasNumericElement;
    }

    // Parse Element ===========================================================
    void tExcelFormatParser::ParseElement(const tString& sElement, tInt sPosition) {
        tExcelFormatElement element;
        element.m_Element = sElement;
        element.m_Position = sPosition;
        element.m_Length = static_cast<SkRoot::tInt>(sElement.length());
        element.m_IsOptional = IsOptionalPattern(sElement);
        element.m_IsGrouping = IsGroupingSeparator(sElement);
        element.m_IsDecimal = IsDecimalSeparator(sElement);
        
        // Determine element type
        if (element.m_IsGrouping) {
            element.m_Type = "separator";
        } else if (element.m_IsDecimal) {
            element.m_Type = "separator";
        } else if (IsCurrencySymbol(sElement)) {
            element.m_Type = "currency";
        } else if (IsPercentSymbol(sElement)) {
            element.m_Type = "percent";
        } else if (sElement.find('#') != tString::npos || sElement.find('0') != tString::npos) {
            element.m_Type = "numeric";
        } else {
            element.m_Type = "unknown";
        }
        
        m_Elements.push_back(element);
    }

    // Analyze Elements ========================================================
    void tExcelFormatParser::AnalyzeElements() {
        tBool foundDecimal = false;
        tString integerPattern;
        tString decimalPattern;
        
        // First pass: identify separators and their roles
        tString lastSeparator;
        tInt separatorCount = 0;
        
        for (const auto& element : m_Elements) {
            if (element.m_Type == "separator") {
                separatorCount++;
                lastSeparator = element.m_Element;
            }
        }
        
        // Detect scientific notation (look for 'E' with optional sign and digits)
        {
            tSize ePos = m_ParseNormalized.find('E');
            if (ePos != tString::npos && ePos + 1 < m_ParseNormalized.size()) {
                m_IsScientific = true;
                // Count exponent digits after optional sign
                tInt count = 0;
                tSize ie = ePos + 1;
                if (ie < m_ParseNormalized.size() && (m_ParseNormalized[ie] == '+' || m_ParseNormalized[ie] == '-')) {
                    ie++;
                }
                while (ie < m_ParseNormalized.size() && m_ParseNormalized[ie] == '0') { count++; ie++; }
                m_ExponentDigits = count;
            }
        }

        // Determine decimal separator based on context
        // If there's only one separator, it's likely decimal
        // If there are multiple separators, the last one is usually decimal
        tString decimalSep = "";
        tString thousandsSep = "";
        
        if (separatorCount == 1) {
            // Only one separator present.
            // Heuristic: if there are numeric placeholders after the separator in the
            // format string (e.g., "0,00" or "0.00"), then this separator is decimal.
            // Also, if the format contains a percent symbol, the single separator is decimal.
            tBool hasPercentSymbol = (m_ParseNormalized.find('%') != tString::npos);
            tSize sepPos = m_ParseNormalized.find(lastSeparator);
            tBool hasDigitsAfterSep = false;
            tBool looksLikeGrouping = false;
            if (sepPos != tString::npos) {
                for (tSize k = sepPos + 1; k < m_ParseNormalized.size(); ++k) {
                    if (m_ParseNormalized[k] == '0' || m_ParseNormalized[k] == '#') {
                        hasDigitsAfterSep = true;
                        break;
                    }
                    // Stop scanning at section or currency/percent symbols
                    if (m_ParseNormalized[k] == ';' || m_ParseNormalized[k] == '%' || m_ParseNormalized[k] == '$') {
                        break;
                    }
                    // Currency glyphs after normalization (€ £ ¥ UTF-8)
                    if (m_ParseNormalized[k] == (char)0xE2 && k + 2 < m_ParseNormalized.size() &&
                        m_ParseNormalized[k + 1] == (char)0x82 && m_ParseNormalized[k + 2] == (char)0xAC)
                        break;
                    if (m_ParseNormalized[k] == (char)0xC2 && k + 1 < m_ParseNormalized.size() &&
                        (m_ParseNormalized[k + 1] == (char)0xA3 || m_ParseNormalized[k + 1] == (char)0xA5))
                        break;
                }
                // Grouping detection like "#,##0" or "##,###"
                if ((sepPos > 0 && m_ParseNormalized[sepPos-1] == '#') &&
                    (sepPos + 1 < m_ParseNormalized.size() && m_ParseNormalized[sepPos+1] == '#')) {
                    looksLikeGrouping = true;
                }
            }
            if (!looksLikeGrouping && (hasPercentSymbol || hasDigitsAfterSep)) {
                decimalSep = lastSeparator;
                thousandsSep = "";
            } else {
                thousandsSep = lastSeparator;
                decimalSep = "";
            }
        } else if (separatorCount > 1) {
            // Multiple separators - last one is decimal, others are thousands
            for (const auto& element : m_Elements) {
                if (element.m_Type == "separator") {
                    if (element.m_Element == lastSeparator) {
                        decimalSep = element.m_Element;
                    } else {
                        thousandsSep = element.m_Element;
                    }
                }
            }
        }
        
        m_DecimalSeparator = decimalSep;
        m_ThousandsSeparator = thousandsSep;
        
        // Second pass: build patterns
        for (const auto& element : m_Elements) {
            if (element.m_Type == "percent") {
                m_HasPercent = true;
            } else if (element.m_Type == "currency") {
                m_CurrencySymbol = element.m_Element;
            } else if (element.m_Type == "separator") {
                if (element.m_Element == decimalSep) {
                    foundDecimal = true;
                } else if (element.m_Element == thousandsSep) {
                    // Include thousands separator in integer pattern
                    integerPattern += element.m_Element;
                }
            } else if (element.m_Type == "numeric") {
                if (foundDecimal) {
                    decimalPattern += element.m_Element;
                } else {
                    integerPattern += element.m_Element;
                }
            }
        }

        // If no decimal pattern was found, ensure decimal separator is cleared
        // and the single separator (if any) is treated as thousands separator only.
        if (decimalPattern.empty()) {
            decimalSep = "";
            m_DecimalSeparator = decimalSep;
            m_ThousandsSeparator = thousandsSep;
        }
        
        m_IntegerPart = integerPattern;
        m_DecimalPart = decimalPattern;
        
        // Count decimal places
        m_DecimalPlaces = 0;
        for (tChar c : m_DecimalPart) {
            if (c == '0') {
                m_DecimalPlaces++;
            }
        }
        
        // Count integer digits (including optional ones)
        m_IntegerDigits = 0;
        for (tChar c : m_IntegerPart) {
            if (c == '0' || c == '#') {
                m_IntegerDigits++;
            }
        }
    }

    // Extract Pattern =========================================================
    tString tExcelFormatParser::ExtractPattern(const tString& sElement) {
        tString pattern;
        for (tChar c : sElement) {
            if (c == '0' || c == '#') {
                pattern += c;
            }
        }
        return pattern;
    }

    // Is Optional Pattern =====================================================
    tBool tExcelFormatParser::IsOptionalPattern(const tString& sElement) {
        return sElement.find('#') != tString::npos;
    }

    // Is Grouping Separator ===================================================
    tBool tExcelFormatParser::IsGroupingSeparator(const tString& sElement) {
        return sElement == "." || sElement == ",";
    }

    // Is Decimal Separator ====================================================
    tBool tExcelFormatParser::IsDecimalSeparator(const tString& sElement) {
        // In Excel, we need to determine which separator is decimal based on context
        // This will be determined during analysis, not here
        return false; // Will be set during analysis
    }

    // Is Currency Symbol ======================================================
    tBool tExcelFormatParser::IsCurrencySymbol(const tString& sElement) {
        return sElement == "$" || sElement == "€" || sElement == "£" || sElement == "¥";
    }

    // Is Percent Symbol =======================================================
    tBool tExcelFormatParser::IsPercentSymbol(const tString& sElement) {
        return sElement == "%";
    }

    // To SkRoot Format Type ===================================================
    tFormatStringType tExcelFormatParser::ToSkRootFormatType() const {
        if (m_HasPercent) {
            return tFormatStringType::percent;
        } else if (!m_CurrencySymbol.empty()) {
            return tFormatStringType::accounting;
        } else if (!m_DecimalPart.empty()) {
            return tFormatStringType::numeric;
        } else {
            return tFormatStringType::numeric0;
        }
    }

    // To SkRoot Format String =================================================
    tString tExcelFormatParser::ToSkRootFormatString() const {
        tStringStream ss;
        
        if (!m_CurrencySymbol.empty()) {
            ss << m_CurrencySymbol;
        }
        
        // Integer part
        if (!m_IntegerPart.empty()) {
            ss << m_IntegerPart;
        }
        
        // Decimal part
        if (!m_DecimalPart.empty()) {
            ss << m_DecimalSeparator << m_DecimalPart;
        }
        
        if (m_HasPercent) {
            ss << "%";
        }
        
        return ss.str();
    }

    // Format Number ===========================================================
    tString tExcelFormatParser::FormatNumber(tDouble sNumber) const {
        if (!m_IsValid) {
            return "";
        }
        
        // Apply percent if needed
        tDouble number = m_HasPercent ? sNumber * 100.0 : sNumber;
        
        // Format the number with proper decimal places or scientific
        tString result;
        result.reserve(32); // Pre-allocate for typical number length
        if (m_IsScientific) {
            // Pre-round the mantissa using round-half-away-from-zero to match
            // Excel. Without this, std::setprecision relies on the standard
            // library's banker's rounding (round-half-to-even) which gives
            // wrong results for ".5" boundaries (e.g., 2.5e0 -> "3E+00").
            //
            // The output uses setprecision(m_DecimalPlaces + 1), which in
            // std::scientific means (m_DecimalPlaces + 1) digits after the
            // mantissa's decimal point — i.e. (m_DecimalPlaces + 2) total
            // significant digits. The pre-rounding scale must therefore be
            // 10^((m_DecimalPlaces + 1) - order), one more than the digits
            // after the point.
            if (number != 0.0) {
                const int wOrder = static_cast<int>(std::floor(std::log10(std::fabs(number))));
                const tDouble wScale = std::pow(10.0, static_cast<int>(m_DecimalPlaces) + 1 - wOrder);
                number = std::round(number * wScale) / wScale;
            }
            std::ostringstream sss;
            sss.setf(std::ios::scientific, std::ios::floatfield);
            sss << std::setprecision(static_cast<int>(m_DecimalPlaces + 1)) << number; // one leading digit + decimals
            result = sss.str();
            // Normalize exponent sign and digits to requested width (E+00 or E-00)
            tSize epos = result.find('e');
            if (epos != tString::npos) {
                // Uppercase 'E'
                result[epos] = 'E';
                // Ensure sign is present and pad exponent digits
                tString mantissa = result.substr(0, epos);
                tString expo = result.substr(epos + 1); // e.g., +06 or -06
                if (!expo.empty() && (expo[0] == '+' || expo[0] == '-')) {
                    tChar sign = expo[0];
                    tString digits = expo.substr(1);
                    // Remove leading plus-minus artifacts
                    while (!digits.empty() && digits[0] == '0') {
                        // keep zeros; we'll repad below
                        break;
                    }
                    // Pad to requested width (default 2)
                    tInt width = (m_ExponentDigits > 0) ? m_ExponentDigits : 2;
                    if (digits.length() < static_cast<tSize>(width)) {
                        digits = tString(width - static_cast<tInt>(digits.length()), '0') + digits;
                    }
                    result = mantissa + "E" + sign + digits;
                }
            }
        } else {
            // Pre-round to the requested decimal precision using
            // round-half-away-from-zero to match Excel. std::setprecision
            // alone uses banker's rounding which produces "16%" for 16.5%
            // and similar boundary mismatches against Excel.
            const tDouble wScale = std::pow(10.0, static_cast<int>(m_DecimalPlaces));
            const tDouble wRounded = std::round(number * wScale) / wScale;
            tStringStream ss;
            ss << std::fixed << std::setprecision(m_DecimalPlaces);
            ss << wRounded;
            result = ss.str();
        }
        
        // Special case: if thousands separator is '.' and decimal separator is ','
        // we need to apply points as thousands separators first, then replace only the decimal point
        if (m_IsScientific) {
            // Do not apply thousands separators to scientific notation,
            // but apply decimal separator from the pattern if needed
            if (!m_DecimalSeparator.empty() && m_DecimalSeparator != ".") {
                tSize epos = result.find('E');
                if (epos != tString::npos) {
                    tSize dotPos = result.find('.');
                    if (dotPos != tString::npos && dotPos < epos) {
                        result.replace(dotPos, 1, m_DecimalSeparator);
                    }
                }
            }
        } else if (m_ThousandsSeparator == "." && m_DecimalSeparator == ",") {
            // Apply thousands separator with points
            result = ApplyThousandsSeparator(result, ".");
            
            // Replace only the decimal point with comma
            tSize decimalPos = result.find_last_of('.');
            if (decimalPos != tString::npos) {
                result[decimalPos] = ',';
            }
        } else if (!m_ThousandsSeparator.empty() && !m_DecimalSeparator.empty() && 
                   m_ThousandsSeparator != m_DecimalSeparator) {
            // Apply thousands separator to integer part only
            result = ApplyThousandsSeparator(result, m_ThousandsSeparator);
            
            // Replace decimal separator
            if (m_DecimalSeparator != ".") {
                tSize decimalPos = result.find('.');
                if (decimalPos != tString::npos) {
                    result.replace(decimalPos, 1, m_DecimalSeparator);
                }
            }
        } else if (!m_ThousandsSeparator.empty()) {
            // Only thousands separator
            result = ApplyThousandsSeparator(result, m_ThousandsSeparator);
        } else if (!m_DecimalSeparator.empty() && m_DecimalSeparator != ".") {
            // Only decimal separator
            tSize decimalPos = result.find('.');
            if (decimalPos != tString::npos) {
                result.replace(decimalPos, 1, m_DecimalSeparator);
            }
        }
        
        // Add currency symbol if present
        if (!m_CurrencySymbol.empty()) {
            // Check if currency symbol should be at the end by looking at the normalized format
            tString formatTrimmed = m_ParseNormalized.empty() ? m_FormatString : m_ParseNormalized;
            // Remove spaces from the end to check position
            while (!formatTrimmed.empty() && formatTrimmed.back() == ' ') {
                formatTrimmed.pop_back();
            }
            
            // Check if format ends with currency symbol (with or without space)
            tBool isAtEnd = false;
            if (formatTrimmed.length() >= m_CurrencySymbol.length()) {
                tString suffix = formatTrimmed.substr(formatTrimmed.length() - m_CurrencySymbol.length());
                isAtEnd = (suffix == m_CurrencySymbol);
            }
            
            // Also check if there's a space before the currency symbol at the end
            if (!isAtEnd && formatTrimmed.length() > m_CurrencySymbol.length()) {
                tString suffixWithSpace = formatTrimmed.substr(formatTrimmed.length() - m_CurrencySymbol.length() - 1);
                if (suffixWithSpace == " " + m_CurrencySymbol) {
                    isAtEnd = true;
                }
            }
            
            if (isAtEnd) {
                result = result + " " + m_CurrencySymbol;
            } else {
                result = m_CurrencySymbol + result;
            }
        }
        
        // Add percent symbol if present in the format (respect locale decimal)
        if (m_HasPercent) {
            result += "%";
        }

        // Parsing/output intermediate uses US separators (, thousands . decimal); swap for UI locale.
        if (tApplication::Instance() != nullptr && tApplication::Instance()->Locale() != nullptr) {
            tLocale* wLoc = tApplication::Instance()->Locale();
            result = ApplyLocaleSeparatorsToFormattedNumber(result, wLoc->Decimal(), wLoc->Thousand(), m_IsScientific);
        }

        return result;
    }
    
    // Apply Thousands Separator ===============================================
    tString tExcelFormatParser::ApplyThousandsSeparator(const tString& sNumber, const tString& sThousandsSep) const {
        // Find decimal point position (always use '.' as it's the C++ standard)
        tSize decimalPos = sNumber.find('.');
        if (decimalPos == tString::npos) {
            decimalPos = sNumber.length();
        }
        
        // Extract integer part only
        tString integerPart = sNumber.substr(0, decimalPos);
        tString decimalPart = (decimalPos != sNumber.length()) ? sNumber.substr(decimalPos) : "";
        
        // Add thousands separators to integer part only
        tString formattedInteger = "";
        tInt start = (integerPart[0] == '-') ? 1 : 0; // Skip negative sign
        
        // Add thousands separators from right to left
        tInt digitsProcessed = 0;
        for (tInt i = static_cast<tInt>(integerPart.length()) - 1; i >= start; --i) {
            if (integerPart[i] >= '0' && integerPart[i] <= '9') {
                digitsProcessed++;
                if (digitsProcessed > 1 && digitsProcessed % 3 == 1) {
                    formattedInteger = sThousandsSep + formattedInteger;
                }
                formattedInteger = integerPart[i] + formattedInteger;
            } else {
                formattedInteger = integerPart[i] + formattedInteger;
            }
        }
        
        // Add negative sign if present
        if (start == 1) {
            formattedInteger = "-" + formattedInteger;
        }
        
        // Reconstruct the result (decimal part unchanged)
        return formattedInteger + decimalPart;
    }

    // Get Description =========================================================
    tString tExcelFormatParser::GetDescription() const {
        tStringStream ss;
        ss << "Excel Format: " << m_FormatString << "\n";
        ss << "Integer Part: " << m_IntegerPart << "\n";
        ss << "Decimal Part: " << m_DecimalPart << "\n";
        ss << "Decimal Places: " << m_DecimalPlaces << "\n";
        ss << "Integer Digits: " << m_IntegerDigits << "\n";
        ss << "Thousands Separator: '" << m_ThousandsSeparator << "'\n";
        ss << "Decimal Separator: '" << m_DecimalSeparator << "'\n";
        ss << "Currency Symbol: '" << m_CurrencySymbol << "'\n";
        ss << "Has Percent: " << (m_HasPercent ? "Yes" : "No") << "\n";
        ss << "Valid: " << (m_IsValid ? "Yes" : "No") << "\n";
        return ss.str();
    }

    // Print Elements ==========================================================
    void tExcelFormatParser::PrintElements() const {
        cout << "Excel Format Elements for: " << m_FormatString << endl;
        cout << "================================================" << endl;
        
        for (const auto& element : m_Elements) {
            cout << "Element: '" << element.m_Element << "'" << endl;
            cout << "  Type: " << element.m_Type << endl;
            cout << "  Position: " << element.m_Position << endl;
            cout << "  Length: " << element.m_Length << endl;
            cout << "  Optional: " << (element.m_IsOptional ? "Yes" : "No") << endl;
            cout << "  Grouping: " << (element.m_IsGrouping ? "Yes" : "No") << endl;
            cout << "  Decimal: " << (element.m_IsDecimal ? "Yes" : "No") << endl;
            cout << "---" << endl;
        }
    }

    // Utility Functions =======================================================

    // Parse Excel Format ======================================================
    tVectorExcelFormatElement ParseExcelNumberFormat(const tString& sFormatString) {
        const tString norm = NormalizeExcelNumberFormatToUs(sFormatString);
        const auto secs = SplitExcelFormatSections(norm);
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            tExcelFormatParser parser(secs[i]);
            if (parser.IsValid()) {
                return parser.GetElements();
            }
        }
        tExcelFormatParser parser(norm);
        return parser.GetElements();
    }

    // Is Valid Excel Format ===================================================
    tBool IsValidExcelNumberFormat(const tString& sFormatString) {
        const tString norm = NormalizeExcelNumberFormatToUs(sFormatString);
        const auto secs = SplitExcelFormatSections(norm);
        if (secs.empty()) {
            return false;
        }
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            tExcelFormatParser parser(secs[i]);
            if (parser.IsValid()) {
                return true;
            }
        }
        return false;
    }

    // Convert To SkRoot Format ================================================
    tString ConvertToSkRootFormat(const tString& sExcelFormat) {
        const tString norm = NormalizeExcelNumberFormatToUs(sExcelFormat);
        const auto secs = SplitExcelFormatSections(norm);
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            tExcelFormatParser parser(secs[i]);
            if (parser.IsValid()) {
                return parser.ToSkRootFormatString();
            }
        }
        tExcelFormatParser parser(norm);
        return parser.ToSkRootFormatString();
    }

    // Format With Excel Format ================================================
    tString FormatWithExcelNumberFormat(tDouble sNumber, const tString& sExcelFormat) {
        tNumberFormatter formatter;
        return formatter.FormatNumber(sNumber, sExcelFormat);
    }


    // Number Formatter with Pool Implementation ===============================
    // Constructor =============================================================
    tNumberFormatter::tNumberFormatter() {
        // Initialize with empty pool
    }

    // Destructor ==============================================================
    tNumberFormatter::~tNumberFormatter() {
        ClearPool();
    }

    // Get Parser ==============================================================
    std::shared_ptr<tExcelFormatParser> tNumberFormatter::GetParser(const SkRoot::tString& sFormatString) {
        const tString key = NormalizeExcelNumberFormatToUs(sFormatString);
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        
        // Check if format is already cached
        auto it = m_FormatPool.find(key);
        if (it != m_FormatPool.end()) {
            return it->second;
        }
        
        // Create new parser and cache it (canonical US-mask key)
        auto parser = std::make_shared<tExcelFormatParser>(key);
        m_FormatPool[key] = parser;
        return parser;
    }

    // Format Number ===========================================================
    tString tNumberFormatter::FormatNumber(SkRoot::tDouble sNumber, const SkRoot::tString& sFormatString) {
        const tString normalizedFull = NormalizeExcelNumberFormatToUs(sFormatString);
        const std::vector<tString> sections = SplitExcelFormatSections(normalizedFull);
        const tBool hasSections = sections.size() > 1;
        tString selectedFormat = normalizedFull;
        tBool useParensForNegative = false;
        if (hasSections) {
            selectedFormat = SelectExcelFormatNumericSection(sections, sNumber);
            if (selectedFormat.find('(') != tString::npos && selectedFormat.find(')') != tString::npos) {
                useParensForNegative = true;
            }
        }

        auto parser = GetParser(selectedFormat);
        if (parser && parser->IsValid()) {
            tDouble valueToFormat = hasSections && sNumber < 0 ? std::fabs(sNumber) : sNumber;
            tString result = parser->FormatNumber(valueToFormat);

            // If negative section demands parentheses and currency is a suffix, wrap numeric only
            if (hasSections && sNumber < 0 && useParensForNegative) {
                const tString currency = parser->GetCurrencySymbol();
                if (!currency.empty()) {
                    // Check for suffix with optional space
                    tString suffixWithSpace = " " + currency;
                    if (result.size() >= suffixWithSpace.size() &&
                        result.compare(result.size() - suffixWithSpace.size(), suffixWithSpace.size(), suffixWithSpace) == 0) {
                        tString numericPart = result.substr(0, result.size() - suffixWithSpace.size());
                        result = "(" + numericPart + ")" + suffixWithSpace;
                        return result;
                    }
                }
                // Default: wrap whole result
                result = "(" + result + ")";
            }
            // Negative section with explicit '-' sign but no parentheses
            if (hasSections && sNumber < 0 && !useParensForNegative) {
                if (selectedFormat.find('-') != tString::npos) {
                    // Prefix a minus if not already present
                    if (result.empty() || result[0] != '-') {
                        result = "-" + result;
                    }
                }
            }
            return result;
        }
        return "";
    }

    // Format Numbers ==========================================================
    std::vector<SkRoot::tString> tNumberFormatter::FormatNumbers(const std::vector<SkRoot::tDouble>& sNumbers, const SkRoot::tString& sFormatString) {
        std::vector<tString> results;
        results.reserve(sNumbers.size());
        for (const auto& number : sNumbers) {
            results.push_back(FormatNumber(number, sFormatString));
        }
        return results;
    }

    // Format Number With Info ================================================
    tFormatResult tNumberFormatter::FormatNumberWithInfo(tDouble sNumber, const tString& sFormatString) {
        tFormatResult out;
        const tString normalizedFull = NormalizeExcelNumberFormatToUs(sFormatString);
        const std::vector<tString> sections = SplitExcelFormatSections(normalizedFull);
        const tBool hasSections = sections.size() > 1;
        tString selectedFormat = normalizedFull;
        if (hasSections) {
            selectedFormat = SelectExcelFormatNumericSection(sections, sNumber);
            const tString tag = _ExtractColorTag(selectedFormat);
            if (!tag.empty()) {
                out.hasColor = true;
                out.colorName = tag;
            }
        } else {
            const tString tag = _ExtractColorTag(normalizedFull);
            if (!tag.empty()) {
                out.hasColor = true;
                out.colorName = tag;
            }
        }

        out.text = FormatNumber(sNumber, sFormatString);
        return out;
    }

    // Get Format Parser =======================================================
    std::shared_ptr<tExcelFormatParser> tNumberFormatter::GetFormatParser(const SkRoot::tString& sFormatString) {
        const tString norm = NormalizeExcelNumberFormatToUs(sFormatString);
        const auto secs = SplitExcelFormatSections(norm);
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            auto probe = std::make_shared<tExcelFormatParser>(secs[i]);
            if (probe->IsValid()) {
                return probe;
            }
        }
        return GetParser(norm);
    }

    // Clear Pool ==============================================================
    void tNumberFormatter::ClearPool() {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        m_FormatPool.clear();
    }

    // Get Pool Size ===========================================================
    SkRoot::tSize tNumberFormatter::GetPoolSize() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        return m_FormatPool.size();
    }

    // Is Format Cached ========================================================
    SkRoot::tBool tNumberFormatter::IsFormatCached(const SkRoot::tString& sFormatString) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        return m_FormatPool.find(NormalizeExcelNumberFormatToUs(sFormatString)) != m_FormatPool.end();
    }

    // Get Cached Formats ======================================================
    std::vector<SkRoot::tString> tNumberFormatter::GetCachedFormats() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        std::vector<SkRoot::tString> formats;
        formats.reserve(m_FormatPool.size());
        
        for (const auto& pair : m_FormatPool) {
            formats.push_back(pair.first);
        }
        
        return formats;
    }

    // Preload Common Formats ==================================================
    void tNumberFormatter::PreloadCommonFormats() {
        std::vector<SkRoot::tString> commonFormats = {
            "0",                    // Integer
            "0.00",                 // Decimal
            "0,00",                 // Decimal (European)
            "#,##0.00",             // US format with thousands
            "###.##0,00",           // European format with thousands
            "$#,##0.00",            // US currency
            "€#,##0.00",            // Euro currency
            "¥#,##0",               // Yen currency (no decimals)
            "[$¥-ja-JP]#,##0",      // Yen currency with Japanese locale
            "£#,##0.00",            // Pound sterling currency (2 decimals)
            "[$£-en-GB]#,##0.00",   // Pound sterling with UK locale
            // Accounting-style samples (alignment/fillers simplified)
            "[$¥-ja-JP]* #,##0_-;([$¥-ja-JP]* #,##0_-);([$¥-ja-JP]* \"-\"_-);(@_)",
            "[$£-en-GB]* #,##0.00_-;([$£-en-GB]* #,##0.00_-);([$£-en-GB]* \"-\"_-);(@_)",
            // Examples with colors/conditions (stripped in rendering for now)
            "[Red]-#,##0.00;[Green]#,##0.00;0;@",
            "0.00%",                // Percentage
            "0,00%",                // Percentage (European)
            "###,###,###.00",       // Large numbers
            "###.##0,00 €",         // European with Euro symbol
            "0.00E+00",             // Scientific notation
            "#,##0",                // Integer with thousands
            "###,00",               // European integer with thousands
            "0.0",                  // One decimal place
            "0.000",                // Three decimal places
            "0.0000"                // Four decimal places
        };
        
        for (const auto& format : commonFormats) {
            GetParser(format); // This will cache the format
        }
    }

} // End of namespace SkFormat
