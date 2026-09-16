//=============================================================================
// SkColRowCellRange (Container spreadsheet table)
//=============================================================================
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkCellClassAttribute.hpp"

#include <cctype>
#include <unordered_map>
#include <vector>

#define _debugcolrow
#define _debugrange
#define _debugwasm
#define _debugcolrowcellrange
#define _debugattribute
#define _debugrangecovered
#define _debugcallback
#define _debugtree


namespace SkSpreadSheet {



    static tChar FindToLowerAscii(tChar sChar) {
        return static_cast<tChar>(std::tolower(static_cast<unsigned char>(sChar)));
    }

    static tBool FindStringsEqualIgnoreCase(const tString& sA, const tString& sB) {
        if (sA.size() != sB.size()) {
            return(false);
        }
        for (size_t wI = 0; wI < sA.size(); ++wI) {
            if (FindToLowerAscii(sA[wI]) != FindToLowerAscii(sB[wI])) {
                return(false);
            }
        }
        return(true);
    }

    static tBool FindContainsIgnoreCase(const tString& sHaystack, const tString& sNeedle) {
        if (sNeedle.empty()) {
            return(true);
        }
        if (sNeedle.size() > sHaystack.size()) {
            return(false);
        }
        for (size_t wI = 0; wI + sNeedle.size() <= sHaystack.size(); ++wI) {
            tBool wMatch = true;
            for (size_t wJ = 0; wJ < sNeedle.size(); ++wJ) {
                if (FindToLowerAscii(sHaystack[wI + wJ]) != FindToLowerAscii(sNeedle[wJ])) {
                    wMatch = false;
                    break;
                }
            }
            if (wMatch) {
                return(true);
            }
        }
        return(false);
    }

// Excel find options: partial vs entire cell, optional case sensitivity.
// Search compares against grid display text (FormatString), not raw values or CSS.
static tString FindCellDisplayText(tWorkBook* sWorkbook, tCell* sCell) {
	if (sWorkbook == nullptr || sCell == nullptr) {
		return("");
	}
	tString wText = sWorkbook->CellFormatString(sCell);
	if (!wText.empty()) {
		return(wText);
	}
	const tVariant& wValue = sCell->Value();
	if (wValue.IsString()) {
		return(wValue.String());
	}
	if (!wValue.IsNull()) {
		return(wValue.Str());
	}
	return("");
}

static tBool FindCellTextMatches(const tString& sCellText, const tString& sSearch,
        tBool sMatchCase, tBool sMatchEntireCell) {
        if (sMatchEntireCell) {
            if (sMatchCase) {
                return(sCellText == sSearch);
            }
            return(FindStringsEqualIgnoreCase(sCellText, sSearch));
        }
        if (sMatchCase) {
            return(sCellText.find(sSearch) != tString::npos);
        }
        return(FindContainsIgnoreCase(sCellText, sSearch));
    }

	//================================================================================
    // Call back for find cell
	tCallBackFindCell::tCallBackFindCell(tColRowCellRange* sColRowCellRange, tString sSearch,
		tBool sMatchCase, tBool sMatchEntireCell)
		: tSparseArrayCallBack<tAllocatorRef>(), m_ColRowCellRange(sColRowCellRange), m_Search(sSearch),
		  m_MatchCase(sMatchCase), m_MatchEntireCell(sMatchEntireCell) {
        m_Workbook=m_ColRowCellRange->Sheet()->WorkBook();
	}

	tBool tCallBackFindCell::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell != nullptr) {
            tString wFormattedValue=FindCellDisplayText(m_Workbook, wCell);
			if (FindCellTextMatches(wFormattedValue, m_Search, m_MatchCase, m_MatchEntireCell)) {
				m_Vector.push_back(sAllocatorRef);
			}
		}
		return(true);
	}

    tVectorCell* tCallBackFindCell::Vector() const {
        tVectorCell* wVector = new tVectorCell();
        for(auto wAllocatorRef : m_Vector) {
            wVector->push_back(m_ColRowCellRange->Cell(wAllocatorRef));
        }
        return(wVector);
    }
    //================================================================================
    // Call back for find unique value 
    tCallBackFindUniqueValue::tCallBackFindUniqueValue(tColRowCellRange* sColRowCellRange) : tSparseArrayCallBack<tAllocatorRef>(), m_ColRowCellRange(sColRowCellRange) {
        m_Workbook=m_ColRowCellRange->Sheet()->WorkBook();
	}

	tBool tCallBackFindUniqueValue::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell != nullptr) {
            // Display text for filter pickers — not CSS format descriptors.
            tString wFormattedValue = m_Workbook->CellFormatString(wCell);
            if (wFormattedValue.empty()) {
                wFormattedValue = wCell->Value().Str();
            }
            if (!wFormattedValue.empty()) {
                m_UniqueValues.Insert(wFormattedValue);
            }
		}
		return(true);
	}

    tVectorString* tCallBackFindUniqueValue::Vector() const {
        tVectorString* wVector = new tVectorString();
        for (const tString& wValue : *m_UniqueValues.Container()) {
            wVector->push_back(wValue);
        }
        std::sort(wVector->begin(), wVector->end());
        return(wVector);
    }
    //================================================================================
	// Container of Cell 
	tColRowCellRange::tColRowCellRange() : tClass(), m_Sheet(nullptr),
		m_SheetAllocator(-1),
        m_ClearInProgress(false),
        m_ConditionalFormatContainer(nullptr),
        m_CachedLastRow(-1),
        m_CachedLastCol(-1) {
            m_CellClassContainer.Set(this);
        }

	// Just for CallCack of Delete SkAllocator
	void tColRowCellRange::Clear() {
		clear();
	}

	void tColRowCellRange::clear() {
        InvalidateExtentCache();

        // Release css refs while Sheet/WorkBook/FormatApi are still valid (before bulk allocator clear).
        ReleaseAllCellFormats();

        m_ClearInProgress=true;
        m_AllocatorCell.Clear();
         m_AllocatorCellExtend.Clear();
		m_AllocatorCellAttribute.Clear();
        m_AllocatorCellExtend.Clear();
		m_AllocatorRange.Clear();
        
		m_Rows.Clear();
		m_Cols.Clear();

        m_AllocatorRow.Clear();
		m_AllocatorCol.Clear();
		m_Cell2D.Clear();
		
        // Delete CellClass Container
        m_CellClassContainer.Clear();
        
        // Delete conditional-format rules (applied CSS already released above).
        if (m_ConditionalFormatContainer!=nullptr) {
            delete(m_ConditionalFormatContainer);
            m_ConditionalFormatContainer=nullptr;
        }
        // Delete m_VectorVolatileCell
        m_VectorVolatileCell.clear();
	}

    tCellClassContainer*  tColRowCellRange::CellClassContainer() {
        return(&m_CellClassContainer);
    }
        
    tBool tColRowCellRange::InsertCellClassAttributeContainer(tCell* sCell) {
        if (sCell->ClassAttribute()!=nullptr) {
            tIndex wRow=sCell->RowIndex();
            tIndex wCol=sCell->ColIndex();
            tString wName=sCell->ClassAttribute()->RefName();
            m_CellClassContainer.InsertCellClass(wName,CellAllocatorRef(wRow, wCol));
            return(true);
        }
        return(false);
    }


    tBool tColRowCellRange::EraseCellClassAttributeContainer(tCell* sCell) {
        if (sCell->ClassAttribute()!=nullptr) {
            tIndex wRow=sCell->RowIndex();
            tIndex wCol=sCell->ColIndex();
            tString wName=sCell->ClassAttribute()->RefName();
            m_CellClassContainer.DeleteCellClassByRef(CellAllocatorRef(wRow, wCol));
            return(true);
        }
        return(false);
    }
	// Validation
	tBool tColRowCellRange::ValidRow(tIndex sRow) { return((sRow >= 0) && (sRow <= Cst_MaxRow)); }
	tBool tColRowCellRange::ValidCol(tIndex sCol) { return((sCol >= 0) && (sCol <= Cst_MaxCol)); }

	tBool tColRowCellRange::ValidRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
		if (!ValidRow(sTop)) return(false);
		if (!ValidCol(sLeft)) return(false);
		if (!ValidRow(sBottom)) return(false);
		if (!ValidCol(sRight)) return(false);
		if (sTop > sBottom) return(false);
		if (sLeft > sRight) return(false);

		return(true);
	}

	// Sheet management
	void tColRowCellRange::Sheet(tSheet* sSheet) { m_Sheet = sSheet; }
	tSheet* tColRowCellRange::Sheet() const { return(m_Sheet); }

	void tColRowCellRange::SheetAllocator(tIndex sIndiceAllocator) { m_SheetAllocator = sIndiceAllocator; }
    tAllocatorRef tColRowCellRange::SheetAllocator() { return(m_SheetAllocator); }
    
	// ColRow ================================================================
	tColRow* tColRowCellRange::EnsureRow(tIndex sIndex) {
		if (!ValidRow(sIndex)) return(nullptr);
		tAllocatorRef wIndexAllocator = m_Rows[sIndex];
		tColRow* wRow =  m_AllocatorRow(wIndexAllocator);
		const tBool wCreated = (wRow == nullptr);
		if (wCreated) {
			tie(wIndexAllocator,wRow) = m_AllocatorRow.Alloc();
			wRow->Index(sIndex);
			//wRow->Sheet(m_Sheet);
			m_Rows[sIndex] = wIndexAllocator; 
#ifdef checksp
			if (m_Rows(sIndex) != wIndexAllocator) {
				tStringStream wStream;
				wStream << "throw: m_Rows(sIndex) != wIndexAllocator ->" << m_Rows(sIndex) << "!=" << wIndexAllocator << "  !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
#ifdef debugcolrow
			cout << "Ensure Row " << sIndex << endl;
#endif
#endif
			AttachFullColumnRangesToNewRow(wRow, sIndex);
		}
		return(wRow);
	}

	void  tColRowCellRange::DeleteRow(tIndex sIndex) {
		if (!ValidRow(sIndex)) return;
		tAllocatorRef wIndexAllocator = m_Rows[sIndex];
		if (wIndexAllocator != 0) {
            // if Node tree: reparent children, then unlink from parent
            tColRow* wColRow=Row(sIndex);
            tColRow* wColRowParent=ColRowByAllocatorRef(wColRow->ParentRef(), true);
            tColRow::tVectorColRow wChildrenCopy=wColRow->m_Children;
            for (auto wChildRef : wChildrenCopy) {
                tColRow* wColRowChildren=ColRowByAllocatorRef(wChildRef, true);
                if (wColRowChildren==nullptr) continue;
                wColRowChildren->ParentRef(wColRow->ParentRef());
                // Is already deleted parent = nullptr
                if (wColRowParent!=nullptr)
                    wColRowParent->AddChildren(wChildRef);
            }
            if (wColRowParent!=nullptr) {
                wColRowParent->DeleteChildren(wIndexAllocator);
            }
            wColRow->m_Children.clear();
        
            Row(sIndex)->DeleteFormat();
			m_AllocatorRow.Delete(wIndexAllocator);
            if (m_Rows.RealSize()==sIndex) {
                m_Rows.Erase(sIndex,1);
            } else {
                m_Rows[sIndex]=0;
            }
		}
	}

    tAllocatorRef tColRowCellRange::RowAllocatorRef(tIndex sIndex) {
        if (!ValidRow(sIndex)) return(0);
        return(m_Rows[sIndex]);
    }

    tAllocatorRef tColRowCellRange::ColAllocatorRef(tIndex sIndex) {
        if (!ValidCol(sIndex)) return(0);
        return(m_Cols[sIndex]);
    }

	tColRow* tColRowCellRange::Row(tIndex sIndex) {
		if (!ValidRow(sIndex)) return(nullptr);
		return(m_AllocatorRow(m_Rows(sIndex)));
	}

	tColRow* tColRowCellRange::EnsureCol(tIndex sIndex) {
		if (!ValidCol(sIndex)) return(nullptr);
		tAllocatorRef wIndexAllocator = m_Cols[sIndex];
		tColRow* wCol = m_AllocatorCol(wIndexAllocator);
		const tBool wCreated = (wCol == nullptr);
		if (wCreated) {
			tie(wIndexAllocator,wCol) = m_AllocatorCol.Alloc();
			wCol->Index(sIndex);
			m_Cols[sIndex] = wIndexAllocator;
#ifdef checksp
			if (m_Cols(sIndex) != wIndexAllocator) {
				tStringStream wStream;
				wStream << "m_Cols(sIndex) != wIndexAllocator ->" << m_Cols(sIndex) << "!=" << wIndexAllocator << "  !";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
			}
#ifdef debugcolrow
			cout << "tColRowCellRange::EnsureCol(" << Base10ToAlpha(sIndex) <<")" << endl;
#endif
#endif
			AttachFullRowRangesToNewCol(wCol, sIndex);
		}
		return(wCol);
	};

	void  tColRowCellRange::DeleteCol(tIndex sIndex) {
		if (!ValidCol(sIndex)) return;
		tAllocatorRef wIndexAllocator = m_Cols[sIndex];
        if (wIndexAllocator != 0) {
            // if Node tree: reparent children, then unlink from parent
            tColRow* wColRow=Col(sIndex);
            tColRow* wColRowParent=ColRowByAllocatorRef(wColRow->ParentRef(), false);
            tColRow::tVectorColRow wChildrenCopy=wColRow->m_Children;
            for (auto wChildRef : wChildrenCopy) {
                tColRow* wColRowChildren=ColRowByAllocatorRef(wChildRef, false);
                if (wColRowChildren==nullptr) continue;
                wColRowChildren->ParentRef(wColRow->ParentRef());
                // Is already deleted parent = nullptr
                if (wColRowParent!=nullptr)
                    wColRowParent->AddChildren(wChildRef);
            }
            if (wColRowParent!=nullptr) {
                wColRowParent->DeleteChildren(wIndexAllocator);
            }
            wColRow->m_Children.clear();
            
            Col(sIndex)->DeleteFormat();
            m_AllocatorCol.Delete(wIndexAllocator);
            if (m_Cols.RealSize()==sIndex) {
                m_Cols.Erase(sIndex,1);
            } else {
                m_Cols[sIndex]=0;
            }
        }
	}

	tColRow* tColRowCellRange::Col(tIndex sIndex) { 
		if (!ValidCol(sIndex)) return(nullptr);
		return(m_AllocatorCol(m_Cols(sIndex)));
	}

    
	tAllocatorRef tColRowCellRange::AllocCellAttribute() {
		tCellAttribute* wCellAttribute;
		tAllocatorRef wAllocatorRef;
		tie(wAllocatorRef, wCellAttribute) = m_AllocatorCellAttribute.Alloc();
#ifdef debugattribute
		cout << "AllocCellAttribute(" << wAllocatorRef << ")" << endl;
#endif
		return(wAllocatorRef);
	}

	tCellAttribute* tColRowCellRange::CellAttribute(tAllocatorRef sAllocatorRef) {
		return(m_AllocatorCellAttribute(sAllocatorRef));
	}

	tCellAttribute* tColRowCellRange::CellAttribute(tInt sRow, tInt sCol, tString sAttribute) {
		tCellAttribute* wCellAttribute = nullptr;
		tCell* wCell = Cell(sRow, sCol);
		if (wCell != nullptr) {
			tVariant* wVariant = wCell->PtValue();
			if (wVariant->Type() == tVariantType::t_class) {
				tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wVariant->Class());
				if (wCellClass != nullptr) {
					wCellAttribute = wCellClass->Find(sAttribute);
                    if (wCellAttribute==nullptr) {
                        // Add Default ========================================
                        tModelClass* wModelClass=wCellClass->ModelClass();
                        if (wModelClass!=nullptr) {
                            tModelProperty* wModelProperty=wModelClass->Property(sAttribute);
                            if (wModelProperty!=nullptr) {
                                wCellAttribute = wCellClass->CellAttribute(sAttribute);
                                wCellAttribute->Value(wModelProperty->DefaultValue());
                            }
                        }
                    }
				}
			}
		}
		return(wCellAttribute);
	}

	tAllocatorRef tColRowCellRange::AllocCellExtend() {
		tCellExtend* wCellExtend;
		tAllocatorRef wAllocatorRef;
		tie(wAllocatorRef, wCellExtend) = m_AllocatorCellExtend.Alloc();
		return(wAllocatorRef);
	}

    tCellExtend* tColRowCellRange::CellExtend(tAllocatorRef sAllocatorRef) {
        return(m_AllocatorCellExtend(sAllocatorRef));
    }

	tBool tColRowCellRange::DeleteCellExtend(tAllocatorRef sAllocatorRef) {
		return(m_AllocatorCellExtend.Delete(sAllocatorRef));
	}

	tCellAttribute* tColRowCellRange::EnsureCellAttribute(tInt sRow, tInt sCol, tString sAttribute) {
		tCellAttribute* wCellAttribute = nullptr;
		tCell* wCell = Cell(sRow, sCol);
		if (wCell != nullptr) {
			tVariant* wVariant = wCell->PtValue();
			if (wVariant->Type() == tVariantType::t_class) {
				tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wVariant->Class());
				if (wCellClass != nullptr) {
                    wCellAttribute = wCellClass->CellAttribute(sAttribute);
#ifdef debugattribute
					cout << "tColRowCellRange::EnsureCellAttribute(" << Base10ToAlpha(sCol) << sRow << ")";
                    cout << " Cell:" << wCell->tItem::StrRef() << " Class:" << wCellClass->ClassName() << " Attribute:" << wCellAttribute->Name() << endl;
					cout << wCellClass->Debug();
#endif
                   
				}
			}
		}
		return(wCellAttribute);
	}

	tBool tColRowCellRange::DeleteCellAttribute(tInt sRow, tInt sCol, tString sAttribute) {
		tCell* wCell = Cell(sRow, sCol);
		if (wCell != nullptr) {
			tVariant* wVariant = wCell->PtValue();
			if (wVariant->Type() == tVariantType::t_class) {
				tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wVariant->Class());
				if (wCellClass != nullptr) {
					return(wCellClass->DeleteCellAttribute(sAttribute));
				}
			}
		}
		return(false);
	}

	tBool tColRowCellRange::DeleteCellAttribute(tAllocatorRef sAllocatorRef) {
#ifdef debugattribute
		cout << "tColRowCellRange::DeleteCellAttribute(" << sAllocatorRef << ")" << endl;
#endif
		return(m_AllocatorCellAttribute.Delete(sAllocatorRef));
	}


	tCell* tColRowCellRange::EnsureCell(tIndex sRow, tIndex sCol) {
		if (!ValidRow(sRow)) return(nullptr);
		if (!ValidCol(sCol)) return(nullptr);
#ifdef DebugDbAllocator
		cout << "tColRowCellRange::Ensure Cell " << sRow << ":" << sCol << "  m_Cell2D.Size() -> " << m_Cell2D.Size();
		cout << "  m_AllocatorCell.Size() -> " << m_AllocatorCell.Size() << endl;
#endif
		// First Get Cell to Row
		tSparseArrayCell* wSparseArrayCell = m_Cell2D(sRow);
		if (wSparseArrayCell != nullptr) {
			tAllocatorRef wIndex = (*wSparseArrayCell)(sCol);
			if (wIndex != 0) {
				return(m_AllocatorCell(wIndex));
			}
		}
        // Verify or create row and col for cell 
		tColRow* wRow=EnsureRow(sRow);
		tColRow* wCol=EnsureCol(sCol);

		// Alloc new wSparseArrayCell ?
		if (wSparseArrayCell == nullptr) {
			wSparseArrayCell = new tSparseArrayCell();
			m_Cell2D[sRow] = wSparseArrayCell;
		}
		tAllocatorRef wCellAllocatorRef = (*wSparseArrayCell)[sCol];
		tCell* wCell = m_AllocatorCell(wCellAllocatorRef);
		if (wCell == nullptr) {
#ifdef debugcolrow
			cout << "Ensure Cell " << Base10ToAlpha(sCol) << sRow << endl;
#endif
			tie(wCellAllocatorRef, wCell) = m_AllocatorCell.Alloc();
			(*wSparseArrayCell)[sCol] = wCellAllocatorRef;
			wCell->Set(m_SheetAllocator,m_Rows[sRow], m_Cols[sCol]);

			wRow->IncNbCells();
			wCol->IncNbCells();
            InvalidateExtentCache();
		}
		return(wCell);
	}

	tBool tColRowCellRange::DeleteCell(tIndex sRow, tIndex sCol) {
		if (!ValidRow(sRow)) return(false);
		if (!ValidCol(sCol)) return(false);
#ifdef debugcolrow
        cout << "tColRowCellRange::DeleteCell(" << Base10ToAlpha(sCol)  << sRow << ")" << endl;
#endif

		tSparseArrayCell* wSparseArrayCell = m_Cell2D(sRow);
		if (wSparseArrayCell != nullptr) {
			tAllocatorRef wCellAllocatorRef = (*wSparseArrayCell)(sCol);
			if (wCellAllocatorRef !=0) {
                tCell* wCell= Cell(sRow,sCol);
                // Delete CellClassName
                if (wCell->Value().IsClass()) {
                    tCellClassAttribute* wCellClassAttribute=wCell->ClassAttribute();
                    if (wCellClassAttribute!=nullptr)
                        m_CellClassContainer.DeleteCellClassByRef(wCellAllocatorRef);
                }
                // Always unregister: Formula may already be null while the cell is still listed.
                DeleteVolatile(wCell);
                // Drop format pool refcount before allocator destroys the cell (~tCell does not Clear).
                if (wCell->Css() != 0) {
                    wCell->WorkBook()->DeleteCellFormat(wCell->Css());
                    wCell->Css(0);
                }

				m_AllocatorCell.Delete(wCellAllocatorRef);
				// Set null at position
				(*wSparseArrayCell)(sCol) = 0;
                // Nb Cells on Colrow
                // Delete Row ?
                tColRow* wRow = Row(sRow);
                DecNbCells(wRow,true);

                // Delete Col ?
                tColRow* wCol = Col(sCol);
                DecNbCells(wCol, false);
                InvalidateExtentCache();
            }
		}
		return(true);
	}
        
    tCell* tColRowCellRange::EnsureCellClass(tIndex sRow, tIndex sCol,tString sClassName) {
        tCell* wCell=EnsureCell(sRow, sCol);
#ifdef debugcolrow
        cout << " -> wCell : " << wCell->StrRef() << "=" << wCell->Value();
#endif
        tVirtualClass* wClass=tClassFactory::Instance()->Create(sClassName);
    
        tCellClassAttribute* wClassAttribute = dynamic_cast<tCellClassAttribute*>(wClass);
        if (wClassAttribute != nullptr) {
            tModelClass* wModelClass=tClassFactory::Instance()->Get(sClassName);
            // Fix sClassName to Name
            wClassAttribute->ClassName(sClassName);
            wClassAttribute->RefName(sClassName);
            tVariant wVariant(wClassAttribute);
            delete(wClassAttribute);
            wCell->Value(wVariant);
#ifdef debugcolrow
            cout << "tUndoClass::Do wCell->" << wCell->StrRef() << " Indice Sheet=" << wCell->Sheet()->IndexAllocatorColRowCellRange() << endl;
#endif
            tVariant* wPtVariant = wCell->PtValue();
            tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wPtVariant->Class());
            if (wCellClass == nullptr) {
                tStringStream wStream;
                wStream << "throw: Undo Cell Bad Variant Class !";
                cerr<< wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
            wCellClass->SheetIndice(wCell->Sheet()->IndexAllocatorColRowCellRange());
            wCellClass->CellRootRef(this, sRow, sCol);
            wCellClass->SetModelClass(wModelClass);
            wCellClass->Rooted(wCell);
            
            // Add In m_CellContainer change name if duplicate
            tString wName=wCellClass->RefName();
            wName=m_CellClassContainer.GetNextName(wName);
            wCellClass->RefName(wName);
            
            // Insert Class in container
            InsertCellClassAttributeContainer(wCell);
        } else {
                tCellClass* wCellClass=dynamic_cast<tCellClass*>(wClass);
                if (wCellClass!=nullptr) {
                    tVariant wVariant(wCellClass);
                    delete(wClass);
                    wCell->Value(wVariant);
                } else {
                    delete(wClass);
                }
        }
        return(wCell);
    }
        
	tCell* tColRowCellRange::Cell(tIndex sRow, tIndex sCol) {
		if (!ValidRow(sRow)) return(nullptr);
		if (!ValidCol(sCol)) return(nullptr);
		tSparseArrayCell* wSparseArrayCell = m_Cell2D(sRow);
		if (wSparseArrayCell != nullptr) {
			tAllocatorRef wCellAllocatorRef = (*wSparseArrayCell)(sCol);
			return(m_AllocatorCell(wCellAllocatorRef));
		}
		return(nullptr);
	}
        
        tBool tColRowCellRange::IsDeletedCell(tAllocatorRef sAllocatorRef) {
            return(m_AllocatorCell.IsNullptr(sAllocatorRef));
        }
        

	tAllocatorRef tColRowCellRange::CellAllocatorRef(tIndex sRow, tIndex sCol) {
		if (!ValidRow(sRow)) return(0);
		if (!ValidCol(sCol)) return(0);
		tSparseArrayCell* wSparseArrayCell = m_Cell2D(sRow);
		if (wSparseArrayCell != nullptr) {
			tAllocatorRef wCellAllocatorRef = (*wSparseArrayCell)(sCol);
			return(wCellAllocatorRef);
		}
		return(0);
	}
        
