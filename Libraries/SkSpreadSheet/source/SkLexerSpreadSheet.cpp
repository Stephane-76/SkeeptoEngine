// Initial work (thank you)
// A simple Lexer meant to demonstrate a few theoretical concepts. It can
// support several parser concepts and is very fast (though speed is not its
// design goal).
//
// J. Arrieta
// (C) 2018 Nabla Zero Labs
// https://gist.github.com/arrieta lexer.cpp

#include "../include/SkLexerSpreadSheet.hpp"
#include <SkApplication.hpp>
#include <SkTypesClass.hpp>
#include <algorithm>
#include <cctype>
#include <iterator>

using namespace SkRoot;
using namespace std;

namespace SkSpreadSheet {

    tLexerToken::tLexerToken() noexcept : m_Kind{},
        m_Begin(nullptr),m_End(nullptr),
        m_LockRow(false), m_RowBegin(nullptr),m_RowEnd(nullptr),
        m_LockCol(false), m_ColBegin(nullptr),m_ColEnd(nullptr),
        m_RowInt(-1),m_ColInt(-1),m_R1C1(false), m_IndLex(-1) {}
    
    tLexerToken::tLexerToken(tKind sKind) noexcept : m_Kind{ sKind },
        m_Begin(nullptr),m_End(nullptr),
        m_LockRow(false), m_RowBegin(nullptr),m_RowEnd(nullptr),
        m_LockCol(false), m_ColBegin(nullptr),m_ColEnd(nullptr),
        m_RowInt(-1),m_ColInt(-1), m_R1C1(false),m_IndLex(-1) {}

    tLexerToken::tLexerToken(tKind kind, const tChar* sBegin, tSize sLen) noexcept: m_Kind{ kind },
        m_Begin(sBegin),m_End(sBegin+sLen),
        m_LockRow(false), m_RowBegin(nullptr),m_RowEnd(nullptr),
        m_LockCol(false), m_ColBegin(nullptr),m_ColEnd(nullptr),
        m_RowInt(-1), m_ColInt(-1), m_R1C1(false), m_IndLex(-1) {}

    tLexerToken::tLexerToken(tKind kind, const tChar* sBegin, const tChar* sEnd) noexcept: m_Kind{ kind },
        m_Begin(sBegin), m_End(sEnd),
        m_LockRow(false), m_RowBegin(nullptr),m_RowEnd(nullptr),
        m_LockCol(false), m_ColBegin(nullptr),m_ColEnd(nullptr),
        m_RowInt(-1), m_ColInt(-1), m_R1C1(false), m_IndLex(-1) {}

   tLexerToken::tLexerToken(tKind kind, const tChar* sBegin, const tChar* sEnd,
        tBool sLockRow, const tChar* sRowBegin, const tChar* sRowEnd,
        tBool sLockCol, const tChar* sColBegin, const tChar* sColEnd,
        tInt sRowIndex, tInt sColIndex, tBool sR1C1) noexcept :
        m_Kind{ kind },
        m_Begin(sBegin), m_End(sEnd),
        m_LockRow(sLockRow),
        m_RowBegin(sRowBegin), m_RowEnd(sRowEnd),
        m_LockCol(sLockCol),
        m_ColBegin(sColBegin), m_ColEnd(sColEnd),
        m_RowInt(sRowIndex), m_ColInt(sColIndex),
        m_R1C1(sR1C1), m_IndLex(-1) {
    }

    tLexerToken::tLexerToken(const tLexerToken& sLexerToken) noexcept {
        m_Kind = sLexerToken.m_Kind;
        m_Begin = sLexerToken.m_Begin;
        m_End = sLexerToken.m_End;
        m_LockRow = sLexerToken.m_LockRow;
        m_RowBegin = sLexerToken.m_RowBegin;
        m_RowEnd = sLexerToken.m_RowEnd;
        
        m_LockCol = sLexerToken.m_LockCol;
        m_ColBegin = sLexerToken.m_ColBegin;
        m_ColEnd=sLexerToken.m_ColEnd;

        m_RowInt = sLexerToken.m_RowInt;
        m_ColInt = sLexerToken.m_ColInt;

        m_R1C1 = sLexerToken.m_R1C1;

        m_IndLex = sLexerToken.m_IndLex;
    }

    tLexerToken::~tLexerToken() {}

    void* tLexerToken::operator new(tSize sz) noexcept {
        return(tApplication::Instance()->AllocTemporary(sz));
    }

    void tLexerToken::operator  delete(void* ptr) noexcept {

    }

    void tLexerToken::Kind(tKind sValue) noexcept { m_Kind = sValue; }

