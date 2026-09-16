//=============================================================================
// SkLexerSpreadSheet Lex SkRoot for SpreadSheet
//=============================================================================
#ifndef SkLexerSpreadSheet_hpp
#define SkLexerSpreadSheet_hpp

#include <SkTypes.hpp>
#include <SkClass.hpp>
#include "SkTools.hpp"
using namespace SkRoot;
using namespace std;

namespace SkSpreadSheet {

    class tCell;
    //! Kind of lexer
    enum class tKind : tChar {
        Integer,
        Float,
        Identifier,
        Bool,
        Sheet,
        Cell,
        Attribute,
        Range,
        /// Dynamic range: one or both bounds from expression (e.g. A1:INDEX(...) or INDEX(...):B2). Resolved at calculation time.
        DynamicRangeRight,  // left = static cell, right = from stack (e.g. CELL COLON function)
        DynamicRangeLeft,   // left = from stack, right = static cell (e.g. function COLON CELL)
        DynamicRangeBoth,   // both bounds from stack (e.g. Function1():Function2())
        LeftParen,
        RightParen,
        LeftSquare, // Table
        RightSquare,
        LeftCurly, // List
        RightCurly,
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
        Equal,
        NotEqual,
        Plus,
        Minus,
        Times,
        Divide,
        UnaryMinus,
        Exclamation,
        Ampersand,
        Hash,
        Dot,
        Comma,
        Colon,
        Semicolon,
        Percent,
        LabelSimple,
        LabelDouble,
        LabelSquare,
        Comment,
        Pipe,
        Dollar,
       
        Function,
        Name,
        ErrorRef,
        ErrorName,
        NewLine,
        End,
        Unexpected,
        NotCell,
        // LET local-variable opcodes (bytecode-only; never produced by the lexer).
        // They implement an evaluation-time scope stack for Excel's LET(name, value, ..., body).
        LetBeginScope,  // push a new empty local scope
        LetBind,        // pop the stack top and bind it to the carried name in the current scope
        LetVarRef,      // push the value bound to the carried name (searched innermost-first)
        LetEndScope,    // pop the current local scope (the body result stays on the stack)
        LambdaRef,      // push a first-class LAMBDA value for the carried named-lambda name (higher-order funcs)
        IfCond,         // short-circuit IF: pop condition, then run only the taken branch (see IfElse/IfEnd)
        IfElse,         // short-circuit IF: boundary between the then-branch and the else-branch
        IfEnd,          // short-circuit IF: end marker (the chosen branch's value stays on the stack)
        /// Excel spilled-range operator D14# / Sheet!D14#: same VectorRef as Cell, expands to SpillRange at eval.
        SpillRef,
    };
     enum class tTypeData : tByte { t_ThisRow, t_Column, t_Headers, t_Totals, t_All };
     
    //=========================================================================
    //! Token used by lexer Allocated in Circular Memory
    class tLexerToken  {
    private:
        //! Kind
        tKind            m_Kind;
        //! Lexeme content
        const tChar*    m_Begin;
        const tChar*    m_End;

        // Cell or Range ======================================================
        //! m_LockRow for A$1 by example lock row 1
        tBool            m_LockRow;
        //! m_Row string by example  "1" 
        const tChar*     m_RowBegin;
        const tChar*     m_RowEnd;

        //! m_LockCol for $A1 by example lock col A
        tBool            m_LockCol;
        
        //! m_Col string by example  "A"
        const tChar*     m_ColBegin;
        const tChar*     m_ColEnd;
        
        //! Cell row position (Or Diff for R1C1 notation)
        tInt             m_RowInt;
        //! Cell col position (Or Diff for R1C1 notation)
        tInt             m_ColInt;

        //! R1C1 notation active
        tBool            m_R1C1;

        //! Pos Cell Lex and pos Lemon
        tInt             m_IndLex;
    public:
        tLexerToken() noexcept;
        /// @brief      Constructor with kind.
        /// @param[in]  sKind tKind
        tLexerToken(tKind sKind) noexcept;

