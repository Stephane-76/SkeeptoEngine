//=============================================================================
// SkRoot SharedFormula
// Manage shared Formula
// Author : Stephane Allez 
//=============================================================================
#include "../include/SkSharedFormula.hpp"
#include "../include/SkApplication.hpp"
#include "../include/SkSpreadSheet.hpp"

#define _debugformula

namespace SkSpreadSheet {
	// tSharedFormulaItem =================================================
	tSharedFormulaItem::tSharedFormulaItem() : tClass(), m_Count(0), m_AllocatorRef(0), m_Formula() {}
	tSharedFormulaItem::tSharedFormulaItem(const tFormula& sFormula) : tClass(), m_Count(0), m_AllocatorRef(0), m_Formula(sFormula){}

	tSharedFormulaItem::~tSharedFormulaItem() {}

	/// @brief      Clear.
	void tSharedFormulaItem::Clear() {}

	void tSharedFormulaItem::AllocatorRef(tAllocatorRef sAllocatorRef) { m_AllocatorRef = sAllocatorRef; }
	tAllocatorRef tSharedFormulaItem::AllocatorRef() { return(m_AllocatorRef); }

	void tSharedFormulaItem::Formula(const tFormula sFormula) { m_Formula = sFormula; }
	tFormula* tSharedFormulaItem::Formula() { return(&m_Formula); }
	const tFormula* tSharedFormulaItem::Formula() const { return(&m_Formula); }



	void tSharedFormulaItem::Inc() { m_Count++; }
	void tSharedFormulaItem::Dec() { m_Count--; }
	tBool tSharedFormulaItem::IsEmpty() { return(m_Count == 0); }

	tString tSharedFormulaItem::operator()() { 
		return(m_Formula());
	}
	
	// tSharedFormulaPool ================================================
	tSharedFormulaPool::tSharedFormulaPool() : tClass() {}
	tSharedFormulaPool::~tSharedFormulaPool() {
		m_MapFormulaItem.clear();
	};

	void tSharedFormulaPool::Clear() {
		m_MapFormulaItem.clear();
	}




	tSharedFormulaItem* tSharedFormulaPool::Get(const tFormula& sFormula) {
		tAllocatorRef wAllocatorRef = m_MapFormulaItem[sFormula()];
		return(m_AllocatorFormulaItem(wAllocatorRef));
	}

	tSharedFormulaItem* tSharedFormulaPool::Get(tIndex sIndex) {
		return(m_AllocatorFormulaItem(sIndex));
	}

	tSharedFormulaItem* tSharedFormulaPool::Add(const tFormula& sFormula) {

		tAllocatorRef wAllocatorRef = m_MapFormulaItem[sFormula()];
		tSharedFormulaItem* wSharedFormulaItem = m_AllocatorFormulaItem(wAllocatorRef);
		if (wSharedFormulaItem != nullptr) {
			wSharedFormulaItem->Inc();
			return(wSharedFormulaItem);
		}
		tie(wAllocatorRef, wSharedFormulaItem) = m_AllocatorFormulaItem.Alloc();
		wSharedFormulaItem->Inc();
		m_MapFormulaItem[sFormula()] = wAllocatorRef;
#ifdef debugformula
        cout << "Search " << sFormula() << " ";
        cout << "Return " << wAllocatorRef;
#endif
		wSharedFormulaItem->AllocatorRef(wAllocatorRef);
		wSharedFormulaItem->Formula(sFormula);
#ifdef debugformula
        cout << " wSharedFormulaItem " << wSharedFormulaItem->Formula()->Debug() << endl;
#endif
		return(wSharedFormulaItem);
	};

	void tSharedFormulaPool::Remove(tSharedFormulaItem* sSharedFormulaItem) {
		sSharedFormulaItem->Dec();
		if (sSharedFormulaItem->IsEmpty()) {
#ifdef debugformula
            cout << "tSharedFormulaPool::Remove " << sSharedFormulaItem->AllocatorRef();
#endif
			tAllocatorRef wAllocatorRef = sSharedFormulaItem->AllocatorRef();
			tString wFormulaStr = (*sSharedFormulaItem)();
			m_AllocatorFormulaItem.Delete(wAllocatorRef);
#ifdef debugformula
            cout << "." << endl;
#endif
			tMapFormulaItem::iterator wIterator = m_MapFormulaItem.find(wFormulaStr);
			m_MapFormulaItem.erase(wIterator);
			delete(sSharedFormulaItem);
		}
	}

	tSize tSharedFormulaPool::Size() {
		return(m_MapFormulaItem.size());
	}



	// tSharedFormula =====================================================
	tSharedFormula::tSharedFormula() : tClass(), m_SharedFormulaItem(nullptr){}

	tSharedFormula::tSharedFormula(const tSharedFormula& sSharedFormula) : tClass(sSharedFormula) {
		m_SharedFormulaItem = sSharedFormula.m_SharedFormulaItem;
		m_SharedFormulaItem->Inc();
	}
	
	tSharedFormula::tSharedFormula(const tFormula& sFormula) : tClass() { 
		m_SharedFormulaItem = tSpreadSheetContainer::Instance()->SharedFormulaPool()->Add(sFormula); 
	}

	tFormula* tSharedFormula::Formula() {
		if (m_SharedFormulaItem != nullptr) {
			return(m_SharedFormulaItem->Formula());
		}
		return(nullptr);
	}
	
	const tFormula* tSharedFormula::Formula() const {
		if (m_SharedFormulaItem != nullptr) {
			return(m_SharedFormulaItem->Formula());
		}
		return(nullptr);
	}

	
	void tSharedFormula::Clear() {
		if (m_SharedFormulaItem != nullptr) {
			tSpreadSheetContainer::Instance()->SharedFormulaPool()->Remove(m_SharedFormulaItem);
			m_SharedFormulaItem = nullptr;
		}
	}

	
	tSharedFormula& tSharedFormula::operator =(const tSharedFormula& sSharedFormula) {
		if (this != &sSharedFormula) {
            // Delete Old Value
            if (m_SharedFormulaItem != nullptr)
                tSpreadSheetContainer::Instance()->SharedFormulaPool()->Remove(m_SharedFormulaItem);
            
            if (sSharedFormula.m_SharedFormulaItem != nullptr) {
                m_SharedFormulaItem = sSharedFormula.m_SharedFormulaItem;
                m_SharedFormulaItem->Inc();
            } else {
                // Must null after Remove: otherwise Formula() returns a dangling item.
                m_SharedFormulaItem = nullptr;
            }
        }
		return(*this);
	}

	tBool tSharedFormula::operator ==(const tSharedFormula& sSharedFormula) const {
		return(m_SharedFormulaItem == sSharedFormula.m_SharedFormulaItem);
	}

	tString tSharedFormula::operator()() { 
		if (m_SharedFormulaItem != nullptr) {
			return((*m_SharedFormulaItem)());
		}
		return("");
	}

}; // Fin du name space ========================================================
