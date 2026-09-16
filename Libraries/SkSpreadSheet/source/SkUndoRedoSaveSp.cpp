//=============================================================================
// SkSpreadSheet save element for Undo Redo
//=============================================================================
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkUndoRedoRebaseFormula.hpp"

namespace SkSpreadSheet {

#define _debugundoredo

    // Get Indice of Sheet & tPoint of cell
    tuple<tIndex, tTempoPoint*> GetCellRef(tString sCellRef, tSheet* sSheet) {
    
    // Check if reference contains sheet name (format: "Sheet1!A10")
    tTempoPoint* wPointAdress = new tTempoPoint();
    tSheet* wSheet = nullptr;
    tIndex wSheetAllocator = 0;
    tSize wExclamationPos = sCellRef.find('!');
    if (wExclamationPos != tString::npos) {
        // Extract sheet name and cell address
        tString wSheetName = sCellRef.substr(0, wExclamationPos);
        tString wCellAddress = sCellRef.substr(wExclamationPos + 1);
        
        // Parse cell address
        tTempoPoint wPoint(wCellAddress);
        *wPointAdress = wPoint;
        
        // Find sheet by name and get its index
        wSheet = sSheet->WorkBook()->Sheet(wSheetName);
        if (wSheet != nullptr) {
            //Store IndiceSheet
            wSheetAllocator = wSheet->ColRowCellRange()->SheetAllocator();
        }
    } else {
        // Simple cell reference without sheet name
        tTempoPoint wPoint(sCellRef);
        *wPointAdress = wPoint;
    }
    return(make_tuple(wSheetAllocator, wPointAdress));
}

// Get Indice of Sheet & Rect of range
tuple<tIndex, tTempoRect*> GetRangeRef(tString sRangeRef, tSheet* sSheet) {
    // Check if reference contains sheet name (format: "Sheet1!A10")
    tTempoRect* wRectAdress = new tTempoRect();
    tSheet* wSheet = nullptr;
    tAllocatorRef wSheetAllocator = 0;
    tSize wExclamationPos = sRangeRef.find('!');
    if (wExclamationPos != tString::npos) {
        // Extract sheet name and cell address
        tString wSheetName = sRangeRef.substr(0, wExclamationPos);
        tString wRangeAddress = sRangeRef.substr(wExclamationPos + 1);
        
        // Parse cell address
        tTempoRect wRect(wRangeAddress);
        *wRectAdress = wRect;
        // Update sheet information based on compilation mode
        wSheet = sSheet->WorkBook()->Sheet(wSheetName);
        // Find sheet by name and get its index
        if (wSheet != nullptr) {
            wSheetAllocator = wSheet->ColRowCellRange()->SheetAllocator();
        }
    } else {
        // Simple cell reference without sheet name
        tTempoRect wRect(sRangeRef);
        *wRectAdress = wRect;
    }
    return(make_tuple(wSheetAllocator, wRectAdress));
}


    tSave::tSave() : tClass(),
    m_ColRowCellRangeRef(0)
    {}

    tSave::tSave(tAllocatorRef sColRowCellRangeRef) : tClass(), m_ColRowCellRangeRef(sColRowCellRangeRef) {}
    tSave::tSave(const tSave& sSave) : tClass(sSave) {
        m_ColRowCellRangeRef=sSave.m_ColRowCellRangeRef;
    }


    tAllocatorRef tSave::ColRowCellRangeRef() { return (m_ColRowCellRangeRef); }


    tColRowCellRange* tSave::ColRowCellRange() {
        return(tStaticColRowCellRange::Instance()->ColRowCellRange(m_ColRowCellRangeRef));
    }


    tSheet* tSave::Sheet() {
        return(ColRowCellRange()->Sheet());
   }

    tRebasePlan tSave::IsRebaseAnotherSheet(tRebasePlan sRebasePlan) {
        // Is Another Sheet ==================================================
        tRebasePlan wRebasePlan=sRebasePlan;
        tAllocatorRef wSheetRef=Sheet()->AllocatorRef();
        if (sRebasePlan.m_SheetAllocator!= wSheetRef) {
            wRebasePlan=Sheet()->WorkBook()->UndoRebaseLog().BuildPlanAnotherSheet(sRebasePlan,wSheetRef);
        }
        return(wRebasePlan);
    }

    tRebasePlan tSave::IsRebaseAnotherSheet(tRebasePlan sRebasePlan,tAllocatorRef sSheetRef) {
        // Is Another Sheet ==================================================
        tRebasePlan wRebasePlan=sRebasePlan;
        if (sRebasePlan.m_SheetAllocator!= sSheetRef) {
            wRebasePlan=Sheet()->WorkBook()->UndoRebaseLog().BuildPlanAnotherSheet(sRebasePlan,sSheetRef);
        }
        return(wRebasePlan);
    }

	//========================================================================
    tSaveFormulaCell::tSaveFormulaCell() : tSave(0),m_Type(tTypeItem::t_Cell),m_Row(-1),m_Col(-1),m_Index(0),m_Attribute(),m_SharedFormula() {}

    tSaveFormulaCell::tSaveFormulaCell(tAllocatorRef sSheetAllocator, tTypeItem sType, tIndex sRow, tIndex sCol, tString sAttribute,tIndex sIndex) : tSave(sSheetAllocator),
		m_Type(sType),
		m_Row(sRow), 
		m_Col(sCol), 
		m_Index(sIndex),
		m_Attribute(sAttribute),
		m_SharedFormula() {
	        Formula(Cell()->Formula());
        }
	tSaveFormulaCell::tSaveFormulaCell(const tSaveFormulaCell& sSaveFormulaCell): tSave(sSaveFormulaCell) {
		m_Row = sSaveFormulaCell.m_Row;
		m_Col = sSaveFormulaCell.m_Col;
		m_Index = sSaveFormulaCell.m_Index;
        m_Attribute = sSaveFormulaCell.m_Attribute;
        m_SharedFormula = sSaveFormulaCell.m_SharedFormula;
	}

	tSaveFormulaCell::~tSaveFormulaCell() {
        m_SharedFormula.Clear();
    }


	tTypeItem tSaveFormulaCell::Type() { return(m_Type); };
	tIndex tSaveFormulaCell::Row() { return(m_Row); }
	tIndex tSaveFormulaCell::Col() { return(m_Col); }
	tString tSaveFormulaCell::Attribute() { return(m_Attribute()); };

	tIndex tSaveFormulaCell::Index() { return(m_Index); }

	tCell* tSaveFormulaCell::Cell() {
		tColRowCellRange* wColRowCellRange = tStaticColRowCellRange::Instance()->ColRowCellRange(m_ColRowCellRangeRef);
        if (wColRowCellRange == nullptr) {
            return(nullptr);
        }
        if (m_Attribute()!="") {
            return(wColRowCellRange->CellAttribute(m_Row, m_Col,m_Attribute()));
        }
		return(wColRowCellRange->Cell(m_Row, m_Col));
	};

    void tSaveFormulaCell::Formula(const tFormula* sFormula) {
        m_SharedFormula = *sFormula;
    }

    tFormula* tSaveFormulaCell::Formula() {
        return(m_SharedFormula.Formula());
    }

    void tSaveFormulaCell::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
    sWriter->StartObject();
    sWriter->Key(kJsonKeyCell);
    tSheet* wThisSheet=Sheet();
    if (wThisSheet!=sSheet) {
        tStringStream wStream;
        wStream << wThisSheet->Name() << "!" << tTempoPoint(m_Row, m_Col).StrRef();
        sWriter->String(wStream.str().c_str());
    } else {
        sWriter->String(tTempoPoint(m_Row, m_Col).StrRef().c_str());
    }
    
    sWriter->Key(kJsonKeyFormula);
    tSharedFormulaItem* wSharedFormulaItem = tSpreadSheetContainer::Instance()->SharedFormulaPool()->Get(*m_SharedFormula.Formula());
    
    wSharedFormulaItem->Formula()->Json(sWriter);
    m_SharedFormula=*wSharedFormulaItem->Formula();
    
	if (m_Attribute()!="") {
		sWriter->Key(kJsonKeyAttribute);
		sWriter->String(m_Attribute().c_str());
	}

	if (m_Index!=0) {
		sWriter->Key(kJsonKeyIndex);
		sWriter->Int(m_Index);
	}
    sWriter->EndObject();
}

void tSaveFormulaCell::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
    m_ColRowCellRangeRef=sSheet->ColRowCellRange()->SheetAllocator();
	// Read cell reference if present
    if (sValue.HasMember(kJsonKeyCell)) {
        tString wCellRef = sValue[kJsonKeyCell].GetString();
        tTempoPoint* wPointAdress;
        tAllocatorRef wSheetAllocator;
        tie(wSheetAllocator,wPointAdress)=GetCellRef(wCellRef,sSheet);
        if (wSheetAllocator!=0) {
			m_ColRowCellRangeRef=wSheetAllocator;
		} else {
			m_ColRowCellRangeRef=Sheet()->ColRowCellRange()->SheetAllocator();
		}
		m_Row = wPointAdress->Row();
        m_Col = wPointAdress->Col();
    }

        const rapidjson::Value& wFormulaValue=sValue[kJsonKeyFormula];
        tFormula wFormula;
        wFormula.Json(wFormulaValue);
        // Get Shared Formula
        tSharedFormulaItem* wSharedFormulaItem=tSpreadSheetContainer::Instance()->SharedFormulaPool()->Add(wFormula);
        m_SharedFormula.Clear();
        m_SharedFormula=*wSharedFormulaItem->Formula();
        
        // Read attribute if present
        if (sValue.HasMember(kJsonKeyAttribute)) {
            m_Attribute = sValue[kJsonKeyAttribute].GetString();
        }
        
        // Read index if present
        if (sValue.HasMember(kJsonKeyIndex)) {
            m_Index = sValue[kJsonKeyIndex].GetInt();
        }
}

	// Rebase ===================================================================
	tBool tSaveFormulaCell::Rebase(const tRebasePlan& sRebasePlan) {
        // Is Another Sheet
        tRebasePlan wRebasePlan=IsRebaseAnotherSheet(sRebasePlan);
		
		// If no operations match this formula cell's sheet, no rebase needed
		if (wRebasePlan.m_Operations.empty()) {
			return true;
		}
		
		// Rebase row coordinate
		auto wRebasedRow = wRebasePlan.RebaseRow(m_Row);
		if (!wRebasedRow.has_value()) {
			// Cell was deleted
			return false;
		}
		
		// Rebase column coordinate
		auto wRebasedCol = wRebasePlan.RebaseCol(m_Col);
		if (!wRebasedCol.has_value()) {
			// Cell was deleted
			return false;
		}
		
		// Update coordinates
		m_Row = wRebasedRow.value();
		m_Col = wRebasedCol.value();
		
		return true;
	}

#ifdef _DEBUGSK
	tString  tSaveFormulaCell::Debug() {
        tStringStream wStream;
        wStream << Sheet()->Name() << "!" << Base10ToAlpha(m_Col) << m_Row;
        if (m_Attribute()!="") wStream << "." << m_Attribute();
        wStream << " Formula Index " << m_Index;;
        return(wStream.str());
	}
#endif

	//=========================================================================
	tSaveCell::tSaveCell() : tSave(),
		m_Type(tTypeItem::t_Cell),
		m_Row(-1), 
		m_Col(-1), 
        m_Variant(),
        m_Extension(),
        m_Css(0),
        m_FormatStr(),
		m_VectorDependJson(nullptr),
		m_ClassName(),
        m_RefName(),
        m_Attribute("") {}

	tSaveCell::tSaveCell(tAllocatorRef sSheetAllocator, tTypeItem sType,tIndex sRow, tIndex sCol,tFormatRef sCss, tString sAttribute,tExtension sExtension) : tSave(sSheetAllocator),
		m_Type(sType),
		m_Row(sRow),
		m_Col(sCol),
        m_Variant(),
        m_Extension(sExtension),
        m_Css(sCss),
        m_FormatStr(),
		m_VectorDependJson(nullptr),
		m_ClassName(),
        m_RefName(),
		m_Attribute(sAttribute) {}
	/// @brief constructor of copy.
	tSaveCell::tSaveCell(const tSaveCell& sSaveCell) : tSave(sSaveCell) {
		m_Type = sSaveCell.m_Type;
		m_Row = sSaveCell.m_Row;
		m_Col = sSaveCell.m_Col;
        m_Css = sSaveCell.m_Css;
        m_FormatStr = sSaveCell.m_FormatStr;
		m_ClassName = sSaveCell.m_ClassName;
		m_Attribute=sSaveCell.m_Attribute;
		m_Variant = sSaveCell.m_Variant;
        m_Extension = sSaveCell.m_Extension;
		m_VectorDependJson = nullptr; // Don't copy Json dependencies
        /*
		for (auto wItem : sSaveCell.m_VectorDepend) {
			m_VectorDepend.push_back(wItem);
		}
        */
	}

	tSaveCell::~tSaveCell() {
		// the tSaveCell is in tSaveSelect m_VectorSaveCell
		// Delete only objects allocated in JsonDependent (not objects managed by tSaveSelect)
        if (m_VectorDependJson != nullptr) {
            for(auto wSaveCell : *m_VectorDependJson) {
                if (wSaveCell != nullptr) {
                    delete(wSaveCell);
                }
            }
            delete(m_VectorDependJson);
            m_VectorDependJson = nullptr;
        }
        m_VectorDepend.clear();      
        if (m_Css!=0)  {
            ColRowCellRange()->Sheet()->WorkBook()->DeleteCellFormat(m_Css);
        }
    }
    
    void tSaveCell::Set(tItem* sItem) {
		// Set Cell
		if ((sItem->Type()==tTypeItem::t_Cell) || (sItem->Type()==tTypeItem::t_Attribute)) {
            tCell* wCell;
            if (sItem->Type()==tTypeItem::t_Cell) {
                wCell=static_cast<tCell*>(sItem);
                tVariant* wVariant = wCell->PtValue();
                if (wVariant->Type() == tVariantType::t_class) {
                    // Only one class attribute is allowed (not another cell class)
                    tCellClassAttribute* wCellClassAttribute = dynamic_cast<tCellClassAttribute*>(wVariant->Class());
                    if (wCellClassAttribute != nullptr) {
                        // Test if ref name is not already set
                        tString wRefName = wCellClassAttribute->RefName();
                        
                        // Check if another cell class is already set
                        if (m_ClassName() != "") {
                            tStringStream wStream;
                            wStream << "Save Cell " <<  StrRef() << " another cell class is already set " << m_ClassName() << " !";
                            cerr << wStream.str() << endl;
                            throw(tExceptionInternalError(wStream.str()));
                        }
                        m_ClassName = wCellClassAttribute->ClassName();
                        m_RefName = wCellClassAttribute->RefName();
#ifdef debugundoredo
                        cout << "Add " << m_ClassName() << ":" << m_RefName() << endl;
#endif
                    }
                }
            }
           
			if (sItem->Type() == tTypeItem::t_Attribute) {
				tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(sItem);
				m_Attribute = wCellAttribute->Name();
                wCell=wCellAttribute;
    		}
            // Set Row and Col
            m_Row = wCell->RowIndex();
            m_Col = wCell->ColIndex();
            
            // Extention for matrix
            m_Extension = wCell->Extension();
            
			// Wire form: A1 + US separators (collab/server Locale us). Display stays FormulaStr(locale).
			tString wFormula = wCell->FormulaWire();
			if (wFormula != "") {
				m_Variant = "=" + wFormula;
			}
			else {
				m_Variant = wCell->Value();
			}
		} else {
			// Set Range
			tRange* wRange = sItem->Range();
			if (wRange != nullptr) {
				m_Row = wRange->TopIndex();
				m_Col = wRange->LeftIndex();
			}
		}
        
	}

	void tSaveCell::SetDepend(tSaveSelect* sSaveSelect, tItem* sItem) {
		// Set Dependent vector
		tItem::tContainerCell* wVectorDepend = sItem->ContainerCellDepend();
		tItem::tContainerCell::tIteratorClass wIterator;
  
#ifdef debugundoredo
        cout << "SetDependent--->" << sItem->StrRef() << endl;
#endif

		for (wIterator = wVectorDepend->Container()->begin(); wIterator != wVectorDepend->Container()->end(); wIterator++) {
			tCell* wCell = (*wIterator);
			if (wCell != nullptr) {
                tCell* wCellDepend = wCell->Cell();
                if (wCellDepend != nullptr) {
                    tSaveCell* wSaveCell = sSaveSelect->AddCell(wCellDepend);
#ifdef debugundoredo
                    cout << "  push[" << m_VectorDepend.size() << "] ->" <<   wCell->StrRef() << endl;
#endif
                    m_VectorDepend.push_back(wSaveCell);
                }
                else {
#ifdef debugundoredo
                cout << "  push[" << m_VectorDepend.size() << "] ->nullptr" << endl;
#endif
                    m_VectorDepend.push_back(nullptr);
                }
			}
			else {
#ifdef debugundoredo
                    cout << "  push[" << m_VectorDepend.size() << "] ->nullptr" << endl;
#endif
				m_VectorDepend.push_back(nullptr);
			}
		}
	}
        
    void tSaveCell::PassCss(tCell* sCell) {
        // Pass Format ======================================================
        m_Css = sCell->Css();
        sCell->Css(0);
    }
    

	tVectorSaveCell* tSaveCell::VectorDepend() { return(&m_VectorDepend); }

	tSheet* tSaveCell::Sheet() {
        return(ColRowCellRange()->Sheet());
	}

	tIndex tSaveCell::Row() { return(m_Row); }

	tIndex tSaveCell::Col() { return(m_Col); }
        
    tString tSaveCell::Attribute() { return(m_Attribute()); }

    tExtension tSaveCell::Extension() const { return(m_Extension); }
    
    tString tSaveCell::ClassName() { return(m_ClassName()); }
    
    tString tSaveCell::RefName() { return(m_RefName()); }
    
    tFormatRef tSaveCell::Css() { return(m_Css); }
    void tSaveCell::Css(tFormatRef sCss) { m_Css=sCss; };

    tString tSaveCell::FormatStr() { return(m_FormatStr()); }
            
	const tString tSaveCell::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row;
		return(wStream.str());
	}

    void tSaveCell::Value(tVariant& sValue) {
        m_Variant=sValue;
    }

	tVariant* tSaveCell::Value() {
		return(&m_Variant);
	}
    
	tCell* tSaveCell::Cell() {
        tColRowCellRange* wColRowCellRange = ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(nullptr);
        }
		tCell* wCell = wColRowCellRange->Cell(m_Row, m_Col);
        if (wCell!=nullptr) {
            if (m_Type == tTypeItem::t_Attribute) {
                tCellClassAttribute* wCellClass = wCell->ClassAttribute();
                if (wCellClass != nullptr) {
                    return(wCellClass->CellAttribute(m_Attribute()));
                }
            }
        }
		return(wCell);
	}

	tCell* tSaveCell::EnsureCell() {
        tColRowCellRange* wColRowCellRange = ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(nullptr);
        }
        tCell* wCell;
        if (m_Attribute()!="") {
            wCell = wColRowCellRange->EnsureCellAttribute(m_Row, m_Col, m_Attribute());
        } else {
            wCell = wColRowCellRange->EnsureCell(m_Row, m_Col);
        }
       
		return(wCell);
	}

	void tSaveCell::PushFormulaCell(tSaveFormulaCell& sSaveFormulaCell) {
		m_VectorFormulaCellDepend.push_back(sSaveFormulaCell);
	}

	void tSaveCell::RecoverFormulaCell(tBool sIsJsonUndo) {
        (void)sIsJsonUndo;
		tItem* wSourceItem = nullptr;
		if (tSaveRange* wSaveRange = dynamic_cast<tSaveRange*>(this)) {
			tRange* wRange = wSaveRange->Range();
			if (wRange == nullptr) {
				if (tColRowCellRange* wColRowCellRange = ColRowCellRange()) {
					wRange = wColRowCellRange->Range(m_Row, m_Col, wSaveRange->Bottom(), wSaveRange->Right());
					wSaveRange->Range(wRange);
				}
			}
			wSourceItem = wRange;
		}
		if (wSourceItem == nullptr) {
			tCell* wCell = Cell();
			if (wCell == nullptr) {
				return;
			}
			wSourceItem = wCell;
		}
#ifdef debugundoredo
        tString wCellRef = wSourceItem->StrRef(true);
        cout << " RecoverFormulaCell(" << wCellRef;
        cout <<  ")"  << " --> Depend " << endl;
#endif
    
        // Loop  on dependent formula cell ============================================
		for (auto wSaveFormulaCell : m_VectorFormulaCellDepend) {
			tCell* wCellWithFormula = wSaveFormulaCell.Cell();
            if (wCellWithFormula == nullptr) {
                continue;
            }
			const tIndex wIndex = wSaveFormulaCell.Index();
			const tFormula* wSavedFormula = wSaveFormulaCell.Formula();
			tItem* wItemFormula = nullptr;
			if (wIndex < wCellWithFormula->VectorRef()->size()) {
				wItemFormula = (*wCellWithFormula->VectorRef())[wIndex];
			}
			// Delete-sheet Do() nullifies VectorRef slots and CalculateDo may rewrite formula text.
			if (wSavedFormula != nullptr &&
				(wItemFormula == nullptr || wItemFormula != wSourceItem)) {
				wCellWithFormula->Formula(wSavedFormula);
			}
			if (wCellWithFormula->VectorRef()->size() <= wIndex) {
				wCellWithFormula->VectorRef()->resize(wIndex + 1);
			}
			(*wCellWithFormula->VectorRef())[wIndex] = wSourceItem;
			wSourceItem->AddDependent(wCellWithFormula);
#ifdef debugundoredo
            cout << "     " << wCellWithFormula->tItem::StrRef() << " Index=" << wSaveFormulaCell.Index() << " Value=" << wCellWithFormula->Value() << " Formula " << wCellWithFormula->FormulaStr() << endl;
            cout << "Add Dependent " << wCellWithFormula->tItem::StrRef()<< " to " << wSourceItem->StrRef(true) << endl;
#endif
		}
	}


	tVectorSaveFormulaCell* tSaveCell::VectorFormulaCellDepend() { return(&m_VectorFormulaCellDepend); }


    void tSaveCell::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet,tBool sIsWriteDependent) {
        tSheet* wThisSheet=ColRowCellRange()->Sheet();
        sWriter->StartObject();
        sWriter->Key(kJsonKeyCell);
        tTempoPoint wTempoPoint(m_Row,m_Col);
        if (wThisSheet!=sSheet) {
            tStringStream wStream;
            wStream << wThisSheet->Name() << "!" << wTempoPoint.StrRef();
            sWriter->String(wStream.str().c_str());
        } else {
            sWriter->String(wTempoPoint.StrRef().c_str());
        }
        
        if (m_Type != tTypeItem::t_Cell) {
			sWriter->Key(kJsonKeyTypeCell);
			sWriter->Int(static_cast<int>(m_Type));
		}
        // Value
        m_Variant.Json(sWriter);

        // Extension for matrix (same bits as tItem / SkStackElem)
        if (m_Extension.Value(t_MatOrigin) || m_Extension.Value(t_MatExtend)) {
            sWriter->Key(kJsonKeyExtension);
            sWriter->Int(m_Extension.BitSet());
        }
        // Class
        if(m_ClassName()!="") {
#ifdef debugundoredo
            cout << "Write  " << m_ClassName()  << ":" << m_RefName() << endl;
#endif
            sWriter->Key(kJsonKeyClassName);
            sWriter->String(m_ClassName().c_str());
           
            sWriter->Key(kJsonKeyRefName);
            sWriter->String(m_RefName().c_str());
        }

		if (m_Attribute()!="") {
			sWriter->Key(kJsonKeyAttribute);
			sWriter->String(m_Attribute().c_str());
		}
    
        // Add format/CSS field that is read in Json(const rapidjson::Value&)
        // First pass
        if (m_Css != 0) {
            tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
            if (wFormatApi!=nullptr) {
                // Return Index +1 
                tSize wJsonCss=wFormatApi->WriteJsonAddFormat(m_Css)+1;
                sWriter->Key(kJsonKeyFormat);
                sWriter->Int(tInt(wJsonCss));
            }
        }
        
        if ((sIsWriteDependent) && (m_VectorDepend.size()!=0)) {
            sWriter->Key(kJsonKeyDependent);
            JsonDependent(sWriter,sSheet);
        }
		if (!m_VectorFormulaCellDepend.empty()) {
			sWriter->Key(kJsonKeyFormulaCell);
			JsonFormulaCell(sWriter,sSheet);
		}
        sWriter->EndObject();
    }
    
    void tSaveCell::JsonDependent(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        sWriter->StartArray();
        for(auto wDepend : m_VectorDepend) {
            if (wDepend != nullptr) {
                wDepend->Json(sWriter,sSheet, false); // One level
            }
        }
        sWriter->EndArray();
	}

	void tSaveCell::JsonFormulaCell(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
		sWriter->StartArray();
		for (auto wSaveFormulaCell : m_VectorFormulaCellDepend) {
			wSaveFormulaCell.Json(sWriter,sSheet);
		}
		sWriter->EndArray();
	}	
        
    
    void tSaveCell::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        // Read cell reference if present
        if (sValue.HasMember(kJsonKeyCell)) {
            tString wCellRef = sValue[kJsonKeyCell].GetString();
            tTempoPoint* wPointAdress;
            tAllocatorRef wSheetAllocator;
            tie(wSheetAllocator,wPointAdress)=GetCellRef(wCellRef,sSheet);
    
            if (wSheetAllocator!=0) {
                m_ColRowCellRangeRef=wSheetAllocator;
            } else {
                m_ColRowCellRangeRef=sSheet->ColRowCellRange()->SheetAllocator();
            }
            
            if (m_ColRowCellRangeRef==0) {
                cout << "tSaveCell::Json Error " << m_ColRowCellRangeRef << endl;
            }
            m_Row = wPointAdress->Row();
            m_Col = wPointAdress->Col();
        }
		if (sValue.HasMember(kJsonKeyTypeCell)) {
			m_Type=static_cast<tTypeItem>(sValue[kJsonKeyTypeCell].GetInt());
		}
        
        // Read classname & refname  if present
        if (sValue.HasMember(kJsonKeyClassName)) {
            m_ClassName = sValue[kJsonKeyClassName].GetString();
        }
        if (sValue.HasMember(kJsonKeyRefName)) {
            m_RefName = sValue[kJsonKeyRefName].GetString();
#ifdef debugundoredo
            cout << "Read  " << m_ClassName()  << ":" << m_RefName() << endl;
#endif
        }
        
        // Read attribute if present
        if (sValue.HasMember(kJsonKeyAttribute)) {
            m_Attribute = sValue[kJsonKeyAttribute].GetString();
        }
        
        // Value Formula is in value string with =
        m_Variant.Json(sValue);
      
        // Read extension if present
        if (sValue.HasMember(kJsonKeyExtension)) {
            m_Extension.BitSet(sValue[kJsonKeyExtension].GetInt());
        }
    
        // Read format/CSS if present
        if (sValue.HasMember(kJsonKeyFormat)) {
            tInt wFormatInt = sValue[kJsonKeyFormat].GetInt();
            wFormatInt--;
            tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
            if (wFormatApi!=nullptr && wFormatInt >= 0) {
                m_FormatStr = wFormatApi->ReadJsonGetFormat(static_cast<tSize>(wFormatInt));
            }
        }
        
        
        if (sValue.HasMember(kJsonKeyDependent)) {
            const rapidjson::Value& wDepend = sValue[kJsonKeyDependent];
            JsonDependent(wDepend,sSheet);
        }
		
		if (sValue.HasMember(kJsonKeyFormulaCell)) {
			const rapidjson::Value& wFormula = sValue[kJsonKeyFormulaCell];
			JsonFormulaCell(wFormula,sSheet);
		}
    }

    void tSaveCell::JsonDependent(const rapidjson::Value& sDepends,tSheet* sSheet) {
        if (!sDepends.IsArray()) {
            return; // Safety check: ensure it's an array
        }
        // Allocate vector only when needed (lazy allocation)
        if (m_VectorDependJson == nullptr) {
            m_VectorDependJson = new tVectorSaveCell();
        }
        for (const auto& wDepend : sDepends.GetArray()) {
            if (wDepend.IsObject()) {
                // Determine if this is a tSaveCell or tSaveRange based on content
                tSaveCell* wSaveCell = nullptr;
                
                // Check if it has range-specific fields (like "r" for range reference)
                if (wDepend.HasMember(kJsonKeyRange)) {
                    // This is likely a tSaveRange
                    wSaveCell = new tSaveRange();
                } else {
                    // This is likely a tSaveCell
                    wSaveCell = new tSaveCell();
                }
                
                if (wSaveCell != nullptr) {
                    wSaveCell->Json(wDepend,sSheet);
                    m_VectorDepend.push_back(wSaveCell);
                    // Also add to Json vector for proper cleanup (these objects are not managed by tSaveSelect)
                    m_VectorDependJson->push_back(wSaveCell);
                }
            }
        }
    }

	void tSaveCell::JsonFormulaCell(const rapidjson::Value& sValue,tSheet *sSheet) {
        if (sValue.IsArray()) {	
            for (const auto& wSaveFormulaCellValue : sValue.GetArray()) {
                tSaveFormulaCell wSaveFormulaCell;
                wSaveFormulaCell.Json(wSaveFormulaCellValue,sSheet);
                m_VectorFormulaCellDepend.push_back(wSaveFormulaCell);
            }
        }
	}

	tBool tSaveCell::operator < (tSaveCell & sSaveCell) {
		if (m_ColRowCellRangeRef == sSaveCell.m_ColRowCellRangeRef) {
			if (m_Type == sSaveCell.m_Type) {
				if (m_Row == sSaveCell.m_Row) {
					if (m_Col == sSaveCell.m_Col) {
						return(m_Attribute() < sSaveCell.m_Attribute());
					} else return(m_Col < sSaveCell.m_Col);
				} else return(m_Row < sSaveCell.m_Row);
			} else return(m_Type < sSaveCell.m_Type);
		} else return(m_ColRowCellRangeRef < sSaveCell.m_ColRowCellRangeRef);
	}

	tBool tSaveCell::operator == (tSaveCell & sSaveCell) {
		tBool wResult = (m_ColRowCellRangeRef == sSaveCell.m_ColRowCellRangeRef) &&
			(m_Type==sSaveCell.m_Type) &&
			(m_Row == sSaveCell.m_Row) &&
			(m_Col == sSaveCell.m_Col) &&
			(m_Attribute == sSaveCell.m_Attribute);
			return(wResult);
	}

