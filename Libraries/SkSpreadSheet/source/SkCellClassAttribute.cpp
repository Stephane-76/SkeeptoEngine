//=============================================================================
// SkSpreadSheet CellClassAtribute
//=============================================================================
#include "../include/SkCellClassAttribute.hpp"
#include "../include/SkCellClass.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#include <algorithm>
#include <cstring>
#include <set>
#define _debugcellclassattribute
#define _debugattribute

namespace SkSpreadSheet {

    namespace {
        // Try to store a date scalar (t_date) from wire UsDate or locale text — never leave "31/12/2026" as t_string.
        static tBool TryCoerceStringToDateVariant(tString sValue, tVariant& oOut) {
            if (sValue.empty()) {
                return(false);
            }
            // Never coerce bool wire values — Check/Switch calculable scalars.
            if (sValue == "true" || sValue == "false"
                || sValue == "TRUE" || sValue == "FALSE"
                || sValue == "1" || sValue == "0") {
                return(false);
            }

            auto wTryUsDate = [&](tString sCandidate) -> tBool {
                tClassDate wUsDate;
                wUsDate.UsDate(sCandidate);
                tInt wYear = 0;
                tInt wMonth = 0;
                tInt wDay = 0;
                wUsDate.YearMonthDay(wYear, wMonth, wDay);
                if (wYear >= 1900 && wYear <= 2100 && wMonth >= 1 && wMonth <= 12 && wDay >= 1 && wDay <= 31) {
                    oOut.SetDate(wUsDate.Value());
                    return(true);
                }
                return(false);
            };

            if (wTryUsDate(sValue)) {
                return(true);
            }

            tString wNormalized = sValue;
            std::replace(wNormalized.begin(), wNormalized.end(), '/', '-');
            if (wNormalized != sValue && wTryUsDate(wNormalized)) {
                return(true);
            }

            tVariant wParsed;
            wParsed.Parse(sValue);
            if (wParsed.Type() == tVariantType::t_date) {
                oOut = wParsed;
                return(true);
            }
            if (wNormalized != sValue) {
                wParsed.Parse(wNormalized);
                if (wParsed.Type() == tVariantType::t_date) {
                    oOut = wParsed;
                    return(true);
                }
            }
            return(false);
        }
    }
	//=========================================================================
	//! Sorted CellAttribute by operator < on SkRect
	class SkComparatorAttributeElem {
	public:
		/// @brief      Operator() compare with tCellAttribute < operator.
		/// @param[in]  sE2 SkRect*
		/// @param[in]  sE1 SkRect*
		SkInline bool operator()(tAttributeElem sE1, tAttributeElem sE2) {
			return(sE1.Name() < sE2.Name());
		}
	};

	//! tCellModelClass =======================================================
    tCellModelClassAttribute::tCellModelClassAttribute(tString sName,tString sLabel,tString sFamily, tFunctionCreate sFunctionCreate) : tCellModelClass(sName,sLabel,sFamily,sFunctionCreate) {}

    tBool tCellModelClassAttribute::SaveModel() { return(false); }
    tBool tCellModelClassAttribute::SaveData() { return(false); }

	void tCellModelClassAttribute::Property(tVirtualClass* sThis, tString sName, tVariant sValue) {
		tModelClass::Property(sThis, sName, sValue);
		tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(sThis);
		if (wCellClass != nullptr) {
			tCellAttribute* wCellAttribute = wCellClass->CellAttribute(sName);
			wCellAttribute->Value(sValue);
		}
	}

	tVariant tCellModelClassAttribute::Property(tVirtualClass* sThis, tString sName) {
		tVariant wResult = tModelClass::Property(sThis, sName);
		tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(sThis);
		if (wCellClass != nullptr) {
			tCellAttribute* wCellAttribute = wCellClass->Find(sName);
			if (wCellAttribute != nullptr) wResult = wCellAttribute->Value();
		}
		return(wResult);
	};

	// tAttributeElem =========================================================
	tAttributeElem::tAttributeElem(tString sName, tAllocatorRef sAllocatorRef) : tClass(), m_Name(sName), m_AllocatorRef(sAllocatorRef) {}

	tAttributeElem::tAttributeElem(const tAttributeElem& sAttributeElem) : tClass(), m_Name(sAttributeElem.m_Name), m_AllocatorRef(sAttributeElem.m_AllocatorRef) {}