namespace {

static tBool RectContainsCell(tRect& sRect, tIndex sRow, tIndex sCol) {
    return(sRow >= sRect.Top() && sRow <= sRect.Bottom() &&
           sCol >= sRect.Left() && sCol <= sRect.Right());
}

static void RootRelocatedCell(tColRowCellRange* sGrid, tIndex sRow, tIndex sCol, tCell* sCell) {
    if (sCell == nullptr) {
        return;
    }
    sCell->SetColRow(sGrid->RowAllocatorRef(sRow), sGrid->ColAllocatorRef(sCol));
    if (tCellClassAttribute* wCellClass = sCell->ClassAttribute()) {
        wCellClass->CellRootRef(sGrid, sRow, sCol);
        wCellClass->Rooted(sCell);
    }
}

/// Re-root every occupied cell from sFirstRow downward (inclusive). Skips empty tracks.
static void RelocateOccupiedCellsFrom(tColRowCellRange* sGrid,
    tColRowCellRange::tSparseArray2DCell& sCell2D,
    tIndex sFirstRow) {
    if (sGrid == nullptr) {
        return;
    }
    const tSize wFrom = (sFirstRow > 0) ? static_cast<tSize>(sFirstRow) : 0;
    sCell2D.ForEachOccupiedFrom(wFrom, [sGrid](tSize wRow, tColRowCellRange::tSparseArrayCell* wRowCells) {
        wRowCells->ForEachOccupied([sGrid, wRow](tSize wCol, tAllocatorRef wRef) {
            RootRelocatedCell(sGrid, static_cast<tIndex>(wRow), static_cast<tIndex>(wCol),
                sGrid->Cell(wRef));
            return true;
        });
        return true;
    });
}

// DeleteBorder ends with ApplyCell which IncCell even when the pool entry already exists.
static void UndoInsertStripApplyCellBump(tWorkBook* sWorkBook, tFormatRef sCssOrigin, tFormatRef sCss,
                                         tBool& sDedupeUndoOut) {
    sDedupeUndoOut = false;
    if (sWorkBook == nullptr || sCss == 0 || sCss == sCssOrigin) {
        return;
    }
    // Fresh stripped ref stays at count 1 until the first cell assign; only undo dedup orphan bumps.
    if (sWorkBook->CellFormatRefCount(sCss) > 1) {
        sWorkBook->DeleteCellFormat(sCss);
        sDedupeUndoOut = true;
    }
}

// One stripped ref per origin ref.
struct tInsertCellCssCache {
    std::unordered_map<tFormatRef, tFormatRef> m_StrippedByOrigin;
    std::unordered_map<tFormatRef, tUInt> m_StrippedAssignCount;
    std::unordered_map<tFormatRef, tBool> m_StrippedDedupeUndo;
};

static tShort BuildInsertRowStripMask(tWorkBook* sWorkBook, tFormatRef sCssOrigin, tBool sValidBelowIns) {
    const tShort wMask = sWorkBook->BorderMask(sCssOrigin);
    if (wMask == 0) {
        return(0);
    }
    tShort wStripMask = (tShort)(tBorderTop);
    if (sValidBelowIns) {
        wStripMask = (tShort)(wStripMask | tBorderBottom);
    }
    const tBool wStripLeftRight =
        ((wMask & tBorderBottom) != 0) || ((wMask & tBorderAll) != 0);
    if (wStripLeftRight) {
        wStripMask = (tShort)(wStripMask | tBorderLeft | tBorderRight);
    }
    return(wStripMask);
}

static tShort BuildInsertColStripMask(tWorkBook* sWorkBook, tFormatRef sCssOrigin, tBool sValidRightIns) {
    const tShort wMask = sWorkBook->BorderMask(sCssOrigin);
    if (wMask == 0) {
        return(0);
    }
    tShort wStripMask = (tShort)(tBorderLeft);
    if (sValidRightIns) {
        wStripMask = (tShort)(wStripMask | tBorderRight);
    }
    const tBool wStripTopBottom =
        ((wMask & tBorderRight) != 0) || ((wMask & tBorderAll) != 0);
    if (wStripTopBottom) {
        wStripMask = (tShort)(wStripMask | tBorderTop | tBorderBottom);
    }
    return(wStripMask);
}

static tFormatRef StripCssForInsertRow(tWorkBook* sWorkBook, tFormatRef sCssOrigin, tBool sValidBelowIns,
                                       tInsertCellCssCache& sCache) {
    const tShort wStripMask = BuildInsertRowStripMask(sWorkBook, sCssOrigin, sValidBelowIns);
    if (wStripMask == 0) {
        return(sCssOrigin);
    }
    const tFormatRef wCss = sWorkBook->DeleteBorder(sCssOrigin, wStripMask);
    tBool wDedupeUndo = false;
    UndoInsertStripApplyCellBump(sWorkBook, sCssOrigin, wCss, wDedupeUndo);
    if (wDedupeUndo && sCache.m_StrippedDedupeUndo.find(wCss) == sCache.m_StrippedDedupeUndo.end()) {
        sCache.m_StrippedDedupeUndo.emplace(wCss, true);
    }
    return(wCss);
}

static tFormatRef StripCssForInsertCol(tWorkBook* sWorkBook, tFormatRef sCssOrigin, tBool sValidRightIns,
                                       tInsertCellCssCache& sCache) {
    const tShort wStripMask = BuildInsertColStripMask(sWorkBook, sCssOrigin, sValidRightIns);
    if (wStripMask == 0) {
        return(sCssOrigin);
    }
    const tFormatRef wCss = sWorkBook->DeleteBorder(sCssOrigin, wStripMask);
    tBool wDedupeUndo = false;
    UndoInsertStripApplyCellBump(sWorkBook, sCssOrigin, wCss, wDedupeUndo);
    if (wDedupeUndo && sCache.m_StrippedDedupeUndo.find(wCss) == sCache.m_StrippedDedupeUndo.end()) {
        sCache.m_StrippedDedupeUndo.emplace(wCss, true);
    }
    return(wCss);
}

static tFormatRef CachedStripCssForInsertRow(tWorkBook* sWorkBook, tFormatRef sCssOrigin,
                                             tBool sValidBelowIns, tInsertCellCssCache& sCache) {
    const auto wFound = sCache.m_StrippedByOrigin.find(sCssOrigin);
    if (wFound != sCache.m_StrippedByOrigin.end()) {
        return(wFound->second);
    }
    const tFormatRef wCss = StripCssForInsertRow(sWorkBook, sCssOrigin, sValidBelowIns, sCache);
    sCache.m_StrippedByOrigin.emplace(sCssOrigin, wCss);
    return(wCss);
}

static tFormatRef CachedStripCssForInsertCol(tWorkBook* sWorkBook, tFormatRef sCssOrigin,
                                             tBool sValidRightIns, tInsertCellCssCache& sCache) {
    const auto wFound = sCache.m_StrippedByOrigin.find(sCssOrigin);
    if (wFound != sCache.m_StrippedByOrigin.end()) {
        return(wFound->second);
    }
    const tFormatRef wCss = StripCssForInsertCol(sWorkBook, sCssOrigin, sValidRightIns, sCache);
    sCache.m_StrippedByOrigin.emplace(sCssOrigin, wCss);
    return(wCss);
}

static void AssignInsertCellCss(tWorkBook* sWorkBook, tCell* sCell, tFormatRef sCssOrigin, tFormatRef sCss,
                                tInsertCellCssCache& sCache) {
    sCell->Css(sCss);
    if (sCss == 0) {
        return;
    }
    if (sCss == sCssOrigin) {
        sWorkBook->IncCellFormat(sCss);
        return;
    }
    const tBool wDedupeUndo = sCache.m_StrippedDedupeUndo.find(sCss) != sCache.m_StrippedDedupeUndo.end();
    if (wDedupeUndo || sCache.m_StrippedAssignCount[sCss]++ > 0) {
        sWorkBook->IncCellFormat(sCss);
    }
}

static void AssignInsertColRowCss(tWorkBook* sWorkBook, tColRow* sColRow, tFormatRef sCssOrigin, tFormatRef sCss,
                                  tInsertCellCssCache& sCache) {
    if (sColRow->Css() != 0) {
        sWorkBook->DeleteCellFormat(sColRow->Css());
    }
    sColRow->Css(sCss);
    if (sCss == 0) {
        return;
    }
    if (sCss == sCssOrigin) {
        sWorkBook->IncCellFormat(sCss);
        return;
    }
    const tBool wDedupeUndo = sCache.m_StrippedDedupeUndo.find(sCss) != sCache.m_StrippedDedupeUndo.end();
    if (wDedupeUndo || sCache.m_StrippedAssignCount[sCss]++ > 0) {
        sWorkBook->IncCellFormat(sCss);
    }
}

} // namespace

    tBool tColRowCellRange::RelocateRect(tRect& sSource, tRect& sDest) {
        struct tMovingCell {
            tIndex m_Row;
            tIndex m_Col;
            tAllocatorRef m_Ref;
        };
        std::vector<tMovingCell> wMoving;

        for (tIndex wRow = sSource.Top(); wRow <= sSource.Bottom(); wRow++) {
            for (tIndex wCol = sSource.Left(); wCol <= sSource.Right(); wCol++) {
                const tAllocatorRef wRef = CellAllocatorRef(wRow, wCol);
                if (wRef != 0) {
                    wMoving.push_back({wRow, wCol, wRef});
                }
            }
        }

        if (wMoving.empty()) {
            return(true);
        }

        if (sSource.Top() + (sDest.Bottom() - sSource.Bottom()) != sDest.Top() ||
            sSource.Left() + (sDest.Right() - sSource.Left()) != sDest.Left()) {
            return(false);
        }
        const tIndex wDeltaRow = sDest.Top() - sSource.Top();
        const tIndex wDeltaCol = sDest.Left() - sSource.Left();

        // Erase dest-only cells (not part of source overlap — those are detached, not deleted).
        for (tIndex wRow = sDest.Top(); wRow <= sDest.Bottom(); wRow++) {
            for (tIndex wCol = sDest.Left(); wCol <= sDest.Right(); wCol++) {
                if (RectContainsCell(sSource, wRow, wCol)) {
                    continue;
                }
                if (CellAllocatorRef(wRow, wCol) != 0) {
                    DeleteCell(wRow, wCol);
                }
            }
        }

        for (const tMovingCell& wSlot : wMoving) {
            tColRow* wColRow = Row(wSlot.m_Row);
            tColRow* wColRowCol = Col(wSlot.m_Col);
            SetCellAllocatorRef(0, wSlot.m_Row, wSlot.m_Col);
            if (wColRow != nullptr) {
                DecNbCells(wColRow, true);
            }
            if (wColRowCol != nullptr) {
                DecNbCells(wColRowCol, false);
            }
        }

        for (const tMovingCell& wSlot : wMoving) {
            const tIndex wNewRow = wSlot.m_Row + wDeltaRow;
            const tIndex wNewCol = wSlot.m_Col + wDeltaCol;
            const tBool wDestWasEmpty = (CellAllocatorRef(wNewRow, wNewCol) == 0);
            SetCellAllocatorRef(wSlot.m_Ref, wNewRow, wNewCol);
            if (wDestWasEmpty) {
                tColRow* wColRow = EnsureRow(wNewRow);
                tColRow* wColRowCol = EnsureCol(wNewCol);
                if (wColRow != nullptr) {
                    wColRow->IncNbCells();
                }
                if (wColRowCol != nullptr) {
                    wColRowCol->IncNbCells();
                }
            }
            RootRelocatedCell(this, wNewRow, wNewCol, Cell(wNewRow, wNewCol));
        }
        return(true);
    }

        void tColRowCellRange::SetCellAllocatorRef(tAllocatorRef sAllocatorRef,tIndex sRow, tIndex sCol) {
            if (!ValidRow(sRow)) return;
            if (!ValidCol(sCol)) return;
            
            // Verify or create row and col for cell
            if (sAllocatorRef!=0) {
                EnsureRow(sRow);
                EnsureCol(sCol);
            }
           
            tSparseArrayCell* wSparseArrayCell = m_Cell2D(sRow);
            // Alloc new wSparseArrayCell ?
            if (wSparseArrayCell == nullptr) {
                wSparseArrayCell = new tSparseArrayCell();
                m_Cell2D[sRow] = wSparseArrayCell;
            }
            tCell* wCell = m_AllocatorCell(sAllocatorRef);
            (*wSparseArrayCell)[sCol] = sAllocatorRef;
            if (wCell!=nullptr) {
                wCell->Set(m_SheetAllocator,m_Rows[sRow], m_Cols[sCol]);
            }

    }
        
	tCell* tColRowCellRange::Cell(tAllocatorRef sAllocatorRef) {
		return(m_AllocatorCell(sAllocatorRef));
	}

	void tColRowCellRange::DeleteCell(tAllocatorRef sAllocatorRef) {
		if (sAllocatorRef != 0) {
            tCell* wCell=Cell(sAllocatorRef);
            // Delete CellClassName
            if (wCell->Value().IsClass()) {
                tCellClassAttribute* wCellClass=dynamic_cast<tCellClassAttribute*>(wCell->Value().Class());
                if (wCellClass!=nullptr)
                    m_CellClassContainer.DeleteCellClassByRef(sAllocatorRef);
            }
            // Important Delete Volatile if cell is in volatile cells.
            DeleteVolatile(wCell);
            if (wCell->Css() != 0) {
                wCell->WorkBook()->DeleteCellFormat(wCell->Css());
                wCell->Css(0);
            }
			m_AllocatorCell.Delete(sAllocatorRef);
		}
	}
 
        
    tBool tColRowCellRange::AddVolatile(tCell* sCell) {
        if (sCell == nullptr) {
            return(false);
        }
        const tFormula* wFormula = sCell->Formula();
        if (wFormula == nullptr || wFormula->BitSetVolatile().Empty()) {
#ifdef checksp
            tStringStream wStream;
            wStream << "tColRowCellRange::AddVolatile error Cell " << sCell->StrRef(true) << "=" << sCell->FormulaStr()  << " not volatile !";
            cerr << wStream.str() << endl;
#endif
            return(false);
        }
        const tAllocatorRef wAllocatorRef = CellAllocatorRef(sCell->RowIndex(), sCell->ColIndex());
        if (wAllocatorRef == 0) {
            return(false);
        }
        auto it = std::lower_bound(m_VectorVolatileCell.begin(), m_VectorVolatileCell.end(), wAllocatorRef);
        if ((it == m_VectorVolatileCell.end()) || (*it != wAllocatorRef)) {
            m_VectorVolatileCell.insert(it, wAllocatorRef);
            return(true);
        }
        return(false);
    }
 
   tBool tColRowCellRange::DeleteVolatile(tCell* sCell) {
       if (sCell == nullptr) {
           return(false);
       }
       // Match this object by pointer through the allocator. Do not resolve via Row/Col first:
       // recycled slots still carry stale Row/Col and would unregister a different live cell.
       for (auto it = m_VectorVolatileCell.begin(); it != m_VectorVolatileCell.end(); ++it) {
           if (m_AllocatorCell(*it) == sCell) {
               m_VectorVolatileCell.erase(it);
               return(true);
           }
       }
       return(false);
    }

    void tColRowCellRange::AddPathVolatile(tContainerPath*  sContainerPath,tVolatile sVolatile) {
        // Resolve via allocator so deleted slots become nullptr (raw tCell* would dangle).
        auto it = m_VectorVolatileCell.begin();
        while (it != m_VectorVolatileCell.end()) {
            const tAllocatorRef wAllocatorRef = *it;
            tCell* wCell = m_AllocatorCell(wAllocatorRef);
            if (wCell == nullptr) {
#ifdef checksp
                tStringStream wStream;
                wStream << "tColRowCellRange::AddPathVolatile error Cell-Allocator(" << wAllocatorRef << ") nullptr";
                cerr << wStream.str() << endl;
#endif
                it = m_VectorVolatileCell.erase(it);
                continue;
            }
            const tFormula* wFormula = wCell->Formula();
            if (wFormula == nullptr) {
                it = m_VectorVolatileCell.erase(it);
                continue;
            }
            if (wFormula->Volatile(sVolatile)) {
                sContainerPath->Add(wCell);
            }
#ifdef checksp
            if (wFormula->BitSetVolatile().Empty()) {
                tStringStream wStream;
                wStream << "tColRowCellRange::AddPathVolatile error Cell " << wCell->StrRef(true) << "=" << wCell->FormulaStr()  << " not volatile !";
                cerr << wStream.str() << endl;
            }
#endif
            ++it;
        }
    }
             
#ifdef checksp
    tBool tColRowCellRange::IsInVolatileCells(tCell* sCell) {
        if (sCell == nullptr || sCell->Row() == nullptr || sCell->Col() == nullptr) {
            return(false);
        }
        const tAllocatorRef wAllocatorRef = CellAllocatorRef(sCell->RowIndex(), sCell->ColIndex());
        auto it = std::lower_bound(m_VectorVolatileCell.begin(), m_VectorVolatileCell.end(), wAllocatorRef);
        if ((it != m_VectorVolatileCell.end()) && (*it == wAllocatorRef)) {
            return(true);
        }
        return(false);
    }
#endif

   
    tRange* tColRowCellRange::Range(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
        if (!ValidRange(sTop, sLeft, sBottom, sRight)) return(nullptr);
       
        tColRow* wTop=m_AllocatorRow(m_Rows(sTop));
        // if not Col or Row not range
        if (wTop==nullptr) return(nullptr);
        
        tColRow* wLeft=m_AllocatorCol(m_Cols(sLeft));
        if (wLeft==nullptr) return(nullptr);
        
        tColRow* wBottom=m_AllocatorRow(m_Rows(sBottom));
        if (wBottom==nullptr) return(nullptr);
        
        tColRow* wRight=m_AllocatorCol(m_Cols(sRight));
        if (wRight==nullptr) return(nullptr);
        
        // Search in ColRow
        tColRow::tContainerRange::tResult wResult1;
        wTop->GetIntersection(wLeft, &wResult1);
        if (wResult1.size()==0) return(nullptr);
        
        tColRow::tContainerRange::tResult wResult2;
        wBottom->GetIntersection(wRight, &wResult2);
        if (wResult2.size()==0) return(nullptr);
        
        // Make TopLeft and BottomRight Result
        tColRow::tContainerRange wTopLeft;
        for(auto wAllocatorRange : wResult1) {
            wTopLeft.InsertClass(wAllocatorRange);
        }
        
        tColRow::tContainerRange wBottomRight;
        for(auto wAllocatorRange : wResult2) {
            wBottomRight.InsertClass(wAllocatorRange);
        }
        
        tColRow::tContainerRange::tResult wResult;
        wTopLeft.GetIntersection(&wBottomRight, &wResult);
       
        for(auto wRangeRef : wResult) {
#ifdef debugcolrow
            if (IsDeletedRange(wRangeRef)) {
                cout << "Error Range(" << wRangeRef << ") =nullptr" << endl;
            }
#endif
      
            tRange* wRange=Range(wRangeRef);
            if ((wRange->TopIndex()==sTop) &&
                (wRange->LeftIndex()==sLeft) &&
                (wRange->BottomIndex()==sBottom) &&
                (wRange->RightIndex()==sRight)) {
                return(wRange);
            }
        }
        
        return(nullptr);
    }
        
    tRange* tColRowCellRange::EnsureRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
