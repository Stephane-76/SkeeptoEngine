//=============================================================================
// Skeema TypesClass
/**
* @page SkTypeClass
* @par
* @par Class for types Int, Float, Double, String and Date.
* @par We use the concept of Boxing and Unboxing from C #.
*/
//=============================================================================
#ifndef SkTypesClass_hpp
#define SkTypesClass_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkLocale.hpp"
#include "SkFormatApi.hpp"
#include "SkFormatString.hpp"
#include <iomanip>
#include <regex>

#include <algorithm>

namespace SkRoot {

    class tVariant;

    // Format String ==========================================================
    template <typename T>
    class tClassFormatString : public tClass {
    private:
        T              m_Value;
        tFormatString* m_FormatString;
    public:
        /// @brief Constructor
        /// @param[in] sValue T
        /// @param[in]  sFormatString  tFormatString&
        tClassFormatString(const T sValue,tFormatString* const sFormatString);
        
        /// @brief Round the value
        /// @return T
        T Round();
        
        /// @brief Format the value
        /// @param[in] sFormatString tFormatStringType
        /// @return tString
        tString Format();
        
        /// @brief Format the value with Excel format string
        /// @param[in] sFormatString tFormatStringType
        /// @return tString
        tString FormatStr();
        
        /// @brief Format the string
        /// @param[in] sValue tString
        /// @param[in] sGrouping tByte
        /// @return tString
        tString FormatStr(tString sValue,tByte sGrouping);
    };


    //=========================================================================
    //! Class Int (boxing)
    class tClassInt : public tClass {
    private:
        //! Value tInt for tClassInt.
        tInt m_Value;
    public:
        /// @brief      Constructor tClassInt.
        tClassInt();
        
        /// @brief      Constructor tClassInt with value.
        /// @param[in]	sValue tInt
        tClassInt(tInt sValue);

        /// @brief      Constructor of copy.
        /// @param[in]	sClassInt const tClassInt&
        tClassInt(const tClassInt& sClassInt);
        
        /// @brief   Set FormatString
        /// @param[in]   sFormatString tFormatString* const
        /// @return tString
        tString FormatString(tFormatString* const sFormatString);

        /// @brief      Operator ==.
        /// @param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator==(const tClassInt& sClassInt);

        /// @brief      Operator !=.
        /// @param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator!=(const tClassInt& sClassInt);

        /// @brief      Operator <.
        ///	@param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator < (const tClassInt& sClassInt);
        /// @brief      Operator >.
        ///	@param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator > (const tClassInt& sClassInt);
        /// @brief      Operator <=.
        ///	@param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator <= (const tClassInt& sClassInt);
        /// @brief      Operator >=.
        ///	@param[in]	sClassInt const tClassInt&
        /// @return tBool 
        tBool operator >= (const tClassInt& sClassInt);

        /// @brief      Operator -- with value.
        ///	@param[in]	sClassInt tClassInt&
        /// @return tClassInt
        tClassInt operator --(tInt sClassInt);
        /// @brief      Operator ++ with value.
        ///	@param[in]	sClassInt tClassInt&
        /// @return tClassInt
        tClassInt operator ++(tInt sClassInt);

        /// @brief      Operator --().
        /// @return tClassInt&
        tClassInt& operator --();

        /// @brief      Operator ++().
        /// @return tClassInt&
        tClassInt& operator ++();

        // Friend function ====================================================
        /// @brief      Operator +.
        ///	@param[in]	sClassInt1 const tClassInt&
        ///	@param[in]	sClassInt2 const tClassInt&
        /// @return tClassInt
        friend tClassInt operator+(const tClassInt& sClassInt1, const tClassInt& sClassInt2);
        
        /// @brief      Operator -.
        ///	@param[in]	sClassInt1 const tClassInt&
        ///	@param[in]	sClassInt2 const tClassInt&
        /// @return tClassInt
        friend tClassInt operator-(const tClassInt& sClassInt1, const tClassInt& sClassInt2);
        
