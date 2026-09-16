//=============================================================================
// SkFormatDate - Excel Date Format Parser
/**
* @page SkFormatDate
* @par 
* @par Parse Excel date formats like dd/mm/yyyy, mmm-yy, dddd to extract formatting elements
*/
//=============================================================================
#ifndef SkFormatDate_hpp
#define SkFormatDate_hpp

#include "SkTypes.hpp"
#include "SkFormatString.hpp"
#include "SkLocale.hpp"

#include <ctime>
#include <chrono>

namespace SkRoot {
    // Language Support ========================================================
    // Note: Language support is now handled through tLocale class

    // Excel Date Format Elements ==============================================
    struct tExcelDateElement {
        tString m_Element;       // The element string (e.g., "dd", "mm", "yyyy", "mmm")
        tString m_Type;          // Type: "day", "month", "year", "hour", "minute", "second", "separator", "text"
        tInt    m_Position;      // Position in the format string
        tInt    m_Length;        // Length of the element
        tInt    m_Width;         // Width of the element (e.g., 2 for "dd", 4 for "yyyy")
        tBool   m_IsOptional;    // Whether this element is optional
        tBool   m_IsLeadingZero; // Whether to show leading zeros
        tBool   m_IsAbbreviated; // Whether to show abbreviated form
        tBool   m_IsFullName;    // Whether to show full name
        tBool   m_Is24Hour;      // Whether to use 24-hour format
    };

    typedef std::vector<tExcelDateElement> tVectorExcelDateElement;

    // Excel Date Format Parser ================================================
    class tExcelDateParser : public tClass {
    private:
        tString m_FormatString;                    // Original format string (locale-specific tokens may appear)
        tString m_NormalizedFormatString;          // English Excel tokens after NormalizeExcelDateFormatToEnglish()
        tVectorExcelDateElement m_Elements;      // Parsed elements
        tString m_DayPart;                        // Day part pattern
        tString m_MonthPart;                      // Month part pattern
        tString m_YearPart;                       // Year part pattern
        tString m_HourPart;                       // Hour part pattern
        tString m_MinutePart;                     // Minute part pattern
        tString m_SecondPart;                     // Second part pattern
        tString m_AMPMPart;                       // AM/PM part pattern
        tString m_Separator;                      // Date separator
        tString m_TimeSeparator;                  // Time separator
        tBool   m_HasTime;                        // Whether format includes time
        tBool   m_HasDate;                        // Whether format includes date
        tBool   m_IsValid;                        // Whether format is valid
        tInt    m_YearDigits;                     // Number of year digits
        tInt    m_MonthDigits;                    // Number of month digits
        tInt    m_DayDigits;                      // Number of day digits
     
        tBool  m_TitleCaseNames;                   // Capitalize first letter of names

        // Internal parsing methods
        void ParseFormatString();
        void ParseElement(const tString& sElement, tInt sPosition);
        void AnalyzeElements();
        tString ExtractPattern(const tString& sElement);
        tBool IsDayPattern(const tString& sElement);
        tBool IsMonthPattern(const tString& sElement);
        tString IsYearPattern(const tString& sElement);
        tBool IsHourPattern(const tString& sElement);
        tBool IsMinutePattern(const tString& sElement);
        tBool IsSecondPattern(const tString& sElement);
        tBool IsAMPMPattern(const tString& sElement);
        tBool IsSeparator(const tString& sElement);
        tInt CountConsecutive(const tString& sFormat, tChar sChar, tInt sStart);
        
        // Formatting helper methods
        tString FormatDay(tInt sDay, const tExcelDateElement& sElement) const;
        tString FormatMonth(tInt sMonth, const tExcelDateElement& sElement) const;
        tString FormatYear(tInt sYear, const tExcelDateElement& sElement) const;
        tString FormatHour(tInt sHour, const tExcelDateElement& sElement) const;
        tString FormatMinute(tInt sMinute, const tExcelDateElement& sElement) const;
        tString FormatSecond(tInt sSecond, const tExcelDateElement& sElement) const;
        tString FormatAMPM(tInt sHour) const;

