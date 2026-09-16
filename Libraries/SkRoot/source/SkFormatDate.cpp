//=============================================================================
// SkFormatDate - Excel Date Format Parser Implementation
//=============================================================================

#include "../include/SkFormatDate.hpp"
#include "../include/SkLocale.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <locale>
#include "../include/SkApplication.hpp"
#include <cmath>

namespace SkRoot {

// Excel-serial helpers for section selection (same convention as tVariant / tClassDate).
static time_t LocalMidnightMkTime(tInt y, tInt mo, tInt d) {
    struct tm tms = {};
    tms.tm_year = y - 1900;
    tms.tm_mon = mo - 1;
    tms.tm_mday = d;
    tms.tm_hour = 0;
    tms.tm_min = 0;
    tms.tm_sec = 0;
#if defined(WIN32)
    tms.tm_isdst = -1;
#endif
    return mktime(&tms);
}

static time_t LocalDateTimeMkTime(tInt y, tInt mo, tInt d, tInt h, tInt mi, tInt se) {
    struct tm tms = {};
    tms.tm_year = y - 1900;
    tms.tm_mon = mo - 1;
    tms.tm_mday = d;
    tms.tm_hour = h;
    tms.tm_min = mi;
    tms.tm_sec = se;
#if defined(WIN32)
    tms.tm_isdst = -1;
#endif
    return mktime(&tms);
}

static tDouble ExcelSerialFromUnixSeconds(long long u) {
    const double day = std::floor(static_cast<double>(u) / 86400.0);
    const double frac = (static_cast<double>(u) - day * 86400.0) / 86400.0;
    return static_cast<tDouble>(day + 25569.0) + frac;
}

    // Thin wrapper: same result as tLocale::NormalizeExcelDateFormatToEnglish (canonical implementation in SkLocale.cpp)
    tString NormalizeExcelDateFormatToEnglish(const tString& sFormatString) {
        return tLocale::NormalizeExcelDateFormatToEnglish(sFormatString);
    }

    std::vector<tString> SplitExcelFormatSections(const tString& sFormatString) {
        std::vector<tString> sections;
        if (sFormatString.empty()) {
            return sections;
        }
        tString current;
        current.reserve(sFormatString.size());
        bool inQuotes = false;
        tInt bracketDepth = 0;
        for (tSize i = 0; i < sFormatString.size(); ++i) {
            const tChar c = sFormatString[i];
            if (c == '"') {
                inQuotes = !inQuotes;
                current.push_back(c);
                continue;
            }
            if (!inQuotes) {
                if (c == '[') {
                    ++bracketDepth;
                } else if (c == ']' && bracketDepth > 0) {
                    --bracketDepth;
                }
                if (c == ';' && bracketDepth == 0) {
                    sections.push_back(current);
                    current.clear();
                    continue;
                }
            }
            current.push_back(c);
        }
        sections.push_back(current);
        if (sections.size() > 4) {
            for (tSize i = 4; i < sections.size(); ++i) {
                sections[3].push_back(';');
                sections[3] += sections[i];
            }
            sections.resize(4);
        }
        return sections;
    }

    tSize SelectExcelFormatNumericSectionIndex(const std::vector<tString>& sections, tDouble sNumber) {
        const tSize n = sections.size();
        if (n <= 1) {
            return 0;
        }
        const bool pos = sNumber > 0;
        const bool neg = sNumber < 0;
        if (n == 2) {
            return neg ? 1 : 0;
        }
        if (n == 3) {
            if (pos) {
                return 0;
            }
            if (neg) {
                return 1;
            }
            return 2;
        }
        // Four sections: positive ; negative ; zero ; text — numeric uses first three only.
        if (pos) {
            return 0;
        }
        if (neg) {
            return 1;
        }
        return 2;
    }

    tString SelectExcelFormatNumericSection(const std::vector<tString>& sections, tDouble sNumber) {
        if (sections.empty()) {
            return "";
        }
        const tSize idx = SelectExcelFormatNumericSectionIndex(sections, sNumber);
        if (idx < sections.size() && !sections[idx].empty()) {
            return sections[idx];
        }
        if (!sections[0].empty()) {
            return sections[0];
        }
        for (const tString& s : sections) {
            if (!s.empty()) {
                return s;
            }
        }
        return "";
    }

    tString FormatExcelTextSection(const std::vector<tString>& sections, const tString& sText) {
        if (sections.size() < 4) {
            return sText;
        }
        const tString& sec = sections[3];
        if (sec.empty()) {
            return sText;
        }
        tString out;
        out.reserve(sec.size() + sText.size() * 2);
        for (tSize i = 0; i < sec.size(); ++i) {
            if (sec[i] == '@') {
                out += sText;
            } else {
                out.push_back(sec[i]);
            }
        }
        return out;
    }

    static tBool FormatCompactNumericExcelDate(tInt sYear, tInt sMonth, tInt sDay,
                                               const tString& sExcelFormat,
                                               tString& sOut) {
        const tString wNorm = NormalizeExcelDateFormatToEnglish(sExcelFormat);
        if (wNorm.empty()) {
            return false;
        }

        struct tCompactToken {
            tChar m_Kind;
            tInt m_Width;
        };
        std::vector<tCompactToken> wTokens;
        wTokens.reserve(4);

        for (tSize wI = 0; wI < wNorm.size();) {
            const tChar wC = static_cast<tChar>(std::tolower(static_cast<unsigned char>(wNorm[wI])));
            if (wC != 'y' && wC != 'm' && wC != 'd') {
                return false;
            }
            tSize wJ = wI + 1;
            while (wJ < wNorm.size()
                   && static_cast<tChar>(std::tolower(static_cast<unsigned char>(wNorm[wJ]))) == wC) {
                ++wJ;
            }
            const tInt wWidth = static_cast<tInt>(wJ - wI);
            // Compact fallback is for numeric date fields only. Longer m/d runs mean month/day
            // names in Excel and must stay with the regular parser.
            if ((wC == 'm' || wC == 'd') && wWidth > 2) {
                return false;
            }
            if (wC == 'y' && wWidth > 4) {
                return false;
            }
            wTokens.push_back({wC, wWidth});
            wI = wJ;
        }

        tBool wHasYear = false;
        tBool wHasMonth = false;
        tBool wHasDay = false;
        for (const auto& wToken : wTokens) {
            if (wToken.m_Kind == 'y') {
                wHasYear = true;
            } else if (wToken.m_Kind == 'm') {
                wHasMonth = true;
            } else if (wToken.m_Kind == 'd') {
                wHasDay = true;
            }
        }
        if (!wHasMonth || (!wHasYear && !wHasDay)) {
            return false;
        }

        tStringStream wStream;
        for (const auto& wToken : wTokens) {
            if (wToken.m_Kind == 'y') {
                if (wToken.m_Width <= 2) {
                    wStream << std::setw(wToken.m_Width) << std::setfill('0') << (sYear % 100);
                } else {
                    wStream << std::setw(wToken.m_Width) << std::setfill('0') << sYear;
                }
            } else if (wToken.m_Kind == 'm') {
                if (wToken.m_Width == 1) {
                    wStream << sMonth;
                } else {
                    wStream << std::setw(2) << std::setfill('0') << sMonth;
                }
            } else if (wToken.m_Kind == 'd') {
                if (wToken.m_Width == 1) {
                    wStream << sDay;
                } else {
                    wStream << std::setw(2) << std::setfill('0') << sDay;
                }
            }
        }
        sOut = wStream.str();
        return true;
    }