#ifdef _DEBUGSK
    tString tSaveCell::Debug() {
        tStringStream wStream;
        wStream << "tSaveCell " << Sheet()->Name() << "!" << StrRef();
        wStream << "->";
        if (!m_VectorDepend.empty()) {
            wStream << "depend(";
            for (auto wItem : m_VectorDepend) {
                tSaveRange* wSaveRange = dynamic_cast<tSaveRange*>(wItem);
                if (wSaveRange != nullptr) {
                    wStream << ";" << wSaveRange->Sheet()->Name() << "!" << Base10ToAlpha(wSaveRange->Col()) << wSaveRange->Row() << ":" << Base10ToAlpha(wSaveRange->Right()) << wSaveRange->Bottom();
                }
                else {
                    wStream << ";" << Sheet()->Name() << "!" << Base10ToAlpha(wItem->Col()) << wItem->Row();
                }
            }
            wStream << ")";
        }
    
        
		if (!m_VectorFormulaCellDepend.empty()) {
            wStream << " Depend(";
			for (auto wSaveFormulaCell : m_VectorFormulaCellDepend) {
				wStream << wSaveFormulaCell.Debug();
			}
            wStream << ")";
		}
        
        wStream << " Value:" << m_Variant;
        if (m_Css!=0) wStream << " Css:" << m_Css;
        tFormatApi* wFormatApi=Sheet()->WorkBook()->FormatApi();
        if (wFormatApi!=nullptr) {
            cout << " Format "  << wFormatApi->CellFormat(m_Css);
        }
        tCell* wCell=Cell();
        if (wCell==nullptr)
            wStream << " nullptr";
        
        return(wStream.str());
	}
#endif
        
#ifdef checkfo
    void tSaveCell::IncCheckfo(tFormatApi* sFormatApi) {
        if (m_Css!=0)  {
            sFormatApi->IncCheck(m_Css);
        }
    }
#endif

    // Rebase ===================================================================
    tBool tSaveCell::Rebase(const tRebasePlan& sRebasePlan) {
        // Is Another Sheet
        tRebasePlan wRebasePlan=IsRebaseAnotherSheet(sRebasePlan);
        
        // If no operations match this cell's sheet, no rebase needed
        if (!wRebasePlan.m_Operations.empty()) {
            // Debug: log original coordinates and plan
            // tIndex wOriginalCol = m_Col;
            // tIndex wOriginalRow = m_Row;
            
            // Rebase row coordinate
            auto wRebasedRow = wRebasePlan.RebaseRow(m_Row);
            if (!wRebasedRow.has_value()) {
                return false;
            }
            
            // Rebase column coordinate
            auto wRebasedCol = wRebasePlan.RebaseCol(m_Col);
            if (!wRebasedCol.has_value()) {
                return false;
            }
            
            // Debug: log rebase result
            // if (wOriginalCol != wRebasedCol.value() || wOriginalRow != wRebasedRow.value()) {
            //     cout << "RebaseCell: " << Base10ToAlpha(wOriginalCol) << wOriginalRow 
            //          << " -> " << Base10ToAlpha(wRebasedCol.value()) << wRebasedRow.value() 
            //          << " | Plan operations: " << wRebasePlan.m_Operations.size() << endl;
            // }
            
            // Update coordinates
            m_Row = wRebasedRow.value();
            m_Col = wRebasedCol.value();
        }
        // Rebase Formula =====================================================
        // See Later olnly formula on other sheet
        
        if (m_Variant.Type()==tVariantType::t_string) {
            tString wFormula=m_Variant.String();
            if (wFormula.length()>0) {
                if (wFormula[0]=='=') {
                    tUndoRedoRebaseFormula wUndoRedoRebaseFormula(ColRowCellRangeRef(),m_Row,m_Col,wFormula);
                    if (wUndoRedoRebaseFormula.Rebase(wRebasePlan)) {
                        m_Variant = wUndoRedoRebaseFormula.Formula();
                    }
                    // Keep original formula when rebase fails (e.g. DATARANGE through delete undo plan);
                    // CalculateDo after UndoDeleteColRow will recompile with the restored grid.
                }
            }
        }
        
        // Rebase formula cells
        for (auto& wSaveFormulaCell : m_VectorFormulaCellDepend) {
            if (!wSaveFormulaCell.Rebase(wRebasePlan)) {
                // Formula cell was deleted
                return false;
            }
        }
        
        // Rebase dependent cells
        for (auto wSaveCell : m_VectorDepend) {
            if (!wSaveCell->Rebase(wRebasePlan)) {
                // Dependent cell was deleted
                return false;
            }
        }
        
        return true;
    }


	
	//=========================================================================
	tSaveRange::tSaveRange() : tSaveCell(),
		m_Range(nullptr),
		m_RangeAllocatorRef(0),
		m_Deleted(false),
        m_ExternalCovered(false),
        m_Bottom(-1),
        m_Right(-1),
        m_Name(""),
        m_RangeData(nullptr),
        m_Extension(0){}

	tSaveRange::tSaveRange(const tSaveRange & sSaveRange) : tSaveCell(sSaveRange) {
		m_Deleted = sSaveRange.m_Deleted;
		m_ExternalCovered = sSaveRange.m_ExternalCovered;
		m_Range = sSaveRange.m_Range;
		m_RangeAllocatorRef = sSaveRange.m_RangeAllocatorRef;
		m_Bottom = sSaveRange.m_Bottom;
		m_Right = sSaveRange.m_Right;
		m_Name = sSaveRange.m_Name;
        if (sSaveRange.m_RangeData!=nullptr) {
            m_RangeData=new tRangeData(*sSaveRange.m_RangeData);
        } else {
            m_RangeData=nullptr;
        }
        m_Extension=sSaveRange.m_Extension;
	}

	tSaveRange::tSaveRange(tIndex sIndiceSheet, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tString sName) : tSaveCell(sIndiceSheet,tTypeItem::t_Range, sTop, sLeft,0,"",0),
        m_Deleted(false),
		m_ExternalCovered(false),
		m_Bottom(sBottom),
		m_Right(sRight),
		m_Name(sName),
        m_RangeData(nullptr) {
		tColRowCellRange* wColRowCellRange = tStaticColRowCellRange::Instance()->ColRowCellRange(m_ColRowCellRangeRef);
		m_Range = wColRowCellRange->Range(m_Row, m_Col, m_Bottom, m_Right);
		if (m_Range!=nullptr) m_RangeAllocatorRef = m_Range->AllocatorRef();
	}
	tSaveRange::tSaveRange(tRange * sRange) : tSaveCell(), 
		m_Range(sRange),
		m_RangeAllocatorRef(0),
		m_Deleted(false), 
		m_ExternalCovered(false),
		m_RangeData(nullptr) {
		m_ColRowCellRangeRef = sRange->ColRowCellRangeRef();
		m_RangeAllocatorRef = sRange->AllocatorRef();
		m_Row = sRange->TopIndex();
		m_Col = sRange->LeftIndex();
		m_Bottom = sRange->BottomIndex();
		m_Right = sRange->RightIndex();

		m_Name = sRange->Name();
        if (sRange->IsData()) {
            tWorkBook* wWorkBook=sRange->WorkBook();
            tRangeData* wRangeData=wWorkBook->RangeData(m_Name());
            if (wRangeData!=nullptr) {
                m_RangeData=new tRangeData(*wRangeData);
            }
        }
        SetExtension(sRange);
	}

	tSaveRange::~tSaveRange() {
        // Clean up RangeData if allocated
        if (m_RangeData != nullptr) {
            delete(m_RangeData);
            m_RangeData = nullptr;
        }
        // Base class destructor (tSaveCell) will be called automatically
    }

    tAllocatorRef tSaveRange::AllocatorRef() {
            return(m_RangeAllocatorRef);
    }

    void tSaveRange::SetExtension(tRange* sRange) {
        m_Extension.ClearAll();
        if (sRange->IsNamed()) SetNamed();
        if (sRange->IsMerged()) SetMerged();
        if (sRange->IsData()) SetData();
        if (sRange->IsCFHR()) SetCFHR();
        if (sRange->IsCFDB()) SetCFDB();
        if (sRange->IsCFCS()) SetCFCS();
        if (sRange->IsCFIS()) SetCFIS();
        if (sRange->IsCFCF()) SetCFCF();
    }
        
	void tSaveRange::SetRefAndDepend(tSaveSelect * sSaveSelect, tItem * sRange) {
		tSaveCell::SetDepend(sSaveSelect, sRange);
		tRange* wRange = sRange->Range();
        if (wRange != nullptr) {
            m_Bottom = wRange->BottomIndex();
            m_Right = wRange->RightIndex();
            m_Name = wRange->Name();
            m_RangeAllocatorRef = wRange->AllocatorRef();
            SetExtension(wRange);
        }
	}

	void tSaveRange::Deleted(tBool sDeleted) { m_Deleted = sDeleted; }
	tBool tSaveRange::Deleted() { return(m_Deleted); }

	void tSaveRange::ExternalCovered(tBool sExternalCovered) { m_ExternalCovered = sExternalCovered; }
	tBool tSaveRange::ExternalCovered() { return(m_ExternalCovered); }

	// Rebase ===================================================================
	tBool tSaveRange::Rebase(const tRebasePlan& sRebasePlan) {
        // Is Another Sheet
        tRebasePlan wRebasePlan=IsRebaseAnotherSheet(sRebasePlan);

		
		// If no operations match this range's sheet, no rebase needed
		if (wRebasePlan.m_Operations.empty()) {
			return true;
		}
		
		// First rebase the cell coordinates (inherited from tSaveCell)
		// Note: tSaveCell::Rebase will also filter, but we use the filtered plan here for consistency
		if (!tSaveCell::Rebase(wRebasePlan)) {
			// Cell coordinates were deleted
			return false;
		}
		
		// Rebase bottom row coordinate using filtered plan
		auto wRebasedBottom = wRebasePlan.RebaseRow(m_Bottom);
		if (!wRebasedBottom.has_value()) {
			// Bottom row was deleted
			return false;
		}
		
		// Rebase right column coordinate using filtered plan
		auto wRebasedRight = wRebasePlan.RebaseCol(m_Right);
		if (!wRebasedRight.has_value()) {
			// Right column was deleted
			return false;
		}
		
		// Update range coordinates
		m_Bottom = wRebasedBottom.value();
		m_Right = wRebasedRight.value();
		
		// Validate that the range is still valid (top <= bottom, left <= right)
		if (m_Row > m_Bottom || m_Col > m_Right) {
			// Range became invalid
			return false;
		}
		
		return true;
	}
        
    tBool tSaveRange::IsMerged() { return(m_Extension.Value(t_Merged)); }
    void tSaveRange::SetMerged() { if (!m_Extension.Value(t_Merged)) m_Extension.Set(t_Merged); }
   
    tBool tSaveRange::IsNamed() { return(m_Extension.Value(t_Named)); }
    void tSaveRange::SetNamed() { if (!m_Extension.Value(t_Named)) m_Extension.Set(t_Named); }

    tBool tSaveRange::IsData() { return(m_Extension.Value(t_Data)); }
    void tSaveRange::SetData() { if (!m_Extension.Value(t_Data)) m_Extension.Set(t_Data); }

    tBool tSaveRange::IsConditionalFormat() { return(m_Extension.Value(t_CFHR) || m_Extension.Value(t_CFDB) || m_Extension.Value(t_CFCS) || m_Extension.Value(t_CFIS) || m_Extension.Value(t_CFCF)); }

    tBool tSaveRange::IsCFHR() { return(m_Extension.Value(t_CFHR)); }
    void tSaveRange::SetCFHR() { if (!IsCFHR()) m_Extension.Set(t_CFHR); }
    tBool tSaveRange::IsCFDB() { return(m_Extension.Value(t_CFDB)); }
    void tSaveRange::SetCFDB() { if (!IsCFDB()) m_Extension.Set(t_CFDB); }
    tBool tSaveRange::IsCFCS() { return(m_Extension.Value(t_CFCS)); }
    void tSaveRange::SetCFCS() { if (!IsCFCS()) m_Extension.Set(t_CFCS); }
    tBool tSaveRange::IsCFIS() { return(m_Extension.Value(t_CFIS)); }
    void tSaveRange::SetCFIS() { if (!IsCFIS()) m_Extension.Set(t_CFIS); }
    tBool tSaveRange::IsCFCF() { return(m_Extension.Value(t_CFCF)); }
    void tSaveRange::SetCFCF() { if (!IsCFCF()) m_Extension.Set(t_CFCF); }

    void tSaveRange::ClearConditionalFormatExtension() {
        if (IsCFHR()) m_Extension.Clear(t_CFHR);
        if (IsCFDB()) m_Extension.Clear(t_CFDB);
        if (IsCFCS()) m_Extension.Clear(t_CFCS);
        if (IsCFIS()) m_Extension.Clear(t_CFIS);
        if (IsCFCF()) m_Extension.Clear(t_CFCF);
    }

	tIndex tSaveRange::Bottom() { return(m_Bottom); }
	tIndex tSaveRange::Right() { return(m_Right); }

	tTempoRect tSaveRange::Rect() {
		return(tTempoRect(m_Row, m_Col, m_Bottom, m_Right));
	}

	const tString tSaveRange::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row << ":";
		wStream << Base10ToAlpha(m_Right) << m_Bottom;
		return(wStream.str());
	}

	const tString tSaveRange::StrRefAfter() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_CoordAfter.Left()) << m_CoordAfter.Row() << ":";
		wStream << Base10ToAlpha(m_CoordAfter.Right()) << m_CoordAfter.Bottom();
		return(wStream.str());
	}


	void  tSaveRange::CoordAfter(tTempoRect & sCoordAfter) { m_CoordAfter = sCoordAfter; }
	tTempoRect tSaveRange::CoordAfter() { return(m_CoordAfter); }

    tRange* tSaveRange::Range() {
        return(m_Range);
    }
    void tSaveRange::Range(tRange* sRange) {
        m_Range=sRange;
    }
    
    tString  tSaveRange::Name() { return(m_Name.Str()); }
    
    tRangeData* tSaveRange::RangeData() { return(m_RangeData); }
    void tSaveRange::RangeData(tRangeData sRangeData) {m_RangeData=new tRangeData(sRangeData); }

        
    tBool tSaveRange::operator < (tSaveRange & sSaveRange) {
        if (m_Row == sSaveRange.m_Row) {
            if (m_Col == sSaveRange.m_Col) {
                if (m_Bottom == sSaveRange.m_Bottom) {
                    if (m_Right == sSaveRange.m_Right) {
                        return(m_Deleted < sSaveRange.m_Deleted);
                    } else return(m_Right < sSaveRange.m_Right);
                }
                else { return(m_Bottom < sSaveRange.m_Bottom); }
            }
            else { return(m_Col < sSaveRange.m_Col); }
        }
        else { return(m_Row < sSaveRange.m_Row); }
    }
    
    void tSaveRange::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet,tBool sIsWriteDependent) {
        tSheet* wThisSheet=ColRowCellRange()->Sheet();
        sWriter->StartObject();
        sWriter->Key(kJsonKeyRange);
        tTempoRect wTempoRect(m_Row,m_Col,m_Bottom,m_Right);
        if (wThisSheet!=sSheet) {
            tStringStream wStream;
            wStream << wThisSheet->Name() << "!" << wTempoRect.StrRef();
            sWriter->String(wStream.str().c_str());
        } else {
            sWriter->String(wTempoRect.StrRef().c_str());
        }
        
        
        if (m_Attribute()!="") {
            sWriter->Key(kJsonKeyAttribute);
            sWriter->String(m_Attribute().c_str());
        }
        
        if (IsNamed()) {
            sWriter->Key(kJsonKeyName);
            sWriter->String(m_Name().c_str());
        }
        if (IsMerged()) {
            sWriter->Key(kJsonKeyMerged);
            sWriter->Bool(true);
        };
        if (IsData()) {
            sWriter->Key(kJsonKeyData);
            m_RangeData->Json(sWriter);
        };
        if (IsCFHR()) {
            sWriter->Key(kJsonKeyConditionalFormatHR);
            sWriter->Bool(IsCFHR());
        }
        if (IsCFDB()) {
            sWriter->Key(kJsonKeyConditionalFormatDB);
            sWriter->Bool(IsCFDB());
        }
        if (IsCFCS()) {
            sWriter->Key(kJsonKeyConditionalFormatCS);
            sWriter->Bool(IsCFCS());
        }
        if (IsCFIS()) {
            sWriter->Key(kJsonKeyConditionalFormatIS);
            sWriter->Bool(IsCFIS());
        }
        if (IsCFCF()) {
            sWriter->Key(kJsonKeyConditionalFormatCF);
            sWriter->Bool(IsCFCF());
        }
        if (m_Deleted) {
            sWriter->Key(kJsonKeyDeleted);
            sWriter->Bool(true);
        } else if (m_CoordAfter.Bottom() >= 0 && m_CoordAfter.Right() >= 0) {
            sWriter->Key(kJsonKeyCoordAfter);
            sWriter->String(StrRefAfter().c_str());
        }
        
        if ((sIsWriteDependent) && (m_VectorDepend.size()!=0)) {
            sWriter->Key(kJsonKeyDependent);
            JsonDependent(sWriter,sSheet);
        }
        if (!m_VectorFormulaCellDepend.empty()) {
            sWriter->Key(kJsonKeyFormula);
            JsonFormulaCell(sWriter,sSheet);
        }
        
        sWriter->EndObject();
    }
    
    void tSaveRange::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        tString wRangeRef = sValue[kJsonKeyRange].GetString();
        tTempoRect* wRectAdress = new tTempoRect();
        tIndex wIndiceSheet;
        tie(wIndiceSheet,wRectAdress)=GetRangeRef(wRangeRef,sSheet);
        // 0 = same sheet as sSheet (see GetRangeRef); do not bind allocator slot 0.
        if (wIndiceSheet != 0) {
            m_ColRowCellRangeRef=wIndiceSheet;
        } else {
            m_ColRowCellRangeRef=sSheet->ColRowCellRange()->SheetAllocator();
        }
        m_Row = wRectAdress->Row();
        m_Col = wRectAdress->Col();
        m_Bottom = wRectAdress->Bottom();
        m_Right = wRectAdress->Right();
        
        if (sValue.HasMember(kJsonKeyAttribute)) {
            m_Attribute=sValue[kJsonKeyAttribute].GetString();
        }
        if (sValue.HasMember(kJsonKeyName)) {
            m_Name=sValue[kJsonKeyName].GetString();
            SetNamed();
        }
        
        if (sValue.HasMember(kJsonKeyMerged)) {
            sValue[kJsonKeyMerged].GetBool();
            SetMerged();
        }
        if (sValue.HasMember(kJsonKeyData)) {
            if (m_RangeData != nullptr) {
                delete(m_RangeData);
            }
            m_RangeData = new tRangeData();
            SetData();
            m_RangeData->Json(sValue[kJsonKeyData]);
        }
        if (sValue.HasMember(kJsonKeyConditionalFormatHR)) {
            SetCFHR();
        }
        if (sValue.HasMember(kJsonKeyConditionalFormatDB)) {
            SetCFDB();
        }
        if (sValue.HasMember(kJsonKeyConditionalFormatCS)) {
            SetCFCS();
        }
        if (sValue.HasMember(kJsonKeyConditionalFormatIS)) {
            SetCFIS();
        }
        if (sValue.HasMember(kJsonKeyConditionalFormatCF)) {
            SetCFCF();
        }
        if (sValue.HasMember(kJsonKeyDeleted) && sValue[kJsonKeyDeleted].GetBool()) {
            m_Deleted = true;
        }
        if (sValue.HasMember(kJsonKeyCoordAfter)) {
            m_CoordAfter = tRect(sValue[kJsonKeyCoordAfter].GetString());
        }
        if (sValue.HasMember(kJsonKeyDependent)) {
            const rapidjson::Value& wDepend = sValue[kJsonKeyDependent];
            JsonDependent(wDepend,sSheet);
        }
        if (sValue.HasMember(kJsonKeyFormula)) {
            const rapidjson::Value& wFormula = sValue[kJsonKeyFormula];
            JsonFormulaCell(wFormula,sSheet);
        } else if (sValue.HasMember(kJsonKeyFormulaCell)) {
            const rapidjson::Value& wFormula = sValue[kJsonKeyFormulaCell];
            JsonFormulaCell(wFormula,sSheet);
        }
    }
    tBool tSaveRange::operator == (tSaveRange & sSaveRange) {
        return(
               (m_Row == sSaveRange.m_Row) &&
               (m_Col == sSaveRange.m_Col) &&
               (m_Bottom == sSaveRange.m_Bottom) &&
               (m_Right == sSaveRange.m_Right)
               );
    }
