//=============================================================================
// SkFormatString tFormatStringClass
/*
 * @page tFormatStringClass 
 * @par 
 * @par Allows you to format numbers or dates as a string
 */ 
//=============================================================================
#ifndef SkFormatString_hpp
#define SkFormatString_hpp

#include "SkClass.hpp"
#include "SkUnit.hpp"

namespace SkRoot {

    // Format Excel ===========================================================
    // 0 Decimal _ Decimal separator
    enum class tFormatStringType : tByte {
        numeric,
        numeric0,
        numeric_,
        numeric0_,
        numeric0_P,
        
        percent,
        percent0,
        
        scientific,
        scientific1,
        scientific2,
        
        accounting,
        accountingP,
        accounting0,
        accounting0P,
        
        datelong,
        dateshort,
        datestr,
        datestryy,
        datestryyyy,
        datedmm,
        datemmmyy,
        datehmmap,
        datehmmssap,
        datehmm,
        datehmmss,
        dateddmmyyyyhmm,
        dateddmmyyyyhmmss,
        excelnumber,
        exceldate,
        none
    };

    enum class tFormatStringFamily : tByte {
        numeric,
        percent,
        scientific,
        accounting,
        date,
        none
    };

    struct tRecFormatStringFamily {
        tFormatStringFamily  m_Family;
        tString              m_Label;
    };


    const tRecFormatStringFamily CstRecFormatStringFamily[] = {
        { tFormatStringFamily::numeric,"Numeric" },
        { tFormatStringFamily::percent,"Percent" },
        { tFormatStringFamily::scientific,"Scientific" },
        { tFormatStringFamily::accounting,"Accounting" },
        { tFormatStringFamily::date,"Date" }
    };

    class  tRecFormatString : public tClass {
        public:
        tFormatStringType    m_Type;
        tString              m_Key;
        tString              m_LocalFormat;
        tShort               m_Decimal;
        tFormatStringFamily  m_FormatFamily;
        
        /// @brief Constructor
        /// @param[in] sType tFormatStringType
        /// @param[in] sKey  tString
        /// @param[in] sLocalFormat  tString
        /// @param[in] sPrecision  tShort
        /// @param[in] sFormatFamily tFormatStringFamily
        tRecFormatString(tFormatStringType    sType,
                          tString              sKey,
                          tString              sLocalFormat,
                          tShort               sPrecision,
                          tFormatStringFamily  sFormatFamily);
        
        /// @brief Copy constructor
        /// @param[in]  sRecFormatString tRecFormatString&
        tRecFormatString(const tRecFormatString& sRecFormatString);
    };

    typedef vector<tRecFormatString> tVectorRectFormatString;

    /// @brief Get the number of decimal places in a number
    /// @param[in] sNumber tDouble
    /// @return tShort
    tShort GetNbDecimal(tDouble sNumber);

    class tFormatStringRoot;

    // tFormatString ==========================================================
    class tFormatString : public tClass {
    private:
        tFormatStringType  m_FormatStringType;
        t_UnitMoney        m_UnitMoney;
        tShort             m_Decimal;
        tString            m_ExcelFormat;
        
        tFormatStringRoot* FormatStringRoot();
    public:
        /// @brief Constructor
        tFormatString();
        
        /// @brief Clear the format string
        void Clear();
        
        /// @brief Set the format string
        /// @param[in] sFormat tString
        void Format(tString sFormat);

        /// @brief Set the format type
        /// @param[in] sFormat tFormatStringType
        void FormatType(tFormatStringType sFormat);
        
        /// @brief Get the format type
        /// @return tFormatStringType
        tFormatStringType FormatType() const;

        /// @brief Get the local format string
        /// @return tString
        tString FormatLocal();
        
        /// @brief Set the money unit
        /// @param[in] sUnitMoney t_UnitMoney
        void Money(t_UnitMoney sUnitMoney);
        
        /// @brief Get the money unit
        /// @return t_UnitMoney
        t_UnitMoney Money() const;

        /// @brief Set the precision
        /// @param[in] sDecimal tShort
        void Decimal(tShort sDecimal);
        
        /// @brief Get the precision
        /// @return tShort
        tShort Decimal();
        
        /// @brief Set the custom format (verify if Number or date
        /// @param[in] sCustom tString
        /// #return true
        tBool ExcelFormat(tString sCustom);

        /// @brief Get the custom format
        /// @return tString
        tString FormatExcel() const;

        /// @brief Get the Excel format corresponding to the current type for a given locale (e.g. "en", "fr-FR")
        /// @param[in] locale tString - locale hint ("fr", "fr-FR", "en", "en-GB", ...)
        /// @return tString - Excel-compatible format string
        tString ExcelEquivalent(const tString& locale);
        
