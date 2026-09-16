//=============================================================================
// SkFormatNumbet - Excel Format Parser
/**
* @page SkFormat
* @par 
* @par Parse Excel number formats like ###.##0,00 to extract formatting elements
*/
//=============================================================================
#ifndef SkFormat_hpp
#define SkFormat_hpp

#include "SkTypes.hpp"
#include "SkFormatString.hpp"

namespace SkRoot {
    struct tFormatResult {
        tString text;       // formatted output
        tString colorName;  // e.g. "Red", "Color 6", empty if none
        tBool   hasColor;   // true if a color tag was present in the applied section
        tFormatResult() : hasColor(false) {}
    };

    // Excel Format Elements ==================================================
    struct tExcelFormatElement {
        tString m_Element;        // The element string (e.g., "###", "##0", ",00")
        tString m_Type;          // Type: "integer", "decimal", "separator", "currency", "percent"
        tInt    m_Position;      // Position in the format string
        tInt    m_Length;        // Length of the element
        tBool   m_IsOptional;    // Whether this element is optional (marked with #)
        tBool   m_IsGrouping;    // Whether this is a thousands separator
        tBool   m_IsDecimal;     // Whether this is a decimal separator
    };

    typedef std::vector<tExcelFormatElement> tVectorExcelFormatElement;

    // Excel Format Parser ====================================================
    class tExcelFormatParser : public tClass {
    private:
        tString m_FormatString;                    // Original format string
        tString m_ParseNormalized;                 // Preprocessed for parsing (e.g. [$€-40C] → €)
        tVectorExcelFormatElement m_Elements;      // Parsed elements
        tString m_IntegerPart;                     // Integer part pattern
        tString m_DecimalPart;                     // Decimal part pattern
        tString m_ThousandsSeparator;              // Thousands separator
        tString m_DecimalSeparator;                // Decimal separator
        tString m_CurrencySymbol;                  // Currency symbol if present
        tBool   m_HasPercent;                      // Whether format includes percent
        tBool   m_IsScientific;                    // Whether format includes scientific notation
        tInt    m_ExponentDigits;                  // Number of exponent digits (e.g., E+00 => 2)
        tInt    m_DecimalPlaces;                   // Number of decimal places
        tInt    m_IntegerDigits;                   // Number of integer digits
        tBool   m_IsValid;                         // Whether format is valid

        // Internal parsing methods
        void ParseFormatString();
        void ParseElement(const tString& sElement, tInt sPosition);
        void AnalyzeElements();
        tString ExtractPattern(const tString& sElement);
        tBool IsOptionalPattern(const tString& sElement);
        tBool IsGroupingSeparator(const tString& sElement);
        tBool IsDecimalSeparator(const tString& sElement);
        tBool IsCurrencySymbol(const tString& sElement);
        tBool IsPercentSymbol(const tString& sElement);
        tString ApplyThousandsSeparator(const tString& sNumber, const tString& sThousandsSep) const;

    public:
        /// @brief Constructor
        /// @param[in] sFormatString tString - Excel format string like "###.##0,00"
        tExcelFormatParser(const tString& sFormatString = "");

        /// @brief Destructor
        ~tExcelFormatParser();

        /// @brief Set the format string to parse
        /// @param[in] sFormatString tString
        void SetFormatString(const tString& sFormatString);

        /// @brief Get the original format string
        /// @return tString
        tString GetFormatString() const;

        /// @brief Get all parsed elements
        /// @return const tVectorExcelFormatElement&
        const tVectorExcelFormatElement& GetElements() const;

        /// @brief Get the integer part pattern
        /// @return tString
        tString GetIntegerPart() const;

        /// @brief Get the decimal part pattern
        /// @return tString
        tString GetDecimalPart() const;

        /// @brief Get the thousands separator
        /// @return tString
        tString GetThousandsSeparator() const;

        /// @brief Get the decimal separator
        /// @return tString
        tString GetDecimalSeparator() const;

        /// @brief Get the currency symbol
        /// @return tString
        tString GetCurrencySymbol() const;

        /// @brief Check if format includes percent
        /// @return tBool
        tBool HasPercent() const;

        /// @brief Get number of decimal places
        /// @return tInt
        tInt GetDecimalPlaces() const;

        /// @brief Get number of integer digits
        /// @return tInt
        tInt GetIntegerDigits() const;

        /// @brief Check if format is valid
        /// @return tBool
        tBool IsValid() const;