#ifdef _DEBUGSK
    tString tSaveRange::Debug() {
        tStringStream wStream;
        wStream << "tSaveRange ";
        wStream << "Range " << StrRef() << " ";
        if (!m_Deleted) wStream << " Alloc(" << AllocatorRef() <<") -->" << m_Range << " " ;
        if (IsNamed())  wStream << "Name=" << m_Name() << " ";
        if (IsMerged()) wStream << "Merged ";
        if (IsData()) wStream << "Data ";
        if (IsConditionalFormat())  wStream << "ConditionalFormat ";
     
        if (m_Deleted) {
            wStream << "Deleted ";
        } else {
            wStream << "after " << StrRefAfter() << " ";
        }
        if (m_ExternalCovered) {
            wStream << "ExternalCovered ";
        }
        
        if (!m_VectorFormulaCellDepend.empty()) {
            wStream << "Formula Cell  " << " ";
            for (auto wSaveFormulaCell : m_VectorFormulaCellDepend) wStream << wSaveFormulaCell.Debug();
        }
        return(wStream.str());
    }
#endif

    //=========================================================================
    tSaveConditionalFormat::tSaveConditionalFormat() : tClass(),
        m_Ref(""),
        m_Key(""),
        m_Type(tConditionalFormatType::t_None),
        m_IconType(tIconType::t_None),
        m_ConditionalFormat(nullptr),
        m_Param1(""), 
        m_Param2(""), 
        m_Param3(""), 
        m_Param4(""), 
        m_Param5(""), 
        m_Param6(""), 
        m_Param7(""), 
        m_Param8(""), 
        m_Param9(""), 
        m_Param10("") {}
    
    
    tSaveConditionalFormat::tSaveConditionalFormat(tConditionalFormat* sConditionalFormat) : tClass(),
        m_Ref(sConditionalFormat->Ref()),
        m_Key(sConditionalFormat->Key()),
        m_Type(sConditionalFormat->Type()),
        m_IconType(sConditionalFormat->IconType()),
        m_ConditionalFormat(sConditionalFormat),
        m_Param1(sConditionalFormat->Param1()),
        m_Param2(sConditionalFormat->Param2()),
        m_Param3(sConditionalFormat->Param3()),
        m_Param4(sConditionalFormat->Param4()),
        m_Param5(sConditionalFormat->Param5()),
        m_Param6(sConditionalFormat->Param6()),
        m_Param7(sConditionalFormat->Param7()),
        m_Param8(sConditionalFormat->Param8()),
        m_Param9(sConditionalFormat->Param9()),
        m_Param10(sConditionalFormat->Param10()) {}
    
    tSaveConditionalFormat::~tSaveConditionalFormat() {}
    
    tConditionalFormat* tSaveConditionalFormat::ConditionalFormat() { return(m_ConditionalFormat); }

    tIconType tSaveConditionalFormat::IconType() { return(m_IconType); }

    tString tSaveConditionalFormat::FormatParam1() { return(m_Param1()); }
    tString tSaveConditionalFormat::FormatParam2() { return(m_Param2()); }
    tString tSaveConditionalFormat::FormatParam3() { return(m_Param3()); }
    tString tSaveConditionalFormat::FormatParam4() { return(m_Param4()); }
    tString tSaveConditionalFormat::FormatParam5() { return(m_Param5()); }
    tString tSaveConditionalFormat::FormatParam6() { return(m_Param6()); }
    tString tSaveConditionalFormat::FormatParam7() { return(m_Param7()); }
    tString tSaveConditionalFormat::FormatParam8() { return(m_Param8()); }
    tString tSaveConditionalFormat::FormatParam9() { return(m_Param9()); }
    tString tSaveConditionalFormat::FormatParam10() { return(m_Param10()); }

    tConditionalFormatType tSaveConditionalFormat::Type() { return (m_Type);}
    tString tSaveConditionalFormat::Ref() { return(m_Ref); }
    tString tSaveConditionalFormat::Key() { return(m_Key); }

    static tRange* RangeFromSelectItem(tSheet* sSheet, tTempoPoint* sItem, tBool sEnsureIfMissing) {
        if (sSheet == nullptr || sItem == nullptr) {
            return(nullptr);
        }
        tTempoRect* wRect = dynamic_cast<tTempoRect*>(sItem);
        if (wRect != nullptr) {
            tRange* wRange = sSheet->Range(wRect->Top(), wRect->Left(), wRect->Bottom(), wRect->Right());
            if (wRange == nullptr && sEnsureIfMissing) {
                wRange = sSheet->EnsureRange(wRect->Top(), wRect->Left(), wRect->Bottom(), wRect->Right());
            }
            return(wRange);
        }
        tTempoPoint* wPoint = sItem;
        if (wPoint != nullptr) {
            tRange* wRange = sSheet->Range(wPoint->Row(), wPoint->Col(), wPoint->Row(), wPoint->Col());
            if (wRange == nullptr && sEnsureIfMissing) {
                wRange = sSheet->EnsureRange(wPoint->Row(), wPoint->Col(), wPoint->Row(), wPoint->Col());
            }
            return(wRange);
        }
        return(nullptr);
    }

    static void StripOrphanConditionalFormatFlags(tConditionalFormatType sType, tString sRef,
                                                  tSheet* sSheet, tConditionalFormatContainer* sContainer) {
        tSelect wSelect;
        if (!wSelect.Parse(sRef)) {
            return;
        }
        for (auto wItem : *wSelect.VectorSelect()) {
            tRange* wRange = RangeFromSelectItem(sSheet, wItem, false);
            if (wRange == nullptr || !wRange->IsConditionalFormat()) {
                continue;
            }
            if (sContainer == nullptr || sContainer->ConditionalFormatByRange(wRange) == nullptr) {
                wRange->RemoveConditionalFormat();
            }
        }
        (void)sType;
    }
    
    tConditionalFormat* tSaveConditionalFormat::Undo(tColRowCellRange* sColRowCellRange) {
        tSheet* wSheet = sColRowCellRange->Sheet();
        tConditionalFormatContainer* wContainer = sColRowCellRange->ConditionalFormatContainer();

        StripOrphanConditionalFormatFlags(m_Type, m_Ref, wSheet, wContainer);

        tConditionalFormat* wConditionalFormat = sColRowCellRange->ConditionalFormat(m_Type, m_Ref);
        if (wConditionalFormat == nullptr) {
            wConditionalFormat = sColRowCellRange->AddConditionalFormat(m_Type, m_Ref);
        } else if (wContainer != nullptr) {
            // Partial RemoveByRect can leave the rule but drop deleted ranges from m_MapRange.
            if (!wContainer->AddRangeInMapRange(m_Type, m_Ref, wConditionalFormat)) {
                StripOrphanConditionalFormatFlags(m_Type, m_Ref, wSheet, wContainer);
            }
        }
        if (wConditionalFormat == nullptr) {
            return(nullptr);
        }
   
        switch(m_Type) {
            case tConditionalFormatType::t_CustomFormulas:
            case tConditionalFormatType::t_HighlightCellsRules: {
                // Compil 
                wConditionalFormat->Compil(m_Param1(), m_Param2(), m_Param3());
                break;
            }
            case tConditionalFormatType::t_DataBars:
            case tConditionalFormatType::t_IconSets:
            case tConditionalFormatType::t_ColorScales: {
                break;
            default:break;
            }
        }
        wConditionalFormat->SetParam1(m_Param1());
        wConditionalFormat->SetParam2(m_Param2());
        wConditionalFormat->SetParam3(m_Param3());
        wConditionalFormat->SetParam4(m_Param4());
        wConditionalFormat->SetParam5(m_Param5());
        wConditionalFormat->SetParam6(m_Param6());
        wConditionalFormat->SetParam7(m_Param7());
        wConditionalFormat->SetParam8(m_Param8());
        wConditionalFormat->SetParam9(m_Param9());
        wConditionalFormat->SetParam10(m_Param10());
        return(wConditionalFormat);
    }

    void tSaveConditionalFormat::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key(kJsonKeyRef);
        sWriter->String(m_Ref.c_str());
        sWriter->Key(kJsonKey);
        sWriter->String(m_Key.c_str());
        sWriter->Key(kJsonKeyTypeCell);
        sWriter->Int(static_cast<int>(m_Type));
        sWriter->Key("it");
        sWriter->Int(static_cast<int>(m_IconType));
        auto writeParam = [&](const char* sKey, const tSharedString& sParam) {
            if (!sParam().empty()) {
                sWriter->Key(sKey);
                sWriter->String(sParam().c_str());
            }
        };
        writeParam("p1", m_Param1);
        writeParam("p2", m_Param2);
        writeParam("p3", m_Param3);
        writeParam("p4", m_Param4);
        writeParam("p5", m_Param5);
        writeParam("p6", m_Param6);
        writeParam("p7", m_Param7);
        writeParam("p8", m_Param8);
        writeParam("p9", m_Param9);
        writeParam("p10", m_Param10);
        sWriter->EndObject();
    }

    void tSaveConditionalFormat::Json(const rapidjson::Value& sValue) {
        if (sValue.HasMember(kJsonKeyRef)) {
            m_Ref = sValue[kJsonKeyRef].GetString();
        }
        if (sValue.HasMember(kJsonKey)) {
            m_Key = sValue[kJsonKey].GetString();
        }
        if (sValue.HasMember(kJsonKeyTypeCell)) {
            m_Type = static_cast<tConditionalFormatType>(sValue[kJsonKeyTypeCell].GetInt());
        }
        if (sValue.HasMember("it")) {
            m_IconType = static_cast<tIconType>(sValue["it"].GetInt());
        }
        auto readParam = [&](const char* sKey, tSharedString& sParam) {
            if (sValue.HasMember(sKey)) {
                sParam = sValue[sKey].GetString();
            }
        };
        readParam("p1", m_Param1);
        readParam("p2", m_Param2);
        readParam("p3", m_Param3);
        readParam("p4", m_Param4);
        readParam("p5", m_Param5);
        readParam("p6", m_Param6);
        readParam("p7", m_Param7);
        readParam("p8", m_Param8);
        readParam("p9", m_Param9);
        readParam("p10", m_Param10);
        m_ConditionalFormat = nullptr;
    }
    
    //=========================================================================
    tSaveColRow::tSaveColRow() : tClass(), m_Index(-1),m_Size(0), m_Css(0),m_JsonFormat(""), m_Parent(-1)  {}
    tSaveColRow::tSaveColRow(tIndex sIndex) : tClass(), m_Index(sIndex),m_Size(0), m_Css(0),m_JsonFormat(""), m_Parent(-1)  {}
    tSaveColRow::tSaveColRow(tColRow* sColRow) : tClass(), m_Index(sColRow->Index()),m_Size(sColRow->Size()), m_Css(sColRow->Css()),m_JsonFormat(), m_Parent(-1)  {
        // Children filled as sheet indices by AddColRow (I/O / rebase)
    }
    tSaveColRow::~tSaveColRow() {}
    
    tIndex tSaveColRow::Index() { return(m_Index); };
    
    tDouble tSaveColRow::Size() { return(m_Size); }
    
    void tSaveColRow::Css(tFormatRef sCss) { m_Css=sCss; };
    tFormatRef tSaveColRow::Css() { return (m_Css); }
    
    tString tSaveColRow::JsonFormat() { return(m_JsonFormat()); };
    
    void tSaveColRow::Parent(tIndex sParent) { m_Parent=sParent; }
    tIndex tSaveColRow::Parent() { return(m_Parent); };
    
    void tSaveColRow::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key(kJsonKeyIndexColRow);
        sWriter->Int(m_Index);
        sWriter->Key(kJsonKeySize);
        sWriter->Double(m_Size);
        // Save format if CSS is available (for JSON mode)
        if (m_Css!=0) {
            tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
            if (wFormatApi!=nullptr) {
                tSize wJsonCss;
                if (m_Css!=0) {
                    // Use CSS directly
                    wJsonCss=wFormatApi->WriteJsonAddFormat(m_Css)+1;
                } else {
                    wJsonCss=0;
                }
                if (wJsonCss!=0) {
                    sWriter->Key(kJsonKeyFormat);
                    sWriter->Int(tInt(wJsonCss));
                }
            }
        }
        if (m_Parent!=-1) {
            sWriter->Key(kJsonKeyParent);
            sWriter->Int(m_Parent);
        }
        if (!m_Children.empty()) {
            sWriter->Key(kJsonKeyChildren);
            sWriter->StartArray();
            for (auto wChild : m_Children) {
                sWriter->Int(wChild);
            }
            sWriter->EndArray();
        }
        sWriter->EndObject();
    }
    
    void tSaveColRow::Json(const rapidjson::Value& sValue) {
        m_Index=sValue[kJsonKeyIndexColRow].GetInt();
        m_Size=sValue[kJsonKeySize].GetDouble();
        if (sValue.HasMember(kJsonKeyFormat)) {
            if (sValue.HasMember(kJsonKeyFormat)) {
                tInt wFormatInt = sValue[kJsonKeyFormat].GetInt();
                wFormatInt--;
                tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
                if (wFormatApi!=nullptr) {
                    m_JsonFormat = wFormatApi->ReadJsonGetFormat(wFormatInt);
                }
                m_Css=0;
            }
        }
        if (sValue.HasMember(kJsonKeyParent)) {
            m_Parent=sValue[kJsonKeyParent].GetInt();
        }
        if (sValue.HasMember(kJsonKeyChildren)) {
            const rapidjson::Value& wChildren = sValue[kJsonKeyChildren];
            for (auto& wChild : wChildren.GetArray()) {
                m_Children.push_back(wChild.GetInt());
            }
        }
    }
    
    tBool tSaveColRow::operator < (tSaveColRow & sSaveColRow) {
        return(m_Index<sSaveColRow.m_Index);
    }
    tBool tSaveColRow::operator == (tSaveColRow & sSaveColRow) {
        return(m_Index==sSaveColRow.m_Index);
    }
    
#ifdef checkfo
    void tSaveColRow::IncCheckfo(tFormatApi* sFormatApi) {
        if (m_Css!=0) {
            sFormatApi->IncCheck(m_Css);
        }
    }