    public:
        /// @brief Constructor
        /// @param[in] sFormatString tString - Excel format string like "dd/mm/yyyy"
        tExcelDateParser(const tString& sFormatString = "");

        /// @brief Destructor
        ~tExcelDateParser();

        /// @brief Set the format string to parse
        /// @param[in] sFormatString tString
        void SetFormatString(const tString& sFormatString);

        /// @brief Get the original format string
        /// @return tString
        tString GetFormatString() const;

        /// @brief Get the English-normalized format string (same as NormalizeExcelDateFormatToEnglish(GetFormatString()))
        /// @return tString
        tString GetEnglishFormatString() const;

        /// @brief Get all parsed elements
        /// @return const tVectorExcelDateElement&
        const tVectorExcelDateElement& GetElements() const;

        /// @brief Get the day part pattern
        /// @return tString
        tString GetDayPart() const;

        /// @brief Get the month part pattern
        /// @return tString
        tString GetMonthPart() const;

        /// @brief Get the year part pattern
        /// @return tString
        tString GetYearPart() const;

        /// @brief Get the hour part pattern
        /// @return tString
        tString GetHourPart() const;

        /// @brief Get the minute part pattern
        /// @return tString
        tString GetMinutePart() const;

        /// @brief Get the second part pattern
        /// @return tString
        tString GetSecondPart() const;

        /// @brief Get the AM/PM part pattern
        /// @return tString
        tString GetAMPMPart() const;

        /// @brief Get the date separator
        /// @return tString
        tString GetSeparator() const;

        /// @brief Get the time separator
        /// @return tString
        tString GetTimeSeparator() const;

        /// @brief Check if format includes time
        /// @return tBool
        tBool HasTime() const;

        /// @brief Check if format includes date
        /// @return tBool
        tBool HasDate() const;

        /// @brief Check if format is valid
        /// @return tBool
        tBool IsValid() const;

        /// @brief Get number of year digits
        /// @return tInt
        tInt GetYearDigits() const;

        /// @brief Get number of month digits
        /// @return tInt
        tInt GetMonthDigits() const;

        /// @brief Get number of day digits
        /// @return tInt
        tInt GetDayDigits() const;

        /// @brief Clear all data
        void Clear();

        /// @brief Convert to SkRoot format string type
        /// @return tFormatStringType
        tFormatStringType ToSkRootFormatType() const;

        /// @brief Get SkRoot format string
        /// @return tString
        tString ToSkRootFormatString() const;

        /// @brief Format a date using the parsed format
        /// @param[in] sYear tInt - Year
        /// @param[in] sMonth tInt - Month (1-12)
        /// @param[in] sDay tInt - Day (1-31)
        /// @return tString - Formatted date string
        tString FormatDate(tInt sYear, tInt sMonth, tInt sDay) const;

        /// @brief Format a time using the parsed format
        /// @param[in] sHour tInt - Hour (0-23)
        /// @param[in] sMinute tInt - Minute (0-59)
        /// @param[in] sSecond tInt - Second (0-59)
        /// @return tString - Formatted time string
        tString FormatTime(tInt sHour, tInt sMinute, tInt sSecond) const;

        /// @brief Format a datetime using the parsed format
        /// @param[in] sYear tInt - Year
        /// @param[in] sMonth tInt - Month (1-12)
        /// @param[in] sDay tInt - Day (1-31)
        /// @param[in] sHour tInt - Hour (0-23)
        /// @param[in] sMinute tInt - Minute (0-59)
        /// @param[in] sSecond tInt - Second (0-59)
        /// @return tString - Formatted datetime string
        tString FormatDateTime(tInt sYear, tInt sMonth, tInt sDay, 
                                      tInt sHour, tInt sMinute, tInt sSecond) const;

        /// @brief Format a date from time_t
        /// @param[in] sTimeT time_t - Time value
        /// @return tString - Formatted date string
        tString FormatDate(time_t sTimeT) const;