        /// @brief      Operator *.
        ///	@param[in]	sClassInt1 const tClassInt&
        ///	@param[in]	sClassInt2 const tClassInt&
        /// @return tClassInt
        friend tClassInt operator*(const tClassInt& sClassInt1, const tClassInt& sClassInt2);
        
        /// @brief      Operator /.
        ///	@param[in]	sClassInt1 const tClassInt&
        ///	@param[in]	sClassInt2 const tClassInt&
        /// @return tClassInt
        friend tClassInt operator/(const tClassInt& sClassInt1, const tClassInt& sClassInt2);

        /// @brief      Operator ().
        /// @return tInt
        tInt operator()();
        
        /// @brief      Str return Int to String.
        /// @return tString
        tString Str();

        /// @brief      friend ostream& operator<<.
        ///	@param[in]	os ostream&
        ///	@param[in]	sClassInt const tClassInt&
        /// @return ostream&
        friend ostream& operator<<(ostream& os, const tClassInt& sClassInt);
    };
    
    //=========================================================================
    //! Class float (boxing)
    class tClassFloat : public tClass {
    private:
        //! Value tFloat for tClassFloat.
        tFloat m_Value;
    public:
        /// @brief      Constructor tClassFloat.
        tClassFloat();

        /// @brief      Constructor tClassFloat with value.
        /// @param[in]	sValue tFloat
        tClassFloat(tFloat sValue);

        /// @brief      Constructor tClassFloat of copy.
        /// @param[in]	sClassFloat const tClassFloat&
        tClassFloat(const tClassFloat& sClassFloat);
        
        /// @brief   Set FormatString
        /// @param[in]   sFormatString tFormatString* const
        /// @return tString
        tString FormatString(tFormatString* const sFormatString);

        /// @brief      Operator = with string (convert).
        /// @param[in]	sString const tString&
        /// @return tClassFloat& 
        tClassFloat operator = (const tString& sString);

        /// @brief      Operator ==.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator==(const tClassFloat& sClassFloat);

        /// @brief      Operator !=.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator!=(const tClassFloat& sClassFloat);

        /// @brief      Operator <.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator < (const tClassFloat& sClassFloat);
        /// @brief      Operator >.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator > (const tClassFloat& sClassFloat);
        /// @brief      Operator <=.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator <= (const tClassFloat& sClassFloat);
        /// @brief      Operator >=.
        /// @param[in]	sClassFloat const tClassFloat& 
        /// @return tBool 
        tBool operator >= (const tClassFloat& sClassFloat);

        // Friend function ==========================================================
        /// @brief      Operator +.
        ///	@param[in]	sClassFloat1 const tClassFloat&
        ///	@param[in]	sClassFloat2 const tClassFloat&
        /// @return tClassFloat
        friend tClassFloat operator+(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2);
        /// @brief      Operator -.
        ///	@param[in]	sClassFloat1 const tClassFloat&
        ///	@param[in]	sClassFloat2 const tClassFloat&
        /// @return tClassFloat
        friend tClassFloat operator-(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2);
        /// @brief      Operator *.
        ///	@param[in]	sClassFloat1 const tClassFloat&
        ///	@param[in]	sClassFloat2 const tClassFloat&
        /// @return tClassFloat
        friend tClassFloat operator*(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2);
        /// @brief      Operator /.
        ///	@param[in]	sClassFloat1 const tClassFloat&
        ///	@param[in]	sClassFloat2 const tClassFloat&
        /// @return tClassFloat
        friend tClassFloat operator/(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2);

        /// @brief      Operator  Get float 
        /// @return		tFloat 
        tFloat operator()();

        /// @brief      Method Get float in string 
        /// @return		tString 
        tString Str();

    
        /// @brief      Operator << for friend ostream 
        /// @param[in]  os ostream& stream ouput 
        /// @param[in]  sClassFloat const tClassFloat& value
        friend ostream& operator<<(ostream& os, const tClassFloat& sClassFloat);
    };

    //=========================================================================
    //! Class double (boxing)
    class tClassDouble : public tClass {
    private:
        //! Value tDouble for tClassDouble.
        tDouble m_Value;
    public:
        /// @brief      Constructor tClassDouble.
        tClassDouble();