    // Constructor =============================================================
    tExcelDateParser::tExcelDateParser(const tString& sFormatString) 
        : m_FormatString(sFormatString)
        , m_NormalizedFormatString(NormalizeExcelDateFormatToEnglish(sFormatString))
        , m_Elements()
        , m_DayPart("")
        , m_MonthPart("")
        , m_YearPart("")
        , m_HourPart("")
        , m_MinutePart("")
        , m_SecondPart("")
        , m_AMPMPart("")
        , m_Separator("")
        , m_TimeSeparator("")
        , m_HasTime(false)
        , m_HasDate(false)
        , m_IsValid(false)
        , m_YearDigits(0)
        , m_MonthDigits(0)
        , m_DayDigits(0)
        , m_TitleCaseNames(false) {
        if (!sFormatString.empty()) {
            ParseFormatString();
        }
    }

    // Destructor ==============================================================
    tExcelDateParser::~tExcelDateParser() {
        Clear();
    }

    // Set Format String ======================================================
    void tExcelDateParser::SetFormatString(const tString& sFormatString) {
        m_FormatString = sFormatString;
        m_NormalizedFormatString = NormalizeExcelDateFormatToEnglish(sFormatString);
        Clear();
        if (!sFormatString.empty()) {
            ParseFormatString();
        }
    }

    // Get Format String ======================================================
    tString tExcelDateParser::GetFormatString() const {
        return m_FormatString;
    }

    // Get English format string ===============================================
    tString tExcelDateParser::GetEnglishFormatString() const {
        return m_NormalizedFormatString;
    }

    // Get Elements ============================================================
    const tVectorExcelDateElement& tExcelDateParser::GetElements() const {
        return m_Elements;
    }

    // Get Day Part ============================================================
    tString tExcelDateParser::GetDayPart() const {
        return m_DayPart;
    }

    // Get Month Part ==========================================================
    tString tExcelDateParser::GetMonthPart() const {
        return m_MonthPart;
    }

    // Get Year Part ===========================================================
    tString tExcelDateParser::GetYearPart() const {
        return m_YearPart;
    }

    // Get Hour Part ===========================================================
    tString tExcelDateParser::GetHourPart() const {
        return m_HourPart;
    }

    // Get Minute Part =========================================================
    tString tExcelDateParser::GetMinutePart() const {
        return m_MinutePart;
    }

    // Get Second Part =========================================================
    tString tExcelDateParser::GetSecondPart() const {
        return m_SecondPart;
    }

    // Get AM/PM Part ==========================================================
    tString tExcelDateParser::GetAMPMPart() const {
        return m_AMPMPart;
    }

    // Get Separator ===========================================================
    tString tExcelDateParser::GetSeparator() const {
        return m_Separator;
    }

    // Get Time Separator ======================================================
    tString tExcelDateParser::GetTimeSeparator() const {
        return m_TimeSeparator;
    }

    // Has Time ================================================================
    tBool tExcelDateParser::HasTime() const {
        return m_HasTime;
    }

    // Has Date ================================================================
    tBool tExcelDateParser::HasDate() const {
        return m_HasDate;
    }

    // Is Valid ================================================================
    tBool tExcelDateParser::IsValid() const {
        return m_IsValid;
    }

    // Get Year Digits =========================================================
    tInt tExcelDateParser::GetYearDigits() const {
        return m_YearDigits;
    }

    // Get Month Digits ========================================================
    tInt tExcelDateParser::GetMonthDigits() const {
        return m_MonthDigits;
    }

    // Get Day Digits ==========================================================
    tInt tExcelDateParser::GetDayDigits() const {
        return m_DayDigits;
    }

    // Clear ===================================================================
    void tExcelDateParser::Clear() {
        m_Elements.clear();
        m_DayPart.clear();
        m_MonthPart.clear();
        m_YearPart.clear();
        m_HourPart.clear();
        m_MinutePart.clear();
        m_SecondPart.clear();
        m_AMPMPart.clear();
        m_Separator.clear();
        m_TimeSeparator.clear();
        m_HasTime = false;
        m_HasDate = false;
        m_YearDigits = 0;
        m_MonthDigits = 0;
        m_DayDigits = 0;
        m_IsValid = false;
    }

