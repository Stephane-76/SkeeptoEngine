//=============================================================================
// SkLemonSpreadSheet SpreadSheet Parser
//=============================================================================
#ifndef SkLemonSpreadSheet_hpp
#define SkLemonSpreadSheet_hpp
#include <iostream>  
#include <iomanip>      // std::setw

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <SkTypes.hpp>
#include <SkClass.hpp>

#include "SkLemonSpreadSheet.h"
#include "SkLemonToken.hpp"
#include "SkLexerSpreadSheet.hpp"
#include "SkLemonReserved.hpp"
#include "SkLemonInterface.hpp"

#include "SkApi.hpp"
#include "SkTools.hpp"

namespace SkSpreadSheet {
	/// @brief		Init lemon parser with Lemon interface.
	/// @param[in]	sInterface SkLemonInterface*
	void ParserInit(tLemonInterface* sInterface);

	/// @brief		Done lemon parser with Lemon interface.
	/// @param[in]	sInterface SkLemonInterface*
	void ParserDone(tLemonInterface* sInterface);
     
	/// @brief		Parse.
	/// @param[in]	sInterface SkLemonInterface*
	/// @param[in]	sCode tChar*
	/// @return		tBool false if error
	tBool ParserRun(tLemonInterface* sInterface, const tChar* sCode);
}
#endif