        /// @brief      Constructor tClassDouble with value.
        /// @param[in]	sValue tDouble
        tClassDouble(tDouble sValue);

        /// @brief      Constructor tClassDouble of copy.
        /// @param[in]	sClassDouble const tClassDouble&
        tClassDouble(const tClassDouble& sClassDouble);

        /// @brief   Set FormatString
        /// @param[in]   sFormatString tFormatString* const
        /// @return tString
        tString FormatString(tFormatString* const sFormatString);
        
        /// @brief      Operator = with string (convert).
        /// @param[in]	sString const tString&
        /// @return tClassDouble&
        tClassDouble operator = (const tString& sString);

        /// @brief      Operator ==.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator==(const tClassDouble& sClassDouble);

        /// @brief      Operator !=.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator!=(const tClassDouble& sClassDouble);

        /// @brief      Operator <.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator < (const tClassDouble& sClassDouble);
        /// @brief      Operator >.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator > (const tClassDouble& sClassDouble);
        /// @brief      Operator <=.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator <= (const tClassDouble& sClassDouble);
        /// @brief      Operator >=.
        /// @param[in]	sClassDouble const tClassDouble& 
        /// @return tBool 
        tBool operator >= (const tClassDouble& sClassDouble);

        // Friend function ==========================================================
        /// @brief      Operator +.
        ///	@param[in]	sClassDouble1 const tClassDouble&
        ///	@param[in]	sClassDouble2 const tClassDouble&
        /// @return tClassDouble
        friend tClassDouble operator+(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2);
        /// @brief      Operator -.
        ///	@param[in]	sClassDouble1 const tClassDouble&
        ///	@param[in]	sClassDouble2 const tClassDouble&
        /// @return tClassDouble
        friend tClassDouble operator-(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2);
        /// @brief      Operator *.
        ///	@param[in]	sClassDouble1 const tClassDouble&
        ///	@param[in]	sClassDouble2 const tClassDouble&
        /// @return tClassDouble
        friend tClassDouble operator*(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2);
        /// @brief      Operator /.
        ///	@param[in]	sClassDouble1 const tClassDouble&
        ///	@param[in]	sClassDouble2 const tClassDouble&
        /// @return tClassDouble
        friend tClassDouble operator/(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2);

        /// @brief      Operator  Get double 
        /// @return		tDouble 
        tDouble operator()();

        /// @brief      Method Get double in string 
        /// @return		tString 
        tString Str();

        /// @brief      Operator << for friend ostream
        /// @param[in]  os ostream& stream ouput 
        /// @param[in]  sClassDouble const tClassDouble value
        friend ostream& operator<<(ostream& os, const tClassDouble& sClassDouble);
    };

    //=========================================================================
    //! Class string (boxing)
    class tClassString : public tClass {
    private:
        //! Value tString for tClassString.
        tString m_Value;
    public:
        /// @brief      Constructor tClassString.
        tClassString();

        /// @brief      Constructor tClassString of copy.
        /// @param[in]    sClassString const tClassString&
        tClassString(const tClassString& sClassString);

        /// @brief      Constructor with value tString
        /// @param[in]	sValue tString
        tClassString(tString sValue);

        
        /// @brief      Constructor with value Char*.
        /// @param[in]    sValue tChar*
        tClassString(tChar* sValue);

        /// @brief      Constructor with value tInt.
        /// @param[in]	sValue tString
        tClassString(tInt sValue);

        /// @brief      Constructor with value tDouble.
        /// @param[in]	sValue tString
        tClassString(tDouble sValue);

        /// @brief      Left return left string
        /// @param[in]	sLength tInt Length
        /// @return		tString
        tString Left(tSize sLength);

        /// @brief      Right return right string
        /// @param[in]	sLength tInt Length
        /// @return		tString
        tString Right(tSize sLength);

        /// @brief      Mid return mid string
        /// @param[in]	sStart tInt Start
        /// @param[in]	sLength tInt Length
        /// @return		tString
        tString Mid(tSize sStart, tSize sLength);

