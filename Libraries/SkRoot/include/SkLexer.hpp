//=============================================================================
// SkLexer  Lexer for Lemon or direct Interface 
//=============================================================================
#ifndef SkLexer_hpp
#define SkLexer_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"

using namespace std;

namespace SkRoot {
    //! Kind of lexer
    enum class tKind :  tChar {
        Integer,
        Float,
        Identifier,
        Bool,
        LeftParen,
        RightParen,
        LeftSquare,
        RightSquare,
        LeftCurly,
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
        Exclamation,
        Ampersand,
        At,
        Hash,
        HexaString,
        Dot,
        Comma,
        Colon,
        Semicolon,
        LabelSimple,
        LabelDouble,
        Comment,
        Pipe,
        Dollar,
        Percent,
        NewLine,
        End,
        Unexpected
    };

    typedef stack<tKind> tStackKind;
    
    //=========================================================================
    //! Token used by lexer. Allocated in Circular Memory
    class tLexerToken  {
    private:
        //! Kind
        tKind            m_Kind;
        //! Lexeme content
        const tChar*     m_Begin;
        const tChar*     m_End;
     
    public:
        /// @brief Default constructor
        tLexerToken() noexcept;
        
        /// @brief Constructor with kind
        /// @param[in] sKind tKind
        tLexerToken(tKind sKind) noexcept;

        /// @brief Constructor with kind and selection char
        /// @param[in] kind tKind
        /// @param[in] beg tChar* pointer of begin char
        /// @param[in] len tSize length of selection
        tLexerToken(tKind kind, const tChar* beg, tSize len) noexcept;

        /// @brief Constructor with kind and selection char
        /// @param[in] kind tKind
        /// @param[in] beg tChar* pointer of begin char
        /// @param[in] end tChar* pointer of end char
        tLexerToken(tKind kind, const tChar* beg, const tChar* end) noexcept;

        /// @brief Copy constructor
        /// @param[in] sSkLexerToken const tLexerToken&
        tLexerToken(const tLexerToken& sSkLexerToken) noexcept;

        /// @brief Destructor
        ~tLexerToken();

        /// @brief Return Kind
        /// @return tKind
        tKind Kind() const noexcept;

        /// @brief Set Kind
        /// @param[in] sValue tKind
        void Kind(tKind sValue) noexcept;

        /// @brief Test if Kind is sKind
        /// @param[in] sKind tKind
        /// @return tBool
        tBool is(tKind sKind) const noexcept;

        /// @brief Test if Kind is not sKind
        /// @param[in] sKind tKind
        /// @return tBool
        tBool is_not(tKind sKind) const noexcept;

        /// @brief Test if Kind is sK1 or sK2
        /// @param[in] sK1 tKind
        /// @param[in] sK2 tKind
        /// @return tBool
        tBool is_one_of(tKind sK1, tKind sK2) const noexcept;

        /// @brief Test if Kind is sK1 or sK2 or in list
        /// @param[in] sK1 tKind
        /// @param[in] sK2 tKind
        /// @param[in] sKs Ts...
        /// @return tBool
        template <typename... Ts>
        tBool is_one_of(tKind sK1, tKind sK2, Ts... sKs) const noexcept;

        /// @brief Return lexeme 
        /// @return tString
        const tString Lexeme() const noexcept;

        /// @brief Return Begin
        /// @return const tChar*
        const tChar* Begin() const noexcept;
        
        /// @brief New for use circular memory
        /// @param[in] sz tSize size to alloc
        /// @return void*
        void* operator new(tSize sz) noexcept;

        /// @brief Delete for use circular memory (do nothing)
        /// @param[in] ptr void*
        void operator delete(void* ptr) noexcept;
    };

    typedef stack<tLexerToken*> SkStackLexerToken;
    
    //=========================================================================
    //! struct Token used by lexer
    struct tToken {
    public:
        //! Token for parser
        tLexerToken* m_Token;
        //! Current line
        tInt          m_Line;
        //! Current column
        tInt          m_Column;
    };

    //=========================================================================
    //! Lexer for variant date number
    class tLexer {
    private:
        tChar m_SeparatorDecimal;
        //! Current line
        tInt m_Line;
        //! Current column
        tInt m_Column;

        //! Pointer of char for make string
        const tChar* m_beg = nullptr;

        /// @brief Return identifier
        /// @return tLexerToken
        tLexerToken identifier() noexcept;

        /// @brief Return number
        /// @return tLexerToken
        tLexerToken number() noexcept;

        /// @brief Return label with " " 
        /// @return tLexerToken
        tLexerToken label_double_quote() noexcept;

        /// @brief Return label with ' ' 
        /// @return tLexerToken
        tLexerToken label_simple_quote() noexcept;

        /// @brief Return str hexa like  #FG4500
        /// @return tLexerToken
        tLexerToken HexaString() noexcept;
        
        /// @brief Alloc tLexerToken
        /// @param[in] sKind tKind
        /// @return tLexerToken
        inline tLexerToken atom(tKind sKind) noexcept;

        /// @brief Return the last character
        /// @return tChar
        inline tChar peek() const noexcept;

        /// @brief Return the last character and skip
        /// @return tChar
        inline tChar get() noexcept;

    public:
        /// @brief Default constructor
        tLexer() noexcept;
        
        /// @brief Constructor with pointer of char
        /// @param[in] beg const tChar*
        tLexer(const tChar* beg) noexcept;
        
        /// @brief Set decimal separator
        /// @param[in] sSeparatorDecimal tChar
        void SeparatorDecimal(tChar sSeparatorDecimal) noexcept;

        /// @brief Search next item with notation A1
        /// @return tLexerToken
        tLexerToken next() noexcept;

        /// @brief Get current column in lexer
        /// @return tInt
        tInt Column();

        /// @brief Get current line in lexer
        /// @return tInt
        tInt Line();
    };

}
#endif