        /// @brief Format a date from tm struct
        /// @param[in] sTm const tm* - Time structure
        /// @return tString - Formatted date string
        tString FormatDate(const tm* sTm) const;

        /// @brief Get format description
        /// @return tString
        tString GetDescription() const;

        /// @brief Debug: Print all elements
        void PrintElements() const;
        
        /// @brief Enable/disable title case for day/month names (capitalize first letter)
        /// @param[in] sEnable tBool - true to enable, false to disable
        void SetTitleCaseNames(tBool sEnable);

        /// @brief Get title case setting
        /// @return tBool - true if title case is enabled
        tBool GetTitleCaseNames() const;
        
        /// @brief Get month name in current language
        /// @param[in] sMonth tInt - Month (1-12)
        /// @param[in] sAbbreviated tBool - Whether to return abbreviated form
        /// @return tString - Month name
        tString GetMonthName(tInt sMonth, tBool sAbbreviated) const;
        
        /// @brief Get day name in current language
        /// @param[in] sDayOfWeek tInt - Day of week (0=Sunday, 1=Monday, etc.)
        /// @param[in] sAbbreviated tBool - Whether to return abbreviated form
        /// @return tString - Day name
        tString GetDayName(tInt sDayOfWeek, tBool sAbbreviated) const;
    };

    // Date Formatter with Pool ================================================
    class tDateFormatter : public tClass {
    private:
        std::unordered_map<tString, std::shared_ptr<tExcelDateParser>> m_FormatPool;
        std::mutex m_PoolMutex;
        
        // Get or create parser from pool
        std::shared_ptr<tExcelDateParser> GetParser(const tString& sFormatString);
        
    public:
        /// @brief Constructor
        tDateFormatter();
        
        /// @brief Destructor
        ~tDateFormatter();
        
        /// @brief Format a date with Excel format (cached)
        /// @param[in] sYear tInt - Year
        /// @param[in] sMonth tInt - Month (1-12)
        /// @param[in] sDay tInt - Day (1-31)
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted date string
        tString FormatDate(tInt sYear, tInt sMonth, tInt sDay, const tString& sFormatString);
        
        /// @brief Format a time with Excel format (cached)
        /// @param[in] sHour tInt - Hour (0-23)
        /// @param[in] sMinute tInt - Minute (0-59)
        /// @param[in] sSecond tInt - Second (0-59)
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted time string
        tString FormatTime(tInt sHour, tInt sMinute, tInt sSecond, const tString& sFormatString);
        
        /// @brief Format a datetime with Excel format (cached)
        /// @param[in] sYear tInt - Year
        /// @param[in] sMonth tInt - Month (1-12)
        /// @param[in] sDay tInt - Day (1-31)
        /// @param[in] sHour tInt - Hour (0-23)
        /// @param[in] sMinute tInt - Minute (0-59)
        /// @param[in] sSecond tInt - Second (0-59)
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted datetime string
        tString FormatDateTime(tInt sYear, tInt sMonth, tInt sDay, 
                                      tInt sHour, tInt sMinute, tInt sSecond, 
                                      const tString& sFormatString);
        
        /// @brief Format a datetime from time_t with Excel format (cached)
        /// @param[in] sTimeT time_t - Time value
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted datetime string
        tString FormatDateTime(time_t sTimeT, const tString& sFormatString);
        
        /// @brief Format a date from time_t with Excel format (cached)
        /// @param[in] sTimeT time_t - Time value
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted date string
        tString FormatDate(time_t sTimeT, const tString& sFormatString);
        
        /// @brief Format multiple dates with same format (optimized)
        /// @param[in] sDates std::vector<std::tuple<tInt, tInt, tInt>> - Dates to format (year, month, day)
        /// @param[in] sFormatString tString - Excel format string
        /// @return std::vector<tString> - Formatted date strings
        std::vector<tString> FormatDates(const std::vector<std::tuple<tInt, tInt, tInt>>& sDates, const tString& sFormatString);
        