	tString tAttributeElem::Name() { return(m_Name()); };
	tAllocatorRef tAttributeElem::AllocatorRef() { return(m_AllocatorRef); }

	// tAttributeContainer ====================================================
	tAttributeContainer::tAttributeContainer() : tClass(), m_NbInstance(1) {}
	tAttributeContainer::~tAttributeContainer() {};

	void tAttributeContainer::IncInstance() { m_NbInstance++;  }
	void tAttributeContainer::DecInstance() { m_NbInstance--; }
	tInt tAttributeContainer::NbInstance() { return(m_NbInstance); };

	tAllocatorRef tAttributeContainer::Alloc(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange) {
		tAllocatorRef wAllocatorRef = sColRowCellRange->AllocCellAttribute();
		tCellAttribute* wCellAttribute = sColRowCellRange->CellAttribute(wAllocatorRef);
		wCellAttribute->Name(sName);
		wCellAttribute->Rooted(sCellRoot);
#ifdef debugattribute
		cout << "tAttributeContainer::Alloc Attribute    " << sName << ":"  << wCellAttribute << ": Allocator " << wAllocatorRef;
		cout << " " << wCellAttribute->StrRef() << " <- " << sCellRoot->StrRef() << endl;
#endif
		// Rechange Type of Item not ste same of cell Parent

		return(wAllocatorRef);
	}

	tAllocatorRef tAttributeContainer::Find(tString sName) {
		tVectorAttributeElem::iterator wWhere;
		tAttributeElem wSearch(sName,0);
		wWhere = std::lower_bound(m_VectorAttributeElem.begin(), m_VectorAttributeElem.end(), wSearch, SkComparatorAttributeElem());
		if (wWhere != m_VectorAttributeElem.end()) {
			if ((*wWhere).Name() == sName) {
				return((*wWhere).AllocatorRef());
			}
		}
		return(0);
	}

	tAllocatorRef tAttributeContainer::Insert(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wWhere;
		tAttributeElem wSearch(sName,0);
		tAllocatorRef wAllocatorRef = 0;
		wWhere = std::lower_bound(m_VectorAttributeElem.begin(), m_VectorAttributeElem.end(), wSearch, SkComparatorAttributeElem());
		if (wWhere != m_VectorAttributeElem.end()) {
			if ((*wWhere).Name() != sName) {
				wAllocatorRef = Alloc(sName, sCellRoot, sColRowCellRange);
				m_VectorAttributeElem.insert(wWhere, tAttributeElem(sName,wAllocatorRef));
            }
			else {
				// Exist
				return(0);
			}
        }
		else {
			wAllocatorRef = Alloc(sName, sCellRoot, sColRowCellRange);
			m_VectorAttributeElem.insert(wWhere, tAttributeElem(sName, wAllocatorRef));
		}
		return(wAllocatorRef);
	}

	tCellAttribute* tAttributeContainer::CellAttributeGetorCreate(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange) {
		if (sCellRoot == nullptr) {
			tStringStream wStream;
			wStream << "throw: tAttributeContainer::CellAttribute -> Cell = nullptr !";
            cout << wStream.str() << endl;
			throw(tExceptionInternalError(wStream.str()));
		}
#ifdef debugattribute
        cout << " tAttributeContainer::CellAttribute(" << sCellRoot->StrRef() << "." << sName << ")" << endl;
#endif
		if (sCellRoot->PtValue()->Type() != tVariantType::t_class) {
			tStringStream wStream;
			wStream << "throw: tAttributeContainer::CellAttribute  ->" << Base10ToAlpha(sCellRoot->ColIndex()) << sCellRoot->Row()->Index() << "  on cell with not class !";
            cout << wStream.str() << endl;
			throw(tExceptionInternalError(wStream.str()));
		}
		tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(sCellRoot->PtValue()->Class());
		if (wCellClass == nullptr) {
			tStringStream wStream;
			wStream << "throw: tAttributeContainer::CellAttribute  ->" << Base10ToAlpha(sCellRoot->ColIndex()) << sCellRoot->Row()->Index() << "  wCellClass == nullptr !";
            cout << wStream.str() << endl;
			throw(tExceptionInternalError(wStream.str()));
		}
		tAllocatorRef wAllocatorRef = Find(sName);
		if (wAllocatorRef == 0) {
			tAllocatorRef wAllocatorRef = Insert(sName, sCellRoot, sColRowCellRange);
			tCellAttribute* wCellAttribute = sColRowCellRange->CellAttribute(wAllocatorRef);
			return(wCellAttribute);
		}
		else {
			return(sColRowCellRange->CellAttribute(wAllocatorRef));
		}
	}

