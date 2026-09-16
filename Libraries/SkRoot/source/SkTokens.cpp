//=============================================================================
// Skeema Tokens
//  Allows to attach tVirtualClass to Dictionaries...
//  Contains for example the globals of SkApplication.
//=============================================================================
#include "../include/SkTokens.hpp"

namespace SkRoot {

	
	tTokens::tTokens() : tVirtualClass() {
	}

	tTokens::~tTokens() {
		Clear();
	};

	void tTokens::Clear() {
		for (auto wToken : m_Map) {
			delete(wToken.second);
		}
		m_Map.clear();
	}

	tBool tTokens::Add(tString sKey, tVirtualClass* sVirtualClass) {
		auto wIterator=m_Map.find(sKey);
		if (wIterator != m_Map.end()) {
			return(false);
		}
		m_Map[sKey] = sVirtualClass;
		return(true);
	}

	tBool tTokens::Delete(tString sKey) {
		auto wIterator = m_Map.find(sKey);
		if (wIterator != m_Map.end()) {
			if ((*wIterator).second!=nullptr) delete((*wIterator).second);
			m_Map.erase(wIterator);
			return(true);
		}
		return(false);
	}

	tVirtualClass* tTokens::Token(tString sKey) {
		auto wIterator = m_Map.find(sKey);
		if (wIterator != m_Map.end()) {
			return(m_Map[sKey]);
		}
		return(nullptr);
	}



}; // end of namespace

