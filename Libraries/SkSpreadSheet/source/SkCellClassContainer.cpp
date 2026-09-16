//=============================================================================
// SkSpreadSheet Container of Cell Class by Name
//=============================================================================
#include "../include/SkCellClassContainer.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"

#define _debugclasscontainer

namespace SkSpreadSheet {

    tCellClassAttribute* CreateGenericCellClassAttribute() {
        return(new tCellClassAttribute());
    }

	//! Container of CellClassContainer================================================
	tCellClassContainer::tCellClassContainer() : tClass(), m_ColRowCellRange(nullptr){}
	tCellClassContainer::~tCellClassContainer() {}

    void tCellClassContainer::Clear() {
        m_MapName.clear();
        m_MapRef.clear();
        m_MapCellJson.clear();
    }

	void tCellClassContainer::Set(tColRowCellRange* sColRowCellRange) {
		m_ColRowCellRange = sColRowCellRange;
	}

    tString tCellClassContainer::GetNextName(tString sName) {
        tMapName::iterator wIteratorName;
        tString wName=sName;
        wIteratorName = m_MapName.find(sName);
        if (wIteratorName != m_MapName.end()) {
            
            tSize wLast=wName.length();
            while((wName[wLast]>='0') && (wName[wLast]<='9')) {
                wName.pop_back();
                wLast=wName.length();
            }
            
            tSize wIndex=1;
            tString wKey=wName;
            wIteratorName = m_MapName.find(wName);
            while (wIteratorName != m_MapName.end()) {
                tStringStream wStream;
                wStream << wName << wIndex++;
                wIteratorName = m_MapName.find(wStream.str());
                wKey=wStream.str();
                wStream.str("");
            }
            wName=wKey;
        }
#ifdef debugclasscontainer
        cout << "tCellClassContainer::GetNextName(" << sName <<")->" << wName << endl;
#endif
        return(wName);
    }

	void tCellClassContainer::InsertCellClass(tString sName, tAllocatorRef sAllocatorRef) {
#ifdef debugclasscontainer
        cout << "tCellClassContainer::InsertCellClass(" << sName <<"," << sAllocatorRef << ")" << endl;
#endif
		m_MapName[sName] = sAllocatorRef;
		m_MapRef[sAllocatorRef] = sName;
	}

	tBool tCellClassContainer::DeleteCellClassByRef(tAllocatorRef sAllocatorRef) {
#ifdef debugclasscontainer
        cout << "tCellClassContainer::DeleteCellClassByRef(" << sAllocatorRef << ")" << endl;
#endif
		tMapAllocatorRefJsonPayLoad::iterator wIteratorRef;
		wIteratorRef = m_MapRef.find(sAllocatorRef);
		if (wIteratorRef != m_MapRef.end()) {
			tString wName = (*wIteratorRef).second;
			tMapName::iterator wIteratorName;
			wIteratorName = m_MapName.find(wName);
			if (wIteratorName != m_MapName.end()) {
				m_MapName.erase(wIteratorName);
			}
			else {
				tStringStream wStream;
				wStream << "throw: Check error DeleteCellClassContainerByRef not Name in m_MapName !";
				throw(new tExceptionInternalError(wStream.str()));
			}
			m_MapRef.erase(wIteratorRef);
            return(true);
		}
		return(false);
	}

	tBool tCellClassContainer::DeleteCellClassByName(tString sName) {
#ifdef debugclasscontainer
        cout << "tCellClassContainer::DeleteCellClassByNale(" << sName <<")";
#endif
		tMapName::iterator wIteratorName;
		wIteratorName = m_MapName.find(sName);
		if (wIteratorName != m_MapName.end()) {
			tAllocatorRef wRef = (*wIteratorName).second;
			tMapAllocatorRefJsonPayLoad::iterator wIteratorRef;
			wIteratorRef = m_MapRef.find(wRef);
			if (wIteratorRef != m_MapRef.end()) {
				m_MapRef.erase(wIteratorRef);
			}
			else {
				tStringStream wStream;
				wStream << "throw: Check error DeleteCellClassContainerByName not Name in m_MapRef !";
				throw(new tExceptionInternalError(wStream.str()));
			}
			m_MapName.erase(wIteratorName);
#ifdef debugclasscontainer
            cout << " ->Ok"  << endl;
#endif
			return(true);
		}
#ifdef debugclasscontainer
        cout << " ->D'ont find !"  << endl;
#endif
		return(false);
	}

