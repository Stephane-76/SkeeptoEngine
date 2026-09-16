//=============================================================================
// SkReservedId for reserved word
//=============================================================================
#include "../include/SkLemonFormatReserved.hpp"

namespace SkFormat {

	// Reference for Reserved Id ==============================================
	tLemonFormatIdRef::tLemonFormatIdRef() : tClass(), m_Id(-1), m_Class(nullptr), m_Kind() {}
	tLemonFormatIdRef::tLemonFormatIdRef(const tLemonFormatIdRef& sLemonIdRef) : tClass(sLemonIdRef), m_Id(sLemonIdRef.m_Id), m_Class(sLemonIdRef.m_Class), m_Kind(sLemonIdRef.m_Kind){ }

	tLemonFormatIdRef::tLemonFormatIdRef(tInt sId, tVirtualClass* sClass, tKind sKind) : tClass(), m_Id(sId), m_Class(sClass), m_Kind(sKind){}

	tInt tLemonFormatIdRef::Id() { return(m_Id); }
	void tLemonFormatIdRef::Id(tInt sId) { m_Id = sId; }

	tVirtualClass* tLemonFormatIdRef::Class() { return(m_Class); }
	void tLemonFormatIdRef::Class(tVirtualClass* sClass) { m_Class = sClass; };

	tKind tLemonFormatIdRef::Kind() { return(m_Kind); }
	// SkReservedId Container =================================================
	tLemonFormatReserved::tLemonFormatReserved() : tClass(), m_EmptyLemonIdRef(){
        Init();
    }

	tLemonFormatReserved::~tLemonFormatReserved() { Clear(); }

	void tLemonFormatReserved::Clear() {
		m_MapIdRef.clear();
	}

    void tLemonFormatReserved::Init() {
        //cout << "Size Of sizeof(CstRecTabCss)" << sizeof(CstRecTabCss)/sizeof(tTabCss) << endl;
        tInt wSize=sizeof(CstRecCss)/sizeof(tRecCss);
        for(tInt wInd=0; wInd<wSize;wInd++ ) {
            const tRecCss* wTabCss=&CstRecCss[wInd];
            AddIdRef(wTabCss->m_Name,wTabCss->m_LemonCst,nullptr, wTabCss->m_Kind);
        }
    }
	// Id =====================================================================
	tBool tLemonFormatReserved::AddIdRef(const tString sName, const tInt sId, tVirtualClass* sClass,const tKind sKind) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator != m_MapIdRef.end()) {
			return(false);
		}
		m_MapIdRef[sName] = tLemonFormatIdRef(sId, sClass,sKind);
		return(true);
	}

	tBool tLemonFormatReserved::DeleteIdRef(tString sName) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator != m_MapIdRef.end()) {
			m_MapIdRef.erase(wIterator);
			return(true);
		}
		return(false);
	}

	tLemonFormatIdRef& tLemonFormatReserved::IdRef(tString sName) {
		auto wIterator = m_MapIdRef.find(sName);
		if (wIterator == m_MapIdRef.end()) {
			return(m_EmptyLemonIdRef);
		}
		return((*wIterator).second);
	}

	tInt tLemonFormatReserved::Id(tString sName) {
		tLemonFormatIdRef wIdRef = IdRef(sName);
		return(wIdRef.Id());
	}
    
    const tRecColor* tLemonFormatReserved::Color(tString sName) {
        tFormatRoot* wFormatRoot=tFormatRoot::Instance();
        return(wFormatRoot->FindColorByName(sName));
    }

	void tLemonFormatReserved::Debug() {
		cout << "SkReservedId ------------" << endl;
		for (auto wOp : m_MapIdRef) {
			cout << wOp.first <<   endl;
		}
	}

} // end of namespace
