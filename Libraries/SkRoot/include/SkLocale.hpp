///=============================================================================
// Skeema Locale
//=============================================================================
#ifndef SkLocale_hpp
#define SkLocale_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkSharedString.hpp"
#include "SkUnit.hpp"
#include "SkFormatString.hpp"
namespace SkRoot {

enum class tLang : tChar {
    US,
    FR,
    DE,
    EN,
    SP,
    IT
};

class tLocale : public tClass {
    private:
        tLang m_Lang;
        tChar m_Decimal;
        tChar m_Thousand;
        tString m_Grouping;
        
        tChar m_Date;
        tChar m_Time;
        
        tChar m_Arg; // SpreadSheet function
        
        // Name of day & Month
        tString m_DayStr[7];        // 0=Sunday, 1=Monday, ..., 6=Saturday
        tString m_MonthStr[13];     // 1=January, 2=February, ..., 12=December (index 0 unused)
        
        // Abbreviated names
        tString m_DayStrAbbr[7];    // Abbreviated day names
        tString m_MonthStrAbbr[13]; // Abbreviated month names
        
        void SetDayMonth(tLang sLang);
        
        // Money Symbol
        t_UnitMoney   m_UnitMoney;

        // Default Excel-style date format codes (locale UI: FR uses j/a; US uses m/d/y; see SkFormatDate::NormalizeExcelDateFormatToEnglish)
        tString m_ExcelFormatDateShort;
        tString m_ExcelFormatDateLong;
        tString m_ExcelFormatDateTime;
    public:
        /// @brief Constructor
        tLocale();
        
        /// @brief Set locale
        /// @param[in] sLang tString 
        void Lang(tString sLang);
        
        /// @brief Get locale
        /// @return tString
        tString Lang();

        /// @brief Set locale settings
        void Set();
 
        /// @brief Get Decimal separator
        /// @return tChar
        tChar Decimal();
        
        /// @brief Get Thousand separator
        /// @return tChar
        tChar Thousand();
   
        /// @brief Get Grouping
        /// @return tString
        tString Grouping();

        /// @brief Get Date separator
        /// @return tChar
        tChar Date();
    
        /// @brief Get Time separator
        /// @return tChar
        tChar Time();
        
        /// @brief Get argument separator
        /// @return tChar
        tChar Arg();
        
        /// @brief Get format for long date
        /// @return tString
        tString FormatDateLong();

        /// @brief Get format for short date
        /// @return tString
        tString FormatDateShort();

        /// @brief Get format for long time
        /// @return tString
        tString FormatTimeLong();
        
        /// @brief Get format for short time
        /// @return tString
        tString FormatTimeShort();

        /// @brief Default Excel short date pattern for this locale (e.g. jj/mm/aaaa for FR, mm/dd/yyyy for US)
        /// @return tString
        tString ExcelFormatDateShort();

        /// @brief Default Excel long date pattern for this locale (e.g. jjjj jj mmmm aaaa for FR)
        /// @return tString
        tString ExcelFormatDateLong();

        /// @brief Default Excel date+time pattern for this locale
        /// @return tString
        tString ExcelFormatDateTime();

        /// @brief Map locale-specific Excel date codes to English (French j/jj/jjj/jjjj -> d/..., aa/aaaa -> y/...; preserves AM/PM, A/P).
        /// Identical to SkRoot::NormalizeExcelDateFormatToEnglish (implementation lives here). Same normalization as ExcelFormatDateShort/Long/Time semantics.
        /// @param[in] sFormatString raw Excel format string (may contain j, a, etc.)
        /// @return tString English-token format (d, y, m, h, s)
        static tString NormalizeExcelDateFormatToEnglish(const tString& sFormatString);
        
        /// @brief Return day string
        /// @param[in] sDayNum tInt
        /// @return tString
        tString DayStr(tInt sDayNum);
  
        /// @brief Return month string
        /// @param[in] sMonthNum tInt
        /// @return tString
        tString MonthStr(tInt sMonthNum);
        
        /// @brief Return abbreviated day string
        /// @param[in] sDayNum tInt
        /// @return tString
        tString DayStrAbbr(tInt sDayNum);
        
        /// @brief Return abbreviated month string
        /// @param[in] sMonthNum tInt
        /// @return tString
        tString MonthStrAbbr(tInt sMonthNum);
        
        /// @brief Return unit money
        /// @return t_UnitMoney
        t_UnitMoney UnitMoney();
        
        /// @brief Return money symbol
        /// @return tString
        tString MoneySymbol();
    };

    /// @brief RAII: temporarily switch tApplication locale, restore on scope exit.
    ///             Used so formula wire (undo/collab/.sker) stays US while UI locale may be FR.
    class tLocalePush {
        tString m_Prev;
        tBool m_Changed;
    public:
        explicit tLocalePush(const tString& sLang);
        ~tLocalePush();
        tLocalePush(const tLocalePush&) = delete;
        tLocalePush& operator=(const tLocalePush&) = delete;
    };

} // end of namespace

#endif /* SkLocale_h */