    // Parse Format String ====================================================
    void tExcelDateParser::ParseFormatString() {
        if (m_FormatString.empty()) {
            m_IsValid = false;
            return;
        }

        // Split format string into elements (use English-normalized form: d/y/h/m/s)
        tString currentElement;
        tInt position = 0;
        
        for (tSize i = 0; i < m_NormalizedFormatString.length(); ++i) {
            tChar c = m_NormalizedFormatString[i];
            // Handle AM/PM or A/P as atomic tokens before splitting on '/'
            if (i + 4 < m_NormalizedFormatString.length()) {
                tString token5 = m_NormalizedFormatString.substr(i, 5);
                if (token5 == "AM/PM" || token5 == "am/pm") {
                    if (!currentElement.empty()) {
                        ParseElement(currentElement, position);
                        position += static_cast<SkRoot::tInt>(currentElement.length());
                        currentElement.clear();
                    }
                    ParseElement(token5, position);
                    position += 5;
                    i += 4; // advance past token
                    continue;
                }
            }
            if (i + 2 < m_NormalizedFormatString.length()) {
                tString token3 = m_NormalizedFormatString.substr(i, 3);
                if (token3 == "A/P" || token3 == "a/p") {
                    if (!currentElement.empty()) {
                        ParseElement(currentElement, position);
                        position += static_cast<SkRoot::tInt>(currentElement.length());
                        currentElement.clear();
                    }
                    ParseElement(token3, position);
                    position += 3;
                    i += 2;
                    continue;
                }
            }
            
            // Check for separators that split elements
            if (c == '/' || c == '-' || c == ' ' || c == ':' || c == '.' || c == ',') {
                if (!currentElement.empty()) {
                    ParseElement(currentElement, position);
                    position += static_cast<SkRoot::tInt>(currentElement.length());
                    currentElement.clear();
                }
                
                // Handle the separator itself
                tString separator(1, c);
                ParseElement(separator, position);
                position++;
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
        
        // Only consider valid if we have at least one date or time element
        m_IsValid = (m_HasDate || m_HasTime);
    }

    // Parse Element ===========================================================
    void tExcelDateParser::ParseElement(const tString& sElement, tInt sPosition) {
        tExcelDateElement element;
        element.m_Element = sElement;
        element.m_Position = sPosition;
        element.m_Length = static_cast<SkRoot::tInt>(sElement.length());
        element.m_Width = 0;
        element.m_IsOptional = false;
        element.m_IsLeadingZero = false;
        element.m_IsAbbreviated = false;
        element.m_IsFullName = false;
        element.m_Is24Hour = true;
        
        // Determine element type and properties
        if (IsDayPattern(sElement)) {
            element.m_Type = "day";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            element.m_IsLeadingZero = (sElement[0] == 'd');
            element.m_IsOptional = (sElement[0] == 'd' && sElement.length() == 1);
        } else if (IsHourPattern(sElement)) {
            element.m_Type = "hour";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            element.m_IsLeadingZero = (sElement[0] == 'h' || sElement[0] == 'H');
            element.m_Is24Hour = (sElement[0] == 'H');
        } else if (IsSecondPattern(sElement)) {
            element.m_Type = "second";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            element.m_IsLeadingZero = (sElement[0] == 's');
        } else if (IsAMPMPattern(sElement)) {
            element.m_Type = "ampm";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
        } else if (sElement.size() >= 1 && sElement[0] == 'm') {
            // Disambiguate between month and minute based on position relative to time elements
            bool isAfterTimeElement = false;
            
            // Check if this 'm' element comes after a time element (h, H, or :)
            for (const auto& existingElement : m_Elements) {
                if (existingElement.m_Position < sPosition && 
                    (existingElement.m_Type == "hour" || 
                     (existingElement.m_Element == ":" && existingElement.m_Type == "separator"))) {
                    isAfterTimeElement = true;
                    break;
                }
            }
            
            if (isAfterTimeElement && IsMinutePattern(sElement)) {
                element.m_Type = "minute";
                element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
                element.m_IsLeadingZero = true;
            } else if (IsMonthPattern(sElement)) {
                element.m_Type = "month";
                element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
                element.m_IsLeadingZero = true;
                element.m_IsAbbreviated = (sElement.length() == 3);
                element.m_IsFullName = (sElement.length() > 3);
            } else {
                // Fallback: treat as text
                element.m_Type = "text";
                element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            }
        } else if (IsMonthPattern(sElement)) {
            element.m_Type = "month";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            element.m_IsLeadingZero = (sElement[0] == 'm');
            element.m_IsAbbreviated = (sElement.length() == 3);
            element.m_IsFullName = (sElement.length() > 3);
        } else if (!IsYearPattern(sElement).empty()) {
            element.m_Type = "year";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
            element.m_IsLeadingZero = (sElement[0] == 'y');
        } else if (IsSeparator(sElement)) {
            element.m_Type = "separator";
            element.m_Width = 1;
        } else {
            element.m_Type = "text";
            element.m_Width = static_cast<SkRoot::tInt>(sElement.length());
        }
        
        m_Elements.push_back(element);
    }

    // Analyze Elements ========================================================
    void tExcelDateParser::AnalyzeElements() {
        tString dayPattern;
        tString monthPattern;
        tString yearPattern;
        tString hourPattern;
        tString minutePattern;
        tString secondPattern;
        tString ampmPattern;
        tString separator;
        tString timeSeparator;
        
        tBool foundDate = false;
        tBool foundTime = false;
        tBool inTimeSection = false;
        
        for (const auto& element : m_Elements) {
            if (element.m_Type == "day") {
                dayPattern += element.m_Element;
                foundDate = true;
            } else if (element.m_Type == "month") {
                // If we are in time section and token is up to 2 m's, treat as minute
                if (inTimeSection && element.m_Element.size() <= 2) {
                    minutePattern += element.m_Element;
                    foundTime = true;
                } else {
                    monthPattern += element.m_Element;
                    foundDate = true;
                }
            } else if (element.m_Type == "year") {
                yearPattern += element.m_Element;
                foundDate = true;
            } else if (element.m_Type == "hour") {
                hourPattern += element.m_Element;
                foundTime = true;
                inTimeSection = true;
            } else if (element.m_Type == "minute") {
                minutePattern += element.m_Element;
                foundTime = true;
                inTimeSection = true;
            } else if (element.m_Type == "second") {
                secondPattern += element.m_Element;
                foundTime = true;
                inTimeSection = true;
            } else if (element.m_Type == "ampm") {
                ampmPattern += element.m_Element;
                foundTime = true;
                inTimeSection = true;
            } else if (element.m_Type == "separator") {
                if (foundTime && timeSeparator.empty()) {
                    timeSeparator = element.m_Element;
                    if (element.m_Element == ":") inTimeSection = true;
                } else if (foundDate && separator.empty()) {
                    separator = element.m_Element;
                }
            }
        }
        
        m_DayPart = dayPattern;
        m_MonthPart = monthPattern;
        m_YearPart = yearPattern;
        m_HourPart = hourPattern;
        m_MinutePart = minutePattern;
        m_SecondPart = secondPattern;
        m_AMPMPart = ampmPattern;
        m_Separator = separator;
        m_TimeSeparator = timeSeparator;
        m_HasDate = foundDate;
        m_HasTime = foundTime;

        // Lowercase h/hh follow Excel: 12-hour clock only when AM/PM appears in the format.
        // Without AM/PM, use 24-hour (0–23) so e.g. "h:mm" and "h:mm;@" show 14:30 not 2:30.
        if (m_AMPMPart.empty()) {
            for (auto& element : m_Elements) {
                if (element.m_Type == "hour" && !element.m_Element.empty() && element.m_Element[0] == 'h') {
                    element.m_Is24Hour = true;
                }
            }
        }
        
        // Count digits
        m_YearDigits = static_cast<SkRoot::tInt>(yearPattern.length());
        m_MonthDigits = static_cast<SkRoot::tInt>(monthPattern.length());
        m_DayDigits = static_cast<SkRoot::tInt>(dayPattern.length());
    }

    // Is Day Pattern ==========================================================
    tBool tExcelDateParser::IsDayPattern(const tString& sElement) {
        if (sElement.empty()) return false;
        
        // Check for day patterns: d, dd, ddd, dddd
        if (sElement[0] == 'd') {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != 'd') return false;
            }
            return true;
        }
        return false;
    }