        /// @brief      Constructor with kind and selection char.
        /// @param[in]  kind tKind
        /// @param[in]  sBegin tChar* pointer of begin char
        /// @param[in]  sLen tSize length of selection
        tLexerToken(tKind kind, const tChar* sBegin, tSize sLen) noexcept;

        /// @brief      Constructor with kind and selection char
        /// @param[in]  kind tKind
        /// @param[in]  sBegin  tChar* pointer of begin char
        /// @param[in]  sEnd  tChar* pointer of end char
        tLexerToken(tKind kind, const tChar* sBegin, const tChar* sEnd) noexcept;

        /// @brief      Constructor for cell ref
        /// @param[in]  kind tKind
        /// @param[in]  sBegin tChar* pointer of begin Lexeme
        /// @param[in]  sEnd  tChar* pointer of end Lexeme
        /// @param[in]  sLockRow tBool  Is lock Row A$1 for example
        /// @param[in]  sRowBegin  tChar* pointer for first caracter row
        /// @param[in]  sRowEnd  tChar* pointer for last caracter row
        /// @param[in]  sLockCol tBool  Is lock Col $A1 for example
        /// @param[in]  sColBegin  tChar* pointer for first caracter col
        /// @param[in]  dColEnd tChar* pointer for last caracter col
        /// @param[in]  sRowIndex tInt row index
        /// @param[in]  sColIndex tInt col index
        tLexerToken(tKind kind, const tChar* sBegin, const tChar* endLexeme,
            tBool sLockRow, const tChar* sRowBegin, const tChar* sRowEnd,
            tBool sLockCol, const tChar* sColBegin, const tChar* sColEnd,
            tInt sRowIndex,tInt sColIndex, tBool sR1C1 = false) noexcept;

        /// @brief      Constructor of copy
        /// @param[in]  sSkLexerToken const SkLexerToken&
        tLexerToken(const tLexerToken& sSkLexerToken) noexcept;

        /// @brief      destructor
        ~tLexerToken();

        /// @brief      Return Kind
        /// @return     tKind
        tKind Kind() const noexcept;

        /// @brief      Set Kind
        /// @param[in]  sValue tKind
        void Kind(tKind sValue) noexcept;
        

        /// @brief      Test if Kind is sKind
        /// @param[in]  sKind tKind
        /// @return     tBool
        tBool is(tKind sKind) const noexcept;

        /// @brief      Test if Kind is not sKind
        /// @param[in]  sKind tKind
        /// @return     tBool
        tBool is_not(tKind sKind) const noexcept;

        /// @brief      Test if Kind is Sk1 or Sk2
        /// @param[in]  sK1 tKind
        /// @param[in]  sK2 tKind
        /// @return     tBool
        tBool is_one_of(tKind sK1, tKind sK2) const noexcept;

        /// @brief      Test if Kind is Sk1 or Sk2 or in list
        /// @param[in]  sK1 tKind
        /// @param[in]  sK2 tKind
        /// @param[in]  sKs Ts...
        /// @return     tBool
        template <typename... Ts>
        tBool is_one_of(tKind sK1, tKind sK2, Ts... sKs) const noexcept {
            return is(sK1) || is_one_of(sK2, sKs...);
        }

        /// @brief      Return lexeme 
        /// @return     tString
        const tString Lexeme() const noexcept;

        /// @brief      Return lexeme without single or double quotes at the beginning and end
        /// @return     tString
        const tString Unquote() const noexcept;
        
        /// @brief      Return lexeme without single or double quotes at the beginning and end
        /// @return     tString
        const tString PureLexeme() const noexcept;
 
        // Interface spreadsheet ==============================================
        /// @brief      Return row is lock
        /// @return     tBool
        tBool LockRow() const noexcept;

        /// @brief      Set row lock
        /// @param[in]  sValue tBool
        void LockRow(tBool sValue) noexcept;

        /// @brief      Return row string "12"
        /// @return     tString
        const tString Row() const noexcept;

        /// @brief      Return col is lock
        /// @return     tBool
        tBool LockCol() const noexcept;

