//=============================================================================
// SkSpreadSheet CellClassAtribute
//=============================================================================
#include "../include/SkCellClass.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#define _debugcellclass


namespace SkSpreadSheet {

    //! tCellModelClass =============================================================
    tCellModelClass::tCellModelClass(tString sName,tString sLabel,tString sFamily,  tFunctionCreate sFunctionCreate) : tModelClass(sName,sLabel, sFunctionCreate),m_Family(sFamily) {}


    void tCellModelClass::Family(tString sFamily) { m_Family=sFamily; };
    tString tCellModelClass::Family() { return(m_Family()); };

    tBool tCellModelClass::SaveModel() { return(false); }
    tBool tCellModelClass::SaveData() { return(true); }

    void tCellModelClass::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("n");
        sWriter->String(ClassName().c_str());
        sWriter->Key("l");
        sWriter->String(m_Label().c_str());
        sWriter->Key("fm");
        sWriter->String(m_Family().c_str());
        sWriter->Key("p");
        sWriter->StartArray();
        for (auto wProperty : m_OrderedPropertiesModel) {
            wProperty->Json(sWriter);
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

	// tCellClass =============================================================
    tCellClass::tCellClass() : tVariantClass() {}
    tCellClass::tCellClass(const tVariant& sValue) : tVariantClass(sValue) {}

    tCellClass::tCellClass(const tCellClass& sCellClass) : tVariantClass(sCellClass) {}

	tCellClass::~tCellClass() {}

    tVirtualClass* tCellClass::Clone() { return new tCellClass(*this); }

	tString tCellClass::ClassName() const { return("tCellClass"); }

    tBool tCellClass::Cover() const {
        return(false);
    }
    
    tBool tCellClass::IsCalculationPropagation() const {
        return(true);
    }

    void tCellClass::Json(Writer<StringBuffer>* sWriter) {
        m_Value.Json(sWriter);
    }

    void tCellClass::Json(const rapidjson::Value& sValue) {
        m_Value.Json(sValue);
    }


#ifdef checksp
	/// @brief      Check.
	void tCellClass::Check() {
		
	}
#endif

#ifdef _DEBUGSK		
	tString tCellClass::Debug() {
        tStringStream wStream;
		wStream << "ClassName=" << ClassName() << " ------------------" << endl;
        return(wStream.str());
	}
#endif
}
