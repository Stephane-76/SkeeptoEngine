//=============================================================================
// SkSpreadSheet Api
//=============================================================================
#include <rapidjson/error/en.h>
#include <cctype>

#include "../include/SkApi.hpp"
#include "../include/SkFloatingObject.hpp"
#include "../include/SkFillSeries.hpp"


#ifdef _DEBUGSK
// Build flag: _DEBUGSK active
#endif
#ifdef _RELEASE
// Build flag: _RELEASE active
#endif
#ifdef checksp
// Build flag: checksp active
#endif
#ifdef checkfo
// Build flag: checkfo active
#endif

#define _debugjson

namespace SkSpreadSheet {

    namespace {

        static tBool SheetNameNeedsQuotes(const tString& sSheetName) {
            if (sSheetName.empty()) {
                return false;
            }
            for (tChar wChar : sSheetName) {
                if (!std::isalnum(static_cast<unsigned char>(wChar)) && wChar != '_') {
                    return true;
                }
            }
            return false;
        }

        static tString SheetQualifiedCellRef(tSheet* sSheet, tCell* sCell) {
            if (sSheet == nullptr || sCell == nullptr) {
                return "";
            }
            const tString wCellRef = sCell->StrRef();
            const tString wSheetName = sSheet->Name();
            if (wSheetName.empty()) {
                return wCellRef;
            }
            if (!SheetNameNeedsQuotes(wSheetName)) {
                return wSheetName + "!" + wCellRef;
            }
            tString wEscaped;
            wEscaped.reserve(wSheetName.size() + 2);
            for (tChar wChar : wSheetName) {
                if (wChar == '\'') {
                    wEscaped += "''";
                } else {
                    wEscaped += wChar;
                }
            }
            return "'" + wEscaped + "'!" + wCellRef;
        }

    } // namespace

    // tApi ===================================================================
	tApi::tApi() : tClass(), m_MultiUserActive(false), m_LastModelClassAttribute(nullptr) {
		m_SpreadSheetContainer = tSpreadSheetContainer::Instance();
		m_Application = tApplication::Instance();
		m_Application->ClearUndoRedo();
        RegisterCellClassUnit();
        InstallCellClassModelStubHandler();
        IsUndoActif(true);
	}

	tApi::~tApi() {
		// Very Important (before clear Allocator Cell Attribute & Range)
		m_Application->ClearUndoRedo();
        tClassFactory::Instance()->Clear();
		DoneSpreadSheet();
	}
    /*
    tBool tApi::Alone() { return(m_Mode.Value(t_Alone)); }
    void tApi::Alone(tBool sAlone) { if(sAlone) { m_Mode.Set(t_Alone); } else { m_Mode.Clear(t_Alone); }}
    */
    void tApi::IsUndoActif(tBool sIsUndoActif) { if (sIsUndoActif) { m_Mode.Set(t_IsUndoActif); } else { m_Mode.Clear(t_IsUndoActif); }}
    tBool tApi::IsUndoActif() { return(m_Mode.Value(t_IsUndoActif)); }

    void tApi::IsJson(tBool sIsJson) { if (sIsJson) { m_Mode.Set(t_IsJson); } else { m_Mode.Clear(t_IsJson); }}
    tBool tApi::IsJson() { return(m_Mode.Value(t_IsJson)); }

    tBool tApi::Client() { return(m_Mode.Value(t_Client)); }
    void tApi::Client(tBool sClient) { if (sClient) { m_Mode.Set(t_Client); } else { m_Mode.Clear(t_Client); }}

    tBool tApi::Server() { return(m_Mode.Value(t_Server)); }
    void tApi::Server(tBool sServer) { if (sServer) { m_Mode.Set(t_Server); } else { m_Mode.Clear(t_Server); }}

    void tApi::MultiUserActive(tBool sActive) { m_MultiUserActive = sActive; }
    tBool tApi::MultiUserActive() { return(m_MultiUserActive); }

    tBool tApi::IsRenameAllowed() {
        // In pure local mode (no server, no peers), renames are always safe:
        // the undo stream never leaves this process so there is no stale
        // reference to rebase.
        if (!Client()) {
            return(true);
        }
        // In client/collaborative mode, renames are only accepted when we are
        // currently alone. MultiUserActive() is virtual so tInterfaceWeb can
        // return the live value computed from its dispatcher.
        return(!MultiUserActive());
    }

	tBool tApi::Do(tUndo* sUndo) {
        tBool wOk = false;
        if (tUndoSpreadSheet* wSpreadSheetUndo = dynamic_cast<tUndoSpreadSheet*>(sUndo)) {
            wOk = DispatchSpreadSheetDo(m_Application->UndoRedoContainer(), wSpreadSheetUndo);
        } else {
            wOk = m_Application->Do(sUndo);
        }
        #ifdef checksp
            Check();
        #endif
        return(wOk);
	}

	tBool tApi::Undo() {
        tBool wOk = DispatchSpreadSheetUndo(m_Application->UndoRedoContainer());
#ifdef checksp
        Check();
#endif
        return(wOk);
	}

	tBool tApi::Redo() {
        tBool wOk = DispatchSpreadSheetRedo(m_Application->UndoRedoContainer());
#ifdef checksp
        Check();
#endif
        return(wOk);
	}

	tUndo* tApi::LastUndo() {
		return(m_Application->LastUndo());
	}
        
    tUndo* tApi::LastRedo() {
        return(m_Application->LastRedo());
    }

    void tApi::ClearUndoRedo() {
        m_SpreadSheetContainer->ClearUndoRedo();
    }


