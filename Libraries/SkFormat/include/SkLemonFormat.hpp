//=============================================================================
// SkLemonFormat Format Parser
//=============================================================================
#ifndef SkLemonFormat_hpp
#define SkLemonFormat_hpp

#include <iostream>
#include <iomanip>      // std::setw

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <SkTypes.hpp>
#include <SkClass.hpp>

#include "SkLexerFormat.hpp"
#include "SkLemonFormat.h"
#include "SkLemonFormatToken.hpp"
#include "SkLemonFormatInterface.hpp"

namespace SkFormat {
	/// @brief		Init lemon parser with Lemon interface.
	/// @param[in]	sInterface SkLemonInterface*
	void ParserInit(tLemonFormatInterface* sInterface);

	/// @brief		Done lemon parser with Lemon interface.
	/// @param[in]	sInterface SkLemonInterface*
	void ParserDone(tLemonFormatInterface* sInterface);

	/// @brief		Parse.
	/// @param[in]	sInterface SkLemonInterface*
	/// @param[in]	sCode tChar*
	/// @return		tBool false if error
	tBool ParserRun(tLemonFormatInterface* sInterface, const tChar* sCode);
}

#endif