    tCellAttribute* tAttributeContainer::CellAttribute(tSize sPos, tColRowCellRange* sColRowCellRange) {
        if ((sPos >= 0) && (sPos < m_VectorAttributeElem.size())) {
            tAttributeElem wAttributeElem = m_VectorAttributeElem[sPos];
            return(sColRowCellRange->CellAttribute(wAttributeElem.AllocatorRef()));
        }
        return(nullptr);
    }

	tBool tAttributeContainer::DeleteCellAttribute(tString sName, tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wWhere;
		tAllocatorRef wAllocatorRef = 0;
		tAttributeElem wSearch(sName, 0);
		wWhere = std::lower_bound(m_VectorAttributeElem.begin(), m_VectorAttributeElem.end(), wSearch, SkComparatorAttributeElem());
		if (wWhere != m_VectorAttributeElem.end()) {
			if ((*wWhere).Name() == sName) {
				wAllocatorRef = (*wWhere).AllocatorRef();
				m_VectorAttributeElem.erase(wWhere);
			}
		}
		if (wAllocatorRef != 0) {
			return(sColRowCellRange->DeleteCellAttribute(wAllocatorRef));
		}
		return(false);
	}

	void tAttributeContainer::ClearAttribute(tColRowCellRange* sColRowCellRange) {
		if (sColRowCellRange != nullptr) {
			tVectorAttributeElem::iterator wIterator;
			for (wIterator = m_VectorAttributeElem.begin(); wIterator != m_VectorAttributeElem.end(); wIterator++) {
				tAllocatorRef wAllocatorRef = (*wIterator).AllocatorRef();
				sColRowCellRange->DeleteCellAttribute(wAllocatorRef);
			}
		};
		m_VectorAttributeElem.clear();
	}
	void tAttributeContainer::ClearFormulaVariant(tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wIterator;
		for (wIterator = m_VectorAttributeElem.begin(); wIterator != m_VectorAttributeElem.end(); wIterator++) {
			tAllocatorRef wAllocatorRef = (*wIterator).AllocatorRef();
			sColRowCellRange->CellAttribute(wAllocatorRef)->ClearFormulaAndVariant();
		}
		ClearAttribute(sColRowCellRange);
	}

	void tAttributeContainer::Rooted(tCell* sCell, tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wIterator;
		for (wIterator = m_VectorAttributeElem.begin(); wIterator != m_VectorAttributeElem.end(); wIterator++) {
			tAllocatorRef wAllocatorRef = (*wIterator).AllocatorRef();
			sColRowCellRange->CellAttribute(wAllocatorRef)->Rooted(sCell);
#ifdef debugattribute
			tCellAttribute* wCellAttribute = sColRowCellRange->CellAttribute(wAllocatorRef);
			cout << "   Attribute  " << wCellAttribute << ":";
			cout << wCellAttribute->StrRef() << " <- " << sCell->StrRef() << endl;
#endif

		}
	}
	tSize tAttributeContainer::Size() { return(m_VectorAttributeElem.size()); }

#ifdef checksp
	/// @brief      Check.
	void tAttributeContainer::Check(tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wIterator;
		for (wIterator = m_VectorAttributeElem.begin(); wIterator != m_VectorAttributeElem.end(); wIterator++) {
			tAllocatorRef wAllocatorRef = (*wIterator).AllocatorRef();
			sColRowCellRange->CellAttribute(wAllocatorRef)->Check();
		}
	}
#endif

#ifdef _DEBUGSK
	tString tAttributeContainer::Debug(tColRowCellRange* sColRowCellRange) {
		tVectorAttributeElem::iterator wIterator;
		tStringStream wStream;
		for (wIterator = m_VectorAttributeElem.begin(); wIterator != m_VectorAttributeElem.end(); wIterator++) {
			tAllocatorRef wAllocatorRef = (*wIterator).AllocatorRef();
			tCellAttribute* wCellAttribute = sColRowCellRange->CellAttribute(wAllocatorRef);
			wStream << "  " << wCellAttribute->Name().c_str() << " Allocator=" << wAllocatorRef << endl;
			wStream << wCellAttribute->Debug();
		}
		return(wStream.str());
	}
#endif