	tDouble tApi::UndoSizeRow(tIndex sIndex, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->SizeRow(sIndex));
	}

	tDouble tApi::UndoSizeCol(tIndex sIndex, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->SizeCol(sIndex));
	}

	tBool tApi::CellValue(tIndex sRow, tIndex sCol, tVariant sValue,tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tCell* wCell=sSheet->EnsureCell(sRow, sCol);
		if (wCell == nullptr) return(false);
		tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
		tString wHeaderError;
		if (!tRangeData::ValidateHeaderCellValue(wWorkBook, sSheet, sRow, sCol, sValue, &wHeaderError)) {
			// tWorkBook::LemonInterface() may be nullptr here: m_LemonInterface is only
			// lazily initialized in CompilCell()/CellValue(tCell*), never in this import path.
			// Fall back to the container singleton (always valid) to avoid a null deref.
			tLemonInterface* wLemon = wWorkBook->LemonInterface();
			if (wLemon == nullptr) {
				wLemon = tSpreadSheetContainer::Instance()->LemonInterface();
			}
			if (wLemon != nullptr) {
				wLemon->Error(wHeaderError, 0, 0);
			}
			return false;
		}
		wCell->ClearFormulaAndVariant();
		// is Formula		
		if (sValue.Type() == tVariantType::t_string) {
			tString wValue = sValue.String();
			if (wValue.length() > 0) {
				if (wValue[0] == '=') {
					wValue.erase(0, 1);
					tBool wResult = wWorkBook->CompilCell(wCell, wValue.c_str());
					return(wResult);
				}
			}
		}
		
		wCell->Value(sValue);
		tString wTableName;
		tRange* wTableRange = nullptr;
		tie(wTableName, wTableRange) = sSheet->FindRangeDataCovered(sRow, sCol);
		if (wTableRange != nullptr && wTableRange->IsData() && sRow == wTableRange->TopIndex()) {
			tRangeData* wRangeData = wWorkBook->RangeData(wTableName);
			if (wRangeData != nullptr && wRangeData->HasHeaders()) {
				wRangeData->CaptureHeaderLabelFromCell(sSheet, wTableRange, sCol, sValue.Str());
			}
		}
		return(true);
	}

	tBool tApi::CellValue(tString sRef, tVariant sValue, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
        tSelect wSelect;
		tBool wResult=wSelect.Parse(sRef);
		if (wResult) {
			tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
			for (auto wItem : *wVectorSelect) {
                if (wItem!=nullptr) {
                    tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                    if (wRect != nullptr) {
                        for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                            for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                                CellValue(wRow, wCol,sValue);
                            }
                        }
                    } else {
                        wResult=CellValue(wItem->Row(), wItem->Col(),sValue);
                    }
                }
			}
		}
		return(wResult);
	}

	tVariant tApi::CellValue(tIndex sRow, tIndex sCol, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tCell* wCell = sSheet->Cell(sRow, sCol);
		if (wCell != nullptr) {
            return(wCell->CalculableValue());
		}
		return(tVariant());
	}

	tVariant tApi::CellValue(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRow; tIndex wCol;
		if (ParseCell(sRef, wRow, wCol)) {
			tCell* wCell = sSheet->Cell(wRow, wCol);
			if (wCell != nullptr) return(wCell->CalculableValue());
		}
		return(tVariant());
	}

    tVariant tApi::CellCalculableScalarValue(tString sRef, tSheet* sSheet) {
        return tCell::CalculableScalarFromVariant(CellValue(sRef, sSheet));
    }

    tString tApi::CellFormat(tIndex sRow, tIndex sCol, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tWorkBook* wWorkBook=m_SpreadSheetContainer->ActiveWorkBook();
        tFormatApi* wFormatApi=wWorkBook->FormatApi();
        if (wFormatApi!=nullptr) {
            tFormatRef wFormatSheet=sSheet->Css();
            
            tFormatRef wFormatCol=0;
            tColRow* wCol=sSheet->Col(sCol);
            if (wCol!=nullptr) wFormatCol=wCol->Css();
            
            tFormatRef wFormatRow=0;
            tColRow* wRow=sSheet->Row(sRow);
            if (wRow!=nullptr) wFormatRow=wRow->Css();
            
            tFormatRef wFormatCell=0;
            tCell* wCell = sSheet->Cell(sRow, sCol);
            if (wCell!=nullptr) wFormatCell=wCell->Css();
                
            // Optimize non format on sheet , col & row
            if ((wFormatSheet==0) && (wFormatCol==0) && (wFormatRow==0)) {
                if (wFormatCell!=0) return(wWorkBook->CellFormat(wFormatCell));
            } else {
                tVectorFormatRef wVector;
                if (wFormatSheet!=0) wVector.push_back(wFormatSheet);
                if (wFormatRow!=0) wVector.push_back(wFormatRow);
                if (wFormatCol!=0) wVector.push_back(wFormatCol);
                if (wFormatCell!=0) wVector.push_back(wFormatCell);
                return(wWorkBook->CellFormat(&wVector));
            }
        }
        return("");
    }

    tString tApi::CellFormat(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tIndex wRow; tIndex wCol;
        if (ParseCell(sRef, wRow, wCol)) {
            return(CellFormat(wRow, wCol,sSheet));
        }
        return("");
    }

    tString tApi::FormatValueWithCellFormat(tString sRef, tDouble sValue, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tIndex wRow = 0;
        tIndex wCol = 0;
        if (!ParseCell(sRef, wRow, wCol)) {
            return("");
        }
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return("");
        }
        tFormatApi* wFormatApi = wWorkBook->FormatApi();
        if (wFormatApi == nullptr) {
            tStringStream wStream;
            wStream << sValue;
            return wStream.str();
        }

        tFormatRef wFormatSheet = sSheet->Css();
        tFormatRef wFormatCol = 0;
        tColRow* wColRow = sSheet->Col(wCol);
        if (wColRow != nullptr) {
            wFormatCol = wColRow->Css();
        }
        tFormatRef wFormatRow = 0;
        tColRow* wRowRow = sSheet->Row(wRow);
        if (wRowRow != nullptr) {
            wFormatRow = wRowRow->Css();
        }
        tFormatRef wFormatCell = 0;
        tCell* wCell = sSheet->Cell(wRow, wCol);
        if (wCell != nullptr) {
            wFormatCell = wCell->Css();
        }

        tVariant wVariant(sValue);
        tString wResult;
        if ((wFormatSheet == 0) && (wFormatCol == 0) && (wFormatRow == 0) && (wFormatCell != 0)) {
            wResult = wFormatApi->CellFormatString(wFormatCell, &wVariant);
        } else {
            tVectorFormatRef wVector;
            if (wFormatSheet != 0) {
                wVector.push_back(wFormatSheet);
            }
            if (wFormatRow != 0) {
                wVector.push_back(wFormatRow);
            }
            if (wFormatCol != 0) {
                wVector.push_back(wFormatCol);
            }
            if (wFormatCell != 0) {
                wVector.push_back(wFormatCell);
            }
            if (wVector.empty()) {
                tStringStream wStream;
                wStream << sValue;
                return wStream.str();
            }
            wResult = wFormatApi->CellFormatString(&wVector, &wVariant);
        }
        if (wResult.empty()) {
            tStringStream wStream;
            wStream << sValue;
            return wStream.str();
        }
        return wResult;
    }

    
	tVariant tApi::CellAttributeValue(tString sRef, tString sAttribute, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRow; tIndex wCol;
		if (ParseCell(sRef, wRow, wCol)) {
			tCellAttribute* wCellAttribute = sSheet->CellAttribute(wRow, wCol,sAttribute);

			if (wCellAttribute != nullptr) return(wCellAttribute->Value());
		}
		return(tVariant());
	}

	tCellAttribute* tApi::CellAttribute(tString sRef, tString sAttribute, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRow; tIndex wCol;
		if (ParseCell(sRef, wRow, wCol)) {
			tCellAttribute* wCellAttribute = sSheet->CellAttribute(wRow, wCol, sAttribute);

			if (wCellAttribute != nullptr) return(wCellAttribute);
		}
		return(nullptr);

	}
    // Direct =================================================================
	tDouble tApi::SizeRow(tIndex sIndex, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->SizeRow(sIndex));
	}

	tDouble tApi::SizeCol(tIndex sIndex, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->SizeCol(sIndex));
	}

	tBool tApi::UndoSizeRow(tIndex sBegin, tIndex sEnd,tDouble sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoChangeSize* wUndoChangeSize = new tUndoChangeSize(true,sBegin,sEnd,sSize,m_Mode,sSheet);
		return(Do(wUndoChangeSize));
	}

	tBool tApi::UndoSizeCol(tIndex sBegin, tIndex sEnd, tDouble sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoChangeSize* wUndoChangeSize = new tUndoChangeSize(false, sBegin,sEnd, sSize,m_Mode,sSheet);
		return(Do(wUndoChangeSize));
	}

    tBool tApi::RegisterClassAttribute(tString sClassName,tString sLabel,tString sFamily) {
        tCellModelClassAttribute* wCellModelClassAttribute = new tCellModelClassAttribute(sClassName,sLabel,sFamily, &CreateGenericCellClassAttribute);
        tBool wOk=tClassFactory::Instance()->Register(wCellModelClassAttribute);
        if (wOk) {
            m_LastModelClassAttribute=wCellModelClassAttribute;
            return(true);
        }
        delete wCellModelClassAttribute;
        // Class already registered — keep m_LastModelClassAttribute so AddProperty can append new props.
        m_LastModelClassAttribute=dynamic_cast<tCellModelClassAttribute*>(tClassFactory::Instance()->Get(sClassName));
        return(false);
    }

    tBool tApi::AddProperty(tString sName, tString sType, tString sLabel, tSize sOrder, tString sDefaultValue, tString sKind) {
        if (m_LastModelClassAttribute==nullptr) return(false);
                
        tVariantType wVariantType=Str2VariantType(sType);
        if (wVariantType==tVariantType::t_null) return(false);
        tVariant wDefaultValue;
        wDefaultValue.Parse(sDefaultValue);

        tSetterGetter wSetterGetter;

        return(m_LastModelClassAttribute->AddProperty(sName, wVariantType, sLabel, sOrder, wDefaultValue,"", wSetterGetter, sKind));
    }

    // Undo ===================================================================
	tBool tApi::UndoRaz(tString sRef, tBool sKeepFormat, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoRaz* wUndoRaz = new tUndoRaz(sRef, m_Mode, sSheet);
		wUndoRaz->KeepFormat(sKeepFormat);
		return(Do(wUndoRaz));
	}

	tBool tApi::UndoRazFormat(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoRaz* wUndoRaz = new tUndoRaz(sRef, m_Mode, sSheet);
		// Format-only clear: remove CSS, keep content and class attributes.
		wUndoRaz->FormatOnly(true);
		return(Do(wUndoRaz));
	}

    tBool tApi::UndoCellFormat(tString sRef, tString sFormat, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoFormat* wUndoFormat = new tUndoFormat(sRef, sFormat, m_Mode, sSheet);
        return(Do(wUndoFormat));
    }

    tBool tApi::UndoCellFormat(tRect sRect, tString sFormat, tSheet* sSheet) {
        return UndoCellFormat(sRect.StrRef(), sFormat, sSheet);
    }

    tBool tApi::UndoCellPrecision(tString sRef,tBool sInc,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoPrecision* wUndoPrecision = new tUndoPrecision(sRef, sInc, m_Mode, sSheet);
        return(Do(wUndoPrecision));
        return(false);
    }


    tBool tApi::UndoCellBorder(tString sRef,tShort sBorder, tString sFormat, tSheet* sSheet) {        NormalizeSheet(&sSheet);
        tUndoBorder* wUndoBorder = new tUndoBorder(sRef,sBorder, sFormat, m_Mode, sSheet);
        return(Do(wUndoBorder));
    }


	tBool tApi::UndoCellValue(tString sRef, tVariant sValue, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoCellValue* wUndoCell = new tUndoCellValue(sRef, sValue, m_Mode, sSheet);
		return(Do(wUndoCell));
	}

	tBool tApi::UndoCellValue(tIndex sRow,tIndex sCol, tVariant sValue, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tStringStream wStream;
		wStream << Base10ToAlpha(sCol) << sRow;
		return(UndoCellValue(wStream.str(),sValue,sSheet));
	}

	// Fill series (Excel-like fill handle) ===================================
	tBool tApi::UndoFillSeries(tString sSourceRef, tString sDestRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);

		// Parse the source (seed) and destination (extension) rectangles.
		auto wRectBounds = [](const tString& sRef, tIndex& oR0, tIndex& oC0,
		                      tIndex& oR1, tIndex& oC1) -> tBool {
			tSelect wSelect;
			if (!wSelect.Parse(sRef)) {
				return(false);
			}
			if (tTempoRect* wRect = wSelect.FirstRect()) {
				oR0 = wRect->Row();
				oC0 = wRect->Col();
				oR1 = wRect->Bottom();
				oC1 = wRect->Right();
				return(true);
			}
			if (tTempoPoint* wPoint = wSelect.FirstPoint()) {
				oR0 = oR1 = wPoint->Row();
				oC0 = oC1 = wPoint->Col();
				return(true);
			}
			return(false);
		};

		tIndex wSr0, wSc0, wSr1, wSc1;
		tIndex wDr0, wDc0, wDr1, wDc1;
		if (!wRectBounds(sSourceRef, wSr0, wSc0, wSr1, wSc1) ||
		    !wRectBounds(sDestRef, wDr0, wDc0, wDr1, wDc1)) {
			return(false);
		}

		// Infer orientation / direction from geometry (the drag is single-axis).
		tBool wVertical;
		tBool wForward; // down (vertical) or right (horizontal)
		if (wDc0 == wSc0 && wDc1 == wSc1 && (wDr0 > wSr1 || wDr1 < wSr0)) {
			wVertical = true;
			wForward = (wDr0 > wSr1);
		} else if (wDr0 == wSr0 && wDr1 == wSr1 && (wDc0 > wSc1 || wDc1 < wSc0)) {
			wVertical = false;
			wForward = (wDc0 > wSc1);
		} else {
			return(false);
		}

		// Seed / target coordinates along the fill axis, in forward (fill) order.
		std::vector<tIndex> wSeedAxis;
		std::vector<tIndex> wTargetAxis;
		if (wVertical) {
			if (wForward) {
				for (tIndex r = wSr0; r <= wSr1; r++) wSeedAxis.push_back(r);
				for (tIndex r = wDr0; r <= wDr1; r++) wTargetAxis.push_back(r);
			} else {
				for (tIndex r = wSr1; r >= wSr0; r--) wSeedAxis.push_back(r);
				for (tIndex r = wDr1; r >= wDr0; r--) wTargetAxis.push_back(r);
			}
		} else {
			if (wForward) {
				for (tIndex c = wSc0; c <= wSc1; c++) wSeedAxis.push_back(c);
				for (tIndex c = wDc0; c <= wDc1; c++) wTargetAxis.push_back(c);
			} else {
				for (tIndex c = wSc1; c >= wSc0; c--) wSeedAxis.push_back(c);
				for (tIndex c = wDc1; c >= wDc0; c--) wTargetAxis.push_back(c);
			}
		}
		if (wSeedAxis.empty() || wTargetAxis.empty()) {
			return(false);
		}

		// Destination rectangle geometry for the row-major value buffer.
		tIndex wDestWidth = wDc1 - wDc0 + 1;
		tIndex wDestHeight = wDr1 - wDr0 + 1;
		tVectorString wValues(static_cast<tSize>(wDestWidth) * static_cast<tSize>(wDestHeight));

		// Fixed axis range (one fill line per column [vertical] or row [horizontal]).
		tIndex wLineFrom = wVertical ? wSc0 : wSr0;
		tIndex wLineTo = wVertical ? wSc1 : wSr1;

		auto wCellRef = [&](tIndex sAxisPos, tIndex sLine) -> tString {
			tStringStream wStream;
			if (wVertical) {
				wStream << Base10ToAlpha(sLine) << sAxisPos; // line = col, axis = row
			} else {
				wStream << Base10ToAlpha(sAxisPos) << sLine; // line = row, axis = col
			}
			return(wStream.str());
		};

		auto wFlatIndex = [&](tIndex sRow, tIndex sCol) -> tSize {
			return(static_cast<tSize>(sRow - wDr0) * static_cast<tSize>(wDestWidth) +
			       static_cast<tSize>(sCol - wDc0));
		};

		for (tIndex wLine = wLineFrom; wLine <= wLineTo; wLine++) {
			// Read seeds for this line, in fill order. A formula cell yields its
			// formula string ("=..."); other cells yield their displayed input
			// string (values, dates, text) so the series engine can extend them.
			tVectorString wSeeds;
			tBool wHasFormula = false;
			for (tIndex wPos : wSeedAxis) {
				tIndex wRow = wVertical ? wPos : wLine;
				tIndex wCol = wVertical ? wLine : wPos;
				tCell* wCell = Cell(wRow, wCol, sSheet);
				if (wCell != nullptr && wCell->Formula() != nullptr) {
					wHasFormula = true;
					wSeeds.push_back("=" + wCell->FormulaStr());
				} else {
					wSeeds.push_back(CellInputString(wCellRef(wPos, wLine), sSheet));
				}
			}

			if (wHasFormula) {
				// Continue formulas: shift the seed formula's relative references.
				for (tSize p = 0; p < wTargetAxis.size(); p++) {
					tSize wSrc = p % wSeeds.size();
					tIndex wDelta = wTargetAxis[p] - wSeedAxis[wSrc];
					tString wShifted = tFillSeries::ShiftFormula(
						wSeeds[wSrc],
						wVertical ? static_cast<tInt>(wDelta) : 0,
						wVertical ? 0 : static_cast<tInt>(wDelta));
					tIndex wRow = wVertical ? wTargetAxis[p] : wLine;
					tIndex wCol = wVertical ? wLine : wTargetAxis[p];
					wValues[wFlatIndex(wRow, wCol)] = wShifted;
				}
			} else {
				tVectorString wGenerated =
					tFillSeries::Extend(wSeeds, wTargetAxis.size());
				for (tSize p = 0; p < wTargetAxis.size() && p < wGenerated.size(); p++) {
					tIndex wRow = wVertical ? wTargetAxis[p] : wLine;
					tIndex wCol = wVertical ? wLine : wTargetAxis[p];
					wValues[wFlatIndex(wRow, wCol)] = wGenerated[p];
				}
			}
		}

		// Normalized destination range (rectangle) for the single undo.
		tStringStream wDestStream;
		wDestStream << Base10ToAlpha(wDc0) << wDr0 << ":"
		            << Base10ToAlpha(wDc1) << wDr1;

		tUndoFillSeries* wUndo =
			new tUndoFillSeries(wDestStream.str(), wValues, m_Mode, sSheet);
		return(Do(wUndo));
	}

    // Class ==================================================================
    tBool tApi::UndoCellClass(tString sRef, tVariant* sValue, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoCellClass* wUndoCellClass = new tUndoCellClass(sRef, sValue, m_Mode, sSheet);
        return(Do(wUndoCellClass));
    }

    tBool tApi::UndoCellClass(tIndex sRow, tIndex sCol, tVariant* sValue, tSheet* sSheet) {
        tStringStream wStream;
        wStream << Base10ToAlpha(sCol) << sRow;
        return(UndoCellClass(wStream.str(), sValue, sSheet));
    }

	tBool tApi::UndoCellClass(tString sRef, tString sClassName, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
        // Search LastClassName;
        
		tUndoCellClass* wUndoCellClass = new tUndoCellClass(sRef, sClassName, m_Mode, sSheet);
		return(Do(wUndoCellClass));
	}

	tBool tApi::UndoCellClass(tIndex sRow, tIndex sCol, tString sClassName, tSheet* sSheet) {
		tStringStream wStream;
		wStream << Base10ToAlpha(sCol) << sRow;
		return(UndoCellClass(wStream.str(), sClassName, sSheet));
	}

	tBool tApi::UndoCellAttribute(tString sRef, tString sAttribute, tVariant sVariant, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoCellAttribute* wUndoCellAttribute=new tUndoCellAttribute(sRef, sAttribute,sVariant, m_Mode, sSheet);
		return(Do(wUndoCellAttribute));
	}

	tBool tApi::UndoCellAttribute(tIndex sRow, tIndex sCol, tString sAttribute, tVariant sVariant, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tStringStream wStream;
		wStream << Base10ToAlpha(sCol) << sRow;
		return(UndoCellAttribute(wStream.str(), sAttribute,sVariant, sSheet));
	}

    tBool tApi::UndoCellClassCalculable(tString sRef, tVariant sVariant, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoCellClassCalculable* wUndo =
            new tUndoCellClassCalculable(sRef, sVariant, m_Mode, sSheet);
        return(Do(wUndo));
    }

    tBool tApi::UndoApplyUnit(tString sRef,tString sFamily,tString sUnit) {
        tVariant wValue; // Empty for replace
        tCellClass wCellClass(wValue);
        tClassUnit* wClassUnit=nullptr;

        t_UnitFamily wFamily=UnitFamily(sFamily);
                                    
        switch(wFamily) {
            case t_UnitFamily::None : break;
            case t_UnitFamily::Monetary : {
                t_UnitMoney wUnitMoney=UnitMoney(sUnit);
                wClassUnit=new  tClassUnit(wUnitMoney);
                break;
            }
            case t_UnitFamily::Length : {
                t_UnitLength wUnitLength=UnitLength(sUnit);
                wClassUnit = new tClassUnit(wUnitLength);
                break;
            }
            case t_UnitFamily::Mass : {
                t_UnitMass wUnitMass=UnitMass(sUnit);
                wClassUnit = new tClassUnit(wUnitMass);
                break;
            }
            case t_UnitFamily::Time : {
                t_UnitTime wUnitTime=UnitTime(sUnit);
                wClassUnit = new tClassUnit(wUnitTime);
                break;
            }
            case t_UnitFamily::AmountOfSubstance : break; // - mole (mol)
            case t_UnitFamily::ElectricCurrent : break; // - ampere (A)
            case t_UnitFamily::TemperatureKelvin : break; // (K)
            case t_UnitFamily::LuminousIntensityCandela : break; // (cd)
        }
        if (wClassUnit!=nullptr) {
            tCellClassUnit wCellClassUnit(wValue,*wClassUnit);
            delete(wClassUnit);
            tVariant wUnitValue(&wCellClassUnit);
            return(UndoCellClass(sRef, &wUnitValue));
        }
        
        return(false);
    }

    tBool tApi::UndoApplyMerge(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoApplyMerge* wUndoApplyMerge=new tUndoApplyMerge(sRef, m_Mode, sSheet);
        return(Do(wUndoApplyMerge));
    }

	tBool tApi::UndoInsertRow(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoInsertRow* wUndoInsertRow = new tUndoInsertRow(sRow, sSize, m_Mode, sSheet);
		return(Do(wUndoInsertRow));
	}

    tBool tApi::UndoInsertRowByRect(tRect sRect, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoInsertRow* wUndoInsertRow = new tUndoInsertRow(sRect, m_Mode, sSheet);
        return(Do(wUndoInsertRow));
    }

    tBool tApi::UndoInsertRowByRectWithLabel(tRect sRect, tString sLabelRef, tString sLabelValue,
                                             tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoInsertRowWithLabel* wUndo =
            new tUndoInsertRowWithLabel(sRect, sLabelRef, sLabelValue, m_Mode, sSheet);
        return(Do(wUndo));
    }


	tBool tApi::UndoInsertCol(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoInsertCol* wUndoInsertCol = new tUndoInsertCol(sRow, sSize, m_Mode, sSheet);
		return(Do(wUndoInsertCol));
	}

    tBool tApi::UndoInsertColByRect(tRect sRect, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoInsertCol* wUndoInsertCol = new tUndoInsertCol(sRect, m_Mode, sSheet);
        return(Do(wUndoInsertCol));
    }

	tBool tApi::UndoDeleteRow(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoDeleteRow* wUndoDeleteRow = new tUndoDeleteRow(sRow, sSize, m_Mode, sSheet);
		return(Do(wUndoDeleteRow));
	}

    tBool tApi::UndoDeleteRowByRect(tRect sRect, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoDeleteRow* wUndoDeleteRow = new tUndoDeleteRow(sRect, m_Mode, sSheet);
        return(Do(wUndoDeleteRow));
    }

	tBool tApi::UndoDeleteCol(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoDeleteCol* wUndoDeleteCol = new tUndoDeleteCol(sRow, sSize, m_Mode, sSheet);
		return(Do(wUndoDeleteCol));
	}

    tBool tApi::UndoDeleteColByRect(tRect sRect, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoDeleteCol* wUndoDeleteCol = new tUndoDeleteCol(sRect, m_Mode, sSheet);
        return(Do(wUndoDeleteCol));
    }

    tBool tApi::UndoOpenCloseTreeRow(tIndex sRow,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoOpenCloseTree* wUndoOpenClose=new tUndoOpenCloseTree(true, sRow, m_Mode, sSheet);
        return(Do(wUndoOpenClose));
    }

    tBool tApi::UndoOpenCloseTreeCol(tIndex sRow,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoOpenCloseTree* wUndoOpenClose=new tUndoOpenCloseTree(false, sRow, m_Mode, sSheet);
        return(Do(wUndoOpenClose));
    }

    tBool tApi::UndoChangeTreeRow(tBool sRight,tIndex sRow,tIndex sSize,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoChangeTree* wUndoChangeTree=new tUndoChangeTree(true, sRight, sRow, sSize, m_Mode, sSheet);
        return(Do(wUndoChangeTree));
    }

    tBool tApi::UndoChangeTreeCol(tBool sRight,tIndex sCol,tIndex sSize,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoChangeTree* wUndoChangeTree=new tUndoChangeTree(false, sRight, sCol, sSize, m_Mode, sSheet);
        return(Do(wUndoChangeTree));
    }

    tBool tApi::UndoSplitView(tByte sCde, tIndex sPosition, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoSplitView* wUndo = new tUndoSplitView(sCde, sPosition, m_Mode, sSheet);
        return(Do(wUndo));
    }

    tBool tApi::UndoAddSheet(tString sName,tString sSheetLeft) {
        tWorkBook* wWorkBook=m_SpreadSheetContainer->ActiveWorkBook();
        tSheet* wSheet=wWorkBook->Sheet(sName);
        if (wSheet!=nullptr) {
            return(false);
        }
        
        tUndoAddSheet* wUndoAddSheet=new tUndoAddSheet(sName, sSheetLeft, m_Mode);
        return(Do(wUndoAddSheet));
    }

    tBool tApi::UndoRenameSheet(tString sName,tString sNewName) {
        // A sheet rename rewrites every qualified reference in existing
        // formulas. We cannot rebase that on peers, so we forbid the action
        // as soon as another user is connected. Pure no-op renames
        // (sName==sNewName) are still filtered out to stay symmetrical with
        // UndoUpdateRangeNamed.
        if (sName != sNewName && !IsRenameAllowed()) {
            return(false);
        }
        tUndoRenameSheet* wUndoRenameSheet=new tUndoRenameSheet(sName,sNewName,m_Mode);
        return(Do(wUndoRenameSheet));
    }

    tBool tApi::UndoSwapSheet(tString sName1,tString sName2,tBool sInsertAfter) {
        tUndoSwapSheet* wUndoSwapSheet=new tUndoSwapSheet(sName1,sName2,m_Mode,sInsertAfter);
        return(Do(wUndoSwapSheet));
    }

    tBool tApi::UndoDeleteSheet(tString sName) {
        tWorkBook* wWorkBook=m_SpreadSheetContainer->ActiveWorkBook();
        tSheet* wSheet=wWorkBook->Sheet(sName);
        if (wSheet==nullptr) return(false);
        
		tUndoDeleteSheet* wUndoDeleteSheet = new tUndoDeleteSheet(wSheet->Name(),m_Mode);
		return(Do(wUndoDeleteSheet));
	}

	tBool tApi::UndoAddRangeNamed(tString sName, tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoAddRangeNamed* wUndoAddRangeNamed =
			new tUndoAddRangeNamed(sName, sRef, m_Mode, sSheet);
		return Do(wUndoAddRangeNamed);
	}

	tBool tApi::UndoInsertRangeNamed(tString sName, tString sRef, tSheet* sSheet) {
		return UndoAddRangeNamed(sName, sRef, sSheet);
	}

	tBool tApi::UndoAddRangeData(tString sName, tString sRef, tString sJsonData,
	                             tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoAddRangeData* wUndoAddRangeData =
			new tUndoAddRangeData(sName, sJsonData, sRef, m_Mode, sSheet);
		return Do(wUndoAddRangeData);
	}

	tBool tApi::UndoInsertRangeData(tString sName, tString sRef, tString sJsonData,
	                                tSheet* sSheet) {
		return UndoAddRangeData(sName, sRef, sJsonData, sSheet);
	}

	tBool tApi::UndoApplyRangeData(tString sName, tString sJsonData, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoApplyRangeData* wUndoApplyRangeData =
			new tUndoApplyRangeData(sName, sJsonData, m_Mode, sSheet);
		return Do(wUndoApplyRangeData);
	}

	tBool tApi::UndoDeleteRangeNamed(tString sName, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoDeleteRangeNamed* wUndoDeleteRangeNamed =
			new tUndoDeleteRangeNamed(sName, m_Mode, sSheet);
		return Do(wUndoDeleteRangeNamed);
	}

	tBool tApi::UndoUpdateRangeNamed(tString sOldName, tString sNewName,
	                                 tString sNewRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		// Refuse the *rename* part only. Pure reselects (same name, new
		// selection) remain legal in multi-user mode because the string that
		// identifies the range on every peer is unchanged; only the coords
		// move, and those are already rebased through tRebasePlan.
		if (sOldName != sNewName && !IsRenameAllowed()) {
			return(false);
		}
		tUndoUpdateRangeNamed* wUndoUpdateRangeNamed =
			new tUndoUpdateRangeNamed(sOldName, sNewName, sNewRef, m_Mode, sSheet);
		return(Do(wUndoUpdateRangeNamed));
	}

    tBool tApi::UndoInsertFormulaNamed(tString sName,tString sFormula,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoInsertFormulaNamed* wUndoInsertFormulaNamed = new tUndoInsertFormulaNamed(sName,sFormula,m_Mode,sSheet);
        return(Do(wUndoInsertFormulaNamed));
    }

    tBool tApi::UndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tSheet* sSheet,
                                         tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight,
                                         tDouble sOpacity, tString sAnchorCellRef) {
        NormalizeSheet(&sSheet);
        const tBool wUseInitialLayout = (sWidth > 0.0 || sHeight > 0.0 || !sAnchorCellRef.empty());
        if (!wUseInitialLayout) {
            tUndoInsertFloatingObject* wUndo = new tUndoInsertFloatingObject(sName, sClassName, sTargetSheetName, 0, m_Mode, sSheet);
            return (Do(wUndo));
        }
        tFloatingObjectLayout wInitialLayout;
        wInitialLayout.EnsureDefaults();
        wInitialLayout.DiffX(sDiffX);
        wInitialLayout.DiffY(sDiffY);
        if (sWidth > 0.0) {
            wInitialLayout.Width(sWidth);
        }
        if (sHeight > 0.0) {
            wInitialLayout.Height(sHeight);
        }
        if (sOpacity > 0.0) {
            wInitialLayout.Opacity(sOpacity);
        }
        tUndoInsertFloatingObject* wUndo = new tUndoInsertFloatingObject(
            sName, sClassName, sTargetSheetName, 0, true, sAnchorCellRef, wInitialLayout, m_Mode, sSheet);
        return (Do(wUndo));
    }

    tBool tApi::UndoDeleteFloatingObject(tString sName, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoDeleteFloatingObject* wUndo = new tUndoDeleteFloatingObject(sName, m_Mode, sSheet);
        return (Do(wUndo));
    }

    tBool tApi::UndoFloatingObjectLayout(tString sName, tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight, tDouble sOpacity, tString sAnchorCellRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tWorkBook* const wWorkBook = sSheet->WorkBook();
        tFloatingObject* const wObject = wWorkBook->FloatingObjectContainer()->ByName(sName);
        if (wObject == nullptr) {
            return false;
        }
        tFloatingObjectLayout wNewLayout(wObject->Layout());
        wNewLayout.DiffX(sDiffX);
        wNewLayout.DiffY(sDiffY);
        wNewLayout.Width(sWidth);
        wNewLayout.Height(sHeight);
        wNewLayout.Opacity(sOpacity);
        tBool wChangeAnchor = false;
        if (!sAnchorCellRef.empty()) {
            tCell* const wHost = wObject->HostCell();
            tCell* const wAnchor = FloatingObjectAnchorCellFromRef(sAnchorCellRef, wHost);
            if (wAnchor == nullptr) {
                return false;
            }
            wNewLayout.AnchorCell(wAnchor);
            wChangeAnchor = true;
        }
        tUndoFloatingObjectLayout* wUndo = new tUndoFloatingObjectLayout(sName, wNewLayout, wChangeAnchor, m_Mode, sSheet);
        return (Do(wUndo));
    }

    tBool tApi::UndoFloatingObjectBringToFront(tString sName, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tWorkBook* const wWorkBook = sSheet->WorkBook();
        tFloatingObject* const wObject = wWorkBook->FloatingObjectContainer()->ByName(sName);
        if (wObject == nullptr) {
            return false;
        }
        const tInt wCurrent = wObject->Layout().ZIndex();
        const tInt wMax = wWorkBook->FloatingObjectContainer()->MaxZIndexOnTargetSheet(wObject->TargetSheetName());
        if (wCurrent > 0 && wCurrent >= wMax) {
            return true;
        }
        tFloatingObjectLayout wNewLayout(wObject->Layout());
        wNewLayout.ZIndex(wMax + 1);
        tUndoFloatingObjectLayout* wUndo = new tUndoFloatingObjectLayout(sName, wNewLayout, false, m_Mode, sSheet);
        return (Do(wUndo));
    }

    tBool tApi::UndoFloatingObjectAttribute(tString sName, tString sAttribute, tVariant sVariant) {
        tWorkBook* const wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return false;
        }
        tFloatingObject* const wObject = wWorkBook->FloatingObjectContainer()->ByName(sName);
        if (wObject == nullptr) {
            return false;
        }
        tCell* const wHost = wObject->HostCell();
        if (wHost == nullptr) {
            return false;
        }
        tSheet* wHostSheet = wHost->Sheet();
        if (wHostSheet == nullptr) {
            wHostSheet = wWorkBook->SheetClassAnchor();
        }
        if (wHostSheet == nullptr) {
            return false;
        }
        return UndoCellAttribute(wHost->RowIndex(), wHost->ColIndex(), sAttribute, sVariant, wHostSheet);
    }

    tBool tApi::UndoCellClassAttributes(tString sRef, const tVectorCellAttributeWire& sAttributes, tSheet* sSheet) {
        if (sAttributes.empty()) {
            return false;
        }
        NormalizeSheet(&sSheet);
        tUndoCellClassAttributes* wUndo =
            new tUndoCellClassAttributes(sRef, sAttributes, m_Mode, sSheet);
        return(Do(wUndo));
    }

    tBool tApi::UndoFloatingObjectAttributes(tString sName, const tVectorCellAttributeWire& sAttributes) {
        if (sAttributes.empty()) {
            return false;
        }
        tWorkBook* const wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return false;
        }
        tFloatingObject* const wObject = wWorkBook->FloatingObjectContainer()->ByName(sName);
        if (wObject == nullptr) {
            return false;
        }
        tCell* const wHost = wObject->HostCell();
        if (wHost == nullptr) {
            return false;
        }
        tSheet* wHostSheet = wHost->Sheet();
        if (wHostSheet == nullptr) {
            wHostSheet = wWorkBook->SheetClassAnchor();
        }
        if (wHostSheet == nullptr) {
            return false;
        }
        tStringStream wStream;
        wStream << Base10ToAlpha(wHost->ColIndex()) << wHost->RowIndex();
        return UndoCellClassAttributes(wStream.str(), sAttributes, wHostSheet);
    }

    tBool tApi::UndoDeleteFormulaNamed(tString sName, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoDeleteFormulaNamed* wUndoDeleteFormulaNamed = new tUndoDeleteFormulaNamed(sName,m_Mode,sSheet);
        return(Do(wUndoDeleteFormulaNamed));
    }

    tBool tApi::UndoJsonPayload(tString sRef, tString sJsonPayload, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoJsonPayload* wUndoJsonPayload = new tUndoJsonPayload(sRef, sJsonPayload, m_Mode, sSheet);
        return(Do(wUndoJsonPayload));
    }


	
	tBool tApi::Copy(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tCopy* wCopy = new tCopy();
		tBool wOk=wCopy->Treat(sRef,sSheet);
		delete(wCopy);
		return(wOk);
	}

	tBool tApi::UndoPaste(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoSpreadSheet* wUndo = CreateUndoPasteOrMoveAfterCut(
		    sRef, m_Mode, sSheet, m_Application->UndoRedoContainer());
		if (wUndo == nullptr) {
		    wUndo = new tUndoPaste(sRef, m_Mode, sSheet);
		}
		return(Do(wUndo));
	}

	tBool tApi::UndoCut(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tUndoCut* wUndoCut = new tUndoCut(sRef, m_Mode, sSheet);
		return(Do(wUndoCut));
	}

    tBool tApi::UndoMove(tString sSourceRef, tString sDestRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoMove* wUndoMove = new tUndoMove(sSourceRef, sDestRef, m_Mode, sSheet);
        return(Do(wUndoMove));
    }

    tBool tApi::UndoConditionalFormat(tString sType,tString sRef,tString sParam1,tString sParam2,tString sParam3,tString sParam4,tString sParam5,tString sParam6,tString sParam7,tString sParam8,tString sParam9,tString sParam10, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        
        
        tUndoConditionaFormat* wUndoConditionalFormat=new tUndoConditionaFormat (sRef, sType, sParam1, sParam2, sParam3, sParam4, sParam5, sParam6, sParam7, sParam8, sParam9, sParam10, m_Mode, sSheet);
        return(Do(wUndoConditionalFormat));
    }

    tBool tApi::UndoDeleteConditionalFormat(tString sType,tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tUndoDeleteConditionaFormat* wUndoDeleteConditionalFormat = new tUndoDeleteConditionaFormat(sRef,sType, m_Mode, sSheet);
        return(Do(wUndoDeleteConditionalFormat));
    }


	tCell* tApi::EnsureCell(tIndex sRow, tIndex sCol, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->EnsureCell(sRow, sCol));
	}

	tCell* tApi::EnsureCell(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRow; tIndex wCol;
		if (ParseCell(sRef, wRow, wCol)) {
			return(sSheet->EnsureCell(wRow, wCol));
		}
		return(nullptr);
	}


    void tApi::FormatApi(tFormatApi*  sFormatApi) {
        m_SpreadSheetContainer->ActiveWorkBook()->FormatApi(sFormatApi);
    }

    tFormatApi* tApi::FormatApi() { return(m_SpreadSheetContainer->ActiveWorkBook()->FormatApi()); }

    tString tApi::JsonFormatString() { return(tApplication::Instance()->FormatStringRoot()->JsonFormatString()); }

    tString tApi::CellFormatString(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tSelect wSelect;
        tBool wResult=wSelect.Parse(sRef);
        if (wResult) {
            tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
            for (auto wItem : *wVectorSelect) {
                if (wItem!=nullptr) {
                    tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                    if (wRect != nullptr) {
                        return("Error Select CellFormatString");
                    } else {
                        tCell* wCell=Cell(wItem->Row(), wItem->Col());
                        return(sSheet->WorkBook()->CellFormatString(wCell));
                    }
                }
            }
        }
        return("Error CellFormatString "+sRef);
    }

    tString tApi::CellInputString(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tSelect wSelect;
        tBool wResult=wSelect.Parse(sRef);
        if (wResult) {
            tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
            for (auto wItem : *wVectorSelect) {
                if (wItem!=nullptr) {
                    tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                    if (wRect != nullptr) {
                        return("Error Select CellInputString");
                    } else {
                        tCell* wCell=Cell(wItem->Row(), wItem->Col());
                        return(sSheet->WorkBook()->CellInputString(wCell));
                    }
                }
            }
        }
        return("Error CellInputString "+sRef);
    }