        /// @brief      Length return length of string
        /// @return		tSize
        tSize Length();

        /// @brief      Count how many times a character appears in the string.
        /// @param[in]  sChar character to count
        /// @return     tSize number of occurrences
        tSize CountChar(tChar sChar) const;

        // UpperCase Lower Case ===================================
        /// @brief      get upper string
        /// @return		tString
        tString Upper();
        /// @brief      get lower string
        /// @return		tString
        tString Lower();

        /// @brief      Method to remove spaces to the left
        /// @return		tString
        tString Ltrim();
        /// @brief      Method to remove spaces to the right
        /// @return		tString
        tString Rtrim();
        /// @brief      Method to remove spaces to the left an right
        /// @return		tString
        tString Trim();

        /// @brief      Method to remove single or double quotes from the beginning and end of the string
        /// @return		tString
        tString Unquote();

        /// @brief      Proper return proper string
        /// @return		tString
        tString Proper();

        /// @brief      Find string at pos
        /// @param[in]  sValue
        /// @return     size_t
        size_t Find(const tString sValue);
        
        /// @brief      Method to get all element separate by separator to vector
        /// @param[in]  sSeparator tString Separator
        /// @return		SkStringVector
        tVectorString Split(tString sSeparator);

        /// @brief      Method to replace all from by to
        /// @param[in]  sFrom tString from
        /// @param[in]  sTo tString to
        /// @return		SkStringVector
        void Replace(const tString sFrom, const tString sTo);
        
        // Fonction Regex =========================================
        // ECMAScript (std::regex). Invalid patterns are caught (no throw to caller).
        /// @brief      Full-string regex match. Returns false on no match or invalid pattern.
        tBool Regex_Match(tString sRegex, tBool sIgnoreCase = false);

        /// @brief      Substring regex search. Returns false on no match or invalid pattern.
        tBool Regex_Search(tString sRegex, tBool sIgnoreCase = false);

        /// @brief      Replace all regex matches. Returns false if the pattern is invalid.
        tBool Regex_Replace(tString sRegex, tString sReplace, tBool sIgnoreCase = false);

        /// @brief      First regex match substring into oOut. Returns false if none/invalid.
        tBool Regex_ExtractFirst(tString sRegex, tString& oOut, tBool sIgnoreCase = false);

        // Test validity of concept ===============================
        /// @brief      verify is Unsigned integer
        /// @@return	tBool		 
        tBool IsUnsigned();

        /// @brief      verify is integer
        /// @@return	tBool		 
        tBool IsInteger();

        /// @brief      verify is number
        /// @@return	tBool		 
        tBool IsNumber();

        /// @brief      StartsWith return true if string beginning by sValue
        /// @param[in]	sValue tString
        /// @@return	tBool
        tBool StartsWith(tString sValue);
				 
        /// @brief      verify is key code 
        /// @@return	tBool		 
        tBool IsKeyCode();

        /// @brief      verify is email adress
        /// @@return	tBool		 
        tBool IsEmailAdress();

        /// @brief      verify is url
        /// @@return	tBool		 
        tBool IsUrl();

        // @brief convert to int
        /// @@return	tInt		 
        tInt ToInt();

        // @brief convert to Double
        /// @@return	SkSDouble
        tDouble ToDouble();
        

        /// @brief      Operator =.
        /// @param[in]  sValue tChar*
        /// @return tBool
        tClassString& operator = (const tChar* sValue);
        
        /// @brief      Operator ==.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator==(const tClassString& sClassString);

        /// @brief      Operator !=.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator!=(const tClassString& sClassString);

        /// @brief      Operator <.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator < (const tClassString& sClassString);
        /// @brief      Operator >.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator > (const tClassString& sClassString);
        /// @brief      Operator <=.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator <= (const tClassString& sClassString);
        /// @brief      Operator >=.
        /// @param[in]	sClassString const tClassString&
        /// @return tBool
        tBool operator >= (const tClassString& sClassString);