#endif
    
    //=========================================================================
    tSaveSelect::tSaveSelect() : tClass(),m_UndoSpreadSheet(nullptr), m_SelectStr(),m_ColRowCellRange(nullptr){
    }
    
    tSaveSelect::~tSaveSelect() {
        Clear();
    }
    
    void tSaveSelect::Clear() {
        for (auto wSaveCell : m_VectorSaveCell) delete(wSaveCell);
        for (auto wSaveRange : m_VectorSaveRange) delete(wSaveRange);
        m_VectorSaveCell.clear();
        m_VectorSaveRange.clear();
        m_VectorCalculateExternalSaveCell.clear();
        m_VectorCellCalculate.clear();
    }

    void tSaveSelect::AbsorbFrom(tSaveSelect& sOther) {
        m_VectorSaveCell.insert(m_VectorSaveCell.end(),
            std::make_move_iterator(sOther.m_VectorSaveCell.begin()),
            std::make_move_iterator(sOther.m_VectorSaveCell.end()));
        sOther.m_VectorSaveCell.clear();
        m_VectorSaveRange.insert(m_VectorSaveRange.end(),
            std::make_move_iterator(sOther.m_VectorSaveRange.begin()),
            std::make_move_iterator(sOther.m_VectorSaveRange.end()));
        sOther.m_VectorSaveRange.clear();
        m_VectorCalculateExternalSaveCell.insert(m_VectorCalculateExternalSaveCell.end(),
            std::make_move_iterator(sOther.m_VectorCalculateExternalSaveCell.begin()),
            std::make_move_iterator(sOther.m_VectorCalculateExternalSaveCell.end()));
        sOther.m_VectorCalculateExternalSaveCell.clear();
        for (auto& wEntry : sOther.m_MapSaveJsonPayload) {
            m_MapSaveJsonPayload[wEntry.first] = wEntry.second;
        }
        sOther.m_MapSaveJsonPayload.clear();
        m_VectorCellCalculate.insert(m_VectorCellCalculate.end(),
            std::make_move_iterator(sOther.m_VectorCellCalculate.begin()),
            std::make_move_iterator(sOther.m_VectorCellCalculate.end()));
        sOther.m_VectorCellCalculate.clear();
    }
    
    void tSaveSelect::ColRowCellRange(tColRowCellRange* sColRowCellRange) {
        m_ColRowCellRange=sColRowCellRange;
    }

    // Rebase ===================================================================
    tBool tSaveSelect::Rebase(const tRebasePlan& sRebasePlan) {
        // Rebase select Str
        tSelect wSelect;
        wSelect.Parse(m_SelectStr());
        if (wSelect.Rebase(sRebasePlan)) {
            m_SelectStr = wSelect.StrRef();
        }
    

        // Rebase all save cells
        for (auto wSaveCell : m_VectorSaveCell) {
            if (!wSaveCell->Rebase(sRebasePlan)) {
                return false;
            }
        }
        
        // Rebase all save ranges using plan with all sheets
        for (auto wSaveRange : m_VectorSaveRange) {
            if (!wSaveRange->Rebase(sRebasePlan)) {
                return false;
            }
        }
        
        // Rebase external calculate cells using plan with all sheets
        for (auto wSaveCell : m_VectorCalculateExternalSaveCell) {
            if (!wSaveCell->Rebase(sRebasePlan)) {
                // External cell was deleted
                return false;
            }
        }
        
        return true;
    }
    
    tBool tSaveSelect::IsUndoActif() {
        // Method Do not Undo see Api DoDeleteCol Row etc
        if (m_UndoSpreadSheet==nullptr) return(false);
        return(m_UndoSpreadSheet->IsUndoActif());
    }
    
    tUndoSpreadSheet* tSaveSelect::UndoSpreadSheet() { return(m_UndoSpreadSheet); };
    void tSaveSelect::UndoSpreadSheet(tUndoSpreadSheet*  sUndoSpreadSheet) { m_UndoSpreadSheet=sUndoSpreadSheet; }
    
    tBool tSaveSelect::ParseSelect(tString sSelection) {
        m_SelectStr=sSelection;
        tSelect wSelect;
        tBool wOk=wSelect.Parse(sSelection);
        
        return(wOk);
    }

    tSelect tSaveSelect::Select() {
        tSelect wSelect;
        if (m_SelectStr()!="") {
            wSelect.Parse(m_SelectStr());
        }
        return(wSelect);
    }
            
    tSaveCell* tSaveSelect::FindCell(tIndex sRow,tIndex sCol,tSheet* sSheet) {
        tTypeItem wTypeItem=tTypeItem::t_Cell;
        tString wAttribute="";
        tAllocatorRef wIndiceAllocatorCellRange=sSheet->IndexAllocatorColRowCellRange();
        tSaveCell wSearch(wIndiceAllocatorCellRange,wTypeItem, sRow, sCol,0, wAttribute,0);
        // If exist return save cell
        tVectorSaveCell::iterator wWhere;
        wWhere = std::lower_bound(m_VectorSaveCell.begin(), m_VectorSaveCell.end(), &wSearch, tComparatorSaveCell());
        if (wWhere != m_VectorSaveCell.end()) {
            tSaveCell* wSaveCell = *wWhere;
            if (*wSaveCell == wSearch) {
                return(wSaveCell);
            }
        }
        return(nullptr);
    }
        
    tSaveCell* tSaveSelect::FindCell(tCell* sCell) {
        tIndex wRow = sCell->RowIndex();
        tIndex wCol = sCell->ColIndex();
        tString wAttribute = "";
        if (sCell->Type() == tTypeItem::t_Attribute) {
            tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(sCell);
            wAttribute = wCellAttribute->Name();
        }
        tAllocatorRef wIndiceAllocatorCellRange=sCell->ColRowCellRangeRef();
        tSaveCell wSearch(wIndiceAllocatorCellRange,sCell->Type(), wRow, wCol,0, wAttribute,0);
        // If exist return save cell
        tVectorSaveCell::iterator wWhere;
        wWhere = std::lower_bound(m_VectorSaveCell.begin(), m_VectorSaveCell.end(), &wSearch, tComparatorSaveCell());
        if (wWhere != m_VectorSaveCell.end()) {
            tSaveCell* wSaveCell = *wWhere;
            if (*wSaveCell == wSearch) {
                return(wSaveCell);
            }
        }
        return(nullptr);
    }
            
	tSaveCell* tSaveSelect::AddCell(tCell * sCell) {
#ifdef debugundoredo
        cout << "tSaveSelect::AddCell(" << ((tItem*)(sCell))->StrRef(true) << ")" << endl;
#endif
		tIndex wRow = sCell->RowIndex();
		tIndex wCol = sCell->ColIndex();
		tString wAttribute = "";
		if (sCell->Type() == tTypeItem::t_Attribute) {
			tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(sCell);
			wAttribute = wCellAttribute->Name();
		}
		tAllocatorRef wIndiceAllocatorCellRange=sCell->ColRowCellRangeRef();
		tSaveCell wSearch(wIndiceAllocatorCellRange,sCell->Type(), wRow, wCol,0, wAttribute,0);
        // If exist return save cell
		tVectorSaveCell::iterator wWhere;
		wWhere = std::lower_bound(m_VectorSaveCell.begin(), m_VectorSaveCell.end(), &wSearch, tComparatorSaveCell());
		if (wWhere != m_VectorSaveCell.end()) {
			tSaveCell* wSaveCell = *wWhere;
			if (*wSaveCell == wSearch) {
				return(wSaveCell);
			}
		}

		tSaveCell* wSaveCell = new tSaveCell(wSearch);

		m_VectorSaveCell.insert(wWhere, wSaveCell);
        // Set Save Cell Value
		wSaveCell->Set(sCell);
		return(wSaveCell);
	}

	void tSaveSelect::AddCellAttributes(tCell* sCell) {
        tCellClassAttribute* wCellClass=sCell->ClassAttribute();
        if (wCellClass!=nullptr){
			// Loop on attributes =====================================
			// only attribute are erased
			tIndex wPos = 0;
			tSize wSize = wCellClass->Size();
			for (wPos = 0; wPos < wSize; wPos++) {
				tCellAttribute* wCellAttribute = wCellClass->CellAttribute(wPos);
				AddCell(wCellAttribute);
			}
		}
	}

    tBool tSaveSelect::AddJsonPayload(tColRowCellRange* sColRowCellRange,tTempoPoint* sPoint) {
        tString wJsonPayLoad;
        if (sColRowCellRange->GetCellJsonPayload(sPoint->Row(),sPoint->Col(),wJsonPayLoad)) {
            const tPoint wPoint(sPoint->Row(),sPoint->Col());
            m_MapSaveJsonPayload[wPoint]=wJsonPayLoad;
            return true;
        }
        return false;
    }
    
    void tSaveSelect::UndoJsonPayload() {
        for(auto wMapRef : m_MapSaveJsonPayload) {
            tPoint wPoint=wMapRef.first;
            m_ColRowCellRange->SetCellJsonPayload(wPoint.Row(),wPoint.Col(),wMapRef.second);
        }
        m_MapSaveJsonPayload.clear();
    }

	void tSaveSelect::PushCalculate(tCell* sCell) {
#ifdef debugundoredo
    cout << "Add Cell Calculate " << sCell->tItem::StrRef() << endl;
#endif
        m_VectorCellCalculate.push_back(sCell);
	}

	void tSaveSelect::ClearCellCalculate() {
		m_VectorCellCalculate.clear();
	}

	/// @brief      CalculateDo with External cell dependent Not in RectDeleteArea Delete Col Row or attribute)
	void tSaveSelect::PopulateCalculationGraph(
		tContainerPath* sContainerPath,
		tColRowCellRange* sColRowCellRange,
		tVolatile sVolatile) {
		if (sContainerPath == nullptr || sColRowCellRange == nullptr) {
			return;
		}
		sContainerPath->BeginCalculate();
        tSelect wSelect;
		// For Set & Raz ======================================================
        if (m_SelectStr()!="") {
            wSelect.Parse(m_SelectStr());
            sContainerPath->AddSelect(sColRowCellRange, wSelect);
        }
		// To Delete Col row or attribute =====================================
		for (auto wItem : m_VectorCalculateExternalSaveCell) {
			tCell* wCell = wItem->Cell();
            if (wCell != nullptr) {
                sContainerPath->Add(wCell);
#ifdef _DEBUGSK
                assert(wCell->Row()!=nullptr);
                assert(wCell->Col()!=nullptr);
#endif
           }
		}
        m_VectorCalculateExternalSaveCell.clear();
		for (auto wItem : m_VectorCellCalculate) {
            if (wItem == nullptr || wItem->ColRowCellRange() == nullptr) {
                continue;
            }
            sContainerPath->Add(wItem);
        }
        // Add Volatile
        if (sVolatile!=tVolatile::t_None) {
            sColRowCellRange->AddPathVolatile(sContainerPath, sVolatile);
        }
		m_VectorCellCalculate.clear();
	}

	void tSaveSelect::BeginCooperativeCalculateDo(
		tColRowCellRange* sColRowCellRange,
		tVolatile sVolatile,
		tWorkBook* sWorkBook) {
		if (sColRowCellRange == nullptr || sWorkBook == nullptr) {
			return;
		}
		sWorkBook->BeginCooperativeCalculateFromSaveSelect(this, sColRowCellRange, sVolatile);
	}

	void tSaveSelect::CalculateDo(tColRowCellRange* sColRowCellRange,tVolatile sVolatile) {
		tSheet* wSheet = (sColRowCellRange != nullptr) ? sColRowCellRange->Sheet() : nullptr;
		tWorkBook* wWorkBook = (wSheet != nullptr) ? wSheet->WorkBook() : nullptr;
		if (wWorkBook != nullptr && wWorkBook->CooperativeCalculateEnabled()) {
			BeginCooperativeCalculateDo(sColRowCellRange, sVolatile, wWorkBook);
			return;
		}
		tContainerPath wContainerPath;
		PopulateCalculationGraph(&wContainerPath, sColRowCellRange, sVolatile);
		wContainerPath.EndCalculate();
	};


	tSaveCell* tSaveSelect::FindSaveCell(tAllocatorRef sAllocatorColRange,tTypeItem sType, tIndex sRow, tIndex sCol, tString sAttribute) {
		tSaveCell wSearch(sAllocatorColRange, sType,sRow, sCol,0,sAttribute,0);
		tVectorSaveCell::iterator wWhere;
		wWhere = std::lower_bound(m_VectorSaveCell.begin(), m_VectorSaveCell.end(), &wSearch, tComparatorSaveCell());
		if (wWhere != m_VectorSaveCell.end()) {
			tSaveCell* wSaveCell = *wWhere;
			if (*wSaveCell ==  wSearch) { return(wSaveCell); }
		}
		return(nullptr);
	}

	tSaveRange* tSaveSelect::AddRange(tRange * sRange) {
		tIndex wTop = sRange->TopIndex();
		tIndex wLeft = sRange->LeftIndex();
		tIndex wBottom = sRange->BottomIndex();
		tIndex wRight = sRange->RightIndex();
		// Search RangeNamed
		tString wName = sRange->WorkBook()->FindRangeNamed(sRange->AllocatorRef(),sRange->Sheet());
		tAllocatorRef wIndiceAllocatorCellRange = sRange->ColRowCellRangeRef();
		tSaveRange wSearch(tIndex(wIndiceAllocatorCellRange), wTop, wLeft, wBottom, wRight,wName);
		tVectorSaveRange::iterator wWhere;
		wWhere = std::lower_bound(m_VectorSaveRange.begin(), m_VectorSaveRange.end(), &wSearch, tComparatorSaveRange());
		if (wWhere != m_VectorSaveRange.end()) {
			tSaveRange* wSaveRange = *wWhere;
			if ((wTop == wSaveRange->Row()) &&
				(wLeft == wSaveRange->Col()) &&
				(wBottom == wSaveRange->Bottom()) &&
				(wRight == wSaveRange->Right())) {
				return(wSaveRange);
			}
		}
		tSaveRange* wSaveRange = new tSaveRange(tIndex(wIndiceAllocatorCellRange), wTop, wLeft, wBottom, wRight,wName);
		m_VectorSaveRange.insert(wWhere, wSaveRange);
		wSaveRange->SetRefAndDepend(this, sRange);
		return(wSaveRange);
	}

	void tSaveSelect::TreatsExternalDependencyCells(tCell* sCell,  tBool sEraseSheet) {
#ifdef debugundoredo
		cout << "TreatsExternalDependencyCells  " << sCell->Sheet()->Name() << "!" << ((tItem*)(sCell))->StrRef() << endl;
#endif
		// Loop on depency cell ===============================================
		for (auto wItemDependent : *sCell->ContainerCellDepend()->Container()) {
			tCell* wCellDependent = wItemDependent->Cell();
			tIndex wIndex = 0;
#ifdef debugundoredo
            cout << "   --Dependent  ";
            cout << ((tItem*)(wCellDependent))->StrRef(true);
            cout << "=" << wCellDependent->FormulaStr() << " : " << wCellDependent->Value() << endl;
#endif
            // Loop on formula elem to find sCell and modify formula
			for (auto wItemFormula : *wCellDependent->VectorRef()) {
				if (wItemFormula != nullptr) {
					tCell* wCellFormula = wItemFormula->Cell();
					if (wCellFormula != nullptr) {
						if (wCellFormula == sCell) {
							tBool wIsExternal = ExternalCell(sCell, wCellDependent, sEraseSheet);
                            if (wIsExternal) {
#ifdef debugundoredo
                                cout << "   --External Modify Dependent " << wCellDependent->StrRef(true) << " ";
#endif
                                tIndex wRow = wCellDependent->RowIndex();
                                tIndex wCol = wCellDependent->ColIndex();
                                
                                // To calculate after
                                PushCalculate(wCellDependent);
                                
                                if (IsUndoActif()) {
                                    tString wAttribute;
                                    if (wCellDependent->Type() == tTypeItem::t_Attribute) {
                                        tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCellDependent);
                                        wAttribute = wCellAttribute->Name();
                                    }
#ifdef debugundoredo
									cout << "Alloc SaveFormulaCell:" << Base10ToAlpha(wCol) << wRow << endl; 
#endif
    
                                    tSaveFormulaCell wSaveFormulaCell(tIndex(wCellDependent->ColRowCellRangeRef()),
                                                                      wCellDependent->Type(),
                                                                      wRow,
                                                                      wCol,
                                                                      wAttribute,
                                                                      wIndex);
                                    tSaveCell* wSaveCell = AddCell(sCell);
                                    wSaveCell->PushFormulaCell(wSaveFormulaCell);
                                    if (ExternalCell(sCell )) {
                                        m_VectorCalculateExternalSaveCell.push_back(wSaveCell);
                                    }
                                }
                                //=============================================
                                // Modif vector ref formula
                                //=============================================
                                (*wCellDependent->VectorRef())[wIndex] = nullptr;
#ifdef debugundoredo
                            cout << endl;
#endif
                            }
						}
					}
				}
				wIndex++;
			}
			}
#ifdef debugundoredo
		cout << "Erase dependent cell..." << endl;
#endif
            // Erase dependent cell (sCell is erase after this operation)
            for (auto wItemFormula : *sCell->VectorRef()) {
                if (wItemFormula!=nullptr) {
                    tCell* wCellFormula=wItemFormula->Cell();
                    if (wCellFormula!=nullptr) {
                        if (ExternalCell(sCell, wCellFormula, sEraseSheet)) {
                            // Delete dependent cell
                            wCellFormula->DeleteDependent(sCell);
#ifdef debugundoredo
                            cout << "   --Delete Dependent formula  ";;
                            cout << wCellFormula->Sheet()->Name() << "!" << ((tItem*)(wCellFormula))->StrRef() << endl;
#endif
                            // If Erase Sheet save dependent
                            if ((sEraseSheet) && (IsUndoActif())) {
                                AddCell(sCell);
                            }
                            
                        }
                    }
                }
            }
		}

	void tSaveSelect::TreatsExternalDependencyRange(tRange* sRange, tBool sEraseSheet) {
		if (sRange == nullptr) {
			return;
		}
		tItem::tContainerCell* wDependents = sRange->ContainerCellDepend();
		if (wDependents == nullptr) {
			return;
		}
		tCell* wAnchorCell = sRange->Cell();
		if (wAnchorCell == nullptr && m_ColRowCellRange != nullptr) {
			wAnchorCell = m_ColRowCellRange->Cell(sRange->TopIndex(), sRange->LeftIndex());
		}
		for (auto wItemDependent : *wDependents->Container()) {
			tCell* wCellDependent = wItemDependent->Cell();
			if (wCellDependent == nullptr) {
				continue;
			}
			tIndex wIndex = 0;
			for (auto wItemFormula : *wCellDependent->VectorRef()) {
				if (wItemFormula == sRange) {
					tBool wIsExternal = false;
					if (wAnchorCell != nullptr) {
						wIsExternal = ExternalCell(wAnchorCell, wCellDependent, sEraseSheet);
					} else if (sEraseSheet && m_ColRowCellRange != nullptr) {
						wIsExternal = (wCellDependent->Sheet() != m_ColRowCellRange->Sheet());
					}
					if (wIsExternal) {
						PushCalculate(wCellDependent);
						if (IsUndoActif() && wAnchorCell != nullptr) {
							tString wAttribute;
							if (wCellDependent->Type() == tTypeItem::t_Attribute) {
								tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCellDependent);
								wAttribute = wCellAttribute->Name();
							}
							tSaveFormulaCell wSaveFormulaCell(tIndex(wCellDependent->ColRowCellRangeRef()),
							                                  wCellDependent->Type(),
							                                  wCellDependent->RowIndex(),
							                                  wCellDependent->ColIndex(),
							                                  wAttribute,
							                                  wIndex);
							tSaveCell* wSaveCell = AddCell(wAnchorCell);
							wSaveCell->PushFormulaCell(wSaveFormulaCell);
							if (ExternalCell(wAnchorCell)) {
								m_VectorCalculateExternalSaveCell.push_back(wSaveCell);
							}
						}
						(*wCellDependent->VectorRef())[wIndex] = nullptr;
						sRange->DeleteDependent(wCellDependent);
					}
				}
				wIndex++;
			}
		}
	}

    tBool tSaveSelect::ExternalCell(tCell* sCell,tBool sEraseSheet) { return(false); };

	tBool tSaveSelect::ExternalCell(tCell* sCell, tCell* sCellDependent, tBool sEraseSheet) { return(true); }
            
    void tSaveSelect::UndoCell(tSaveCell* sSaveCell) {
        tCell* wCell = sSaveCell->EnsureCell();
        tWorkBook* wWorkBook=wCell->WorkBook();
        const tBool wIsJsonUndo = (m_UndoSpreadSheet != nullptr && m_UndoSpreadSheet->IsJson());
   
#ifdef debugundoredo
        cout << "UndoCell:" << sSaveCell->Debug() << endl;
#endif
        // Recup Formula and dependent cell (Create Range Calculate)
        // Formula variants are US wire; CompilCell must use Locale us (UI may be FR).
        // Format-only clear never touched the content: skip content restore, restore CSS only.
        if (!m_FormatOnly) {
            if (sSaveCell->Value()->HasFormula()) {
                tLocalePush wUs("us");
                wWorkBook->CellValue(wCell, *sSaveCell->Value(), false);
            } else {
                wWorkBook->CellValue(wCell, *sSaveCell->Value(), false);
            }
            // Restore MatOrigin / MatExtend / MatDynamic / SpillRange flags after CellValue
            // (compile paths may clear them). Needed so Undo of "type into spill slave" can
            // re-spill: orphaned MatExtend slaves are skipped by SpillDestinationIsClear.
            wCell->Extension(sSaveCell->Extension());
        }
        // In KeepFormat mode the format was never removed: restore content only.
        if (!ExternalCell(wCell) && !m_KeepFormat) {
            if (!wIsJsonUndo) {
                // PassCss snapshot is consumed on first UndoCell; a second visit must
                // not re-apply Css 0 (clears live format) — e.g. move undo: paste bxr
                // restores E15, then UndoSourceOutsideDest hits the same save again.
                if (sSaveCell->Css() != 0) {
                    // Delete old Format
                    if (wCell->Css()!=0) {
                        wWorkBook->DeleteCellFormat(wCell->Css()); // Bug with Paste
                    }
                    // Apply New
                    wCell->Css(sSaveCell->Css());
                    sSaveCell->Css(0);
                }
            } else {
                // Apply Format ================================================
                // Delete old Format
                if (wCell->Css()!=0) {
                    wWorkBook->DeleteCellFormat(wCell->Css());
                    wCell->Css(0);
                }
                // Apply New
                wWorkBook->ApplyCellFormat(wCell,sSaveCell->FormatStr());
                
#ifdef debugundoredo
                cout << "On " << wWorkBook->Uri() << " Format=" << wWorkBook->CellFormat(wCell->Css()) << endl;
#endif
                sSaveCell->Css(0);
            }
        }
        
        // Replace Class ======================================================
        // Format-only clear preserves live class attributes: never re-root/clear them here.
        tCellClassAttribute* wCellClassAttribute=wCell->ClassAttribute();
        if (!m_FormatOnly && wCellClassAttribute!=nullptr) {
            tColRowCellRange* wColRowCellRange=sSaveCell->ColRowCellRange();
            wCellClassAttribute->CellRootRef(wColRowCellRange,sSaveCell->Row(),sSaveCell->Col());
            wCellClassAttribute->Rooted(wCell);
            // Clear an add Attribute by Instance of tClassAttribute in Value of cell
            wCellClassAttribute->ClearAttribute();
            wCellClassAttribute->ClassName(sSaveCell->ClassName());
            tString wRefName=sSaveCell->RefName();
            // if name already exist, get next name
            tCellClassContainer* wCellClassContainer=wColRowCellRange->CellClassContainer();
            if (wCellClassContainer->CellByName(wRefName)!=nullptr) {
                wRefName=wCellClassContainer->GetNextName(wRefName);
            }
            
            wCellClassAttribute->RefName(wRefName);
            wColRowCellRange->InsertCellClassAttributeContainer(wCell);
        }
        // Recup Formula
#ifdef debugundoredo
        cout << "Recup fomula->" << sSaveCell->StrRef() << endl;
#endif
        sSaveCell->RecoverFormulaCell(wIsJsonUndo);
        // Wire inverse deps on sources (B16 lists D16). Must run after CompilCell/RecoverFormulaCell.
        wCell->AddDependant();
        // Drop formula cells still listed on this source but no longer referencing it in VectorRef.
        wCell->PruneStaleInverseDependents();
        if (wCell->Formula() != nullptr || !wCell->FormulaStr().empty()) {
            PushCalculate(wCell);
        }
            
        if (wCell->Type()==tTypeItem::t_Attribute) {
            PushCalculate(wCell);
        }
    }

