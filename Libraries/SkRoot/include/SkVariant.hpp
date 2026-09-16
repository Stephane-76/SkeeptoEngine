//=============================================================================
// Skeema Variant
/**
* @page SkVariant
* @par
* @par Manage variants for all applications (like spreadsheet).
* @par  uses:
* @par		tInt			m_Int;
* @par		tBool			m_Bool;
* @par		tDouble		m_Double;
* @par		tSharedString* m_String;
* @par		tClassError*	m_Error;
* @par		tDate			m_Date;
* @par		tVirtualClass* m_Class;
* @par
*/
//=============================================================================
#ifndef SkVariant_hpp
#define SkVariant_hpp

#include <limits.h>

#include "SkTypes.hpp"
#include "SkTypesClass.hpp"
#include "SkSharedString.hpp"
#include "SkException.hpp"

#ifdef __EMSCRIPTEN__
//#pragma message ("Emscripten")
#define __EMSCRIPTEN__32__
#endif	
#ifdef __wasm64__
//#pragma message("__wasm64__") 
#endif	

#ifdef __wasm32__
//#pragma message("__wasm32__")
#endif	


using namespace rapidjson;
namespace SkRoot {

    //! SKVariant Union Contains the different elements 8 bytes
    union tVariant_Union { 
        tInt			m_Int;
        tBool			m_Bool;
        tDouble			m_Double;
        tSharedString* 	m_String;
        tClassError*	m_Error;
        tDate			m_Date;
        tVirtualClass*  m_Class;
    };

    //=========================================================================
    //! Type of Variant 
    enum class tVariantType : tChar {
        t_null,
        t_int,
        t_bool,
        t_double,
        t_string,
        t_error,
        t_date,
        t_class
    };

    tString VariantType2Str(tVariantType sVariantType);
    
    tVariantType Str2VariantType(tString sVariantType);
    
    //=========================================================================
    //! Variant 16 bytes (Element for storing multiple type of value). Used by SpreadSheet 
    class alignas(SkAlign) tVariant : public tClass {
    private:
        tVariantType  	m_Type;	 // 1 Bytes
        tShort			m_Extra; // 2 Bytes (bytes available for extra stockage) -32768 +32767
        tVariant_Union	m_Union; // 8 bytes
    public:
        inline void Assign(const tVariant& sVariant);
    public:
        /// @brief      Constructor
        tVariant();
        
        /// @brief      Constructor copy
        /// @param[in]  sVariant const tVariant&
        tVariant(const tVariant& sVariant);

        /// @brief      Constructor by Int
        /// @param[in]  sValue tInt
        tVariant(tInt sValue);

        /// @brief      Constructor by boolean
        /// @param[in]  sValue tBool
        tVariant(tBool sValue);
        
        /// @brief      Constructor by double
        /// @param[in]  sValue tDouble
        tVariant(tDouble sValue);
        
        /// @brief      Constructor by string
        /// @param[in]  sValue tString
        tVariant(tString sValue);
        
        /// @brief      Constructor by Char*
        /// @param[in]  sValue tChar*
        tVariant(tChar* sValue);
        
        /// @brief      Constructor by Class Error
        /// @param[in]  sValue tClassError
        tVariant(tClassError sValue);
        
        /// @brief      Constructor by const tChar
        /// @param[in]  sValue tChar*
        tVariant(const tChar* sValue);

        /// @brief      Constructor by Date
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
        /// @param[in]  sValue tDate
        tVariant(tDate sValue);
#endif	
        /// @brief      Constructor by Virtual Class
        /// @param[in]  sValue tVirtualClass*
        tVariant(tVirtualClass* sValue);
        
        /// @brief      Destructor
        virtual ~tVariant();

        /// @brief      Clear (String Class...)
        void Clear();

        /// @brief      Return String value
        /// @return     tString
        tString Str() const;
        
        /// @brief      Return type
        /// @return     tVariantType
        tVariantType Type() const;

		/// @brief    Set integer
        /// @param[in]  sValue tInt
        void SetInt(tInt sValue);

        /// @brief Get integer value
        /// @return tInt
        tInt Int() const;

        /// @brief    Set boolean
        /// @param[in]  sValue tBool
		void SetBool(tBool sValue);

        /// @brief Get boolean value
        /// @return tBool
        tBool Bool() const;

       
		 /// @brief    Set double
        /// @param[in]  sValue tDouble
		void SetDouble(tDouble sValue);

        /// @brief Get double value
        /// @return tDouble
        tDouble Double() const;

		 /// @brief    Set String
        /// @param[in]  sValue tString
        void SetString(tString sValue);

        /// @brief Get string value
        /// @return tString
        tString String() const;

		/// @brief    Set Char
        /// @param[in]  sValue tChar
        void SetChar(tChar* sValue);

        /// @brief    Set Error
        /// @param[in]  sValue tClassError
		void SetError(tClassError sValue);