#ifdef debugrange
		tStringStream wStream;
		wStream << Base10ToAlpha(sLeft) << sTop << ":" << Base10ToAlpha(sRight) << sBottom;
        cout << "tColRowCellRange::EnsureRange(" << wStream.str() << ")" << endl;
      
#endif

		if (!ValidRange(sTop, sLeft, sBottom, sRight)) return(nullptr);
		
        tRange* wRange=Range(sTop, sLeft, sBottom, sRight);
        if (wRange!=nullptr) {
            NotifySpillExtent(sBottom, sRight);
            return(wRange);
        }
#ifdef debugrange
        cout << "  --->EnsureRange(" << wStream.str() << ")" << endl;
#endif
        EnsureRow(sTop);
        EnsureCol(sLeft);
        EnsureRow(sBottom);
        EnsureCol(sRight);

        tAllocatorRef wIndex;
        tie(wIndex,wRange) = m_AllocatorRange.Alloc();

        wRange->Set(m_SheetAllocator, m_Rows[sTop], m_Cols[sLeft], m_Rows[sBottom], m_Cols[sRight]);
        wRange->AllocatorRef(wIndex);

        // SetColRow Container ranges
        AttachRangeToColRow(wRange);
#ifdef debugrange
        cout << " Allocator Ref " << wRange->AllocatorRef() << endl;;