namespace {

// After UndoCell restores values, some in-area formula links only exist in VectorRef
// (class/sparkline paths) without ContainerCellDepend on the source; checksp requires both.
// Scoped to saved cells in this undo payload — not a sheet-wide repair (contrast with removed RewireAll in Check).
static tBool SaveRangeHasConditionalFormatOverlay(tSaveRange* sSaveRange) {
    if (sSaveRange == nullptr) {
        return(false);
    }
    return(sSaveRange->IsCFCF() || sSaveRange->IsCFHR() || sSaveRange->IsCFDB()
        || sSaveRange->IsCFCS() || sSaveRange->IsCFIS());
}

static void RewireOutgoingFormulaDependentsImpl(const tVectorSaveCell& sSaveCells) {
    for (auto wSaveCell : sSaveCells) {
        tCell* wCell = wSaveCell->EnsureCell();
        if (wCell == nullptr) {
            continue;
        }
        for (auto wItemRef : *wCell->VectorRef()) {
            if (wItemRef == nullptr) {
                continue;
            }
            tItem::tContainerCell* wDependContainer = wItemRef->ContainerCellDepend();
            if (wDependContainer == nullptr) {
                wItemRef->AddDependent(wCell);
                continue;
            }
            tBool wLinked = false;
            for (auto wDep : *wDependContainer->Container()) {
                if (wDep == wCell) {
                    wLinked = true;
                    break;
                }
            }
            if (!wLinked) {
                wItemRef->AddDependent(wCell);
            }
        }
    }
}

} // namespace

void tSaveSelect::RewireOutgoingFormulaDependents() {
    RewireOutgoingFormulaDependentsImpl(m_VectorSaveCell);
}

void tSaveSelect::PushSavedFormulaDependentsCalculate() {
    for (auto wSaveCell : m_VectorSaveCell) {
        for (auto& wSaveFormulaCell : *wSaveCell->VectorFormulaCellDepend()) {
            if (tCell* wCellWithFormula = wSaveFormulaCell.Cell()) {
                PushCalculate(wCellWithFormula);
            }
        }
    }
    for (auto wSaveRange : m_VectorSaveRange) {
        for (auto& wSaveFormulaCell : *wSaveRange->VectorFormulaCellDepend()) {
            if (tCell* wCellWithFormula = wSaveFormulaCell.Cell()) {
                PushCalculate(wCellWithFormula);
            }
        }
    }
}

	void tSaveSelectErase::DeleteRemovedRangeClean(tAllocatorRef sAllocatorRef) {
        if (m_ColRowCellRange == nullptr || m_ColRowCellRange->IsDeletedRange(sAllocatorRef)) {
            return;
        }
        m_ColRowCellRange->DeleteRangeByAllocatorRef(sAllocatorRef, true);
    }

    void tSaveSelectErase::SaveConditionalFormatsForRangeUndo(tSaveRange* sSaveRange, tRange* sRange) {
        tRangeConditionnalFormat* wRangeConditionalFormat = m_ColRowCellRange->ConditionalFormatByRange(sRange);
        if (wRangeConditionalFormat == nullptr) {
            tStringStream wStream;
            wStream << "throw: tSaveSelectErase::RemovesRangesAndProcessesOverlays "
                << sRange->tItem::StrRef() << " Not Format contional predent";
            cerr << wStream.str() << endl;
            return;
        }
        static const tConditionalFormatType kCfTypes[] = {
            tConditionalFormatType::t_HighlightCellsRules,
            tConditionalFormatType::t_DataBars,
            tConditionalFormatType::t_ColorScales,
            tConditionalFormatType::t_IconSets,
            tConditionalFormatType::t_CustomFormulas,
        };
        for (tConditionalFormatType wType : kCfTypes) {
            tConditionalFormat* wConditionalFormat = wRangeConditionalFormat->ConditionalFormat(wType);
            if (wConditionalFormat != nullptr) {
                GetOrCreateSaveConditionalFormat(wConditionalFormat, sSaveRange);
            }
        }
    }

    void tSaveSelectErase::EraseConditionalFormatRangeInDeleteArea(tSaveRange* sSaveRange) {
        if (m_ColRowCellRange->IsDeletedRange(sSaveRange->AllocatorRef())) {
            return;
        }
        tRange* wCfRange = sSaveRange->Range();
        if (wCfRange == nullptr || !wCfRange->InsideRect(&m_RectDeleteArea)) {
            return;
        }
        tConditionalFormatContainer* wContainer = m_ColRowCellRange->ConditionalFormatContainer();
        if (wContainer != nullptr) {
            wContainer->DeleteRangeInMapRange(wCfRange);
        }
        if (wCfRange->IsConditionalFormat()) {
            wCfRange->RemoveConditionalFormat();
        }
        m_ColRowCellRange->DeleteRangeByAllocatorRef(sSaveRange->AllocatorRef(), true);
    }

    void tSaveSelectErase::ProcessDeletedRangeRemoval(tSaveRange* sSaveRange) {
        if (sSaveRange->IsConditionalFormat()) {
#ifdef debugundoredo
            cout << "Delete ByRange --->" << sSaveRange->StrRef() << endl;
#endif
            tRange* wRange = sSaveRange->Range();
            SaveConditionalFormatsForRangeUndo(sSaveRange, wRange);
            EraseConditionalFormatRangeInDeleteArea(sSaveRange);
        } else {
            DeleteRemovedRangeClean(sSaveRange->AllocatorRef());
        }
    }

    void tSaveSelectErase::ProcessConditionalFormatsInDeleteArea() {
        for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) {
            m_ColRowCellRange->RemoveConditionalFormatByRect(
                wSaveConditionalFormat->Type(), wSaveConditionalFormat->Key(), &m_RectDeleteArea);
        }
        m_ColRowCellRange->StripOrphanConditionalFormatExtension();
    }

    tBool tSaveSelectErase::ComputeRangeFullyInsideDeleteArea(tBool sIsRect, tRange* sRange,
        tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
        tBool wIsInRect = ((sTop >= m_RectDeleteArea.Top()) && (sBottom <= m_RectDeleteArea.Bottom())
            && (sLeft >= m_RectDeleteArea.Left()) && (sRight <= m_RectDeleteArea.Right()));
        if (!sIsRect) {
            return(wIsInRect);
        }
        tItem::tContainerCell* wVectorDependent = sRange->ContainerCellDepend();
        tInt wNbDepend = 0;
        for (auto wCellDepend : (*wVectorDependent->Container())) {
            tCell* wCell = wCellDepend->Cell();
            tBool wCellIsInRect = ((wCell->RowIndex() >= m_RectDeleteArea.Top())
                && (wCell->RowIndex() <= m_RectDeleteArea.Bottom())
                && (wCell->ColIndex() >= m_RectDeleteArea.Left())
                && (wCell->ColIndex() <= m_RectDeleteArea.Right()));
            if (wCellIsInRect) {
                wNbDepend++;
            }
        }
        if (wNbDepend == wVectorDependent->Container()->size()) {
            wIsInRect = true;
        }
        return(wIsInRect);
    }

    void tSaveSelectErase::ApplyRangeCoordAfterDelete(tIndex sPosition, tIndex sSize,
        tIndex& sTop, tIndex& sLeft, tIndex& sBottom, tIndex& sRight) {
        if (m_DoesRow) {
            tBool wStartInZone = ((sTop >= sPosition) && (sTop < sPosition + sSize));
            tBool wEndInZone = ((sBottom >= sPosition) && (sBottom < sPosition + sSize));

            if (wStartInZone) {
                sTop = sPosition;
            } else if (sTop > sPosition + sSize) {
                sTop -= sSize;
            }

            if (wEndInZone) {
                if (sPosition > 0) {
                    sBottom = sPosition - 1;
                } else if (wStartInZone) {
                    sBottom = static_cast<tIndex>(-1);
                } else {
                    sBottom = static_cast<tIndex>(-1);
                }
            } else if (sBottom >= sPosition + sSize) {
                sBottom -= sSize;
            }
        } else {
            tBool wStartInZone = ((sLeft >= sPosition) && (sLeft < sPosition + sSize));
            tBool wEndInZone = ((sRight >= sPosition) && (sRight < sPosition + sSize));

            if (wStartInZone) {
                sLeft = sPosition;
            } else if (sLeft > sPosition + sSize) {
                sLeft -= sSize;
            }

            if (wEndInZone) {
                if (sPosition > 0) {
                    sRight = sPosition - 1;
                } else if (wStartInZone) {
                    sRight = static_cast<tIndex>(-1);
                } else {
                    sRight = static_cast<tIndex>(-1);
                }
            } else if (sRight >= sPosition + sSize) {
                sRight -= sSize;
            }
        }
    }

    tBool tSaveSelectErase::IsRangeCoordInvalidAfterDelete(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
        if (m_DoesRow) {
            return(sBottom < sTop || sBottom == static_cast<tIndex>(-1) || sTop == static_cast<tIndex>(-1));
        }
        return(sRight < sLeft || sRight == static_cast<tIndex>(-1) || sLeft == static_cast<tIndex>(-1));
    }

	void tSaveSelect::UndoCells() {
#ifdef debugundoredo
            cout << "UndoCells ---------------------------------------------" << endl;
#endif
		for (auto wSaveCell : m_VectorSaveCell) {
#ifdef debugundoredo
            cout << "Undo --> " << Base10ToAlpha(wSaveCell->Col()) << wSaveCell->Row();
            if (wSaveCell->Attribute() !="") cout <<"." << wSaveCell->Attribute();
            cout  << "-> Css=" << wSaveCell->Css();
            cout << endl;
#endif
			
            UndoCell(wSaveCell);
		}
        RewireOutgoingFormulaDependents();
#ifdef debugundoredo
            cout << "-------------------------------------------------------" << endl;
#endif
        // Recup JsonPayload
        UndoJsonPayload();
	}

    void tSaveSelect::UndoCellsOutsideSelect() {
        tSelect wSelect;
        if (m_SelectStr() != "") {
            wSelect.Parse(m_SelectStr());
        }
        auto wSelectCovers = [&wSelect](tIndex sRow, tIndex sCol) -> tBool {
            tVectorTempoPoint* wVec = wSelect.VectorSelect();
            if (wVec == nullptr) {
                return false;
            }
            for (tTempoPoint* wElem : *wVec) {
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
                if (wRect != nullptr) {
                    if (sRow >= wRect->Top() && sRow <= wRect->Bottom()
                        && sCol >= wRect->Left() && sCol <= wRect->Right()) {
                        return true;
                    }
                } else if (wElem != nullptr) {
                    if (wElem->Row() == sRow && wElem->Col() == sCol) {
                        return true;
                    }
                }
            }
            return false;
        };
        for (auto wSaveCell : m_VectorSaveCell) {
            // Only plain cells (spill siblings). Attributes stay on the CallBack / class path.
            if (wSaveCell == nullptr || wSaveCell->Attribute() != "") {
                continue;
            }
            if (wSelectCovers(wSaveCell->Row(), wSaveCell->Col())) {
                continue;
            }
            UndoCell(wSaveCell);
        }
    }

    tBool tSaveSelect::IsEmpty() {
        return(m_VectorSaveCell.empty() && m_VectorSaveRange.empty());
    }

    void  tSaveSelect::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        // RESET Json Index of format
        tWorkBook* wWorkBook=sSheet->WorkBook();
      
        tFormatApi* wFormatApi =wWorkBook->FormatApi();
        if (wFormatApi!=nullptr) {
            // Init save Format
            wFormatApi->BeginWriteJson();
        }
    
     
        sWriter->Key(kJsonKeySelection);
		sWriter->String(m_SelectStr().c_str());

        if (!m_VectorSaveCell.empty()) {
            sWriter->Key(kJsonKeyListCell);
            sWriter->StartArray();
            for (auto wSaveCell : m_VectorSaveCell) {
                wSaveCell->Json(sWriter,sSheet,true);
            }
            sWriter->EndArray();
        }
        
        if (!m_VectorSaveRange.empty()) {
            sWriter->Key(kJsonKeyListRange);
            sWriter->StartArray();
            for (auto wSaveRange : m_VectorSaveRange) {
                wSaveRange->Json(sWriter,sSheet,true);
            }
            sWriter->EndArray();
        }
    
        // Save Format Str
        if (wFormatApi!=nullptr) {
            sWriter->Key(kJsonKeyFormatString);
            wFormatApi->Json(sWriter);
        }
     
    }

    void tSaveSelect::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        m_ColRowCellRange=sSheet->ColRowCellRange();
        
        // load Format String
        tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
        if (wFormatApi!=nullptr) {
            if (sValue.HasMember(kJsonKeyFormatString)) {
                wFormatApi->Json(sValue[kJsonKeyFormatString]);
            }
        }
		m_SelectStr=sValue[kJsonKeySelection].GetString();
        if (sValue.HasMember(kJsonKeyListCell)) {
            const rapidjson::Value& wSaveCellValue = sValue[kJsonKeyListCell];
            for (auto& wCellValue : wSaveCellValue.GetArray()) {
                tSaveCell* wSaveCell=new tSaveCell();
                wSaveCell->Json(wCellValue,m_ColRowCellRange->Sheet());
				m_VectorSaveCell.push_back(wSaveCell);
            }
        }
        if (sValue.HasMember(kJsonKeyListRange)) {
            const rapidjson::Value& wSaveRangeValue = sValue[kJsonKeyListRange];
            for (auto& wRangeValue : wSaveRangeValue.GetArray()) {
                tSaveRange* wSaveRange=new tSaveRange();
				wSaveRange->Json(wRangeValue,m_ColRowCellRange->Sheet());
				m_VectorSaveRange.push_back(wSaveRange);
            }
        }
       
    }
            
    tString tSaveSelect::WriteJson(tSheet* sSheet) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        Json(&wWriter,sSheet);
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    void tSaveSelect::ReadJson(tString sJson,tColRowCellRange* sColRowCellRange) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        Json(wDocument,sColRowCellRange->Sheet());
    };
            
#ifdef _DEBUGSK
	tString tSaveSelect::Debug() {
        tStringStream wStream;
		wStream << "tSaveSelect ------------------------------" << endl;
		for (auto wSaveCell : m_VectorSaveCell) wStream << wSaveCell->Debug() << endl;
        wStream  <<  "tSaveRange -------------------------------" << endl;
		for (auto wSaveRange : m_VectorSaveRange) wStream << wSaveRange->Debug() << endl;
        wStream  << "tSaveCalulate ------------------------------" << endl;
        for (auto wCell : m_VectorCellCalculate) wStream << wCell->StrRef(true) << endl;
        wStream << "tSaveExternalCalculate -------------------" << endl;
        for (auto wCell : m_VectorCalculateExternalSaveCell) wStream << wCell->StrRef() << endl;

		wStream << "-------------------------------------------" << endl;
        return(wStream.str());
	}
#endif
            
#ifdef checkfo
        void tSaveSelect::IncCheckfo(tFormatApi* sFormatApi) {
            // Save Cell
            for (auto wSaveCell : m_VectorSaveCell) {
                wSaveCell->IncCheckfo(sFormatApi);
            }
        };
#endif
    //=========================================================================
    tSaveSelectColRow::tSaveSelectColRow(tBool sDoesRow) : tSaveSelect(),m_DoesRow(sDoesRow) {}
    tSaveSelectColRow::~tSaveSelectColRow() {
        Clear();
    }
    
    void tSaveSelectColRow::Clear() {
        tSaveSelect::Clear();
        for(tSaveColRow* wSaveColRow : m_VectorSaveColRow) {
            delete(wSaveColRow);
        }
        m_VectorSaveColRow.clear();
    }
            
    void tSaveSelectColRow::DeleteCellFormat(tSheet* sSheet) {
        for(tSaveColRow* wSaveColRow : m_VectorSaveColRow) {
            if (wSaveColRow->Css()!=0)  {
                sSheet->WorkBook()->DeleteCellFormat(wSaveColRow->Css());
                wSaveColRow->Css(0);
            }
        }
    }
        
    tSaveColRow* tSaveSelectColRow::AddColRow(tColRow* sColRow,tColRowCellRange* sColRowCellRange,tBool sIsRow) {
        tSaveColRow wSearch(sColRow->Index());
        // If exist return save cell
        tVectorSaveColRow::iterator wWhere;
        wWhere = std::lower_bound(m_VectorSaveColRow.begin(), m_VectorSaveColRow.end(), &wSearch, tComparatorSaveColRow());
        if (wWhere != m_VectorSaveColRow.end()) {
            tSaveColRow* wSaveColRow = *wWhere;
            if (*wSaveColRow == wSearch) {
                return(wSaveColRow);
            }
        }
        // else create SaveColRow
        tSaveColRow* wSaveColRow = new tSaveColRow(sColRow);
        
        // Save FormatStr in JSON mode (similar to how cells handle it)
        if (m_UndoSpreadSheet != nullptr && m_UndoSpreadSheet->IsJson() && sColRow->Css() != 0) {
            tWorkBook* wWorkBook = sColRowCellRange->Sheet()->WorkBook();
            tString wFormatStr = wWorkBook->CellFormat(sColRow->Css());
            if (wFormatStr != "") {
                wSaveColRow->m_JsonFormat = wFormatStr;
            }
        }
        
        tColRow* wColRowParent=sColRow->ColRowParent(sColRowCellRange, sIsRow);
        // Save Parent By Index
        if (wColRowParent!=nullptr) wSaveColRow->Parent(wColRowParent->Index());
        // Save Children By Index (AllocatorRef links on live tColRow)
        for (auto wChildRef : *sColRow->VectorChildren()) {
            tColRow* wChild=sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
            if (wChild!=nullptr) {
                wSaveColRow->m_Children.push_back(wChild->Index());
            }
        }
                
        m_VectorSaveColRow.insert(wWhere, wSaveColRow);
        return(wSaveColRow);
    }

    tSaveColRow* tSaveSelectColRow::FindColRow(tIndex sIndex) {
        tSaveColRow wSearch(sIndex);
        tVectorSaveColRow::iterator wWhere;
        wWhere = std::lower_bound(m_VectorSaveColRow.begin(), m_VectorSaveColRow.end(), &wSearch, tComparatorSaveColRow());
        if (wWhere != m_VectorSaveColRow.end()) {
            tSaveColRow* wSaveColRow = *wWhere;
            if (*wSaveColRow == wSearch) {
                return(wSaveColRow);
            }
        }
        return(nullptr);
    }
            
    void tSaveSelectColRow::RecupColRow(tColRow* sColRow,tSaveColRow* sSaveColRow,tColRowCellRange* sColRowCellRange,tBool sIsRow,tBool sFormatSaveOwned) {
        sColRow->Index(sSaveColRow->Index());
        sColRow->Size(sSaveColRow->Size());
        
        // Handle format restoration similar to UndoCell
        if (m_UndoSpreadSheet != nullptr && m_UndoSpreadSheet->IsJson()) {
            // JSON mode: delete old format and apply from FormatStr
            if (sColRow->Css() != 0) {
                sColRowCellRange->Sheet()->WorkBook()->DeleteCellFormat(sColRow->Css());
                sColRow->Css(0);
            }
            // Apply format from FormatStr if available
            if (sSaveColRow->JsonFormat() != "") {
                sColRowCellRange->Sheet()->WorkBook()->ApplyColRowFormat(sColRow, sSaveColRow->JsonFormat());
            }
        } else {
            // Non-JSON mode.
            // Delete undo: AddColRow + IncCellFormat, then DeleteFormat on the live
            // ColRow — save owns the ref. Recup must Dec any insert-copied Css on
            // the new ColRow, assign save Css, then caller zeros save Css (transfer).
            // Tree undo: AddColRow only copies Css (no Inc). Live ColRow still owns
            // it. Dec+reassign of the same ref undercounts Real (e.g. Real:1 Check:5).
            const tFormatRef wSavedCss = sSaveColRow->Css();
            const tFormatRef wCurrentCss = sColRow->Css();
            tWorkBook* wWorkBook = sColRowCellRange->Sheet()->WorkBook();
            if (sFormatSaveOwned) {
                if (wCurrentCss != 0) {
                    wWorkBook->DeleteCellFormat(wCurrentCss);
                }
                sColRow->Css(wSavedCss);
            } else if (wCurrentCss != wSavedCss) {
                if (wCurrentCss != 0) {
                    wWorkBook->DeleteCellFormat(wCurrentCss);
                }
                if (wSavedCss != 0) {
                    wWorkBook->IncCellFormat(wSavedCss);
                }
                sColRow->Css(wSavedCss);
            }
        }
        
        tAllocatorRef wParentRef=0;
        // Refuse self-parent (would freeze GrandParent / Deep / JsonView walks).
        if (sSaveColRow->Parent()!=-1 && sSaveColRow->Parent()!=sColRow->Index()) {
            if (sIsRow) {
                wParentRef=sColRowCellRange->RowAllocatorRef(sSaveColRow->Parent());
            } else {
                wParentRef=sColRowCellRange->ColAllocatorRef(sSaveColRow->Parent());
            }
        }
        sColRow->ParentRef(wParentRef);
        // Self allocator for restoring child -> parent links (children may not be in the save list)
        tAllocatorRef wSelfRef=sIsRow
            ? sColRowCellRange->RowAllocatorRef(sColRow->Index())
            : sColRowCellRange->ColAllocatorRef(sColRow->Index());
        sColRow->m_Children.clear();
        for (auto wChildIndex : sSaveColRow->m_Children) {
            if (wChildIndex==sColRow->Index()) {
                continue;
            }
            sColRowCellRange->EnsureColRow(wChildIndex, sIsRow);
            tAllocatorRef wChildRef=sIsRow
                ? sColRowCellRange->RowAllocatorRef(wChildIndex)
                : sColRowCellRange->ColAllocatorRef(wChildIndex);
            if (wChildRef!=0 && wChildRef!=wSelfRef) {
                sColRow->m_Children.push_back(wChildRef);
                // Keep ParentRef in sync when undoing delete of a parent alone
                tColRow* wChild=sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wChild!=nullptr) {
                    wChild->ParentRef(wSelfRef);
                }
            }
        }
    }
    tVectorSaveColRow* tSaveSelectColRow::VectorColRow() { return(&m_VectorSaveColRow); }
            
    void tSaveSelectColRow::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        tSaveSelect::Json(sWriter,sSheet);
        sWriter->Key(kJsonKeyColRow);
        sWriter->StartArray();
        for (auto wSaveColRow : m_VectorSaveColRow) {
            wSaveColRow->Json(sWriter);
        }
        sWriter->EndArray();
    }
            
    void tSaveSelectColRow::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        tSaveSelect::Json(sValue,sSheet);
        if (sValue.HasMember(kJsonKeyColRow)) {
            const rapidjson::Value& wSaveColRowValue = sValue[kJsonKeyColRow];
            for (auto& wColRowValue : wSaveColRowValue.GetArray()) {
                tSaveColRow* wSaveColRow = new tSaveColRow();
                wSaveColRow->Json(wColRowValue);
                m_VectorSaveColRow.push_back(wSaveColRow);
            }
        }
    }
            
