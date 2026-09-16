// Initial work (thank you Juan)
// A simple Lexer meant to demonstrate a few theoretical concepts. It can
// support several parser concepts and is very fast (though speed is not its
// design goal).
//
// J. Arrieta
// (C) 2018 Nabla Zero Labs
// https://gist.github.com/arrieta lexer.cpp

#include "../include/SkLexerFormat.hpp"
#include "../include/SkApplication.hpp"
#include <cctype>

using namespace SkRoot;
using namespace std;

namespace SkFormat {

    tLexerToken::tLexerToken() noexcept : m_Kind{}, 
                        m_Begin(nullptr),m_End(nullptr)  {}

    tLexerToken::tLexerToken(tKind sKind) noexcept: m_Kind{ sKind },
                        m_Begin(nullptr),m_End(nullptr) {}

    tLexerToken::tLexerToken(tKind kind, const tChar* sBegin, tSize sLen) noexcept:
                        m_Kind{ kind },
                        m_Begin(sBegin), m_End(sBegin+sLen)  {}

    tLexerToken::tLexerToken(tKind kind, const tChar* sBegin, const tChar* sEnd) noexcept: 
                        m_Kind{ kind },
                        m_Begin(sBegin), m_End(sEnd) {}

 
    tLexerToken::tLexerToken(const tLexerToken& sLexerToken) noexcept {
        m_Kind  = sLexerToken.m_Kind;
        m_Begin = sLexerToken.m_Begin;
        m_End   = sLexerToken.m_End;
    }

    tLexerToken::~tLexerToken() {}

    void* tLexerToken::operator new(tSize sz) noexcept {
        return(tApplication::Instance()->AllocTemporary(sz));
    }

    void tLexerToken::operator  delete(void* ptr) noexcept {

    }

    tKind tLexerToken::Kind() const noexcept { return m_Kind; }
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

    template <typename... Ts>
    tBool tLexerToken::is_one_of(tKind sK1, tKind sK2, Ts... sKs) const noexcept {
        return is(sK1) || is_one_of(sK2, sKs...);
    }

    const tString tLexerToken::Lexeme() const noexcept {
        return(tString(m_Begin,distance(m_Begin, m_End)));
    }
    
    // SkLexer ====================================================================
    tLexer::tLexer() noexcept : m_Line(1),m_Column(0), m_InCss(false), m_beg(nullptr) {};
    tLexer::tLexer(const tChar* beg) noexcept : m_Line(1),m_Column(0), m_InCss(false), m_beg{ beg } {}

    void tLexer::InCss(tBool sInCss) noexcept { m_InCss=sInCss; }

    tBool tLexer::InCss() noexcept { return(m_InCss); }

    tBool is_space(tChar c) noexcept {
        switch (c) {
        case ' ':   // Space
        case '\t': // Tabulation
        //case '\r': // Carriage return
            return true;
        default:
            return false;
        }
    }

    tBool is_digit_char(tChar c) noexcept {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }


    tBool is_identifier(tChar c) noexcept {
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
        case '-': // padding-left
        case '_':
            return true;
        default:
            return false;
        }
    }
    tBool is_hexa(tChar c) noexcept {
        switch (c) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
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
            return true;
        default:
            return false;
        }
    }

    tLexerToken tLexer::atom(tKind kind) noexcept {
        m_Column++;
        return tLexerToken(kind, m_beg++, 1);
    }


    tChar tLexer::peek() const noexcept {
        return *m_beg;
    }

    tChar tLexer::get() noexcept {
        m_Column++; // Next Column
        return *m_beg++;
    }

    tInt tLexer::Column() { return(m_Column); }
    tInt tLexer::Line() { return(m_Line); }

    tLexerToken tLexer::next() noexcept {
        while (is_space(peek())) { get();  }
      
        switch (peek()) {
        case '\0':
            return tLexerToken(tKind::End, m_beg, 1);
        default:
            return atom(tKind::Unexpected);
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
            return identifier();
            
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
        case 'Z': {
            return (identifier());
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
        case '[':
            return atom(tKind::LeftSquare);
        case ']':
            return atom(tKind::RightSquare);
        case '{':
            return atom(tKind::LeftCurly);
        case '}':
            return atom(tKind::RightCurly);
        case '<':
            return(LessThan_or_Equal_or_NotEqual());
        case '>':
            return(GreaterThan_or_Equal());
        case '=':
            return atom(tKind::Equal);
        case '+':
            return atom(tKind::Plus);
        case '-': {
            // If the minus sign is immediately followed by a digit, lex the
            // whole "-<digits>[.<digits>]" sequence as a single negative
            // Integer or Float token. This makes CSS values like
            // "text-rotate:-45;" or "padding-left:-5px;" parse without
            // introducing a dedicated grammar rule for Minus.
            if (is_digit_char(*(m_beg + 1))) return number();
            return atom(tKind::Minus);
        }
        case '*':
            return atom(tKind::Times);
        case '/':
            return divide_or_comment();
        case '!':
            return atom(tKind::Exclamation);
        case '&':
            return atom(tKind::Ampersand);
        case '#':
            if (m_InCss) {
                return color();
            } else {
                return(atom(tKind::Hash));
            }
        case '.':
            return atom(tKind::Dot);
        case ',':
            return atom(tKind::Comma);
        case ':':
            return atom(tKind::Colon);
        case ';':
            return atom(tKind::Semicolon);
        case '@' :
            return atom(tKind::At);
        case '\'':
            return label_simple_quote();
        case '"':
            return label_double_quote();
        case '|':
            return atom(tKind::Pipe);
        case '$':
            return atom(tKind::Dollar);
        case '%':
            return atom(tKind::Percent);
        case '\n': {
            m_Line++; m_Column = 1;
            return atom(tKind::NewLine);
        }
        }
    }

    tLexerToken tLexer::identifier() noexcept {
        const tChar* start = m_beg;
        get();
        while (is_identifier(peek())) get();

        return tLexerToken(tKind::Identifier, start, m_beg);
    }
    
    tLexerToken tLexer::number() noexcept {
        const tChar* start = m_beg;
        // Optional leading minus sign (negative number).
        if (peek() == '-') get();
        tLong value = peek() - '0';;
        tBool overflow = false;
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
        // Parse decimal part
        if (peek() == '.') {
                get();
                while (is_digit_char(peek())) get();
                return tLexerToken(tKind::Float, start, m_beg);
        }
        if (overflow) {
            return tLexerToken(tKind::Float, start, m_beg);
        }
        return tLexerToken(tKind::Integer, start, m_beg);
    }


    tLexerToken tLexer::color() noexcept {
        const tChar* start = m_beg;
        get();
        tInt wLenght=1;
        if (is_hexa(peek()))  {
            get();
            while (is_hexa(peek())) { wLenght++; get(); }
            if ((wLenght >= 2) && (wLenght<=8))
                return tLexerToken(tKind::ColorHash, start, distance(start, m_beg));
        }
        m_beg=start;
        get();
        return tLexerToken(tKind::Hash, start, 1);

    }

    tLexerToken tLexer::divide_or_comment() noexcept {
        const tChar* start = m_beg;
        get();
        if (peek() == '/') {
            get(); // Comment 
            start = m_beg;
            while (peek() != '\0') {
                if (get() == '\n') {
                    return tLexerToken(tKind::Comment, start,
                        distance(start, m_beg) - 1);
                }
            }
            return tLexerToken(tKind::Unexpected, m_beg, 1);
        } else if (peek() == '*') {
            get(); /* Comment */
            tBool wOk=false;
            while (!wOk) {
                while ((peek() != '*') && (!wOk)) {
                    get();
                    if (peek() == '\0') {
                        return tLexerToken(tKind::Divide, start, 1);
                    }
                }
                get();
                if (peek()=='\0') {
                    return tLexerToken(tKind::Divide, start, 1);
                }
                if (peek() == '/') {
                    get();
                    return tLexerToken(tKind::Comment, start,
                                       distance(start, m_beg));
                }
                get();
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

    tLexerToken tLexer::label_simple_quote() noexcept {
        const tChar* start;
        get();
        start = m_beg;
        while (peek() != '\0') {
            if (get() == '\'') {
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

#include <iomanip>
#include <iostream>

    ostream& operator<<(ostream& os, const tLexerToken& sSkLexerToken) {
        os <<  sSkLexerToken.Lexeme();
        return(os);
    }

}