        /// @brief Add or remove decimal places from a numeric format string
        /// @param[in] format tString - the format string (e.g., "###", "###.0", "###.00")
        /// @param[in] decimalChange tInt - positive to add decimals, negative to remove (e.g., 1 adds one decimal, -1 removes one)
        /// @return tString - modified format string
        tString AddDecimals(const tString& format, tInt decimalChange);
        
        /// @brief Set the default format
        /// @param[in] sVariant tVariant*
        /// @return tBool
        tBool SetDefaultFormat(tVariant* sVariant);
        
        // Json ===============================================================
        /// @brief Write JSON
        /// @param[in] sWriter Writer<StringBuffer>*
        void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

        /// @brief Read JSON
        /// @param[in] sValue Value&
        void Json(const rapidjson::Value& sValue);

        /// @brief Check if the format string is empty
        /// @return tBool
        tBool Empty();
        
        /// @brief Equality operator
        /// @param[in] sFormatString tFormatString&
        /// @return tBool
        tBool operator == (tFormatString& sFormatString);
        
        /// @brief Assignment operator
        /// @param[in] sFormatString tFormatString&
        /// @return tFormatString
        tFormatString operator = (tFormatString& sFormatString);
    };

    // Root Container =========================================================
    class tFormatStringRoot : public tClass {
    private:
        tVectorRectFormatString m_VectorFormatString;
    
    public:
        /// @brief Constructor
        tFormatStringRoot();
        
        /// @brief Destructor
        ~tFormatStringRoot();
     
        /// @brief Clear the format string root
        void Clear();
        
        /// @brief Fill the format string root
        void Fill();
        
        /// @brief Get format string type by key
        /// @param[in] sFormatStringKey tString
        /// @return tFormatStringType
        tFormatStringType StringKey2FormatStringType(tString sFormatStringKey);
        
        /// @brief Get format string family by key
        /// @param[in] sFormatStringKey tString
        /// @return tFormatStringFamily
        tFormatStringFamily StringKey2FormatStringFamily(tString sFormatStringKey);

        /// @brief Set local format string
        /// @param[in] sFormatStringType tFormatStringType
        /// @param[in] sFormatString tString
        void SetLocalFormatString(tFormatStringType sFormatStringType, tString sFormatString);

        /// @brief Get local format string by format string type
        /// @param[in] sFormatStringType tFormatStringType
        /// @return tString
        tString FormatStringType2LocalFormatString(tFormatStringType sFormatStringType);

        /// @brief      Get the predefined key (locale-independent) for a format type.
        ///
        /// The key is what `StringKey2FormatStringType` matches against in the
        /// reverse direction; it is the only safe payload to round-trip through
        /// CSS `format-string:"..."` for locale-aware types (date*, numeric*,
        /// percent*, ...). LocalFormat() returns a locale-resolved literal
        /// (e.g. `%m-%d-%Y` in en-US), which the parser cannot match back to a
        /// type and therefore falls through to ExcelFormat() and breaks the
        /// locale-aware behavior on reload.
        /// @param[in]  sFormatStringType tFormatStringType
        /// @return     tString empty if the type has no registered key (e.g. excelnumber/exceldate/none)
        tString FormatStringType2Key(tFormatStringType sFormatStringType);
        
        /// @brief Get format string family by format string type
        /// @param[in] sFormatStringType tFormatStringType
        /// @return tFormatStringFamily
        tFormatStringFamily FormatString2Family(tFormatStringType sFormatStringType);

        /// @brief Get default precision by format string type
        /// @param[in] sFormatStringType tFormatStringType
        /// @return tShort
        tShort DefaultPrecision(tFormatStringType sFormatStringType);

        /// @brief Get JSON format string
        /// @return tString
        tString JsonFormatString();

        /// @brief Get default format string
        /// @param[in] sFormatString tString
        /// @return tString
        tString DefaultFormatString(tString sFormatString);

        // Excel bridge ======================================================
        /// @brief Check if the given string is a valid Excel DATE format
        /// @param[in] sFormatString tString
        /// @return tBool
        tBool IsValidExcelDate(tString sFormatString);

        /// @brief Check if the given string is a valid Excel NUMBER format
        /// @param[in] sFormatString tString
        /// @return tBool
        tBool IsValidExcelNumber(tString sFormatString);

        /// @brief Convert an Excel date/number format to a local SkRoot format string
        /// @param[in] sExcelFormat tString
        /// @return tString (SkRoot local format string) or empty if unknown
        tString ExcelToLocalFormatString(tString sExcelFormat);

        /// @brief Build a preview string from an Excel format (date or number)
        /// @param[in] sExcelFormat tString
        /// @return tString preview using sample values
        tString DefaultFormatStringExcel(tString sExcelFormat);
    };

}; // End of NameSpace

#endif