        /// @brief      Operator +.
        /// @param[in]	sClassString const tClassString&
        /// @return tClassString&
        tClassString operator +(const tClassString& sClassString);


        /// @brief      Operator () for get result
        /// @return     tString              
        tString operator()();

        /// @brief      Operator << for friend ostream 
        /// @param[in]  os ostream& stream ouput 
        /// @param[in]  sClassString tClassString  value
        friend ostream& operator<<(ostream& os, const tClassString& sClassString);
    };

    enum class tStateParse :  tByte {
        none,
        _percent,
        YEAR,
        year,
        month,
        day,
        hour,
        minute,
        second
    };

    //=========================================================================
    //! Class date (boxing)
    class tClassDate : public tClass {
    private:
        tDate m_Value;
     
        tDate TimeOffset();
      
        /// @brief      Put date to tm.
        /// @param[in]  sTMStruct struct tm& 
        void DateToTMStruct(struct tm& sTMStruct);

        /// @brief      Put tm to date to tm.
        /// @param[in]  sTMStruct struct tm& 
        void TMStructToDate(struct tm& sTMStruct);

        /// @brief      Just Year month day.
        void InternalSet();
        
        // Add Int to day
        /// @param[in] sInt tInt
        tDate AddInt(tInt sInt);
 
        // Add Double
        /// @param[in] sDouble tDouble
        tDate AddDouble(tDouble sDouble);
        
        tString Day0();
        
        tString Month0();
        
        tString YearShort0();
        
        tString YearLong0();
        
        tString  AMPM(tFormatStringType sFormatStringType);
        
    public: // Test
        // Return Date Time with format
        /// @param[in] sFormat tString
        /// @return    tString
        string FormatDateTime(tString sFormat);
    public:
        /// @brief      Constructor tClassDate.
        tClassDate();

        /// @brief      Constructor tClassDate withtDate Value.
        /// @param[in] sValuetDate
        tClassDate(tDate sValue);
        
        /// @brief C lear Date
        void Clear();
        
        /// @brief      Set now.
        static tDate Now();
                
        /// @brief   Set FormatString
        /// @param[in]   sFormatString tFormatString* const
        /// @return tString
        tString FormatString(tFormatString* const sFormatString);

        /// @brief      Constructor tClassDate with year, month and day.
        /// @param[in] sYear tInt Year
        /// @param[in] sMonth tInt Month
        /// @param[in] sDay tInt Day
        tClassDate(tInt sYear, tInt sMonth, tInt sDay);

        /// @brief      Constructor tClassDate of copy.
        /// @param[in]	sClassDate const tClassDate&
        tClassDate(const tClassDate& sClassDate);

        /// @brief Build from a variant: copy t_date; interpret t_int/t_double as Excel serial days (fraction = time of day).
        /// @param[in] sVariant source variant
        explicit tClassDate(const tVariant& sVariant);

        /// @brief      Get  Date
        /// @return		tDate
        tDate Value();

        /// @brief      Set Date (For Debug )
        /// @return        tDate
        void Value(tDate sValue);
        
        /// @brief      Return true (01/01/1900)
        /// @return     tBool
        tBool IsHours();
     
        /// @brief      Set year, month and day
        /// @param[in] sYear tInt Year
        /// @param[in] sMonth tInt Month
        /// @param[in] sDay tInt Day
        /// @return		tDate
        tDate Set(tInt sYear, tInt sMonth, tInt sDay);
        
        /// @brief      Set year, month and day, hour, minute second
        /// @param[in] sHour  tInt Hour
        /// @param[in] sMinute  tInt Minute
        /// @param[in] sSecond  tInt second
        tDate SetHour(tInt sHour,tInt sMinute,tInt sSecond);

        /// @brief      Set year, month and day, hour, minute second
        /// @param[in] sYear tint  Year
        /// @param[in] sMonth tInt Month
        /// @param[in] sDay tInt Day
        /// @param[in] sHour  tInt Hour
        /// @param[in] sMinute  tInt Minute
        /// @param[in] sSecond  tInt second
        tDate SetDateHour(tInt sYear, tInt sMonth, tInt sDay,tInt sHour,tInt sMinute,tInt sSecond);