	// tCellClass =============================================================
    tCellClassAttribute::tCellClassAttribute() : tCellClass(),
        m_ClassName("tCellClassAttribute"),
        m_RefName("tCellClassAttribute"),
		m_SheetIndice(0),
		m_CellRootRef(0),
        m_Id(0),
		m_ModelClass(nullptr) {
		m_AttributeContainer = new tAttributeContainer();
	}

	tCellClassAttribute::tCellClassAttribute(const tCellClassAttribute& sCellClassAttribute) : tCellClass(sCellClassAttribute) {
		m_SheetIndice = sCellClassAttribute.m_SheetIndice;
        m_ClassName=sCellClassAttribute.m_ClassName;
        m_RefName = sCellClassAttribute.m_RefName;
		m_CellRootRef = sCellClassAttribute.m_CellRootRef;
        m_Id = sCellClassAttribute.m_Id;
		m_ModelClass = sCellClassAttribute.m_ModelClass;
		m_AttributeContainer = sCellClassAttribute.m_AttributeContainer;
		m_AttributeContainer->IncInstance();
	}

	tCellClassAttribute::~tCellClassAttribute() {
		m_AttributeContainer->DecInstance();
		if (m_AttributeContainer->NbInstance() == 0) {
			delete(m_AttributeContainer);
		};
	}

    tVirtualClass* tCellClassAttribute::Clone() { return new tCellClassAttribute(*this); }

    tBool tCellClassAttribute::IsCalculationPropagation() const { return(false); };

    static tBool IsComboBoxStringCalculableClass(const tCellClassAttribute* sAttr) {
        return sAttr != nullptr && sAttr->ClassName() == "SkCellClassComboBox";
    }

    tVariant& tCellClassAttribute::CalculableValue() {
        tVariant* wValue = Value();
        // ComboBox labels must stay t_string for formulas (=G6), never auto-coerce to t_date.
        if (!IsComboBoxStringCalculableClass(this)
            && wValue->Type() == tVariantType::t_string
            && !wValue->String().empty()) {
            tVariant wCoerced;
            if (TryCoerceStringToDateVariant(wValue->String(), wCoerced)) {
                SetCalculableValue(wCoerced);
                wValue = Value();
            }
        }
        return(*wValue);
    }

    const tVariant& tCellClassAttribute::CalculableValue() const {
        return(m_Value);
    }

    void tCellClassAttribute::SetCalculableValue(const tVariant& sValue) {
        if (IsComboBoxStringCalculableClass(this)) {
            tVariant wStringValue;
            if (sValue.Type() == tVariantType::t_string) {
                wStringValue.SetString(sValue.String());
            } else {
                wStringValue.SetString(sValue.Str());
            }
            Value(&wStringValue);
            return;
        }
        if (sValue.Type() == tVariantType::t_string) {
            tVariant wCoerced;
            if (TryCoerceStringToDateVariant(sValue.String(), wCoerced)) {
                Value(&wCoerced);
                return;
            }
        }
        Value(const_cast<tVariant*>(&sValue));
    }

	void tCellClassAttribute::ClearFormulaVariant() {
		tColRowCellRange* wColRowCellRange = ColRowCellRange();
		m_AttributeContainer->ClearFormulaVariant(wColRowCellRange);
	}

	void tCellClassAttribute::ClearAttribute() {
		tColRowCellRange* wColRowCellRange = ColRowCellRange();
		m_AttributeContainer->ClearAttribute(wColRowCellRange);
	}

    void tCellClassAttribute::RefName(tString sName) { m_RefName = sName; };
    tString tCellClassAttribute::RefName() { return(m_RefName()); }

    void tCellClassAttribute::Id(tInt sId) { m_Id = sId; };
    tInt tCellClassAttribute::Id() { return(m_Id); }

	tColRowCellRange* tCellClassAttribute::ColRowCellRange() {
		return(tStaticColRowCellRange::Instance()->ColRowCellRange(m_SheetIndice));
	}

tString tCellClassAttribute::ClassName() const { return(m_ClassName()); }

void tCellClassAttribute::ClassName(tString sClassName) { m_ClassName=sClassName; }