	tCell* tCellClassContainer::CellByRef(tAllocatorRef sAllocatorRef) {
#ifdef debugclasscontainer
        cout << "tCellClassContainer::CellByRef(" << sAllocatorRef <<")" << endl;
#endif
        return(m_ColRowCellRange->Cell(sAllocatorRef));
	}

	tCell* tCellClassContainer::CellByName(tString sName) {
#ifdef debugclasscontainer
        cout << "tCellClassContainer::CellByName(" << sName <<")";
#endif
		tMapName::iterator wIteratorName;
		wIteratorName = m_MapName.find(sName);
		if (wIteratorName != m_MapName.end()) {
			tAllocatorRef wRef = (*wIteratorName).second;
#ifdef debugclasscontainer
            cout << " ->Ok"  << endl;
#endif
            return(m_ColRowCellRange->Cell(wRef));
		}
#ifdef debugclasscontainer
        cout << " ->D'ont find !"  << endl;
#endif
		return(nullptr);
	}

    tCellClassContainer::tMapName& tCellClassContainer::MapName() {
        return(m_MapName);
    }

    tCellClassContainer::tMapAllocatorRefJsonPayLoad& tCellClassContainer::MapRef() {
        return(m_MapRef);
    }

	// Json Payload Registry ================================================
	void tCellClassContainer::SetCellJsonPayload(tAllocatorRef sAllocatorRef, const tString& sJson) {
		if (sAllocatorRef>0) m_MapCellJson[sAllocatorRef] = sJson;
	}

	tBool tCellClassContainer::GetCellJsonPayload(tAllocatorRef sAllocatorRef, tString& sOutJson) {
		auto it = m_MapCellJson.find(sAllocatorRef);
		if (it == m_MapCellJson.end()) return(false);
		sOutJson = it->second;
		return(true);
	}

	tBool tCellClassContainer::DeleteCellJsonPayload(tAllocatorRef sAllocatorRef) {
		auto it = m_MapCellJson.find(sAllocatorRef);
		if (it == m_MapCellJson.end()) return(false);
		m_MapCellJson.erase(it);
		return(true);
	}

	tCellClassContainer::tMapCellJson& tCellClassContainer::MapCellJsonPayload() {
		return(m_MapCellJson);
	}

	void tCellClassContainer::Json(Writer<StringBuffer>* sWriter) {
		sWriter->Key("CellClass");
		sWriter->StartArray();
		for (auto wCellClassContainer : m_MapName) {
			sWriter->StartObject();
			tString wName = wCellClassContainer.first;
			sWriter->Key("n"); sWriter->String(wName.c_str());
            tAllocatorRef wCellRef=wCellClassContainer.second;
            tCell* wCell=m_ColRowCellRange->Cell(wCellRef);
            sWriter->Key("r"); sWriter->Int(wCell->Row()->Index());
            sWriter->Key("c"); sWriter->Int(wCell->ColIndex());
            
            // Optional JSON payload for this cell
            auto it = m_MapCellJson.find(wCellRef);
            if (it != m_MapCellJson.end()) {
                sWriter->Key("j");
                sWriter->String(it->second.c_str());
            }
            
 			sWriter->EndObject();
		}
		sWriter->EndArray();
	}

	void tCellClassContainer::Json(const Value& sValue) {

	}

#ifdef _DEBUGSK
    /// @brief      Debug.
    tString tCellClassContainer::Debug() {
        tStringStream wStream;
        wStream << "Cell Class Container =================" << endl;
        for(auto wItem : m_MapName) {
            tCell* wCell=m_ColRowCellRange->Cell(wItem.second);
            wStream << wCell->StrRef() << ":" << wItem.first << endl;
        }
        return(wStream.str());
    };
#endif
} // End of Name Space