    // Is Month Pattern ========================================================
    tBool tExcelDateParser::IsMonthPattern(const tString& sElement) {
        if (sElement.empty()) return false;
        
        // Check for month patterns: m, mm, mmm, mmmm
        if (sElement[0] == 'm') {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != 'm') return false;
            }
            return true;
        }
        return false;
    }

    // Is Year Pattern =========================================================
    tString tExcelDateParser::IsYearPattern(const tString& sElement) {
        if (sElement.empty()) return "";
        
        // Check for year patterns: y, yy, yyy, yyyy
        if (sElement[0] == 'y') {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != 'y') return "";
            }
            return sElement;
        }
        return "";
    }

    // Is Hour Pattern =========================================================
    tBool tExcelDateParser::IsHourPattern(const tString& sElement) {
        if (sElement.empty()) return false;
        
        // Check for hour patterns: h, hh, H, HH
        if (sElement[0] == 'h' || sElement[0] == 'H') {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != sElement[0]) return false;
            }
            return true;
        }
        return false;
    }

    // Is Minute Pattern =======================================================
    tBool tExcelDateParser::IsMinutePattern(const tString& sElement) {
        if (sElement.empty()) return false;
        
        // Check for minute patterns: m, mm (but not month patterns)
        if (sElement[0] == 'm' && sElement.length() <= 2) {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != 'm') return false;
            }
            return true;
        }
        return false;
    }

    // Is Second Pattern =======================================================
    tBool tExcelDateParser::IsSecondPattern(const tString& sElement) {
        if (sElement.empty()) return false;
        
        // Check for second patterns: s, ss
        if (sElement[0] == 's') {
            for (tSize i = 1; i < sElement.length(); ++i) {
                if (sElement[i] != 's') return false;
            }
            return true;
        }
        return false;
    }

    // Is AM/PM Pattern ========================================================
    tBool tExcelDateParser::IsAMPMPattern(const tString& sElement) {
        return sElement == "AM/PM" || sElement == "A/P" || sElement == "am/pm" || sElement == "a/p";
    }

    // Is Separator ============================================================
    tBool tExcelDateParser::IsSeparator(const tString& sElement) {
        return sElement == "/" || sElement == "-" || sElement == " " || sElement == ":" || sElement == "." || sElement == ",";
    }

    // Count Consecutive =======================================================
    tInt tExcelDateParser::CountConsecutive(const tString& sFormat, tChar sChar, tInt sStart) {
        tInt count = 0;
        for (tSize i = sStart; i < sFormat.length() && sFormat[i] == sChar; ++i) {
            count++;
        }
        return count;
    }

    // Format Day ==============================================================
    tString tExcelDateParser::FormatDay(tInt sDay, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "day") {
            if (sElement.m_Width == 1) {
                // d - day without leading zero
                return std::to_string(sDay);
            } else if (sElement.m_Width == 2) {
                // dd - day with leading zero
                tStringStream ss;
                ss << std::setfill('0') << std::setw(2) << sDay;
                return ss.str();
            } else if (sElement.m_Width == 3) {
                // ddd - abbreviated day name
                return GetDayName(sDay, true);
            } else if (sElement.m_Width == 4) {
                // dddd - full day name
                return GetDayName(sDay, false);
            }
        }
        return "";
    }

    // Format Month ============================================================
    tString tExcelDateParser::FormatMonth(tInt sMonth, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "month") {
            if (sElement.m_Width == 1) {
                // m - month without leading zero
                return std::to_string(sMonth);
            } else if (sElement.m_Width == 2) {
                // mm - month with leading zero
                tStringStream ss;
                ss << std::setfill('0') << std::setw(2) << sMonth;
                return ss.str();
            } else if (sElement.m_Width == 3) {
                // mmm - abbreviated month name
                return GetMonthName(sMonth, true);
            } else if (sElement.m_Width == 4) {
                // mmmm - full month name
                return GetMonthName(sMonth, false);
            }
        }
        return "";
    }

    // Format Year =============================================================
    tString tExcelDateParser::FormatYear(tInt sYear, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "year") {
            if (sElement.m_Width == 1) {
                // y - year without leading zero
                return std::to_string(sYear);
            } else if (sElement.m_Width == 2) {
                // yy - 2-digit year
                return std::to_string(sYear % 100);
            } else if (sElement.m_Width == 3) {
                // yyy - 3-digit year
                tStringStream ss;
                ss << std::setfill('0') << std::setw(3) << (sYear % 1000);
                return ss.str();
            } else if (sElement.m_Width == 4) {
                // yyyy - 4-digit year
                tStringStream ss;
                ss << std::setfill('0') << std::setw(4) << sYear;
                return ss.str();
            }
        }
        return "";
    }

    // Format Hour =============================================================
    tString tExcelDateParser::FormatHour(tInt sHour, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "hour") {
            tInt displayHour = sHour;
            
            // Convert to 12-hour format if needed
            if (!sElement.m_Is24Hour) {
                if (sHour == 0) {
                    displayHour = 12;
                } else if (sHour > 12) {
                    displayHour = sHour - 12;
                }
            }
            
            if (sElement.m_Width == 1) {
                // h - hour without leading zero
                return std::to_string(displayHour);
            } else if (sElement.m_Width == 2) {
                // hh - hour with leading zero
                tStringStream ss;
                ss << std::setfill('0') << std::setw(2) << displayHour;
                return ss.str();
            }
        }
        return "";
    }

    // Format Minute ===========================================================
    tString tExcelDateParser::FormatMinute(tInt sMinute, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "minute") {
            if (sElement.m_Width == 1) {
                // m - minute without leading zero
                return std::to_string(sMinute);
            } else if (sElement.m_Width == 2) {
                // mm - minute with leading zero
                tStringStream ss;
                ss << std::setfill('0') << std::setw(2) << sMinute;
                return ss.str();
            }
        }
        return "";
    }

    // Format Second ===========================================================
    tString tExcelDateParser::FormatSecond(tInt sSecond, const tExcelDateElement& sElement) const {
        if (sElement.m_Type == "second") {
            if (sElement.m_Width == 1) {
                // s - second without leading zero
                return std::to_string(sSecond);
            } else if (sElement.m_Width == 2) {
                // ss - second with leading zero
                tStringStream ss;
                ss << std::setfill('0') << std::setw(2) << sSecond;
                return ss.str();
            }
        }
        return "";
    }

    // Format AM/PM ============================================================
    tString tExcelDateParser::FormatAMPM(tInt sHour) const {
        return (sHour < 12) ? "AM" : "PM";
    }

    // Get Month Name ==========================================================
    tString tExcelDateParser::GetMonthName(tInt sMonth, tBool sAbbreviated) const {
        tLocale* wLocale=tApplication::Instance()->Locale();
        if (sMonth < 1 || sMonth > 12 || !wLocale) {
            return "";
        }
        
        tString name = sAbbreviated ? wLocale->MonthStrAbbr(sMonth) : wLocale->MonthStr(sMonth);
        
        if (m_TitleCaseNames && !name.empty()) {
            name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        }
        
        return name;
    }

    // Get Day Name ============================================================
    tString tExcelDateParser::GetDayName(tInt sDayOfWeek, tBool sAbbreviated) const {
        tLocale* wLocale=tApplication::Instance()->Locale();
        if (sDayOfWeek < 0 || sDayOfWeek > 6 || !wLocale) {
            return "";
        }
        
        tString name = sAbbreviated ? wLocale->DayStrAbbr(sDayOfWeek) : wLocale->DayStr(sDayOfWeek);
        
        if (m_TitleCaseNames && !name.empty()) {
            name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        }
        
        return name;
    }

    // To SkRoot Format Type ===================================================
    tFormatStringType tExcelDateParser::ToSkRootFormatType() const {
        if (m_HasTime && m_HasDate) {
            // Check if it's a specific datetime format
            if (m_NormalizedFormatString.find("h:mm") != tString::npos) {
                return tFormatStringType::dateddmmyyyyhmm;
            } else if (m_NormalizedFormatString.find("h:mm:ss") != tString::npos) {
                return tFormatStringType::dateddmmyyyyhmmss;
            } else {
                return tFormatStringType::dateddmmyyyyhmm; // Default datetime
            }
        } else if (m_HasTime) {
            // Check specific time formats
            if (m_NormalizedFormatString.find("AM/PM") != tString::npos || m_NormalizedFormatString.find("am/pm") != tString::npos) {
                if (m_NormalizedFormatString.find("ss") != tString::npos) {
                    return tFormatStringType::datehmmssap;
                } else {
                    return tFormatStringType::datehmmap;
                }
            } else {
                if (m_NormalizedFormatString.find("ss") != tString::npos) {
                    return tFormatStringType::datehmmss;
                } else {
                    return tFormatStringType::datehmm;
                }
            }
        } else if (m_HasDate) {
            // Check specific date formats (normalized English tokens)
            if (m_NormalizedFormatString.find("mmmm") != tString::npos || m_NormalizedFormatString.find("dddd") != tString::npos) {
                return tFormatStringType::datelong;
            } else if (m_NormalizedFormatString.find("mmm") != tString::npos) {
                return tFormatStringType::datemmmyy;
            } else if (m_NormalizedFormatString.find("yy") != tString::npos && m_NormalizedFormatString.find("yyyy") == tString::npos) {
                return tFormatStringType::datestryy;
            } else if (m_NormalizedFormatString.find("yyyy") != tString::npos) {
                return tFormatStringType::datestryyyy;
            } else {
                return tFormatStringType::dateshort;
            }
        } else {
            
            return tFormatStringType::exceldate;
        }
    }

    // To SkRoot Format String =================================================
    tString tExcelDateParser::ToSkRootFormatString() const {
        tStringStream ss;
        
        if (m_HasDate) {
            ss << m_DayPart;
            if (!m_Separator.empty()) {
                ss << m_Separator;
            }
            ss << m_MonthPart;
            if (!m_Separator.empty()) {
                ss << m_Separator;
            }
            ss << m_YearPart;
        }
        
        if (m_HasTime) {
            if (m_HasDate) {
                ss << " ";
            }
            ss << m_HourPart;
            if (!m_TimeSeparator.empty()) {
                ss << m_TimeSeparator;
            }
            ss << m_MinutePart;
            if (!m_SecondPart.empty()) {
                if (!m_TimeSeparator.empty()) {
                    ss << m_TimeSeparator;
                }
                ss << m_SecondPart;
            }
            if (!m_AMPMPart.empty()) {
                ss << " " << m_AMPMPart;
            }
        }
        
        return ss.str();
    }

    // Format Date =============================================================
    tString tExcelDateParser::FormatDate(tInt sYear, tInt sMonth, tInt sDay) const {
        if (!m_IsValid || !m_HasDate) {
            return "";
        }
        
        tStringStream result;
        
        // Helper: compute day of week (0=Sunday..6=Saturday)
        auto computeDayOfWeek = [](int year, int month, int day) -> int {
            // Zeller's congruence (for Gregorian calendar)
            int m = month;
            int y = year;
            if (m < 3) {
                m += 12;
                y -= 1;
            }
            int K = y % 100;
            int J = y / 100;
            int h = (day + (13 * (m + 1)) / 5 + K + (K / 4) + (J / 4) + 5 * J) % 7;
            // h: 0=Saturday, 1=Sunday, 2=Monday, ...
            int dow = (h + 6) % 7; // convert to 0=Sunday..6=Saturday
            return dow;
        };
        int dayOfWeek = computeDayOfWeek(static_cast<int>(sYear), static_cast<int>(sMonth), static_cast<int>(sDay));

        for (const auto& element : m_Elements) {
            if (element.m_Type == "day") {
                if (element.m_Width <= 2) {
                    result << FormatDay(sDay, element);
                } else {
                    // ddd / dddd -> use weekday name
                    result << GetDayName(dayOfWeek, element.m_Width == 3);
                }
            } else if (element.m_Type == "month") {
                result << FormatMonth(sMonth, element);
            } else if (element.m_Type == "year") {
                result << FormatYear(sYear, element);
            } else if (element.m_Type == "separator") {
                result << element.m_Element;
            } else if (element.m_Type == "text") {
                result << element.m_Element;
            }
        }
        
        return result.str();
    }

    // Format Time =============================================================
    tString tExcelDateParser::FormatTime(tInt sHour, tInt sMinute, tInt sSecond) const {
        if (!m_IsValid || !m_HasTime) {
            return "";
        }
        
        tStringStream result;
        
        for (const auto& element : m_Elements) {
            if (element.m_Type == "hour") {
                result << FormatHour(sHour, element);
            } else if (element.m_Type == "minute") {
                result << FormatMinute(sMinute, element);
            } else if (element.m_Type == "second") {
                result << FormatSecond(sSecond, element);
            } else if (element.m_Type == "ampm") {
                result << FormatAMPM(sHour);
            } else if (element.m_Type == "separator") {
                result << element.m_Element;
            } else if (element.m_Type == "text") {
                result << element.m_Element;
            }
        }
        
        return result.str();
    }

    // Format DateTime =========================================================
    tString tExcelDateParser::FormatDateTime(tInt sYear, tInt sMonth, tInt sDay, 
                                            tInt sHour, tInt sMinute, tInt sSecond) const {
        if (!m_IsValid) {
            return "";
        }
        
        tStringStream result;
        
        // Helper: compute day of week (0=Sunday..6=Saturday)
        auto computeDayOfWeek = [](int year, int month, int day) -> int {
            int m = month;
            int y = year;
            if (m < 3) {
                m += 12;
                y -= 1;
            }
            int K = y % 100;
            int J = y / 100;
            int h = (day + (13 * (m + 1)) / 5 + K + (K / 4) + (J / 4) + 5 * J) % 7;
            int dow = (h + 6) % 7; // 0=Sunday..6=Saturday
            return dow;
        };
        int dayOfWeek = computeDayOfWeek(static_cast<int>(sYear), static_cast<int>(sMonth), static_cast<int>(sDay));

        for (const auto& element : m_Elements) {
            if (element.m_Type == "day") {
                if (element.m_Width <= 2) {
                    result << FormatDay(sDay, element);
                } else {
                    result << GetDayName(dayOfWeek, element.m_Width == 3);
                }
            } else if (element.m_Type == "month") {
                result << FormatMonth(sMonth, element);
            } else if (element.m_Type == "year") {
                result << FormatYear(sYear, element);
            } else if (element.m_Type == "hour") {
                result << FormatHour(sHour, element);
            } else if (element.m_Type == "minute") {
                result << FormatMinute(sMinute, element);
            } else if (element.m_Type == "second") {
                result << FormatSecond(sSecond, element);
            } else if (element.m_Type == "ampm") {
                result << FormatAMPM(sHour);
            } else if (element.m_Type == "separator") {
                result << element.m_Element;
            } else if (element.m_Type == "text") {
                result << element.m_Element;
            }
        }
        
        return result.str();
    }

    // Format Date from time_t =================================================
    tString tExcelDateParser::FormatDate(time_t sTimeT) const {
        tm timeInfo;
    #if !defined(WIN32)
        if (localtime_r(&sTimeT, &timeInfo) != nullptr) {
            return FormatDate(timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
        }
    #else
        if (localtime_s(&timeInfo, &sTimeT) == 0) {
            return FormatDate(timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
        }
    #endif
        return "";
    }

    // Format Date from tm =====================================================
    tString tExcelDateParser::FormatDate(const tm* sTm) const {
        if (sTm) {
            return FormatDate(sTm->tm_year + 1900, sTm->tm_mon + 1, sTm->tm_mday);
        }
        return "";
    }

    // Get Description =========================================================
    tString tExcelDateParser::GetDescription() const {
        tStringStream ss;
        ss << "Excel Date Format: " << m_FormatString << "\n";
        ss << "Day Part: " << m_DayPart << "\n";
        ss << "Month Part: " << m_MonthPart << "\n";
        ss << "Year Part: " << m_YearPart << "\n";
        ss << "Hour Part: " << m_HourPart << "\n";
        ss << "Minute Part: " << m_MinutePart << "\n";
        ss << "Second Part: " << m_SecondPart << "\n";
        ss << "AM/PM Part: " << m_AMPMPart << "\n";
        ss << "Separator: '" << m_Separator << "'\n";
        ss << "Time Separator: '" << m_TimeSeparator << "'\n";
        ss << "Has Date: " << (m_HasDate ? "Yes" : "No") << "\n";
        ss << "Has Time: " << (m_HasTime ? "Yes" : "No") << "\n";
        ss << "Year Digits: " << m_YearDigits << "\n";
        ss << "Month Digits: " << m_MonthDigits << "\n";
        ss << "Day Digits: " << m_DayDigits << "\n";
        ss << "Valid: " << (m_IsValid ? "Yes" : "No") << "\n";
        return ss.str();
    }

    // Print Elements ==========================================================
    void tExcelDateParser::PrintElements() const {
        cout << "Excel Date Format Elements for: " << m_FormatString << endl;
        cout << "================================================" << endl;
        
        for (const auto& element : m_Elements) {
            cout << "Element: '" << element.m_Element << "'" << endl;
            cout << "  Type: " << element.m_Type << endl;
            cout << "  Position: " << element.m_Position << endl;
            cout << "  Length: " << element.m_Length << endl;
            cout << "  Width: " << element.m_Width << endl;
            cout << "  Optional: " << (element.m_IsOptional ? "Yes" : "No") << endl;
            cout << "  Leading Zero: " << (element.m_IsLeadingZero ? "Yes" : "No") << endl;
            cout << "  Abbreviated: " << (element.m_IsAbbreviated ? "Yes" : "No") << endl;
            cout << "  Full Name: " << (element.m_IsFullName ? "Yes" : "No") << endl;
            cout << "  Is 24 Hour: " << (element.m_Is24Hour ? "Yes" : "No") << endl;
            cout << "---" << endl;
        }
    }

    // Free-function Excel date utilities (Parse / IsValid / Convert / Format*) ===========

    // Parse Excel Date Format =================================================
    tVectorExcelDateElement ParseExcelDateFormat(const tString& sFormatString) {
        const auto secs = SplitExcelFormatSections(sFormatString);
        const tString& probe = secs.empty() ? sFormatString : secs[0];
        tExcelDateParser parser(probe);
        return parser.GetElements();
    }

    // Is Valid Excel Date Format ==============================================
    tBool IsValidExcelDateFormat(const tString& sFormatString) {
        const auto secs = SplitExcelFormatSections(sFormatString);
        if (secs.empty()) {
            return false;
        }
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            tExcelDateParser parser(secs[i]);
            if (parser.IsValid()) {
                return true;
            }
            tString wCompactProbe;
            if (FormatCompactNumericExcelDate(2001, 2, 3, secs[i], wCompactProbe)) {
                return true;
            }
        }
        return false;
    }

    // Convert To SkRoot Date Format ===========================================
    tString ConvertToSkRootDateFormat(const tString& sExcelFormat) {
        const auto secs = SplitExcelFormatSections(sExcelFormat);
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            tExcelDateParser parser(secs[i]);
            if (parser.IsValid()) {
                return parser.ToSkRootFormatString();
            }
        }
        tExcelDateParser parser(sExcelFormat);
        return parser.ToSkRootFormatString();
    }

    // Format With Excel Date Format ===========================================
    tString FormatWithExcelDateFormat(tInt sYear, tInt sMonth, tInt sDay, const tString& sExcelFormat) {
        const auto secs = SplitExcelFormatSections(sExcelFormat);
        if (secs.empty()) {
            return "";
        }
        const time_t mt = LocalMidnightMkTime(sYear, sMonth, sDay);
        const tDouble serial = ExcelSerialFromUnixSeconds(static_cast<long long>(mt));
        const tString sel = SelectExcelFormatNumericSection(secs, serial);
        tExcelDateParser parser(sel);
        if (!parser.IsValid()) {
            tString wCompact;
            if (FormatCompactNumericExcelDate(sYear, sMonth, sDay, sel, wCompact)) {
                return wCompact;
            }
            return "";
        }
        return parser.FormatDate(sYear, sMonth, sDay);
    }

    // Format With Excel Time Format ===========================================
    tString FormatWithExcelTimeFormat(tInt sHour, tInt sMinute, tInt sSecond, const tString& sExcelFormat) {
        const auto secs = SplitExcelFormatSections(sExcelFormat);
        if (secs.empty()) {
            return "";
        }
        const tDouble serial = static_cast<tDouble>(sHour * 3600 + sMinute * 60 + sSecond) / 86400.0;
        const tString sel = SelectExcelFormatNumericSection(secs, serial);
        tExcelDateParser parser(sel);
        if (!parser.IsValid()) {
            return "";
        }
        return parser.FormatTime(sHour, sMinute, sSecond);
    }

    // Format With Excel DateTime Format =======================================
    tString FormatWithExcelDateTimeFormat(tInt sYear, tInt sMonth, tInt sDay, 
                                         tInt sHour, tInt sMinute, tInt sSecond, 
                                         const tString& sExcelFormat) {
        const auto secs = SplitExcelFormatSections(sExcelFormat);
        if (secs.empty()) {
            return "";
        }
        const time_t tt = LocalDateTimeMkTime(sYear, sMonth, sDay, sHour, sMinute, sSecond);
        const tDouble serial = ExcelSerialFromUnixSeconds(static_cast<long long>(tt));
        const tString sel = SelectExcelFormatNumericSection(secs, serial);
        tExcelDateParser parser(sel);
        if (!parser.IsValid()) {
            return "";
        }
        return parser.FormatDateTime(sYear, sMonth, sDay, sHour, sMinute, sSecond);
    }

    // Date Formatter with Pool Implementation =================================

    // Constructor =============================================================
    tDateFormatter::tDateFormatter() {
        // Initialize with empty pool
    }

    // Destructor ==============================================================
    tDateFormatter::~tDateFormatter() {
        ClearPool();
    }

    // Get Parser ==============================================================
    std::shared_ptr<tExcelDateParser> tDateFormatter::GetParser(const SkRoot::tString& sFormatString) {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        
        // Check if format is already cached
        auto it = m_FormatPool.find(sFormatString);
        if (it != m_FormatPool.end()) {
            return it->second;
        }
        
        // Create new parser and cache it
        auto parser = std::make_shared<tExcelDateParser>(sFormatString);
        m_FormatPool[sFormatString] = parser;
        return parser;
    }

    // Format Date =============================================================
    SkRoot::tString tDateFormatter::FormatDate(tInt sYear, tInt sMonth, tInt sDay, const SkRoot::tString& sFormatString) {
        return FormatWithExcelDateFormat(sYear, sMonth, sDay, sFormatString);
    }

    // Format Time =============================================================
    SkRoot::tString tDateFormatter::FormatTime(tInt sHour, tInt sMinute, tInt sSecond, const SkRoot::tString& sFormatString) {
        return FormatWithExcelTimeFormat(sHour, sMinute, sSecond, sFormatString);
    }

    // Format DateTime =========================================================
    SkRoot::tString tDateFormatter::FormatDateTime(tInt sYear, tInt sMonth, tInt sDay, 
                                                   tInt sHour, tInt sMinute, tInt sSecond, 
                                                   const SkRoot::tString& sFormatString) {
        return FormatWithExcelDateTimeFormat(sYear, sMonth, sDay, sHour, sMinute, sSecond, sFormatString);
    }

    // Format Date from time_t =================================================
    SkRoot::tString tDateFormatter::FormatDate(time_t sTimeT, const SkRoot::tString& sFormatString) {
        tm timeInfo;
    #if !defined(WIN32)
        if (localtime_r(&sTimeT, &timeInfo) == nullptr) {
            return "";
        }
    #else
        if (localtime_s(&timeInfo, &sTimeT) != 0) {
            return "";
        }
    #endif
        return FormatWithExcelDateFormat(timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, sFormatString);
    }

    // Format DateTime from time_t ============================================
    SkRoot::tString tDateFormatter::FormatDateTime(time_t sTimeT, const SkRoot::tString& sFormatString) {
        tm timeInfo;
    #if !defined(WIN32)
        if (localtime_r(&sTimeT, &timeInfo) == nullptr) {
            return "";
        }
    #else
        if (localtime_s(&timeInfo, &sTimeT) != 0) {
            return "";
        }
    #endif
        return FormatWithExcelDateTimeFormat(timeInfo.tm_year + 1900,
                                             timeInfo.tm_mon + 1,
                                             timeInfo.tm_mday,
                                             timeInfo.tm_hour,
                                             timeInfo.tm_min,
                                             timeInfo.tm_sec,
                                             sFormatString);
    }

    // Format Dates ============================================================
    std::vector<SkRoot::tString> tDateFormatter::FormatDates(const std::vector<std::tuple<tInt, tInt, tInt>>& sDates, const SkRoot::tString& sFormatString) {
        std::vector<SkRoot::tString> results;
        results.reserve(sDates.size());
        for (const auto& date : sDates) {
            tInt year = 0;
            tInt month = 0;
            tInt day = 0;
            std::tie(year, month, day) = date;
            results.push_back(FormatWithExcelDateFormat(year, month, day, sFormatString));
        }
        return results;
    }

    // Get Format Parser =======================================================
    std::shared_ptr<tExcelDateParser> tDateFormatter::GetFormatParser(const SkRoot::tString& sFormatString) {
        const auto secs = SplitExcelFormatSections(sFormatString);
        for (tSize i = 0; i < secs.size() && i < 3; ++i) {
            if (secs[i].empty()) {
                continue;
            }
            auto probe = std::make_shared<tExcelDateParser>(secs[i]);
            if (probe->IsValid()) {
                return probe;
            }
        }
        return GetParser(sFormatString);
    }

    // Clear Pool ==============================================================
    void tDateFormatter::ClearPool() {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        m_FormatPool.clear();
    }

    // Get Pool Size ===========================================================
    SkRoot::tSize tDateFormatter::GetPoolSize() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        return m_FormatPool.size();
    }

    // Is Format Cached ========================================================
    SkRoot::tBool tDateFormatter::IsFormatCached(const SkRoot::tString& sFormatString) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        return m_FormatPool.find(sFormatString) != m_FormatPool.end();
    }

    // Get Cached Formats ======================================================
    std::vector<SkRoot::tString> tDateFormatter::GetCachedFormats() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_PoolMutex));
        std::vector<SkRoot::tString> formats;
        formats.reserve(m_FormatPool.size());
        
        for (const auto& pair : m_FormatPool) {
            formats.push_back(pair.first);
        }
        
        return formats;
    }

    // Preload Common Formats ==================================================
    void tDateFormatter::PreloadCommonFormats() {
        std::vector<SkRoot::tString> commonFormats = {
            "dd/mm/yyyy",           // European date format
            "mm/dd/yyyy",           // US date format
            "yyyy-mm-dd",           // ISO date format
            "dd-mmm-yyyy",          // European with month name
            "mmm-yy",               // Short month-year
            "dddd, mmmm dd, yyyy",  // Full date format
            "dd/mm/yy",             // Short European date
            "mm/dd/yy",             // Short US date
            "dd-mmm-yy",            // European short with month
            "mmm dd, yyyy",         // Month day, year
            "h:mm AM/PM",           // 12-hour time
            "hh:mm:ss",             // 24-hour time
            "h:mm:ss AM/PM",        // 12-hour time with seconds
            "dd/mm/yyyy h:mm",      // Date and time
            "mm/dd/yyyy hh:mm:ss",  // US date and time
            "yyyy-mm-dd hh:mm:ss",  // ISO datetime
            "dddd",                 // Full day of week
            "ddd",                  // Abbreviated day of week
            "mmmm",                 // Full month name
            "mmm",                  // Abbreviated month
            "yy",                   // Two-digit year
            "yyyy",                 // Four-digit year
            "d",                    // Day without leading zero
            "dd",                   // Day with leading zero
            "m",                    // Month without leading zero
            "mm",                   // Month with leading zero
            "h",                    // Hour without leading zero
            "hh",                   // Hour with leading zero
            "s",                    // Second without leading zero
            "ss"                    // Second with leading zero
        };
        
        for (const auto& format : commonFormats) {
            GetParser(format); // This will cache the format
        }
    }

    // Set Title Case ===========================================================
    void tExcelDateParser::SetTitleCaseNames(SkRoot::tBool sEnable) {
        m_TitleCaseNames = sEnable;
    }

    // Get Title Case ===========================================================
    SkRoot::tBool tExcelDateParser::GetTitleCaseNames() const {
        return m_TitleCaseNames;
    }

} // End of namespace SkRoot