    tBool tLexerToken::is(tKind sKind) const noexcept {
        return m_Kind == sKind;
    }

    tBool tLexerToken::is_not(tKind sKind) const noexcept {
        return m_Kind != sKind;
    }

    tBool tLexerToken::is_one_of(tKind sK1, tKind sK2) const noexcept {
        return is(sK1) || is(sK2);
    }

    const tString tLexerToken::Lexeme() const noexcept {
        // Guard negative or huge spans (e.g. lexer edge cases): avoids std::length_error / wasm abort on copy.
        if (m_Begin == nullptr || m_End == nullptr) {
            return {};
        }
        const ptrdiff_t wDist = std::distance(m_Begin, m_End);
        if (wDist <= 0) {
            return {};
        }
        static constexpr tSize kMaxLexemeChars = 65536;
        tSize wLen = static_cast<tSize>(wDist);
        if (wLen > kMaxLexemeChars) {
            wLen = kMaxLexemeChars;
        }
        return tString(m_Begin, wLen);
    }

    const tString tLexerToken::Unquote() const noexcept {
        tString wLexeme = Lexeme();
        tClassString wClassString(wLexeme);
        return(wClassString.Unquote());
    }
    const tString tLexerToken::PureLexeme() const noexcept {
        if ((m_Kind==tKind::LabelDouble) || ((m_Kind==tKind::LabelSimple))) return(Unquote());
        return(Lexeme());
    }
 
    tBool tLexerToken::LockRow() const noexcept {
        return(m_LockRow);
    }

    void tLexerToken::LockRow(tBool sValue) noexcept {
        m_LockRow = sValue;
    }

    const tString tLexerToken::Row() const noexcept {
        return(tString(m_RowBegin,distance(m_RowBegin, m_RowEnd)));
    }
  
    tBool tLexerToken::LockCol() const noexcept {
        return(m_LockCol);
    }

    void tLexerToken::LockCol(tBool sValue) noexcept { m_LockCol = sValue; }
    
    const tString tLexerToken::Col() const noexcept {
        return(tString(m_ColBegin,distance(m_ColBegin, m_ColEnd)));
    }

    
    tBool tLexerToken::R1C1() { return(m_R1C1); };

    tKind tLexerToken::Kind() const noexcept { return m_Kind; }
    tInt tLexerToken::ColInt() { return(m_ColInt); }
    tInt tLexerToken::RowInt() { return(m_RowInt); }

    tString tLexerToken::FormulaKey() {
        tStringStream wStream;
        if (m_LockCol) wStream << "$";
        wStream << "C";
        if (m_LockRow) wStream << "$";
        wStream << "R";
        return(wStream.str());
    }

    void tLexerToken::IndLex(tInt sIndex) { m_IndLex = sIndex; }
    tInt tLexerToken::IndLex() { return(m_IndLex); }

    // SkLexer ====================================================================
    tLexer::tLexer() noexcept : m_Line(1),m_Column(0), m_beg(nullptr), m_formulaBegin(nullptr), m_begRow(nullptr), m_endRow(nullptr), m_begCol(nullptr), m_endCol(nullptr) {};
    tLexer::tLexer(const tChar* beg) noexcept : m_Line(1),m_Column(0), m_beg{ beg }, m_formulaBegin{ beg } {}


    tBool is_space(tChar c) noexcept {
        switch (c) {
        case ' ':   // Space
        case '\t': // Tabulation
        case '\r': // Carriage return
            return true;
        default:
            return false;
        }
    }

    tBool is_digit_char(tChar c) noexcept {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }


    tBool tLexer::is_identifier_char(tChar c) noexcept  {
        // UTF8 ? ==== Codepoint ==============================================
        if ((c & 0x80) != 0x00) {
            if ((c & 0xe0) == 0xc0) {
                get(); // Skip
                m_Column++;
                return(true);
            }
            else {
                get(); // Skip
                get(); // Skip
                m_Column++; 
                return(true);
            }
        }
        switch (c) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_':
        case '\n': // Excel
            return true;
        default:
            return false;
        }
    }

    tLexerToken tLexer::atom(tKind kind) noexcept { m_Column++; return tLexerToken(kind, m_beg++, 1); }

    tLexerToken tLexer::atom_left_curly() noexcept {
        m_arrayLiteralDepth++;
        return atom(tKind::LeftCurly);
    }

    tLexerToken tLexer::atom_right_curly() noexcept {
        if (m_arrayLiteralDepth > 0) {
            m_arrayLiteralDepth--;
        }
        return atom(tKind::RightCurly);
    }

    tLexerToken tLexer::atom_pipe() noexcept {
        if (!m_inPipeLiteral) {
            m_inPipeLiteral = true;
            m_arrayLiteralDepth++;
        } else {
            m_inPipeLiteral = false;
            if (m_arrayLiteralDepth > 0) {
                m_arrayLiteralDepth--;
            }
        }
        return atom(tKind::Pipe);
    }

    tChar tLexer::peek() const noexcept {
        return *m_beg;
    }

    tChar tLexer::peek_skip_space() const noexcept {
        const tChar* pos = m_beg;
        while (is_space(*pos) && *pos != '\0') {
            pos++;
        }
        return *pos;
    }

    tChar tLexer::get() noexcept {
        m_Column++; // Next Column
        return *m_beg++;
    }

    tInt tLexer::Column() { return(m_Column); }
    tInt tLexer::Line() { return(m_Line); }

    void tLexer::SeparatorArg(tChar sSeparatorArg) { m_SeperatorArg=sSeparatorArg;
    }

    tChar tLexer::SeparatorArg() { return(m_SeperatorArg); };

    void tLexer::SeparatorDecimal(tChar sSeparatorDecimmal) { m_SeparatorDecimal=sSeparatorDecimmal; }

    tChar tLexer::SeparatorDecimal() { return(m_SeparatorDecimal); }
    
    tLexerToken tLexer::next() noexcept {
        
        while (is_space(peek())) get();

        switch (peek()) {
        case '\0':
            return tLexerToken(tKind::End, m_beg, 1);
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            return identifier_or_sheet();
        case 'R': {
            // Is Sheet
            tLexerToken wTokenIdentifier=Is_sheet();
            if (wTokenIdentifier.is(tKind::Sheet)) {
                return(wTokenIdentifier);
            }
            // Is RC Cell Adress
            tLexerToken  wToken = Is_cell_RC();
            if (wToken.is(tKind::Cell)) return(wToken);
            wToken = Is_cell_A1();
            if (wToken.is(tKind::Cell)) return(wToken);
            return identifier_or_sheet();
            break;
        }
        case '@':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
//      case 'R': RC notation
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z': {
            // Is Sheet
            tLexerToken wTokenIdentifier=Is_sheet();
            if (wTokenIdentifier.is(tKind::Sheet)) {
                return(wTokenIdentifier);
            }
            // Is Cell address
            tLexerToken wToken = Is_cell_A1();
            if (wToken.is(tKind::Cell)) return(wToken);
            return identifier_or_sheet();
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return number();
        case '(':
            return atom(tKind::LeftParen);
        case ')':
            return atom(tKind::RightParen);
        case '[': {
            return label_square_bracket();
        }
        case ']':
            return atom(tKind::RightSquare);
        case '{':
            return atom_left_curly();
        case '}':
            return atom_right_curly();
        case '<':
            return(LessThan_or_Equal_or_NotEqual());
        case '>':
            return(GreaterThan_or_Equal());
        case '=':
            return atom(tKind::Equal);
        case '+':
            return atom(tKind::Plus);
        case '-':
            return atom(tKind::Minus);
        case '*':
            return atom(tKind::Times);
        case '/':
            return divide_or_comment();
        case '!':
            return atom(tKind::Exclamation);
        case '&':
            return atom(tKind::Ampersand);
        case '%':
            return atom(tKind::Percent);
        case '#': {
            tLexerToken wToken = Is_ErrorRef();
            if (wToken.is(tKind::ErrorRef)) return(wToken);
            wToken = Is_ErrorName();
            if (wToken.is(tKind::ErrorName)) return(wToken);
            return atom(tKind::Hash);
        }
        case '.':
            return Is_Attribute();
        case ',':
            return atom(tKind::Comma);
        case ':':
            return atom(tKind::Colon);
        case ';':
            return atom(tKind::Semicolon);
        case '\'':
            return label_simple_quote_or_sheet();
        case '"':
            return label_double_quote();
        case '|':
            return atom_pipe();
        case '$': {
            // Is Cell address
            tLexerToken wToken = Is_cell_A1(true);
            if (wToken.is(tKind::Cell)) return(wToken);
            return atom(tKind::Dollar);
        }
        case '\n': {
            m_Line++; m_Column = 1;
            return atom(tKind::NewLine);
        }
        default : {
            return identifier_or_sheet();
        }
        }
    }

    tLexerToken tLexer::nextKey() noexcept {
        while (is_space(peek())) get();

        switch (peek()) {
        case '\0':
            return tLexerToken(tKind::End, m_beg, 1);
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            return identifier_or_sheet();
        case 'C': {
            // Is Cell Adress
            tLexerToken wToken = Is_cell_Key();
            if (wToken.is(tKind::Cell)) return(wToken);
            return identifier_or_sheet();
            break;
        }
        case '@':
        case 'A':
        case 'B':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z': {
            return identifier_or_sheet();
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return number();
        case '(':
            return atom(tKind::LeftParen);
        case ')':
            return atom(tKind::RightParen);
        case '[': {
            return label_square_bracket();
        }
        case ']':
            return atom(tKind::RightSquare);
        case '{':
            return atom_left_curly();
        case '}':
            return atom_right_curly();
        case '<':
            return(LessThan_or_Equal_or_NotEqual());
        case '>':
            return(GreaterThan_or_Equal());
        case '=':
            return atom(tKind::Equal);
        case '+':
            return atom(tKind::Plus);
        case '-':
            return atom(tKind::Minus);
        case '*':
            return atom(tKind::Times);
        case '/':
            return divide_or_comment();
        case '!': {
            // Is Cell address
            tLexerToken wToken = Is_cell_Key();
            if (wToken.is(tKind::Cell)) return(wToken);
            return(atom(tKind::Exclamation));
        }
        case '&':
            return atom(tKind::Ampersand);
        case '%':
            return atom(tKind::Percent);
        case '#': {
            tLexerToken wToken = Is_ErrorRef();
            if (wToken.is(tKind::ErrorRef)) return(wToken);
            wToken = Is_ErrorName();
            if (wToken.is(tKind::ErrorName)) return(wToken);
            return atom(tKind::Hash);
        }
        case '.':
            return Is_Attribute();
        case ',':
            return atom(tKind::Comma);
        case ':':
            return atom(tKind::Colon);
        case ';':
            return atom(tKind::Semicolon);
        case '\'':
                return label_simple_quote_or_sheet();
        case '"':
            return label_double_quote();
        case '|':
            return atom_pipe();
        case '$': {
            // Is Cell address
            tLexerToken wToken = Is_cell_Key();
            if (wToken.is(tKind::Cell)) return(wToken);
            return atom(tKind::Dollar);
        }
        case '\n': {
            m_Line++; m_Column = 1;
            return atom(tKind::NewLine);
        }
        default: {
            //return atom(tKind::Unexpected);
            return(identifier_or_sheet());
        }
        }
    }
    

    tLexerToken tLexer::identifier_or_sheet() noexcept {
        const tChar* start = m_beg;
        get();
        while (is_identifier_char(peek())) get();
        // Excel R1C1: MyNameR[-1]C[2] — trailing R begins RC offset, not part of the name.
        if (peek() == '[' && m_beg > start + 1 && *(m_beg - 1) == 'R') {
            const tChar* wRcStart = m_beg - 1;
            const tInt wColAtR = m_Column - 1;
            m_beg = wRcStart;
            m_Column = wColAtR;
            tLexerToken wRc = Is_cell_RC();
            if (wRc.is(tKind::Cell)) {
                // Validate only — leave m_beg at R so the next token is the Cell ref.
                m_beg = wRcStart;
                m_Column = wColAtR;
                return tLexerToken(tKind::Identifier, start, wRcStart);
            }
            m_beg = wRcStart + 1;
            m_Column = wColAtR + 1;
        }
        // Is Sheet!
        if (peek()=='!') {
            get();
            return(tLexerToken(tKind::Sheet, start, m_beg));
        }
      
        // Bare TRUE/FALSE are boolean literals. TRUE()/FALSE() are Excel functions,
        // so keep the Identifier when immediately followed by '('.
        tLexerToken wLexerToken=tLexerToken(tKind::Identifier, start, m_beg);
        tString wLexeme = wLexerToken.Lexeme();
        std::transform(wLexeme.begin(), wLexeme.end(), wLexeme.begin(),
            [](unsigned char c) { return static_cast<tChar>(std::toupper(c)); });
        if (wLexeme == "TRUE" || wLexeme == "FALSE") {
            const tChar* wP = m_beg;
            while (is_space(*wP)) {
                ++wP;
            }
            if (*wP != '(') {
                wLexerToken.Kind(tKind::Bool);
            }
        }
        return wLexerToken;
    }


    tLexerToken tLexer::number() noexcept {
        const tChar* start = m_beg;
        tLong value = peek() - '0';;
        tBool overflow = false;
        tBool hasDecimalPart = false;
        get();
        // Parse integer part
        while (is_digit_char(peek())) {
            tInt digit = peek() - '0';
            if (value > (INT32_MAX - digit) / 10) {
                overflow = true;
            }
            value = value * 10 + digit;
            get();  
        }
        // Parse decimal part (locale separator)
        // Outside { } and | |, FR uses comma as decimal so "1,1" is one float — not two args for DATE(…,1,1).
        // Use semicolons for FR-style args, or Locale("us") with comma arg separators (matches exported .sker).
        if (peek() == m_SeparatorDecimal) {
            // Inside { ... } or | ... |, comma is always column separator (Excel-style), not decimal
            if (m_arrayLiteralDepth > 0 && m_SeparatorDecimal == ',') {
                if (overflow) {
                    return tLexerToken(tKind::Float, start, m_beg);
                }
                return tLexerToken(tKind::Integer, start, m_beg);
            }
            get();
            while (is_digit_char(peek())) get();
            hasDecimalPart = true;
        }
        // FR locale: comma is list separator in arrays; allow dot decimal inside array constant
        else if (m_arrayLiteralDepth > 0 && peek() == '.') {
            get();
            while (is_digit_char(peek())) get();
            hasDecimalPart = true;
        }
        // US-style scientific literals (8.89E-2) from Excel/JSON when locale decimal is ','.
        else if (m_SeparatorDecimal != '.' && peek() == '.') {
            const tChar* wDot = m_beg;
            get();
            if (is_digit_char(peek())) {
                while (is_digit_char(peek())) get();
                hasDecimalPart = true;
            } else {
                m_beg = wDot;
            }
        }

        // Parse optional scientific notation exponent (e.g. 12e12, 12E12, 100.67E-3)
        if ((peek() == 'e') || (peek() == 'E')) {
            const tChar* exponentStart = m_beg;
            get();
            if ((peek() == '+') || (peek() == '-')) get();
            if (is_digit_char(peek())) {
                while (is_digit_char(peek())) get();
                return tLexerToken(tKind::Float, start, m_beg);
            }
            // Invalid exponent form: rollback so next tokenization stays consistent
            m_beg = exponentStart;
        }

        if (hasDecimalPart) {
            return tLexerToken(tKind::Float, start, m_beg);
        }
        // Row-only shorthand inside ranges: 13:13
        auto hasColonAround = [&](const tChar* tokenStart) -> tBool {
            const tChar* right = m_beg;
            while (is_space(*right)) right++;
            if (*right == ':') return true;
            const tChar* left = tokenStart;
            while ((left > m_formulaBegin) && is_space(*(left - 1))) left--;
            return ((left > m_formulaBegin) && (*(left - 1) == ':'));
        };
        if (hasColonAround(start)) {
            m_begRow = start;
            m_endRow = m_beg;
            m_begCol = m_beg;
            m_endCol = m_beg;
            tInt wRowIndex = atoi(tString(start, distance(start, m_beg)).c_str());
            return tLexerToken(tKind::Cell, start, m_beg,
                false, m_begRow, m_endRow,
                false, m_begCol, m_endCol,
                wRowIndex, 1);
        }
        if (overflow) {
            return tLexerToken(tKind::Float, start, m_beg);
        }
        return tLexerToken(tKind::Integer, start, m_beg);
    }

    tLexerToken tLexer::divide_or_comment() noexcept {
        const tChar* start = m_beg;
        get();
        if (peek() == '/') {
            get();
            start = m_beg;
            while (peek() != '\0') {
                if (get() == '\n') {
                    return tLexerToken(tKind::Comment, start,
                        distance(start, m_beg) - 1);
                }
            }
            return tLexerToken(tKind::Unexpected, m_beg, 1);
        }
        else {
            return tLexerToken(tKind::Divide, start, 1);
        }
    }

    tLexerToken tLexer::LessThan_or_Equal_or_NotEqual() noexcept {
        const tChar* start = m_beg;
        get();
        if (peek() == '=') {
            get();
            return tLexerToken(tKind::LessThanOrEqual);
        }
        if (peek() == '>') {
            get();
            return tLexerToken(tKind::NotEqual);
        }
        return tLexerToken(tKind::LessThan, start, 1);
    }

    tLexerToken tLexer::GreaterThan_or_Equal() noexcept {
        const tChar* start = m_beg;
        get();
        if (peek() == '=') {
            get();
            return tLexerToken(tKind::GreaterThanOrEqual);
        }
        return tLexerToken(tKind::GreaterThan, start, 1);
    }

    tLexerToken tLexer::Is_sheet() noexcept {
        tInt wSaveColumn = m_Column;
        const tChar* start = m_beg;
        get();
        while (is_identifier_char(peek())) get();
        // Is Sheet!
        if (peek()=='!') {
            get();
            return(tLexerToken(tKind::Sheet, start, m_beg));
        }
        m_Column = wSaveColumn;
        m_beg = start;
        return tLexerToken(tKind::NotCell);
    }

    tLexerToken tLexer::Is_cell_A1(tBool sLockCol) noexcept {
        const tChar* start = m_beg;
        tInt wSaveColumn = m_Column;
        auto hasColonAround = [&](const tChar* tokenStart) -> tBool {
            const tChar* right = m_beg;
            while (is_space(*right)) right++;
            if (*right == ':') return true;
            const tChar* left = tokenStart;
            while ((left > m_formulaBegin) && is_space(*(left - 1))) left--;
            return ((left > m_formulaBegin) && (*(left - 1) == ':'));
        };
        auto isCellBoundary = [&](tChar c) -> tBool {
            if (c == '\0') return true;
            if (is_space(c)) return true;
            switch (c) {
            case ':': case ',': case ';':
            case ')': case ']': case '}':
            case '+': case '-': case '*': case '/':
            case '^': case '&':
            case '<': case '>': case '=':
            case '!':
                return true;
            default:
                return false;
            }
        };
        // Function names that look like A1 refs: LOG10(...), BIN2DEC(...), HEX2OCT(...).
        // Reject a cell-shaped prefix only when it is clearly a function call:
        //   LOG10(          — digits then '('
        //   BIN2DEC(        — digits, more letters, then '('
        // Do not reject bare refs like T6B (legacy select) or A1 alone.
        auto followedByCall = [&]() -> tBool {
            const tChar* wP = m_beg;
            while (is_space(*wP)) {
                ++wP;
            }
            return (*wP == '(');
        };
        auto followedByIdentThenCall = [&]() -> tBool {
            const tChar* wP = m_beg;
            if (!is_identifier_char(*wP)) return(false);
            while (is_identifier_char(*wP)) ++wP;
            while (is_space(*wP)) ++wP;
            return(*wP == '(');
        };
        
        if (sLockCol) get();
        // Support row-only shorthand with leading '$' (e.g. $13)
        // Expanded later in Range() when paired as $13:$13.
        if (sLockCol && (peek() >= '0') && (peek() <= '9')) {
            tBool wLockCol = true;
            tBool wLockRow = true;
            m_begCol = m_beg;
            m_endCol = m_beg; // Empty col marker
            m_begRow = m_beg;
            tString wRowStr = "";
            int nbInt = 0;
            while ((peek() >= '0') && (peek() <= '9')) {
                nbInt++;
                wRowStr += get();
            }
            if ((nbInt == 0) || (nbInt > 9)) {
                m_beg = start;
                m_Column = wSaveColumn;
                return tLexerToken(tKind::NotCell);
            }
            if (!isCellBoundary(peek()) || followedByCall()) {
                m_beg = start;
                m_Column = wSaveColumn;
                return tLexerToken(tKind::NotCell);
            }
            m_endRow = m_beg;
            tInt wRowIndex = atoi(wRowStr.c_str());
            return tLexerToken(tKind::Cell, start, m_beg,
                wLockRow, m_begRow, m_endRow,
                wLockCol, m_begCol, m_endCol,
                wRowIndex, 1);
        }
        int nbChar = 1;
        tBool wLockCol = sLockCol;
        tBool wLockRow = false;

        m_begCol = m_beg;
        tString wColStr = "";
        while ((peek() >= '@') && (peek() <= 'Z')) {
            nbChar++;
            wColStr+=get();
        }
        // Not a cell: need at least one column letter, and at most 4
        if (wColStr.empty() || (nbChar == 0) || (nbChar > 4)) {
            m_Column = wSaveColumn;
            m_beg = start;
            return tLexerToken(tKind::NotCell);
        }
        tInt wColIndex = AlphaToBase10(wColStr);
        m_endCol = m_beg;
        
        if (peek() == '$') { get();  wLockRow = true; }
        tString wRowStr = "";
        int nbInt = 0;
        m_begRow = m_beg;
        while ((peek() >= '0') && (peek() <= '9')) {
            nbInt++;
            wRowStr+=get();
        }
        // Support col-only shorthand with leading '$' (e.g. $B)
        // Expanded later in Range() when paired as $B:$B.
        if ((nbInt == 0) && (sLockCol || hasColonAround(start))) {
            // Avoid matching prefixes of identifiers like Table1 after ":".
            if (!isCellBoundary(peek())) {
                m_beg = start;
                m_Column = wSaveColumn;
                return tLexerToken(tKind::NotCell);
            }
            m_begRow = m_beg;
            m_endRow = m_beg; // Empty row marker
            return tLexerToken(tKind::Cell, start, m_beg,
                wLockRow, m_begRow, m_endRow,
                wLockCol, m_begCol, m_endCol,
                1, wColIndex);
        }
        // Not Adress of Cell Return to begin
        if ((nbInt == 0) || (nbInt > 9)) { //1 000 000 000
            m_beg = start;
            m_Column = wSaveColumn;
            return tLexerToken(tKind::NotCell);
        }
        m_endRow = m_beg;
        tInt wRowIndex = atoi(wRowStr.c_str());
        if (followedByCall() || followedByIdentThenCall()) {
            m_beg = start;
            m_Column = wSaveColumn;
            return tLexerToken(tKind::NotCell);
        }

        return tLexerToken(tKind::Cell, start, m_beg,
            wLockRow, m_begRow, m_endRow,
            wLockCol, m_begCol, m_endCol,
            wRowIndex,wColIndex);
    }

    tLexerToken tLexer::Is_cell_Key() noexcept {
        const tChar* start = m_beg;
        tInt wSaveColumn = m_Column;
        
        m_begCol = m_beg;

        tBool wLockRow = false;
        tBool wLockCol = false;
        
        tInt wRowIndex = 0; // -1 for Sheet
        if (peek() == '!') {
            wRowIndex = -1;
            get();
        }
        if (peek() == '$') {
            wLockCol = true;
            get();
        }
        if (peek() != 'C') {
            m_Column = wSaveColumn;
            m_beg = start;
            return(tLexerToken(tKind::NotCell));
        }
        get();
        m_endCol = m_beg;
        m_begRow = m_beg;

        if (peek() == '$') {
            wLockRow = true;
            get();
        }
        if (peek() != 'R') {
            m_Column = wSaveColumn;
            m_beg = start;
            return(tLexerToken(tKind::NotCell));
        }
        get();

        m_endRow = m_beg;

        return tLexerToken(tKind::Cell, start, m_beg,
            wLockRow, m_begRow, m_endRow,
            wLockCol, m_begCol, m_endCol,
            wRowIndex,0);
    }


    tLexerToken tLexer::Is_cell_RC() noexcept {
        const tChar* start = m_beg;
        tInt wSaveColumn = m_Column;
        
        m_begRow = m_beg;

        tBool wLockRow = true;
        tBool wLockCol = true;

        tInt wRowIndex = 0;
        tInt wColIndex = 0;
        if (peek() == '!') {
            get();
        }

        if (peek() != 'R') {
            m_Column = wSaveColumn;
            m_beg = start;
            return(tLexerToken(tKind::NotCell));
        }
        m_begRow = m_beg;
        get();
        tString wRowStr = "";
        if (peek() == '[') {
            get();
            wLockRow = false;
            if ((peek() == '+') || (peek() == '-')) {
                wRowStr += get();
            }
        }
        int nbInt = 0;
        while ((peek() >= '0') && (peek() <= '9')) {
            nbInt++;
            wRowStr += get();
        }
        if ((nbInt == 0) || (nbInt > 10)) { //10 000 000 000
            m_Column = wSaveColumn;
            m_beg = start;
            return tLexerToken(tKind::NotCell);
        }
        wRowIndex = atoi(wRowStr.c_str());

        if (!wLockRow) {
            if (peek() != ']') {
                m_Column = wSaveColumn;
                m_beg = start;
                return(tLexerToken(tKind::NotCell));
            }
            get();
        }
        m_endRow = m_beg;
        m_begCol = m_beg;
        if (peek() != 'C') {
            m_Column = wSaveColumn;
            m_beg = start;
            return(tLexerToken(tKind::NotCell));
        }
        get();

        tString wColStr = "";
        if (peek() == '[') {
            get();
            wLockCol = false;
            if ((peek() == '+') || (peek() == '-')) {
                wColStr += get();
            }
        }
        nbInt = 0;
        while ((peek() >= '0') && (peek() <= '9')) {
            nbInt++;
            wColStr += get();
        }

        if ((nbInt == 0) || (nbInt > 10)) { //10 000 000 000
            m_Column = wSaveColumn;
            m_beg = start;
            return tLexerToken(tKind::NotCell);
        }
        wColIndex = atoi(wColStr.c_str());

        if (!wLockCol) {
            if (peek() != ']') {
                m_Column = wSaveColumn;
                m_beg = start;
                return(tLexerToken(tKind::NotCell));
            }
            get();
        }
        m_endCol = m_beg;


        return tLexerToken(tKind::Cell, start, m_beg,
            wLockRow, m_begRow, m_endRow,
            wLockCol, m_begCol, m_endCol,
            wRowIndex, wColIndex,true);
    }

    SkInline tLexerToken tLexer::Is_Attribute() noexcept {
        const tChar* start = m_beg;
        get();
        if (is_identifier_char(peek())) {
            while (is_identifier_char(peek())) get();
            return(tLexerToken(tKind::Attribute, start, m_beg));
        }
        return(atom(tKind::Dot));
    }



    tLexerToken tLexer::label_simple_quote_or_sheet() noexcept {
        const tChar* start;
        const tChar* startSheet;
        startSheet= m_beg;
        get();
        start = m_beg;
        while (peek() != '\0') {
            if (get() == '\'') {
                // Is Sheet!
                if (peek()=='!') {
                    get();
                    return(tLexerToken(tKind::Sheet, startSheet,  m_beg));
                }
                return tLexerToken(tKind::LabelSimple, start,
                    distance(start, m_beg) - 1);
            }
        }
        return tLexerToken(tKind::Unexpected, m_beg, 1);
    }

    tLexerToken tLexer::label_double_quote() noexcept {
        const tChar* start;
        get();
        start = m_beg;
        while (peek() != '\0') {
            if (get() == '"') {
                return tLexerToken(tKind::LabelDouble, start,distance(start, m_beg) - 1);
            }
        }
        return tLexerToken(tKind::Unexpected, m_beg, 1);
    }

    tLexerToken tLexer::label_square_bracket() noexcept {
        get(); // consume '['
        const tChar* start = m_beg;
        // Bracket counting: accept any characters between [ and matching ], including nested [ ]
        int depth = 1;
        while (depth > 0) {
            tChar c = get();
            // Skip embedded nulls (e.g. UTF-16LE when interpreted as char*) so bracket matching continues
            if (c == '\0' && depth > 0 && peek() != '\0') {
                c = get(); // skip padding byte, get next real character
            }
            if (c == '\0') return tLexerToken(tKind::Unexpected, m_beg, 1);
            if (c == '[') ++depth;
            else if (c == ']') --depth;
        }
        // Lexeme is content between brackets (exclude the closing ']' we just consumed)
        return tLexerToken(tKind::LabelSquare, start, distance(start, m_beg) - 1);
    }

    tLexerToken tLexer::Is_ErrorRef() noexcept {
        const tChar* start= m_beg;
        tInt wSaveColumn=m_Column;
        // Error Ref is #REF!   
        get(); // consume '#'
        if (peek() != 'R') {  m_Column = wSaveColumn; m_beg = start;  return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != 'E') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != 'F') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != '!') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        return tLexerToken(tKind::ErrorRef, start, distance(start, m_beg));
    }

    tLexerToken tLexer::Is_ErrorName() noexcept {
        const tChar* start = m_beg;
        tInt wSaveColumn=m_Column;
        // Error Name is #NAME?   
        get(); // consume '#'
        if (peek() != 'N') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != 'A') { m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != 'M') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != 'E') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        if (peek() != '?') {  m_Column = wSaveColumn; m_beg = start; return tLexerToken(tKind::Unexpected, start, 1); }
        get();
        return tLexerToken(tKind::ErrorName, start, distance(start, m_beg));
    }



