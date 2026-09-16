//=============================================================================
// Lemon Token 
//=============================================================================

#ifndef SkLemonToken_hpp
#define SkLemonToken_hpp
#include <SkVariant.hpp>
#include "SkLexerSpreadSheet.hpp"
using namespace SkRoot;

namespace SkSpreadSheet {
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


