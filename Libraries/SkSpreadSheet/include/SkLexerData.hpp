//=============================================================================
// SkLexerData.hpp
// Stephane Allez
// 2026-02-11
//=============================================================================

 #ifndef SkLexer_hpp
 #define SkLexer_hpp


#include <SkTypes.hpp>
#include <SkTypesClass.hpp>
#include "SkLexerSpreadSheet.hpp"

namespace SkSpreadSheet {

    class tLexerData : public tClass {
     private:
         tLexer        m_Lexer;
         tLexerToken*  m_Table;
         
        // Séparator country
         tChar         m_Arg;
        
        // Table element
         tTypeData m_SpecialKey;
         tString   m_Col1;
         tString   m_Col2;
         
         tIndex    m_NumCol1;
         tIndex    m_NumCol2;
         
         tSize     m_IndexCol1;
         tSize     m_IndexCol2;
         
         tVectorString m_FormulaKey;
         
        
     public:
        /// @brief
        /// @param[in] sSheet tLexerToken*
        /// @param[in] sTable tLexerToken*
        /// @param[ind]  sArg tChar
        tLexerData(tLexerToken* sTable,tChar sArg);
         /// @brief Destructor
         ~tLexerData();

       
        /// @brief Get file name
        tString Table() const;
        /// @brief Get special key
        tTypeData SpecialKey() const;
        /// @brief Get column 1
        tString Col1() const;
        /// @brief Get column 2
        tString Col2() const;

        ///@brief Get index column 1
        ///@return tIndex
        tIndex  NumCol1();
        ///@brief Set index column 1
        ///@param[in] sIndex tIndex
        void  NumCol1(tIndex sIndex);

        ///@brief Get index column 2
        ///@return tIndex
        tIndex  NumCol2();
        ///@brief Set index column 2
        ///@param[in] sIndex tIndex
        void  NumCol2(tIndex sIndex);
        
        
        ///@brief return token formula
        tString TokenFormula();

        /// @brief Parse Origin
        /// @param[in] sLexerToken tLexerToken*
        /// @return tBool
        tBool Parse(tLexerToken* sLexerToken);
     };
}; // end of name space
 #endif // SkLexer_hpp