#ifdef checkfo
    void tSaveSelectColRow::IncCheckfo(tFormatApi* sFormatApi) {
        // SaveCell Format
        tSaveSelect::IncCheckfo(sFormatApi);
        // Row col Format
        for(tSaveColRow* wSaveColRow : m_VectorSaveColRow) {
            wSaveColRow->IncCheckfo(sFormatApi);
        }
    }
#endif

    // Rebase ====================================================
    tBool tSaveSelectColRow::Rebase(const tRebasePlan& sRebasePlan) {
        // First rebase parent class (cells and ranges)
        if (!tSaveSelect::Rebase(sRebasePlan)) {
            return false;
        }
  
        // Rebase all colrow indices using the DoesRow parameter
        for (auto wSaveColRow : m_VectorSaveColRow) {
            tIndex wIndex = wSaveColRow->Index();
            
            // Rebase based on whether we're working with rows or columns
            std::optional<tIndex> wRebased;
            if (m_DoesRow) {
                wRebased = sRebasePlan.RebaseRow(wIndex);
            } else {
                wRebased = sRebasePlan.RebaseCol(wIndex);
            }
            
            if (!wRebased.has_value()) {
                return false;
            }
            
            // Update index
            wSaveColRow->m_Index = wRebased.value();
            
            // Rebase parent index if present
            tIndex wParent = wSaveColRow->Parent();
            if (wParent != -1) {
                std::optional<tIndex> wRebasedParent;
                if (m_DoesRow) {
                    wRebasedParent = sRebasePlan.RebaseRow(wParent);
                } else {
                    wRebasedParent = sRebasePlan.RebaseCol(wParent);
                }
                if (!wRebasedParent.has_value()) {
                    // Parent was deleted - set to -1
                    wSaveColRow->Parent(-1);
                } else {
                    wSaveColRow->Parent(wRebasedParent.value());
                }
            }
            
            // Rebase children indices - remove deleted ones
            vector<tIndex>& wChildren = wSaveColRow->m_Children;
            vector<tIndex> wValidChildren;
            for (auto wChildIndex : wChildren) {
                std::optional<tIndex> wRebasedChild;
                if (m_DoesRow) {
                    wRebasedChild = sRebasePlan.RebaseRow(wChildIndex);
                } else {
                    wRebasedChild = sRebasePlan.RebaseCol(wChildIndex);
                }
                if (wRebasedChild.has_value()) {
                    // Child still valid
                    wValidChildren.push_back(wRebasedChild.value());
                }
                // If rebase fails, child was deleted - don't add to valid children
            }
            wChildren = wValidChildren;
        }
        
        return true;
    }
            
	//=========================================================================
	tSaveCoveredRange::tSaveCoveredRange() : tClass(), m_ExternalCovered(nullptr){}
	tSaveCoveredRange::~tSaveCoveredRange() {
		m_VectorSaveRange.clear();
	}

	void tSaveCoveredRange::PushSaveRange(tSaveRange * sSaveRange) { m_VectorSaveRange.push_back(sSaveRange); }

	tVectorSaveRange* tSaveCoveredRange::VectorSaveRange() { return(&m_VectorSaveRange); }

	tSaveRange* tSaveCoveredRange::LastSaveRange() {
		if (m_VectorSaveRange.empty()) return(nullptr);
		return(m_VectorSaveRange.back());
	};

	tSaveRange* tSaveCoveredRange::ExternalCovered() {
		return(m_ExternalCovered);
	}

	void tSaveCoveredRange::ExternalCovered(tSaveRange* sSaveRange) { m_ExternalCovered = sSaveRange; };
        
    void tSaveCoveredRange::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        sWriter->Key(kJsonKeyRangeArray);
        sWriter->StartArray();
        for (auto wSaveRange : m_VectorSaveRange) wSaveRange->Json(sWriter,sSheet,true);
        sWriter->EndArray();
    }
        
    void tSaveCoveredRange::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        if (sValue.HasMember(kJsonKeyRangeArray)) {
            const rapidjson::Value& wSaveRangeValue = sValue[kJsonKeyRangeArray];
            for (auto& wRangeValue : wSaveRangeValue.GetArray()) {
                tSaveRange* wSaveRange = new tSaveRange();
                wSaveRange->Json(wRangeValue,sSheet);
                m_VectorSaveRange.push_back(wSaveRange);
            }
        }
    }
    
    // Rebase ===================================================================
    tBool tSaveCoveredRange::Rebase(const tRebasePlan& sRebasePlan) {
        // Rebase all ranges in the vector
        for (auto wSaveRange : m_VectorSaveRange) {
            if (!wSaveRange->Rebase(sRebasePlan)) {
                // Range was deleted or became invalid
                return false;
            }
        }
        
        // Rebase external covered range if present
        if (m_ExternalCovered != nullptr) {
            if (!m_ExternalCovered->Rebase(sRebasePlan)) {
                // External covered range was deleted
                return false;
            }
        }
        
        return true;
    }
       
#ifdef _DEBUGSK
	tString tSaveCoveredRange::Debug() {
        tStringStream wStream;
        wStream << "tSaveCoveredRange ";
        for (auto wSaveRange : m_VectorSaveRange) wStream << wSaveRange->Debug() << endl;
		if (m_ExternalCovered != nullptr) {
			wStream << " ExternalCovered ";
			wStream << m_ExternalCovered->Debug();
        }
        
        return(wStream.str());
	}
#endif
            
	//=========================================================================
	tSaveSelectErase::tSaveSelectErase() : tSaveSelectColRow(true), m_IsRect(false){
	}

	tSaveSelectErase::~tSaveSelectErase() {
		Clear();
	}

	void tSaveSelectErase::Clear() {
		tSaveSelectColRow::Clear();
        
		// Owner
        for (auto wSaveRange : m_VectorSaveRangeCoordAfter) delete(wSaveRange);
        m_VectorSaveRangeCoordAfter.clear();
        
        // Owner
        for (auto wSaveRange : m_VectorSaveCoveredRange) delete(wSaveRange);
		m_VectorSaveCoveredRange.clear();

		m_VectorUniqueRange.Container()->clear();
        
        // Owner
        for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) delete(wSaveConditionalFormat);
        m_VectorSaveConditionalFormat.clear();
        
		m_VectorNamedMergedDataConditionalRange.clear();
     
		m_VectorCellCalculate.clear();
	}
            
    void tSaveSelectErase::ClearAfterDo() {
        // Delete vector not used by undo
        m_VectorUniqueRange.Container()->clear();
        m_VectorCellCalculate.clear();
    }
		
    void tSaveSelectErase::RectDeleteArea(tRect sRect) {
        m_RectDeleteArea = sRect;
        // Very important for Calculate
        m_SelectStr=m_RectDeleteArea.StrRef();
    }

    void tSaveSelectErase::EnsureRectDeleteAreaFromSheet() {
        if (m_ColRowCellRange == nullptr) {
            return;
        }
        tRect wRect = m_RectDeleteArea;
        if (wRect.Bottom() >= wRect.Top() && wRect.Right() >= wRect.Left()
            && (wRect.Bottom() > 0 || wRect.Right() > 0)) {
            return;
        }
        RectDeleteArea(tRect(0, 0, m_ColRowCellRange->LastRow(), m_ColRowCellRange->LastCol()));
    }

	tRect tSaveSelectErase::RectDeleteArea() { return(m_RectDeleteArea); }

	tTempoRect* tSaveSelectErase::TempoRectDeleteArea() {
		return(new tTempoRect(m_RectDeleteArea.Top(), 
			m_RectDeleteArea.Left(), 
			m_RectDeleteArea.Bottom(), 
			m_RectDeleteArea.Right()));
	}


tBool tSaveSelectErase::AddUniqueRange(tRange* sRange) {
        tBool wOk;
#ifdef debugundoredo
        cout << "tSaveSelectErase::AddUniqueRange " << sRange->StrRef();
#endif
		wOk=m_VectorUniqueRange.InsertClass(sRange);
#ifdef debugundoredo
    if (wOk) {
        cout << " ok" << endl;
    } else {
        cout << " already exist !" << endl;
    }
#endif
        return(wOk);
	}

    void tSaveSelectErase::DetachRangeAfterFromColRow() {
#ifdef debugundoredo
        cout << "tSaveSelectErase::DetachRangeFromColRow()" << endl;
#endif
        for(auto wSaveRange : m_VectorNamedMergedDataConditionalRange) {
            if (!wSaveRange->Deleted()) {
#ifdef debugundoredo
                cout << " Detach->" << wSaveRange->Debug() << endl;
#endif
                tTempoRect wRect=wSaveRange->CoordAfter();
                tRange* wRange=m_ColRowCellRange->Range(wRect.Row(), wRect.Col(), wRect.Bottom(), wRect.Right());
                // if wSaveRange with Same Coord After already Deleted 
                if (wRange!=nullptr) {
                    // Save Range
                    m_ColRowCellRange->DetachRangeFromColRow(wRange,false);
                    wSaveRange->Range(wRange);
                }
            }
        }
    }
    
            
    void tSaveSelectErase::AttachRangeToColRow() {
#ifdef debugundoredo
        cout << "tSaveSelectErase::AttcahRangeToColRow()" << endl;
#endif
        for(auto wSaveRange : m_VectorNamedMergedDataConditionalRange) {
#ifdef debugundoredo
                cout << " Attach->" << wSaveRange->Debug();
#endif
            if (!wSaveRange->Deleted()) {
                tRange* wRange=wSaveRange->Range(); //  m_ColRowCellRange->Range(wSaveRange->Row(), wSaveRange->Col(), wSaveRange->Bottom(), wSaveRange->Right());
                if (wRange!=nullptr) {
                    // CF is restored via scf Undo, not mr (same model as tUndoPaste)
                    if (wRange->IsConditionalFormat()) {
                        wRange->RemoveConditionalFormat();
                    }
                m_ColRowCellRange->SetRange(wRange, wSaveRange->Row(), wSaveRange->Col(), wSaveRange->Bottom(), wSaveRange->Right());
                   
                }
            }
        }
    }
	void tSaveSelectErase::RemovesRangesAndProcessesOverlays(tIndex sIndiceAllocatorCellRange) {

		// Delete range and get recover =============================================
		tSaveRange* wLastSaveRange = nullptr;
		tSaveCoveredRange* wLastSaveCoveredRange = nullptr;
#ifdef debugundoredo
		cout << "Delete range and get recover " << endl;
#endif

		for (tSaveRange* wSaveRange : m_VectorSaveRangeCoordAfter) {
#ifdef debugundoredo
			cout << "m_VectorSaveRangeCoordAfter ";
			cout << wSaveRange->Debug() << endl;
#endif
			// Deleted work  ==================================================
			if (wSaveRange->Deleted()) {
				tWorkBook* wWorkBook= wSaveRange->ColRowCellRange()->Sheet()->WorkBook(); 

				// Modify formula an cell use this range =====================
				tVectorSaveFormulaCell* wVectorSaveFormulaCell = wSaveRange->VectorFormulaCellDepend();
				for (auto wSaveFormulaCell : *wVectorSaveFormulaCell) {
#ifdef debugundoredo
					cout << "Modify Vector Ref";
					cout << wSaveFormulaCell.Debug();
					cout << endl;
#endif
					tCell* wCell = wSaveFormulaCell.Cell();

#ifdef checksp
					if (wSaveFormulaCell.Index() >= wCell->VectorRef()->size()) {
						tStringStream wStream;
						wStream << "throw: Check tSavecell::RemovesRangesAndProcessesOverlays on range " << wSaveRange->StrRef() << " dependent cell " << ((tItem*)wCell)->StrRef() << " Index formula " << wSaveFormulaCell.Index() << " > " << wCell->VectorRef()->size() << " wCell->VectorRef()->size !";
                        cerr << wStream.str() << endl;
						throw(tExceptionInternalError(wStream.str()));
					}
#endif
					// Is RangeNamed (Recup Name Range 
					tString wName = wSaveRange->Name();
					if (wName != "") {
#ifdef debugundoredo
						cout << "WorkBoook  AddNotUsedRangeNamed(" << wName << "," << ((tItem*)wCell)->StrRef() << ")" << endl;;
#endif
 
						PushCalculate(wCell);
					}

					// Modify reference formula (m_VectorRef)...
                    // set reference to null
					(*wCell->VectorRef())[wSaveFormulaCell.Index()] = nullptr;
                    // Erase Dependent Cell of Range
					wSaveRange->Range()->DeleteDependent(wCell);
				}
                
                
				// Is RangeNamed idem Is RangeData
                if (wSaveRange->IsNamed()) {
                    tString wName = wSaveRange->Name();
                   
#ifdef debugundoredo
                    cout << "DeleteRange(" << wSaveRange->Range()->Sheet()->Name() << "!" << wSaveRange->Range()->StrRef() << ")" << endl;;
#endif

#ifdef debugundoredo
					cout << " Name " << wName;
#endif
					if (!wWorkBook->DeleteRangeNamed(wName)) {
						tStringStream wStream;
						wStream << "throw: tSaveSelectErase::DeleteRangeNamed" << wName << " don't exist !";
                        cerr << wStream.str();
						throw  (new tExceptionInternalError(wStream.str()));
					}
                }
                    
                // Treats Conditionnl Format / merge removal ====================
                ProcessDeletedRangeRemoval(wSaveRange);
        
                
#ifdef debugundoredo
                cout << wSaveRange->Debug();
#endif
			} else {
                // Else not deleted ===========================================
                // 
				// Search recover =============================================
				tBool wIsInLastCovered = false;
				if (wLastSaveRange != nullptr) {
                    // Same Coordinate --> Recover
					if (wSaveRange->CoordAfter() == wLastSaveRange->CoordAfter()) {
						if (wLastSaveCoveredRange != nullptr) {
							if (wLastSaveCoveredRange->LastSaveRange()->CoordAfter() == wSaveRange->CoordAfter()) {
                                // Ok covered
								wIsInLastCovered = true;
							}
						}
						// If necessary allocate the first SkSaveCoveredRange
						if (!wIsInLastCovered) {
							wLastSaveCoveredRange = new tSaveCoveredRange();
							m_VectorSaveCoveredRange.push_back(wLastSaveCoveredRange);
							// Push last save range
							wLastSaveCoveredRange->PushSaveRange(wLastSaveRange);
						}
						// Push first current
						wLastSaveCoveredRange->PushSaveRange(wSaveRange);
					}
					else {
						wLastSaveCoveredRange = nullptr;
					}
				}
				wLastSaveRange = wSaveRange;
			}
		}
#ifdef debugundoredo
		cout << "Search range outside the deletion zone." << endl;
#endif
        ProcessConditionalFormatsInDeleteArea();
       
		// If recoved range search If the range exist outside the deletion zone.
		for (tSaveCoveredRange* wSaveCoveredRange : m_VectorSaveCoveredRange) {
			tSaveRange* wSaveRange = wSaveCoveredRange->LastSaveRange();
			tTempoRect wRect = wSaveRange->CoordAfter();

            tRange* wRange=m_ColRowCellRange->Range(wSaveRange->AllocatorRef());
			
			if (wRange != nullptr) {
				tSaveRange* wSaveRange = AddRange(wRange);
				wSaveRange->CoordAfter(wRect);
				wSaveRange->Deleted(false);
				wSaveRange->ExternalCovered(true);

				// Save Reference in cell dependent +
				wSaveCoveredRange->ExternalCovered(wSaveRange);
				wSaveCoveredRange->ExternalCovered()->SetRefAndDepend(this, wRange);
				// Loop on Cell dependent =====================================
				for (tItem* wItem : *wRange->ContainerCellDepend()->Container()) {
						tCell* wCell = wItem->Cell();
						if (wCell != nullptr) {
							tIndex wIndex = 0;
							for (auto wFormula : *wCell->VectorRef()) {
								if (wFormula == wRange) {
									// Is Attribute
									tString wAttribute = "";
									if (wCell->Type() == tTypeItem::t_Attribute) {
										tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCell);
										wAttribute = wCellAttribute->Name();
									}
									tSaveFormulaCell wSaveFormulaCell(tIndex(wCell->ColRowCellRangeRef()), 
										wCell->Type(),
										wCell->RowIndex(), 
										wCell->ColIndex(), 
										wAttribute,
										wIndex);

									wSaveRange->PushFormulaCell(wSaveFormulaCell);
									if (wSaveRange->ColRowCellRangeRef() != sIndiceAllocatorCellRange) {
										m_VectorCalculateExternalSaveCell.push_back(AddCell(wCell));
									}
								}
								wIndex++;
							}
						}
				}
#ifdef debugundoredo
				cout << "External Covered " << wRange->StrRef() << " after " << wSaveRange->StrRefAfter() << " " << wRange->AllocatorRef() << endl;
#endif
			}
		}
#ifdef debugundoredo
        cout << "merge all range." << endl;