#include <iomanip>
#include <iostream>
    
    ostream& operator<<(ostream& os, const tKind& kind) {
        static const tChar* const names[]{
            "Integer",        "Float",          "Identifier",   "Bool",
            "Sheet",          "Cell",           "Attribute",    "Range",
            "DynamicRangeRight", "DynamicRangeLeft", "DynamicRangeBoth",
            "LeftParen",      "RightParen",
            "LeftSquare",     "RightSquare",
            "LeftCurly",      "RightCurly",
            "LessThan",       "LessThanOrEqual", "GreaterThan",  "GreaterThanOrEqual",
            "Equal",          "NotEqual",
            "Plus",           "Minus",          "Times",        "Divide",
            "UnaryMinus",     "Exclamation",    "Ampersand",    "Hash",
            "Dot",            "Comma",          "Colon",        "Semicolon",
            "Percent",
            "LabelSimple",    "LabelDouble",    "LabelSquare",    "Comment",
            "Pipe",           "Dollar",
            "Function",       "Name",
            "ErrorRef",       "ErrorName",
            "NewLine",        "End",            "Unexpected",   "NotCell",
            "LetBeginScope",  "LetBind",        "LetVarRef",    "LetEndScope",
            "LambdaRef",      "IfCond",         "IfElse",       "IfEnd",
            "SpillRef"
        };
        return os << names[static_cast<int>(kind)];
    }

    ostream& operator<<(ostream& os, const tLexerToken& sSkLexerToken) {
        os << sSkLexerToken.Kind() << " : " << sSkLexerToken.Lexeme();
        return(os);
    }

}