#endif
        NotifySpillExtent(sBottom, sRight);
		return(wRange);
	}
	
    tRange*  tColRowCellRange::EnsureRange(tCell* sCellLeft, tCell* sCellRight) {
        tRange* wRange=nullptr;
        if (sCellLeft!=nullptr && sCellRight!=nullptr) {
            tTempoPoint wPointLeft(sCellLeft->RowIndex(),sCellLeft->ColIndex());
            tTempoPoint wPointRight(sCellRight->RowIndex(),sCellRight->ColIndex());
            if ((wPointLeft.Row()<=wPointRight.Row()) &&
                (wPointLeft.Col()<=wPointRight.Col())) {
                wRange=Sheet()->EnsureRange(wPointLeft.Row(),
                                                    wPointLeft.Col(),
                                                    wPointRight.Row(),
                                                    wPointRight.Col());
                
            }
        }
        return(wRange);
    }

	void tColRowCellRange::DeleteRangeByAllocatorRef(tAllocatorRef sAllocatorRef,tBool sClean) {
        tRange* wRange = m_AllocatorRange(sAllocatorRef);
        if (wRange == nullptr) {
            // Cleanup paths can legitimately attempt to delete the same range twice
            // (e.g. duplicated refs in a failed compile). In clean mode, ignore.
            if (sClean) {
                return;
            }
            tStringStream wStream;
            wStream << "throw: EraseRange tAllocatorRef ->" << sAllocatorRef << "  on nullptr value  !";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
#ifdef debugrange
        // For Debug
        if (sAllocatorRef == 17) {
        }
        cout << "ColRowCellRange::DeleteRange : " << wRange->Debug();
#endif
        // Est ce que named Range
        if (wRange->IsNamed()) {
            tWorkBook* wWorkBook = m_Sheet->WorkBook();
            
            tString wName= wWorkBook->FindRangeNamed(sAllocatorRef,m_Sheet);
            if (wName == "") {
                tStringStream wStream;
                wStream << "throw: EraseRange tAllocatorRef ->" << sAllocatorRef << "  " << wRange->StrRef() << " Not named Range !";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
            wWorkBook->DeleteRangeNamed(wName);
        }
        
        if (!wRange->NotDependent()) {
            tStringStream wStream;
            wStream << "throw: Error delete range " << wRange->Sheet()->Name() << "!" << wRange->StrRef() << " width Dependant !!!! ";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
            wRange->ClearDependent();
        }
        
        // Erase in ColRow container
        DetachRangeFromColRow(wRange,true,sClean);
        // Delete in Allocator
        m_AllocatorRange.Delete(wRange->AllocatorRef());

	}

	tBool tColRowCellRange::DeleteRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
#ifdef debugrange
		cout << "tColRowCellRange::DeleteRange(" << Base10ToAlpha(sLeft) << sTop << ":" << Base10ToAlpha(sRight) << sBottom << ") by Index ";
#endif
		if (!ValidRange(sTop, sLeft, sBottom, sRight)) return(false);
		// if not Col or Row not range
		if (m_AllocatorRow(m_Rows(sTop)) == nullptr) return(false);
		if (m_AllocatorCol(m_Cols(sLeft)) == nullptr) return(false);
		if (m_AllocatorRow(m_Rows(sBottom)) == nullptr) return(false);
		if (m_AllocatorCol(m_Cols(sRight)) == nullptr) return(false);
		
        
        tRange* wRange=Range(sTop, sLeft, sBottom, sRight);
        if (wRange != nullptr) {
            if (!wRange->NotDependent()) {
                tStringStream wStream;
                wStream << "throw: Error range " << wRange->Sheet()->Name() << "!" << wRange->StrRef() << " width Dependant !!!!" << endl;
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
                wRange->ClearDependent();
            }
            // IsData 
            if (wRange->IsNamed()) {
                tWorkBook* wWorkBook=m_Sheet->WorkBook();
                wWorkBook->RangeNamedContainer()->DeleteRangeDataByRef(m_SheetAllocator,
                                                                    wRange->AllocatorRef());
            }

            // Erase in SkColRow container ranges
            DetachRangeFromColRow(wRange,true);
            m_AllocatorRange.Delete(wRange->AllocatorRef());
            return(true);
		}
#ifdef debugrange
		cout << "not found " << endl;;
#endif
		return(false);
	}
    
            
            
	void tColRowCellRange::SetRange(tRange* sRange, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex  sRight) {

		EnsureRow(sTop);
		EnsureCol(sLeft);
		EnsureRow(sBottom);
		EnsureCol(sRight);

		if (sRange->TopIndex() != sTop || sRange->LeftIndex() != sLeft
			|| sRange->BottomIndex() != sBottom || sRange->RightIndex() != sRight) {
			DetachRangeFromColRow(sRange, false, true);
		}
    
		sRange->Set(m_SheetAllocator, m_Rows[sTop], m_Cols[sLeft], m_Rows[sBottom], m_Cols[sRight]);
      
		// SetColRow Container ranges =========================================
        // SkColRow is the only class that contains ranges
        AttachRangeToColRow(sRange);
	}

	void tColRowCellRange::DetachRangeFromColRow(tRange* sRange,tBool sEraseColRow, tBool sClean) {
#ifdef debugrangecovered
		cout << "tColRowCellRange::DeleteRangeInColRowContainer " << sRange->StrRef() << ")" << sRange->AllocatorRef() << endl;
#endif
		// SetColRow Container ranges
		tIndex wTop = sRange->TopIndex();
		tIndex wBottom = sRange->AttachBottomBound();
		for (tIndex wRow = wTop; wRow <= wBottom; wRow++) {
			tColRow* wColRow = Row(wRow);
			if (wColRow != nullptr) {
#ifdef debugrangecovered  
				cout << "   Row-> " << wRow << " " << wRangeStr << endl;
#endif
				if (!wColRow->DeleteRange(sRange)) {
                    // See InterfaceCompil->ClearVectorRefAndDeleteDependant();
                    if (!sClean) {  //  Range not on ColRow
                        tString wRangeStr = sRange->StrRef();
                        tStringStream wStream;
                        wStream << "throw: Range " << wRangeStr << " delete on row " << wRow << " don't exist !";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
				};
				if (sEraseColRow && (wColRow->IsEmpty())) {
					DeleteRow(wRow);
				}

			}
		}
		tIndex wLeft = sRange->LeftIndex();
		tIndex wRight = sRange->AttachRightBound();
		for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
			tColRow* wColRow = Col(wCol);
			if (wColRow != nullptr) {
#ifdef debugrangecovered  
				cout << "Delete on col " << wCol << " " << wRangeStr << endl;
#endif
				if (!wColRow->DeleteRange(sRange)) {
                    // See InterfaceCompil->ClearVectorRefAndDeleteDependant();
                    if (!sClean) { //  Range not on ColRow
                        tString wRangeStr = sRange->StrRef();
                        tStringStream wStream;
                        wStream << "Range " << wRangeStr << " delete on col " << wCol << " don't exist !";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
				};
                if (sEraseColRow && (wColRow->IsEmpty())) {
					DeleteCol(wCol);
				}

			}
		}
	}
            
       
    void tColRowCellRange::AttachRangeToColRow(tRange* sRange) {
        const tIndex wTop = sRange->TopIndex();
        const tIndex wBottom = sRange->AttachBottomBound();
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wRight = sRange->AttachRightBound();

        // Add Row — bounded to sheet used extent (IterateBottom).
        for (tIndex wIndex = wTop; wIndex <= wBottom; wIndex++) {
            tColRow* wColRow = EnsureRow(wIndex);
#ifdef  debugcolrow
            cout << "  -->Add Range:" << sRange->StrRef() << "(" << sRange->AllocatorRef() << ") on row " << wIndex << endl;
#endif
            wColRow->AddRange(sRange);
        }
        // Add Col
        for (tIndex wIndex = wLeft; wIndex <= wRight; wIndex++) {
            tColRow* wColRow = EnsureCol(wIndex);
#ifdef  debugcolrow
            cout << "  -->Add Range:" << sRange->StrRef() << " on col " << wIndex << endl;
#endif
            wColRow->AddRange(sRange);
        }
    }

    void tColRowCellRange::AttachFullColumnRangesToNewRow(tColRow* sRow, tIndex sRowIndex) {
        if (sRow == nullptr) {
            return;
        }
        const tIndex wPhysLastCol = tIndex(m_Cols.RealSize());
        for (tIndex wCol = 1; wCol <= wPhysLastCol; wCol++) {
            tColRow* wColRow = Col(wCol);
            if (wColRow == nullptr) {
                continue;
            }
            tColRow::tContainerRange* wContainer = wColRow->ContainerRange();
            if (wContainer == nullptr || wContainer->Container() == nullptr) {
                continue;
            }
            for (tAllocatorRef wRangeRef : *wContainer->Container()) {
                tRange* wRange = Range(wRangeRef);
                if (wRange == nullptr) {
                    continue;
                }
                if (wRange->BottomIndex() >= Cst_MaxRow
                    && wRange->TopIndex() <= sRowIndex
                    && wRange->LeftIndex() <= wCol
                    && wRange->RightIndex() >= wCol) {
                    sRow->AddRange(wRange);
                }
            }
        }
    }

    void tColRowCellRange::AttachFullRowRangesToNewCol(tColRow* sCol, tIndex sColIndex) {
        if (sCol == nullptr) {
            return;
        }
        const tIndex wPhysLastRow = tIndex(m_Rows.RealSize());
        for (tIndex wRow = 1; wRow <= wPhysLastRow; wRow++) {
            tColRow* wRowColRow = Row(wRow);
            if (wRowColRow == nullptr) {
                continue;
            }
            tColRow::tContainerRange* wContainer = wRowColRow->ContainerRange();
            if (wContainer == nullptr || wContainer->Container() == nullptr) {
                continue;
            }
            for (tAllocatorRef wRangeRef : *wContainer->Container()) {
                tRange* wRange = Range(wRangeRef);
                if (wRange == nullptr) {
                    continue;
                }
                if (wRange->RightIndex() >= Cst_MaxCol
                    && wRange->LeftIndex() <= sColIndex
                    && wRange->TopIndex() <= wRow
                    && wRange->BottomIndex() >= wRow) {
                    sCol->AddRange(wRange);
                }
            }
        }
    }
            
    void tColRowCellRange::FindRanges(tRect sRect, tVectorRange* sResult) {
        tColRow::tContainerRange wResultRow;
        for(tIndex wIndexRow=sRect.Top(); wIndexRow<=sRect.Bottom();wIndexRow++) {
            tColRow* wRow=Row(wIndexRow);
            tColRow::tContainerRange*  wContainer;
            if (wRow!=nullptr) {
                wContainer=wRow->ContainerRange();
                for(auto wRangeAllocatorRef : *wContainer->Container()) {
                    wResultRow.InsertClass(wRangeAllocatorRef);
                }
            }
        }
        // If row empty exit
        if (wResultRow.Container()->size()==0) return;
        tColRow::tContainerRange wResultCol;
        for(tIndex wIndexCol=sRect.Left(); wIndexCol<=sRect.Right();wIndexCol++) {
            tColRow* wCol=Col(wIndexCol);
            tColRow::tContainerRange* wContainer;
            if (wCol!=nullptr) {
                wContainer=wCol->ContainerRange();
                for(auto wRangeAllocatorRef : *wContainer->Container()) {
                    wResultCol.InsertClass(wRangeAllocatorRef);
                }
            }
        }
        tColRow::tContainerRange::tResult wResult;
        wResultRow.GetIntersection(&wResultCol, &wResult);
        for(auto wRangeAllocatorRef : wResult) {
            tRange* wRange = Range(wRangeAllocatorRef);
            sResult->push_back(wRange);
        }
    }

            
	void  tColRowCellRange::FindRanges(tCell* sCell, tColRow::tContainerRange::tResult* sResult) {
		sResult->clear();
        if (sCell->Row()==nullptr) return;
        if (sCell->Col()==nullptr) return;
        
#ifdef _DEBUGSK
        assert(sCell->Row()!=nullptr);
        assert(sCell->Col()!=nullptr);
#endif
		// Not attribute
		sCell->Row()->GetIntersection(sCell->Col(), sResult);
#ifdef debugrangecovered
		if (sResult->size() > 0) {
			cout << "GetIndexersection " << endl;
			cout << "Row ";
			sCell->Row()->Debug();
			cout << "Col ";
			sCell->Col()->Debug();
			cout << "Result ";

			for (auto wRangeRef : *sResult) {
				tRange* wRange = Range(wRangeRef);
				cout << wRange->StrRef() << " ";
			}
		cout << endl;
		}
#endif
	}

	void tColRowCellRange::FindRangesCovered(tIndex sRow, tIndex sCol, tColRow::tContainerRange::tResult* sResult) {
		sResult->clear();
		tColRow* wRow = Row(sRow);
		tColRow* wCol = Col(sCol);
		if ((wRow != nullptr) && (wCol != nullptr)) {
			wRow->GetIntersection(wCol, sResult);
		}
	}
    
    void tColRowCellRange::FindRangesCovered(tRect sRect, tVectorRange* sResult) {
        tColRow::tContainerRange wResultRow;
        for(tIndex wIndexRow=sRect.Top(); wIndexRow<=sRect.Bottom();wIndexRow++) {
            tColRow* wRow=Row(wIndexRow);
            tColRow::tContainerRange*  wContainer;
            if (wRow!=nullptr) {
                wContainer=wRow->ContainerRange();
                for(auto wRangeAllocatorRef : *wContainer->Container()) {
                    tRange* wRange = Range(wRangeAllocatorRef);
                    if (wRange->IsMerged()) {
                        wResultRow.InsertClass(wRangeAllocatorRef);
                    }
                }
            }
        }
        // If row empty exit
        if (wResultRow.Container()->size()==0) return;
        tColRow::tContainerRange wResultCol;
        for(tIndex wIndexCol=sRect.Left(); wIndexCol<=sRect.Right();wIndexCol++) {
            tColRow* wCol=Col(wIndexCol);
            tColRow::tContainerRange*  wContainer;
            if (wCol!=nullptr) {
                wContainer=wCol->ContainerRange();
                for(auto wRangeAllocatorRef : *wContainer->Container()) {
                    tRange* wRange = Range(wRangeAllocatorRef);
                    if (wRange->IsMerged()) {
                        wResultCol.InsertClass(wRangeAllocatorRef);
                    }
                }
            }
        }
        tColRow::tContainerRange::tResult wResult;
        wResultRow.GetIntersection(&wResultCol, &wResult);
        for(auto wRangeAllocatorRef : wResult) {
            tRange* wRange = Range(wRangeAllocatorRef);
            sResult->push_back(wRange);
        }
    }
    
    tuple<tString,tRange*> tColRowCellRange::FindRangeDataCovered(tInt sRow, tInt sCol) {
        if (Row(sRow)==nullptr) return(make_tuple("",nullptr));
        if (Col(sCol)==nullptr) return(make_tuple("",nullptr));
        tColRow::tContainerRange::tResult wResult;
        Row(sRow)->GetIntersection(Col(sCol), &wResult);
        tWorkBook* wWorkBook = Sheet()->WorkBook();
        for(auto wRangeRef : wResult) {
            tRange* wRange = Range(wRangeRef);
            if (wRange->IsData()) {
                tString wName=wWorkBook->FindRangeNamed(wRange->AllocatorRef(), wRange->Sheet());
                if (wName=="") {
                  	tStringStream wStream;
					wStream << "throw: Range Data without name !";
					cerr << wStream.str() << endl;
					throw(tExceptionInternalError(wStream.str()));
                }
                return(make_tuple(wName,wRange));
            }
        }
        // Excel OOXML table ref includes the totals row, but we attach IsData to header+data only so SUBTOTAL
        // on the totals row does not self-reference. Structured refs like Table[[#Totals],[Col]] still target
        // cells on BottomIndex()+1; treat that row as covered by the same table when RangeData has totals.
        const tInt wPrevRow = sRow - 1;
        if (wPrevRow >= 1 && Row(wPrevRow) != nullptr && Col(sCol) != nullptr) {
            tColRow::tContainerRange::tResult wPrevResult;
            Row(wPrevRow)->GetIntersection(Col(sCol), &wPrevResult);
            for (auto wRangeRef : wPrevResult) {
                tRange* wRange = Range(wRangeRef);
                if (!wRange->IsData()) {
                    continue;
                }
                tString wName = wWorkBook->FindRangeNamed(wRange->AllocatorRef(), wRange->Sheet());
                if (wName == "") {
                    continue;
                }
                tRangeData* wRangeData = wWorkBook->RangeData(wName);
                if (wRangeData == nullptr || !wRangeData->HasTotals()) {
                    continue;
                }
                if (sRow != (tInt)wRange->BottomIndex() + 1) {
                    continue;
                }
                return (make_tuple(wName, wRange));
            }
        }
        return(make_tuple("",nullptr));
    }
  
  
    tRange* tColRowCellRange::MergedRange(tIndex sRow, tIndex sCol) {
        tColRow::tContainerRange::tResult wContainerRange;
        FindRangesCovered(sRow,sCol, &wContainerRange);
        for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
            tRange* wRange = Range(wRangeAllocatorRef);
            if (wRange->IsMerged()) return(wRange);
        }
        return(nullptr);
    }

	tRange* tColRowCellRange::Range(tAllocatorRef sAllocatorRef) { return(m_AllocatorRange(sAllocatorRef)); }

    tBool tColRowCellRange::IsDeletedRange(tAllocatorRef sAllocatorRef) {
        return(m_AllocatorRange.IsNullptr(sAllocatorRef));
    }
    
	void tColRowCellRange::RenumRow(tIndex sRow) {
        tIndex wPosition = sRow;
        while(wPosition <(tIndex) m_Rows.Size()) {
			tColRow* wColRow = Row(wPosition);
			if (wColRow != nullptr) {
                wColRow->m_Children.clear();
                wColRow->Index(wPosition);
                if (wColRow->m_ParentRef!=0) {
                    tColRow* wColRowParent=wColRow->ColRowParent(this,true);
                    // Drop dangling / self ParentRef so GrandParent/Deep cannot hang.
                    if (wColRowParent==nullptr || wColRowParent==wColRow) {
                        wColRow->ParentRef(0);
                    } else {
                        wColRowParent->m_Children.push_back(m_Rows[wPosition]);
                    }
                }
            }
            wPosition++;
		}
	}

	void tColRowCellRange::RenumCol(tIndex sCol) {
        tIndex wPosition = sCol;
        while(wPosition <(tIndex) m_Cols.Size()) {
            tColRow* wColRow = Col(wPosition);
            if (wColRow != nullptr) {
                wColRow->m_Children.clear();
                wColRow->Index(wPosition);
                if (wColRow->m_ParentRef!=0) {
                    tColRow* wColRowParent=wColRow->ColRowParent(this,false);
                    if (wColRowParent==nullptr || wColRowParent==wColRow) {
                        wColRow->ParentRef(0);
                    } else {
                        wColRowParent->m_Children.push_back(m_Cols[wPosition]);
                    }
                }
            }
            wPosition++;
        }
	}

    void tColRowCellRange::SyncTreeParentRefsFromChildren(tBool sIsRow) {
        // Parent "c" lists without child "p" survive in memory until Renum clears
        // m_Children; restore ParentRef so Renum can rebuild the outline.
        const tIndex wSize = sIsRow ? (tIndex)m_Rows.Size() : (tIndex)m_Cols.Size();
        for (tIndex wPosition = 1; wPosition < wSize; wPosition++) {
            tColRow* wColRow = ColRowByIndex(wPosition, sIsRow);
            if (wColRow == nullptr || wColRow->m_Children.empty()) {
                continue;
            }
            tAllocatorRef wSelfRef = sIsRow ? m_Rows[wPosition] : m_Cols[wPosition];
            if (wSelfRef == 0) {
                continue;
            }
            for (auto wChildRef : wColRow->m_Children) {
                tColRow* wChild = ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wChild == nullptr) {
                    continue;
                }
                if (wChild->m_ParentRef == 0) {
                    wChild->ParentRef(wSelfRef);
                }
            }
        }
    }
        
    tRect tColRowCellRange::GetRectDeleteAreaCol(tIndex sCol,tIndex sSize) {
        return(tRect(1,sCol, tIndex(m_Rows.RealSize()),sCol+sSize-1));
    }
    
    tRect tColRowCellRange::GetRectDeleteAreaRow(tIndex sRow,tIndex sSize) {
        tRect wRectDeleteArea(sRow, 1, sRow+sSize-1 , 1);
        // Search right position for make RectDeleteArea
        for (tIndex wRow = sRow; wRow < sRow + sSize; wRow++) {
            tSparseArrayCell* wSparseArrayCell=m_Cell2D(wRow);
            if (wSparseArrayCell != nullptr) {
                if ((tIndex) wSparseArrayCell->RealSize() > wRectDeleteArea.Right()) {
                    wRectDeleteArea.Right(tIndex(wSparseArrayCell->RealSize()));
                }
            }
        }
        
    
        //wRectDeleteArea.Right(tIndex(m_Cols.RealSize()));
        return(wRectDeleteArea);
    }

    // ItemCF ===========================================================
    tAllocatorRef tColRowCellRange::AllocItemCF() {
        tItemCF* wItemCF;
        tAllocatorRef wAllocatorRef;
        tie(wAllocatorRef, wItemCF) = m_AllocatorItemCF.Alloc();
        return(wAllocatorRef);
    }

    tItemCF* tColRowCellRange::ItemCF(tAllocatorRef sAllocatorRef) {
        return(m_AllocatorItemCF(sAllocatorRef));
    }

    void tColRowCellRange::DeleteItemCF(tAllocatorRef sAllocatorRef) {
        m_AllocatorItemCF.Delete(sAllocatorRef);
    }

    void tColRowCellRange::DecNbCells(tColRow* sColRow,tBool sIsRow) {
        sColRow->DecNbCells();
        tIndex wIndex=sColRow->Index();
        // ? Delete Row or Col
        if (sColRow->IsEmpty()) {
            if (sIsRow) {
#ifdef debugcolrow
                cout << " Delete Row " << wIndex << endl;
#endif
                // Delete Row
                DeleteRow(wIndex);
                // Cell (col)
                m_Cell2D.Delete(wIndex);
            } else {
#ifdef debugcolrow
                cout << " Delete Col " << wIndex << endl;
#endif
                // Delete Row
                DeleteCol(wIndex);
            }
        }
    }
        
   
	void tColRowCellRange::DoInsertRow(tIndex sRow, tIndex sSize,tSaveSelectErase* sSaveSelectErase) {
        InvalidateExtentCache();
#ifdef debugcolrow
            cout << "tColRowCellRange::DoInsertRow(" << sRow << "," << sSize << ")" << endl;
#endif
        if (sSaveSelectErase != nullptr) {
            sSaveSelectErase->ColRowCellRange(this);
        }
		// 1 Get Range to copy
		tVectorRange wVectorRange;
        tColRow::tContainerRange* wContainerRange = nullptr;
		tColRow* wColRow = Row(sRow);
		if (wColRow != nullptr) {
			wContainerRange = wColRow->ContainerRange();
			tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
			tColRow::tContainerRange::tIteratorClass wIterator;
			for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
				tRange* wRange = m_AllocatorRange(*wIterator);
                    if (wRange!=nullptr) {
#ifdef debugcolrow
                        cout << " -->Range " << wRange->Debug();
#endif
                        if (wRange->TopIndex() < sRow) {
                            wVectorRange.push_back(wRange);
                        }
                    }
			}
		}
        
        // 2 Is Tree
        tAllocatorRef wParentRef=0;
        if (sSaveSelectErase==nullptr) {
            wColRow=Row(sRow);
            if (wColRow!=nullptr) {
                wParentRef=wColRow->ParentRef();
            }
        }
        
		// 3 Insert Rows
		m_Rows.Insert(sRow, sSize);
        
        // 4 Renum Sub Tree — rebuild from tree root so ancestors' m_Children are cleared
        if ((sSaveSelectErase==nullptr) && (wParentRef!=0)) {
            tColRow* wColRowParent=ColRowByAllocatorRef(wParentRef, true);
            for(tIndex wIndex=sRow;wIndex<sRow+sSize;wIndex++) {
                tColRow* wColRow=EnsureRow(wIndex);
                wColRow->ParentRef(wParentRef);
            }
            RenumRow(GrandParent(wColRowParent->Index(), true));
        } else {
            RenumRow(sRow);
        }
		// 5 Recopy range
		if (!wVectorRange.empty()) {
			// Set empty Value for new insert line ===========================
			for (tIndex wIndex = sRow; wIndex < sRow + sSize; wIndex++) {
				tColRow* wColRow = EnsureRow(wIndex);
				for (auto wRange : wVectorRange) {
#ifdef debugcolrow
                    cout << "  -->Add Range" << wRange->StrRef() << " on row " << wIndex;
#endif
					wColRow->AddRange(wRange);

				}
			}
		}
    
		// 6 Move cells
		m_Cell2D.Insert(sRow, sSize);
        // m_Cell2D.Insert shifts slots without updating tCell / ClassAttribute roots.
        RelocateOccupiedCellsFrom(this, m_Cell2D, sRow + sSize);

        // 7 copy Format + row height
        // Duplicate cell CSS onto inserted rows (cleared by Insert). Horizontal seams: strip top/bottom.
        // When origin has border-bottom, also strip left/right (vertical seam at bottom edge).
        // Source row: row above the inserted block (sRow - 1) — Excel xlFormatFromLeftOrAbove.
        // Also copy row height from that origin (Excel inserts inherit RowHeight, not just cell CSS).
        // Skip when undoing a row delete, or insert at row 1 (no format duplication on first line).
        if (sSaveSelectErase == nullptr && sRow > 1) {
            const tIndex wOriginRow = sRow - 1;
            if (ValidRow(wOriginRow)) {
                tWorkBook* wWorkBook = Sheet()->WorkBook();
                tInsertCellCssCache wCssCache;
                const tBool wValidBelowIns = ValidRow(sRow + sSize);
                tColRow* wOriginColRow = Row(wOriginRow);
                // Row height is stored on tColRow::m_Size (-1 = workbook default). New slots are empty.
                if (wOriginColRow != nullptr) {
                    const tDouble wOriginSize = wOriginColRow->Size();
                    for (tIndex wRow = sRow; wRow < sRow + sSize; wRow++) {
                        tColRow* wDestColRow = EnsureRow(wRow);
                        if (wDestColRow != nullptr) {
                            wDestColRow->Size(wOriginSize);
                        }
                    }
                }
                // Row-level Css on inserted rows (full-sheet insert only): sparse cells still merge sheet→col→row→cell.
                {
                    if (wOriginColRow != nullptr && wOriginColRow->Css() != 0) {
                        const tFormatRef wRowCssOrigin = wOriginColRow->Css();
                        const tFormatRef wRowCss = CachedStripCssForInsertRow(
                            wWorkBook, wRowCssOrigin, wValidBelowIns, wCssCache);
                        for (tIndex wRow = sRow; wRow < sRow + sSize; wRow++) {
                            tColRow* wDestColRow = EnsureRow(wRow);
                            if (wDestColRow != nullptr) {
                                AssignInsertColRowCss(wWorkBook, wDestColRow, wRowCssOrigin, wRowCss, wCssCache);
                            }
                        }
                    }
                }
                for (tIndex wRow = sRow; wRow < sRow + sSize; wRow++) {
                    for (tIndex wCol = 1; wCol <= m_Cols.Size(); wCol++) {
                        tCell* wCellOrigin = Cell(wOriginRow, wCol);
                        if (wCellOrigin != nullptr && wCellOrigin->Css() != 0) {
                            tCell* wCell = EnsureCell(wRow, wCol);
                            if (wCell != nullptr) {
                                const tFormatRef wCssOrigin = wCellOrigin->Css();
                                const tFormatRef wCss = CachedStripCssForInsertRow(
                                    wWorkBook, wCssOrigin, wValidBelowIns, wCssCache);
                                AssignInsertCellCss(wWorkBook, wCell, wCssOrigin, wCss, wCssCache);
                            }
                        }
                    }
                }
            }
        }

        // 8 Add Volatile Row in path
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        AddPathVolatile(&wContainerPath,tVolatile::t_Row);
        wContainerPath.EndCalculate();
	}
        
    void tColRowCellRange::DoInsertRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells,tBool sPreserveSpanningRanges) {
#ifdef debugcolrow
            cout << "tColRowCellRange::DoInsertRow(" << sRect.StrRef()  << ")" << endl;
#endif
        sSaveSelectErase->ColRowCellRange(this);
        tRect wRectDeleteArea=GetRectDeleteAreaCol(sRect.Left(),sRect.Right()-sRect.Left()+1);
        
#ifdef debugcolrow
        for(tIndex wRow=wRectDeleteArea.Top(); wRow<=wRectDeleteArea.Bottom();wRow++) {
          tColRow* wColRow=Row(wRow);
          if (wColRow!=nullptr)
              cout << "Row " << wRow << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        wRectDeleteArea.Top(sRect.Top());
        // Insert Cell in Rect
        // Use sRectMoveCells if provided (for undo with rebased positions), otherwise use sRect
        const tRect& wRectForMove = (sRectMoveCells != nullptr) ? *sRectMoveCells : sRect;
        tRect wRectForMoveNonConst(wRectForMove); // Create non-const copy to call non-const methods
        // Loop on Bottom on sheet + Height  to  bottom on sRect
        // Move from bottom to top to avoid overwrite while shifting down
        for(tIndex wRowDest=wRectDeleteArea.Bottom()+wRectForMoveNonConst.Height(); wRowDest > wRectForMoveNonConst.Bottom();wRowDest--) {
            tIndex   wRowOrigin=wRowDest-wRectForMoveNonConst.Height();
            tColRow* wColRowOrigin=EnsureRow(wRowOrigin);
            tColRow* wColRowDest=EnsureRow(wRowDest);
#ifdef debugcolrow
            cout << "Move Cells of  " << wRowOrigin << "to " << wRowDest << endl;
#endif
            for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right();wCol++) {
                // Move Cell wRowOrigin -> wRowDest
                tAllocatorRef wAllocatorRef=CellAllocatorRef(wRowOrigin,wCol);
                SetCellAllocatorRef(wAllocatorRef,wRowDest,wCol);
                if (wAllocatorRef!=0) {
                    // Dec on Origin
                    DecNbCells(wColRowOrigin,true);
                    // Inc  On Destination
                    wColRowDest->IncNbCells();
                   
                    // AnchorCell
                    tCell* wCell=m_AllocatorCell(wAllocatorRef);
                    wCell->SetColRow(m_Rows[wRowDest],m_Cols[wCol]);
                }
                // Raz Origin
                SetCellAllocatorRef(0, wRowOrigin, wCol);
            }
#ifdef debugcolrow
#ifdef checksp
            if (wColRowDest!=nullptr) cout << "wColRowwDest " << wRowDest << " NbCells=" << wColRowDest->NbCells() << endl;
            if (wColRowOrigin!=nullptr) cout << "wColRowOrigin " << wRowOrigin << " NbCells=" << wColRowOrigin->NbCells() << endl;
#endif
#endif
        }
        
#ifdef debugcolrow
        for(tIndex wRow=wRectDeleteArea.Top(); wRow<=wRectDeleteArea.Bottom()+sRect.Height();wRow++) {
          tColRow* wColRow=Row(wRow);
          if (wColRow!=nullptr)
              cout << "Row " << wRow << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        
            
        // Get Range
        tColRow::tContainerRange wResultRow;
        for(tIndex wIndexRow=sRect.Top(); wIndexRow<=sRect.Bottom();wIndexRow++) {
            tColRow* wRow=Row(wIndexRow);
            if (wRow!=nullptr) {
                tColRow::tContainerRange* wContainerRange = wRow->ContainerRange();
                tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
                tColRow::tContainerRange::tIteratorClass wIterator;
                for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
                    if (!(wResultRow.Exist(*wIterator))) {
                        tRange* wRange = m_AllocatorRange(*wIterator);
                   
                        // Only shift ranges overlapping the insert column band (same rule as cell move / AddUniqueRange on delete).
                        if (wRange->IntersectRect(&wRectDeleteArea)) {
                            wResultRow.InsertClass(wRange->AllocatorRef());
                            sSaveSelectErase->AddRange(wRange);
                        }
                    }
                }
            }
        }
        // Remove Range =======================================================
        tColRow::tContainerRange::tIteratorClass wIterator;
        // Detach Range
        for (wIterator = wResultRow.Container()->begin(); wIterator != wResultRow.Container()->end(); wIterator++) {
            tRange* wRange = m_AllocatorRange(*wIterator);
                                                                             
            DetachRangeFromColRow(wRange,false);
        }


        // Replace Range ======================================================
        for (wIterator = wResultRow.Container()->begin(); wIterator != wResultRow.Container()->end(); wIterator++) {
            tRange* wRange = m_AllocatorRange(*wIterator);
            // A range that extends past the rect on the axis perpendicular to the shift
            // (columns here) must NOT be grown/shifted; it is only re-attached as-is.
            const tBool wSpansPerpendicular =
                (wRange->LeftIndex() < sRect.Left() || wRange->RightIndex() > sRect.Right());
            // Two situations require this preservation:
            //  - Undo of a delete-by-rect: the delete kept the range as-is
            //    (tSaveSelectErase::DeleteColRow), so growing it back would grow the range by
            //    the rect height (e.g. a conditional format on B4:I28 would come back as B4:I29).
            //  - Forward partial insert of a conditional format: a "shift down" on e.g. D7:D7
            //    only moves column D, so Excel does not grow a CF applied to B4:I28 into B4:I29.
            //    We keep it B4:I28 rather than over-extending the whole multi-column range.
            if (wSpansPerpendicular
                && (sPreserveSpanningRanges || wRange->IsConditionalFormat())) {
                SetRange(wRange, wRange->TopIndex(), wRange->LeftIndex(), wRange->BottomIndex(), wRange->RightIndex());
                continue;
            }
            // Is Before
            tInt wTop=wRange->TopIndex();
            if (wTop>=sRect.Top()) {
                wTop+=sRect.Height();
            }
            
            
            SetRange(wRange,wTop, wRange->LeftIndex(), wRange->BottomIndex()+sRect.Height(), wRange->RightIndex());
        }
        
        // Duplicate cell CSS only (do not touch Row-level Css). Horizontal span: wRectDeleteArea columns.
        // Skip insert at row 1 (no format duplication on first line).
        {
            const tIndex wInsTop = wRectForMoveNonConst.Top();
            const tIndex wInsHeight = (tIndex)wRectForMoveNonConst.Height();
            const tIndex wBelowIns = wInsTop + wInsHeight;
            if (wInsTop > 1) {
                const tIndex wOriginRow = wInsTop - 1;
                if (ValidRow(wOriginRow)) {
                tWorkBook* wWorkBook = Sheet()->WorkBook();
                tInsertCellCssCache wCellCssCache;
                const tBool wValidBelowIns = ValidRow(wBelowIns);
                for (tIndex wRow = wInsTop; wRow < wInsTop + wInsHeight; wRow++) {
                    for (tIndex wCol = wRectDeleteArea.Left(); wCol <= wRectDeleteArea.Right(); wCol++) {
                        tCell* wCellOrigin = Cell(wOriginRow, wCol);
                        if (wCellOrigin != nullptr && wCellOrigin->Css() != 0) {
                            tCell* wCell = EnsureCell(wRow, wCol);
                            if (wCell != nullptr) {
                                const tFormatRef wCssOrigin = wCellOrigin->Css();
                                const tFormatRef wCss = CachedStripCssForInsertRow(
                                    wWorkBook, wCssOrigin, wValidBelowIns, wCellCssCache);
                                AssignInsertCellCss(wWorkBook, wCell, wCssOrigin, wCss, wCellCssCache);
                            }
                        }
                    }
                }
                }
            }
        }
        
#ifdef checksp
        // Undo insert: saved cells are restored after the shift; mid-insert graph is not checksp-valid yet.
        if (sSaveSelectErase->IsEmpty()) {
            Check();
        }
#endif

        // MoveCeel
        // Add Volatile Row in path
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        AddPathVolatile(&wContainerPath,tVolatile::t_Row);
        wContainerPath.EndCalculate();
    }
        
    void tColRowCellRange::DoInsertColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells,tBool sPreserveSpanningRanges) {
#ifdef debugcolrow
            cout << "tColRowCellRange::DoInsertColByRect(" << sRect.StrRef()  << ")" << endl;
#endif
        // cout << "DoInsertColByRect - Rect: Left=" << sRect.Left() << " Right=" << sRect.Right() << " Width=" << sRect.Width() << endl;
        sSaveSelectErase->ColRowCellRange(this);
        tRect wRectDeleteArea=GetRectDeleteAreaRow(sRect.Top(),sRect.Bottom()-sRect.Top()+1);
        
#ifdef debugcolrow
        for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right();wCol++) {
          tColRow* wColRow=Col(wCol);
          cout << "Col " << wCol << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        wRectDeleteArea.Left(sRect.Left());
        // Insert Cell in Rect
        // Use sRectMoveCells if provided (for undo with rebased positions), otherwise use sRect
        const tRect& wRectForMove = (sRectMoveCells != nullptr) ? *sRectMoveCells : sRect;
        tRect wRectForMoveNonConst(wRectForMove); // Create non-const copy to call non-const methods
        // Loop on Right on sheet + Width  to  right on sRect
        // Move from right to left to avoid overwrite while shifting right
        // cout << "DoInsertColByRect - Moving cells: wRectDeleteArea.Right()=" << wRectDeleteArea.Right() 
        //      << " sRect.Right()=" << sRect.Right() << " sRect.Width()=" << sRect.Width() << endl;
        for(tIndex wColDest=wRectDeleteArea.Right()+wRectForMoveNonConst.Width(); wColDest > wRectForMoveNonConst.Right();wColDest--) {
            tIndex   wColOrigin=wColDest-wRectForMoveNonConst.Width();
            // cout << "DoInsertColByRect - Move from col " << wColOrigin << " to col " << wColDest << endl;
            tColRow* wColRowOrigin=EnsureCol(wColOrigin);
            tColRow* wColRowDest=EnsureCol(wColDest);
#ifdef debugcolrow
            cout << "Move Cells of  " << wColOrigin << "to " << wColDest << endl;
#endif
            for(tIndex wRow=wRectDeleteArea.Top(); wRow<=wRectDeleteArea.Bottom();wRow++) {
                // Move Cell wColOrigin -> wColDest
                tAllocatorRef wAllocatorRef=CellAllocatorRef(wRow,wColOrigin);
                SetCellAllocatorRef(wAllocatorRef,wRow,wColDest);
                if (wAllocatorRef!=0) {
                    // Dec on Origin
                    DecNbCells(wColRowOrigin,false);
                    // Inc  On Destination
                    wColRowDest->IncNbCells();
                   
                    // AnchorCell
                    tCell* wCell=m_AllocatorCell(wAllocatorRef);
                    wCell->SetColRow(m_Rows[wRow],m_Cols[wColDest]);
                }
                // Raz Origin
                SetCellAllocatorRef(0, wRow, wColOrigin);
            }
#ifdef debugcolrow
#ifdef checksp
            if (wColRowDest!=nullptr) cout << "wColRowDest " << wColDest << " NbCells=" << wColRowDest->NbCells() << endl;
            if (wColRowOrigin!=nullptr) cout << "wColRowOrigin " << wColOrigin << " NbCells=" << wColRowOrigin->NbCells() << endl;
#endif
#endif
        }
        
#ifdef debugcolrow
        for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right()+sRect.Width();wCol++) {
          tColRow* wColRow=Col(wCol);
          if (wColRow!=nullptr)
              cout << "Col " << wCol << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        
            
        // Get Range
        tColRow::tContainerRange wResultCol;
        for(tIndex wIndexCol=sRect.Left(); wIndexCol<=sRect.Right();wIndexCol++) {
            tColRow* wCol=Col(wIndexCol);
            if (wCol!=nullptr) {
                tColRow::tContainerRange* wContainerRange = wCol->ContainerRange();
                tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
                tColRow::tContainerRange::tIteratorClass wIterator;
                for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
                    if (!(wResultCol.Exist(*wIterator))) {
                        tRange* wRange = m_AllocatorRange(*wIterator);
                   
                        // Only shift ranges overlapping the insert row band (same rule as cell move / AddUniqueRange on delete).
                        if (wRange->IntersectRect(&wRectDeleteArea)) {
                            wResultCol.InsertClass(wRange->AllocatorRef());
                            sSaveSelectErase->AddRange(wRange);
                        }
                    }
                }
            }
        }
        // Remove Range =======================================================
        tColRow::tContainerRange::tIteratorClass wIterator;
        // Detach Range
        for (wIterator = wResultCol.Container()->begin(); wIterator != wResultCol.Container()->end(); wIterator++) {
            tRange* wRange = m_AllocatorRange(*wIterator);
                                                                             
            DetachRangeFromColRow(wRange,false);
        }


        // Replace Range ======================================================
        for (wIterator = wResultCol.Container()->begin(); wIterator != wResultCol.Container()->end(); wIterator++) {
            tRange* wRange = m_AllocatorRange(*wIterator);
            // A range that extends past the rect on the axis perpendicular to the shift
            // (rows here) must NOT be widened/shifted; it is only re-attached as-is.
            const tBool wSpansPerpendicular =
                (wRange->TopIndex() < sRect.Top() || wRange->BottomIndex() > sRect.Bottom());
            // Two situations require this preservation:
            //  - Undo of a delete-by-rect: the delete kept the range as-is
            //    (tSaveSelectErase::DeleteColRow), so widening it back would grow the range by
            //    the rect width (e.g. a conditional format on B4:I28 would come back as B4:J28).
            //  - Forward partial insert of a conditional format: a "shift right" on e.g. D7:D7
            //    only moves row 7, so Excel does not grow a CF applied to B4:I28 into B4:J28.
            //    We keep it B4:I28 rather than over-extending the whole multi-row range.
            // Data ranges keep their historical shifting behavior so formula dependencies stay
            // consistent with SyncRangeDataAfterColumnInsert below.
            if (wSpansPerpendicular
                && (sPreserveSpanningRanges || wRange->IsConditionalFormat())) {
                SetRange(wRange, wRange->TopIndex(), wRange->LeftIndex(), wRange->BottomIndex(), wRange->RightIndex());
                continue;
            }
            // Is Before
            tInt wLeft=wRange->LeftIndex();
            if (wLeft>=sRect.Left()) {
                wLeft+=sRect.Width();
            }
            
            
            SetRange(wRange,wRange->TopIndex(), wLeft, wRange->BottomIndex(), wRange->RightIndex()+sRect.Width());
            if (wRange->IsData()) {
                Sheet()->WorkBook()->SyncRangeDataAfterColumnInsert(
                    wRange, sRect.Left(), sRect.Width());
            }
        }
        
        // Duplicate cell CSS only (do not touch Col-level Css). Vertical span: wRectDeleteArea rows.
        // Skip insert at column 1 (no format duplication on first column).
        {
            const tIndex wInsLeft = wRectForMoveNonConst.Left();
            const tIndex wInsWidth = (tIndex)wRectForMoveNonConst.Width();
            const tIndex wRightIns = wInsLeft + wInsWidth;
            if (wInsLeft > 1) {
                const tIndex wOriginCol = wInsLeft - 1;
                if (ValidCol(wOriginCol)) {
                tWorkBook* wWorkBook = Sheet()->WorkBook();
                tInsertCellCssCache wCellCssCache;
                const tBool wValidRightIns = ValidCol(wRightIns);
                for (tIndex wCol = wInsLeft; wCol < wInsLeft + wInsWidth; wCol++) {
                    for (tIndex wRow = wRectDeleteArea.Top(); wRow <= wRectDeleteArea.Bottom(); wRow++) {
                        tCell* wCellOrigin = Cell(wRow, wOriginCol);
                        if (wCellOrigin != nullptr && wCellOrigin->Css() != 0) {
                            tCell* wCell = EnsureCell(wRow, wCol);
                            if (wCell != nullptr) {
                                const tFormatRef wCssOrigin = wCellOrigin->Css();
                                const tFormatRef wCss = CachedStripCssForInsertCol(
                                    wWorkBook, wCssOrigin, wValidRightIns, wCellCssCache);
                                AssignInsertCellCss(wWorkBook, wCell, wCssOrigin, wCss, wCellCssCache);
                            }
                        }
                    }
                }
                }
            }
        }
        