#endif
         
		// Finally merge all range on first range and delete the other range
		for (auto wSaveCoveredRange : m_VectorSaveCoveredRange) {
			tSaveRange* wRootCovered = wSaveCoveredRange->ExternalCovered();
#ifdef debugundoredo
			if (wRootCovered != nullptr) {
				cout << "First Recover " << wRootCovered->StrRef() << " after :" << wRootCovered->StrRefAfter() << " " << wRootCovered->AllocatorRef() << endl;
			}
#endif
            for (auto wSaveRange : *wSaveCoveredRange->VectorSaveRange()) {
#ifdef debugundoredo
                tRange* wRange = wSaveRange->Range();
                cout << "Merge Delete " << wRange->StrRef() << " after " << wSaveRange->StrRefAfter() << ":" << wRange->AllocatorRef() << endl;
#endif
                for (auto wItem : *wSaveRange->VectorDepend()) {
                    if (wItem != nullptr) {
                        tSaveRange* wSaveRangeTest = dynamic_cast<tSaveRange*>(wItem);
                        if (wSaveRangeTest != nullptr) {
                            tStringStream wStream;
                            wStream << "Range in vector depend ";
                            cerr << wStream.str() << endl;
                            throw(tExceptionInternalError(wStream.str()));
                        }
                        else {
                            if (wRootCovered == nullptr) {
                                wRootCovered = wSaveRange;
#ifdef debugundoredo
                                cout << "First Recover " << wRootCovered->StrRef() << " after :" << wRootCovered->StrRefAfter() << " " << wRootCovered->AllocatorRef() << endl;
#endif
                            }
                            else {
#ifdef debugundoredo
                                cout << "... Recover " <<  wRootCovered->StrRef() <<" on cell " << wItem->StrRef() << endl;
#endif
                                tRange* wExternalRangeCovered = wRootCovered->Range();
                                // Report on the root range named & Merged
                                if (wExternalRangeCovered != nullptr) {
                                    if (wExternalRangeCovered->IsNamed()) {
                                        wRootCovered->SetNamed();
                                    }
                                    if (wExternalRangeCovered->IsMerged()
                                        && !SaveRangeHasConditionalFormatOverlay(wRootCovered)) {
                                        wRootCovered->SetMerged();
                                    }
                                    if (wExternalRangeCovered->IsData()) {
                                        wRootCovered->SetData();
                                    }
                                
                                    tSaveCell* wSaveCell = dynamic_cast<tSaveCell*>(wItem);
                                    tCell* wCell = wSaveCell->Cell();
                                    // Place Reference on Root
                                    tVectorSaveFormulaCell* wVectorSaveFormulaCell = wSaveRange->VectorFormulaCellDepend();
                                    for (auto wSaveFormulaCell : *wVectorSaveFormulaCell) {
                                        tCell* wCell = wSaveFormulaCell.Cell();
                                        (*wCell->VectorRef())[wSaveFormulaCell.Index()] = wExternalRangeCovered;
                                    }
                                    
                                    // Change dependant on the covered range
                                    wExternalRangeCovered->AddDependent(wCell);
                                } else {
                                    tStringStream wStream;
                                    wStream << "throw: tSaveSelectErase::DeleteColRow wExternalRangeCovered Range "<< wRootCovered->StrRef() << " after " << wRootCovered->StrRefAfter() << " don't exist !";
                                    cerr << wStream.str() << endl;
                                    throw(tExceptionInternalError(wStream.str()));
                                }
                            }
                        }
                    }
                } // end of loop of dependent Cell
                // Erase Range if not Root Covered ============================
                if (wRootCovered!=nullptr) {
                    if (wSaveRange->AllocatorRef() != wRootCovered->AllocatorRef()) {
                        // Erase range of list (threat RangeNamed & RangeData) 
                        if (wSaveRange->Range()->Sheet() == m_ColRowCellRange->Sheet()) {
                            tRange* wRange = m_ColRowCellRange->Range(wSaveRange->AllocatorRef());
                            wRange->ClearDependent();
                            DeleteRemovedRangeClean(wSaveRange->AllocatorRef());
                        } else {
                            tStringStream wStream;
                            wStream << "throw: tSaveSelectErase::DeleteColRow Range not in good sheet ! ";
                            wStream << wSaveRange->Range()->Sheet() << "!=" << m_ColRowCellRange->Sheet() << endl;
                            cerr << wStream.str() << endl;
                            throw(tExceptionInternalError(wStream.str()));
                        }
                        wSaveRange->Deleted(true);
                    }
                }
			} // end of loop of Range
		} // En of of looop Range Recoverd
	}
                

	void tSaveSelectErase::DeleteColRow(tBool sDoesRow, tIndex sIndiceAllocatorCellRange, tIndex sPosition, tIndex sSize,tBool sIsRect) {
		m_ColRowCellRange = tStaticColRowCellRange::Instance()->ColRowCellRange(sIndiceAllocatorCellRange);
		m_DoesRow = sDoesRow;
        m_IsRect=sIsRect;

#ifdef debugundoredo
		if (sDoesRow) {
			cout << "Delete row ";
		}
		else {
			cout << "Delete col ";
		}
		cout << Base10ToAlpha(m_RectDeleteArea.Left()) << m_RectDeleteArea.Top() << ":" << Base10ToAlpha(m_RectDeleteArea.Right()) << m_RectDeleteArea.Bottom() << endl;
#endif
        // Pass Save Cell ======================================================
		for (tIndex wRow = m_RectDeleteArea.Top(); wRow <= m_RectDeleteArea.Bottom(); wRow++) {
			for (tIndex wCol = m_RectDeleteArea.Left(); wCol <= m_RectDeleteArea.Right(); wCol++) {
				tCell* wCell = m_ColRowCellRange->Cell(wRow, wCol);
				if (wCell != nullptr) {
                    if (IsUndoActif()) {
#ifdef debugundoredo
                        cout << "Save Cell  " << wCell->StrRef() << endl;
#endif
                        // Add Cell
                        tSaveCell* wSaveCell=AddCell(wCell);
                       
                       
                        // Pass Format to save
                        wSaveCell->PassCss(wCell);
                        
                        // Savee JsonPayload
                        // Add JsonPayload
                        tTempoPoint wTempoPoint(wRow,wCol);
                        
                        AddJsonPayload(m_ColRowCellRange,&wTempoPoint);
                    }
                    
					// Search external dependent ==============================
					TreatsExternalDependencyCells(wCell);
                    // Add Attributes of class
                    if (wCell->ClassAttribute()!=nullptr) {
                        tCellClassAttribute* wClass=wCell->ClassAttribute();
                        tVectorCellAttribute wVectorCellAttribute;
                        wClass->CellAttribute(&wVectorCellAttribute);
                        for(auto wCellAttribute : wVectorCellAttribute) {
#ifdef debugundoredo
                            cout << "Save Cell attribute " << wCellAttribute->StrRef() << endl;
#endif
                            /*tSaveCell* wSaveCellAttribute=*/ AddCell(wCellAttribute);
                            TreatsExternalDependencyCells(wCellAttribute);
                        }
                        
                    }
				}
			}
		}


		// loop and all range for create tRangeSave with good coordinate ======
		for (auto wRange : *m_VectorUniqueRange.Container()) {

#ifdef debugundoredo
			cout << "m_VectorUniqueRange Range " << wRange->StrRef() << " " << wRange->AllocatorRef() << endl;
#endif
			// 2 Sauvegarde les ranges 
			tIndex wTop = wRange->TopIndex();
			tIndex wLeft = wRange->LeftIndex();
			tIndex wBottom = wRange->BottomIndex();
			tIndex wRight = wRange->RightIndex();
            // this Save Range go in m_VectorSaveRangeCoordAfter order Coord after
			tSaveRange* wSaveRange = new tSaveRange(wRange);
            // Set AllValue
			wSaveRange->SetRefAndDepend(this, wRange);
			// Save formula references of range dependent cells 
			// Warning : Used by undo and Covered Range
			for (auto wItem : *wRange->ContainerCellDepend()->Container()) {
				tCell* wCell = wItem->Cell();

				if (wCell != nullptr) {
#ifdef debugundoredo
                cout << " Depend Cell " << wCell->StrRef() << "=" << wCell->FormulaStr() << " : " << wCell->Value() << endl;
#endif

                tIndex wIndex = 0;
                for (auto wFormula : *wCell->VectorRef()) {
                    if (wFormula == wRange) {
                            // Is Attribute
                            tString wAttribute = "";
                            if (wCell->Type() == tTypeItem::t_Attribute) {
                                tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCell);
                                wAttribute = wCellAttribute->Name();
                            }
                            
                            tSaveFormulaCell wSaveFormulaCell(tIndex(wCell->ColRowCellRangeRef()),
                                                              wCell->Type(),
                                                              wCell->RowIndex(),
                                                              wCell->ColIndex(),
                                                              wAttribute,
                                                              wIndex);
                            
                            wSaveRange->PushFormulaCell(wSaveFormulaCell);
                            //// Is not same Sheet Calculate After
                            if (wCell->ColRowCellRangeRef() != sIndiceAllocatorCellRange) {
                                    // Only External Cell =====================
                                    if (ExternalCell(wCell)) {
                                        m_VectorCalculateExternalSaveCell.push_back(AddCell(wCell));
                                    }
                                } else {
                                    // Only External Cell =====================
                                    if (ExternalCell(wCell)) {
#ifdef debugundoredo
                                        cout << " External : << Add Cell ";
#endif
                                        PushCalculate(wCell);
                                    }
                                }
                            }
                            wIndex++;
                        }
                        
				}
			}
            tBool wIsInRect = ComputeRangeFullyInsideDeleteArea(sIsRect, wRange, wTop, wLeft, wBottom, wRight);

            // A delete-by-rect only removes the cells inside the rect and shifts the cells on
            // the same band (rows for a row shift, columns for a column shift). A range that
            // extends past the rect on the axis perpendicular to the shift cannot be uniformly
            // shifted, so it must be preserved as-is instead of being resized or dropped. This
            // is a single geometric rule that applies to every range kind (formula, named,
            // merged, conditional format): e.g. a single-cell rect delete (E6:E6) keeps a range
            // on B4:I28 at B4:I28, whether it is a conditional format or a SUM reference.
            tBool wPreserveRangeAcrossRect = false;
            if (sIsRect) {
                if (m_DoesRow) {
                    wPreserveRangeAcrossRect = (wLeft < m_RectDeleteArea.Left() || wRight > m_RectDeleteArea.Right());
                } else {
                    wPreserveRangeAcrossRect = (wTop < m_RectDeleteArea.Top() || wBottom > m_RectDeleteArea.Bottom());
                }
            }

            if (wPreserveRangeAcrossRect) {
                tTempoRect wRect(wTop, wLeft, wBottom, wRight);
                wSaveRange->CoordAfter(wRect);
            } else {
                if (wIsInRect) {
                     wSaveRange->Deleted(true);
                } else {
                    ApplyRangeCoordAfterDelete(sPosition, sSize, wTop, wLeft, wBottom, wRight);
                }

                if (IsRangeCoordInvalidAfterDelete(wTop, wLeft, wBottom, wRight)) {
                    wSaveRange->Deleted(true);
                } else if (!wIsInRect) {
                    tTempoRect wRect(wTop, wLeft, wBottom, wRight);
                    wSaveRange->CoordAfter(wRect);
                }
            }

#ifdef debugundoredo
            cout << "After " << wSaveRange->Debug() << endl;;
#endif
            // Insert Merged, Named and Data ranges (CF is restored via scf, like tUndoPaste cells-only model)
            if ((wSaveRange->IsMerged()) || (wSaveRange->IsNamed()) || (wSaveRange->IsData())) {
                    m_VectorNamedMergedDataConditionalRange.push_back(wSaveRange);
            }
            
            // Insert with after coord order for detect covered range
			tVectorSaveRange::iterator wWhere;
            wWhere = std::lower_bound(m_VectorSaveRangeCoordAfter.begin(), m_VectorSaveRangeCoordAfter.end(), wSaveRange, tComparatorSaveRangeCoordAfter());
			m_VectorSaveRangeCoordAfter.insert(wWhere, wSaveRange);
		}
		// Arrange all ranges .. Delete and overlay ranges
		RemovesRangesAndProcessesOverlays(sIndiceAllocatorCellRange);
        for (tIndex wRow = m_RectDeleteArea.Top(); wRow <= m_RectDeleteArea.Bottom(); wRow++) {
            for (tIndex wCol = m_RectDeleteArea.Left(); wCol <= m_RectDeleteArea.Right(); wCol++) {
                tCell* wCell = m_ColRowCellRange->Cell(wRow, wCol);
                if (wCell != nullptr) {
                    m_ColRowCellRange->DeleteCell(wRow, wCol);
                }
            }
        }
#ifdef debugundoredo
        cout << "End DeleteColRow()" << endl;
#endif
 	}

        
    void tSaveSelectErase::UndoNamedMergedDataConditionalRange() {
        if (m_ColRowCellRange == nullptr) {
            return;
        }
        // JSON path: restore deleted merged ranges and tables (AttachRangeToColRow skips del:true), then scf.
        if (m_UndoSpreadSheet != nullptr && m_UndoSpreadSheet->IsJson()) {
            tWorkBook* wWorkBook = m_ColRowCellRange->Sheet()->WorkBook();
            for (auto wSaveRange : m_VectorNamedMergedDataConditionalRange) {
                if (!wSaveRange->Deleted()) {
                    continue;
                }
                tSheet* wTargetSheet = wSaveRange->Sheet();
                if (wTargetSheet == nullptr) {
                    wTargetSheet = m_ColRowCellRange->Sheet();
                }
                if (wTargetSheet == nullptr) {
                    continue;
                }

                if (wSaveRange->IsNamed()) {
                    if (wSaveRange->IsData()) {
                        tRangeData* wRangeData = wSaveRange->RangeData();
                        if (wRangeData != nullptr && wWorkBook != nullptr) {
                            const tString wName = wSaveRange->Name();
                            if (!wName.empty()) {
                                wWorkBook->DeleteRangeNamed(wName);
                            }
                            tRange* wRestored = wWorkBook->InsertRangeData(wName,
                                                                           *wRangeData,
                                                                           wSaveRange->Rect(),
                                                                           wTargetSheet);
                            if (wRestored != nullptr) {
                                wSaveRange->Range(wRestored);
                                wRestored->SetNamed();
                                wRestored->SetData();
                            }
                        }
                    } else if (wWorkBook != nullptr) {
                        tRange* wNamedRange = wWorkBook->InsertRangeNamed(wSaveRange->Name(),
                                                                          wSaveRange->Rect(),
                                                                          wTargetSheet);
                        if (wNamedRange != nullptr) {
                            wSaveRange->Range(wNamedRange);
                            wNamedRange->SetNamed();
                        }
                    }
                } else {
                    tRange* wRange = m_ColRowCellRange->EnsureRange(
                        wSaveRange->Row(), wSaveRange->Col(), wSaveRange->Bottom(), wSaveRange->Right());
                    if (wRange == nullptr) {
                        continue;
                    }
                    wSaveRange->Range(wRange);
                    if (wSaveRange->IsMerged()) {
                        wRange->SetMerged();
                    }
                    if (wSaveRange->IsData()) {
                        wRange->SetData();
                    }
                }
            }
            for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) {
                wSaveConditionalFormat->Undo(m_ColRowCellRange);
            }
            m_ColRowCellRange->StripOrphanConditionalFormatExtension();
            m_ColRowCellRange->StripMergedOnConditionalFormatRanges();
            return;
        }
        tWorkBook* wWorkBook = m_ColRowCellRange->Sheet()->WorkBook();
        
#ifdef debugundoredo
            cout << "tSaveSelectErase::UndoNamedMergedDataConditionalRange() " << endl;