        /// @brief Clear all data
        void Clear();

        /// @brief Convert to SkRoot format string type
        /// @return tFormatStringType
        tFormatStringType ToSkRootFormatType() const;

        /// @brief Get SkRoot format string
        /// @return tString
        tString ToSkRootFormatString() const;

        /// @brief Format a number using the parsed format
        /// @param[in] sNumber tDouble - Number to format
        /// @return tString - Formatted number string
        tString FormatNumber(tDouble sNumber) const;

        /// @brief Get format description
        /// @return tString
        tString GetDescription() const;

        /// @brief Debug: Print all elements
        void PrintElements() const;
    };

    // Number Formatter with Pool =============================================
    class tNumberFormatter : public tClass {
    private:
        std::unordered_map<tString, std::shared_ptr<tExcelFormatParser>> m_FormatPool;
        std::mutex m_PoolMutex;
        
        // Get or create parser from pool
        std::shared_ptr<tExcelFormatParser> GetParser(const tString& sFormatString);
        
    public:
        /// @brief Constructor
        tNumberFormatter();
        
        /// @brief Destructor
        ~tNumberFormatter();
        
        /// @brief Format a number with Excel format (cached)
        /// @param[in] sNumber tDouble - Number to format
        /// @param[in] sFormatString tString - Excel format string
        /// @return tString - Formatted number string
        tString FormatNumber(tDouble sNumber, const tString& sFormatString);

        /// @brief Format a number and also return applied color information from section tags
        /// Recognizes [Red], [Blue], [Green], [Black], [White], [Magenta], [Yellow], [Cyan], and [Color n]
        /// @param[in] sNumber tDouble - Number to format
        /// @param[in] sFormatString tString - Excel format string (supports up to 4 sections)
        /// @return tFormatResult - text plus optional color name
        tFormatResult FormatNumberWithInfo(tDouble sNumber, const tString& sFormatString);
        
        /// @brief Format multiple numbers with same format (optimized)
        /// @param[in] sNumbers std::vector<tDouble> - Numbers to format
        /// @param[in] sFormatString tString - Excel format string
        /// @return std::vector<tString> - Formatted number strings
        std::vector<tString> FormatNumbers(const std::vector<tDouble>& sNumbers, const tString& sFormatString);
        
        /// @brief Get format parser from pool (for advanced usage)
        /// @param[in] sFormatString tString - Excel format string
        /// @return std::shared_ptr<tExcelFormatParser> - Parser from pool
        std::shared_ptr<tExcelFormatParser> GetFormatParser(const tString& sFormatString);
        
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
        
        /// @brief Preload common formats
        void PreloadCommonFormats();
    };

    // Utility Functions ======================================================
    
    /// @brief Parse Excel format string and return elements
    /// @param[in] sFormatString tString
    /// @return tVectorExcelFormatElement
    tVectorExcelFormatElement ParseExcelNumberFormat(const tString& sFormatString);

    /// @brief Check if string is a valid Excel number format
    /// @param[in] sFormatString tString
    /// @return tBool
    tBool IsValidExcelNumberFormat(const tString& sFormatString);
    
    /// @brief Convert Excel format to SkRoot format
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString ConvertToSkRootFormat(const tString& sExcelFormat);
     
    /// @brief Format number with Excel format
    /// @param[in] sNumber tDouble
    /// @param[in] sExcelFormat tString
    /// @return tString
    tString FormatWithExcelNumberFormat(tDouble sNumber, const tString& sExcelFormat);

    /// @brief Replace OOXML international currency brackets [$€-40C], [$£-en-GB], [$$-409], etc.
    /// with literal € £ ¥ $ so downstream stripping/mapping still sees the currency glyph.
    /// @param[in] sFormatString Raw Excel number format fragment or full code
    /// @return Mask with bracket tokens expanded
    tString NormalizeExcelIntlCurrencyBracketsInMask(const tString& sFormatString);

    /// @brief Normalize Excel number-format masks to US structural convention: comma = thousands, dot = decimal.
    /// Sections honor quoted literals and bracket tokens. Used for parsing, caching, and stored excelnumber codes.
    /// @param[in] sFormatString Raw number format (European or US-style separators)
    /// @return Canonical mask (internally consistent with Sk Excel number rendering pipeline)
    tString NormalizeExcelNumberFormatToUs(const tString& sFormatString);

     
}; // End of namespace SkRoot

#endif