#ifdef checksp
        if (sSaveSelectErase->IsEmpty()) {
            Check();
        }
#endif

        // MoveCell
        // Add Volatile Col in path
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        AddPathVolatile(&wContainerPath,tVolatile::t_Col);
        wContainerPath.EndCalculate();
    }
        
    void tColRowCellRange::DoDeleteRowByRect(tSaveSelectErase *sSaveSelectErase,tRect sRect,tBool sIsUndoInsert,const tRect* sRectMoveCells) {
#ifdef debugcolrow
            cout << "tColRowCellRange::DoDeleteRowByRect(" << sRect.StrRef()  << ")" << endl;
#endif
        sSaveSelectErase->RectDeleteArea(sRect);
        sSaveSelectErase->ParseSelect(sRect.StrRef());
        tRect wRectDeleteArea=GetRectDeleteAreaCol(sRect.Left(),sRect.Right()-sRect.Left()+1);
        wRectDeleteArea.Top(sRect.Top());
        // Set Range
        tColRow::tContainerRange wResultRow;
        for(tIndex wIndexRow=wRectDeleteArea.Top(); wIndexRow<=wRectDeleteArea.Bottom();wIndexRow++) {
            tColRow* wRow=Row(wIndexRow);
            if (wRow!=nullptr) {
              
                tColRow::tContainerRange* wContainerRange = wRow->ContainerRange();
                tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
                tColRow::tContainerRange::tIteratorClass wIterator;
                for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
                    if (!(wResultRow.Exist(*wIterator))) {
                        wResultRow.InsertClass(*wIterator);
                        tRange* wRange = m_AllocatorRange(*wIterator);
 
                        // Only if wRange itersect  Rect
                        if (wRange->IntersectRect(&wRectDeleteArea)) {
    #ifdef debugcolrow
                            cout << " Add Range In sSaveDelectErase " << wRange->StrRef() << endl;
    #endif
                            sSaveSelectErase->AddUniqueRange(wRange);
                        }
                    }
                }
            }
        }
        
        if (!sIsUndoInsert) {
            sSaveSelectErase->DeleteColRow(true, m_SheetAllocator, sRect.Top(), sRect.Height(),true);
        };
        // Move Cell ==========================================================
#ifdef debugcolrow
        for(tIndex wRow=wRectDeleteArea.Top(); wRow<=wRectDeleteArea.Bottom();wRow++) {
          tColRow* wColRow=Row(wRow);
          if (wColRow!=nullptr)
              cout << "Row " << wRow << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
      
        // Move  Cell in Rect
        // Use sRectMoveCells if provided (for undo with rebased positions), otherwise use sRect
        const tRect& wRectForMove = (sRectMoveCells != nullptr) ? *sRectMoveCells : sRect;
        tRect wRectForMoveNonConst(wRectForMove); // Create non-const copy to call non-const methods
        // Loop from bottom to top to shift all rows up after deletion
        for(tIndex wRowOrigin=wRectForMoveNonConst.Bottom()+1; wRowOrigin <= wRectDeleteArea.Bottom()+wRectForMoveNonConst.Height(); wRowOrigin++) {
           
            tIndex   wRowDest=wRowOrigin-wRectForMoveNonConst.Height();
            tColRow* wColRowDest=EnsureRow(wRowDest);
            tColRow* wColRowOrigin=EnsureRow(wRowOrigin);
#ifdef debugcolrow
            cout << "Move " << wRowOrigin << " --> " << wRowDest << endl;
#endif
            for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right();wCol++) {
                tAllocatorRef wRefDest = CellAllocatorRef(wRowDest, wCol);
                tAllocatorRef wRefOrigin = CellAllocatorRef(wRowOrigin, wCol);
                // Undo insert deletes rows by shifting; inserted-band cells must release Css (SetCellAllocatorRef overwrites without DeleteCell).
                if (wRefDest != 0 && wRefDest != wRefOrigin) {
                    DeleteCell(wRowDest, wCol);
                }
                const tBool wDestEmptyBeforePlace = (CellAllocatorRef(wRowDest, wCol) == 0);
                SetCellAllocatorRef(wRefOrigin, wRowDest, wCol);
                if (wRefOrigin != 0) {
                    SetCellAllocatorRef(0, wRowOrigin, wCol);
                    DecNbCells(wColRowOrigin, true);
                    DecNbCells(Col(wCol), false);
                    if (wDestEmptyBeforePlace) {
                        wColRowDest->IncNbCells();
                        if (Col(wCol) == nullptr) EnsureCol(wCol);
                        Col(wCol)->IncNbCells();
                    }
                }
            }
#ifdef debugcolrow
#ifdef checksp
            if (wColRowDest!=nullptr)
              cout << "wColRowwDest " << wRowDest << " NbCells=" << wColRowDest->NbCells() << endl;
            if (wColRowOrigin!=nullptr)
              cout << "wColRowOrigin " << wRowOrigin << " NbCells=" << wColRowOrigin->NbCells() << endl;
#endif
#endif
        }
        
        
#ifdef debugcolrow
        for(tIndex wRow=wRectDeleteArea.Top(); wRow<=wRectDeleteArea.Bottom();wRow++) {
          tColRow* wColRow=Row(wRow);
          if (wColRow!=nullptr)
              cout << "Row " << wRow << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        
        
        if (sIsUndoInsert) {
            sSaveSelectErase->ApplyColRowUndoInsertRect();
        } else {
            sSaveSelectErase->DeleteRangeInColRowContainer();
            sSaveSelectErase->ApplyColRow();
        }
        sSaveSelectErase->CalculateDo(this,tVolatile::t_Row);
#ifdef debugcolrow
        cout << sSaveSelectErase->Debug();
#endif
#ifdef checksp
        Check();
#endif
    }
        
    void tColRowCellRange::DoDeleteColByRect(tSaveSelectErase *sSaveSelectErase,tRect sRect,tBool sIsUndoInsert,const tRect* sRectMoveCells) {
#ifdef debugcolrow
            cout << "tColRowCellRange::DoDeleteColByRect(" << sRect.StrRef()  << ")" << endl;
#endif
        // Save Rect Delete Area ==============================
        sSaveSelectErase->RectDeleteArea(sRect);
        sSaveSelectErase->ParseSelect(sRect.StrRef());
        
        tRect wRectDeleteArea=GetRectDeleteAreaCol(sRect.Left(),sRect.Right()-sRect.Left()+1);
        wRectDeleteArea.Left(sRect.Left());
        // Set Range
        tColRow::tContainerRange wResultCol;
        for(tIndex wIndexCol=wRectDeleteArea.Left(); wIndexCol<=wRectDeleteArea.Right();wIndexCol++) {
            tColRow* wCol=Col(wIndexCol);
            if (wCol!=nullptr) {
              
                tColRow::tContainerRange* wContainerRange = wCol->ContainerRange();
                tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
                tColRow::tContainerRange::tIteratorClass wIterator;
                for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
                    if (!(wResultCol.Exist(*wIterator))) {
                        wResultCol.InsertClass(*wIterator);
                        tRange* wRange = m_AllocatorRange(*wIterator);
                        // Only if wRange itersect  Rect
                        if (wRange->IntersectRect(&wRectDeleteArea)) {
#ifdef debugcolrow
                            cout << " Add Range In sSaveDelectErase " << wRange->StrRef() << endl;
#endif
                            sSaveSelectErase->AddUniqueRange(wRange);
                        }
  
                    }
                }
            }
        }
        if (!sIsUndoInsert) {
            sSaveSelectErase->DeleteColRow(false, m_SheetAllocator, sRect.Left(), sRect.Width(),true);
        };
    
        
        // Move Cell ==========================================================
