//=============================================================================
// SkReservedId for reserved word
//=============================================================================
#include "../include/SkLemonReserved.hpp"

namespace SkSpreadSheet {

	// Reference for Reserved Id ==============================================
	tLemonIdRef::tLemonIdRef() : tClass(), m_Id(-1), m_Class(nullptr), m_Kind(){}
	tLemonIdRef::tLemonIdRef(const tLemonIdRef& sLemonIdRef) : tClass(sLemonIdRef), m_Id(sLemonIdRef.m_Id), m_Class(sLemonIdRef.m_Class), m_Kind(sLemonIdRef.m_Kind){ }

	tLemonIdRef::tLemonIdRef(tInt sId, tVirtualClass* sClass, tKind sKind) : tClass(), m_Id(sId), m_Class(sClass), m_Kind(sKind){}

	tInt tLemonIdRef::Id() { return(m_Id); }
	void tLemonIdRef::Id(tInt sId) { m_Id = sId; }

	tVirtualClass* tLemonIdRef::Class() { return(m_Class); }
	void tLemonIdRef::Class(tVirtualClass* sClass) { m_Class = sClass; };

	tKind tLemonIdRef::Kind() { return(m_Kind); }
	// SkReservedId Container =================================================
	tLemonReserved::tLemonReserved() : tClass(), m_EmptyLemonIdRef(){}

	tLemonReserved::~tLemonReserved() { Clear(); }

	void tLemonReserved::Clear() {
		m_MapIdRef.clear();
	}
	// Id =====================================================================
	tBool tLemonReserved::AddIdRef(tString sName, tInt sId, tVirtualClass* sClass,tKind sKind) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator != m_MapIdRef.end()) {
			return(false);
		}
		m_MapIdRef[sName] = tLemonIdRef(sId, sClass,sKind);
		return(true);
	}

	tBool tLemonReserved::DeleteIdRef(tString sName) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator != m_MapIdRef.end()) {
			m_MapIdRef.erase(wIterator);
			return(true);
		}
		return(false);
	}

	tLemonIdRef& tLemonReserved::IdRef(tString sName) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator == m_MapIdRef.end()) {
			return(m_EmptyLemonIdRef);
		}
		return(m_MapIdRef[sName]);
	}

	tInt tLemonReserved::Id(tString sName) {
		tLemonIdRef wIdRef = IdRef(sName);
		return(wIdRef.Id());
	}

	void tLemonReserved::Debug() {
		cout << "SkReservedId ------------" << endl;
		for (auto wOp : m_MapIdRef) {
			cout << wOp.first << endl;
		}
	}

} // end of namespace