	void tCellClassAttribute::SheetIndice(tIndex sSheetIndice) { m_SheetIndice = sSheetIndice; }
	tIndex tCellClassAttribute::SheetIndice() { return(m_SheetIndice); }

	void tCellClassAttribute::CellRootRef(tColRowCellRange* sColRowCellRange,tIndex sRow,tIndex sCol) {
		tColRowCellRange* wColRowCellRange = sColRowCellRange;
        m_SheetIndice=sColRowCellRange->SheetAllocator();
        m_CellRootRef = wColRowCellRange->CellAllocatorRef(sRow,sCol);
		
	}

	void tCellClassAttribute::Rooted(tCell* sCell) {
#ifdef debugattribute
		cout << "Rooted " << sCell->StrRef() << endl;
#endif
		m_AttributeContainer->Rooted(sCell,ColRowCellRange());
	}

	tAllocatorRef tCellClassAttribute::CellRootRef() { return(m_CellRootRef); }
	void tCellClassAttribute::SetModelClass(tModelClass* sModelClass) {
        if (sModelClass!=nullptr) {
            m_ModelClass=sModelClass;
        } else {
            m_ModelClass = tClassFactory::Instance()->Get(ClassName());
        }
	}
	tModelClass* tCellClassAttribute::ModelClass() { return(m_ModelClass); };


	tCellAttribute* tCellClassAttribute::Find(tString sName) {
		tAllocatorRef wAllocatorRef = m_AttributeContainer->Find(sName);
		return(ColRowCellRange()->CellAttribute(wAllocatorRef));
	}

	tBool tCellClassAttribute::DeleteCellAttribute(tString sName) {
		return(m_AttributeContainer->DeleteCellAttribute(sName,ColRowCellRange()));
	}
	
	tAllocatorRef tCellClassAttribute::CellAttributeRef(tString sName) {
		return(m_AttributeContainer->Find(sName));
	}
		
	tCellAttribute* tCellClassAttribute::CellAttribute(tString sName) {
		tColRowCellRange* wColRowCellRange = ColRowCellRange();
		tCell* wCellRoot = wColRowCellRange->Cell(m_CellRootRef);

        return(m_AttributeContainer->CellAttributeGetorCreate(sName, wCellRoot, wColRowCellRange));
	}

	tCellAttribute* tCellClassAttribute::CellAttribute(tIndex sPos) { return(m_AttributeContainer->CellAttribute(sPos, ColRowCellRange())); }

    tSize    tCellClassAttribute::Size() { return(m_AttributeContainer->Size()); };

    void  tCellClassAttribute::CellAttribute(tVectorCellAttribute* sVectorCellAttribute) {
        for(tIndex wIndex=0; wIndex< Size(); wIndex++) {
            tCellAttribute* wCellAttribute=CellAttribute(wIndex);
            sVectorCellAttribute->push_back(wCellAttribute);
        }
    }

    tBool tCellClassAttribute::IsReactComponent() { return(true); }

    void tCellClassAttribute::JsonCell(Writer<StringBuffer>* sWriter, tBool sR1C1, tPoint* sDiff) {
        sWriter->StartObject();
        // Formula-visible scalar (CalculableValue) — same keys as tVariant::Json ("t", "v").
        Value()->Json(sWriter);
        sWriter->Key("n");
        sWriter->String(m_RefName().c_str());
        sWriter->Key("a");
        sWriter->StartArray();
        
        for(tIndex wIndex=0; wIndex< Size(); wIndex++) {
            tCellAttribute* wCellAttribute=CellAttribute(wIndex);
            wCellAttribute->Json(sWriter,sR1C1);
        }
        
        sWriter->EndArray();
        sWriter->EndObject();
    }
    
    void tCellClassAttribute::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        Value()->Json(sWriter);
        sWriter->Key("n");
        sWriter->String(m_RefName().c_str());
        sWriter->Key("a");
        sWriter->StartArray();
        