#ifdef debugcolrow
        for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right();wCol++) {
          tColRow* wColRow=Col(wCol);
          if (wColRow!=nullptr)
              cout << "Col " << wCol << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
      
        // Move  Cell in Rect
        // Use sRectMoveCells if provided (for undo with rebased positions), otherwise use sRect
        const tRect& wRectForMove = (sRectMoveCells != nullptr) ? *sRectMoveCells : sRect;
        tRect wRectForMoveNonConst(wRectForMove); // Create non-const copy to call non-const methods
        // cout << "DoDeleteColByRect - Using rect for move: Left=" << wRectForMoveNonConst.Left() << " Right=" << wRectForMoveNonConst.Right() << " Width=" << wRectForMoveNonConst.Width() << endl;
        //cout << "m_Cols.RealSize()=" << m_Cols.RealSize() << endl;
        // Loop from right to left to shift all columns left after deletion
        
        for(tIndex wColOrigin=wRectForMoveNonConst.Right()+1; wColOrigin <= tIndex(m_Cols.RealSize())+wRectForMoveNonConst.Width(); wColOrigin++) {
           
            tIndex   wColDest=wColOrigin-wRectForMoveNonConst.Width();
            tColRow* wColRowDest=EnsureCol(wColDest);
            tColRow* wColRowOrigin=EnsureCol(wColOrigin);
#ifdef debugcolrow
            cout << "Move " << wColOrigin << " --> " << wColDest << endl;
#endif
            for(tIndex wRow=sRect.Top(); wRow<=sRect.Bottom();wRow++) {
                tAllocatorRef wRefDest = CellAllocatorRef(wRow, wColDest);
                tAllocatorRef wRefOrigin = CellAllocatorRef(wRow, wColOrigin);
                // Undo insert deletes cols by shifting; inserted-band cells must release Css (SetCellAllocatorRef overwrites without DeleteCell).
                if (wRefDest != 0 && wRefDest != wRefOrigin) {
                    DeleteCell(wRow, wColDest);
                }
                const tBool wDestEmptyBeforePlace = (CellAllocatorRef(wRow, wColDest) == 0);
                SetCellAllocatorRef(wRefOrigin, wRow, wColDest);
                if (wRefOrigin != 0) {
                    SetCellAllocatorRef(0, wRow, wColOrigin);
                    DecNbCells(wColRowOrigin, false);
                    DecNbCells(Row(wRow), true);
                    if (wDestEmptyBeforePlace) {
                        wColRowDest->IncNbCells();
                        if (Row(wRow) == nullptr) EnsureRow(wRow);
                        Row(wRow)->IncNbCells();
                    }
                }
            }
#ifdef debugcolrow
#ifdef checksp
            if (wColRowDest!=nullptr)
              cout << "wColRowwDest " << wColDest << " NbCells=" << wColRowDest->NbCells() << endl;
            if (wColRowOrigin!=nullptr)
              cout << "wColRowOrigin " << wColOrigin << " NbCells=" << wColRowOrigin->NbCells() << endl;
#endif
#endif
        }
        
        
#ifdef debugcolrow
        for(tIndex wCol=wRectDeleteArea.Left(); wCol<=wRectDeleteArea.Right();wCol++) {
          tColRow* wColRow=Col(wCol);
          if (wColRow!=nullptr)
              cout << "Col " << wCol << " NbCells=" << wColRow->NbCells() << endl;
        }
#endif
        
        
        if (sIsUndoInsert) {
            sSaveSelectErase->ApplyColRowUndoInsertRect();
        } else {
            sSaveSelectErase->DeleteRangeInColRowContainer();
            sSaveSelectErase->ApplyColRow();
        }
        sSaveSelectErase->CalculateDo(this,tVolatile::t_Col);
#ifdef debugcolrow
        cout << sSaveSelectErase->Debug();
#endif
#ifdef checksp
        Check();
#endif
    }
        
    void tColRowCellRange::UndoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) {
        // For undo of DeleteRowByRect, use the original rect to determine which rows to restore,
        // but use the rebased rect (if provided) to determine where to move cells.
        // The rebased rect accounts for other users' operations.
        // Preserve ranges that span past the rect (symmetric with the delete-by-rect preserve).
        DoInsertRowByRect(sSaveSelectErase, sRect, sRectMoveCells, true);
#ifdef debugcolrow
        cout << sSaveSelectErase->Debug();
#endif
        // Rebase saved cells AFTER inserting rows (similar logic as for columns)
        if (sRectMoveCells != nullptr && !sSaveSelectErase->IsEmpty()) {
            tWorkBook* wWorkBook = Sheet()->WorkBook();
            if (wWorkBook != nullptr) {
                tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
                if (wCurrentSequence > 0) {
                    tRebasePlan wRebasePlan = wWorkBook->UndoRebaseLog().BuildPlan(
                        0,
                        wCurrentSequence,
                        Sheet()->AllocatorRef()
                    );
                    // Only rebase if there are operations to apply
                    if (!wRebasePlan.m_Operations.empty()) {
                        sSaveSelectErase->Rebase(wRebasePlan);
                    }
                }
            }
        }
        sSaveSelectErase->UndoDeleteColRow();
        // Calculate - After restoring rows, find all cells that depend on ranges referencing them
        // This ensures cells like A7 with SUM(A1:A6) are recalculated
        sSaveSelectErase->PushRangeDependCalculate(this, sRect);
        // Calculate
        sSaveSelectErase->CalculateDo(this,tVolatile::t_Row);
        // Recalc can disturb formula inverse deps on restored cells (checksp on WASM).
        sSaveSelectErase->RewireOutgoingFormulaDependents();

#ifdef checksp
        Check();
#endif
    }
        
    void tColRowCellRange::UndoDeleteColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) {
        // For undo of DeleteColByRect, use the original rect to determine which columns to restore,
        // but use the rebased rect (if provided) to determine where to move cells.
        // The rebased rect accounts for other users' operations.
        // Preserve ranges that span past the rect (symmetric with the delete-by-rect preserve).
        DoInsertColByRect(sSaveSelectErase, sRect, sRectMoveCells, true);
#ifdef debugcolrow
        cout << sSaveSelectErase->Debug();
#endif
        // Rebase saved cells AFTER inserting columns.
        // The saved cells were NOT rebased in RebaseCol() (see comment there),
        // so we need to rebase them now to account for ALL operations from other users
        // (e.g., User2's InsertCol and InsertRow). The insertion of columns we just did doesn't
        // affect the saved cells because they were saved from the columns that were deleted.
        // IMPORTANT: The saved cells are at their ORIGINAL positions (before deletion).
        // They need to be rebased to account for ALL operations (from the beginning)
        // to get their final positions after all operations.
        if (sRectMoveCells != nullptr && !sSaveSelectErase->IsEmpty()) {
            tWorkBook* wWorkBook = Sheet()->WorkBook();
            if (wWorkBook != nullptr) {
                tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
                if (wCurrentSequence > 0) {
                    // Build rebase plan from the beginning to include ALL operations
                    // (InsertRow, InsertCol, etc.) from all users
                    tRebasePlan wRebasePlan = wWorkBook->UndoRebaseLog().BuildPlan(
                        0,  // From the beginning to include all operations
                        wCurrentSequence,
                        Sheet()->AllocatorRef()
                    );
                    // Only rebase if there are operations to apply
                    if (!wRebasePlan.m_Operations.empty()) {
                        sSaveSelectErase->Rebase(wRebasePlan);
                    }
                }
            }
        }
        sSaveSelectErase->UndoDeleteColRow();
        sSaveSelectErase->PushRangeDependCalculate(this, sRect);
        sSaveSelectErase->CalculateDo(this,tVolatile::t_Col);
        sSaveSelectErase->RewireOutgoingFormulaDependents();

#ifdef checksp
        Check();
#endif
    }

    void tColRowCellRange::SaveColRowsAndRangesForDelete(tSaveSelectErase* sSaveSelectErase,
        tIndex sIndexParent, tIndex sPosition, tIndex sSize, tBool sDoesRow) {
        for (tIndex wIndex = sIndexParent; wIndex < sPosition + sSize; wIndex++) {
            tColRow* wColRow = sDoesRow ? Row(wIndex) : Col(wIndex);
            if (wColRow == nullptr) {
                continue;
            }
            tSaveColRow* wSaveColRow = sSaveSelectErase->AddColRow(wColRow, this, sDoesRow);
            if (wSaveColRow->Css() != 0) {
                Sheet()->WorkBook()->IncCellFormat(wSaveColRow->Css());
            }
            for (auto wItemRange : *wColRow->ContainerRange()->Container()) {
#ifdef debugrange
                if (!sDoesRow) {
                    tRange* wRangePt = m_AllocatorRange(wItemRange);
                    cout << " DoDeleteCol Range ->" << wRangePt->Debug() << endl;
                }
#endif
                tRange* wRange = m_AllocatorRange(wItemRange);
                if (wRange != nullptr) {
                    sSaveSelectErase->AddUniqueRange(wRange);
                }
            }
        }
    }

    void tColRowCellRange::ErasePhysicalRowsAfterDelete(tIndex sRow, tIndex sSize) {
        for (tIndex wRow = sRow; wRow < sRow + sSize; wRow++) {
            DeleteRow(wRow);
        }
        m_Rows.Erase(sRow, sSize);
        m_Cell2D.Erase(sRow, sSize);
        // m_Cell2D.Erase shifts slots without updating tCell / ClassAttribute roots.
        RelocateOccupiedCellsFrom(this, m_Cell2D, sRow);
    }

    void tColRowCellRange::ErasePhysicalColsAfterDelete(tIndex sCol, tIndex sSize) {
#ifdef debugcolrow
        cout << "Erase  all cell(s) in selection." << endl;
#endif
        m_Cell2D.ForEachOccupied([&](tSize wRow, tSparseArrayCell* wSparseArrayCell) {
            for (tIndex wCol = sCol; wCol < sCol + sSize; wCol++) {
#ifdef debugcolrow
                cout << " wRow " << wRow << " wCol " << wCol << " " << wSparseArrayCell << ":"
                    << wSparseArrayCell->RealSize() << endl;
#endif
                tAllocatorRef wIndexCell = (*wSparseArrayCell)(static_cast<tSize>(wCol));
                if (wIndexCell != 0) {
                    DeleteCell(static_cast<tIndex>(wRow), wCol);
                }
            }
            return true;
        });
#ifdef debugcolrow
        cout << "Delete col(s)." << endl;
#endif
        for (tIndex wCol = sCol; wCol < sCol + sSize; wCol++) {
            tColRow* wColRow = Col(wCol);
            if (wColRow != nullptr) {
                DeleteCol(wCol);
            }
        }
        m_Cols.Erase(sCol, sSize);
        m_Cell2D.ForEachOccupied([&](tSize /*wRow*/, tSparseArrayCell* wSparseArrayCellCol) {
            wSparseArrayCellCol->Erase(static_cast<tSize>(sCol), static_cast<tSize>(sSize));
            return true;
        });
    }

    void tColRowCellRange::DoDeleteColRowAxis(tSaveSelectErase* sSaveSelectErase,
        tIndex sPosition, tIndex sSize, tBool sDoesRow) {
        tRect wRectDeleteArea = sDoesRow
            ? GetRectDeleteAreaRow(sPosition, sSize)
            : GetRectDeleteAreaCol(sPosition, sSize);

        sSaveSelectErase->ParseSelect(wRectDeleteArea.StrRef());
        sSaveSelectErase->RectDeleteArea(wRectDeleteArea);

        tInt wIndexParent = sPosition;
        tIndex wParent = GrandParent(sPosition, sDoesRow);
        if (wParent != 0) {
            wIndexParent = wParent;
        }

        SaveColRowsAndRangesForDelete(sSaveSelectErase, wIndexParent, sPosition, sSize, sDoesRow);
        sSaveSelectErase->DeleteColRow(sDoesRow, m_SheetAllocator, sPosition, sSize, false);

        if (sDoesRow) {
            ErasePhysicalRowsAfterDelete(sPosition, sSize);
            RenumRow(wIndexParent);
        } else {
            ErasePhysicalColsAfterDelete(sPosition, sSize);
            RenumCol(wIndexParent);
        }

#ifdef debugcolrow
        if (!sDoesRow) {
            cout << "sSaveSelectErase->ApplyColRow()." << endl;
        }
#endif
        sSaveSelectErase->ApplyColRow();
        sSaveSelectErase->CalculateDo(this, sDoesRow ? tVolatile::t_Row : tVolatile::t_Col);

        if (sDoesRow) {
            sSaveSelectErase->ClearAfterDo();
        }
#ifdef checksp
        if (!sDoesRow) {
            Check();
        }
#endif
    }

	void tColRowCellRange::DoDeleteRow(tSaveSelectErase* sSaveSelectErase, tIndex sRow, tIndex sSize) {
        InvalidateExtentCache();
        DoDeleteColRowAxis(sSaveSelectErase, sRow, sSize, true);
#ifdef debugcolrow
        cout << sSaveSelectErase->Debug();
#endif
	};
        
    
	void tColRowCellRange::UndoDeleteRow(tSaveSelectErase* sSaveSelectErase, tIndex sRow, tIndex sSize) {
#ifdef debugcolrow
		cout << sSaveSelectErase->Debug();
#endif
        sSaveSelectErase->ColRowCellRange(this);
        sSaveSelectErase->DetachRangeAfterFromColRow();

     	// Insert Rows
        DoInsertRow(sRow, sSize,sSaveSelectErase);
        
        sSaveSelectErase->AttachRangeToColRow();

        // Set save value
        sSaveSelectErase->UndoDeleteColRow();

        // Recup ColRow
        for(auto wSaveColRow : *sSaveSelectErase->VectorColRow()) {
            tIndex wPosition=wSaveColRow->Index();
            tColRow* wColRow = Row(wPosition);
            if (wColRow==nullptr) { wColRow=m_Sheet->EnsureRow(wPosition); }
            sSaveSelectErase->RecupColRow(wColRow,wSaveColRow,this,true);
            //  Raz Css
            wSaveColRow->Css(0);
        }
    
        // Recup JsonPayload
        sSaveSelectErase->UndoJsonPayload();

        // Calculate from save (fcell + range deps) — collab GetMessage must not rely on RecalculateAll.
        sSaveSelectErase->PushRangeDependCalculate(this, sSaveSelectErase->RectDeleteArea());
        sSaveSelectErase->CalculateDo(m_Sheet->ColRowCellRange(),tVolatile::t_Row);
        sSaveSelectErase->RewireOutgoingFormulaDependents();

	}

	void tColRowCellRange::DoInsertCol(tIndex sCol, tIndex sSize,tSaveSelectErase* sSaveSelectErase) {
        InvalidateExtentCache();
        if (sSaveSelectErase != nullptr) {
            sSaveSelectErase->ColRowCellRange(this);
        }
		// 1 Get Range to copy
		tVectorRange wVectorRange;
		tColRow::tContainerRange* wContainerRange = nullptr;
		tColRow* wColRow = Col(sCol);
		if (wColRow != nullptr) {
			wContainerRange = wColRow->ContainerRange();
			tColRow::tContainerRange::tContainerClass* wContainer = wContainerRange->Container();
			tColRow::tContainerRange::tIteratorClass wIterator;
			for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
				tRange* wRange = m_AllocatorRange(*wIterator);
				if (wRange->LeftIndex() < sCol) {
					wVectorRange.push_back(wRange);
				}
			}
		}
        
        
        // 2 Is Tree
        tAllocatorRef wParentRef=0;
        if (sSaveSelectErase==nullptr) {
            wColRow=Col(sCol);
            if (wColRow!=nullptr) {
                wParentRef=wColRow->ParentRef();
            }
        }
        
		// 3 insert Col
		m_Cols.Insert(sCol, sSize);
        
        // 4 Renum Sub Tree — rebuild from tree root so ancestors' m_Children are cleared
        if ((sSaveSelectErase==nullptr) && (wParentRef!=0)) {
            tColRow* wColRowParent=ColRowByAllocatorRef(wParentRef, false);
            for(tIndex wIndex=sCol;wIndex<sCol+sSize;wIndex++) {
                tColRow* wColRow=EnsureCol(wIndex);
                wColRow->ParentRef(wParentRef);
            }
            RenumCol(GrandParent(wColRowParent->Index(), false));
        } else {
            RenumCol(sCol);
        }

		// 5 Recopy range
		if (!wVectorRange.empty()) {
			// Set empty Value for new insert line ===========================
			for (tIndex wIndex = sCol; wIndex < sCol + sSize; wIndex++) {
				tColRow* wColRow = EnsureCol(wIndex);
				for (auto wRange : wVectorRange) {
					wColRow->AddRange(wRange);
				}
			}
		}

		// For each occupied row, insert cols
		m_Cell2D.ForEachOccupied([&](tSize /*wRow*/, tSparseArrayCell* wSparseArrayCellCol) {
			wSparseArrayCellCol->Insert(static_cast<tSize>(sCol), static_cast<tSize>(sSize));
			return true;
		});
        // 6 copy Format + column width
        // Duplicate cell CSS onto inserted columns (cleared by Insert). Vertical seams: strip left/right.
        // When origin has border-right, also strip top/bottom (horizontal seam at right edge).
        // Source column: column left of the inserted block (sCol - 1) — Excel xlFormatFromLeftOrAbove.
        // Also copy column width from that origin (same rule as row height on insert).
        // Skip when undoing a column delete, or insert at column 1 (no format duplication on first column).
        if (sSaveSelectErase == nullptr && sCol > 1) {
            const tIndex wOriginCol = sCol - 1;
            if (ValidCol(wOriginCol)) {
                tWorkBook* wWorkBook = Sheet()->WorkBook();
                tInsertCellCssCache wCssCache;
                const tBool wValidRightIns = ValidCol(sCol + sSize);
                tColRow* wOriginColRow = Col(wOriginCol);
                if (wOriginColRow != nullptr) {
                    const tDouble wOriginSize = wOriginColRow->Size();
                    for (tIndex wCol = sCol; wCol < sCol + sSize; wCol++) {
                        tColRow* wDestColRow = EnsureCol(wCol);
                        if (wDestColRow != nullptr) {
                            wDestColRow->Size(wOriginSize);
                        }
                    }
                }
                // Sparse rows often have no cells in new cols: Col-level Css merges sheet→col→row→cell in JsonFormatJavaScript.
                {
                    if (wOriginColRow != nullptr && wOriginColRow->Css() != 0) {
                        const tFormatRef wColCssOrigin = wOriginColRow->Css();
                        const tFormatRef wColCss = CachedStripCssForInsertCol(
                            wWorkBook, wColCssOrigin, wValidRightIns, wCssCache);
                        for (tIndex wCol = sCol; wCol < sCol + sSize; wCol++) {
                            tColRow* wDestColRow = EnsureCol(wCol);
                            if (wDestColRow != nullptr) {
                                AssignInsertColRowCss(wWorkBook, wDestColRow, wColCssOrigin, wColCss, wCssCache);
                            }
                        }
                    }
                }
                for (tIndex wCol = sCol; wCol < sCol + sSize; wCol++) {
                    for (tIndex wRow = 1; wRow <= m_Rows.Size(); wRow++) {
                        tCell* wCellOrigin = Cell(wRow, wOriginCol);
                        if (wCellOrigin != nullptr && wCellOrigin->Css() != 0) {
                            tCell* wCell = EnsureCell(wRow, wCol);
                            if (wCell != nullptr) {
                                const tFormatRef wCssOrigin = wCellOrigin->Css();
                                const tFormatRef wCss = CachedStripCssForInsertCol(
                                    wWorkBook, wCssOrigin, wValidRightIns, wCssCache);
                                AssignInsertCellCss(wWorkBook, wCell, wCssOrigin, wCss, wCssCache);
                            }
                        }
                    }
                }
            }
        }

        // 7 Add Volatile Col in path
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        AddPathVolatile(&wContainerPath,tVolatile::t_Col);
        wContainerPath.EndCalculate();
	}
    

    void tColRowCellRange::DoDeleteCol(tSaveSelectErase* sSaveSelectErase,tIndex sCol, tIndex sSize) {
        InvalidateExtentCache();
#ifdef debugcolrow
        cout << "tColRowCellRange::doDeleteCol " <<  Base10ToAlpha(sCol)  << " to " <<   Base10ToAlpha(sCol+sSize) << endl;
#endif
        DoDeleteColRowAxis(sSaveSelectErase, sCol, sSize, false);
#ifdef debugrangecovered 
		sSaveSelectErase->Debug();
#endif
#ifdef debugcolrow
        cout << "End doDeleteCol " << endl;
#endif
	}

	void tColRowCellRange::UndoDeleteCol(tSaveSelectErase* sSaveSelectErase, tIndex sCol, tIndex sSize) {
        sSaveSelectErase->ColRowCellRange(this);
        sSaveSelectErase->DetachRangeAfterFromColRow();

        const tRect wRectInsert = GetRectDeleteAreaCol(sCol, sSize);
		// insert Col
        DoInsertCol(sCol, sSize,sSaveSelectErase);
        // Insert shifts refs without updating tCell / ClassAttribute roots.
        RelocateOccupiedCellsFrom(this, m_Cell2D, 0);

        sSaveSelectErase->AttachRangeToColRow();
        
		// Set save value
        sSaveSelectErase->UndoDeleteColRow();
        
        // Recup ColRow
        for(auto wSaveColRow : *sSaveSelectErase->VectorColRow()) {
            tIndex wPosition=wSaveColRow->Index();
            tColRow* wColRow = Col(wPosition);
            if (wColRow==nullptr) { wColRow=m_Sheet->EnsureCol(wPosition); }
            sSaveSelectErase->RecupColRow(wColRow,wSaveColRow,this,false);
            //  Css
            wSaveColRow->Css(0);
        }
        
        
        // Recup JsonPayload
        sSaveSelectErase->UndoJsonPayload();
        
        // Calculate from save (fcell + range deps) — collab GetMessage must not rely on RecalculateAll.
        sSaveSelectErase->PushRangeDependCalculate(this, wRectInsert);
        sSaveSelectErase->CalculateDo(this,tVolatile::t_Col);
        sSaveSelectErase->RewireOutgoingFormulaDependents();
        
#ifdef checksp
        Check();
#endif
	}
        
    tColRow* tColRowCellRange::ColRowByIndex(tIndex sIndex,tBool sIsRow) {
        if (sIndex==-1) return(nullptr);
        if (sIsRow) {
            return(Row(sIndex));
        } else {
            return(Col(sIndex));
        }
    }
        
    tColRow* tColRowCellRange::ColRowByAllocatorRef(tAllocatorRef sAllocatorRef,tBool sIsRow) {
            if (sAllocatorRef==0) return(nullptr);
            if (sIsRow) {
                return(m_AllocatorRow(sAllocatorRef));
            } else {
                return(m_AllocatorCol(sAllocatorRef));
            }
    }
    
    const tColRow* tColRowCellRange::ColRowByAllocatorRef(tAllocatorRef sAllocatorRef,tBool sIsRow) const {
            if (sAllocatorRef==0) return(nullptr);
            if (sIsRow) {
                return(m_AllocatorRow(sAllocatorRef));
            } else {
                return(m_AllocatorCol(sAllocatorRef));
            }
    }
    
    tColRow* tColRowCellRange::EnsureColRow(tIndex sIndex,tBool sIsRow) {
        if (sIndex==-1) return(nullptr);
        if (sIsRow) {
            return(EnsureRow(sIndex));
        } else {
            return(EnsureCol(sIndex));
        }
    }
        
        
    // Tree ===================================================================
    tIndex tColRowCellRange::GrandParent(tIndex sPosition,tBool sIsRow) {
        tInt wIndexParent=sPosition;
        tColRow* wColRow=ColRowByIndex(sPosition,sIsRow);
        tInt wGuard=0;
        while(wColRow!=nullptr) {
            wIndexParent=wColRow->m_Index;
            tColRow* wParent=wColRow->ColRowParent(this, sIsRow);
            if (wParent==wColRow) {
                break;
            }
            wColRow=wParent;
            if (++wGuard > 65536) {
                break;
            }
        }
        return(wIndexParent);
    }
        
    tBool tColRowCellRange::DoTreeRight(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        // Need at least parent + one child in the selection.
        if (sSize < 2 || sPosition < 1) {
            return(false);
        }
        // Skip leading nodes that already have children so a selection that
        // includes an existing parent can still nest the following siblings
        // (e.g. select 3:9 where 3 is already a group → nest under 4).
        tIndex wParentPos = sPosition;
        tColRow* wColRowRef = EnsureColRow(wParentPos, sIsRow);
        if (wColRowRef == nullptr) {
            return(false);
        }
        while (wColRowRef != nullptr && !wColRowRef->m_Children.empty()
               && wParentPos + 1 < sPosition + sSize) {
            wParentPos++;
            wColRowRef = EnsureColRow(wParentPos, sIsRow);
        }
        if (wColRowRef == nullptr || !wColRowRef->m_Children.empty()) {
            return(false);
        }
        if (wParentPos + 1 >= sPosition + sSize) {
            return(false);
        }
        
        tIndex wIndexParent=wParentPos;
        tColRow* wColRowParent=wColRowRef->ColRowParent(this, sIsRow);
        if (wColRowParent!=nullptr) wIndexParent=wColRowParent->Index();
        // Save Selection (full original range for undo)
        for (tIndex wPosition = sPosition; wPosition < sPosition + sSize; wPosition++) {
            tColRow* wColRow=EnsureColRow(wPosition,sIsRow);
            if (wColRow == nullptr) {
                return(false);
            }
            sSaveSelectColRow->AddColRow(wColRow,this,sIsRow);
        }
     
        tBool wLinked = false;
        // Nest following same-level siblings under wParentPos
        for(tIndex wPosition=wParentPos+1; wPosition< sPosition + sSize;wPosition++) {
            tColRow* wColRow = EnsureColRow(wPosition, sIsRow);
            if (wColRow == nullptr) {
                continue;
            }
            // If Same Level Parent (Same AllocatorRef)
            if (wColRow->ParentRef()==wColRowRef->ParentRef()) {
                // Raz Old Parent
                if (wColRow->ParentRef()!=0) {
                    tColRow* wOldParent=ColRowByAllocatorRef(wColRow->ParentRef(),sIsRow);
                    if (wOldParent != nullptr) {
                        sSaveSelectColRow->AddColRow(wOldParent,this,sIsRow);
                    }
                }
                if (sIsRow) {
                    wColRow->ParentRef(m_Rows[wParentPos]);
                    wColRowRef->AddChildren(m_Rows[wPosition]);
                } else {
                    wColRow->ParentRef(m_Cols[wParentPos]);
                    wColRowRef->AddChildren(m_Cols[wPosition]);
                }
                wLinked = true;
            }
        }
        if (!wLinked) {
            return(false);
        }
        if (sIsRow) {
            RenumRow(wIndexParent);
        } else {
            RenumCol(wIndexParent);
        }
        return(true);
    }
        
    tBool tColRowCellRange::DoTreeLeft(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        if (sSize < 1 || sPosition < 1) {
            return(false);
        }
        tVectorColRow wVector;
        
        tIndex wIndexParent=GrandParent(sPosition, sIsRow);
        
        // Save Selection
        tColRow* wColRowRef=EnsureColRow(sPosition, sIsRow);
        if (wColRowRef == nullptr) {
            return(false);
        }
        if (wColRowRef->ParentRef()==0) {
            // Root node selected: ungroup by promoting its children (Excel-like ungroup on parent).
            if (wColRowRef->m_Children.empty()) {
                return(false);
            }
            sSaveSelectColRow->AddColRow(wColRowRef, this, sIsRow);
            wVector.push_back(wColRowRef);
        } else {
            // Selected rows share a parent — collect that parent (outdent one level).
            for (tIndex wPosition = sPosition; wPosition < sPosition + sSize; wPosition++) {
                tColRow* wColRow=ColRowByIndex(wPosition,sIsRow);
                if (wColRow != nullptr) {
                    if (wColRowRef->ParentRef()==wColRow->ParentRef()) {
                        tColRow* wColRowParent=ColRowByAllocatorRef(wColRow->ParentRef(),sIsRow);
                        if (wColRowParent!=nullptr) {
                            if (!sSaveSelectColRow->FindColRow(wColRowParent->Index())) {
                                sSaveSelectColRow->AddColRow(wColRowParent,this,sIsRow);
                                wVector.push_back(wColRowParent);
                            }
                        }
                    }
                }
            }
        }
        if (wVector.empty()) {
            return(false);
        }
        // Loop and Parent an descent
        for(tColRow* wColRow : wVector) {
#ifdef debugtree
            cout << "Parent  " << wColRow->Debug() << endl;
#endif
            tAllocatorRef wGrandFatherRef=wColRow->ParentRef();
            tColRow* wColRowGrandFather=ColRowByAllocatorRef(wGrandFatherRef,sIsRow);
            if (wColRowGrandFather!=nullptr) {
                sSaveSelectColRow->AddColRow(wColRowGrandFather,this,sIsRow);
#ifdef debugtree
                cout << "Grand Parent  " << wColRowGrandFather->Debug() << endl;
#endif
            }
            tColRow::tVectorColRow wVectorCopy=wColRow->m_Children;
            for (auto wChildRef : wVectorCopy) {
                tColRow* wColRowChildren=ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wColRowChildren==nullptr) continue;
                
                // Save ColRow
                sSaveSelectColRow->AddColRow(wColRowChildren,this,sIsRow);
#ifdef debugtree
                cout << " -> change " << wColRowChildren->Debug() << " to ";
#endif
               
                // Add New Parent
                if (wColRowGrandFather!=nullptr) {
                    wColRowChildren->ParentRef(wGrandFatherRef);
                    wColRowGrandFather->AddChildren(wChildRef);
                } else {
                    wColRowChildren->ParentRef(0);
                    if (wColRow->IsEmpty()) {
                        if (sIsRow) {
                            DeleteRow(wColRowChildren->Index());
                        } else {
                            DeleteCol(wColRowChildren->Index());
                        }
                    }
                }
#ifdef debugtree
                wColRowChildren=ColRowByAllocatorRef(wChildRef, sIsRow);
                cout << "After ";
                if (wColRowChildren!=nullptr) {
                    cout << wColRowChildren->Debug();
                } else {
                    cout << "nullptr";
                }
                cout << endl;
#endif
            }
            // Children were moved to grandfather (or root); clear stale list before Renum.
            wColRow->m_Children.clear();
#ifdef debugtree
            cout << "After Parent  " << wColRow->Debug() << endl;
            wColRowGrandFather=ColRowByAllocatorRef(wColRow->ParentRef(),sIsRow);
            if (wColRowGrandFather!=nullptr) {
                cout << "After Grand Parent  " << wColRowGrandFather->Debug() << endl;
            }
#endif
        }
        if (sIsRow) {
            RenumRow(wIndexParent);
        } else {
            RenumCol(wIndexParent);
        }
        return(true);
    }
        
    tBool tColRowCellRange::UndoTree(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        (void)sSize;
        for(auto wSaveColRow : *sSaveSelectColRow->VectorColRow()) {
            tIndex wPosition=wSaveColRow->Index();
            tColRow* wColRow=ColRowByIndex(wPosition,sIsRow);
            if (wColRow==nullptr) wColRow=EnsureColRow(wPosition, sIsRow);
#ifdef debugtree
            
            cout << "change " << wColRow->Debug() << " to ";
#endif
            // Tree AddColRow stores a non-owned Css snapshot (no IncCellFormat).
            sSaveSelectColRow->RecupColRow(wColRow,wSaveColRow,this,sIsRow,false);
            if (wColRow->IsEmpty()) {
                if (sIsRow) {
                    DeleteRow(wPosition);
                } else {
                    DeleteCol(wPosition);
                }
            }
            
#ifdef debugtree
            
            cout << "After ";
            if (wColRow!=nullptr) {
                cout << wColRow->Debug();
            } else {
                cout << "nullptr";
            }
            cout << endl;
#endif
        }
        // Recup can leave inconsistent m_Children vs ParentRef (and rare cycles).
        // Rebuild from index 1 so ancestors above sPosition also clear stale children
        // without walking GrandParent on a dirty graph (freeze).
        (void)sPosition;
        if (sIsRow) {
            RenumRow(1);
        } else {
            RenumCol(1);
        }
        return(true);
    }


	// Visitor Range ==For Function Sum, Min, Max....=============================
	void tColRowCellRange::VisitorRange(tRange* sCalcRange, tSparseArrayCallBack<tAllocatorRef>* sSkCallBackRange) {
		tIndex wTop = sCalcRange->TopIndex();
		tIndex wBottom = sCalcRange->IterateBottom();

		tIndex wLeft= sCalcRange->LeftIndex();
		tIndex wRight = sCalcRange->IterateRight();
		if (wTop < 0 || wBottom < wTop || wLeft < 0 || wRight < wLeft) {
			return;
		}
		m_Cell2D.ForEachOccupied(static_cast<tSize>(wTop), static_cast<tSize>(wBottom),
			[&](tSize /*wRow*/, tSparseArrayCell* wRowCells) {
				tBool wContinue = true;
				wRowCells->ForEachOccupied(static_cast<tSize>(wLeft), static_cast<tSize>(wRight),
					[&](tSize /*wCol*/, tAllocatorRef wRef) {
						wContinue = sSkCallBackRange->CallBack(wRef);
						return wContinue;
					});
				return wContinue;
			});
	}

	void tColRowCellRange::DeleteSheet(tSaveSelectErase* sSaveSelectErase) {
		tIndex wMaxCol = 0;
		tRect wRectDeleteArea(0, 0, tIndex(m_Cell2D.Size()), wMaxCol);
		for (tIndex wRow = 0; wRow < (tIndex) m_Cell2D.Size(); wRow++) {
			tSparseArrayCell* wSparseArrayCell = m_Cell2D(wRow);
			if (wSparseArrayCell != nullptr) {
				if ((tIndex) wSparseArrayCell->Size() > wRectDeleteArea.Right()) {
					wRectDeleteArea.Right(tIndex(wSparseArrayCell->Size()));
				}
			}
		}

		sSaveSelectErase->RectDeleteArea(wRectDeleteArea);
		sSaveSelectErase->DeleteSheet(m_SheetAllocator);

		// Drop from workbook sheet list only; sheet data stays until undo object is destroyed.
		m_Sheet->WorkBook()->DeleteSheet(m_Sheet->Name(), !sSaveSelectErase->IsUndoActif());
	}

	void tColRowCellRange::UndoDeleteSheet(tSaveSelectErase* sSaveSelectErase) {
		sSaveSelectErase->UndoDeleteSheet();
		m_Sheet->Calculate(sSaveSelectErase->TempoRectDeleteArea());
	}

	void tColRowCellRange::CallBackAllCell(tSparseArrayCallBack<tAllocatorRef>* sCallBackRange) {
#ifdef debugcolrow
		cout << "tColRowCellRange::CallBackAllCell() ------------------------------------" << endl;
#endif
		tAllocatorRef wTop = 0;
		tAllocatorRef wBottom = LastRow();
		tAllocatorRef wLeft = 0;
		// LastCol is inclusive; sparse col scan uses an exclusive upper bound.
		tAllocatorRef wRight = LastCol() + 1;
		CallBackAllCellInRange(sCallBackRange, wTop, wBottom, wLeft, wRight);
	}

	void tColRowCellRange::CallBackAllCellInRange(
		tSparseArrayCallBack<tAllocatorRef>* sCallBackRange,
		tAllocatorRef sTop,
		tAllocatorRef sBottom,
		tAllocatorRef sLeft,
		tAllocatorRef sRight
	) {
		if (sTop > sBottom || sLeft > sRight) {
			return;
		}
		m_Cell2D.ForEachOccupied(sTop, sBottom, [&](tSize /*wRow*/, tSparseArrayCell* wRowCells) {
			tBool wContinue = true;
			wRowCells->ForEachOccupied(sLeft, sRight, [&](tSize /*wCol*/, tAllocatorRef wRef) {
#ifdef debugcallback
				tCell* wCell = m_AllocatorCell(wRef);
				if (wCell != nullptr) {
					cout << "-> " << wCell->StrRef() << endl;
				}
#endif
				wContinue = sCallBackRange->CallBack(wRef);
				return wContinue;
			});
			return wContinue;
		});
	}

	void tColRowCellRange::CallBackAllCellPhysical(tSparseArrayCallBack<tAllocatorRef>* sCallBackRange) {
		CallBackAllCellInRange(
			sCallBackRange,
			0,
			tIndex(m_Rows.RealSize()),
			0,
			tIndex(m_Cols.RealSize()) + 1);
	}

    void tColRowCellRange::CallBackAllRanges(const std::function<void(tRange*)>& sVisitor) {
        if (!sVisitor) {
            return;
        }
        for (tAllocatorRef wAllocator = 1; wAllocator <= m_AllocatorRange.Size(); wAllocator++) {
            if (IsDeletedRange(wAllocator)) {
                continue;
            }
            tRange* wRange = m_AllocatorRange(wAllocator);
            if (wRange != nullptr) {
                sVisitor(wRange);
            }
        }
    }

	void tColRowCellRange::Delete() {
		// do nothing call by allocator
	}

	//=========================================================================
	//! Used for callback cell (Write Json)
	class tCallBackJson : public tSparseArrayCallBack<tAllocatorRef> {
	protected:
		//! Sheet. 
		tColRowCellRange* m_ColRowCellRange;
		Writer<StringBuffer>* m_Writer;
	public:
		/// @brief		Constructor SkCallBackCheck with owner tColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackJson(tColRowCellRange* sColRowCellRange, Writer<StringBuffer>* sWriter) : tSparseArrayCallBack<tAllocatorRef>() {
			m_ColRowCellRange = sColRowCellRange;
			m_Writer = sWriter;
		}
		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tAllocatorRef Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef) {
			tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
            if (!wCell->IsEmpty()) {
                m_Writer->StartObject();
                wCell->Json(m_Writer,true);
                m_Writer->EndObject();
            }
			return(true);
		}
	};

    // Json ===============================================================
    namespace {
    // Persist row/col bands with real content; always persist bands that carry a format;
    // persist width/height-only bands inside content extent, or anywhere when the sheet has
    // no content extent yet (UndoSizeRow on an empty sheet).
    tBool ShouldSerializeColRowBand(tColRow* sColRow, tIndex sIndex, tIndex sContentLast) {
        if (sColRow == nullptr || sColRow->IsEmpty()) {
            return false;
        }
        if (!sColRow->IsVacantForUsedExtent()) {
            return true;
        }
        // A band that only carries a format (no cells, no grouping) is "vacant" for the
        // content extent, yet its format must always be persisted, even beyond the last
        // row/col that holds cell content. Otherwise formatting a whole empty col/row is lost.
        if (sColRow->Css() != 0) {
            return true;
        }
        if (sContentLast <= 0) {
            return true;
        }
        return sIndex <= sContentLast;
    }
    } // namespace

    void tColRowCellRange::JsonMerged(Writer<StringBuffer>* sWriter) {
        sWriter->String("merged");
        sWriter->StartArray();
        const tIndex wLastRow = LastRow();
        for (tIndex wRow = 0; wRow <= wLastRow; wRow++) {
            tColRow* wColRow = Row(wRow);
            if (wColRow != nullptr) {
                tColRow::tContainerRange*  wContainer;
                wContainer=wColRow->ContainerRange();
                for(auto wRangeAllocatorRef : *wContainer->Container()) {
                    tRange* wRange = Range(wRangeAllocatorRef);
                    if (wRange->IsMerged()) {
                        if (wRange->TopIndex()==wRow) {
                            sWriter->String(wRange->StrRef().c_str());
                        }
                    }
                }
            }
        }

        sWriter->EndArray();
    }
	
    void tColRowCellRange::Json(Writer<StringBuffer>* sWriter) {
        sWriter->SetMaxDecimalPlaces(3);
        // Limit double precision to 3 decimals
        const tIndex wLastRow = LastRowForJson();
        const tIndex wLastCol = LastColForJson();
        const tIndex wContentLastRow = LastRow();
        const tIndex wContentLastCol = LastCol();
		// 1 Row
		sWriter->String("rows");
		sWriter->StartArray();
		for (tIndex wRow = 0; wRow <= wLastRow; wRow++) {
			tColRow* wColRow = Row(wRow);
			if (ShouldSerializeColRowBand(wColRow, wRow, wContentLastRow)) {
                sWriter->StartObject();
                wColRow->Json(sWriter,this,true);
                sWriter->EndObject();
			}
		}
		sWriter->EndArray();
		// 2 col 
		sWriter->String("cols");
		sWriter->StartArray();
		for (tIndex wCol = 0; wCol <= wLastCol; wCol++) {
			tColRow* wColRow = Col(wCol);
			if (ShouldSerializeColRowBand(wColRow, wCol, wContentLastCol)) {
                sWriter->StartObject();
                wColRow->Json(sWriter,this,false);
                sWriter->EndObject();
			}
		}
		sWriter->EndArray();
        // Reset to default behavior
		sWriter->SetMaxDecimalPlaces(rapidjson::Writer<rapidjson::StringBuffer>::kDefaultMaxDecimalPlaces); 

		// 3	Cell
		sWriter->String("cells");
		sWriter->StartArray();
		tCallBackJson wCallBackJson(this,sWriter);
		CallBackAllCell(&wCallBackJson);
		sWriter->EndArray();
        // 4 Merged 
        JsonMerged(sWriter);
        // 5 Conditional Format
        if (m_ConditionalFormatContainer!=nullptr) {
            sWriter->Key("cf");
            m_ConditionalFormatContainer->Json(sWriter);
        }
	}

	void tColRowCellRange::Json(const Value& sValue) {
        // 1 Rows
		const Value& wRows = sValue["rows"];
		assert(wRows.IsArray());
		for (SizeType wIndex = 0; wIndex < wRows.Size(); wIndex++) {
			const Value& wRow = wRows[wIndex];
			tIndex wIndexRow = wRow["i"].GetInt();
			EnsureRow(wIndexRow)->Json(wRow,this,true);
		}
        // 2 Cols
		const Value& wCols = sValue["cols"];
		assert(wCols.IsArray());
		for (SizeType wIndex = 0; wIndex < wCols.Size(); wIndex++) {
			const Value& wCol = wCols[wIndex];
			tIndex wIndexCol = wCol["i"].GetInt();
			EnsureCol(wIndexCol)->Json(wCol,this,false);
		}
        // 3 Cells
		const Value& wCells = sValue["cells"];
		assert(wCells.IsArray());
		for (SizeType wIndex = 0; wIndex < wCells.Size(); wIndex++) {
			const Value& wCell = wCells[wIndex];
			tTempoPoint wPoint(wCell["c"].GetString());
			tIndex wIndexRow = wPoint.Row();
			tIndex wIndexCol = wPoint.Col();
			EnsureCell(wIndexRow,wIndexCol)->Json(wCell);
		}
        // 4 Merged
        const Value& wRangeMerged = sValue["merged"];
        assert(wRangeMerged.IsArray());
        for (SizeType wIndex = 0; wIndex < wRangeMerged.Size(); wIndex++) {
            const Value& wValueRange = wRangeMerged[wIndex];
            if (wValueRange.IsString()) {
                tString wRef=wValueRange.GetString();
                tSelect wSelect;
                tBool wResult = wSelect.Parse(wRef);
                if (wResult) {
                    tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
                    for (auto wItem : *wVectorSelect) {
                        tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                        if (wRect != nullptr) {
                            tRange* wRange= EnsureRange(wRect->Top(), wRect->Left(), wRect->Bottom(), wRect->Right());
                            wRange->SetMerged();
                        }
                    }
                }
            }
        }
        // 5 Conditional Format
        if (sValue.HasMember("cf")) {
            const Value& wConditionalFormat = sValue["cf"];
            assert(wConditionalFormat.IsArray());
            m_ConditionalFormatContainer=new tConditionalFormatContainer(m_Sheet);
            m_ConditionalFormatContainer->Json(wConditionalFormat, m_Sheet);
        }
	}

    tDouble tColRowCellRange::SizeRow(tIndex sRow,tUnitMetrics sUnit) {
        tDouble wSize=0;
        tColRow* wColRow=Row(sRow);
        if (wColRow != nullptr) {
            if (!wColRow->IsInClosedPath(this, true) && wColRow->DataVisible())
                wSize=SkMetrics::Convert(m_Sheet->SizeRow(sRow),tUnitMetrics::millimeters,sUnit);
        } else {
            wSize=SkMetrics::Convert(m_Sheet->WorkBook()->DefaultSizeRow(),tUnitMetrics::millimeters,sUnit);
        }
        return(wSize);
    }

    tDouble tColRowCellRange::SizeCol(tIndex sCol,tUnitMetrics sUnit) {
        tDouble wSize=0;
        tColRow* wColRow=Col(sCol);
        if (wColRow != nullptr) {
            if (!wColRow->IsInClosedPath(this, false))
                wSize=SkMetrics::Convert(m_Sheet->SizeCol(sCol),tUnitMetrics::millimeters,sUnit);
        } else {
            wSize= SkMetrics::Convert(m_Sheet->WorkBook()->DefaultSizeCol(),tUnitMetrics::millimeters,sUnit);
        }
        return(wSize);
    }
   

    namespace {
    class tLastExportedCellScan : public tSparseArrayCallBack<tAllocatorRef> {
        tColRowCellRange* m_Range;
        tIndex m_MaxRow = 0;
        tIndex m_MaxCol = 0;
    public:
        explicit tLastExportedCellScan(tColRowCellRange* sRange) : m_Range(sRange) {}
        tBool CallBack(tAllocatorRef sRef) override {
            tCell* wCell = m_Range->Cell(sRef);
            if (wCell != nullptr && !wCell->IsEmpty()) {
                const tIndex wRow = wCell->RowIndex();
                const tIndex wCol = wCell->ColIndex();
                if (wRow > m_MaxRow) {
                    m_MaxRow = wRow;
                }
                if (wCol > m_MaxCol) {
                    m_MaxCol = wCol;
                }
            }
            return true;
        }
        tIndex MaxRow() const { return m_MaxRow; }
        tIndex MaxCol() const { return m_MaxCol; }
    };

    void ScanLastExportedCell(
        tColRowCellRange* sRange,
        tIndex sMaxRow,
        tIndex sMaxCol,
        tIndex* oMaxRow,
        tIndex* oMaxCol
    ) {
        tLastExportedCellScan wScan(sRange);
        sRange->CallBackAllCellInRange(&wScan, 0, sMaxRow, 0, sMaxCol + 1);
        *oMaxRow = wScan.MaxRow();
        *oMaxCol = wScan.MaxCol();
    }
    } // namespace

    void tColRowCellRange::ComputeExtentUncached(tIndex& oLastRow, tIndex& oLastCol) {
        tIndex wRow = tIndex(m_Rows.RealSize());
        while (wRow > 0) {
            tColRow* wColRow = Row(wRow);
            if (wColRow != nullptr && !wColRow->IsVacantForUsedExtent()) {
                break;
            }
            wRow--;
        }
        tIndex wCol = tIndex(m_Cols.RealSize());
        while (wCol > 0) {
            tColRow* wColRowBand = Col(wCol);
            if (wColRowBand != nullptr && !wColRowBand->IsVacantForUsedExtent()) {
                break;
            }
            wCol--;
        }
        tIndex wMaxRow = 0;
        tIndex wMaxCol = 0;
        ScanLastExportedCell(this, tIndex(m_Rows.RealSize()), tIndex(m_Cols.RealSize()), &wMaxRow, &wMaxCol);
        if (wMaxRow > wRow) {
            wRow = wMaxRow;
        }
        if (wMaxCol > wCol) {
            wCol = wMaxCol;
        }
        oLastRow = wRow;
        oLastCol = wCol;
    }

    void tColRowCellRange::RefreshExtentCache() {
        ComputeExtentUncached(m_CachedLastRow, m_CachedLastCol);
    }

    void tColRowCellRange::InvalidateExtentCache() {
        m_CachedLastRow = -1;
        m_CachedLastCol = -1;
    }

    void tColRowCellRange::NotifySpillExtent(tIndex sBottom, tIndex sRight) {
        if (m_CachedLastRow < 0) {
            return;
        }
        if (sBottom > m_CachedLastRow || sRight > m_CachedLastCol) {
            RefreshExtentCache();
        }
    }

	tIndex tColRowCellRange::LastRow() {
        if (m_CachedLastRow >= 0) {
            return m_CachedLastRow;
        }
        RefreshExtentCache();
		return m_CachedLastRow;
	}

	tIndex tColRowCellRange::LastCol() {
        if (m_CachedLastCol >= 0) {
            return m_CachedLastCol;
        }
        if (m_CachedLastRow >= 0) {
            // LastRow() already refreshed both dimensions.
            return m_CachedLastCol;
        }
        RefreshExtentCache();
        return m_CachedLastCol;
	};

    tIndex tColRowCellRange::LastRowForJson() {
        const tIndex wContentLast = LastRow();
        tIndex wLast = wContentLast;
        for (tIndex wRow = 1; wRow <= tIndex(m_Rows.RealSize()); wRow++) {
            tColRow* wColRow = Row(wRow);
            if (ShouldSerializeColRowBand(wColRow, wRow, wContentLast) && wRow > wLast) {
                wLast = wRow;
            }
        }
        return wLast;
    }

    tIndex tColRowCellRange::LastColForJson() {
        const tIndex wContentLast = LastCol();
        tIndex wLast = wContentLast;
        for (tIndex wCol = 1; wCol <= tIndex(m_Cols.RealSize()); wCol++) {
            tColRow* wColRow = Col(wCol);
            if (ShouldSerializeColRowBand(wColRow, wCol, wContentLast) && wCol > wLast) {
                wLast = wCol;
            }
        }
        return wLast;
    }

    tDouble tColRowCellRange::SumWidth(tIndex sColStart,tIndex sColEnd,tUnitMetrics sUnit) {
        tDouble wResult=0;
        tDouble wDefault=SkMetrics::Convert(m_Sheet->WorkBook()->DefaultSizeCol(),tUnitMetrics::millimeters,sUnit);
        for(tIndex wInd=sColStart;wInd <= sColEnd;wInd++) {
            tColRow* wColRow=Col(wInd);
            if (wColRow != nullptr) {
                if (!wColRow->IsInClosedPath(this, false) && wColRow->DataVisible()) {
                    if (wColRow->m_Size==-1) {
                        wResult+=wDefault;
                    } else {
                        wResult+=SkMetrics::Convert(wColRow->Size(),tUnitMetrics::millimeters,sUnit);
                    }
                }
            } else {
                wResult+=wDefault;
            }
        }
        return(wResult);
    }

    tDouble tColRowCellRange::SumHeight(tIndex sRowStart,tIndex sRowEnd,tUnitMetrics sUnit) {
        tDouble wResult=0;
        tDouble wDefault=SkMetrics::Convert(m_Sheet->WorkBook()->DefaultSizeRow(),tUnitMetrics::millimeters,sUnit);
        for(tIndex wInd=sRowStart;wInd <= sRowEnd;wInd++) {
            tColRow* wColRow=Row(wInd);
            if (wColRow != nullptr) {
                if (!wColRow->IsInClosedPath(this, true) && wColRow->DataVisible()) {
                    if (wColRow->m_Size==-1) {
                        wResult+=wDefault;
                    } else {
                        wResult+=SkMetrics::Convert(wColRow->Size(),tUnitMetrics::millimeters,sUnit);
                    }
                }
            } else {
                wResult+=wDefault;
            }
        }
        return(wResult);
    }
        
    std::tuple<tIndex,tDouble> tColRowCellRange::IndexColByPos(tIndex sColStart,tDouble sPos,tUnitMetrics sUnit) {
        tIndex wInd=sColStart;
        tDouble wPos=0;
        while(wInd<=Cst_MaxCol) {
            tDouble wSize=SizeCol(wInd, sUnit);
            wPos+=wSize;
            if (wPos > sPos) return(std::make_tuple(wInd,(wPos-sPos)-wSize+1));
            wInd++;
        }
        return(std::make_tuple(0,0));
    }
        
    std::tuple<tIndex,tDouble> tColRowCellRange::IndexRowByPos(tIndex sRowStart,tDouble sPos,tUnitMetrics sUnit) {
        tIndex wInd=sRowStart;
        tDouble wPos=0;
        while(wInd<=Cst_MaxRow) {
            tDouble wSize=SizeRow(wInd, sUnit);
            wPos+=wSize;
            if (wPos > sPos) return(std::make_tuple(wInd,(wPos-sPos)-wSize+1));
            wInd++;
        }
        return(std::make_tuple(0,0));
    }
        
    tBool tColRowCellRange::CellValueEmpty(tIndex sRow,tIndex sCol) {
        tCell* wCell=Cell(sRow,sCol);
        if (wCell!=nullptr) return(wCell->IsValueEmpty());
        return(true);
    }
    
    tBool tColRowCellRange::IsRowVisible(tIndex sRow) {
        tColRow* wColRow=Row(sRow);
        if (wColRow!=nullptr) {
            return(wColRow->IsVisible(this,true));
        }
        return(true);
    }

    tBool tColRowCellRange::IsColVisible(tIndex sCol) {
        tColRow* wColRow=Col(sCol);
        if (wColRow!=nullptr) {
            return(wColRow->IsVisible(this,false));
        }
        return(true);
    }
        
    tRect tColRowCellRange::MoveCell(tPoint sCellPoint, tByte sKey, tByte sMeta, tRect sScreen) {
        if (sMeta==1) {
            return(MoveToCell(sCellPoint, sKey));
        }
        tPoint wCellPoint=sCellPoint;
        switch(sKey) {
            case tKey::t_Left: {
                tIndex wCol=wCellPoint.Col()-1;
                if (wCol<1) wCol=1;
                while(!IsColVisible(wCol) && (wCol>1)) wCol--;
                if (IsColVisible(wCol)) wCellPoint.Col(wCol);
                break;
            }
            case tKey::t_Up: {
                tIndex wRow=wCellPoint.Row()-1;
                if (wRow<1) wRow=1;
                while(!IsRowVisible(wRow) && (wRow>1)) wRow--;
                if (IsRowVisible(wRow)) wCellPoint.Row(wRow);
                break;
            }
            case tKey::t_Right: {
                // if is merged go to end
                tIndex wCol=wCellPoint.Col();
                tRange* wRangeMerged=MergedRange(wCellPoint.Row(), wCol);
                if (wRangeMerged!=nullptr) {
                    wCol=wRangeMerged->RightIndex();
                }
                wCol++;
            
                if (wCol>Cst_MaxCol) wCol=Cst_MaxCol;
                while(!IsColVisible(wCol) && (wCol<Cst_MaxCol)) wCol++;
                if (IsColVisible(wCol)) wCellPoint.Col(wCol);
                break;
            }
            case tKey::t_Down: {
                // if is merged go to end
                tIndex wRow=wCellPoint.Row();
                tRange* wRangeMerged=MergedRange(wRow,wCellPoint.Col());
                if (wRangeMerged!=nullptr) {
                    wRow=wRangeMerged->BottomIndex();
                }
                wRow++;
                if (wRow>Cst_MaxRow) wRow=Cst_MaxRow;
                while(!IsRowVisible(wRow) && (wRow<Cst_MaxRow)) wRow++;
                if (IsRowVisible(wRow)) wCellPoint.Row(wRow);
                break;
            }
            case tKey::t_PageUp : {
                break;
            }
            case tKey::t_PageDown : {
                break;
            }
            case tKey::t_Home : {
                tIndex wCol = 1;
                while (!IsColVisible(wCol) && (wCol < Cst_MaxCol)) {
                    wCol++;
                }
                if (IsColVisible(wCol)) {
                    wCellPoint.Col(wCol);
                }
                break;
            }
            case tKey::t_End : {
                tIndex wRow = wCellPoint.Row();
                tIndex wCol = LastCol();
                if (wCol < 1) {
                    wCol = 1;
                }
                while (wCol > 1 && CellValueEmpty(wRow, wCol)) {
                    wCol--;
                }
                while (!IsColVisible(wCol) && (wCol > 1)) {
                    wCol--;
                }
                if (IsColVisible(wCol)) {
                    wCellPoint.Col(wCol);
                }
                break;
            }
        }
        // Finally Is Merged
        tRect wRect;
        tRange* wRangeMerged=MergedRange(wCellPoint.Row(), wCellPoint.Col());
        if (wRangeMerged!=nullptr) {
            tTempoRect wTempoRect=wRangeMerged->Rect();
            wRect=wTempoRect;
        } else {
            wRect.Row(wCellPoint.Row());
            wRect.Col(wCellPoint.Col());
            wRect.Bottom(wCellPoint.Row());
            wRect.Right(wCellPoint.Col());
        }
        return(wRect);
    }
    
    tRect tColRowCellRange::MoveToCell(tPoint sCellPoint,tByte sDirection) {
        tIndex wRow=sCellPoint.Row();
        tIndex wCol=sCellPoint.Col();
        tPoint wCellPoint=sCellPoint;
        tBool wEmpty=CellValueEmpty(wRow,wCol);
        tRect wRect;
        switch (sDirection) {
            case tKey::t_Left : { // left
                if (wCol<1) break;
                while((wCol>1) && (wEmpty==CellValueEmpty(wRow,wCol))) {
                    wRect=MoveCell(tPoint(wRow,wCol), tKey::t_Left, 0, tRect());
                    wCol=wRect.Col();
                }
                if (wCol>0) wCellPoint.Col(wCol);
                break;
            }
            case tKey::t_Up: { // Up
                if (wRow<1) break;
                while((wRow>1) && (wEmpty==CellValueEmpty(wRow,wCol))) {
                    wRect=MoveCell(tPoint(wRow,wCol), tKey::t_Up, 0, tRect());
                    wRow=wRect.Row();
                }
                if (wRow>0) wCellPoint.Row(wRow);
                break;
            }
            case tKey::t_Right: { // Right
                if (wCol>=Cst_MaxCol) break;
                while((wCol<Cst_MaxCol) && (wEmpty==CellValueEmpty(wRow,wCol))) {
                    wRect=MoveCell(tPoint(wRow,wCol), tKey::t_Right, 0, tRect());
                    wCol=wRect.Col();
                }
                if (wCol<=Cst_MaxCol) wCellPoint.Col(wCol);
                break;
            }
            case tKey::t_Down : { // bottom
                if (wRow>=Cst_MaxRow) break;
                while((wRow<Cst_MaxRow) && (wEmpty==CellValueEmpty(wRow,wCol))) {
                    wRect=MoveCell(tPoint(wRow,wCol), tKey::t_Down, 0, tRect());
                    wRow=wRect.Row();
                }
                if (wRow<=Cst_MaxRow) wCellPoint.Row(wRow);
                break;
            }
        }
        
        // Finally Is Merged
        tRange* wRangeMerged=MergedRange(wCellPoint.Row(), wCellPoint.Col());
        if (wRangeMerged!=nullptr) {
            tTempoRect wTempoRect=wRangeMerged->Rect();
            wRect=wTempoRect;
        } else {
            wRect.Row(wCellPoint.Row());
            wRect.Col(wCellPoint.Col());
            wRect.Bottom(wCellPoint.Row());
            wRect.Right(wCellPoint.Col());
        }
        return(wRect);
    }


	tBool tColRowCellRange::SetCellJsonPayload(tIndex sRow, tIndex sCol, const tString& sJson) {
		if (!ValidRow(sRow) || !ValidCol(sCol)) {
			return false;
		}
		tAllocatorRef wAllocatorRef = CellAllocatorRef(sRow, sCol);
		m_CellClassContainer.SetCellJsonPayload(wAllocatorRef, sJson);
		return true;
	}

	tBool tColRowCellRange::GetCellJsonPayload(tIndex sRow, tIndex sCol, tString& sOutJson) {
		if (!ValidRow(sRow) || !ValidCol(sCol)) {
			return false;
		}
		tAllocatorRef wAllocatorRef = CellAllocatorRef(sRow, sCol);
		return m_CellClassContainer.GetCellJsonPayload(wAllocatorRef, sOutJson);
	}

	tBool tColRowCellRange::DeleteCellJsonPayload(tIndex sRow, tIndex sCol) {
		if (!ValidRow(sRow) || !ValidCol(sCol)) {
			return false;
		}
		tAllocatorRef wAllocatorRef = CellAllocatorRef(sRow, sCol);
		return m_CellClassContainer.DeleteCellJsonPayload(wAllocatorRef);
	}
    
    // Conditional Format ==================================================
    tConditionalFormat* tColRowCellRange::AddConditionalFormat(tConditionalFormatType sType,tString sRef) {
        if (m_ConditionalFormatContainer==nullptr) {
            m_ConditionalFormatContainer=new tConditionalFormatContainer(m_Sheet);
        }
        return(m_ConditionalFormatContainer->Add(sType, sRef,m_Sheet));
    }

        
    tConditionalFormat* tColRowCellRange::ConditionalFormat(tConditionalFormatType sType,tString sRef) {
        if (m_ConditionalFormatContainer==nullptr) {
            return(nullptr);
        }
        return(m_ConditionalFormatContainer->ConditionalFormat(sType,sRef));
    }
        
    tBool tColRowCellRange::RemoveConditionalFormatByKey(tString sKey) {
        if (m_ConditionalFormatContainer==nullptr) {
            return(false);
        }
        tBool wResult=m_ConditionalFormatContainer->RemoveByKey(sKey);
        if (wResult) {
            if (m_ConditionalFormatContainer->IsEmpty()) {
                delete(m_ConditionalFormatContainer);
                m_ConditionalFormatContainer=nullptr;
            }
        }
        return(wResult);
    }
    
    tBool tColRowCellRange::RemoveConditionalFormatByRect(tConditionalFormatType sType,tString sRef,tRect* sAreaDelete) {
        if (m_ConditionalFormatContainer==nullptr) {
            return(false);
        }
        tBool wResult=m_ConditionalFormatContainer->RemoveByRect(sType,sRef,sAreaDelete);
        if (wResult) {
            if (m_ConditionalFormatContainer->IsEmpty()) {
                delete(m_ConditionalFormatContainer);
                m_ConditionalFormatContainer=nullptr;
            }
        }
        return(wResult);
    }

    tRangeConditionnalFormat* tColRowCellRange::ConditionalFormatByRange(tRange* sRange) {
        if (m_ConditionalFormatContainer==nullptr) {
            return(nullptr);
        }
        return(m_ConditionalFormatContainer->ConditionalFormatByRange(sRange));
    }

    void tColRowCellRange::StripOrphanConditionalFormatExtension() {
        if (m_ConditionalFormatContainer == nullptr) {
            return;
        }
        for (tAllocatorRef wAllocator = 1; wAllocator <= m_AllocatorRange.Size(); wAllocator++) {
            if (IsDeletedRange(wAllocator)) {
                continue;
            }
            tRange* wRange = m_AllocatorRange(wAllocator);
            if (wRange == nullptr || !wRange->IsConditionalFormat()) {
                continue;
            }
            tRangeConditionnalFormat* wRangeConditionalFormat =
                m_ConditionalFormatContainer->ConditionalFormatByRange(wRange);
            if (wRangeConditionalFormat == nullptr) {
                wRange->RemoveConditionalFormat();
            } else {
                // Partial undo/redo can leave per-type extension flags without a map entry.
                if (wRange->IsCFHR()
                    && wRangeConditionalFormat->ConditionalFormat(
                           tConditionalFormatType::t_HighlightCellsRules) == nullptr) {
                    wRange->RemoveCFHR();
                }
                if (wRange->IsCFDB()
                    && wRangeConditionalFormat->ConditionalFormat(
                           tConditionalFormatType::t_DataBars) == nullptr) {
                    wRange->RemoveCFDB();
                }
                if (wRange->IsCFCS()
                    && wRangeConditionalFormat->ConditionalFormat(
                           tConditionalFormatType::t_ColorScales) == nullptr) {
                    wRange->RemoveCFCS();
                }
                if (wRange->IsCFIS()
                    && wRangeConditionalFormat->ConditionalFormat(
                           tConditionalFormatType::t_IconSets) == nullptr) {
                    wRange->RemoveCFIS();
                }
                if (wRange->IsCFCF()
                    && wRangeConditionalFormat->ConditionalFormat(
                           tConditionalFormatType::t_CustomFormulas) == nullptr) {
                    wRange->RemoveCFCF();
                }
            }
            if (!wRange->IsConditionalFormat() && wRange->IsEmpty()) {
                DeleteRangeByAllocatorRef(wAllocator, true);
            }
        }
    }

    void tColRowCellRange::StripMergedOnConditionalFormatRanges() {
        for (tAllocatorRef wAllocator = 1; wAllocator <= m_AllocatorRange.Size(); wAllocator++) {
            if (IsDeletedRange(wAllocator)) {
                continue;
            }
            tRange* wRange = m_AllocatorRange(wAllocator);
            if (wRange == nullptr) {
                continue;
            }
            if (wRange->IsConditionalFormat() && wRange->IsMerged()) {
                wRange->RemoveMerged();
            }
        }
    }
        
    
        
    tConditionalFormatContainer* tColRowCellRange::ConditionalFormatContainer() {
        return(m_ConditionalFormatContainer);
    }

    tBool tColRowCellRange::ClearInProgress() {
        return(m_ClearInProgress);
    }

    namespace {

    class tCallBackReleaseCellFormat : public tSparseArrayCallBack<tAllocatorRef> {
        tColRowCellRange* m_ColRowCellRange;
        tWorkBook* m_WorkBook;
    public:
        tCallBackReleaseCellFormat(tColRowCellRange* sColRowCellRange, tWorkBook* sWorkBook)
            : tSparseArrayCallBack<tAllocatorRef>(),
              m_ColRowCellRange(sColRowCellRange),
              m_WorkBook(sWorkBook) {}

        tBool CallBack(tAllocatorRef sAllocatorRef) override {
            tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
            if (wCell != nullptr && wCell->Css() != 0) {
                m_WorkBook->DeleteCellFormat(wCell->Css());
                wCell->Css(0);
            }
            return(true);
        }
    };

    } // namespace

    void tColRowCellRange::ReleaseAllCellFormats() {
        if (m_Sheet == nullptr) {
            return;
        }
        tWorkBook* wWorkBook = m_Sheet->WorkBook();
        if (wWorkBook == nullptr) {
            return;
        }

        if (m_ConditionalFormatContainer != nullptr) {
            m_ConditionalFormatContainer->ReleaseAppliedFormats();
        }

        const tIndex wPhysLastRow = tIndex(m_Rows.RealSize());
        const tIndex wPhysLastCol = tIndex(m_Cols.RealSize());
        for (tIndex wRow = 0; wRow <= wPhysLastRow; wRow++) {
            tColRow* wColRow = Row(wRow);
            if (wColRow != nullptr && wColRow->Css() != 0) {
                wWorkBook->DeleteCellFormat(wColRow->Css());
                wColRow->Css(0);
            }
        }
        for (tIndex wCol = 0; wCol <= wPhysLastCol; wCol++) {
            tColRow* wColRow = Col(wCol);
            if (wColRow != nullptr && wColRow->Css() != 0) {
                wWorkBook->DeleteCellFormat(wColRow->Css());
                wColRow->Css(0);
            }
        }

        tCallBackReleaseCellFormat wCallBack(this, wWorkBook);
        CallBackAllCellPhysical(&wCallBack);
    }

    tVectorCell* tColRowCellRange::FindCell(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell) {
        tCallBackFindCell wCallBackFindCell(this, sSearch, sMatchCase, sMatchEntireCell);
        CallBackAllCell(&wCallBackFindCell);
        return(wCallBackFindCell.Vector());
    }

    tVectorString* tColRowCellRange::FindUniqueValue(tRect sRect) {
        tCallBackFindUniqueValue wCallBackFindUniqueValue(this);
        tAllocatorRef wTop = sRect.Top();
        tAllocatorRef wBottom = sRect.Bottom();
        tAllocatorRef wLeft = sRect.Left();
        tAllocatorRef wRight = sRect.Right();
        CallBackAllCellInRange(&wCallBackFindUniqueValue, wTop, wBottom, wLeft, wRight);
        return(wCallBackFindUniqueValue.Vector());
    }