#endif
        // Replace Named & Merged  Range
        for (auto wSaveRange : m_VectorNamedMergedDataConditionalRange) {
#ifdef debugundoredo
            cout << "Replace Range " << wSaveRange->Debug() << endl;
#endif
            tSheet* wTargetSheet = wSaveRange->Sheet();
            if (wTargetSheet == nullptr) {
                wTargetSheet = m_ColRowCellRange->Sheet();
            }
            if (wTargetSheet == nullptr) {
                continue;
            }

            tRange* wRange = m_ColRowCellRange->EnsureRange(wSaveRange->Row(), wSaveRange->Col(), wSaveRange->Bottom(), wSaveRange->Right());
            if (wRange == nullptr) {
                continue;
            }
            wSaveRange->Range(wRange);
            
            // Named
            if (wSaveRange->IsNamed()) {
                if (wSaveRange->IsData()) {
                    wRange->WorkBook()->InsertRangeData(wSaveRange->Name(),
                                                        wSaveRange->RangeData(),
                                                        wSaveRange->Rect(),
                                                        wTargetSheet);
                } else {
                    wRange=wWorkBook->InsertRangeNamed(wSaveRange->Name(),
                                                       wSaveRange->Rect(),
                                                       wTargetSheet);
                }
                if (wRange != nullptr) {
                    wSaveRange->Range(wRange);
                    wRange->SetNamed();
                }
            }
         
            // Merged
            if (wSaveRange->IsMerged() && !SaveRangeHasConditionalFormatOverlay(wSaveRange)) {
                wRange->SetMerged();
            }

            // Data
            if (wSaveRange->IsData()) wRange->SetData();
        }

        // CF source of truth: scf list (not mr extension flags)
        for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) {
            wSaveConditionalFormat->Undo(m_ColRowCellRange);
        }
        m_ColRowCellRange->StripOrphanConditionalFormatExtension();
        m_ColRowCellRange->StripMergedOnConditionalFormatRanges();
    }
	
	void tSaveSelectErase::UndoDeleteColRow() {
        // GetMessage rebuilds SaveSelectErase from JSON; ColRowCellRange is not set by DeleteColRow.
        if (m_ColRowCellRange == nullptr && m_UndoSpreadSheet != nullptr) {
            if (tSheet* wSheet = m_UndoSpreadSheet->Sheet()) {
                m_ColRowCellRange = wSheet->ColRowCellRange();
            }
        }
#ifdef debugundoredo
        cout << "UndoDeleteRow " << m_ColRowCellRange->Sheet()->Name() << ":" << m_ColRowCellRange->Sheet() << endl;
		cout << Debug();
#endif
        // Replace old Named Merged Ranges ====================================
        UndoNamedMergedDataConditionalRange();
        
        // Replace old cells ==================================================
        UndoCells();
        // Collab JSON undo: recalc cells whose formulas were saved in fcell during delete.
        PushSavedFormulaDependentsCalculate();
	}

	void tSaveSelectErase::PushRangeDependCalculate(tColRowCellRange* sColRowCellRange, tRect sRect) {
		// After restoring rows/cols, find all cells that depend on ranges referencing the restored area
		// For each restored row/col, find ranges that reference it, then find cells that depend on those ranges
		// This ensures cells like A7 with SUM(A1:A6) are recalculated when rows 2-4 are restored
		for (tIndex wRow = sRect.Top(); wRow <= sRect.Bottom(); wRow++) {
			tColRow* wColRow = sColRowCellRange->Row(wRow);
			if (wColRow != nullptr) {
				tColRow::tContainerRange* wContainerRange = wColRow->ContainerRange();
				if (wContainerRange != nullptr) {
					for (auto wItemRange : *wContainerRange->Container()) {
						tRange* wRange = sColRowCellRange->Range(wItemRange);
						if (wRange != nullptr) {
							tItem::tContainerCell* wVectorDependent = wRange->ContainerCellDepend();
							if (wVectorDependent != nullptr) {
								for (auto wCellDepend : (*wVectorDependent->Container())) {
									tCell* wCell = wCellDepend->Cell();
									if (wCell != nullptr) {
										// Add cell to calculation list
										PushCalculate(wCell);
									}
								}
							}
						}
					}
				}
			}
		}
		for (tIndex wCol = sRect.Left(); wCol <= sRect.Right(); wCol++) {
			tColRow* wColRow = sColRowCellRange->Col(wCol);
			if (wColRow != nullptr) {
				tColRow::tContainerRange* wContainerRange = wColRow->ContainerRange();
				if (wContainerRange != nullptr) {
					for (auto wItemRange : *wContainerRange->Container()) {
						tRange* wRange = sColRowCellRange->Range(wItemRange);
						if (wRange != nullptr) {
							tItem::tContainerCell* wVectorDependent = wRange->ContainerCellDepend();
							if (wVectorDependent != nullptr) {
								for (auto wCellDepend : (*wVectorDependent->Container())) {
									tCell* wCell = wCellDepend->Cell();
									if (wCell != nullptr) {
										PushCalculate(wCell);
									}
								}
							}
						}
					}
				}
			}
		}
	}
    
                
    tBool tSaveSelectErase::ExternalCell(tCell* sCell,tBool sEraseSheet) {
        tIndex wRow = sCell->RowIndex();
        tIndex wCol = sCell->ColIndex();
        
#ifdef debugundoredo
        cout << "tSaveSelectErase::ExternalCell(sCell:";
        cout << sCell->StrRef() << ") In " << m_RectDeleteArea.StrRef();
        if (sEraseSheet) cout << " erase sheet";
#endif
        
        tBool wIsExternal = true;
        tSheet* wSheet=m_ColRowCellRange->Sheet();
        // Delete Row or col
        if (!sEraseSheet) {
            if (!(sCell->Sheet()==wSheet)) wIsExternal=true;
            if (wIsExternal!=false) {
                wIsExternal = ((wRow < m_RectDeleteArea.Top()) || (wRow > m_RectDeleteArea.Bottom()) ||
                              ((wCol < m_RectDeleteArea.Left()) || (wCol > m_RectDeleteArea.Right())));
            }
        }
        else {
            // Other sheet --> delete Sheet
            wIsExternal = (sCell->Sheet() != wSheet);
        }
#ifdef debugundoredo
        if (wIsExternal) {
            cout << " External" << endl;
        } else {
            cout << " Not External" << endl;
        }
        cout << "------------------------------------------------" << endl;
#endif
        return(wIsExternal);
    }
            
	tBool tSaveSelectErase::ExternalCell(tCell* sCell, tCell* sCellDependent, tBool sEraseSheet) {
		tIndex wRow = sCellDependent->RowIndex();
		tIndex wCol = sCellDependent->ColIndex();
#ifdef debugundoredo
        cout << "tSaveSelectErase::ExternalCell(" << Base10ToAlpha(wCol)<<  wRow << "): In " << m_RectDeleteArea.StrRef();
        cout << "------------------------------------------------" << endl;
#endif
		tBool wIsExternal = false;
        
		// Delete Row or col
		if (!sEraseSheet) {
            if (m_IsRect) {
                wIsExternal = ((wRow < m_RectDeleteArea.Top()) || (wRow > m_RectDeleteArea.Bottom()) ||
                              ((wCol < m_RectDeleteArea.Left()) || (wCol > m_RectDeleteArea.Right())));
            } else {
                if (m_DoesRow) {
                    wIsExternal = ((wRow < m_RectDeleteArea.Top()) || (wRow > m_RectDeleteArea.Bottom()) || (sCell->Sheet() != sCellDependent->Sheet()));
                }
                else {
                    wIsExternal = ((wCol < m_RectDeleteArea.Left()) || (wCol > m_RectDeleteArea.Right()) || (sCell->Sheet() != sCellDependent->Sheet()));
                }
            }
		}
		else {
			// Other sheet --> delete Sheet
			wIsExternal = (sCell->Sheet() != sCellDependent->Sheet());
		}
#ifdef debugundoredo
        if (wIsExternal) {
            cout << " External" << endl;
        } else {
            cout << " Not External" << endl;
        }
#endif
        return(wIsExternal);
	}

    void tSaveSelectErase::DeleteRangeInColRowContainer() {
#ifdef debugundoredo
        cout << "tSaveSelectErase::DeleteRangeInColRowContainer() " << endl;
#endif
        for (auto wSaveRange : m_VectorSaveRangeCoordAfter) {
#ifdef debugundoredo
            cout << wSaveRange->Debug();
#endif
            if (!wSaveRange->Deleted()) {
                m_ColRowCellRange->DetachRangeFromColRow(wSaveRange->Range(),false);
            }
        }
    }


	void tSaveSelectErase::ApplyColRow() {
		for (auto wSaveRange : m_VectorSaveRangeCoordAfter) {
            // Deleted By Formula
            
            tBool wDeleted=m_ColRowCellRange->IsDeletedRange(wSaveRange->AllocatorRef());
            
			if ((!wSaveRange->Deleted()) && (!wDeleted)) {
				tTempoRect wRectAfter = wSaveRange->CoordAfter();
                tRange* wRange = wSaveRange->Range();
#ifdef debugundoredo
				cout << "ApplyColRow Before " << wSaveRange->Sheet()->Name() << "!" << wSaveRange->StrRef() << " Allocator " << wSaveRange->AllocatorRef() << " " << wSaveRange->Deleted();
				cout << " RangeAfter " << wRectAfter.StrRef() << endl;
#endif
				m_ColRowCellRange->SetRange(wRange, wRectAfter.Row(), wRectAfter.Col(), wRectAfter.Bottom(), wRectAfter.Right());
            }
		}
	}

    void tSaveSelectErase::ApplyColRowUndoInsertRect() {
        // Detach
        for (auto wSaveRange : m_VectorSaveRange) {
            m_ColRowCellRange->DetachRangeFromColRow(wSaveRange->Range(),false);
        }
        // Apply Range in new coordinate ============================================
        for (auto wSaveRange : m_VectorSaveRange) {
            tTempoRect wRectBefore = wSaveRange->Rect();
            tRange* wRange=wSaveRange->Range();
            m_ColRowCellRange->SetRange(wRange, wRectBefore.Row(), wRectBefore.Col(), wRectBefore.Bottom(), wRectBefore.Right());
        }
    }

	namespace {

	class tCallBackInvalidateRefsToDeletedSheet : public tSparseArrayCallBack<tAllocatorRef> {
		tColRowCellRange* m_ColRowCellRange;
		tSaveSelectErase* m_SaveSelectErase;
		tAllocatorRef m_DeletedSheetAlloc;

	public:
		tCallBackInvalidateRefsToDeletedSheet(tColRowCellRange* sColRowCellRange,
		                                      tSaveSelectErase* sSaveSelectErase,
		                                      tAllocatorRef sDeletedSheetAlloc)
			: m_ColRowCellRange(sColRowCellRange),
			  m_SaveSelectErase(sSaveSelectErase),
			  m_DeletedSheetAlloc(sDeletedSheetAlloc) {}

		tBool CallBack(tAllocatorRef sAllocatorRef) override {
			tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
			if (wCell == nullptr || wCell->VectorRef()->empty()) {
				return true;
			}
			tIndex wIndex = 0;
			for (auto wItem : *wCell->VectorRef()) {
				if (wItem != nullptr) {
					tBool wOnDeletedSheet = false;
					tCell* wRefCell = wItem->Cell();
					tRange* wRefRange = wItem->Range();
					if (wRefCell != nullptr && wRefCell->ColRowCellRangeRef() == m_DeletedSheetAlloc) {
						wOnDeletedSheet = true;
					} else if (wRefRange != nullptr && wRefRange->ColRowCellRangeRef() == m_DeletedSheetAlloc) {
						wOnDeletedSheet = true;
					}
					if (wOnDeletedSheet) {
						m_SaveSelectErase->PushCalculate(wCell);
						if (m_SaveSelectErase->IsUndoActif()) {
							tString wAttribute;
							if (wCell->Type() == tTypeItem::t_Attribute) {
								tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCell);
								wAttribute = wCellAttribute->Name();
							}
							tSaveFormulaCell wSaveFormulaCell(tIndex(wCell->ColRowCellRangeRef()),
							                                  wCell->Type(),
							                                  wCell->RowIndex(),
							                                  wCell->ColIndex(),
							                                  wAttribute,
							                                  wIndex);
							if (wRefRange != nullptr) {
								tSaveRange* wSaveRange = m_SaveSelectErase->AddRange(wRefRange);
								wSaveRange->PushFormulaCell(wSaveFormulaCell);
							} else if (wRefCell != nullptr) {
								tSaveCell* wSaveCell = m_SaveSelectErase->AddCell(wRefCell);
								wSaveCell->PushFormulaCell(wSaveFormulaCell);
							}
						}
						(*wCell->VectorRef())[wIndex] = nullptr;
						wItem->DeleteDependent(wCell);
					}
				}
				wIndex++;
			}
			return true;
		}
	};

	} // namespace

	void tSaveSelectErase::InvalidateRefsToDeletedSheet() {
		if (m_ColRowCellRange == nullptr) {
			return;
		}
		tSheet* wDeletedSheet = m_ColRowCellRange->Sheet();
		if (wDeletedSheet == nullptr) {
			return;
		}
		// Formula VectorRef uses ColRowCellRange::SheetAllocator, not Sheet::AllocatorRef().
		const tAllocatorRef wDeletedColRowAlloc = m_ColRowCellRange->SheetAllocator();
		const tAllocatorRef wDeletedSheetListAlloc = wDeletedSheet->AllocatorRef();
		tWorkBook* wWorkBook = wDeletedSheet->WorkBook();
		if (wWorkBook == nullptr) {
			return;
		}
		tVectorAllocatorRef* wVectorSheet = wWorkBook->VectorSheet();
		if (wVectorSheet == nullptr) {
			return;
		}
		const tVectorAllocatorRef wSheetsCopy = *wVectorSheet;
		for (tAllocatorRef wSheetAlloc : wSheetsCopy) {
			if (wSheetAlloc == wDeletedSheetListAlloc) {
				continue;
			}
			tSheet* wSheet = wWorkBook->SheetByAllocator(wSheetAlloc);
			if (wSheet == nullptr) {
				continue;
			}
			tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
			if (wColRowCellRange == nullptr) {
				continue;
			}
			tCallBackInvalidateRefsToDeletedSheet wCallBack(wColRowCellRange, this, wDeletedColRowAlloc);
			wColRowCellRange->CallBackAllCell(&wCallBack);
		}
	}

	void tSaveSelectErase::SaveAndRemoveSheetNamedRangesAndConditionalFormats() {
		if (m_ColRowCellRange == nullptr) {
			return;
		}
		tWorkBook* wWorkBook = m_ColRowCellRange->Sheet()->WorkBook();
		if (wWorkBook == nullptr) {
			return;
		}

		tVectorAllocatorRef wVectorOfRanges;
		wWorkBook->GetRangeNamedsBySheet(m_ColRowCellRange->Sheet()->AllocatorRef(), wVectorOfRanges);
		tVectorString wNamedToRemove;
		for (auto wRangeAllocatorRef : wVectorOfRanges) {
			tRange* wNamesRange = m_ColRowCellRange->Range(wRangeAllocatorRef);
			if (wNamesRange == nullptr) {
				continue;
			}
			tSaveRange* wSaveRange = new tSaveRange(wNamesRange);
			wSaveRange->SetNamed();
			m_VectorNamedMergedDataConditionalRange.push_back(wSaveRange);
			m_VectorSaveRangeCoordAfter.push_back(wSaveRange);
			if (wSaveRange->IsNamed()) {
				wNamedToRemove.push_back(wSaveRange->Name());
			}
		}
		for (const tString& wName : wNamedToRemove) {
			wWorkBook->DeleteRangeNamed(wName);
		}

		m_ColRowCellRange->CallBackAllRanges([this](tRange* sRange) {
			if (sRange == nullptr || !sRange->IsConditionalFormat()) {
				return;
			}
			tSaveRange* wSaveRange = new tSaveRange(sRange);
			m_VectorNamedMergedDataConditionalRange.push_back(wSaveRange);
			m_VectorSaveRangeCoordAfter.push_back(wSaveRange);
			SaveConditionalFormatsForRangeUndo(wSaveRange, sRange);
			ProcessDeletedRangeRemoval(wSaveRange);
		});
	}

	void tSaveSelectErase::DeleteSheet(tIndex sIndiceAllocatorCellRange) {
		m_ColRowCellRange = tStaticColRowCellRange::Instance()->ColRowCellRange(sIndiceAllocatorCellRange);

		InvalidateRefsToDeletedSheet();
		SaveAndRemoveSheetNamedRangesAndConditionalFormats();
		CalculateDo(m_ColRowCellRange, tVolatile::t_None);
	}

	void tSaveSelectErase::UndoDeleteSheet() {
        // GetMessage rebuilds SaveSelectErase from JSON; ColRowCellRange is not set by DeleteSheet.
        if (m_ColRowCellRange == nullptr && m_UndoSpreadSheet != nullptr) {
            if (tSheet* wSheet = m_UndoSpreadSheet->Sheet()) {
                m_ColRowCellRange = wSheet->ColRowCellRange();
            }
        }
		UndoNamedMergedDataConditionalRange();

		const tBool wIsJsonUndo =
			(m_UndoSpreadSheet != nullptr && m_UndoSpreadSheet->IsJson());

		if (wIsJsonUndo) {
			// Peer GetMessage: restore full formula text from svse JSON (l_c / l_r.d), then rewire deps.
			UndoCells();
			for (auto wSaveRange : m_VectorSaveRange) {
				for (auto wDepend : *wSaveRange->VectorDepend()) {
					if (wDepend != nullptr) {
						UndoCell(wDepend);
					}
				}
				if (!wSaveRange->VectorFormulaCellDepend()->empty()) {
					wSaveRange->RecoverFormulaCell(true);
				}
			}
		} else {
			for (auto wSaveCell : m_VectorSaveCell) {
				if (wSaveCell->VectorFormulaCellDepend()->empty()) {
					continue;
				}
				wSaveCell->RecoverFormulaCell(false);
			}
			for (auto wSaveRange : m_VectorSaveRange) {
				if (wSaveRange->VectorFormulaCellDepend()->empty()) {
					continue;
				}
				wSaveRange->RecoverFormulaCell(false);
			}
			UndoJsonPayload();
		}
        EnsureRectDeleteAreaFromSheet();
        // Recalc cross-sheet dependents (e.g. Calculs!* after undo delete-sheet on a peer).
        PushSavedFormulaDependentsCalculate();
        if (m_ColRowCellRange != nullptr) {
            PushRangeDependCalculate(m_ColRowCellRange, m_RectDeleteArea);
            CalculateDo(m_ColRowCellRange, tVolatile::t_None);
            RewireOutgoingFormulaDependents();
            // Calculate may touch CF ranges; heal orphan extension flags before Check().
            m_ColRowCellRange->StripOrphanConditionalFormatExtension();
            m_ColRowCellRange->StripMergedOnConditionalFormatRanges();
        }
	}

    tSaveConditionalFormat* tSaveSelectErase::GetOrCreateSaveConditionalFormat(tConditionalFormat* sConditionalFormat, tSaveRange* sSaveRange) {
       // Search
        for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) {
            if (wSaveConditionalFormat->ConditionalFormat() == sConditionalFormat) {;
                return(wSaveConditionalFormat);
            }
        }
        // Save with Key
        tSaveConditionalFormat* wSaveConditionFormat=new tSaveConditionalFormat(sConditionalFormat);
       
        m_VectorSaveConditionalFormat.push_back(wSaveConditionFormat);
        if (sSaveRange != nullptr) {
            sSaveRange->ClearConditionalFormatExtension();
        }
        return(wSaveConditionFormat);
    }

    tBool tSaveSelectErase::IsEmpty() {
        return(m_VectorSaveCell.empty() && m_VectorSaveRange.empty() && m_VectorSaveConditionalFormat.empty());
    }
    
    // Rebase ===================================================================
    tBool tSaveSelectErase::Rebase(const tRebasePlan& sRebasePlan) {
        // Debug: log rebase start for tSaveSelectErase
        // cout << "tSaveSelectErase::Rebase() - Cells=" << m_VectorSaveCell.size() 
        //      << " Ranges=" << m_VectorSaveRange.size() 
        //      << " ColRows=" << m_VectorSaveColRow.size() << endl;
        
        // Use m_DoesRow to indicate if we're working with rows or columns
        if (!tSaveSelectColRow::Rebase(sRebasePlan)) {
            return false;
        }
        
        // Rebase ranges in m_VectorSaveRangeCoordAfter
        for (auto wSaveRange : m_VectorSaveRangeCoordAfter) {
            if (!wSaveRange->Rebase(sRebasePlan)) {
                // Range was deleted or became invalid
                return false;
            }
        }
        
        // Rebase ranges in m_VectorNamedMergedDataConditionalRange
        for (auto wSaveRange : m_VectorNamedMergedDataConditionalRange) {
            if (!wSaveRange->Rebase(sRebasePlan)) {
                // Range was deleted or became invalid
                return false;
            }
        }
        
        // Rebase covered ranges
        for (auto wSaveCoveredRange : m_VectorSaveCoveredRange) {
            if (!wSaveCoveredRange->Rebase(sRebasePlan)) {
                // Covered range was deleted or became invalid
                return false;
            }
        }
        
        // Rebase RectDeleteArea
        tRect wRect = m_RectDeleteArea;
        auto wRebasedRect = sRebasePlan.RebaseRect(wRect);
        if (!wRebasedRect.has_value()) {
            // Rect was deleted or became invalid
            return false;
        }
        m_RectDeleteArea = wRebasedRect.value();
        
        return true;
    }
       
    void tSaveSelectErase::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        sWriter->StartObject();
        tSaveSelectColRow::Json(sWriter,sSheet);
        {
            tRect wRect = m_RectDeleteArea;
            if (wRect.Bottom() >= wRect.Top() && wRect.Right() >= wRect.Left()) {
                sWriter->Key(kJsonKeyRect);
                sWriter->String(m_RectDeleteArea.StrRef().c_str());
            }
        }
       
		sWriter->Key(kJsonKeyMergedRange);
		sWriter->StartArray();
		 for (auto wSaveNamedMergedRange : m_VectorNamedMergedDataConditionalRange) wSaveNamedMergedRange->Json(sWriter,sSheet,false);
		sWriter->EndArray();
        if (!m_VectorSaveConditionalFormat.empty()) {
            sWriter->Key(kJsonKeySaveConditionalFormatList);
            sWriter->StartArray();
            for (auto wSaveConditionalFormat : m_VectorSaveConditionalFormat) {
                wSaveConditionalFormat->Json(sWriter);
            }
            sWriter->EndArray();
        }
        sWriter->EndObject();
    }
        
    void tSaveSelectErase::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        tSaveSelectColRow::Json(sValue,sSheet);
        if (sValue.HasMember(kJsonKeyRect)) {
            tString wRectRectDeleteArea = sValue[kJsonKeyRect].GetString();
            RectDeleteArea(tRect(wRectRectDeleteArea));
        }
        if (sValue.HasMember(kJsonKeyMergedRange)) {
            const rapidjson::Value& wSaveRangeValue = sValue[kJsonKeyMergedRange];
            for (auto& wRangeValue : wSaveRangeValue.GetArray()) {
                tSaveRange* wSaveRange = new tSaveRange();
                wSaveRange->Json(wRangeValue, sSheet);
                m_VectorNamedMergedDataConditionalRange.push_back(wSaveRange);
                m_VectorSaveRangeCoordAfter.push_back(wSaveRange);
            }
        }
        if (sValue.HasMember(kJsonKeySaveConditionalFormatList)) {
            const rapidjson::Value& wScfValue = sValue[kJsonKeySaveConditionalFormatList];
            for (auto& wScfItem : wScfValue.GetArray()) {
                tSaveConditionalFormat* wSaveConditionalFormat = new tSaveConditionalFormat();
                wSaveConditionalFormat->Json(wScfItem);
                m_VectorSaveConditionalFormat.push_back(wSaveConditionalFormat);
            }
        }
    }
    
    tString tSaveSelectErase::WriteJson(tSheet* sSheet) {
        // RESET Json Index of format
        tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
        if (wFormatApi!=nullptr) {
            // Init save Format
            wFormatApi->BeginWriteJson();
        }
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        tSaveSelectErase::Json(&wWriter,sSheet);
        
        if (wFormatApi!=nullptr) {
            // save Format
            wWriter.Key("f");
            wFormatApi->Json(&wWriter);
        }
        
        wWriter.EndObject();
        if (wFormatApi!=nullptr) {
            // Init save Format
            
        }
        return(wStringBuffer.GetString());
    }
        
    void tSaveSelectErase::ReadJson(tString sJson,tColRowCellRange* sColRowCellRange) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        tSaveSelectErase::Json(wDocument,sColRowCellRange->Sheet());
    }
        
#ifdef _DEBUGSK
	tString tSaveSelectErase::Debug() {
        tStringStream wStream;
		wStream << "tSaveSelectErase --------------------------" << endl;
        wStream << tSaveSelect::Debug();
        wStream << "RangeNamedMerged -" << endl;
        for (auto wSaveRange : m_VectorNamedMergedDataConditionalRange) wStream << wSaveRange->Debug() << endl;
        wStream << "RangeCoordAfter -" << endl;
		for (auto wSaveRange : m_VectorSaveRangeCoordAfter) wStream << wSaveRange->Debug() << endl;
        wStream << "SaveCoveredRange -" << endl;
		for (auto wSaveCoveredRange : m_VectorSaveCoveredRange) wStream << wSaveCoveredRange->Debug() << endl;
        return(wStream.str());
	}
#endif

#ifdef checkfo
    void tSaveSelectErase::IncCheckfo(tFormatApi* sFormatApi) {
        tSaveSelectColRow::IncCheckfo(sFormatApi);
    };
#endif
 
    //=========================================================================
    //! tSaveRangeNamed
    //=========================================================================
    tSaveRangeNamed::tSaveRangeNamed() : tClass() {}
	
	tSaveRangeNamed::~tSaveRangeNamed() {
		//if (m_SaveRange != nullptr) delete(m_SaveRange);
		for (tSaveCell* wSaveCell : m_VectorCellDepend) delete(wSaveCell);
	}

	tSaveCell* tSaveRangeNamed::AddCell(tCell* sCell) {
		tIndex wRow = sCell->RowIndex();
		tIndex wCol = sCell->ColIndex();
		tString wAttribute = "";
		if (sCell->Type() == tTypeItem::t_Attribute) {
			tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(sCell);
			wAttribute = wCellAttribute->Name();
		}
		tAllocatorRef wIndiceAllocatorCellRange = sCell->ColRowCellRangeRef();
		tSaveCell wSearch(wIndiceAllocatorCellRange, sCell->Type(), wRow, wCol,0,wAttribute,0);
		tVectorSaveCell::iterator wWhere;
		wWhere = std::lower_bound(m_VectorCellDepend.begin(), m_VectorCellDepend.end(), &wSearch, tComparatorSaveCell());
		if (wWhere != m_VectorCellDepend.end()) {
			tSaveCell* wSaveCell = *wWhere;
			if ((*wSaveCell == wSearch)) {
				return(wSaveCell);
			}
		}

		tSaveCell* wSaveCell = new tSaveCell(wIndiceAllocatorCellRange,sCell->Type(), wRow, wCol,sCell->Css(),wAttribute,0);

		m_VectorCellDepend.insert(wWhere, wSaveCell);
		wSaveCell->Set(sCell);
		return(wSaveCell);
	}

	void tSaveRangeNamed::SaveAndModifyDependent(tRange* sRange) {
		tItem::tContainerCell* wVectorDependent = sRange->ContainerCellDepend();
		tItem::tContainerCell::tIteratorClass wIterator;
		tVectorCell  wVectorCellDependent;

		for (wIterator = wVectorDependent->Container()->begin(); wIterator != wVectorDependent->Container()->end(); wIterator++) {
			tCell* wCell = (*wIterator);
			if (wCell != nullptr) {
					tSaveCell* wSaveCell = AddCell(wCell);
					tIndex wIndex = 0;
					for (auto wItemFormula : *wCell->VectorRef()) {
						if (wItemFormula != nullptr) {
							tRange* wRangeFormula = wItemFormula->Range();
							if (wRangeFormula != nullptr) {
                                if (wRangeFormula == sRange) {
                                    // Is Attribute
                                    tString wAttribute = "";
                                    if (wCell->Type() == tTypeItem::t_Attribute) {
                                        tCellAttribute* wCellAttribute = static_cast<tCellAttribute*>(wCell);
                                        wAttribute = wCellAttribute->Name();
                                }

#ifdef debugundoredo
                                cout << "   --Depend  " << wRangeFormula->StrRef();
#endif
                                tSaveFormulaCell wSaveFormulaCell(sRange->ColRowCellRangeRef(),
                                        wCell->Type(),
                                        wCell->RowIndex(),
                                        wCell->ColIndex(),
                                        wAttribute,
                                        wIndex);

                                wSaveCell->PushFormulaCell(wSaveFormulaCell);

                                (*wCell->VectorRef())[wIndex] = nullptr;
                              }
                            }
						}
						wIndex++;
					}

					wVectorCellDependent.push_back(wCell);
			} // end of Cell != nullptr
		} 

		tContainerPath wContainerPath;
		wContainerPath.BeginCalculate();

		tVectorCell::iterator wIteratorCell;
		for (wIteratorCell = wVectorCellDependent.begin(); wIteratorCell != wVectorCellDependent.end(); wIteratorCell++) {
			tCell* wCell = (*wIteratorCell);
			if (wCell != nullptr) {
				sRange->DeleteDependent(wCell);
				wContainerPath.Add(wCell);
			} 
		}
		wContainerPath.EndCalculate();
	}

	void tSaveRangeNamed::RecupAndModifyDependent(tRange* sRange) {
        // First Loop for verify exist dependent (IsJson())
        for (tSaveCell* wSaveCell : m_VectorCellDepend) {
            tCell* wCellDepend = wSaveCell->Cell();
            if (wCellDepend==nullptr) {
                // Error
                tStringStream wStream;
                wStream << "tSaveRangeNamed::RecupAndModifyDependent() " << sRange->StrRef() << " CellDependent Error";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
                return;
            } else {
                // Verify if formula is correct
                for (tSaveFormulaCell wFormulaCell : *wSaveCell->VectorFormulaCellDepend()) {
                    if (wFormulaCell.Index() >= wCellDepend->VectorRef()->size()) {
                        tStringStream wStream;
                        wStream << "tSaveRangeNamed::RecupAndModifyDependent() " << sRange->StrRef() << " bad index on formula";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
                 }
            }
        }
        
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        
		// Second loop  add Formula with position
		for (tSaveCell* wSaveCell : m_VectorCellDepend) {
			tCell* wCellDepend = wSaveCell->Cell();
            
			for (tSaveFormulaCell wFormulaCell : *wSaveCell->VectorFormulaCellDepend()) {
				(*wCellDepend->VectorRef())[wFormulaCell.Index()] = sRange;
			}
			sRange->AddDependent(wCellDepend);
			wContainerPath.Add(wCellDepend);
		}
		wContainerPath.EndCalculate();
	}

    
        
    void tSaveRangeNamed::Json(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        if (!m_VectorCellDepend.empty()) {
            sWriter->Key(kJsonKeyDependentArray);
            sWriter->StartArray();
            for (auto wSaveCell : m_VectorCellDepend) {
                wSaveCell->Json(sWriter,sSheet,false);
            }
            sWriter->EndArray();
        }
    }
            
    void tSaveRangeNamed::Json(const rapidjson::Value& sValue,tSheet* sSheet) {
        if (sValue.HasMember(kJsonKeyDependentArray)) {
            const rapidjson::Value& wVectorCellDepend = sValue[kJsonKeyDependentArray];
            for (auto& wCellValue : wVectorCellDepend.GetArray()) {
                tSaveCell* wSaveCell = new tSaveCell();
                wSaveCell->Json(wCellValue, sSheet);
                m_VectorCellDepend.push_back(wSaveCell);
            }
        }
    }
    
    // Rebase ===================================================================
    tBool tSaveRangeNamed::Rebase(const tRebasePlan& sRebasePlan,tSheet* sSheet) {
        // Filter rebase plan for this sheet (similar to IsRebaseAnotherSheet)
        tRebasePlan wRebasePlan = sRebasePlan;
        tAllocatorRef wSheetRef = sSheet->AllocatorRef();
        if (sRebasePlan.m_SheetAllocator != wSheetRef) {
            wRebasePlan = sSheet->WorkBook()->UndoRebaseLog().BuildPlanAnotherSheet(sRebasePlan, wSheetRef);
        }
        
        // If no operations match this named range's sheet, no rebase needed
        if (wRebasePlan.m_Operations.empty()) {
            return true;
        }
        
        // Rebase dependent cells
        for (auto wSaveCell : m_VectorCellDepend) {
            if (!wSaveCell->Rebase(wRebasePlan)) {
                // Dependent cell was deleted
                return false;
            }
        }
        return true;
    }
  
 
}; // end of namespace