        for(tIndex wIndex=0; wIndex< Size(); wIndex++) {
            tCellAttribute* wCellAttribute=CellAttribute(wIndex);
            wCellAttribute->Json(sWriter,false);
        }
        
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tCellClassAttribute::Json(const rapidjson::Value& sValue) {
        tCell* wCell=tSpreadSheetContainer::Instance()->CurrentJsonCell();
#ifdef debugcellclassattribute
        cout << "tCellClass::Json()->" << wCell->StrRef() << endl;
#endif
        // m_Current Indice Sheet is not Correct (On Json operartion)
        tColRowCellRange* wColRowCellRange=wCell->Sheet()->ColRowCellRange();
        
        m_CellRootRef = wColRowCellRange->CellAllocatorRef(wCell->Row()->Index(),wCell->ColIndex());
        m_SheetIndice=wCell->Sheet()->IndexAllocatorColRowCellRange();
        
       
        tCell* wCellRoot = wColRowCellRange->Cell(m_CellRootRef);
#ifdef debugattribute
        cout << "Json :: CellRoot  " << wCellRoot->StrRef() << " Index:" << wCellRoot->IndexAllocatorCellRange() << endl;
        cout << wColRowCellRange->Cell(m_CellRootRef)->StrRef() << endl;
#endif
        m_RefName=sValue["n"].GetString();

        if (sValue.HasMember("t")) {
            m_Value.Json(sValue);
        }
        
        const rapidjson::Value& wAttributes=sValue["a"];
        assert(wAttributes.IsArray());
        for (Value::ConstValueIterator wIterator = wAttributes.Begin();
             wIterator != wAttributes.End(); ++wIterator) {
            const rapidjson::Value& wValue=(*wIterator);
            tString wName=wValue["n"].GetString();
            tString wFormulaStr="";
            if (wValue.HasMember("f")) {
                wFormulaStr=wValue["f"].GetString();
            }
            m_ModelClass = tClassFactory::Instance()->Get(ClassName());
            tVariant wVariantValue;
            wVariantValue.Json(wValue);
           
            tCellAttribute* wCellAttribute =CellAttribute(wName);
            
            if (wCellAttribute == nullptr) {
                wCellAttribute=m_AttributeContainer->CellAttributeGetorCreate(wName, wCellRoot, wColRowCellRange);
            }
            if (wFormulaStr != "") {
                wCellAttribute->Value(wFormulaStr);
                // Calculation at the end of process json
                tSpreadSheetContainer::Instance()->PushJsonCell(wCellAttribute);
            } else {
                wCellAttribute->Value(wVariantValue);
            }
#ifdef debugattribute
            cout << "Attribute " << wName << ":" << wCellAttribute->FormulaStr() << " Value=" << wCellAttribute->Value() << endl;
#endif
        }
    }

#ifdef checksp
	/// @brief      Check.
	void tCellClassAttribute::Check() {
		tColRowCellRange* wColRowCellRange = ColRowCellRange();
		m_AttributeContainer->Check(wColRowCellRange);
	}
#endif

#ifdef _DEBUGSK		
	tString tCellClassAttribute::Debug() {
        tStringStream wStream;
		wStream << "ClassName=" << ClassName() << " ------------------" << endl;
		wStream << m_AttributeContainer->Debug(ColRowCellRange());
        return(wStream.str());
	}
#endif

    tBool RegisterCellClassModelsFromJson(const rapidjson::Value& sModels) {
        InstallCellClassModelStubHandler();
        if (!sModels.IsArray()) {
            return false;
        }

        tClassFactory* wFactory = tClassFactory::Instance();
        tBool wProcessed = false;

        for (rapidjson::SizeType wIndex = 0; wIndex < sModels.Size(); ++wIndex) {
            const rapidjson::Value& wModel = sModels[wIndex];
            if (!wModel.IsObject() || !wModel.HasMember("n") || !wModel["n"].IsString()) {
                continue;
            }

            tString wClassName = wModel["n"].GetString();
            tString wLabel = wClassName;
            if (wModel.HasMember("l") && wModel["l"].IsString()) {
                wLabel = wModel["l"].GetString();
            }

            tString wFamily = "Javascript";
            if (wModel.HasMember("fm") && wModel["fm"].IsString()) {
                wFamily = wModel["fm"].GetString();
            }

            tCellModelClassAttribute* wCellModel =
                dynamic_cast<tCellModelClassAttribute*>(wFactory->Get(wClassName));
            if (wCellModel == nullptr) {
                wCellModel = new tCellModelClassAttribute(
                    wClassName, wLabel, wFamily, &CreateGenericCellClassAttribute);
                if (!wFactory->Register(wCellModel)) {
                    delete wCellModel;
                    wCellModel = dynamic_cast<tCellModelClassAttribute*>(wFactory->Get(wClassName));
                }
            }

            if (wCellModel == nullptr) {
                continue;
            }

            if (wModel.HasMember("p") && wModel["p"].IsArray()) {
                const rapidjson::Value& wProperties = wModel["p"];
                for (rapidjson::SizeType wPropIndex = 0; wPropIndex < wProperties.Size(); ++wPropIndex) {
                    tModelProperty* wModelProperty = new tModelProperty();
                    wModelProperty->Json(wProperties[wPropIndex]);
                    wCellModel->AddProperty(wModelProperty);
                }
            }

            wProcessed = true;
        }

        return wProcessed;
    }