#ifdef checksp
	//=========================================================================
	//! Used for callback of cell Check
	class tCallBackCheck : public tSparseArrayCallBack<tAllocatorRef> {
	protected:
		//! Sheet. 
		tColRowCellRange* m_ColRowCellRange;
	public:
		/// @brief		Constructor SkCallBackCheck with owner tColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackCheck(tColRowCellRange* sColRowCellRange) : tSparseArrayCallBack<tAllocatorRef>() {
			m_ColRowCellRange = sColRowCellRange;
		}
		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tAllocatorRef Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef) {
			tCell* wCell=m_ColRowCellRange->Cell(sAllocatorRef);
			if (wCell!=nullptr) wCell->Check();
			return(true);
		}
	};

 	void tColRowCellRange::Check() {
		const tIndex wPhysLastRow = tIndex(m_Rows.RealSize());
		const tIndex wPhysLastCol = tIndex(m_Cols.RealSize());
		// 1 prepare Check NbCells — physical grid; must match every IncNbCells slot.
		for (tIndex wRow = 0; wRow <= wPhysLastRow; wRow++) {
			tColRow* wColRow = Row(wRow);
			if (wColRow != nullptr)
				wColRow->m_NbCellsCheck=0;
		}
		for (tIndex wCol = 0; wCol <= wPhysLastCol; wCol++) {
			tColRow* wColRow = Col(wCol);
			if (wColRow != nullptr)
				wColRow->m_NbCellsCheck = 0;
		}
		// 2 Check Cell
		tCallBackCheck wCallBackCheck(this);
		CallBackAllCellPhysical(&wCallBackCheck);

		//  4 Check Colrow has good range and good Cells numbers
		for (tIndex wRow = 0; wRow <= wPhysLastRow; wRow++) {
			tColRow* wColRow = Row(wRow);
			if (wColRow != nullptr) 
				wColRow->Check(m_Sheet,true);
		}
		for (tIndex wCol = 0; wCol <= wPhysLastCol; wCol++) {
			tColRow* wColRow = Col(wCol);
			if (wColRow != nullptr) 
				wColRow->Check(m_Sheet, false);
		}
        // 5 verify CellClassAttribute ========================================
        tCellClassContainer::tMapName::iterator wIterator;
        tCellClassContainer::tMapName& wMapName=m_CellClassContainer.MapName();
        for (auto wMapItem : wMapName) {
            tCell* wCell=Cell(wMapItem.second);
            if (wCell==nullptr) {
                tStringStream wStream;
                wStream << "throw: Verify CellClass " << wMapItem.first  << ":" << wMapItem.second << " Cell=nullptr !";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            } else
            if (wCell->ClassAttribute()==nullptr) {
                tStringStream wStream;
                wStream << "throw: Verify CellClass " << wMapItem.first  << " don't exist !";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            } else {
                if (wCell->ClassAttribute()->RefName()!=wMapItem.first) {
                    tStringStream wStream;
                    wStream << "throw: Verify CellClass " << wMapItem.first  << " not equal " << wCell->ClassAttribute()->RefName();
                    cerr << wStream.str() << endl;
                    throw(tExceptionInternalError(wStream.str()));
                }
            }
        }
        // Super Checks =======================================================
        // Verify if all élemernts are deleted
        // Check All Cells ====================================================
        for(tAllocatorRef wAllocator=1; wAllocator<=m_AllocatorCell.Size();wAllocator++) {
            if (!IsDeletedCell(wAllocator)) {
                tCell* wCell=m_AllocatorCell(wAllocator);
                wCell->Check();
            }
        }
        // Check All ranges ====================================================
        for(tAllocatorRef wAllocator=1; wAllocator<=m_AllocatorRange.Size();wAllocator++) {
            if (!IsDeletedRange(wAllocator)) {
                tRange* wRange=m_AllocatorRange(wAllocator);
                wRange->Check();
            }
        }
        
        // Check Conditional Format ============================================
        if (m_ConditionalFormatContainer!=nullptr) {
            m_ConditionalFormatContainer->Check();
        }
        
	}