        /// @brief Get format parser from pool (for advanced usage)
        /// @param[in] sFormatString tString - Excel format string
        /// @return std::shared_ptr<tExcelDateParser> - Parser from pool
        std::shared_ptr<tExcelDateParser> GetFormatParser(const tString& sFormatString);
        
        /// @brief Clear the format pool
        void ClearPool();
        
        /// @brief Get pool size (number of cached formats)
        /// @return tSize - Number of cached formats
        tSize GetPoolSize() const;
        
        /// @brief Check if format is cached
        /// @param[in] sFormatString tString - Excel format string
        /// @return tBool - True if format is cached
        tBool IsFormatCached(const tString& sFormatString) const;
        
        /// @brief Get all cached format strings
        /// @return std::vector<tString> - List of cached format strings
        std::vector<tString> GetCachedFormats() const;
        
        /// @brief Preload common date formats
        void PreloadCommonFormats();
    };

    // Utility Functions ======================================================

    /// Split Excel format code into up to four sections (positive;negative;zero;text).
    /// Semicolons inside double quotes or square brackets are not separators (e.g. [h]:mm:ss).
    std::vector<tString> SplitExcelFormatSections(const tString& sFormatString);

    /// Excel section index (0..3) for numeric serial values (positive / negative / zero).
    tSize SelectExcelFormatNumericSectionIndex(const std::vector<tString>& sections, tDouble sNumber);

    /// Resolved section substring with empty-section fallback (matches first non-empty, then "").
    tString SelectExcelFormatNumericSection(const std::vector<tString>& sections, tDouble sNumber);

    /// Apply optional fourth section for text: each '@' is replaced by sText (Excel TEXT-style).
    tString FormatExcelTextSection(const std::vector<tString>& sections, const tString& sText);
    
    /// @brief Parse Excel date format string and return elements
    /// @param[in] sFormatString tString
    /// @return tVectorExcelDateElement
    tVectorExcelDateElement ParseExcelDateFormat(const tString& sFormatString);

    /// @brief Map locale-specific Excel date codes to English (delegates to tLocale::NormalizeExcelDateFormatToEnglish).
    /// Same result as tLocale::NormalizeExcelDateFormatToEnglish (free function is a thin wrapper).
    /// @param[in] sFormatString tString source format (may use j/jj/jjj/jjjj, aa/aaaa, etc.)
    /// @return tString format using d/dd/ddd/dddd and y/yy/yyyy tokens
    tString NormalizeExcelDateFormatToEnglish(const tString& sFormatString);

    /// @brief Check if string is a valid Excel date format
    /// @param[in] sFormatString tString
    /// @return tBool
    tBool IsValidExcelDateFormat(const tString& sFormatString);

    /// @brief Convert Excel date format to SkRoot format
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString ConvertToSkRootDateFormat(const tString& sExcelFormat);

    /// @brief Format date with Excel format
    /// @param[in] sYear tInt
    /// @param[in] sMonth tInt
    /// @param[in] sDay tInt
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString FormatWithExcelDateFormat(tInt sYear, tInt sMonth, tInt sDay, const tString& sExcelFormat);

    /// @brief Format time with Excel format
    /// @param[in] sHour tInt
    /// @param[in] sMinute tInt
    /// @param[in] sSecond tInt
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString FormatWithExcelTimeFormat(tInt sHour, tInt sMinute, tInt sSecond, const tString& sExcelFormat);

    /// @brief Format datetime with Excel format
    /// @param[in] sYear tInt
    /// @param[in] sMonth tInt
    /// @param[in] sDay tInt
    /// @param[in] sHour tInt
    /// @param[in] sMinute tInt
    /// @param[in] sSecond tInt
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString FormatWithExcelDateTimeFormat(tInt sYear, tInt sMonth, tInt sDay, 
                                                 tInt sHour, tInt sMinute, tInt sSecond, 
                                                 const tString& sExcelFormat);

} // End of namespace SkRoot

#endif