    tBool EnsureCellClassModelStub(tString sClassName) {
        if (sClassName.empty()) {
            return false;
        }
        tClassFactory* wFactory = tClassFactory::Instance();
        if (wFactory->Get(sClassName) != nullptr) {
            return true;
        }
        tCellModelClassAttribute* wCellModel = new tCellModelClassAttribute(
            sClassName, sClassName, "Javascript", &CreateGenericCellClassAttribute);
        if (!wFactory->Register(wCellModel)) {
            delete wCellModel;
        }
        return wFactory->Get(sClassName) != nullptr;
    }

    void InstallCellClassModelStubHandler() {
        tClassFactory::SetMissingClassStubHandler(EnsureCellClassModelStub);
    }

    namespace {
        static void CollectClassNamesFromFloatingObjectsJson(
            const rapidjson::Value& sWorkbook, std::set<tString>& oClassNames) {
            if (!sWorkbook.HasMember(kJsonKeyFloatingObjects)) {
                return;
            }
            const rapidjson::Value& wArray = sWorkbook[kJsonKeyFloatingObjects];
            if (!wArray.IsArray()) {
                return;
            }
            for (rapidjson::SizeType wIndex = 0; wIndex < wArray.Size(); ++wIndex) {
                const rapidjson::Value& wObject = wArray[wIndex];
                if (wObject.HasMember("c") && wObject["c"].IsString()) {
                    oClassNames.insert(wObject["c"].GetString());
                }
            }
        }

        static void CollectClassNamesFromSheetsJson(
            const rapidjson::Value& sWorkbook, std::set<tString>& oClassNames) {
            if (!sWorkbook.HasMember("sheets") || !sWorkbook["sheets"].IsArray()) {
                return;
            }
            const rapidjson::Value& wSheets = sWorkbook["sheets"];
            for (rapidjson::SizeType wSheetIndex = 0; wSheetIndex < wSheets.Size(); ++wSheetIndex) {
                const rapidjson::Value& wSheet = wSheets[wSheetIndex];
                if (!wSheet.HasMember("cells") || !wSheet["cells"].IsArray()) {
                    continue;
                }
                const rapidjson::Value& wCells = wSheet["cells"];
                for (rapidjson::SizeType wCellIndex = 0; wCellIndex < wCells.Size(); ++wCellIndex) {
                    const rapidjson::Value& wCell = wCells[wCellIndex];
                    if (wCell.HasMember("class") && wCell["class"].IsString()) {
                        oClassNames.insert(wCell["class"].GetString());
                    }
                    if (wCell.HasMember("t") && wCell["t"].IsString()
                        && std::strcmp(wCell["t"].GetString(), "c") == 0
                        && wCell.HasMember("v") && wCell["v"].IsObject()
                        && wCell["v"].HasMember("n") && wCell["v"]["n"].IsString()) {
                        oClassNames.insert(wCell["v"]["n"].GetString());
                    }
                }
            }
        }
    }

    tBool RegisterMissingCellClassModelsFromWorkbookJson(const rapidjson::Value& sWorkbook) {
        InstallCellClassModelStubHandler();
        if (!sWorkbook.IsObject()) {
            return false;
        }
        std::set<tString> wClassNames;
        CollectClassNamesFromFloatingObjectsJson(sWorkbook, wClassNames);
        CollectClassNamesFromSheetsJson(sWorkbook, wClassNames);
        if (wClassNames.empty()) {
            return false;
        }
        for (const tString& wClassName : wClassNames) {
            EnsureCellClassModelStub(wClassName);
        }
        return true;
    }
}
