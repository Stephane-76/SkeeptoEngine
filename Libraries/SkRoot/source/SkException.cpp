//=============================================================================
// Skeema Exception
// Ancestor class of all exceptions
//=============================================================================
#include "../include/SkException.hpp"

namespace SkRoot {
	const tChar* tException::what() const throw() { return "Exception skeema"; }
	

	tExceptionInternalError::tExceptionInternalError(tString sMsg) : tException(), m_Msg(sMsg){}
	const tChar* tExceptionInternalError::what() const throw() {
		tStringStream wStream;
		wStream << "Internal Error " << m_Msg << " !";
		cout << wStream.str() << endl;
		// static keeps in memory for return
		static tString wResult = wStream.str();
		return wResult.c_str();
	}


	const tChar* tExceptionDiv0::what() const throw() { return "Divide by 0"; }

	tExceptionBadType::tExceptionBadType(tString sMsg) : tException(), m_Msg(sMsg) {};
	const tChar* tExceptionBadType::what() const throw() { 
		tStringStream wStream;
		wStream << "Bad type " << m_Msg << " !";
		// static keeps in memory for return
		static tString wResult=wStream.str();
		return wResult.c_str();
	}

	tExceptionMemory::tExceptionMemory(tString sMsg) : tException(), m_Msg(sMsg) {};
	const tChar* tExceptionMemory::what() const throw() {
		tStringStream wStream;
		wStream << "Memory error " << m_Msg << " !";
		// static keeps in memory for return
		static tString wResult = wStream.str();
		return wResult.c_str();
	}

}