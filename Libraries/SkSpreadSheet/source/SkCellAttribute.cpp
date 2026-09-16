//=============================================================================
// SkSpreadSheet CellAttribute 
//=============================================================================
#include "../include/SkCellAttribute.hpp"
#include "../include/SkSheet.hpp"

namespace SkSpreadSheet {

	// CellAttribute ==========================================================

	tCellAttribute::tCellAttribute() : tCell(), m_Name() {
		Type(tTypeItem::t_Attribute);
	}

	tCellAttribute::tCellAttribute(tString sName) : tCell(), m_Name() {
		Type(tTypeItem::t_Attribute);
	}

	tCellAttribute::tCellAttribute(const tCellAttribute& sCellAttribute) : tCell(sCellAttribute) {
		Type(tTypeItem::t_Attribute);
		m_Name = sCellAttribute.m_Name;
	}

	//tCellAttribute::~tCellAttribute() {}

	void tCellAttribute::Name(tString sName) { m_Name = sName; };
	tString tCellAttribute::Name() { return(m_Name()); }

	const tString tCellAttribute::StrRef(tBool sSheetName) const {
        tStringStream wStream;
        if (sSheetName) wStream << Sheet()->Name()<<"!";
        wStream << Base10ToAlpha(ColIndex()) << RowIndex();
		wStream << "." << m_Name();
		return(wStream.str());
	}

    // Json ===============================================================
    /// @brief        Writer Json.
    /// @param[in]    sWriter Writer<StringBuffer>*
    void tCellAttribute::Json(Writer<StringBuffer>* sWriter,tBool sR1C1,tPoint* sDiff) {
        sWriter->StartObject();
        sWriter->Key("n");
        sWriter->String(m_Name().c_str());
        m_Value.Json(sWriter);
        if (Formula() != nullptr) {
            sWriter->Key("f"); sWriter->String(FormulaStr(sR1C1).c_str());
        }
        sWriter->EndObject();
    }


	tBool tCellAttribute::operator < (const tCellAttribute& sCellAttribute) const {
		return(m_Name() < sCellAttribute.m_Name());
	}

#ifdef _DEBUGSK
	tString tCellAttribute::Debug() {
		tStringStream wStream;
		wStream << "Attribute :" << StrRef() << ":" << FormulaStr() << "=" << Value() << endl;
		return(wStream.str());
	}
#endif

#ifdef checksp
	/// @brief      Check.
	void tCellAttribute::Check() {
		tCell::Check();
	}
#endif

}; // end of namespace