        /// @brief Get error value
        /// @return tClassError
        tClassError Error() const;

		/// @brief    Set Date
        /// @param[in]  sValue tDate
        void SetDate(tDate sValue);

        /// @brief Get date value
        /// @return tDate
        tDate Date() const;

		    /// @brief Get Class
        /// @ret
        void SetClass(tVirtualClass* sValue);

        /// @brief Get class value
        /// @return tVirtualClass*
        tVirtualClass* Class() const;
        
        template<class T>
        T* Class() const {
            if (m_Type==tVariantType::t_class) {
                return(dynamic_cast<T*>(m_Union.m_Class));
            }
            return(nullptr);
        }

        tShort Extra() const;
        void Extra(tShort sValue);
        
        tBool IsNull() const;
        tBool IsInt() const;
        tBool IsBool() const;
        tBool IsDouble() const;
        tBool IsString() const;
        tBool IsError() const;
        tBool IsDate() const;
        
        tBool IsClass() const;
        
        tBool IsNumeric() const;
        tBool IsNumericOrNull() const;
        
        tDouble Numeric() const;
        tDouble NumericOrNull() const;
        
        tBool IsExcelNull() const;
        
        tBool HasFormula() const;

        tVariantType Parse(tString sValue);

        /// @brief      Return formula =formula
        /// @return     tString
        tString Formula() const;
        
        /// @brief      Return FormatString
        /// @param[in]  sFormatString  tString
        /// @return     tString
        tString FormatString(tFormatString* sFormatString) const;
        
        /// @brief        Retun Input string
        /// @param[in]  sFormatString  tString
        /// @return       tString
        tString InputString(tFormatString* sFormatString) const;
        
        // Json ==============================================================
        /// @brief		Writer Json. 
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) const;

        /// @brief		Reader Json Class. 
        /// @param[in]	sValue Value&
        void JsonClass(const rapidjson::Value& sValue);

        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        void Json(const rapidjson::Value& sValue);
        
        // React =============================================================
        /// @brief        Writer Json for Javasript.
        /// @param[in]    sWriter Writer<StringBuffer>*
        virtual void JsonJavaScript(Writer<StringBuffer>* sWriter) const;
        
        virtual tVariant operator = (const tVariant& sVariant);

        tBool operator == (const tVariant& sVariant) const;
        tBool operator != (const tVariant& sVariant) const;
        tBool operator < (const tVariant& sVariant) const;
        tBool operator <= (const tVariant& sVariant) const;
        tBool operator > (const tVariant& sVariant) const;
        tBool operator >= (const tVariant& sVariant) const;

        tBool operator || (const tVariant& sVariant) const;
        tBool operator && (const tVariant& sVariant) const;
        tBool operator ! () const;
        
        // friend method ======================================================
        friend tVariant operator+(const tVariant& sVariant1, const tVariant& sVariant2);
        friend tVariant operator-(const tVariant& sVariant1, const tVariant& sVariant2);
        friend tVariant operator*(const tVariant& sVariant1, const tVariant& sVariant2);
        friend tVariant operator/(const tVariant& sVariant1, const tVariant& sVariant2);
        // Excel "&" concatenation operator: coerces both operands to text
        // before concatenating (unlike "+", which keeps numeric semantics).
        friend tVariant Ampersand(const tVariant& sVariant1, const tVariant& sVariant2);
    };

    typedef vector<tVariant> tVectorVariant;
    typedef stack<tVariant> tStackVariant;

    ostream& operator<<(ostream& sStream, const tVariant& sValue);
    
    //=========================================================================
    //! Class Ancestor for Class Variant 
    class tVariantClass : public tVirtualClass {
    protected:
        tVariant m_Value;
    public:
        /// @brief Constructor
        tVariantClass();
        
        /// @brief Constructor with value
        /// @param[in] sValue tVariant
        tVariantClass(tVariant sValue); 
        
        /// @brief Copy constructor
        /// @param[in] sVariantClass const tVariantClass&
        tVariantClass(const tVariantClass& sVariantClass);

        /// @brief Clone derived class
        /// @return tVirtualClass*
        tVirtualClass* Clone() override;
        
        /// @brief Set value
        /// @param[in] sValue tVariant*
        void Value(tVariant* sValue) override;
        
        /// @brief Get value
        /// @return tVariant*
        tVariant* Value() override;
        
        // React =============================================================
        /// @brief        Writer Json for Javasript.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonJavaScript(Writer<StringBuffer>* sWriter) override;
        
        tVariant Operator_plus(tBool sLeft, const tVariant& sVariant) override;
        tVariant Operator_minus(tBool sLeft, const tVariant& sVariant) override;
        tVariant Operator_multiply(tBool sLeft, const tVariant& sVariant) override;
        tVariant Operator_divide(tBool sLeft, const tVariant& sVariant) override;
    };
    
    


}; // end of namespace ========================================================
#endif
