//======================================================================
// FILE: SkFunctionText.hpp
// DATE: 2025-08-23
//======================================================================
#ifndef SkFunctionText_hpp
#define SkFunctionText_hpp

#include "SkFunction.hpp"

namespace SkSpreadSheet {

    // Concat
    class tFunctionConcat : public tFunction {
    private:
        tString m_Result;
        
        void Concat(tVariant sValue);
    public:
        /// @brief      Constructor
        tFunctionConcat();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };
    
    // Left
    class tFunctionLeft : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionLeft();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Right
    class tFunctionRight : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionRight();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Mid
    class tFunctionMid : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionMid();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Len
    class tFunctionLen : public tFunction {
        public:
            /// @brief      Constructor
            tFunctionLen();
            /// @brief      Call function
            /// @param[in]  sArgVector tStackElemVector*
            /// @return     tVariant
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Find
    class tFunctionFind : public tFunction {
        public:
            /// @brief      Constructor
            tFunctionFind();
            /// @brief      Call function
            /// @param[in]  sArgVector tStackElemVector*
            /// @return     tVariant
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Exact (Excel EXACT) — case-sensitive text equality → boolean.
    class tFunctionExact : public tFunction {
    public:
        tFunctionExact();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Search
    class tFunctionSearch : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionSearch();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Replace
    class tFunctionReplace : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionReplace();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Substitute
    class tFunctionSubstitute : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionSubstitute();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    
    };

    // Upper
    class tFunctionUpper : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionUpper();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };
    
    // Lower
    class tFunctionLower : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionLower();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };
    
    // Proper
    class tFunctionProper : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionProper();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Trim
    class tFunctionTrim : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionTrim();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // CLEAN — remove ASCII control characters (codes 0..31) from text.
    class tFunctionClean : public tFunction {
    public:
        tFunctionClean();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Rept
    class tFunctionRept : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionRept();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Text
    class tFunctionText : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionText();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! T(value) — return value if text, otherwise "" (errors propagate).
    //=========================================================================
    class tFunctionT : public tFunction {
    public:
        tFunctionT();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! FIXED(number, [decimals=2], [no_commas=FALSE]) — number as fixed-decimal text.
    //=========================================================================
    class tFunctionFixed : public tFunction {
    public:
        tFunctionFixed();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! DOLLAR(number, [decimals=2]) — number as $-currency text.
    //=========================================================================
    class tFunctionDollar : public tFunction {
    public:
        tFunctionDollar();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! VALUETOTEXT(value, [format]) — scalar to text; format 0 concise (default), 1 strict.
    //=========================================================================
    class tFunctionValueToText : public tFunction {
    public:
        tFunctionValueToText();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! ARRAYTOTEXT(array, [format]) — array as a single text; format 0 concise, 1 strict.
    //=========================================================================
    class tFunctionArrayToText : public tFunction {
    public:
        tFunctionArrayToText();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // Value
    class tFunctionValue : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionValue();
        /// @brief      Call function
        /// @param[in]  sArgVector tStackElemVector*
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // NUMBERVALUE(text, [decimal_separator], [group_separator]) — locale-independent parse.
    class tFunctionNumberValue : public tFunction {
    public:
        tFunctionNumberValue();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // CHAR / CODE — shared (sToChar=true → CHAR 1..255; false → CODE of first byte).
    class tFunctionCharCode : public tFunction {
    private:
        tBool m_ToChar;
    public:
        explicit tFunctionCharCode(tBool sToChar);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // UNICHAR / UNICODE — shared (sToChar=true → UNICHAR; false → UNICODE of first code point).
    class tFunctionUniCharCode : public tFunction {
    private:
        tBool m_ToChar;
    public:
        explicit tFunctionUniCharCode(tBool sToChar);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // TextAfter / TextBefore (share one class; m_After picks the side)
    class tFunctionTextAfterBefore : public tFunction {
    private:
        //! true -> TEXTAFTER, false -> TEXTBEFORE.
        tBool m_After;
    public:
        /// @brief      Constructor
        /// @param[in]  sAfter true for TEXTAFTER, false for TEXTBEFORE
        explicit tFunctionTextAfterBefore(tBool sAfter);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // TextJoin
    class tFunctionTextJoin : public tFunction {
    public:
        /// @brief      Constructor
        tFunctionTextJoin();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    // TextSplit (Excel TEXTSPLIT) — returns a spilled t_Array
    class tFunctionTextSplit : public tFunction {
    public:
        tFunctionTextSplit();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! REGEXTEST / REGEXREPLACE / REGEXEXTRACT (ECMAScript via std::regex; not PCRE2).
    //=========================================================================
    class tFunctionRegexTest : public tFunction {
    public:
        tFunctionRegexTest();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionRegexReplace : public tFunction {
    public:
        tFunctionRegexReplace();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionRegexExtract : public tFunction {
    public:
        tFunctionRegexExtract();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

}
#endif // SkFunctionText_hpp