        /// @brief      get year, month and day
        /// @param[out] sYear tInt& Year
        /// @param[out] sMonth tInt& Month
        /// @param[out] sDay tInt& Day
        void YearMonthDay(tInt& sYear, tInt& sMonth, tInt& sDay);

        /// @brief      get hour  , minute  and second
        /// @param[out] sHour  tInt& Hour
        /// @param[out] sMinute  tInt& Minute
        /// @param[out] sSecond  tInt& Second
        void HourMinuteSecond(tInt& sHour, tInt& sMinute, tInt& sSecond);

        // function to parse a date or time string.
        /// @param[in] sString const char*
        /// @param[in] sFormat const char*
        /// @return    tBool
        tBool ParseDateTime(const char* sString, const char* sFormat);
       
        /// @brief      Return Year
        /// @return		tInt
        tInt  Year();

        /// @brief      Return Year
        /// @return		tInt
        tInt  Month();

        /// @brief      Return Year
        /// @return		tInt
        tInt  Day();

        /// @brief      Return Hour
        /// @return        tInt
        tInt  Hour();

        /// @brief      Return Minute
        /// @return        tInt
        tInt  Minute();

        /// @brief      Return Second
        /// @return        tInt
        tInt  Second();
        
        /// @brief      Return Day of week
        /// @return		tInt
        tInt  DayWeek();

        /// @brief      Return Week Number
        /// @return		tInt
        tInt  WeekNum();

        /// @brief      get Us date
        /// @param[in]  sTime wiith time
        /// @return		tString
        tString UsDate(tBool sTime=true);
        
        /// @brief      Set Us date
        /// @param[in]	tString
        void UsDate(tString sDate);

        /// @brief      Set Us date (const char* overload).
        ///
        /// Without this overload, a call site that passes a raw `const char*`
        /// (e.g. `wValue.GetString()` from rapidjson) silently resolves to
        /// `UsDate(tBool)` (standard pointer-to-bool conversion is preferred
        /// over the user-defined conversion to tString), invoking the GETTER
        /// and leaving m_Value at the Clear() default (1900-01-01). This
        /// overload forwards explicitly to the tString setter.
        void UsDate(const char* sDate) { UsDate(tString(sDate ? sDate : "")); }
        /*
        /// @brief      Operator =.
        /// @param[in]	sClassDate const tClassDate&
        /// @return tClassDate&
        tClassDate operator = (const tClassDate& sClassDate);
         */
        /// @brief      Operator = whithtDate.
        /// @param[in]	sValue consttDate
        /// @return tClassDate&
        tClassDate operator = (const tDate sValue);

        /// @brief      Operator ==.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator==(tClassDate& sClassDate);

        /// @brief      Operator !=.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator!=(tClassDate& sClassDate);

        /// @brief      Operator <.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator < (tClassDate& sClassDate);
        /// @brief      Operator >.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator > (tClassDate& sClassDate);
        /// @brief      Operator <=.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator <= (tClassDate& sClassDate);
        /// @brief      Operator >=.
        /// @param[in]	sClassDate tClassDate&
        /// @return tBool
        tBool operator >= (tClassDate& sClassDate);

        /// @brief      Operator ++ for add nb  day
        /// @param[in]  sClassInt tClassInt nb day
        /// @return     tClassDate&
        tClassDate operator +(tClassInt sClassInt);
        
        /// @brief      Operator ++ for add nb  day
        /// @param[in]  sInt tInt nb day
        /// @return     tClassDate&
        tClassDate operator +(tInt sInt);

        /// @brief      Operator -- for substract nb  day
        /// @param[in]  sClassInt tClassInt nb day
        /// @return     tClassDate&
        tClassDate operator -(tClassInt sClassInt);
    
        /// @brief      Operator -- for substract nb  day
        /// @param[in]  sInt tInt nb day
        /// @return     tClassDate&
        tClassDate operator -(tInt sInt);
        
