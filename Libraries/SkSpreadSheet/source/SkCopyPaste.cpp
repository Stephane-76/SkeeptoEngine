//=============================================================================
// SkCopyPaste (Copy and paste)
//=============================================================================
#include "../include/SkCopyPaste.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkSelect.hpp"

#define _debugcopypaste

namespace SkSpreadSheet {

    namespace {

        tBool SelectContainsCell(const tSelect& sSelect, tIndex sRow, tIndex sCol) {
            tVectorTempoPoint* wVector = sSelect.VectorSelect();
            if (wVector == nullptr) {
                return(false);
            }
            for (auto wItem : *wVector) {
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                if (wRect != nullptr) {
                    if (sRow >= wRect->Top() && sRow <= wRect->Bottom() &&
                        sCol >= wRect->Left() && sCol <= wRect->Right()) {
                        return(true);
                    }
                } else if (wItem != nullptr) {
                    if (wItem->Row() == sRow && wItem->Col() == sCol) {
                        return(true);
                    }
                }
            }
            return(false);
        }

        void EraseEmptyFormattedCell(tSheet* sSheet, tIndex sRow, tIndex sCol) {
            if (sSheet == nullptr) {
                return;
            }
            tCell* wCell = sSheet->Cell(sRow, sCol);
            if (wCell == nullptr) {
                return;
            }
            // Format-only ghosts still have Css != 0; IsValueEmpty() would skip them.
            if (wCell->Formula() != nullptr ||
                wCell->Value().Type() != tVariantType::t_null) {
                return;
            }
            tWorkBook* wWorkBook = sSheet->WorkBook();
            if (wCell->Css() != 0 && wWorkBook != nullptr) {
                wWorkBook->DeleteCellFormat(wCell->Css());
                wCell->Css(0);
            }
            wCell->ClearFormulaAndVariant();
            if (wCell->IsEmpty()) {
                sSheet->DeleteCell(sRow, sCol);
            }
        }

    } // namespace

    void ClearMoveSourceExteriorNeighborGhosts(tSheet* sSheet, const tSelect& sSourceClearSelect,
                                               const tSelect& sMoveDest) {
        if (sSheet == nullptr) {
            return;
        }
        tTempoRect* wSourceRect = sSourceClearSelect.FirstRect();
        if (wSourceRect == nullptr) {
            return;
        }
        for (tIndex wRow = wSourceRect->Top(); wRow <= wSourceRect->Bottom(); wRow++) {
            for (tIndex wCol = wSourceRect->Left(); wCol <= wSourceRect->Right(); wCol++) {
                if (SelectContainsCell(sMoveDest, wRow, wCol)) {
                    continue;
                }
                EraseEmptyFormattedCell(sSheet, wRow, wCol);
            }
        }
    }

	//=========================================================================
	tCopy::tCopy() : tClass(), m_Sheet(nullptr), m_Point(0, 0) {
    }

	tCopy::~tCopy() {
    };

	tBool tCopy::CallBackCell(tTempoPoint* sPoint) {
		tCell* wCell = m_Sheet->Cell(sPoint->Row(), sPoint->Col());
		if (wCell != nullptr) {
            if (!wCell->IsEmpty()) {
#ifdef debugcopypaste
                cout << "." << wCell->StrRef();
                tString wFormulaStr=wCell->FormulaStr();
                if (wFormulaStr!="") cout << "=" << wFormulaStr;
                cout << wCell->Value() << endl;
#endif
                m_Writer->StartObject();
                wCell->Json(m_Writer, true,&m_Point);
                m_Writer->EndObject();
            }
		}
		return(true);
	}

	tBool tCopy::CallBackRange(tTempoRect* sRect) {
		for (tIndex wRow = sRect->Row(); wRow <= sRect->Bottom(); wRow++) {
			for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
				tTempoPoint wPoint(wRow, wCol);
				if (!CallBackCell(&wPoint)) return(false);
			}
		}
		return(true);
	}

    tBool CopySelectionToJson(tString sRef, tSheet* sSheet, tString& sOutJson,
                              tBool sUpdateClipboard) {
        sOutJson.clear();
        tCopy wCopy;
        if (!wCopy.Treat(sRef, sSheet, &sOutJson)) {
            return(false);
        }
        if (sUpdateClipboard) {
            tApplication::Instance()->Clipboard()->Text(sOutJson);
        }
        return(!sOutJson.empty());
    }

    tBool CopyJsonHasCellPayload(const tString& sCopyJson) {
        if (sCopyJson.empty()) {
            return(false);
        }
        Document wDocument;
        if (wDocument.Parse(sCopyJson.c_str()).HasParseError()) {
            return(false);
        }
        if (!wDocument.HasMember("cells") || !wDocument["cells"].IsArray()) {
            return(false);
        }
        return(wDocument["cells"].Size() > 0);
    }

	tBool tCopy::Treat(tString sRef, tSheet* sSheet, tString* oJson) {
        // Init Json Shared String
        tSpreadSheetContainer::Instance()->JsonBegin();
       
		tBool wResult = false;
		m_Sheet = sSheet;
        // RESET Json Index of format
        tFormatApi* wFormatApi =tSpreadSheetContainer::Instance()->FormatApi();
        if (wFormatApi!=nullptr) {
            // Init save Format
            wFormatApi->BeginWriteJson();
        }
       
		// Important Select use temporary memory
		// We have to parse again every time
        tSelect wSelect;
		if (wSelect.Parse(sRef)) {
            // For copy Just One Cell or Rect
            tBool wOk=((wSelect.JustOneCell()) || (wSelect.JustOneRect()));
            if (!wOk) {
                tApplication::Instance()->Clipboard()->Text("");
                return(false);
            }
			// Get Top Left Point for calculate diff in copy paste
			tTempoPoint* wTempoPoint=wSelect.ReturnTopLeftPoint();
			m_Point.Row(wTempoPoint->Row());
			m_Point.Col(wTempoPoint->Col());
			StringBuffer wJsonBuffer;
			m_Writer=new Writer<StringBuffer>(wJsonBuffer);
			// Write Select for Copy Multiple
			m_Writer->StartObject();
			m_Writer->Key("select");
			m_Writer->StartArray();
			wSelect.Json(m_Writer);
			m_Writer->EndArray();
			
			m_Writer->Key("cells");
			m_Writer->StartArray();
			wResult = wSelect.CallBack(this);
			m_Writer->EndArray();
   
            // write Shared String
            tSpreadSheetContainer::Instance()->JsonShared(m_Writer);

            if (wFormatApi!=nullptr) {
                // save Format
                m_Writer->Key("f");
                wFormatApi->Json(m_Writer);
            }
            
			m_Writer->EndObject();
            // Copy only serializes cells; do not JsonCompil / recalc named ranges (breaks lstAnnées etc.).
            tSpreadSheetContainer::Instance()->JsonEndShared();
            // Put to ClipBoard
			if (wResult) {
                const tString wJson(wJsonBuffer.GetString());
#ifdef debugcopypaste
				cout << wJson << endl;
#endif
                if (oJson != nullptr) {
                    *oJson = wJson;
                }
                tApplication::Instance()->Clipboard()->Text(wJson);
			}
			delete(m_Writer);
		};
		return(wResult);
	}
};