#endif
        
#ifdef checkfo
        //=========================================================================
        //! Used for callback Cell checkFormat
        class tCallBackCellCheckFormat : public tSparseArrayCallBack<tAllocatorRef> {
        protected:
            //! Sheet.
            tColRowCellRange* m_ColRowCellRange;
            tFormatApi*       m_FormatApi;
        public:
            /// @brief        Constructor SkCallBackCheck with owner tColRowCellRange.
            tCallBackCellCheckFormat(tColRowCellRange* sColRowCellRange) : tSparseArrayCallBack<tAllocatorRef>() {
                m_ColRowCellRange = sColRowCellRange;
                m_FormatApi=m_ColRowCellRange->Sheet()->WorkBook()->FormatApi();
            }
            /// @brief        Call method for calculate m_Value if return false stop process.
            /// @param[in]    sAllocatorRef tAllocatorRef Index on allocator cell
            virtual tBool CallBack(tAllocatorRef sAllocatorRef) {
                tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
                if (m_FormatApi!=nullptr) {
                    if (wCell->Css()!=0) {
                        m_FormatApi->IncCheck(wCell->Css());
                    }
                }
                return(true);
            }
        };

        void tColRowCellRange::CheckFormat() {
            // 1 Row & Col
            tFormatApi* wFormatApi=Sheet()->WorkBook()->FormatApi();
            if (wFormatApi!=nullptr) {
                const tIndex wPhysLastRow = tIndex(m_Rows.RealSize());
                const tIndex wPhysLastCol = tIndex(m_Cols.RealSize());
                // 1 prepare Check NbCells
                for (tIndex wRow = 0; wRow <= wPhysLastRow; wRow++) {
                    tColRow* wColRow = Row(wRow);
                    if (wColRow != nullptr) {
                        if (wColRow->Css()!=0) {
                            wFormatApi->IncCheck(wColRow->Css());
                        }
                    }
                }
                
                for (tIndex wCol = 0; wCol <= wPhysLastCol; wCol++) {
                    tColRow* wColRow = Col(wCol);
                    if (wColRow != nullptr) {
                        if (wColRow->Css()!=0) {
                            wFormatApi->IncCheck(wColRow->Css());
                        }
                    }
                }
            }
            // 2 Cell
            tCallBackCellCheckFormat wCallBackCheckFormat(this);
            CallBackAllCellPhysical(&wCallBackCheckFormat);
        }
        
#endif
#ifdef _DEBUGSK
    tString tColRowCellRange::Debug() {
        tStringStream wStream;

        //Loop on Cell
        wStream << "Cell:" << endl;
        for(tIndex wRow=0; wRow<=LastRow();wRow++) {
            for(tIndex wCol=0; wCol<=LastCol();wCol++) {
                tCell* wCell=Cell(wRow,wCol);
                if (wCell!=nullptr) {
                    wStream << wCell->Debug();
                }
            }
        }
        
        // Loop on Range
        wStream << "Range:" << endl;
        tColRow::tContainerRange wResultRow;
        tColRow* wColRow =nullptr;
        for(tIndex wRow=0; wRow<=LastRow();wRow++) {
            wColRow=Row(wRow);
            if (wColRow!=nullptr) {
                tColRow::tContainerRange* wContainerRange = wColRow->ContainerRange();
                tColRow::tContainerRange::tContainerClass* wContainer=wContainerRange->Container();
                tColRow::tContainerRange::tIteratorClass wIterator;
                for (wIterator = wContainer->begin(); wIterator != wContainer->end(); wIterator++) {
                    if (!(wResultRow.Exist(*wIterator))) {
                        wResultRow.InsertClass(*wIterator);
                        tRange* wRange = m_AllocatorRange(*wIterator);
                        if (wRange!=nullptr) {
                            wStream  << wRange->Debug();
                        }
                    }
                }
            }
            
            if (m_VectorVolatileCell.size()!=0) {
                wStream << "Volatile Cell : " << Sheet()->Name() << "----------" << endl;
                for (const tAllocatorRef wAllocatorRef : m_VectorVolatileCell) {
                    tCell* wCell = m_AllocatorCell(wAllocatorRef);
                    if (wCell == nullptr) {
                        wStream << "Cell-Allocator(" << wAllocatorRef << "): nullptr" << endl;
                        continue;
                    }
                    wStream << "Cell:" << wCell->StrRef() << ":";
                    const tFormula* wFormula=wCell->Formula();
                    if (wFormula!=nullptr) {
                        if (!wFormula->BitSetVolatile().Empty()) {
                            wStream << " ..V.. ";
                            if (wFormula->IsVolatile())  wStream << " All ";
                            if (wFormula->IsRowVolatile())  wStream << " Row ";
                            if (wFormula->IsColVolatile())  wStream << " Col ";
                        } else {
                            wStream << " Formula " << wCell->FormulaStr() << " Is not Volatif";
                        }
                    } else {
                        wStream << " Formula nullptr";
                    }
                    wStream << endl;
                }
            }
        }
        //Loop on Conditional Format ==========================================
        if (m_ConditionalFormatContainer!=nullptr) {
            wStream << m_ConditionalFormatContainer->Debug();
        }
        
        wStream << "Range by allocator ========================================" << endl;
        for(tAllocatorRef wAllocator=1; wAllocator<=m_AllocatorRange.Size();wAllocator++) {
            if (!IsDeletedRange(wAllocator)) {
                tRange* wRange=m_AllocatorRange(wAllocator);
                    wStream  <<  wRange->Debug();
#ifdef checksp
                    wRange->Check();
#endif
            }
        }
      
        
        return(wStream.str());
    }
    
    void tColRowCellRange::DebugCell(tString sTitle,tString sRef) {
        tIndex wRow,wCol;
        cout <<  sTitle <<  " -> " << sRef;
        if (ParseCell(sRef, wRow, wCol)) {
            tCell* wCell=Cell(wRow, wCol);
            if (wCell!=nullptr) {
                cout << ":" << wCell->FormulaStr() << "=" << wCell->Value();
            } else {
                cout <<" nullptr";
            }
        }
        cout << endl;
    }
#endif
	tLong tColRowCellRange::NbCell() {
		return(tLong(m_AllocatorCell.Size()));
	}

	tLongLong tColRowCellRange::MemoryColRowSize() {
		return(m_AllocatorCol.MemorySize()+ m_AllocatorRow.MemorySize());
	}

	tLongLong tColRowCellRange::MemoryCellSize() {
		return(m_AllocatorCell.MemorySize());
	}

	tLongLong tColRowCellRange::MemoryRangeSize() {
		return(m_AllocatorRange.MemorySize());
	}


	// One class For all application
	tStaticColRowCellRange* wStaticAllocatorTable=nullptr;

	// SkTableAllocator =======================================================
	// Container of all tColRowCellRange
	//=========================================================================
	tStaticColRowCellRange::tStaticColRowCellRange() {}
	tStaticColRowCellRange::~tStaticColRowCellRange() {
 	}
	
	tColRowCellRange* tStaticColRowCellRange::Alloc(tAllocatorRef& sAllocatorRef) {
		tColRowCellRange* wColRowCellRange;
		tie(sAllocatorRef, wColRowCellRange) = m_ColRowCellRange.Alloc();
#ifdef debugcolrowcellrange
		cout << "tStaticColRowCellRange::Alloc " << sAllocatorRef << endl;
#endif
		return(wColRowCellRange);
	}
	tColRowCellRange* tStaticColRowCellRange::ColRowCellRange(tAllocatorRef sAllocatorRef) {
        // Can be null (Delete m_Api) Cell & Range inter Sheet
        if (m_ColRowCellRange.IsNullptr(sAllocatorRef)) return(nullptr);
		return(m_ColRowCellRange(sAllocatorRef));
	}


	void tStaticColRowCellRange::Delete(tAllocatorRef sAllocatorRef) {
#ifdef debugcolrowcellrange
		cout << "tStaticColRowCellRange::Remove " << sAllocatorRef << endl;
#endif
		m_ColRowCellRange.Delete(sAllocatorRef);
	}

	tStaticColRowCellRange* tStaticColRowCellRange::Instance() {
		if (wStaticAllocatorTable == nullptr)  wStaticAllocatorTable = new tStaticColRowCellRange();
		return(wStaticAllocatorTable);
	};

	void tStaticColRowCellRange::Done() {
		if (wStaticAllocatorTable != nullptr) {
			delete(wStaticAllocatorTable);
			wStaticAllocatorTable = nullptr;
		}
	}
	

}; // end of namespace