        /// @brief      Operator ++ for add nb  day
        /// @param[in]  sClassDouble  tClassDouble nb day &  hourrs 0,x
        /// @return     tClassDate&
        tClassDate operator +(tClassDouble sClassDouble);
        
        /// @brief      Operator ++ for add nb  day & hours 0,x
        /// @param[in]  sDouble tDouble  nb day
        /// @return     tClassDate&
        tClassDate operator +(tDouble sDouble);

        /// @brief      Operator -- for substract nb   day & hours 0,x
        /// @param[in]  sDouble  tDouble  nb day
        /// @return     tClassDate&
        tClassDate operator -(tDouble sDouble);
    
        /// @brief      Operator -- for substract nb  day & hours 0,x
        /// @param[in]  sClassDouble  tClassDouble  nb day
        /// @return     tClassDate&
        tClassDate operator -(tClassDouble sClassDouble);
        
        /// @brief      Operator ++ add tClassDate
        /// @param[in]  sClassDate  tClassDate
        /// @return     tClassDate&
        tClassDate operator +(tClassDate sClassDate);
        
        /// @brief      Operator ++ add tClassDate
        /// @param[in]  sClassDate  tClassDate
        /// @return     tClassDate&
        tClassDate operator -(tClassDate sClassDate);
        
        
        /// @brief      Operator () for get result
        /// @return    tDate
        tDate operator()();

        /// @brief      Operator << for friend ostream 
        /// @param[in]  os ostream& stream ouput 
        /// @param[in]  sClassDate tClassDate& date  
        friend ostream& operator<<(ostream& os, tClassDate& sClassDate);
    };

    //=========================================================================
    //! Type of  error 
    enum class tTypeError : tChar {
        t_none = 0,
        t_value,
        t_div0,
        t_ref,
        t_num,
        t_name,
        t_recursive,
        t_arg,
        t_class,
        t_na,
        t_matrix,
        t_spill,
        // Dynamic-array calculation failure (Excel #CALC!), e.g. an empty spill result.
        // Kept last so existing serialized error codes (CodeInt) keep their integer values.
        t_calc,
        // Sentinel for a LAMBDA optional parameter that was not supplied by the caller. Only ISOMITTED
        // inspects it; any other use propagates like #VALUE! (Excel behavior for a missing argument).
        // Kept last so existing serialized error codes (CodeInt) keep their integer values.
        t_omitted,
    };

    class tSharedString;
    //=========================================================================
    //! Class error (boxing)
    class tClassError : public tClass {
        private:
            tTypeError	m_Code;
            tString		m_String;
        public:
            /// @brief      Constructor tClassError.
            tClassError();

            /// @brief      Constructor tClassError with Code and Message.
            /// @param[in]	sCode SkTypeError
            /// @param[in]	sString tString
            tClassError(tTypeError sCode, tString sString);

            /// @brief      Constructor tClassError of copy.
            /// @param[in]	sClassError const tClassError&
            tClassError(const tClassError& sClassError);

            /// @brief      Destructor tClassError.
            ~tClassError();

            /// @brief      SetError with Code and Message.
            /// @param[in]	sCode SkTypeError
            /// @param[in]	sString tString
            void SetError(tTypeError sCode, tString sString);

            /// @brief      Return code.
            /// @return SkTypeError
            tTypeError Code();

            /// @brief		Return code in integer.
            /// @return		tInt
            tInt CodeInt();
            
            /// @brief      Set code in integer.
            /// @param[in]  sCode tInt
            void CodeInt(tInt sCode);

            /// @brief      Return Message.
            /// @return tString
            tString String();

            /// @brief      Return Message with code.
            /// @return tString
            tString Error();

            /// @brief      Operator ==.
            /// @param[in]	sClassError tClassError&
            /// @return tBool
            tBool operator==(tClassError& sClassError);

            // Json ===============================================================
            /// @brief		Writer Json. 
            /// @param[in]	sWriter rapidjson::Writer<rapidjson::StringBuffer>*
            void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

            /// @brief		Reader Json. 
            /// @param[in]	sValue Value&
            void Json(const rapidjson::Value& sValue);

    };

}; // end of namespace ========================================================
#endif
