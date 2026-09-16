//=============================================================================
// Lemon Token 
//=============================================================================
#ifndef SkLemonFormatToken_hpp
#define SkLemonFormatToken_hpp

#include "SkLexerFormat.hpp"

using namespace SkRoot;

namespace SkFormat {
	//=========================================================================
	//! struct Token used by lexer
	struct tToken {
	public:
		//! Token for parser
		tLexerToken* m_Token;
		//! Current line
		tInt		  m_Line;
		//! Current column
		tInt		  m_Column;
	};
}
#endif