tString tApi::CellJsonPayload(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tSelect wSelect;
        tBool wResult=wSelect.Parse(sRef);
        if (wResult) {
            tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
            for (auto wItem : *wVectorSelect) {
                if (wItem!=nullptr) {
                    tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                    if (wRect != nullptr) {
                        return("Error Select CellIJsonPayload");
                    } else {
                        tString wJsonPayload;
                        if (sSheet->ColRowCellRange()->GetCellJsonPayload(wItem->Row(),wItem->Col(),wJsonPayload)) {
                            return(wJsonPayload);
                        }
                    }
                }
            }
        }
        return("");
   }


    // Interface Check ===================================================
    #ifdef checksp
    /// @brief Check.
    void tApi::Check() {
        try {
            m_SpreadSheetContainer->Check();
        }
        catch (const tExceptionInternalError e) {
            cout << e.what() << endl;
        }
#ifdef checkfo
        CheckFormat();
#endif
    }
#endif
#ifdef checkfo
    void tApi::CheckFormat() {
        if (m_SpreadSheetContainer!=nullptr)
            m_SpreadSheetContainer->CheckFormat();
    }
#endif
    // Interface WorkBook =================================================

    tWorkBook* tApi::ActiveWorkBook() { return(m_SpreadSheetContainer->ActiveWorkBook()); }

    tBool tApi::ActiveWorkBook(tString sUri) {
        return(m_SpreadSheetContainer->ActiveWorkBook(sUri));
    }

    tWorkBook* tApi::AddWorkBook(tString sWorkBookUri) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->FindWorkBook(sWorkBookUri);
        if (wWorkBook == nullptr) {
            wWorkBook = wWorkBook = m_SpreadSheetContainer->AddWorkBook(sWorkBookUri);
        }
        m_SpreadSheetContainer->ActiveWorkBook(wWorkBook);
        return(wWorkBook);
    }

    tBool tApi::WorkBookInfo(tString sWorkBookInfo) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        wWorkBook->WorkBookInfo(sWorkBookInfo);
        return(true);
    }

    tString tApi::WorkBookInfo() {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        return(wWorkBook->WorkBookInfo());
    }

    tString tApi::JsonWorkBook(tString sUri) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->FindWorkBook(sUri);
        if (wWorkBook == nullptr) {
            return("");
        }
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        
        wWriter.StartObject();
        wWriter.Key("uri");
        wWriter.String(wWorkBook->Uri().c_str());
        wWriter.Key("info");
        wWriter.StartObject();
        wWorkBook->JsonInfo(&wWriter);
        wWriter.EndObject();
        wWriter.EndObject();
        
        return(wStringBuffer.GetString());
    }
    tString tApi::JsonWorkBooks() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        tVectorWorkBookClass wVectorWorkBook;
        
        wWriter.StartObject();
        wWriter.Key("list");
        wWriter.StartArray();
        m_SpreadSheetContainer->WorkBooksList(wVectorWorkBook);
        for(auto wWorkBook : wVectorWorkBook) {
            wWriter.StartObject();
            wWriter.Key("uri");
            wWriter.String(wWorkBook->Uri().c_str());
            wWriter.Key("info");
            wWriter.StartObject();
            wWorkBook->JsonInfo(&wWriter);
            wWriter.EndObject();
            wWriter.EndObject();
        }
        wWriter.EndArray();
        wWriter.EndObject();
        
        return(wStringBuffer.GetString());
    }

    tString tApi::JsonWorkBooksList() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        m_SpreadSheetContainer->JsonWorkBooksList(&wWriter);
        return(wStringBuffer.GetString());
    }


    tWorkBook* tApi::NewWorkBook(tString sWorkBookUri) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->AddWorkBook(sWorkBookUri);
        // Insert Sheet1 by Default
        wWorkBook->AddSheet("Sheet1");
        wWorkBook->ActiveSheet("Sheet1");
        return(wWorkBook);
    }

    tBool  tApi::DeleteWorkBook(tString sWorkBookUri) {
        return(m_SpreadSheetContainer->DeleteWorkBook(sWorkBookUri));
    }

    tBool tApi::RenameWorkBook(tString sUri,tString sToUri) {
        return(m_SpreadSheetContainer->RenameWorkBook(sUri, sToUri));
    }

    tWorkBook* tApi::WorkBook(tString sWorkBookUri) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->FindWorkBook(sWorkBookUri);
        if (wWorkBook == nullptr) {
            wWorkBook = NewWorkBook(sWorkBookUri);
        }
        m_SpreadSheetContainer->ActiveWorkBook(wWorkBook);
        return(wWorkBook);
    }

    tString tApi::WriteJson(tString sWorkBookUri) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->FindWorkBook(sWorkBookUri);
        if (wWorkBook != nullptr) {
            // Do not switch locale here: formula strings (e.g. array literals {1,1} vs {1;1}) depend on
            // tLocale::Arg() / lexer separators; forcing "us" breaks round-trip vs FR-style arrays.
            StringBuffer wStringBuffer;
            Writer<StringBuffer> wWriter(wStringBuffer);
            tApplication* wApplication=tApplication::Instance();
            tString wPushLang=wApplication->Locale()->Lang();
            wApplication->Locale("us");
            wWorkBook->Json(&wWriter);
            wApplication->Locale(wPushLang);
#ifdef debugjson
            cout << "Write ->" << sWorkBookUri << "->" << wStringBuffer.GetString() << endl;
#endif
            return(wStringBuffer.GetString());
        }
        return("");
    }

    /// @brief      Read Json.
    /// @param[in]  sJon tString
    /// @return        tBool
    tBool tApi::ReadJson(tString sJson) {
        tApplication* wApplication=tApplication::Instance();
        tString wPushLang=wApplication->Locale()->Lang();
        wApplication->Locale("us");
        
        rapidjson::Document wDocument;
        
        rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());

        if (!wParseResult) {
            // Calculate context window
            const tInt wDiff=230;
            size_t errorOffset = wParseResult.Offset();
            size_t startPos = (errorOffset > wDiff) ? errorOffset - wDiff : 0;
            size_t endPos = std::min(errorOffset + wDiff, sJson.length());
            
            std::cerr << "Parsing error: "
                      << rapidjson::GetParseError_En(wParseResult.Code())
                      << " at offset " << errorOffset
                      << "\nContext: ..." << sJson.substr(startPos, endPos - startPos) << "..."
                      << std::endl;
            return(false);
        }

        tString wUri = wDocument["uri"].GetString();
        tWorkBook* wWorkBook = m_SpreadSheetContainer->FindWorkBook(wUri);
        if (wWorkBook == nullptr) {
            wWorkBook = m_SpreadSheetContainer->AddWorkBook(wUri);
        } else {
            // Reload: Clear() releases this workbook's css refs only (see tColRowCellRange::ReleaseAllCellFormats).
            wWorkBook->Clear();
        }
        // Ensure JsonEnd/JsonCompil and tFormulaNamed construction use this workbook (not a stale active ref).
        m_SpreadSheetContainer->ActiveWorkBook(wWorkBook);
        // Keep current application locale during load so CompilCell uses the same list/array separators as
        // when formulas were authored (WriteJson no longer forces "us" for the same reason).

        // Loadd Json with calcul
        tSpreadSheetContainer::Instance()->JsonBegin();
        tSpreadSheetContainer::Instance()->SetJsonRestoreCachedFormulaValues(true);
        wWorkBook->Json(wDocument);
        tSpreadSheetContainer::Instance()->JsonEnd();
        m_SpreadSheetContainer->ActiveWorkBook(wUri);
        wApplication->Locale(wPushLang);
        return(true);
    }

    // Interface Sheet ===================================================
    tSheet* tApi::AddSheet(tString sSheetName) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return nullptr;
        }
        tSheet* wSheet = wWorkBook->Sheet(sSheetName);
        if (wSheet != nullptr) { return(nullptr); } // Return nullptr if already exist
        return(wWorkBook->AddSheet(sSheetName));
    }

    tSheet* tApi::Sheet(tString sSheetName) {
        return(m_SpreadSheetContainer->ActiveWorkBook()->Sheet(sSheetName));
    }

    tBool tApi::DeleteSheet(tString sSheetName) {
        tSheet* wSheet = m_SpreadSheetContainer->ActiveWorkBook()->Sheet(sSheetName);
        if (wSheet != nullptr) {
            tSaveSelectErase wSaveSelectErase;
            wSheet->DoDeleteSheet(&wSaveSelectErase);
            return(true);
        }

        return(false);
    }

    tSheet* tApi::ActiveSheet(tString sSheetName) {
        tSheet* wSheet= m_SpreadSheetContainer->ActiveWorkBook()->ActiveSheet(sSheetName);

        return(wSheet);
    }

    tSheet* tApi::ActiveSheet() {
        return(m_SpreadSheetContainer->ActiveWorkBook()->ActiveSheet());
    }

    tVectorString tApi::GetVectorOfSheet() {
        tVectorString wResult;
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        tVectorAllocatorRef* wVectorSheet = wWorkBook->VectorSheet();
        for (auto wIndex : *wVectorSheet) {
            tSheet* wSheet = wWorkBook->SheetByAllocator(wIndex);
            wResult.push_back(wSheet->Name());
        }
        return(wResult);
    }

    void tApi::NormalizeSheet(tSheet** sSheet) {
        if (*sSheet==nullptr) *sSheet= m_SpreadSheetContainer->ActiveWorkBook()->ActiveSheet();
    }

	tCell* tApi::Cell(tIndex sRow, tIndex sCol, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tCell* wCell = sSheet->Cell(sRow, sCol);
		if (wCell != nullptr) {
			return(wCell);
		}
		return(nullptr);
	}

	tCell* tApi::Cell(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRow; tIndex wCol;
		if (ParseCell(sRef, wRow, wCol)) {
			return(sSheet->Cell(wRow, wCol));
		}
		return(nullptr);
	}


	tBool tApi::CompilCell(tCell* sCell, const tChar* sValue) {
		return(m_SpreadSheetContainer->ActiveWorkBook()->CompilCell(sCell, sValue));
	}

	tLemonInterface* tApi::LemonInterface() {
		return(tSpreadSheetContainer::Instance()->LemonInterface());
	}

	tString tApi::Formula(tIndex sRow, tIndex sCol, tSheet* sSheet, tBool sUser) {
		NormalizeSheet(&sSheet);
		tCell* wCell = sSheet->Cell(sRow, sCol);
		if (wCell != nullptr) {
			return(wCell->FormulaStr(false, sUser));
		}
		return("");
	}


	namespace {

	tString EscapeStructuredColumnLabel(const tString& sLabel) {
		tString wOut;
		wOut.reserve(sLabel.size() + 2);
		for (tChar wCh : sLabel) {
			if (wCh == ']') {
				wOut += "]]";
			} else {
				wOut.push_back(wCh);
			}
		}
		return wOut;
	}

	tString StructuredColumnBracket(const tString& sLabel) {
		return "[" + EscapeStructuredColumnLabel(sLabel) + "]";
	}

	tString ColumnLabelAtSheetCol(tRangeData* sRangeData, tRange* sRange, tIndex sSheetCol) {
		if (sRangeData == nullptr || sRange == nullptr) {
			return "";
		}
		const tIndex wLeft = sRange->LeftIndex();
		if (sSheetCol < wLeft) {
			return "";
		}
		tColumnData* wCol = sRangeData->FindColumnByOrdinal(
			wLeft, static_cast<tSize>(sSheetCol - wLeft));
		if (wCol == nullptr) {
			return "";
		}
		return wCol->HeaderLabel(sRange->Sheet(), sRange->TopIndex(), wLeft);
	}

	tBool SelectionInsideTableData(tRange* sTableRange, tRangeData* sRangeData,
								   tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight,
								   tIndex& sDataTop, tIndex& sDataBottom) {
		if (sTableRange == nullptr || sRangeData == nullptr) {
			return false;
		}
		sDataTop = sRangeData->HasHeaders()
			? sTableRange->TopIndex() + 1
			: sTableRange->TopIndex();
		sDataBottom = sTableRange->BottomIndex();
		if (sDataTop > sDataBottom) {
			return false;
		}
		if (sTop < sDataTop || sBottom > sDataBottom) {
			return false;
		}
		if (sLeft < sTableRange->LeftIndex() || sRight > sTableRange->RightIndex()) {
			return false;
		}
		return true;
	}

	tString FormatStructuredTablePrefix(const tString& sTableName) {
		if (sTableName.empty()) {
			return "";
		}
		tBool wNeedsQuotes = false;
		for (tChar wCh : sTableName) {
			if (!(std::isalnum(static_cast<unsigned char>(wCh)) != 0 || wCh == '_')) {
				wNeedsQuotes = true;
				break;
			}
		}
		if (wNeedsQuotes) {
			return "'" + sTableName + "'";
		}
		return sTableName;
	}

	tString BuildStructuredColumnSpec(const tString& sLabelLeft, const tString& sLabelRight,
									  tBool sSingleCol, tBool sThisRow) {
		if (sThisRow) {
			if (sSingleCol) {
				return "@" + StructuredColumnBracket(sLabelLeft);
			}
			return "[@" + StructuredColumnBracket(sLabelLeft) + ":"
				+ StructuredColumnBracket(sLabelRight) + "]";
		}
		if (sSingleCol) {
			return "[" + StructuredColumnBracket(sLabelLeft) + "]";
		}
		return "[" + StructuredColumnBracket(sLabelLeft) + ":"
			+ StructuredColumnBracket(sLabelRight) + "]";
	}

	} // namespace

	tString tApi::CellRef(tString sRefAnchor,tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wRowAnchor = 0;
		tIndex wColAnchor = 0;
		if (!ParseCell(sRefAnchor, wRowAnchor, wColAnchor)) {
			return "";
		}
		tSelect wSelect;
		if (!wSelect.Parse(sRef)) {
			return "";
		}
		tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
		if (wVectorSelect == nullptr || wVectorSelect->empty()) {
			return "";
		}
		// Formula pick uses a single area; ignore extra areas from col/row header selects.

		tIndex wTop = 0;
		tIndex wLeft = 0;
		tIndex wBottom = 0;
		tIndex wRight = 0;
		tString wA1Fallback;
		// tTempoRect inherits tTempoPoint — test rect before point.
		tTempoRect* wRect = dynamic_cast<tTempoRect*>(wVectorSelect->at(0));
		if (wRect != nullptr) {
			wTop = wRect->Top();
			wLeft = wRect->Left();
			wBottom = wRect->Bottom();
			wRight = wRect->Right();
			wA1Fallback = wRect->StrRef();
		} else {
			tTempoPoint* wPoint = dynamic_cast<tTempoPoint*>(wVectorSelect->at(0));
			if (wPoint == nullptr) {
				return "";
			}
			wTop = wBottom = wPoint->Row();
			wLeft = wRight = wPoint->Col();
			wA1Fallback = wPoint->StrRef();
		}

		tWorkBook* wWorkBook = sSheet->WorkBook();
		if (wWorkBook == nullptr) {
			return wA1Fallback;
		}

		// Resolve the table from the picked selection (works when the edit
		// anchor is outside the table — Excel inserts Table[[Col]]).
		tString wPickTableName;
		tRange* wPickTable = nullptr;
		tie(wPickTableName, wPickTable) =
			sSheet->FindRangeDataCovered(static_cast<tInt>(wTop),
										 static_cast<tInt>(wLeft));
		if (wPickTable == nullptr || wPickTableName.empty()) {
			return wA1Fallback;
		}

		tRangeData* wRangeData = wWorkBook->RangeData(wPickTableName);
		if (wRangeData == nullptr) {
			return wA1Fallback;
		}

		tIndex wDataTop = 0;
		tIndex wDataBottom = 0;
		if (!SelectionInsideTableData(wPickTable, wRangeData,
									  wTop, wLeft, wBottom, wRight,
									  wDataTop, wDataBottom)) {
			return wA1Fallback;
		}

		const tString wLabelLeft =
			ColumnLabelAtSheetCol(wRangeData, wPickTable, wLeft);
		const tString wLabelRight =
			ColumnLabelAtSheetCol(wRangeData, wPickTable, wRight);
		if (wLabelLeft.empty() || wLabelRight.empty()) {
			return wA1Fallback;
		}

		tString wAnchorTableName;
		tRange* wAnchorTable = nullptr;
		tie(wAnchorTableName, wAnchorTable) =
			sSheet->FindRangeDataCovered(static_cast<tInt>(wRowAnchor),
										 static_cast<tInt>(wColAnchor));
		const tBool wAnchorInSameTable =
			(wAnchorTable == wPickTable) && (wAnchorTableName == wPickTableName);
		const tBool wAnchorOnDataRow =
			wAnchorInSameTable
			&& wRowAnchor >= wDataTop && wRowAnchor <= wDataBottom
			&& wColAnchor >= wPickTable->LeftIndex()
			&& wColAnchor <= wPickTable->RightIndex();

		const tBool wSameRow =
			wAnchorOnDataRow && (wTop == wBottom) && (wTop == wRowAnchor);
		const tBool wFullColumns =
			(wTop == wDataTop) && (wBottom == wDataBottom);
		if (!wSameRow && !wFullColumns) {
			return wA1Fallback;
		}

		const tBool wSingleCol = (wLeft == wRight);
		tString wSpec = BuildStructuredColumnSpec(
			wLabelLeft, wLabelRight, wSingleCol, wSameRow);

		// Inside the same table: omit the table name (Excel bar form).
		// Outside (or other table): prefix TableName[[Col]].
		if (wAnchorInSameTable) {
			return wSpec;
		}
		return FormatStructuredTablePrefix(wPickTableName) + wSpec;
	}
	// Interface Range ==================================================
	tRange* tApi::FindRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->Range(sTop,sLeft,sBottom,sRight));
	}
	
	tRange* tApi::EnsureRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->EnsureRange(sTop, sLeft, sBottom, sRight));
	}

	tRange* tApi::Range(tString sRef, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tIndex wTop, wLeft, wBottom, wRight;
		if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
			return(sSheet->Range(wTop, wLeft, wBottom, wRight));
		}
		return(nullptr);
	}

	tBool tApi::DeleteRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		return(sSheet->EraseRange(sTop, sLeft, sBottom, sRight));
	}

	tRange* tApi::EnsureRangeNamed(tString sName, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
        tWorkBook* wWorkBook=sSheet->WorkBook();
        tTempoRect wRect(sTop,sLeft, sBottom, sRight);
        tRange* wRange=wWorkBook->InsertRangeNamed(sName,wRect,sSheet);
		return(wRange);
	}

	tRange* tApi::EnsureRangeData(tString sName,tString sJsonData, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
        tWorkBook* wWorkBook=sSheet->WorkBook();
        tTempoRect wRect(sTop,sLeft, sBottom, sRight);
        tRangeData wRangeData;
        wRangeData.Json(sJsonData);
        tRange* wRange=nullptr;
        if (!wRangeData.IsEmpty()) {
            wRange=wWorkBook->InsertRangeData(sName,wRangeData, wRect, sSheet);
        } else {
            wRange=wWorkBook->InsertRangeNamed(sName,wRect,sSheet);
        }
		return(wRange);
	}

	tBool tApi::DeleteRangeNamed(tString sName) {
		return(m_SpreadSheetContainer->ActiveWorkBook()->DeleteRangeNamed(sName));
	}

	tRange* tApi::EnsureMergedRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tRange* wRange=sSheet->EnsureRange(sTop, sLeft, sBottom, sRight);
		wRange->SetMerged();
		return(wRange);
	}

	tBool tApi::DeleteMergedRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tRange* wRange = sSheet->EnsureRange(sTop, sLeft, sBottom, sRight);
		// Remove Merged flag
		wRange->RemoveMerged();
		if (wRange->NotDependent()) {
            if (!wRange->IsEmpty()) {
				return(sSheet->EraseRange(sTop, sLeft, sBottom, sRight));
			}
		}
		return(false);
	}

	// Interface Str =============================================================
    tString tApi::Formula(tString sRef, tSheet* sSheet, tBool sUser) {
        NormalizeSheet(&sSheet);
        tIndex wRow; tIndex wCol;
        if (ParseCell(sRef, wRow, wCol)) {
            tCell* wCell = sSheet->Cell(wRow, wCol);
            if (wCell != nullptr) {
                return(wCell->FormulaStr(false, sUser));
            }
        }
        return("");
    }
	tRange* tApi::FindRange(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
		tIndex wTop, wLeft, wBottom, wRight;
		if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
			return(FindRange(wTop, wLeft, wBottom, wRight, sSheet));
		}
		return(nullptr);
	}

	tRange* tApi::EnsureRange(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
		tIndex wTop, wLeft, wBottom, wRight;
		if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
			return(EnsureRange(wTop, wLeft, wBottom, wRight, sSheet));
		}
		return(nullptr);
	}

	tBool tApi::DeleteRange(tString sRef, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
		tIndex wTop, wLeft, wBottom, wRight;
		if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
			return(DeleteRange(wTop, wLeft, wBottom, wRight, sSheet));
		}
		return(false);
	}

	tRange* tApi::EnsureRangeNamed(tString sName, tString sRef,tString sFormula, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
		tIndex wTop, wLeft, wBottom, wRight;
		if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
            return(EnsureRangeNamed(sName,wTop, wLeft, wBottom, wRight, sSheet));
		}
		return(nullptr);
	}
	tRange* tApi::FindRangeNamed(tString sName) {
		return(ActiveWorkBook()->FindRangeNamed(sName));
	}
    
    tRangeData* tApi::RangeData(tString sName) {
		return(ActiveWorkBook()->RangeData(sName));
    }


	// Insert delete Row & Col
	void tApi::InsertRow(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		sSheet->DoInsertRow(sRow,sSize);
		sSheet->WorkBook()->ApplyCalculatedColumnFormulasToInsertedRows(
			sSheet, sRow, sRow + sSize - 1);
	}

	void tApi::DeleteRow(tIndex sRow, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tSaveSelectErase wSaveSelectErase;
		sSheet->DoDeleteRow(&wSaveSelectErase, sRow, sSize);
	}

	void tApi::InsertCol(tIndex sCol, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		sSheet->DoInsertCol(sCol, sSize);
	}

	void tApi::DeleteCol(tIndex sCol, tIndex sSize, tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		tSaveSelectErase wSaveSelectErase;
		sSheet->DoDeleteCol(&wSaveSelectErase, sCol, sSize);
	}

	void tApi::DeleteSheet(tSheet* sSheet) {
		tSaveSelectErase wSaveSelectErase;
		sSheet->DoDeleteSheet(&wSaveSelectErase);
	}

    tPoint  tApi::BottomRight(tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(tPoint(sSheet->LastRow(),sSheet->LastCol()));
    }

    tDouble  tApi::SumPixelWidth(tIndex sColStart,tIndex sColEnd,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->SumWidth(sColStart,sColEnd,tUnitMetrics::pixels));
    }

    tDouble  tApi::SumPixelHeight(tIndex sRowStart,tIndex sRowEnd,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->SumHeight(sRowStart,sRowEnd,tUnitMetrics::pixels));
    }

    std::tuple<tIndex,tDouble> tApi::IndexColByPixel(tIndex sColStart,tDouble sPixel,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->IndexColByPos(sColStart, sPixel,tUnitMetrics::pixels));
    }

    std::tuple<tIndex,tDouble>  tApi::IndexRowByPixel(tIndex sRowStart,tDouble sPixel,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->IndexRowByPos(sRowStart, sPixel,tUnitMetrics::pixels));
    }

    tRect tApi::MoveCell(tPoint sCellPoint, tByte sKey, tByte sMeta, tRect  sScreen,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->MoveCell(sCellPoint,sKey,sMeta,sScreen));
    }

    tRect tApi::MoveToCell(tPoint sCellPoint, tByte sDirection,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return(sSheet->MoveToCell(sCellPoint,sDirection));
    }
    
    // JSON Interface ==========================================================
	tString tApi::JsonView(tIndex sRow, tIndex sCol, tUnitMetrics sUnit, tDouble sViewHeight, tDouble sViewWidth,tDouble sDiffY,tDouble sDiffX, tBool sCss,  tSheet* sSheet) {
		NormalizeSheet(&sSheet);
		StringBuffer wStringBuffer;
		Writer<StringBuffer> wWriter(wStringBuffer);
        
        m_JsonView.ColRowCellRange(sSheet->ColRowCellRange());
        
        wWriter.StartObject();
        m_JsonView.View(&wWriter,sRow, sCol, sUnit, sViewHeight, sViewWidth,sDiffY,sDiffX,sCss);
        wWriter.EndObject();
        
		return(wStringBuffer.GetString());
	}
        
    tString tApi::JsonRightJustify(tIndex sCol, tUnitMetrics sUnit,tDouble sViewWidth,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        m_JsonView.ColRowCellRange(sSheet->ColRowCellRange());
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        m_JsonView.ReturnRightJustify(&wWriter,sCol, sUnit, sViewWidth);
        return(wStringBuffer.GetString());
    }

    tString tApi::JsonBottomJustify(tIndex sRow, tUnitMetrics sUnit,tDouble sViewHeight,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        m_JsonView.ColRowCellRange(sSheet->ColRowCellRange());
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        m_JsonView.ReturnBottomJustify(&wWriter,sRow, sUnit, sViewHeight);
        return(wStringBuffer.GetString());
    }

    tString tApi::JsonColByPixel(tIndex sColStart,tDouble sPixel,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        std::tuple<tIndex,tDouble> wResult=IndexColByPixel(sColStart, sPixel);
        wWriter.StartObject();
        wWriter.Key("c");
        wWriter.Int64(std::get<0>(wResult));
        wWriter.Key("d");
        tInt wDouble=tInt(std::get<1>(wResult)*1000);
        wWriter.Double(wDouble/1000);
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    tString tApi::JsonRowByPixel(tIndex sRowStart,tDouble sPixel,tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        std::tuple<tIndex,tDouble> wResult=IndexRowByPixel(sRowStart, sPixel);
        wWriter.StartObject();
        wWriter.Key("r");
        wWriter.Int64(std::get<0>(wResult));
        wWriter.Key("d");
        tInt wDouble=tInt(std::get<1>(wResult)*1000);
        wWriter.Double(wDouble/1000);
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    tString tApi::JsonFloatingObjectsForSheet(tString sTargetSheetName, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tWorkBook* const wWorkBook = sSheet->WorkBook();
        if (wWorkBook == nullptr) {
            return ("{\"objects\":[]}");
        }
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("objects");
        wWorkBook->FloatingObjectContainer()->JsonFloatingObjectsForSheet(&wWriter, sTargetSheetName);
        wWriter.EndObject();
        return (wStringBuffer.GetString());
    }

    tString tApi::JsonFloatingObjects(tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tWorkBook* const wWorkBook = sSheet->WorkBook();
        if (wWorkBook == nullptr) {
            return ("{\"objects\":[]}");
        }
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("objects");
        wWorkBook->FloatingObjectContainer()->JsonFloatingObjectsList(&wWriter);
        wWriter.EndObject();
        return (wStringBuffer.GetString());
    }

    tString tApi::JsonSheets() {
        return(m_SpreadSheetContainer->ActiveWorkBook()->JsonSheets());
    }
    
    
    tString tApi::JsonRangeNamed() {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook != nullptr) {
            return(wWorkBook->JsonRangeNamed());
        }
        return("");
    }
    
     tString tApi::JsonformulaNamed() {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook != nullptr) {
            return(wWorkBook->JsonFormulaNamed());
        }
        return("");
     }
    
    tString tApi::JsonRangeData() {
       tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook != nullptr) {
            return(wWorkBook->JsonRangeData());
        }
        return("");
    }
    
    tString  tApi::JsonWorkBookConditionalFormats() {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook != nullptr) {
            return(wWorkBook->JsonConditionalFormats());
        }
        return("");
    }
    
   tString tApi::JsonConditionalFormats(tSheet* sSheet) {
    NormalizeSheet(&sSheet);
    tString wJsonConditionalFormat;
    if (sSheet->ColRowCellRange()->ConditionalFormatContainer()!=nullptr) {
        wJsonConditionalFormat = sSheet->ColRowCellRange()->ConditionalFormatContainer()->Json();
    }
    return(wJsonConditionalFormat);
   }

   tString tApi::JsonConditionalFormats(tRect& sRect, tSheet* sSheet) {
    NormalizeSheet(&sSheet);
    tString wJsonConditionalFormat;
    if (sSheet->ColRowCellRange()->ConditionalFormatContainer()!=nullptr) {
        wJsonConditionalFormat = sSheet->ColRowCellRange()->ConditionalFormatContainer()->JsonByRect(&sRect);
    }
    return(wJsonConditionalFormat);
   }

    tString tApi::JsonPrintParameters(tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        return sSheet->JsonPrintParameters();
    }
 
    tBool tApi::JsonPrintParameters(tString sJsonPrintParameters, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        auto* wUndo = new tUndoPrintParameters(sJsonPrintParameters, m_Mode, sSheet);
        return(Do(wUndo));
    }

    tString tApi::JsonFindCell(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tVectorCell* wVectorCell = sSheet->FindCell(sSearch, sMatchCase, sMatchEntireCell);
        if (wVectorCell != nullptr) {
            StringBuffer wStringBuffer;
            Writer<StringBuffer> wWriter(wStringBuffer);
            wWriter.StartObject();
            wWriter.Key("cells");
            wWriter.StartArray();
            for (auto wCell : *wVectorCell) {
                wWriter.String(wCell->StrRef().c_str());
            }
            wWriter.EndArray();
            wWriter.EndObject();
            tString wResult(wStringBuffer.GetString());
            delete wVectorCell;
            return(wResult);
        }
        return("");
    }

    tString tApi::JsonFindCellWorkBook(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell) {
        tWorkBook* wWorkBook = m_SpreadSheetContainer->ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return "";
        }
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("cells");
        wWriter.StartArray();
        tVectorAllocatorRef* wVectorSheet = wWorkBook->VectorSheet();
        if (wVectorSheet != nullptr) {
            for (auto wSheetRef : *wVectorSheet) {
                tSheet* wSheet = wWorkBook->SheetByAllocator(wSheetRef);
                if (wSheet == nullptr || IsSystemSheetName(wSheet->Name())) {
                    continue;
                }
                tVectorCell* wVectorCell = wSheet->FindCell(sSearch, sMatchCase, sMatchEntireCell);
                if (wVectorCell == nullptr) {
                    continue;
                }
                for (auto wCell : *wVectorCell) {
                    wWriter.String(SheetQualifiedCellRef(wSheet, wCell).c_str());
                }
                delete wVectorCell;
            }
        }
        wWriter.EndArray();
        wWriter.EndObject();
        return wStringBuffer.GetString();
    }

    tString tApi::JsonFindUniqueValue(tRect sRect, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        tVectorString* wVectorString = sSheet->FindUniqueValue(sRect);
        if (wVectorString != nullptr) {
            StringBuffer wStringBuffer;
            Writer<StringBuffer> wWriter(wStringBuffer);
            wWriter.StartObject();
            wWriter.Key("values");
            wWriter.StartArray();
            for (const tString& wValue : *wVectorString) {
                wWriter.String(wValue.c_str());
            }
            wWriter.EndArray();
            wWriter.EndObject();
            tString wResult(wStringBuffer.GetString());
            delete wVectorString;
            return(wResult);
        }
        return("");
    }

    tRect tApi::GetSheetSelect() {
        tRect wRect;
        wRect.SheetSelect();
        return(wRect);
    }

    tRect tApi::GetColSelect(tIndex sColBegin,tIndex sSize) {
        return(tRect(0,sColBegin,Cst_MaxRow,sColBegin+sSize-1));
    }

    tRect tApi::GetRowSelect(tIndex sRowBegin,tIndex sSize) {
        return(tRect(sRowBegin,0,sRowBegin+sSize-1,Cst_MaxCol));
    }
    // for test  =============================================================
    tColRow::tContainerRange::tResult* tApi::FindRangesCovered(tCell* sCell, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        sSheet->FindRangesCovered(sCell, &m_VectorResult);
        return(&m_VectorResult);
    }

    tColRow::tContainerRange::tResult* tApi::FindRangesCovered(tIndex sRow, tIndex sCol, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        sSheet->FindRangesCovered(sRow, sCol, &m_VectorResult);

        return(&m_VectorResult);
    };

    void tApi::FindRangesCovered(tRect sRect,tVectorRange* sVectorRange, tSheet* sSheet) {
        NormalizeSheet(&sSheet);
        sSheet->FindRangesCovered(sRect, sVectorRange);
    }

    tBool tApi::PostMessage(tString sMessage) {
        return(false);
    }
     
    tBool tApi::GetMessage(tString sMessage) {
        return(false);
    }

    // Error Spread Sheet ======================================================
    tString tApi::Error() { return(LemonInterface()->Error()); }

    tInt tApi::ErrorLine()  { return(LemonInterface()->ErrorLine()); }

    tInt tApi::ErrorColumn()  { return(LemonInterface()->ErrorColumn()); }

	tString tApi::ErrorWithDetail() { return(LemonInterface()->ErrorWithDetail()); }
} // End of namespace