        /// @brief      Set col lock
        /// @param[in]  sValue tBool
        void LockCol(tBool sValue) noexcept;

        /// @brief      Return col string "AB"
        /// @return     tString
        const tString Col() const noexcept;

       
        /// @brief      R1C1 notation
        /// @return     tBool
        tBool R1C1();

        /// @brief      Return col string convert to integer
        /// @return     tInt
        tInt ColInt();
        /// @brief      Return col string convert to integer
        /// @return     tInt
        tInt RowInt();
        
        /// @brief      Return Key Formula !$C$L  for Sheet $A$1
        /// @return     tString
        tString FormulaKey();

        //! Pos Cell Lex and pos Lemon
        void IndLex(tInt sIndex);
        tInt IndLex();

        /// @brief      new for use circular memory
        /// @return     sz tInt size to alloc
        void* operator new(tSize sz) noexcept;

        /// @brief      delete for use circular memory (do nothing)..
        /// @return     ptr void*
        void operator delete(void* ptr) noexcept;
    };
    // Type Stack & Vector tLexerToken
    typedef vector<tLexerToken*> tVectorLexerToken;
    typedef stack<tLexerToken*> tStackLexerToken;

    //=========================================================================
    //! Lexer for formula SpreadSheet
    class tLexer {
    private:
        tChar   m_SeparatorDecimal;
        // Lang Separator
        tChar   m_SeperatorArg;

        /// Nesting inside constant array { ... } and | ... | (comma is column separator, not decimal)
        tInt m_arrayLiteralDepth = 0;
        /// True between first and second PIPE of a |...| literal
        tBool m_inPipeLiteral = false;

        //! Current line
        tInt m_Line;
        //! Current column
        tInt m_Column;

        //! Pointeur of char for make string
        const tChar* m_beg = nullptr;
        //! Pointer to formula start (for local context checks around ':')
        const tChar* m_formulaBegin = nullptr;
        //! Pointeur of begin row
        const tChar* m_begRow = nullptr;
        //! Pointeur of end row
        const tChar* m_endRow = nullptr;
        //! Pointeur of begin col
        const tChar* m_begCol = nullptr;
        //! Pointeur of end col
        const tChar* m_endCol = nullptr;
        
        /// @brief      Is Identifier Char
        /// @param[in]  tChar
        tBool is_identifier_char(tChar c) noexcept;
       
         
        /// @brief      Return identifier or sheet example ABCDEF or Sheet1!
        /// @return     SkLexerToken
        tLexerToken identifier_or_sheet() noexcept;
        
        
        /// @brief      Return number
        /// @return     SkLexerToken
        tLexerToken number() noexcept;

        /// @brief      Return divide or comment
        /// @return     SkLexerToken
        tLexerToken divide_or_comment() noexcept;

        /// @brief      Return less than or equal or not eqaul < <= <> 
        /// @return     SkLexerToken
        tLexerToken LessThan_or_Equal_or_NotEqual() noexcept;

        /// @brief      Return greater than or equal > >= 
        /// @return     SkLexerToken
        tLexerToken GreaterThan_or_Equal() noexcept;

        /// @brief      Return label with " " 
        /// @return     SkLexerToken
        tLexerToken label_double_quote() noexcept;

        /// @brief      Return label with ' ' 
        /// @return     SkLexerToken
        tLexerToken label_simple_quote_or_sheet() noexcept;

        /// @brief      Return label with [ ] (including spaces, Excel compatible)
        /// @return     SkLexerToken
        tLexerToken label_square_bracket() noexcept;

        /// @brief      Return #REF !
        /// @return     SkLexerToken
        tLexerToken Is_ErrorRef() noexcept;

        /// @brief      Return #NAME?
        /// @return     SkLexerToken
        tLexerToken Is_ErrorName() noexcept;

        /// @brief      Helper function to match table special reference pattern
        /// @param[in]  sPattern const tChar* pattern to match (without '#')
        /// @param[in]  sKind tKind token kind to return on success
        /// @return     SkLexerToken
        tLexerToken Is_TableSpecial(const tChar* sPattern, tKind sKind) noexcept;

        /// @brief      Return #This Row (Excel table special reference)
        /// @return     SkLexerToken
        tLexerToken Is_TableThisRow() noexcept;

        /// @brief      Return #Headers (Excel table special reference)
        /// @return     SkLexerToken
        tLexerToken Is_TableHeaders() noexcept;

        /// @brief      Return #Total  (Excel table special reference)
        /// @return     SkLexerToken
        tLexerToken Is_TableTotals() noexcept;

        /// @brief      Return  is_sheet
        /// @return     SkLexerToken
        SkInline tLexerToken Is_sheet() noexcept;
        
        /// @brief      Return cell A1 notation (if $ before sLockCol = true)
        /// @param[in]  sLockCol tBool
        /// @return     SkLexerToken
        SkInline tLexerToken Is_cell_A1(tBool sLockCol = false) noexcept;

        /// @brief      Return cell lc notation (See formula key)
        /// @return     SkLexerToken
        SkInline tLexerToken Is_cell_Key() noexcept;

        /// @brief      Return cell RC  notation A1 -> R[1]C[1], $A$1 -> R1C1
       /// @return     SkLexerToken
        SkInline tLexerToken Is_cell_RC() noexcept;

        /// @brief      Return Attribute
        /// @return     SkLexerToken
        SkInline tLexerToken Is_Attribute() noexcept;

        /// @brief      Alloc SkLexerToken
        /// @param[in]  sKind tKind
        /// @return     SkLexerToken
        SkInline tLexerToken atom(tKind sKind) noexcept;

        /// Emit LeftCurly and track constant-array nesting
        tLexerToken atom_left_curly() noexcept;
        /// Emit RightCurly and pop constant-array nesting
        tLexerToken atom_right_curly() noexcept;
        /// Emit Pipe and toggle |...| literal nesting
        tLexerToken atom_pipe() noexcept;

        /// @brief      Return the last caracter and skip
        /// @return     SkLexerToken
        SkInline tChar get() noexcept;
        
           /// @brief      Return the last caracter
        /// @return     SkLexerToken
        SkInline tChar peek() const noexcept;
    public:
        /// @brief      Constructor.
        tLexer() noexcept;
        
        /// @brief      Constructor with pointer of char.
        /// @param[in]  beg tKind
        tLexer(const tChar* beg) noexcept;

        /// @brief      Search next item with notation A1.
        /// @return     SkLexerToken
        tLexerToken next() noexcept;
        
        /// @brief      Return the next non-space character without modifying m_beg
        /// @return     tChar the next non-space character, or '\0' if end of string
        tChar peek_skip_space() const noexcept;

        /// @brief      Search next item with notation LC.
        /// @return     SkLexerToken
        tLexerToken nextKey() noexcept;

        //! Current column in lexer
        tInt Column();
        //! Current line in lexer
        tInt Line();
        
        /// @brief      Set Arg  separator
        /// @param[in]  sSeparatorArg tChar
        void SeparatorArg(tChar sSeparatorArg);
        
        /// @brief      Get Arg  separator
        /// @return     tChar
        tChar SeparatorArg();
        
        /// @brief      Set Decimal  separator
        /// @param[in]  sSeparatorDecimal tChar
        void SeparatorDecimal(tChar sSeparatorDecimmal);
        
        /// @brief      GetDecimal   separator
        /// @return     tChar
        tChar SeparatorDecimal();

        /// Nesting depth for constant array { ... } (and |...| via atom_pipe); must be 0 at end of a formula.
        tInt ArrayLiteralDepth() const noexcept { return m_arrayLiteralDepth; }
        /// True between first and second | of a |...| literal.
        tBool InPipeLiteral() const noexcept { return m_inPipeLiteral; }
    };
    ostream& operator<<(ostream& os, const tKind& kind);

    ostream& operator<<(ostream& os, const tLexerToken& sSkLexerTokenk);

}
#endif // SkLexerSpreadSheet_hpp
