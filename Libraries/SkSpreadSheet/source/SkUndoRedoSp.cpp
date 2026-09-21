//=============================================================================
// SkSpreadSheet Undo Redo
//=============================================================================
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkLemonInterface.hpp"
#include "../include/SkSpreadSheet.hpp" 
#include "../include/SkUndoRedoRebaseFormula.hpp"
#include "../include/SkCellClassAttribute.hpp"
#include "../include/SkFloatingObject.hpp"
#include "../include/SkCopyPaste.hpp"
#include "../include/SkCollaborationLimits.hpp"
#include "../include/SkJsonKey.hpp"
#include "../include/SkRangeData.hpp"
#include <SkFormatString.hpp>
#include <set>
#include <string>
#include <vector>
namespace SkSpreadSheet {

    namespace {
        /// Apply a US wire formula (SaveCell / redo / collab). Author first-Do keeps UI locale.
        tBool CellValueFromWire(tWorkBook* sWorkBook, tCell* sCell, tVariant& sValue, tBool sCalculate) {
            if (sWorkBook == nullptr || sCell == nullptr) {
                return false;
            }
            if (sValue.HasFormula()) {
                tLocalePush wUs("us");
                return sWorkBook->CellValue(sCell, sValue, sCalculate);
            }
            return sWorkBook->CellValue(sCell, sValue, sCalculate);
        }

        /// After a successful formula compile, store US A1 text for collab Json (not locale FormulaStr).
        void NormalizeUndoFormulaToWire(tCell* sCell, tVariant& sValue, tVariant& sValueRebased) {
            if (sCell == nullptr || sCell->Formula() == nullptr) {
                return;
            }
            const tString wWire = sCell->FormulaWire();
            if (wWire.empty()) {
                return;
            }
            tVariant wNorm("=" + wWire);
            sValue = wNorm;
            sValueRebased = wNorm;
        }

        static tBool ParseSelectionRects(const tString& sRef, std::vector<tTempoRect>& sOutRects) {
            sOutRects.clear();
            tSelect wSelect;
            wSelect.Parse(sRef);
            tVectorTempoPoint* wVect = wSelect.VectorSelect();
            if (wVect == nullptr) {
                return false;
            }
            for (auto wItem : *wVect) {
                if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
                    sOutRects.push_back(*wRect);
                } else if (tTempoPoint* wPt = dynamic_cast<tTempoPoint*>(wItem)) {
                    tTempoRect wCellRect;
                    wCellRect.Top(wPt->Row());
                    wCellRect.Left(wPt->Col());
                    wCellRect.Bottom(wPt->Row());
                    wCellRect.Right(wPt->Col());
                    sOutRects.push_back(wCellRect);
                }
            }
            return !sOutRects.empty();
        }

        static void TableLogicalBounds(tRange* sTable,
                                       tRangeData* sRangeData,
                                       tIndex& oTop,
                                       tIndex& oLeft,
                                       tIndex& oBottom,
                                       tIndex& oRight) {
            oTop = sTable->TopIndex();
            oLeft = sTable->LeftIndex();
            oBottom = sTable->BottomIndex();
            oRight = sTable->RightIndex();
            if (sRangeData != nullptr) {
                const tInt wTotals = sRangeData->TotalsRowCount();
                if (wTotals > 0) {
                    oBottom += static_cast<tIndex>(wTotals);
                }
            }
        }

        static tBool TableExtentInsideRazRect(tRange* sTable,
                                              tRangeData* sRangeData,
                                              tTempoRect sRaz) {
            if (sTable == nullptr) {
                return false;
            }
            const tIndex wRazTop = sRaz.Top();
            const tIndex wRazLeft = sRaz.Left();
            const tIndex wRazBottom = sRaz.Bottom();
            const tIndex wRazRight = sRaz.Right();
            tIndex wTop = 0;
            tIndex wLeft = 0;
            tIndex wBottom = 0;
            tIndex wRight = 0;
            TableLogicalBounds(sTable, sRangeData, wTop, wLeft, wBottom, wRight);
            if (wTop < wRazTop) {
                return false;
            }
            if (wLeft < wRazLeft) {
                return false;
            }
            if (wBottom > wRazBottom) {
                return false;
            }
            if (wRight > wRazRight) {
                return false;
            }
            return true;
        }
    }

    tBool RejectCollaborationPasteIfTooLarge(tUndoSpreadSheet* sUndo, tBool sCollaborationContext) {
        if (!sCollaborationContext || sUndo == nullptr) {
            return false;
        }
        tSize wCopyBytes = 0;
        if (const tUndoPaste* wPaste = dynamic_cast<const tUndoPaste*>(sUndo)) {
            wCopyBytes = wPaste->CollaborationCopyByteSize();
        } else if (const tUndoCut* wCut = dynamic_cast<const tUndoCut*>(sUndo)) {
            wCopyBytes = wCut->CollaborationCopyByteSize();
        } else {
            return false;
        }
        if (!IsCollaborationPastePayloadTooLarge(wCopyBytes)) {
            return false;
        }
        const tString wMessage(CollaborationPasteTooLargeMessage());
        sUndo->Error(wMessage);
        tLemonInterface* wLemon = tSpreadSheetContainer::Instance()->LemonInterface();
        if (wLemon != nullptr) {
            wLemon->Error(wMessage, 0, 0);
        }
        return true;
    }

    tUndoSpreadSheet* CreateUndoPasteOrMoveAfterCut(tString sDestRef, tMode sMode, tSheet* sSheet,
                                                    tUndoRedoContainer* sContainer) {
        if (sContainer == nullptr) {
            return(nullptr);
        }
        tUndoCut* wCut = dynamic_cast<tUndoCut*>(sContainer->LastUndo());
        if (wCut == nullptr || !wCut->MatchesCutClipboard()) {
            return(nullptr);
        }
        tSelect wSource;
        tSelect wDest;
        if (!wSource.Parse(wCut->RefRebase()) || !wDest.Parse(sDestRef)) {
            return(nullptr);
        }
        tUndoMove wProbe;
        if (!wProbe.MoveSelectSameGeometry(wSource, wDest)) {
            return(nullptr);
        }
        tUndo* wDetached = sContainer->DetachLastUndo();
        if (wDetached != wCut) {
            delete(wDetached);
            return(nullptr);
        }
        tUndoMove* wMove = new tUndoMove(wCut->RefRebase(), sDestRef, sMode, sSheet);
        wMove->AdoptCompletedCut(wCut);
        delete(wCut);
        return(wMove);
    }

namespace {

tBool SelectHasNonEmptyCell(tSheet* sSheet, const tSelect& sSelect) {
    if (sSheet == nullptr) {
        return(false);
    }
    tVectorTempoPoint* wVector = sSelect.VectorSelect();
    if (wVector == nullptr) {
        return(false);
    }
    for (auto wItem : *wVector) {
        tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
        if (wRect != nullptr) {
            for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                    tCell* wCell = sSheet->Cell(wRow, wCol);
                    if (wCell != nullptr && !wCell->IsEmpty()) {
                        return(true);
                    }
                }
            }
        } else if (wItem != nullptr) {
            tCell* wCell = sSheet->Cell(wItem->Row(), wItem->Col());
            if (wCell != nullptr && !wCell->IsEmpty()) {
                return(true);
            }
        }
    }
    return(false);
}

// Grid relocate (detach m_Cell2D, preserve tCell*) for move; overlap supported.
constexpr tBool kUndoMoveUseGridRelocate = true;

static tBool RectsOverlapImpl(tRect& sA, tRect& sB) {
    return(sA.Top() <= sB.Bottom() && sB.Top() <= sA.Bottom() &&
           sA.Left() <= sB.Right() && sB.Left() <= sA.Right());
}

static void CollectFormulaDependents(tCell* sCell, std::vector<tCell*>& ioCells) {
    if (sCell == nullptr) {
        return;
    }
    for (auto wItem : *sCell->ContainerCellDepend()->Container()) {
        if (tCell* wDependent = wItem->Cell(); wDependent != nullptr) {
            tBool wFound = false;
            for (tCell* wExisting : ioCells) {
                if (wExisting == wDependent) {
                    wFound = true;
                    break;
                }
            }
            if (!wFound) {
                ioCells.push_back(wDependent);
            }
        }
    }
}

static void CollectFormulaDependentsFromRect(tColRowCellRange* sColRowCellRange, tRect& sRect,
                                             std::vector<tCell*>& ioCells) {
    if (sColRowCellRange == nullptr) {
        return;
    }
    for (tIndex wRow = sRect.Top(); wRow <= sRect.Bottom(); wRow++) {
        for (tIndex wCol = sRect.Left(); wCol <= sRect.Right(); wCol++) {
            CollectFormulaDependents(sColRowCellRange->Cell(wRow, wCol), ioCells);
        }
    }
}

static tBool SourceRectHasMovingCells(tColRowCellRange* sGrid, tRect& sSourceRect) {
    if (sGrid == nullptr) {
        return(false);
    }
    for (tIndex wRow = sSourceRect.Top(); wRow <= sSourceRect.Bottom(); wRow++) {
        for (tIndex wCol = sSourceRect.Left(); wCol <= sSourceRect.Right(); wCol++) {
            if (sGrid->CellAllocatorRef(wRow, wCol) != 0) {
                return(true);
            }
        }
    }
    return(false);
}

// Grid relocate (detach m_Cell2D, preserve tCell*) for move; overlap supported.

static void RecompilFormulaDependentCells(tWorkBook* sWorkBook, const std::vector<tCell*>& sCells) {
    if (sWorkBook == nullptr) {
        return;
    }
    for (tCell* wCell : sCells) {
        if (wCell == nullptr) {
            continue;
        }
        const tString wFormula = wCell->FormulaStr();
        if (wFormula.empty()) {
            continue;
        }
        (void)sWorkBook->CompilCell(wCell, wFormula.c_str());
        wCell->Calculation();
    }
}

static void SaveSelectCellsForGridMove(tUndoMove* sUndo, tSelect& sSelect,
                                       tBool sPassCss, tBool sDetachClassContainer) {
    tSheet* wSheet = sUndo->Sheet();
    if (wSheet == nullptr) {
        return;
    }
    tSaveSelect* wSaveSelect = sUndo->SaveSelect();
    tVectorTempoPoint* wVector = sSelect.VectorSelect();
    if (wVector == nullptr) {
        return;
    }
    auto wSaveCellAt = [&](tIndex sRow, tIndex sCol) {
        tCell* wCell = wSheet->Cell(sRow, sCol);
        if (wCell == nullptr || wSaveSelect->FindCell(wCell) != nullptr) {
            return;
        }
        tSaveCell* wSaveCell = wSaveSelect->AddCell(wCell);
        if (sPassCss && sUndo->IsUndoActif() && !sUndo->IsJson()) {
            wSaveCell->PassCss(wCell);
        }
        if (tCellClassAttribute* wCellClass = wCell->ClassAttribute()) {
            tVectorCellAttribute wAttributes;
            wCellClass->CellAttribute(&wAttributes);
            for (tCellAttribute* wAttribute : wAttributes) {
                wSaveSelect->AddCell(wAttribute);
                wSaveSelect->TreatsExternalDependencyCells(wAttribute);
            }
            if (sDetachClassContainer) {
                wSheet->DeleteCellClassAttributeContainer(wCell);
            }
        }
    };
    for (auto wItem : *wVector) {
        if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
            for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                    wSaveCellAt(wRow, wCol);
                }
            }
        } else if (wItem != nullptr) {
            wSaveCellAt(wItem->Row(), wItem->Col());
        }
    }
}

static void ClearUnsavedDestCells(tUndoMove* sUndo, tSelect& sDest,
                                  tRect& sSourceRect, tRect& sDestRect) {
    tSheet* wSheet = sUndo->Sheet();
    if (wSheet == nullptr) {
        return;
    }
    tVectorTempoPoint* wVector = sDest.VectorSelect();
    if (wVector == nullptr) {
        return;
    }
    auto wClearIfUnsaved = [&](tIndex sRow, tIndex sCol) {
        if (sRow >= sSourceRect.Top() && sRow <= sSourceRect.Bottom() &&
            sCol >= sSourceRect.Left() && sCol <= sSourceRect.Right()) {
            return;
        }
        if (sRow < sDestRect.Top() || sRow > sDestRect.Bottom() ||
            sCol < sDestRect.Left() || sCol > sDestRect.Right()) {
            return;
        }
        tCell* wCell = wSheet->Cell(sRow, sCol);
        if (wCell == nullptr) {
            return;
        }
        wCell->ClearFormulaAndVariant();
        if (wCell->Css() != 0) {
            if (tFormatApi* wFormatApi = wSheet->WorkBook()->FormatApi()) {
                wFormatApi->DeleteCellFormat(wCell->Css());
            }
            wCell->Css(0);
        }
        wSheet->DeleteCell(sRow, sCol);
    };
    for (auto wItem : *wVector) {
        if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
            for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                    wClearIfUnsaved(wRow, wCol);
                }
            }
        } else if (wItem != nullptr) {
            wClearIfUnsaved(wItem->Row(), wItem->Col());
        }
    }
}

static tBool MoveDestHasPriorContent(tSheet* sSheet, tRect& sDest) {
    if (sSheet == nullptr) {
        return(false);
    }
    tColRowCellRange* wGrid = sSheet->ColRowCellRange();
    if (wGrid == nullptr) {
        return(false);
    }
    for (tIndex wRow = sDest.Top(); wRow <= sDest.Bottom(); wRow++) {
        for (tIndex wCol = sDest.Left(); wCol <= sDest.Right(); wCol++) {
            if (wGrid->CellAllocatorRef(wRow, wCol) != 0) {
                return(true);
            }
            if (tCell* wCell = sSheet->Cell(wRow, wCol); wCell != nullptr) {
                if (!wCell->IsEmpty() || wCell->Css() != 0) {
                    return(true);
                }
            }
            if (tColRow* wColRow = wGrid->Col(wCol); wColRow != nullptr && wColRow->Css() != 0) {
                return(true);
            }
            if (tColRow* wRowRow = wGrid->Row(wRow); wRowRow != nullptr && wRowRow->Css() != 0) {
                return(true);
            }
        }
    }
    return(false);
}

//! Bind sheet pointer from wire key `sh`; optional active-sheet switch (local undo only).
void UndoWireSheetFromJsonSh(
    tSharedString& ioSheetName,
    tSheet*& ioSheet,
    const rapidjson::Value& sValue,
    tBool sSetActiveSheet) {
    tSpreadSheetContainer* wSpreadSheetContainer = tSpreadSheetContainer::Instance();
    tWorkBook* wWorkBook = wSpreadSheetContainer->ActiveWorkBook();
    if (sValue.HasMember(kJsonKeySheet) && sValue[kJsonKeySheet].IsString()) {
        ioSheetName = sValue[kJsonKeySheet].GetString();
    } else {
        ioSheetName = tSharedString();
    }
    tSheet* wSheet = (wWorkBook != nullptr) ? wWorkBook->Sheet(ioSheetName()) : nullptr;
    if (wSheet != nullptr) {
        ioSheet = wSheet;
        wSpreadSheetContainer->ActiveWorkBook(wWorkBook);
        if (sSetActiveSheet && !IsSystemSheetName(ioSheetName())) {
            wWorkBook->ActiveSheet(ioSheetName());
        }
    } else {
        ioSheet = nullptr;
    }
}

//! Emit `sh` using persisted shared sheet name (same role as tUndoSpreadSheetCallBack wiring).
void UndoWriteSheetSh(Writer<StringBuffer>* sWriter, const tSharedString& sSheetName, tSheet* sSheet) {
    sWriter->Key(kJsonKeySheet);
    const tString wSn =
        (!sSheetName().empty()) ? sSheetName() : ((sSheet != nullptr) ? sSheet->Name() : tString());
    sWriter->String(wSn.c_str());
}

} // namespace

#define _debugundoredo

    tString BorderStr(tShort sBorder) {
        switch (sBorder) {
            case tBorderAll : return("border");
            case tBorderTop : return("border-top");
            case tBorderLeft : return("border-left");
            case tBorderBottom : return("border-bottom"); break;
            case tBorderRight : return("border-right"); break;
            // Compound commands (Outside/Inside/Horizontal/Vertical) have no
            // CSS shorthand on their own; CallBackRange expands them into
            // per-cell single-side ApplyBorder calls. Returning a valid CSS
            // token keeps any incidental formatting safe.
            case tBorderOutside    : return("border");
            case tBorderInside     : return("border");
            case tBorderHorizontal : return("border");
            case tBorderVertical   : return("border");
        }
        tStringStream wStream;
        wStream << "Border(" << sBorder << ")..... Error !";
        return(wStream.str());
    }
    //=========================================================================
    tUndoSpreadSheet::tUndoSpreadSheet(tMode sMode) : tUndo(), m_Mode(sMode),m_Error(""),m_SequenceId(0),m_OperationId(0) {
        // Commands always snapshot. tApi::IsUndoActif() decides whether the
        // finished command stays on the undo stack (see tApi::Do).
        m_Mode.Set(t_IsUndoActif);
        // Capture current sequence ID for rebase
        m_SequenceId = 0;
        m_OperationId = 0;
        m_RebasePlan = tRebasePlan();
    }

    tUndoSpreadSheet::~tUndoSpreadSheet() {
    }

    void tUndoSpreadSheet::NormalizeSheet(tSheet** sSheet) {
        if (*sSheet==nullptr) *sSheet= tSpreadSheetContainer::Instance()->ActiveWorkBook()->ActiveSheet();
    }

    tBool DispatchSpreadSheetDo(tUndoRedoContainer* sContainer, tUndoSpreadSheet* sUndo) {
        if (sContainer == nullptr || sUndo == nullptr) {
            return(false);
        }
        if (!sUndo->Do()) {
            delete(sUndo);
            return(false);
        }
        return(sContainer->CommitDo(sUndo));
    }

    tBool DispatchSpreadSheetUndo(tUndoRedoContainer* sContainer) {
        if (sContainer == nullptr || sContainer->LastUndo() == nullptr) {
            return(false);
        }
        tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(sContainer->LastUndo());
        if (wUndo == nullptr) {
            return(sContainer->Undo());
        }
        if (!sContainer->PopUndoToRedoWithoutExecute()) {
            return(false);
        }
        return(wUndo->Undo());
    }

    tBool DispatchSpreadSheetRedo(tUndoRedoContainer* sContainer) {
        if (sContainer == nullptr || sContainer->LastRedo() == nullptr) {
            return(false);
        }
        tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(sContainer->LastRedo());
        if (wUndo == nullptr) {
            return(sContainer->Redo());
        }
        if (sContainer->PopRedoToUndoDeque() == nullptr) {
            return(false);
        }
        return(wUndo->Do());
    }

    void tUndoSpreadSheet::SequenceId(tSequenceId sSequenceId) { m_SequenceId = sSequenceId; }

    tSequenceId tUndoSpreadSheet::SequenceId() { return(m_SequenceId); }
    
    void tUndoSpreadSheet::OperationId(std::uint64_t sOperationId) { m_OperationId = sOperationId; }

    std::uint64_t tUndoSpreadSheet::OperationId() { return(m_OperationId); }

    // Json ===================================================================
    void tUndoSpreadSheet::Json(Writer<StringBuffer>* sWriter) {
        sWriter->Key(kJsonKeyUndo);
        sWriter->String(ClassName().c_str());
        sWriter->Key(kJsonKeyOp);
        sWriter->String(OperationName().c_str());
        // Serialize OperationId as string — JS JSON.parse corrupts uint64 numbers
        // above Number.MAX_SAFE_INTEGER, which breaks collab echo dedup (double Do).
        if (m_OperationId != 0) {
            sWriter->Key(kJsonKeyOperationId);
            sWriter->String(std::to_string(m_OperationId).c_str());
        }
    }
    
    void tUndoSpreadSheet::Json(const rapidjson::Value& sValue) {
        tString wOperationName=sValue[kJsonKeyOp].GetString();
        OperationName(wOperationName);
        // Deserialize OperationId if present (string preferred; legacy numeric tolerated)
        m_OperationId = 0;
        if (sValue.HasMember(kJsonKeyOperationId)) {
            const rapidjson::Value& wOpId = sValue[kJsonKeyOperationId];
            if (wOpId.IsString()) {
                try {
                    m_OperationId = static_cast<std::uint64_t>(std::stoull(wOpId.GetString()));
                } catch (...) {
                    m_OperationId = 0;
                }
            } else if (wOpId.IsUint64()) {
                m_OperationId = wOpId.GetUint64();
            } else if (wOpId.IsUint()) {
                m_OperationId = wOpId.GetUint();
            }
        }
    }

    tBool tUndoSpreadSheet::IsUndoActif() { return(m_Mode.Value(t_IsUndoActif)); }

    void tUndoSpreadSheet::IsUndoActif(tBool sIsUndo) {  
        if(sIsUndo) {
            m_Mode.Set(t_IsUndoActif);
        } else {
            m_Mode.Clear(t_IsUndoActif);
        }
    }   

    tBool tUndoSpreadSheet::IsJson() { return(m_Mode.Value(t_IsJson)); }

    void tUndoSpreadSheet::IsJson(tBool sIsJson) {
        if(sIsJson) {
            m_Mode.Set(t_IsJson);
        } else {
            m_Mode.Clear(t_IsJson);
        }
    }
  
    tBool tUndoSpreadSheet::Client() { return(m_Mode.Value(t_Client)); }

    void tUndoSpreadSheet::Client(tBool sClient) {
        if (sClient) {
            m_Mode.Set(t_Client);
        } else {
            m_Mode.Clear(t_Client);
        }
    }

    tBool tUndoSpreadSheet::Server() { return(m_Mode.Value(t_Server)); }

    void tUndoSpreadSheet::Server(tBool sServer) {
        if (sServer) {
            m_Mode.Set(t_Server);
        } else {
            m_Mode.Clear(t_Server);
        }
    }

    void tUndoSpreadSheet::IsDo(tBool sIsDo) {
        if (sIsDo) {
            m_Mode.Set(t_IsDo);
        } else {
            m_Mode.Clear(t_IsDo);
        }
    }   

    tBool tUndoSpreadSheet::IsDo() { return(m_Mode.Value(t_IsDo)); }

    void tUndoSpreadSheet::IsUndo(tBool sIsUndo) {
        if (sIsUndo) {
            m_Mode.Set(t_IsUndo);
        } else {
            m_Mode.Clear(t_IsUndo);
        }
    }

    tBool tUndoSpreadSheet::IsUndo() { return(m_Mode.Value(t_IsUndo)); }

    void tUndoSpreadSheet::IsRedo(tBool sIsRedo) {
        if (sIsRedo) {
            m_Mode.Set(t_IsRedo);
        } else {
            m_Mode.Clear(t_IsRedo);
        }
    }

    tBool tUndoSpreadSheet::IsRedo() { return(m_Mode.Value(t_IsRedo)); }
    

    tBool tUndoSpreadSheet::IsError() { return(m_Mode.Value(t_IsError)); }

    void tUndoSpreadSheet::Error(tString sError) { m_Error = sError; m_Mode.Set(t_IsError); }
    tString tUndoSpreadSheet::Error() { return(m_Error); }

    tSheet* tUndoSpreadSheet::Sheet() { return(nullptr); }
 
    void tUndoSpreadSheet::SetRebasePlan() {
        tSheet* wSheet=Sheet();
        if (wSheet!=nullptr) {
            tWorkBook* wWorkBook = wSheet->WorkBook();
            tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
            tSequenceId wSequenceId = SequenceId();
            if (wCurrentSequence > wSequenceId) {
                // Build plan and assign directly - now that duplicate member is removed, this works
                m_RebasePlan = wWorkBook->UndoRebaseLog().BuildPlan(wSequenceId,
                                                                    wCurrentSequence,
                                                                    wSheet->AllocatorRef()
                                                                    );
            } else {
                // No rebase needed, clear the plan
                m_RebasePlan.Clear();
            }
        } else {
            // Sheet is nullptr, clear the plan
            m_RebasePlan.Clear();
            tStringStream wStream;
            wStream << ClassName() << "::SetRebasePlan() << Sheet==nullptr";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
    }

    tBool tUndoSpreadSheet::TestSheetExists(tWorkBook* sWorkBook, tString sName) {
        tSheet* wSheet = sWorkBook->Sheet(sName);
        // Test If Sheet exist
        if (wSheet==nullptr) {
            tStringStream wStream;
            wStream << "Sheet " << sName << " don't exist !";
            Error(wStream.str());
            return(false);
        }
        return(true);
    }
    tBool tUndoSpreadSheet::TestSheetAlreadyExists(tWorkBook* sWorkBook,tString sName) {
        tSheet* wSheet = sWorkBook->Sheet(sName);
        // Test If Sheet exist
        if (wSheet!=nullptr) {
            tStringStream wStream;
            wStream << "Sheet " << sName << " already exist !";
            Error(wStream.str());
            return(false);
        }
        return(true);
    }

    tWorkBook* tUndoSpreadSheet::WorkBook() {
        if (m_WorkBookTarget() != "") {
            tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->FindWorkBook(m_WorkBookTarget());
            if (wWorkBook != nullptr) {
                return(wWorkBook);
            }
        }
        tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
#ifdef TestMultiUser
        if (wWorkBook != nullptr) {
            m_WorkBookTarget = wWorkBook->Uri();
        }
#endif
        return(wWorkBook);
    }

    tString tUndoSpreadSheet::WriteJson() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        Json(&wWriter);
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    void tUndoSpreadSheet::ReadJson(tString sJson) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        Json(wDocument);
    }

    tSaveSelect* tUndoSpreadSheet::SaveSelect() { return(nullptr); }

    void tUndoSpreadSheet::DebugFlags(tString sTitle) {
        cout << sTitle << " Flags " << ClassName() << endl;
        if (m_Mode.Value(t_IsUndoActif)) cout << "t_IsUndoActif" << endl;
        if (m_Mode.Value(t_IsDo)) cout << "t_IsDo" << endl;
        if (m_Mode.Value(t_IsRedo)) cout << "t_IsRedo" << endl;
        if (m_Mode.Value(t_IsUndo)) cout << "t_IsUndo" << endl;
        if (m_Mode.Value(t_IsJson)) cout << "t_IsJson" << endl;
        if (m_Mode.Value(t_Client)) cout << "t_Client" << endl;
        if (m_Mode.Value(t_Server)) cout << "t_Server" << endl;
        if (m_Mode.Value(t_IsError)) cout << "t_IsError" << endl;
        cout << "--------------------------------" << endl;
    }

    void tUndoSpreadSheet::WorkBookTarget(tString sWorkBookTarget) {
        m_WorkBookTarget=sWorkBookTarget;
    }


    //=========================================================================
    //! Undo to change the size of row or columns
    //=========================================================================
	tUndoChangeSize::tUndoChangeSize() : tUndoSpreadSheet(tMode()), m_IsRow(false), m_Begin(0), m_End(0),m_BeginRebase(0),m_EndRebase(0), m_Size(0), m_Sheet(nullptr) {
	}

	tUndoChangeSize::tUndoChangeSize(tBool sIsRow, tIndex sBegin,tIndex sEnd, tDouble sSize,tMode sMode,tSheet* sSheet) :
		tUndoSpreadSheet(sMode),
		m_IsRow(sIsRow),
		m_Begin(sBegin),
        m_End(sEnd),
        m_BeginRebase(sBegin),
        m_EndRebase(sEnd),
		m_Size(sSize),
        
		m_Sheet(sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Change size ";
            if (m_IsRow) {
                if (sBegin==sEnd) {
                    wStream << "row "  << sBegin << "("<< sSize << ")";
                } else {
                    wStream << "row "  << sBegin << ":" << sEnd << "("<< sSize << ")";
                }
            } else {
                if (sBegin==sEnd) {
                    wStream << "column " << Base10ToAlpha(sBegin) << "("<< sSize << ")";
                } else {
                    wStream << "column " << Base10ToAlpha(sBegin) << ":" << Base10ToAlpha(sEnd) << "("<< sSize << ")";
                }
            }
            OperationName(wStream.str());
        }
	}

	tUndoChangeSize::~tUndoChangeSize() {
	};

    tString tUndoChangeSize::ClassName() const { return("tUndoChangeSize"); }

    tSaveSelect* tUndoChangeSize::SaveSelect() { return(nullptr); }

    tString tUndoChangeSize::RefRebase() { return(""); }

	tBool tUndoChangeSize::Do() {
		if (m_IsRow) {
            for(tIndex wInd = m_BeginRebase; wInd<= m_EndRebase; wInd++) {
                tDouble wLastSize=m_Sheet->SizeRow(wInd);
                if (IsUndoActif()) m_StackSize.push(wLastSize);
                m_Sheet->SizeRow(wInd, m_Size);
            }
        } else {
            for(tIndex wInd = m_BeginRebase; wInd<= m_EndRebase; wInd++) {
                tDouble wLastSize=m_Sheet->SizeCol(wInd);
                if (IsUndoActif()) m_StackSize.push(wLastSize);
                m_Sheet->SizeCol(wInd, m_Size);
            }
        }
		return(true);
	};

	tBool tUndoChangeSize::Undo()  {
        // Rebase is done by Rebase() method before Undo() is called
        // No need to rebase here to avoid double rebase
        // Copy sizes to restore without mutating the original stack
        std::stack<tDouble> wStackSizeCopy = m_StackSize;
        std::vector<tDouble> wSizes;
        wSizes.reserve(wStackSizeCopy.size());
        while (!wStackSizeCopy.empty()) {
            wSizes.push_back(wStackSizeCopy.top());
            wStackSizeCopy.pop();
        }
        std::reverse(wSizes.begin(), wSizes.end());

        const tIndex wCount = static_cast<tIndex>(wSizes.size());
        // Use rebased range for symmetry: when Redo calls Do() again, it uses m_BeginRebase/m_EndRebase
        const tIndex wRangeLength = (m_EndRebase >= m_BeginRebase) ? (m_EndRebase - m_BeginRebase + 1) : 0;
        if (wRangeLength != wCount) {
            return(false);
        }

        // Use rebased positions for symmetry with Do() which uses m_BeginRebase/m_EndRebase
        for (tIndex wOffset = 0; wOffset < wCount; ++wOffset) {
            const tIndex wInd = static_cast<tIndex>(m_BeginRebase + wOffset);
            const tDouble wSize = wSizes[wOffset];
            if (m_IsRow) {
                m_Sheet->SizeRow(wInd, wSize);
            } else {
                m_Sheet->SizeCol(wInd, wSize);
            }
        }
        
		return(true);
	};

    tSheet* tUndoChangeSize::Sheet() { return(m_Sheet); }

    tBool tUndoChangeSize::Rebase() {
        SetRebasePlan();
        
        m_BeginRebase=m_Begin;
        m_EndRebase=m_End;
     
        // Rebase the begin and end positions if structural operations occurred
        // But skip rebase if this undo comes from JSON (IsJson() == true)
        // because the coordinates in JSON already contain rebased values
        if (!m_RebasePlan.m_Operations.empty()) {
            if (m_IsRow) {
                auto wRebasedBegin = m_RebasePlan.RebaseRow(m_Begin);
                auto wRebasedEnd = m_RebasePlan.RebaseRow(m_End);
                if (!wRebasedBegin.has_value() || !wRebasedEnd.has_value()) {
                    return(false);
                }
                m_BeginRebase = wRebasedBegin.value();
                m_EndRebase = wRebasedEnd.value();
            } else {
                auto wRebasedBegin = m_RebasePlan.RebaseCol(m_Begin);
                auto wRebasedEnd = m_RebasePlan.RebaseCol(m_End);
                if (!wRebasedBegin.has_value() || !wRebasedEnd.has_value()) {
                    return(false);
                }
                m_BeginRebase = wRebasedBegin.value();
                m_EndRebase = wRebasedEnd.value();
            }
        }
        return(true);
    }

    void tUndoChangeSize::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeySheet);
        sWriter->String(m_Sheet->Name().c_str());
        sWriter->Key(kJsonKeyIsRow);
        sWriter->Bool(m_IsRow);
        sWriter->Key(kJsonKeyBegin);
        sWriter->Int(m_BeginRebase);
        sWriter->Key(kJsonKeyEnd);
        sWriter->Int(m_EndRebase);
        sWriter->Key(kJsonKeySize);
        sWriter->Double(m_Size);
        // Save only on undo
        if (IsUndo()) {
            sWriter->Key(kJsonKeyStackSize);
            sWriter->StartArray();
            // Copy the stack to avoid modifying the original
            // We use a vector to reverse the order for correct serialization
            auto stackCopy = m_StackSize;
            std::vector<tDouble> tempVec;
            while (!stackCopy.empty()) {
                tempVec.push_back(stackCopy.top());
                stackCopy.pop();
            }
            // Reverse to maintain the original stack order
            std::reverse(tempVec.begin(), tempVec.end());
            for (tDouble wSize : tempVec) {
                sWriter->Double(wSize);
            }
            sWriter->EndArray();
        }
    }

    void tUndoChangeSize::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        tString wSheetName=sValue[kJsonKeySheet].GetString();
        m_Sheet=tSpreadSheetContainer::Instance()->ActiveWorkBook()->Sheet(wSheetName);
        m_IsRow = sValue[kJsonKeyIsRow].GetBool();
        m_Begin = sValue[kJsonKeyBegin].GetInt();
        m_End = sValue[kJsonKeyEnd].GetInt();
        m_BeginRebase = m_Begin;
        m_EndRebase = m_End;
        m_Size = sValue[kJsonKeySize].GetDouble();
        if (sValue.HasMember(kJsonKeyStackSize)) {
            while(!m_StackSize.empty()) m_StackSize.pop();
            const rapidjson::Value& wStackSize = sValue[kJsonKeyStackSize];
            for (rapidjson::SizeType i = 0; i < wStackSize.Size(); i++) {
                m_StackSize.push(wStackSize[i].GetDouble());
            }
        }
    }

    void tUndoChangeSize::DeleteBeforeDo() {
        // Clear stack of saved sizes before Do
        while(!m_StackSize.empty()) {
            m_StackSize.pop();
        }
    }

    // To Init tUndoSpreadSheetCallBack
    tAllocatorRef SheetAllocator(tSheet* sSheet) {
    if (sSheet!=nullptr) {
        return(sSheet->AllocatorRef());
    }
    return(0);
    }
	//=========================================================================
	tUndoSpreadSheetCallBack::tUndoSpreadSheetCallBack() : tUndoSpreadSheet(tMode()),m_SheetAllocatorRef(0),m_SheetName(),m_Ref(),m_RefRebase(), m_State(tUndoState::BeforeDo) {
        m_SequenceId=Sheet()->WorkBook()->UndoRebaseLog().LatestSequence();
    }

	tUndoSpreadSheetCallBack::tUndoSpreadSheetCallBack(tAllocatorRef sSheetAllocator,tString sRef, tMode sMode) : tUndoSpreadSheet(sMode), m_SheetAllocatorRef(sSheetAllocator),m_SheetName(),m_Ref(sRef),m_RefRebase(sRef),m_State(tUndoState::BeforeDo) {
        tSheet* wSheet=Sheet();
        if (wSheet!=nullptr) {
            m_SheetName=tSharedString(wSheet->Name());
            m_SequenceId=Sheet()->WorkBook()->UndoRebaseLog().LatestSequence();
        }
    }

    tUndoSpreadSheetCallBack::tUndoSpreadSheetCallBack(tString sSheetName,tString sRef,tMode sMode) : tUndoSpreadSheet(sMode), m_SheetAllocatorRef(0),m_SheetName(sSheetName),m_Ref(sRef),m_RefRebase(sRef),m_State(tUndoState::BeforeDo) {}

	tUndoSpreadSheetCallBack::~tUndoSpreadSheetCallBack() {}

    tString tUndoSpreadSheetCallBack::ClassName() const { return("tUndoSpreadSheetCallBack"); }

	void tUndoSpreadSheetCallBack::UndoState(tUndoState sState) { m_State = sState; }
	tUndoState tUndoSpreadSheetCallBack::UndoState() { return(m_State); }

    tSheet* tUndoSpreadSheetCallBack::Sheet() {
        tWorkBook* wWorkBook=WorkBook();
        if (wWorkBook==nullptr) {
            return(nullptr);
        }
        return(wWorkBook->SheetByAllocator(m_SheetAllocatorRef));
    }

    void tUndoSpreadSheetCallBack::RebindActiveSheet() {
        tWorkBook* wWorkBook = WorkBook();
        if (wWorkBook == nullptr) {
            return;
        }
        if (!m_SheetName().empty()) {
            // Named sheet: only refresh when still listed (undo delete-sheet keeps allocator ref off-list).
            if (tSheet* wSheet = wWorkBook->Sheet(m_SheetName())) {
                m_SheetAllocatorRef = wSheet->AllocatorRef();
                m_SheetName = tSharedString(wSheet->Name());
            }
            return;
        }
        if (tSheet* wSheet = wWorkBook->ActiveSheet()) {
            m_SheetAllocatorRef = wSheet->AllocatorRef();
            m_SheetName = tSharedString(wSheet->Name());
        }
    }
     

	tBool tUndoSpreadSheetCallBack::CallBackCell(tTempoPoint* sPoint) { return(true); }
	tBool tUndoSpreadSheetCallBack::CallBackRange(tTempoRect* sRect) { return(true); }


    void tUndoSpreadSheetCallBack::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeySheet);
        sWriter->String(Sheet()->Name().c_str());
        sWriter->Key(kJsonKeyRef);
        sWriter->String(m_RefRebase.c_str());
    }

    void tUndoSpreadSheetCallBack::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        // Get Active WorkBook to find Sheet
        tSpreadSheetContainer* wSpreadSheetContainer=tSpreadSheetContainer::Instance();
        tWorkBook* wWorkBook=wSpreadSheetContainer->ActiveWorkBook();
        m_SheetName=sValue[kJsonKeySheet].GetString();
        tSheet* wSheet=wWorkBook->Sheet(m_SheetName());
        if (wSheet!=nullptr) {
            m_SheetAllocatorRef=wSheet->AllocatorRef();
            wSpreadSheetContainer->ActiveWorkBook(wWorkBook);
            // Peer GetMessage: bind undo target sheet only — do not switch the viewer's active tab.
            if (!IsSystemSheetName(m_SheetName()) && !IsJson()) {
                wWorkBook->ActiveSheet(m_SheetName());
            }
        } else {
            // Delete Sheet
            m_SheetAllocatorRef=0;
        }
        if (sValue.HasMember(kJsonKeyRef)) {
            m_Ref = sValue[kJsonKeyRef].GetString();
            m_RefRebase=m_Ref;
        }
    }

    tSaveSelect* tUndoSpreadSheetCallBack::SaveSelect() { return(nullptr); }
    
    tString tUndoSpreadSheetCallBack::RefRebase() { return(m_RefRebase); }

    tBool tUndoSpreadSheetCallBack::Rebase() {
        SetRebasePlan();
        if (!m_RebasePlan.IsEmpty()) {
            tSelect* wTempoSelect = new tSelect();
            // SaveSelect cell coordinates are shifted in place; only rebase them once.
            const tBool wSaveSelectNeedsRebase = (m_RefRebase == m_Ref);
            // Only rebase m_RefRebase if there are operations to apply
            // Reset to original reference first to avoid double rebase
            m_RefRebase=m_Ref;
          
            if (
                
                !m_RebasePlan.m_Operations.empty()!=0) {
                wTempoSelect->Parse(m_Ref);
                if (wTempoSelect->Rebase(m_RebasePlan)) {
                    m_RefRebase = wTempoSelect->StrRef();
                } else {
                    return(false);
                }
            }
        
            tSaveSelect* wSaveSelect=SaveSelect();
            
            if (wSaveSelect==nullptr) {
#ifdef _debugleak
                delete(wTempoSelect);
#endif
                return(true);
            }
                
            if (wSaveSelectNeedsRebase) {
                // Important Select use temporary memory
                // Parse with original reference (m_Ref), then Rebase() will rebase m_SelectStr
                // Using m_RefRebase here would cause double rebase since SaveSelect::Rebase() also rebases m_SelectStr
                wSaveSelect->ParseSelect(m_Ref);
                
                // Rebase all save structures if there are operations to apply
                if (!wSaveSelect->Rebase(m_RebasePlan)) {
#ifdef _debugleak
                    delete(wTempoSelect);
#endif
                    return(false);
                }
            }
#ifdef _debugleak
            delete(wTempoSelect);
#endif
            return(true);
        } else {
            // No rebase needed, but ensure m_RefRebase is initialized by m_Ref
            m_RefRebase = m_Ref;
        }
        return(true);
    }

    void tUndoSpreadSheetCallBack::DeleteBeforeDo() {
        // Base implementation - clear reference rebase
        m_RefRebase = m_Ref;
    }

    void tUndoSpreadSheetCallBack::Ref(tString sRef) {
        m_Ref = sRef;
        m_RefRebase = sRef;
    }

    void tUndoSpreadSheetCallBack::SheetName(tString sSheetName) {
        m_SheetName = sSheetName;
        
    }


#ifdef _DEBUGSK
    tString tUndoSpreadSheetCallBack::Debug() {
        tStringStream wStream;
        wStream << "Undo :" << ClassName() << endl;
        return(wStream.str());
    };
#endif
    //=========================================================================
    //! Undo to apply JSON to a cell
    tUndoJsonPayload::tUndoJsonPayload() : tUndoSpreadSheetCallBack(), m_JsonPayload("") {}


    tUndoJsonPayload::tUndoJsonPayload(tString sRef,tString sJson,tMode sMode,tSheet* sSheet) : tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), sRef, sMode), m_JsonPayload(sJson) {
    }

    tUndoJsonPayload::~tUndoJsonPayload() {
        m_MapSaveJsonPayload.clear();
    }


    tString tUndoJsonPayload::ClassName() const { return("tUndoJsonPayload"); }


    tBool tUndoJsonPayload::CallBackCell(tTempoPoint* sPoint) {
        // Get Sheet By Alocator
        tSheet* wSheet=Sheet();
#ifdef debugundoredo
        tString wState;

        switch (UndoState()) {
        case tUndoState::BeforeDo: wState = "BeforeDo";  break;
        case tUndoState::Do: wState = "Do";  break;
        case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
        case tUndoState::Undo: wState = "Undo";  break;
        }
        cout << "tUndoJsonPayload::" << wState << " CallBackCell " <<  sPoint->StrRef() << endl;
#endif
        switch (UndoState()) {
        case tUndoState::BeforeDo: {
            break;
        }
        case tUndoState::Do: {
            tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
            tPoint wPoint;
            wPoint.Row(sPoint->Row());
            wPoint.Col(sPoint->Col());
            tString wJsonPayload;
            if (wColRowCellRange->GetCellJsonPayload(wPoint.Row(),wPoint.Col(),wJsonPayload)) {
                m_MapSaveJsonPayload[wPoint]=wJsonPayload;
            }
            // On tiem
            if (!m_ContainerCellDo.Exist(tPoint(wPoint.Row(),wPoint.Col()))) {
                // Ensure Cell
                wColRowCellRange->EnsureCell(wPoint.Row(),wPoint.Col());
                // Place New
                wColRowCellRange->SetCellJsonPayload(wPoint.Row(),wPoint.Col(),m_JsonPayload);
            }
            break;
        }
        case tUndoState::BeforeUndo: {
            break;
        }
        case tUndoState::Undo: {
             break;
        }
        }
        return(true);
    }

    tBool tUndoJsonPayload::Do() {
        // Important Select use temporary memory
        // We have to parse again every time

         tSelect wSelect;
        tBool wOk=wSelect.Parse(m_RefRebase);

        if (wOk) {
            UndoState(tUndoState::Do);
            (void)wSelect.CallBack(this);
        }

        return(wOk);
    }

    tBool tUndoJsonPayload::Undo() {
        // Recup Sheet
        tSheet* wSheet=Sheet();
        for(auto wMapRef : m_MapSaveJsonPayload) {
            tPoint wPoint=wMapRef.first;
            tCell* wCell=wSheet->EnsureCell(wPoint.Row(), wPoint.Col());
            if (wCell!=nullptr) {
                wSheet->ColRowCellRange()->SetCellJsonPayload(wPoint.Row(),wPoint.Col(),wMapRef.second);
            }
        }
        m_MapSaveJsonPayload.clear();
        return(true);
    }

    void tUndoJsonPayload::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyJsonPayload);
        sWriter->String(m_JsonPayload.c_str());
        if (IsUndo()) {
            sWriter->Key(kJsonKeyMapSaveJsonPayLoad);
            sWriter->StartArray();
            for(auto wMapRef : m_MapSaveJsonPayload) {
                tPoint wPoint=wMapRef.first;
                sWriter->StartObject();
                sWriter->Key(kJsonKeyCell);
                sWriter->String(wPoint.StrRef().c_str());
                sWriter->Key(kJsonKeyJsonPayload);
                sWriter->String(wMapRef.second.c_str());
                sWriter->EndObject();
            }
            sWriter->EndArray();
        }
    }

    void tUndoJsonPayload::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        m_JsonPayload = sValue[kJsonKeyJsonPayload].GetString();
        if (sValue.HasMember(kJsonKeyMapSaveJsonPayLoad)) {
            const rapidjson::Value& wMapSaveJsonPayload = sValue[kJsonKeyMapSaveJsonPayLoad];
            for(rapidjson::SizeType i = 0; i < wMapSaveJsonPayload.Size(); i++) {
                tString wRef=wMapSaveJsonPayload[i][kJsonKeyCell].GetString();
                tTempoPoint wTempoPoint;
                wTempoPoint.ParseRef(wRef);
                const tPoint wPoint(wTempoPoint);
                tString wJsonPayload=wMapSaveJsonPayload[i][kJsonKeyJsonPayload].GetString();
                m_MapSaveJsonPayload[wPoint] = wJsonPayload;
            }
        }
    }

    tBool tUndoJsonPayload::Rebase() {
        // Call parent Rebase to rebase m_RefRebase and m_SaveSelect
        if (!tUndoSpreadSheetCallBack::Rebase()) {
            return false;
        }
        
        // Rebase m_MapSaveJsonPayload if there are structural operations
        tSheet* wSheet = Sheet();
        if (wSheet != nullptr && !m_MapSaveJsonPayload.empty()) {
            tWorkBook* wWorkBook = wSheet->WorkBook();
            tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
            tSequenceId wSequenceId = SequenceId();
            
            if (wCurrentSequence > wSequenceId) {
                SetRebasePlan();
                if (!m_RebasePlan.m_Operations.empty()) {
                    // Cannot modify map keys directly, so rebuild the map
                    // Store original data temporarily
                    std::vector<std::pair<tPoint, tString>> wOriginalData;
                    for (auto wPair : m_MapSaveJsonPayload) {
                        wOriginalData.push_back({wPair.first, wPair.second});
                    }
                    
                    // Clear and rebuild with rebased points
                    m_MapSaveJsonPayload.clear();
                    for (auto wPair : wOriginalData) {
                        tPoint wOriginalPoint = wPair.first;
                        auto wRebasedPoint = m_RebasePlan.RebasePoint(wOriginalPoint);
                        if (!wRebasedPoint.has_value()) {
                            // Point was deleted, cannot rebase
                            return false;
                        }
                        // Insert with rebased point as key
                        m_MapSaveJsonPayload[wRebasedPoint.value()] = wPair.second;
                    }
                }
            }
        }
        return true;
    }

    void tUndoJsonPayload::DeleteBeforeDo() {
        // Call parent to reset reference rebase
        tUndoSpreadSheetCallBack::DeleteBeforeDo();
        // Clear map of saved JSON payloads before Do
        m_MapSaveJsonPayload.clear();
        m_ContainerCellDo.Container()->clear();
    }

    //=========================================================================
    tUndoApplyMerge::tUndoApplyMerge() : tUndoSpreadSheetCallBack(0, "", tMode()), m_Select(nullptr), m_Erase(false) {
    }

    tUndoApplyMerge::tUndoApplyMerge(tString sRef,tMode sMode, tSheet* sSheet) :tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), sRef, sMode), m_Erase(false) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Merge " << sRef;
            OperationName(wStream.str());
        }
    }

    tUndoApplyMerge::~tUndoApplyMerge() {};

    tString tUndoApplyMerge::ClassName() const { return("tUndoApplyMerge"); }

    tBool tUndoApplyMerge::CallBackRange(tTempoRect* sRect) {
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        switch (UndoState()) {
            case tUndoState::BeforeDo: break;
            case tUndoState::BeforeUndo: break;
            case tUndoState::Do: {
                // Flip Flap: Do and Undo both route here. We determine the
                // actual direction by checking whether the exact rect is
                // already merged.
                tRange* wRange=wSheet->Range(sRect->Top(),sRect->Left(), sRect->Bottom(), sRect->Right());
                tBool wIsUnmerging = (wRange!=nullptr) && wRange->IsMerged();

                if (wIsUnmerging) {
                    // --- Reverse side of the flip-flap: unmerge the new range
                    //     then restore any merges that were absorbed by the
                    //     original Do (Excel-compatible undo).
                    wRange->RemoveMerged();
                    if (wRange->IsEmpty()) {
                      wSheet->EraseRange(sRect->Top(),sRect->Left(), sRect->Bottom(), sRect->Right());
                    }
                    m_Erase=true;

                    for (auto& wAbsorbed : m_AbsorbedMerges) {
                        tRange* wRestore = wSheet->Range(wAbsorbed.Top(),  wAbsorbed.Left(),
                                                         wAbsorbed.Bottom(),wAbsorbed.Right());
                        if (wRestore == nullptr) {
                            wRestore = wSheet->EnsureRange(wAbsorbed.Top(),  wAbsorbed.Left(),
                                                           wAbsorbed.Bottom(),wAbsorbed.Right());
                        }
                        if (wRestore != nullptr) wRestore->SetMerged();
                    }
                    // Absorbed list is rebuilt on the next Do/Redo.
                    m_AbsorbedMerges.clear();
                } else {
                    // --- Forward side of the flip-flap: absorb every
                    //     overlapping merge (Excel refuses to keep nested
                    //     merges), then apply the new merge.
                    tRect wNewRect(sRect->Top(), sRect->Left(),
                                   sRect->Bottom(), sRect->Right());
                    tVectorRange wOverlap;
                    wSheet->FindRangesCovered(wNewRect, &wOverlap);
                    m_AbsorbedMerges.clear();
                    for (tRange* wMerged : wOverlap) {
                        // FindRangesCovered already filters on IsMerged().
                        // Skip an exact match on sRect (will be handled
                        // below via the SetMerged on wRange).
                        tIndex wMT = wMerged->Top()->Index();
                        tIndex wML = wMerged->Left()->Index();
                        tIndex wMB = wMerged->Bottom()->Index();
                        tIndex wMR = wMerged->Right()->Index();
                        if (wMT == sRect->Top()  && wML == sRect->Left() &&
                            wMB == sRect->Bottom() && wMR == sRect->Right()) {
                            continue;
                        }
                        m_AbsorbedMerges.push_back(tRect(wMT, wML, wMB, wMR));
                        wMerged->RemoveMerged();
                        if (wMerged->IsEmpty()) {
                            wSheet->EraseRange(wMT, wML, wMB, wMR);
                        }
                    }

                    if (wRange != nullptr) {
                        wRange->SetMerged();
                    } else {
                        tRange* wNewRange = wSheet->EnsureRange(sRect->Top(), sRect->Left(),
                                                                sRect->Bottom(), sRect->Right());
                        wNewRange->SetMerged();
                    }
                }
                break;
            }
            case tUndoState::Undo: {
                break;
            }
        }
        return(true);
    }


    tBool tUndoApplyMerge::Do() {
        tBool wResult=false;
        
        // Important Select use temporary memory
        // We have to parse again every time
        m_Select = new tSelect();
        if (m_Select->Parse(m_RefRebase)) {
#ifdef debugundoredo
            cout << "tUndoApplyMerge::Do" << endl;
            cout << m_Select->Debug();
#endif
            UndoState(tUndoState::Do);
            wResult = m_Select->CallBack(this);
        }
#ifdef _debugleak
        delete(m_Select);
#endif
        return(wResult);
    };

    tBool tUndoApplyMerge::Undo()  {
        tBool wResult=true;
        // Important Select use temporary memory
        // We have to parse again every time
        m_Select = new tSelect();
        if (m_Select->Parse(m_RefRebase)) {
#ifdef debugundoredo
        cout << "tUndoApplyMerge::UndDo" << endl;
        cout << m_Select->Debug();
#endif
        // Flip Flap
         UndoState(tUndoState::Do);
            wResult = m_Select->CallBack(this);
        }
#ifdef _debugleak
        delete(m_Select);
#endif
        return(wResult);
    };

    void tUndoApplyMerge::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        // Persist the merges absorbed by this operation so Undo/Redo (and
        // network replay on other clients) can restore them.
        if (!m_AbsorbedMerges.empty()) {
            sWriter->Key("abm");
            sWriter->StartArray();
            for (auto& wRect : m_AbsorbedMerges) {
                sWriter->StartArray();
                sWriter->Int64(wRect.Top());
                sWriter->Int64(wRect.Left());
                sWriter->Int64(wRect.Bottom());
                sWriter->Int64(wRect.Right());
                sWriter->EndArray();
            }
            sWriter->EndArray();
        }
    }

    void tUndoApplyMerge::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        m_AbsorbedMerges.clear();
        if (sValue.HasMember("abm") && sValue["abm"].IsArray()) {
            const rapidjson::Value& wArray = sValue["abm"];
            for (rapidjson::SizeType i = 0; i < wArray.Size(); ++i) {
                const rapidjson::Value& wItem = wArray[i];
                if (wItem.IsArray() && wItem.Size() == 4) {
                    m_AbsorbedMerges.push_back(tRect(wItem[0].GetInt(),
                                                     wItem[1].GetInt(),
                                                     wItem[2].GetInt(),
                                                     wItem[3].GetInt()));
                }
            }
        }
    }

	//=========================================================================
    void tUndoRaz::CollectFullyContainedTables() {
        m_DeletedTables.clear();
        tSheet* wSheet = Sheet();
        tWorkBook* wWorkBook = wSheet != nullptr ? wSheet->WorkBook() : nullptr;
        if (wWorkBook == nullptr) {
            return;
        }
        std::vector<tTempoRect> wRazRects;
        if (!ParseSelectionRects(m_RefRebase, wRazRects)) {
            return;
        }
        tRangeNamedContainer* wContainer = wWorkBook->RangeNamedContainer();
        if (wContainer == nullptr) {
            return;
        }
        const std::vector<tString> wNames = wContainer->AllNames();
        for (const tString& wName : wNames) {
            tRangeData* wRangeData = wWorkBook->RangeData(wName);
            if (wRangeData == nullptr) {
                continue;
            }
            std::vector<tRange*> wRanges = wContainer->Ranges(wName);
            for (tRange* wRange : wRanges) {
                if (wRange == nullptr || wRange->Sheet() != wSheet) {
                    continue;
                }
                tBool wFullyInside = false;
                for (const tTempoRect& wRaz : wRazRects) {
                    if (TableExtentInsideRazRect(wRange, wRangeData, wRaz)) {
                        wFullyInside = true;
                        break;
                    }
                }
                if (!wFullyInside) {
                    continue;
                }
                tRazDeletedTable wSaved;
                wSaved.m_Name = wName;
                wSaved.m_Ref = wRange->StrRef();
                wSaved.m_RangeData = *wRangeData;
                m_DeletedTables.push_back(std::move(wSaved));
                break;
            }
        }
    }

    void tUndoRaz::DeleteCollectedTables() {
        tSheet* wSheet = Sheet();
        tWorkBook* wWorkBook = wSheet != nullptr ? wSheet->WorkBook() : nullptr;
        if (wWorkBook == nullptr) {
            return;
        }
        for (const tRazDeletedTable& wSaved : m_DeletedTables) {
            wWorkBook->DeleteRangeNamed(wSaved.m_Name);
        }
    }

    void tUndoRaz::RestoreDeletedTables() {
        tSheet* wSheet = Sheet();
        tWorkBook* wWorkBook = wSheet != nullptr ? wSheet->WorkBook() : nullptr;
        if (wWorkBook == nullptr) {
            return;
        }
        for (const tRazDeletedTable& wSaved : m_DeletedTables) {
            tTempoRect wRect(wSaved.m_Ref);
            if (!wRect.IsValid()) {
                continue;
            }
            wWorkBook->DeleteRangeNamed(wSaved.m_Name);
            wWorkBook->InsertRangeData(wSaved.m_Name, wSaved.m_RangeData, wRect, wSheet);
        }
    }

    void tUndoRaz::JsonDeletedTables(Writer<StringBuffer>* sWriter) const {
        if (m_DeletedTables.empty()) {
            return;
        }
        sWriter->Key(kJsonKeyRazDeletedTables);
        sWriter->StartArray();
        for (const tRazDeletedTable& wSaved : m_DeletedTables) {
            sWriter->StartObject();
            sWriter->Key(kJsonKeyName);
            sWriter->String(wSaved.m_Name.c_str());
            sWriter->Key(kJsonKeyRange);
            sWriter->String(wSaved.m_Ref.c_str());
            tRangeData wRangeDataCopy(wSaved.m_RangeData);
            if (!wRangeDataCopy.IsEmpty()) {
                sWriter->Key(kJsonKeyData);
                wRangeDataCopy.Json(sWriter);
            }
            sWriter->EndObject();
        }
        sWriter->EndArray();
    }

    void tUndoRaz::JsonDeletedTables(const rapidjson::Value& sValue) {
        m_DeletedTables.clear();
        if (!sValue.HasMember(kJsonKeyRazDeletedTables)) {
            return;
        }
        const rapidjson::Value& wArray = sValue[kJsonKeyRazDeletedTables];
        if (!wArray.IsArray()) {
            return;
        }
        for (rapidjson::SizeType wI = 0; wI < wArray.Size(); ++wI) {
            const rapidjson::Value& wEntry = wArray[wI];
            if (!wEntry.IsObject()) {
                continue;
            }
            tRazDeletedTable wSaved;
            if (wEntry.HasMember(kJsonKeyName)) {
                wSaved.m_Name = wEntry[kJsonKeyName].GetString();
            }
            if (wEntry.HasMember(kJsonKeyRange)) {
                wSaved.m_Ref = wEntry[kJsonKeyRange].GetString();
            }
            if (wEntry.HasMember(kJsonKeyData)) {
                wSaved.m_RangeData.Json(wEntry[kJsonKeyData]);
            }
            if (!wSaved.m_Name.empty() && !wSaved.m_Ref.empty()) {
                m_DeletedTables.push_back(std::move(wSaved));
            }
        }
    }

	tUndoRaz::tUndoRaz() : tUndoSpreadSheetCallBack(0, "", tMode()), m_KeepFormat(false), m_FormatOnly(false), m_SaveSelectColRow_Rows(true), m_SaveSelectColRow_Cols(false), m_CssSheet(0) {
        // Anchor UndoSpreadSheet to
        m_SaveSelect.UndoSpreadSheet(this);
        m_SaveSelectColRow_Rows.UndoSpreadSheet(this);
        m_SaveSelectColRow_Cols.UndoSpreadSheet(this);
	}

	tUndoRaz::tUndoRaz(tString sRef,tMode sMode, tSheet* sSheet) : tUndoSpreadSheetCallBack(SheetAllocator(sSheet), sRef, sMode), m_KeepFormat(false), m_FormatOnly(false), m_SaveSelectColRow_Rows(true), m_SaveSelectColRow_Cols(false), m_CssSheet(0) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Raz " << sRef;
            OperationName(wStream.str());
        }
        // Anchor UndoSpreadSheet to
        m_SaveSelect.UndoSpreadSheet(this);
        m_SaveSelectColRow_Rows.UndoSpreadSheet(this);
        m_SaveSelectColRow_Cols.UndoSpreadSheet(this);
	}

	/// @brief destructor of SkUndoRaz.
	tUndoRaz::~tUndoRaz() {
        m_SaveSelect.Clear();
    }

    tString tUndoRaz::ClassName() const { return("tUndoRaz"); }

    tSaveSelect* tUndoRaz::SaveSelect() { return(&m_SaveSelect); }

    void tUndoRaz::KeepFormat(tBool sKeepFormat) {
        m_KeepFormat = sKeepFormat;
        // Propagate so UndoCell does not touch the CSS on undo/redo
        m_SaveSelect.KeepFormat(sKeepFormat);
    }

    tBool tUndoRaz::KeepFormat() const { return(m_KeepFormat); }

    void tUndoRaz::FormatOnly(tBool sFormatOnly) {
        m_FormatOnly = sFormatOnly;
        // Propagate so UndoCell restores CSS only and leaves content/class untouched.
        m_SaveSelect.FormatOnly(sFormatOnly);
    }

    tBool tUndoRaz::FormatOnly() const { return(m_FormatOnly); }

	tBool tUndoRaz::CallBackCell(tTempoPoint* sPoint) {
        // Get Sheet By Alocator
        tSheet* wSheet=Sheet();
#ifdef debugundoredo
		tString wState;

		switch (UndoState()) {
		case tUndoState::BeforeDo: wState = "BeforeDo";  break;
		case tUndoState::Do: wState = "Do";  break;
		case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
		case tUndoState::Undo: wState = "Undo";  break;
		}
		cout << "tUndoRaz::" << wState << " CallBackCell " <<  sPoint->StrRef() << endl;
#endif
		switch (UndoState()) {
		case tUndoState::BeforeDo: {
			tCell* wCell =wSheet->Cell(sPoint->Row(), sPoint->Col());
			if (wCell != nullptr) {
                // Add Cell just one time
                if (m_SaveSelect.FindCell(wCell)==nullptr) {
#ifdef debugundoredo
                    cout << "BeforeDo Save Cell Css=" << wCell->Css() << endl;
#endif
                    tSaveCell* wSaveCell=m_SaveSelect.AddCell(wCell);
                    // Pass Format to save don't inc()
                    // In KeepFormat mode we clear content only: keep CSS on the live cell.
                    if (!m_KeepFormat && IsUndoActif() && !IsJson()) {
#ifdef debugundoredo
                        cout << " ---->Pass Css=" << wCell->Css() << endl;
#endif
                        wSaveCell->PassCss(wCell);
                    }
                    // Overwriting a spill slave: snapshot the whole spill for undo restore.
                    if (!m_FormatOnly && wCell->IsMatExtend()) {
                        std::vector<tCell*> wSpillCells;
                        wCell->AppendCellsInSpillRange(wSpillCells);
                        for (tCell* wSpillCell : wSpillCells) {
                            if (wSpillCell != nullptr) {
                                m_SaveSelect.AddCell(wSpillCell);
                            }
                        }
                    }
                    // If Class save attribute ====================================
                    // Format-only clear keeps class attributes untouched (content).
                    tCellClassAttribute* wCellClass=wCell->ClassAttribute();
                    if (!m_FormatOnly && wCellClass !=nullptr) {
                        // Loop on attributes =====================================
                        // only attribute are erased
                        tVectorCellAttribute wVectorCellAttribute;
                        wCellClass->CellAttribute(&wVectorCellAttribute);
                        for(auto wCellAttribute : wVectorCellAttribute) {
#ifdef debugundoredo
                            cout << " Erase attribute " << wCellAttribute->StrRef() <<  "=" << wCellAttribute->FormulaStr() << ":" << wCellAttribute->Value() << endl;
#endif
                            m_SaveSelect.AddCell(wCellAttribute);
                            m_SaveSelect.TreatsExternalDependencyCells(wCellAttribute);
                        }
                        wSheet->DeleteCellClassAttributeContainer(wCell);
                    }
                }
            }
			break;
		}
		case tUndoState::Do: {
			tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
			if (wCell != nullptr) {
#ifdef debugundoredo
                cout << "Do  " << wCell->StrRef(true) << ":" << wCell->FormulaStr() << "=" << wCell->Value() << " Css="  <<  wCell->Css() << endl;
#endif
                // Format-only clear keeps the cell content (formula/variant).
                if (!m_FormatOnly) {
                    tCell* wSpillOrigin = nullptr;
                    if (wCell->IsMatExtend()) {
                        wSpillOrigin = wCell->CellMatrixRoot();
                    }
                    wCell->ClearFormulaAndVariant();
                    // Excel: after tearing down a spill slave, recompute origin → #SPILL!.
                    if (wSpillOrigin != nullptr && wSpillOrigin != wCell) {
                        m_SaveSelect.PushCalculate(wSpillOrigin);
                    }
                }
                //If not Pass Format in state  tUndoState::BeforeDo ============
                // In KeepFormat mode the format (CSS) is kept: only content is cleared.
                if (!m_KeepFormat && wCell->Css()!=0) {
                    //If not Pass Format in Save
                    wCell->WorkBook()->DeleteCellFormat(wCell->Css());
                    wCell->Css(0);
                }
                // Keep if not empty ===========================================
                // Format-only never removes the cell: its content is preserved.
                if (!m_FormatOnly && wCell->IsEmpty()) {
                    wSheet->DeleteCell(sPoint->Row(), sPoint->Col());
                }
			}
			break;
		}
		case tUndoState::BeforeUndo: {
            // Erase Cell Class  & Attribute
            // Format-only clear preserves class attributes across undo.
            tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr) {
                if (!m_FormatOnly && wCell->ClassAttribute()!=nullptr) {
                    wSheet->DeleteCellClassAttributeContainer(wCell);
                }
                
            }
			break;
		}
		case tUndoState::Undo: {
            tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(wSheet->IndexAllocatorColRowCellRange(),tTypeItem::t_Cell, sPoint->Row(), sPoint->Col(),"");
#ifdef debugundoredo
            cout << "Undo  UndoCell " << Base10ToAlpha(sPoint->Col()) << sPoint->Row();
            if (wSaveCell==nullptr) cout << " wSaveCell(nullptr)";
#endif
            // Recup Save Cell
            if (wSaveCell != nullptr) {
                m_SaveSelect.UndoCell(wSaveCell);
            } else {
                // Erase
                // Use current cell from sheet to preserve CSS (don't use wSaveCell->Cell() which might be a rebased cell without CSS)
                // ClearFormula() and CellValue() don't modify CSS, so CSS is automatically preserved
                tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
                if (wCell!=nullptr) {
                    // Erase Cell ==============================================
    #ifdef debugundoredo
                    cout << " SaveCell(nullptr) " << "=[" << wCell->FormulaStr() << "]";
    #endif
                    wCell->ClearFormulaAndVariant();
                    // Delete Format (kept in KeepFormat mode: content-only clear)
                    if (!m_KeepFormat && wCell->Css()!=0) {
                        tFormatApi* wFormatApi=wSheet->WorkBook()->FormatApi();
                        // Css != 0 wFormatApi != nullptr
                        wFormatApi->DeleteCellFormat(wCell->Css());
                        wCell->Css(0);
                    }
                    if (wCell->IsEmpty()) {
                       // Delete Cell
                       wSheet->DeleteCell(sPoint->Row(), sPoint->Col());
                   }
                }
            }

     		break;
		}
		}
		return(true);
	}

	tBool tUndoRaz::CallBackRange(tTempoRect* sRect) {
#ifdef debugundoredo
		cout << "tUndoRaz::CallBackRange " << sRect->StrRef() << endl;
#endif
		// A whole sheet/row/column format lives ONLY on tSheet / tColRow, never
		// on individual cells. So a format-only clear must touch just that level
		// and return immediately: a column spans ~1M rows, iterating every cell
		// here would be catastrophic (same reason tUndoFormat returns early too).
		if (m_FormatOnly && (sRect->IsSheetSelect() || sRect->IsRowSelect() || sRect->IsColSelect())) {
			tSheet* wSheet = Sheet();
			switch (UndoState()) {
			case tUndoState::BeforeDo: {
				if (sRect->IsSheetSelect()) {
					m_CssSheet = wSheet->Css();
				} else if (sRect->IsRowSelect()) {
					for (tIndex wRow = sRect->Top(); wRow <= sRect->Bottom(); wRow++) {
						tColRow* wColRow = wSheet->Row(wRow);
						if (wColRow != nullptr)
							m_SaveSelectColRow_Rows.AddColRow(wColRow, wSheet->ColRowCellRange(), true);
					}
				} else {
					for (tIndex wCol = sRect->Left(); wCol <= sRect->Right(); wCol++) {
						tColRow* wColRow = wSheet->Col(wCol);
						if (wColRow != nullptr)
							m_SaveSelectColRow_Cols.AddColRow(wColRow, wSheet->ColRowCellRange(), false);
					}
				}
				break;
			}
			case tUndoState::Do: {
				// Detach without DeleteCellFormat: the saved ref is kept for undo.
				if (sRect->IsSheetSelect()) {
					wSheet->Css(0);
				} else if (sRect->IsRowSelect()) {
					for (tIndex wRow = sRect->Top(); wRow <= sRect->Bottom(); wRow++) {
						tColRow* wColRow = wSheet->Row(wRow);
						if (wColRow != nullptr) wColRow->Css(0);
					}
				} else {
					for (tIndex wCol = sRect->Left(); wCol <= sRect->Right(); wCol++) {
						tColRow* wColRow = wSheet->Col(wCol);
						if (wColRow != nullptr) wColRow->Css(0);
					}
				}
				break;
			}
			default: break;
			}
			return(true);
		}
		for (tIndex wRow = sRect->Row(); wRow <= sRect->Bottom(); wRow++) {
			for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
				tTempoPoint wPoint(wRow, wCol);
				if (!CallBackCell(&wPoint)) return(false);
			}
		}
		return(true);
	}


	tBool tUndoRaz::Do() {
		tBool wResult = false;
        m_SaveSelect.Clear();
        // Reset sheet/row/column format saves so redo re-captures a clean state.
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.Clear();
        m_CssSheet = 0;
        
		// Important Select use temporary memory
		// We have to parse again every time
		if (m_SaveSelect.ParseSelect(m_RefRebase)) {
            CollectFullyContainedTables();
			UndoState(tUndoState::BeforeDo);
			wResult = m_SaveSelect.Select().CallBack(this);
    
			if (wResult) {
				UndoState(tUndoState::Do);
				wResult = m_SaveSelect.Select().CallBack(this);
                if (wResult) {
                    DeleteCollectedTables();
                } else {
                    m_DeletedTables.clear();
                }
			} else {
                m_DeletedTables.clear();
            }

            if (wResult) {
				CalculateDo();
			}
           
#ifdef debugundoredo
			cout << "tUndoRaz::Do" << endl;
            cout<< m_SaveSelect.Select().Debug();
#endif

#ifdef checksp
	 		Sheet()->Check();
#endif
		}
		return(wResult);
	}

	tBool tUndoRaz::Undo() {
		tBool wResult = true;
#ifdef debugundoredo
            cout << "tUndoRaz::UnDo" << endl;
        cout<< m_SaveSelect.Select().Debug();
#endif
        m_SaveSelect.UndoCells();
        // Restore sheet/row/column-level CSS detached by a format-only Do.
        if (m_FormatOnly) {
            tSheet* wSheet = Sheet();
            if (m_CssSheet != 0) {
                if (wSheet->Css() != 0) wSheet->WorkBook()->DeleteCellFormat(wSheet->Css());
                wSheet->Css(m_CssSheet);
                m_CssSheet = 0;
            }
            for (tSaveColRow* wSaveColRow : *m_SaveSelectColRow_Rows.VectorColRow()) {
                tColRow* wColRow = wSheet->Row(wSaveColRow->Index());
                if (wColRow != nullptr) {
                    if (wColRow->Css() != 0) wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                    wColRow->Css(wSaveColRow->Css());
                    wSaveColRow->Css(0);
                }
            }
            for (tSaveColRow* wSaveColRow : *m_SaveSelectColRow_Cols.VectorColRow()) {
                tColRow* wColRow = wSheet->Col(wSaveColRow->Index());
                if (wColRow != nullptr) {
                    if (wColRow->Css() != 0) wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                    wColRow->Css(wSaveColRow->Css());
                    wSaveColRow->Css(0);
                }
            }
            m_SaveSelectColRow_Rows.Clear();
            m_SaveSelectColRow_Cols.Clear();
        }
        RestoreDeletedTables();
        CalculateDo();
#ifdef checksp
		Sheet()->Check();
#endif

		return(wResult);
	}

    void tUndoRaz::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        // Content-only clear flag (kept format). Serialized so undo/redo and
        // collaboration replay keep the same scope.
        if (m_KeepFormat) {
            sWriter->Key(kJsonKeyKeepFormat);
            sWriter->Bool(m_KeepFormat);
        }
        // Format-only clear flag (kept content and class). Serialized so undo/redo
        // and collaboration replay keep the same scope.
        if (m_FormatOnly) {
            sWriter->Key(kJsonKeyFormatOnly);
            sWriter->Bool(m_FormatOnly);
        }
        // Save m_SaveSelect only on undo
        // m_SaveSelect is already rebased
        if (IsUndo()) {
            JsonDeletedTables(sWriter);
            sWriter->Key(kJsonKeySave);
            sWriter->StartObject();
            tSheet* wSheet=Sheet();
            m_SaveSelect.ColRowCellRange(wSheet->ColRowCellRange());
            // Save Cells
            m_SaveSelect.Json(sWriter,wSheet);
            sWriter->EndObject();
        }
    }

    void tUndoRaz::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        tSheet* wSheet=Sheet();
        if (sValue.HasMember(kJsonKeyKeepFormat)) {
            KeepFormat(sValue[kJsonKeyKeepFormat].GetBool());
        }
        if (sValue.HasMember(kJsonKeyFormatOnly)) {
            FormatOnly(sValue[kJsonKeyFormatOnly].GetBool());
        }
        JsonDeletedTables(sValue);
        if (sValue.HasMember(kJsonKeySave)) {
            m_SaveSelect.Json(sValue[kJsonKeySave],wSheet);
        }
    }

    void tUndoRaz::DeleteBeforeDo() {
        // Call parent to reset reference rebase
        tUndoSpreadSheetCallBack::DeleteBeforeDo();
        // Clear save select before Do
        m_SaveSelect.Clear();
        // Free any detached sheet/row/column CSS refs saved for a format-only Do.
        tSheet* wSheet = Sheet();
        if (wSheet != nullptr) {
            if (m_CssSheet != 0) {
                wSheet->WorkBook()->DeleteCellFormat(m_CssSheet);
                m_CssSheet = 0;
            }
            m_SaveSelectColRow_Rows.DeleteCellFormat(wSheet);
            m_SaveSelectColRow_Cols.DeleteCellFormat(wSheet);
        }
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.Clear();
        m_DeletedTables.clear();
    }
    
	void tUndoRaz::CalculateDo() {
		// A format-only clear changes CSS only (color, background, ...): it never
		// touches a value or formula, so nothing needs to be recomputed. Skipping
		// the recalc is both correct and essential for whole-sheet clears, where
		// driving a calculation over the selection would scan the entire grid.
		if (m_FormatOnly) return;
		m_SaveSelect.CalculateDo(Sheet()->ColRowCellRange(),tVolatile::t_None);
	}

#ifdef checkfo
    void tUndoRaz::IncCheckfo(tFormatApi* sFormatApi) {
        m_SaveSelect.IncCheckfo(sFormatApi);
        // A format-only Raz over a whole sheet/row/column detaches the CSS ref
        // from tSheet/tColRow and keeps it here for undo; count it too, otherwise
        // the consistency check (Real vs Check) mismatches and the ref is freed twice.
        if (m_CssSheet != 0) {
            sFormatApi->IncCheck(m_CssSheet);
        }
        m_SaveSelectColRow_Rows.IncCheckfo(sFormatApi);
        m_SaveSelectColRow_Cols.IncCheckfo(sFormatApi);
    }
#endif

#ifdef _DEBUGSK
    tString tUndoRaz::Debug() {
        tStringStream wStream;
        wStream << tUndoSpreadSheetCallBack::Debug();
        wStream << m_SaveSelect.Debug();
        return(wStream.str());
    }
#endif

	//=========================================================================
    tUndoCellValue::tUndoCellValue() : tUndoRaz("",tMode(),nullptr), m_Value(),m_ValueRebased() {
    }

    tUndoCellValue::tUndoCellValue(tString sRef, tMode sMode, tSheet* sSheet) : tUndoRaz(sRef,sMode,sSheet), m_Value(),m_ValueRebased() {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set " << sRef << ":" << m_Value;
            OperationName(wStream.str());
        }
    }

	//! Undo for apply value or formula to cell
    tUndoCellValue::tUndoCellValue(tString sRef, tVariant& sValue, tMode sMode, tSheet* sSheet) : tUndoRaz(sRef,sMode,sSheet), m_Value(sValue),m_ValueRebased(sValue){
		tStringStream wStream;
		wStream << "Set " << sRef << ":" << m_Value;
		OperationName(wStream.str());
	}

    tString  tUndoCellValue::ClassName() const { return("tUndoCellValue"); }

    tBool tUndoCellValue::CallBackCell(tTempoPoint* sPoint) {
        // If Error return false
        if (IsError()) return(false);
        // Get Sheet by Alllocator
        tSheet* wSheet=Sheet();
        
        tBool wOk = true;
#ifdef debugundoredo
		tString wState;

		switch (UndoState()) {
		case tUndoState::BeforeDo: wState = "BeforeDo";  break;
		case tUndoState::Do: wState = "Do";  break;
		case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
		case tUndoState::Undo: wState = "Undo";  break;
		}
		cout << "tUndoCell::" << wState << " CallBackCell " << sPoint->StrRef();
#endif

		switch (UndoState()) {
		case tUndoState::BeforeDo: {
            tCell* wCell =wSheet->Cell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr) {
                // Don't change  and save format
                m_SaveSelect.AddCell(wCell);
                // Typing into a spill slave: snapshot the whole spill footprint for undo.
                if (wCell->IsMatExtend()) {
                    std::vector<tCell*> wSpillCells;
                    wCell->AppendCellsInSpillRange(wSpillCells);
                    for (tCell* wSpillCell : wSpillCells) {
                        if (wSpillCell != nullptr) {
                            m_SaveSelect.AddCell(wSpillCell);
                        }
                    }
                }
                // If Cell Class Attribute Erase in container
                if (wCell->ClassAttribute()!=nullptr) {
                    wSheet->DeleteCellClassAttributeContainer(wCell);
                }
            }
			break;
		}
		case tUndoState::Do: {
			tCell* wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
            // Excel: overwrite of a spilled cell clears the spill, then origin recalcs to #SPILL!.
            tCell* wSpillOrigin = wCell->BreakSpillForUserOverwrite();
            wCell->ClearFormula();
			// If Class Set Error ============================================
			if (m_Value.Type() == tVariantType::t_class) {
				tStringStream wStream;
				wStream << "Error  On cell " << wCell->StrRef() << " SetValue with class  !";
                if (IsJson()) {
                    Error(wStream.str());
                    return(false);
                } else {
                    cerr << wStream.str() << endl;
                    throw(tExceptionInternalError(wStream.str()));
                }
			}
            // Intercept CellClass ============================================
            // Set Value in class ....
            tCellClass* wCellClass=wCell->Class();
            if ((wCellClass!=nullptr) && (!m_Value.HasFormula())) {
                wCellClass->Value(&m_Value);
                // If Cell Class Attribute Erase
                if (wCell->ClassAttribute()!=nullptr) {
                    wSheet->InsertCellClassAttributeContainer(wCell);
                }
            } else {
                // Debug Multi User
                //cout << "On " << wCell->WorkBook()->Uri() << "/" << wCell->StrRef(true) << "=" << m_Variant << endl;
                
                // Rebase formula only when structural ops happened since this undo was created.
                // Empty-plan Rebase still re-lexes and used to drop [] around LabelSquare
                // (e.g. Table2[[#This Row][F]] → Table2[#This Row][F]), breaking Redo.
                m_ValueRebased=m_Value;
                if (IsRedo() && (m_Value.HasFormula()) && !m_RebasePlan.IsEmpty()) {
                   
                    tString wFormula=m_ValueRebased.String();
                    tUndoRedoRebaseFormula wUndoRedoRebaseFormula(Sheet()->AllocatorRef(),sPoint->Row(), sPoint->Col(),wFormula);
                    if (wUndoRedoRebaseFormula.Rebase(m_RebasePlan)) {
                        // Update formula in variant
                        m_ValueRebased=wUndoRedoRebaseFormula.Formula();
                    }
                    // If rebase fails, keep the original wire formula rather than aborting Redo.
                }
                // Author first Do: compile under UI locale (FR → 1,187). Redo/Json: US wire.
                if (IsJson() || IsRedo()) {
                    wOk=CellValueFromWire(wSheet->WorkBook(), wCell, m_ValueRebased,false);
                } else {
                    wOk=wSheet->WorkBook()->CellValue(wCell, m_ValueRebased,false);
                    if (wOk && m_ValueRebased.HasFormula()) {
                        NormalizeUndoFormulaToWire(wCell, m_Value, m_ValueRebased);
                    }
                }
                if (wOk) {
                    // Undo Do clears formula then sets value; inverse dependents can be stale vs VectorRef (checksp Sheet::Check).
                    wCell->PruneStaleInverseDependents();
                }
            }
            if (wSpillOrigin != nullptr) {
                m_SaveSelect.PushCalculate(wSpillOrigin);
            }
        
			break;
		}
		case tUndoState::BeforeUndo: {
            // Erase Cell Class  & Attribute
            tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr) {
                if (wCell->ClassAttribute()!=nullptr) {
                    wSheet->DeleteCellClassAttributeContainer(wCell);
                }
                
            }
			break;
		}
		case tUndoState::Undo: {
			tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(wSheet->IndexAllocatorColRowCellRange(),tTypeItem::t_Cell, sPoint->Row(), sPoint->Col(),"");
#ifdef debugundoredo
			cout << "Undo  UndoCell " << Base10ToAlpha(sPoint->Col()) << sPoint->Row();
            if (wSaveCell==nullptr) cout << " wSaveCell(nullptr)";
#endif
            // If empty before erase ==========================================
			if (wSaveCell == nullptr) {
                // Erase Cell 
				tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
				if (wCell != nullptr) {
#ifdef debugundoredo
					cout << " SaveCell(nullptr) " << "=[" << wCell->FormulaStr() << "]"; 
#endif
            		wCell->ClearFormulaAndVariant();
                    if (wCell->IsEmpty()) {
                       // Delete Cell
                       wSheet->DeleteCell(sPoint->Row(), sPoint->Col());
                   }
				}
			}
			else {
				// Use current cell from sheet to preserve CSS (don't use wSaveCell->Cell() which might be a rebased cell without CSS)
				// ClearFormula() and CellValue() don't modify CSS, so CSS is automatically preserved
				tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
				if (wCell == nullptr) {
					// Cell doesn't exist in current sheet, create it using saved cell's EnsureCell
					// This ensures the cell is created in the correct sheet (after rebase)
					wCell = wSaveCell->EnsureCell();
				}
                if (wCell!=nullptr) {
#ifdef debugundoredo
                    cout << " SaveCell("<< wCell->StrRef() << ")" << "=" << *wSaveCell->Value() <<  " [" << wCell->FormulaStr() << "]";
#endif
                    // Must tear down spill (ClearMatrix) before restore. ClearFormula alone drops the
                    // origin formula/AFO but leaves MatExtend slaves populated — UI EnsureCell before
                    // edit always creates a SaveCell, so this path is the common case (A3==A1:B2).
                    wCell->ClearFormulaAndVariant();
                    CellValueFromWire(wSheet->WorkBook(), wCell, *wSaveCell->Value(),false);
                    // Matrix flags cleared by ClearFormulaAndVariant / CellValue — restore for spill undo.
                    wCell->Extension(wSaveCell->Extension());
                    // If Cell Class Attribute Add in Attribute Container
                    if (wCell->ClassAttribute()!=nullptr) {
                        wSheet->InsertCellClassAttributeContainer(wCell);
                    }
                }
			}
			break;
		}
		}
#ifdef debugundoredo
		cout  << endl;
#endif
		return(wOk);
	}

    tBool tUndoCellValue::Undo() {
        tBool wResult = true;
#ifdef debugundoredo
        cout << "tUndoRaz::UnDo" << endl;
        cout<< m_SaveSelect.Select().Debug();
#endif
        UndoState(tUndoState::BeforeUndo);
        wResult = m_SaveSelect.Select().CallBack(this);
        if (wResult) {
            // Class-safe restore of the selection (CallBackCell), then spill siblings
            // snapshotted in BeforeDo that lie outside m_Ref.
            UndoState(tUndoState::Undo);
            wResult = m_SaveSelect.Select().CallBack(this);
            if (wResult) {
                m_SaveSelect.UndoCellsOutsideSelect();
            }
       
            CalculateDo();
        }
    #ifdef checksp
        Sheet()->Check();
    #endif
        return(wResult);
    }

    void tUndoCellValue::Json(Writer<StringBuffer>* sWriter) {
        tUndoRaz::Json(sWriter);
        // Save variant (Rebased for formula)
        m_ValueRebased.Json(sWriter);
    }
    
    void tUndoCellValue::Json(const rapidjson::Value& sValue) {
        tUndoRaz::Json(sValue);
        //Read variant
        m_Value.Json(sValue);
    }

    //=========================================================================
    //! Undo fill-handle series (one undo for the whole extension range)
    tUndoFillSeries::tUndoFillSeries()
        : tUndoCellValue("", tMode(), nullptr), m_Values(), m_Row0(0), m_Col0(0), m_Width(1) {
    }

    tUndoFillSeries::tUndoFillSeries(tString sRef, const tVectorString& sValues, tMode sMode, tSheet* sSheet)
        : tUndoCellValue(sRef, sMode, sSheet), m_Values(sValues), m_Row0(0), m_Col0(0), m_Width(1) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Fill " << sRef << " (" << sValues.size() << ")";
            OperationName(wStream.str());
        }
    }

    tString tUndoFillSeries::ClassName() const { return("tUndoFillSeries"); }

    void tUndoFillSeries::ComputeRect() {
        tSelect wSelect;
        if (wSelect.Parse(m_RefRebase)) {
            if (tTempoRect* wRect = wSelect.FirstRect()) {
                m_Row0 = wRect->Row();
                m_Col0 = wRect->Col();
                m_Width = wRect->Right() - wRect->Col() + 1;
            } else if (tTempoPoint* wPoint = wSelect.FirstPoint()) {
                m_Row0 = wPoint->Row();
                m_Col0 = wPoint->Col();
                m_Width = 1;
            }
        }
        if (m_Width < 1) {
            m_Width = 1;
        }
    }

    tSize tUndoFillSeries::CellIndex(tTempoPoint* sPoint) const {
        tLong wRowOff = static_cast<tLong>(sPoint->Row()) - static_cast<tLong>(m_Row0);
        tLong wColOff = static_cast<tLong>(sPoint->Col()) - static_cast<tLong>(m_Col0);
        if (wRowOff < 0) {
            wRowOff = 0;
        }
        if (wColOff < 0) {
            wColOff = 0;
        }
        return static_cast<tSize>(wRowOff * static_cast<tLong>(m_Width) + wColOff);
    }

    tBool tUndoFillSeries::Do() {
        // Snapshot the destination rectangle geometry (rebased) so CallBackCell
        // can map each cell to its row-major value.
        ComputeRect();
        return tUndoRaz::Do();
    }

    tBool tUndoFillSeries::CallBackCell(tTempoPoint* sPoint) {
        // Only the Do pass differs from tUndoCellValue: pick this cell's target
        // value. Save (BeforeDo) and restore (BeforeUndo / Undo) are handled by
        // the base class unchanged.
        if (UndoState() == tUndoState::Do) {
            tSize wIndex = CellIndex(sPoint);
            if (wIndex < m_Values.size()) {
                m_Value.Parse(m_Values[wIndex]);
            } else {
                m_Value.Parse("");
            }
            m_ValueRebased = m_Value;
        }
        return tUndoCellValue::CallBackCell(sPoint);
    }

    void tUndoFillSeries::Json(Writer<StringBuffer>* sWriter) {
        tUndoCellValue::Json(sWriter);
        // Per-cell target values (row-major) for collaboration / persistence replay.
        sWriter->Key("fillv");
        sWriter->StartArray();
        for (const tString& wValue : m_Values) {
            sWriter->String(wValue.c_str());
        }
        sWriter->EndArray();
    }

    void tUndoFillSeries::Json(const rapidjson::Value& sValue) {
        tUndoCellValue::Json(sValue);
        m_Values.clear();
        if (sValue.HasMember("fillv") && sValue["fillv"].IsArray()) {
            for (const auto& wElem : sValue["fillv"].GetArray()) {
                if (wElem.IsString()) {
                    m_Values.push_back(wElem.GetString());
                }
            }
        }
    }
    
	//=========================================================================
	//! Undo set attribute value
	tUndoCellAttribute::tUndoCellAttribute() : tUndoRaz("",tMode(),nullptr), m_Attribute(""), m_Variant() {
	}

	tUndoCellAttribute::tUndoCellAttribute(tString sRef,tString sAttribute, tVariant sVariant, tMode sMode, tSheet* sSheet) : tUndoRaz(sRef,sMode,sSheet),  m_Attribute(sAttribute), m_Variant(sVariant) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set attribute " << sRef << ":" << sAttribute << ":" << sVariant;
            OperationName(wStream.str());
        }
	}

	tUndoCellAttribute::~tUndoCellAttribute() {
		m_Variant.Clear();
	}

    tString  tUndoCellAttribute::ClassName() const { return("tUndoCellAttribute"); }

	tBool tUndoCellAttribute::CallBackCell(tTempoPoint* sPoint) {
		tBool wOk = true;
        
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        
#ifdef debugundoredo
		tString wState;

		switch (UndoState()) {
		case tUndoState::BeforeDo: wState = "BeforeDo";  break;
		case tUndoState::Do: wState = "Do";  break;
		case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
		case tUndoState::Undo: wState = "Undo";  break;
		}
		cout << "tUndoAttribute::" << wState << " CallBackCell " << sPoint->StrRef();
#endif


		switch (UndoState()) {
		case tUndoState::BeforeDo: {
			tCellAttribute* wCellAttribute = wSheet->CellAttribute(sPoint->Row(), sPoint->Col(),m_Attribute());
			if (wCellAttribute != nullptr) {
				m_SaveSelect.AddCell(wCellAttribute);
                // no format
			}
			break;
		}
		case tUndoState::Do: {
			tCellAttribute* wCellAttribute = wSheet->EnsureCellAttribute(sPoint->Row(), sPoint->Col(),m_Attribute());
			if (wCellAttribute != nullptr) {
#ifdef debugundoredo
				cout << " -> wCellAttribute : " << wCellAttribute << ":" <<  wCellAttribute->StrRef() << "=" << wCellAttribute->Value();
#endif
				// Author first Do: UI locale. Redo/Json: US wire (see FormulaWire).
				if (IsJson() || IsRedo()) {
					wOk=CellValueFromWire(wSheet->WorkBook(), wCellAttribute, m_Variant, false);
				} else {
					wOk=wSheet->WorkBook()->CellValue(wCellAttribute, m_Variant, false);
					if (wOk && m_Variant.HasFormula()) {
						tVariant wUnused = m_Variant;
						NormalizeUndoFormulaToWire(wCellAttribute, m_Variant, wUnused);
					}
				}
				m_SaveSelect.PushCalculate(wCellAttribute);
			}
			break;
		}
		case tUndoState::BeforeUndo: {
			break;
		}
		case tUndoState::Undo: {
			tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(wSheet->IndexAllocatorColRowCellRange(),tTypeItem::t_Attribute,sPoint->Row(), sPoint->Col(),m_Attribute());
			//cout << "Undo SavecCell " << Base10ToAlpha(sPoint->Col()) << sPoint->Row();

			if (wSaveCell == nullptr) {
				// Raz CellAttribute ==========================================================
				tCellAttribute* wCellAttribute = wSheet->CellAttribute(sPoint->Row(), sPoint->Col(), m_Attribute());
				if (wCellAttribute != nullptr) {
					if (!wCellAttribute->HasDependent())
						wSheet->DeleteCellAttribute(sPoint->Row(), sPoint->Col(), m_Attribute());
				}
			} else {
				tCellAttribute* wCellAttribute = wSheet->EnsureCellAttribute(sPoint->Row(), sPoint->Col(),m_Attribute());
#ifdef debugundoredo
				cout << " SaveCell(ok) " << "=[" << wCellAttribute->FormulaStr() << "]";
#endif
				wCellAttribute->ClearFormulaAndVariant();
				CellValueFromWire(wSheet->WorkBook(), wCellAttribute, *wSaveCell->Value(), false);
				m_SaveSelect.PushCalculate(wCellAttribute);
			}
			break;
		}
		}
#ifdef debugundoredo
		cout << endl;
#endif
		return(wOk);
	}

    void tUndoCellAttribute::Json(Writer<StringBuffer>* sWriter) {
        tUndoRaz::Json(sWriter);
        sWriter->Key(kJsonKeyAttribute);
        sWriter->String(m_Attribute().c_str());
        m_Variant.Json(sWriter);
    }
    
    void tUndoCellAttribute::Json(const rapidjson::Value& sValue) {
        tUndoRaz::Json(sValue);
        m_Attribute = sValue[kJsonKeyAttribute].GetString();
        
        m_Variant.Json(sValue);
    }

	void tUndoCellAttribute::CalculateDo() {
		m_SaveSelect.CalculateDo(Sheet()->ColRowCellRange(),tVolatile::t_None);
	}

	//=========================================================================
	tUndoCellClassAttributes::tUndoCellClassAttributes()
		: tUndoRaz("", tMode(), nullptr) {
	}

	tUndoCellClassAttributes::tUndoCellClassAttributes(
		tString sRef, const tVectorCellAttributeWire& sAttributes, tMode sMode, tSheet* sSheet)
		: tUndoRaz(sRef, sMode, sSheet) {
		m_Attributes.reserve(sAttributes.size());
		for (const tCellAttributeWire& wWire : sAttributes) {
			tItem wItem;
			wItem.m_Name = wWire.Name;
			wItem.m_Value = wWire.Value;
			m_Attributes.push_back(std::move(wItem));
		}
		if (IsUndoActif()) {
			tStringStream wStream;
			wStream << "Set attributes " << sRef << " (" << m_Attributes.size() << ")";
			OperationName(wStream.str());
		}
	}

	tUndoCellClassAttributes::~tUndoCellClassAttributes() {
		for (tItem& wItem : m_Attributes) {
			wItem.m_Value.Clear();
		}
	}

	tString tUndoCellClassAttributes::ClassName() const { return("tUndoCellClassAttributes"); }

	void tUndoCellClassAttributes::RestoreAttribute(
		tSheet* sSheet, tIndex sRow, tIndex sCol, const tString& sAttribute) {
		tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(
			sSheet->IndexAllocatorColRowCellRange(), tTypeItem::t_Attribute, sRow, sCol, sAttribute);
		if (wSaveCell == nullptr) {
			tCellAttribute* wCellAttribute = sSheet->CellAttribute(sRow, sCol, sAttribute);
			if (wCellAttribute != nullptr && !wCellAttribute->HasDependent()) {
				sSheet->DeleteCellAttribute(sRow, sCol, sAttribute);
			}
		} else {
			tCellAttribute* wCellAttribute = sSheet->EnsureCellAttribute(sRow, sCol, sAttribute);
			wCellAttribute->ClearFormulaAndVariant();
			sSheet->WorkBook()->CellValue(wCellAttribute, *wSaveCell->Value(), false);
			m_SaveSelect.PushCalculate(wCellAttribute);
		}
	}

	void tUndoCellClassAttributes::RestoreAllAttributes(tSheet* sSheet, tIndex sRow, tIndex sCol) {
		for (const tItem& wItem : m_Attributes) {
			RestoreAttribute(sSheet, sRow, sCol, wItem.m_Name());
		}
	}

	tBool tUndoCellClassAttributes::CallBackCell(tTempoPoint* sPoint) {
		tBool wOk = true;
		tSheet* wSheet = Sheet();

		switch (UndoState()) {
		case tUndoState::BeforeDo: {
			for (const tItem& wItem : m_Attributes) {
				tCellAttribute* wCellAttribute =
					wSheet->CellAttribute(sPoint->Row(), sPoint->Col(), wItem.m_Name());
				if (wCellAttribute != nullptr) {
					m_SaveSelect.AddCell(wCellAttribute);
				}
			}
			break;
		}
		case tUndoState::Do: {
			for (const tItem& wItem : m_Attributes) {
				tCellAttribute* wCellAttribute =
					wSheet->EnsureCellAttribute(sPoint->Row(), sPoint->Col(), wItem.m_Name());
				if (wCellAttribute == nullptr) {
					wOk = false;
					break;
				}
				tVariant wValue = wItem.m_Value;
				if (!wSheet->WorkBook()->CellValue(wCellAttribute, wValue, false)) {
					wOk = false;
					break;
				}
				m_SaveSelect.PushCalculate(wCellAttribute);
			}
			if (!wOk) {
				RestoreAllAttributes(wSheet, sPoint->Row(), sPoint->Col());
			}
			break;
		}
		case tUndoState::BeforeUndo: {
			break;
		}
		case tUndoState::Undo: {
			RestoreAllAttributes(wSheet, sPoint->Row(), sPoint->Col());
			break;
		}
		}
		return(wOk);
	}

	void tUndoCellClassAttributes::Json(Writer<StringBuffer>* sWriter) {
		tUndoRaz::Json(sWriter);
		sWriter->Key(kJsonKeyAttributes);
		sWriter->StartArray();
		for (const tItem& wItem : m_Attributes) {
			sWriter->StartObject();
			sWriter->Key(kJsonKeyAttribute);
			sWriter->String(wItem.m_Name().c_str());
			wItem.m_Value.Json(sWriter);
			sWriter->EndObject();
		}
		sWriter->EndArray();
	}

	void tUndoCellClassAttributes::Json(const rapidjson::Value& sValue) {
		tUndoRaz::Json(sValue);
		m_Attributes.clear();
		if (!sValue.HasMember(kJsonKeyAttributes)) {
			return;
		}
		const rapidjson::Value& wArray = sValue[kJsonKeyAttributes];
		for (rapidjson::SizeType wI = 0; wI < wArray.Size(); ++wI) {
			const rapidjson::Value& wObject = wArray[wI];
			tItem wItem;
			wItem.m_Name = wObject[kJsonKeyAttribute].GetString();
			wItem.m_Value.Json(wObject);
			m_Attributes.push_back(std::move(wItem));
		}
	}

	void tUndoCellClassAttributes::CalculateDo() {
		m_SaveSelect.CalculateDo(Sheet()->ColRowCellRange(), tVolatile::t_None);
	}

    //=========================================================================
    tUndoCellClassCalculable::tUndoCellClassCalculable()
        : tUndoRaz("", tMode(), nullptr), m_Variant() {
    }

    tUndoCellClassCalculable::tUndoCellClassCalculable(
        tString sRef, tVariant sVariant, tMode sMode, tSheet* sSheet)
        : tUndoRaz(sRef, sMode, sSheet), m_Variant(sVariant) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set class calculable " << sRef << ":" << sVariant;
            OperationName(wStream.str());
        }
    }

    tUndoCellClassCalculable::~tUndoCellClassCalculable() {
        m_Variant.Clear();
    }

    tString tUndoCellClassCalculable::ClassName() const {
        return("tUndoCellClassCalculable");
    }

    tBool tUndoCellClassCalculable::CallBackCell(tTempoPoint* sPoint) {
        tBool wOk = true;
        tSheet* wSheet = Sheet();
        switch (UndoState()) {
        case tUndoState::BeforeDo: {
            tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr) {
                m_SaveSelect.AddCell(wCell);
            }
            break;
        }
        case tUndoState::Do: {
            tCell* wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr && wCell->Value().IsClass()) {
                tCellClassAttribute* wCellClass =
                    dynamic_cast<tCellClassAttribute*>(wCell->Value().Class());
                if (wCellClass != nullptr) {
                    wCellClass->SetCalculableValue(m_Variant);
                    m_SaveSelect.PushCalculate(wCell);
                } else {
                    wOk = false;
                }
            } else {
                wOk = false;
            }
            break;
        }
        case tUndoState::BeforeUndo: {
            break;
        }
        case tUndoState::Undo: {
            tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(
                wSheet->IndexAllocatorColRowCellRange(),
                tTypeItem::t_Cell,
                sPoint->Row(),
                sPoint->Col(),
                "");
            tCell* wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
            if (wSaveCell == nullptr) {
                if (wCell->Value().IsClass()) {
                    tCellClassAttribute* wCellClass =
                        dynamic_cast<tCellClassAttribute*>(wCell->Value().Class());
                    if (wCellClass != nullptr) {
                        wCellClass->SetCalculableValue(tVariant());
                    }
                }
            } else {
                wCell->ClearFormulaAndVariant();
                wSheet->WorkBook()->CellValue(wCell, *wSaveCell->Value(), false);
                m_SaveSelect.PushCalculate(wCell);
            }
            break;
        }
        }
        return(wOk);
    }

    void tUndoCellClassCalculable::Json(Writer<StringBuffer>* sWriter) {
        tUndoRaz::Json(sWriter);
        m_Variant.Json(sWriter);
    }

    void tUndoCellClassCalculable::Json(const rapidjson::Value& sValue) {
        tUndoRaz::Json(sValue);
        m_Variant.Json(sValue);
    }

    void tUndoCellClassCalculable::CalculateDo() {
        m_SaveSelect.CalculateDo(Sheet()->ColRowCellRange(), tVolatile::t_None);
    }


	//=========================================================================
	//! Undo set Class
    tUndoCellClass::tUndoCellClass() : tUndoCellValue("",tMode(),nullptr), m_ClassName(""), m_ApplyNumeric(false) {
	}

    tUndoCellClass::tUndoCellClass(tString sRef, tString sClassName, tMode sMode, tSheet* sSheet) : tUndoCellValue(sRef,sMode, sSheet), m_ClassName(sClassName),m_ApplyNumeric(false) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set class " << sRef << ":" << sClassName;
            OperationName(wStream.str());
        }
	}

    tUndoCellClass::tUndoCellClass(tString sRef, tVariant* sVariant, tMode sMode, tSheet* sSheet) : tUndoCellValue(sRef,sMode,sSheet),m_ApplyNumeric(false) {
        m_Value=*sVariant;
        if (!m_Value.IsClass()) {
            tStringStream wStream;
            wStream << "throw: sValue  Not class ";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
        tCellClassUnit* wCellClassUnit=dynamic_cast<tCellClassUnit*>(m_Value.Class());
        if (wCellClassUnit!=nullptr) {
            if (wCellClassUnit->Value()->IsNull()) {
                m_ApplyNumeric=true;
            }
        }
        tStringStream wStream;
        wStream << "Set class " << sRef << ":" << m_Value.Class()->ClassName();
        OperationName(wStream.str());
    }


	tUndoCellClass::~tUndoCellClass() {
    }

    tString  tUndoCellClass::ClassName() const { return("tUndoCellClass"); }

	tBool tUndoCellClass::CallBackCell(tTempoPoint* sPoint) {
		tBool wOk = true;
        
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        
#ifdef debugundoredo
		tString wState;

		switch (UndoState()) {
		case tUndoState::BeforeDo: wState = "BeforeDo";  break;
		case tUndoState::Do: wState = "Do";  break;
		case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
		case tUndoState::Undo: wState = "Undo";  break;
		}
		cout << "tUndoClass::" << wState << " CallBackCell " << sPoint->StrRef();
#endif
		switch (UndoState()) {
		case tUndoState::BeforeDo: {
            tUndoCellValue::CallBackCell(sPoint);
			break;
		}
		case tUndoState::Do: {
            // Set by class instance
            if (m_Value.IsClass()) {
                tCell* wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
                // Special case of ApplyNumeric
                // for Unit 
                if (m_ApplyNumeric) {
                    // Search Value on tSaveSelect
                    tSaveCell*  wSaveCell=m_SaveSelect.FindCell(sPoint->Row(), sPoint->Col(),wSheet);
                    if (wSaveCell!=nullptr) {
                        tVariant* wSaved = wSaveCell->Value();
                        if ((wSaved->IsInt()) || (wSaved->IsDouble())) {
                            m_Value.Class()->Value(wSaved);
                        } else if (wSaved->IsClass()) {
                            // Nested cell class (e.g. another unit cell); never call Class() on null/string/etc.
                            tVariant wValue = *wSaved;
                            tCellClass* wCellClass = dynamic_cast<tCellClass*>(wValue.Class());
                            if (wCellClass!=nullptr) {
                                m_Value.Class()->Value(wCellClass->Value());
                            }
                        }
                    }
                }
                // Place value (Copy class by variant)
                wCell->Value(m_Value);
            } else {
                // Set by class name
                wSheet->ColRowCellRange()->EnsureCellClass(sPoint->Row(), sPoint->Col(),m_ClassName);
            }
#ifdef debugundoredo
            tCell* wCell = wSheet->ColRowCellRange()->Cell(sPoint->Row(), sPoint->Col());
            cout << " -> wCell : " << wCell->StrRef() << " Create Class " << m_ClassName << endl;
#endif
    		break;
		}
		case tUndoState::BeforeUndo: {
            // Idem tUndoCellValue
            tUndoCellValue::CallBackCell(sPoint);
			break;
		}
		case tUndoState::Undo: {
            // Idem tUndoCellValue
            tUndoCellValue::CallBackCell(sPoint);
            break;
		}
            
		}
#ifdef debugundoredo
		cout << endl;
#endif
		return(wOk);
	}

    void tUndoCellClass::Json(Writer<StringBuffer>* sWriter) {
        // Persist m_Value (class instance), not m_ValueRebased — rebased/scalar paths can leave ValueRebased
        // cleared after Do while m_Value still holds the tCellUnit payload (fixes Web sync: cn/t/v wrong).
        tUndoRaz::Json(sWriter);
        m_Value.Json(sWriter);
        tString wClassName = m_ClassName;
        if (wClassName.empty() && m_Value.IsClass()) {
            wClassName = m_Value.Class()->ClassName();
        }
        sWriter->Key(kJsonKeyClassName);
        sWriter->String(wClassName.c_str());
        sWriter->Key(kJsonKeyApplyNumeric);   
        sWriter->Bool(m_ApplyNumeric);
    }   
    
    void tUndoCellClass::Json(const rapidjson::Value& sValue) {
        tUndoCellValue::Json(sValue);
        if (sValue.HasMember(kJsonKeyClassName) && sValue[kJsonKeyClassName].IsString()) {
            m_ClassName = sValue[kJsonKeyClassName].GetString();
        } else {
            m_ClassName.clear();
        }
        if (sValue.HasMember(kJsonKeyApplyNumeric) && sValue[kJsonKeyApplyNumeric].IsBool()) {
            m_ApplyNumeric = sValue[kJsonKeyApplyNumeric].GetBool();
        } else {
            m_ApplyNumeric = false;
        }
        if (m_ClassName.empty() && m_Value.IsClass()) {
            m_ClassName = m_Value.Class()->ClassName();
        }
    }


    //=========================================================================
    //!  Undo to apply format on Sheet Col Row or Celll
    tUndoFormat::tUndoFormat() : tUndoSpreadSheetCallBack(0, "", tMode()), m_CssSheet(0), m_SaveSelectColRow_Rows(true), m_SaveSelectColRow_Cols(false), m_Format("") {
        // Anchor UndoSpreadSheet to both instances
        m_SaveSelectColRow_Rows.UndoSpreadSheet(this);
        m_SaveSelectColRow_Cols.UndoSpreadSheet(this);
    }

    tUndoFormat::tUndoFormat(tString sRef, tString sFormat,tMode sMode, tSheet* sSheet) : tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), sRef, sMode), m_CssSheet(0),m_SaveSelectColRow_Rows(true),m_SaveSelectColRow_Cols(false),m_Format(sFormat) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set Format " << sRef << ":" << sFormat;
            OperationName(wStream.str());
        }
        // Anchor UndoSpreadSheet to both instances
        m_SaveSelectColRow_Rows.UndoSpreadSheet(this);
        m_SaveSelectColRow_Cols.UndoSpreadSheet(this);
    }

    tUndoFormat::~tUndoFormat() {
        if (m_CssSheet!=0) {
            Sheet()->WorkBook()->DeleteCellFormat(m_CssSheet);
        }
        m_SaveSelectColRow_Rows.DeleteCellFormat(Sheet());
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.DeleteCellFormat(Sheet());
        m_SaveSelectColRow_Cols.Clear();
    }

    tString tUndoFormat::ClassName() const { return("tUndoFormat"); }

    // Return rows instance by default (cells are stored in rows instance)
    // Both instances need to be rebased separately if needed
    tSaveSelect* tUndoFormat::SaveSelect() { return(&m_SaveSelectColRow_Rows); }

    namespace {
        tSaveCell* FindFormatUndoSaveCell(tSheet* sSheet, tSaveSelectColRow& sSave,
                                          tIndex sRow, tIndex sCol) {
            return sSave.FindSaveCell(sSheet->IndexAllocatorColRowCellRange(),
                                      tTypeItem::t_Cell, sRow, sCol, "");
        }

        // Live cell gets its own IncCell slot on the pooled ref (SaveCell keeps the snapshot ref).
        tBool AssignLiveFormatFromSaveCss(tFormatApi* sFormatApi, tCell* sCell, tFormatRef sSaveCss) {
            if (sCell == nullptr || sFormatApi == nullptr || sSaveCss == 0) {
                return true;
            }
            const tString wCss = sFormatApi->CellFormat(sSaveCss);
            if (wCss.empty()) {
                return false;
            }
            tFormatRef wLive = sFormatApi->ApplyCellFormat(wCss);
            if (wLive == 0) {
                return false;
            }
            sCell->Css(wLive);
            return true;
        }

        // After BeforeDo PassCss: old format stays on SaveCell, merged result on live cell only.
        tBool ApplyCellFormatPassCss(tWorkBook* sWorkBook, tCell* sCell,
                                     tSaveSelectColRow& sSave, tSheet* sSheet,
                                     tIndex sRow, tIndex sCol, const tString& sFormat) {
            if (sCell == nullptr || sWorkBook == nullptr) {
                return false;
            }
            if (sCell->Css() != 0) {
                return sWorkBook->ApplyCellFormat(sCell, sFormat);
            }
            tSaveCell* wSaveCell = FindFormatUndoSaveCell(sSheet, sSave, sRow, sCol);
            if (wSaveCell == nullptr || wSaveCell->Css() == 0) {
                return sWorkBook->ApplyCellFormat(sCell, sFormat);
            }
            tFormatApi* wFormatApi = sWorkBook->FormatApi();
            if (wFormatApi == nullptr) {
                return false;
            }
            const tFormatRef wSaveCss = wSaveCell->Css();
            tFormatRef wNew = wFormatApi->ApplyCellFormat(sFormat);
            if (wNew == 0) {
                return false;
            }
            if (wNew == wSaveCss) {
                wFormatApi->DeleteCellFormat(wNew);
                return AssignLiveFormatFromSaveCss(wFormatApi, sCell, wSaveCss);
            }
            wFormatApi->BeginMerge();
            wFormatApi->Merge(wSaveCss);
            wFormatApi->Merge(wNew);
            tFormatRef wApply = wFormatApi->ApplyMerge();
            wFormatApi->DeleteCellFormat(wNew);
            if (wApply == 0) {
                return true;
            }
            if (wApply == wSaveCss) {
                wFormatApi->DeleteCellFormat(wApply);
                return AssignLiveFormatFromSaveCss(wFormatApi, sCell, wSaveCss);
            }
            sCell->Css(wApply);
            return true;
        }

        tFormatRef PassCssSnapshot(tSheet* sSheet, tSaveSelectColRow& sSave,
                                   tCell* sCell, tIndex sRow, tIndex sCol) {
            if (sCell != nullptr && sCell->Css() != 0) {
                return sCell->Css();
            }
            tSaveCell* wSaveCell = FindFormatUndoSaveCell(sSheet, sSave, sRow, sCol);
            if (wSaveCell != nullptr) {
                return wSaveCell->Css();
            }
            return 0;
        }
    }

    tBool tUndoFormat::CallBackCell(tTempoPoint* sPoint) {
        tBool wOk = true;
        
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        
#ifdef debugundoredo
        tString wState;

        switch (UndoState()) {
        case tUndoState::BeforeDo: wState = "BeforeDo";  break;
        case tUndoState::Do: wState = "Do";  break;
        case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
        case tUndoState::Undo: wState = "Undo";  break;
        }
        cout << "tUndoFormat::" << wState << " CallBackCell " << sPoint->StrRef() << endl;
#endif

        switch (UndoState()) {
        case tUndoState::BeforeDo: {
            // Save Format ====================================================
            tCell* wCell = wSheet->Cell(sPoint->Row(), sPoint->Col());
            if (wCell != nullptr) {
                // Use rows instance for cells (both instances can store cells)
                tSaveCell* wSaveCell=m_SaveSelectColRow_Rows.FindCell(wCell);
                if (wSaveCell==nullptr) {
                    wSaveCell=m_SaveSelectColRow_Rows.AddCell(wCell);
                    if (wCell->Css()!=0) {
#ifdef debugundoredo
                        cout << " SaveCell Pass --> Css=" << wCell->Css() << endl;
#endif
                        // Pass Format to save don't inc()
                        wSaveCell->PassCss(wCell);
                     }
                }
            }
            break;
        }
        case tUndoState::Do: {
            // passes only once per cell =======================================
            if (!m_ContainerCellDo.Exist(tPoint(sPoint->Row(),sPoint->Col()))) {
                tCell* wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
                // Apply Format
                tWorkBook* wWorBook=wSheet->WorkBook();
                
                wOk=ApplyCellFormatPassCss(wWorBook, wCell, m_SaveSelectColRow_Rows, wSheet,
                                           sPoint->Row(), sPoint->Col(), m_Format);
                
#ifdef debugundoredo
                cout << "WorkBook:" << wWorBook->Uri() << " Css " << "=" << wCell->Css();
#endif
                m_ContainerCellDo.InsertClass(tPoint(sPoint->Row(),sPoint->Col()));
            }
            break;
        }
        case tUndoState::BeforeUndo: {
            break;
        }
        case tUndoState::Undo: {
            if (!m_ContainerCellDo.Exist(tPoint(sPoint->Row(),sPoint->Col()))) {
                tSaveCell* wSaveCell = m_SaveSelectColRow_Rows.FindSaveCell(wSheet->IndexAllocatorColRowCellRange(),tTypeItem::t_Cell, sPoint->Row(), sPoint->Col(),"");
                if (wSaveCell != nullptr) {
                    // Pass Format Save to Cell
                    tCell* wCell = wSaveCell->EnsureCell();
#ifdef debugundoredo
                    cout << " Css Old " << wCell->Css() << " New " << wSaveCell->Css() << endl; ;
#endif
                    // Delete old Format =======================================
                    if (wCell->Css()!=0) {
                        wSheet->WorkBook()->DeleteCellFormat(wCell->Css());
                        wCell->Css(0);
                    }
       
                    if (!IsJson()) {
                        // Replace Css
                        wCell->Css(wSaveCell->Css());
                        wSaveCell->Css(0);
                    } else {
                        // Apply Format
                        wSheet->WorkBook()->ApplyCellFormat(wCell,wSaveCell->FormatStr());

#ifdef debugundoredo
                        tWorkBook* wWorkBookActive=wSheet->WorkBook();
                        cout << "On " << wWorkBookActive->Uri() << " Format=" << wWorkBookActive->CellFormat(wCell->Css()) << endl;
                        cout << "Apply {" << wSaveCell->FormatStr() << "}" << endl;
#endif
                    }
                    
                } else {
                    // If new format delete
                    tCell* wCell=wSheet->Cell(sPoint->Row(),sPoint->Col());
                    if (wCell!=nullptr) {
                        // Delete old format
                        if (wCell->Css()!=0) {
                            wSheet->WorkBook()->DeleteCellFormat(wCell->Css());
                            wCell->Css(0);
                        }
                    }
                }
                m_ContainerCellDo.InsertClass(tPoint(sPoint->Row(),sPoint->Col()));
            }
            break;
        }
        }
#ifdef debugundoredo
        cout  << endl;
#endif
        return(wOk);
    };


    tBool tUndoFormat::CallBackRange(tTempoRect* sRect) {
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        
#ifdef debugundoredo
        cout << "tUndoFormat::CallBackRange " << sRect->StrRef() << endl;
#endif
        // Sheet is Selected
        if (sRect->IsSheetSelect()) {
            switch (UndoState()) {
                case tUndoState::BeforeDo: {
                    m_CssSheet=wSheet->Css();
                    break;
                }
                case tUndoState::Do: {
                    wSheet->WorkBook()->ApplySheetFormat(wSheet,m_Format);
                    break;
                }
                case tUndoState::BeforeUndo: break;
                case tUndoState::Undo: {
                    if (wSheet->Css()!=0) {
                        wSheet->WorkBook()->DeleteCellFormat(wSheet->Css());
                    }
                    wSheet->Css(m_CssSheet);
                    m_CssSheet=0;
                }
            }
            return(true);
        }
        // All row or All col are selected
        // Use separate instances for rows and columns to avoid index conflicts
        if ((sRect->IsRowSelect()) || (sRect->IsColSelect())) {
            switch (UndoState()) {
                case tUndoState::BeforeDo: {
                    if (sRect->IsRowSelect()) {
                        for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                            tColRow* wColRow=wSheet->Row(wRow);
                            if (wColRow!=nullptr)
                                m_SaveSelectColRow_Rows.AddColRow(wColRow,wSheet->ColRowCellRange(),true);
                        }
                    } else {
                        for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                            tColRow* wColRow=wSheet->Col(wCol);
                            if (wColRow!=nullptr)
                                m_SaveSelectColRow_Cols.AddColRow(wColRow,wSheet->ColRowCellRange(),false);
                        }
                    }
                    break;
                }
                case tUndoState::Do: {
                    if (sRect->IsRowSelect()) {
                        for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                            wSheet->WorkBook()->ApplyColRowFormat(wSheet->EnsureRow(wRow),m_Format);
                        }
                    } else {
                        for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                            wSheet->WorkBook()->ApplyColRowFormat(wSheet->EnsureCol(wCol),m_Format);
                        }
                    }
                    break;
                }
                case tUndoState::BeforeUndo: break;
                case tUndoState::Undo: {
                    if (sRect->IsSheetSelect()) {
                        if (wSheet->Css()!=0) {
                            wSheet->WorkBook()->DeleteCellFormat(wSheet->Css());
                        }
                        wSheet->Css(m_CssSheet);
                        m_CssSheet=0;
                    }
                    if (sRect->IsRowSelect()) {
                        for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                            tSaveColRow* wSaveColRow=m_SaveSelectColRow_Rows.FindColRow(wRow);
                            if (wSaveColRow!=nullptr) {
                                tColRow* wColRow=wSheet->Row(wRow);
                                // Raz Old
                                if (wColRow->Css()!=0) {
                                    wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                }
                                wColRow->Css(wSaveColRow->Css());
                                wSaveColRow->Css(0);
                            }  else {
                                tColRow* wColRow=wSheet->Row(wRow);
                                if (wColRow!=nullptr) {
                                    if (wColRow->Css()!=0) {
                                        wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                        wColRow->Css(0);
                                    }
                                }
                            }
                        }
                    }
                    if (sRect->IsColSelect()) {
                        for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                            tSaveColRow* wSaveColRow=m_SaveSelectColRow_Cols.FindColRow(wCol);
                            if (wSaveColRow!=nullptr) {
                                tColRow* wColRow=wSheet->Col(wCol);
                                // Raz Old
                                if (wColRow->Css()!=0) {
                                    wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                }
                                wColRow->Css(wSaveColRow->Css());
                                wSaveColRow->Css(0);
                                
                            } else {
                                tColRow* wColRow=wSheet->Col(wCol);
                                if (wColRow!=nullptr) {
                                    if (wColRow->Css()!=0) {
                                        wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                        wColRow->Css(0);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            return(true);
        }
        // Else CallBack Cell
        for (tIndex wRow = sRect->Row(); wRow <= sRect->Bottom(); wRow++) {
            for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
                tTempoPoint wPoint(wRow, wCol);
                if (!CallBackCell(&wPoint)) return(false);
            }
        }
        return(true);
    }

    tBool tUndoFormat::Do() {
        tBool wResult = false;
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.Clear();
        m_ContainerCellDo.Container()->clear();
        
        
        // Important Select use temporary memory
        // We have to parse again every time
        // Use rows instance for parsing (both instances can store cells)
        if (m_SaveSelectColRow_Rows.ParseSelect(m_RefRebase)) {
            UndoState(tUndoState::BeforeDo);
            wResult = m_SaveSelectColRow_Rows.Select().CallBack(this);
            if (wResult) {
                UndoState(tUndoState::Do);
                wResult = m_SaveSelectColRow_Rows.Select().CallBack(this);
            }
            
#ifdef debugundoredo
            cout << "tUndoFormat::Do" << endl;
#endif

#ifdef checksp
            Sheet()->Check();
#endif
        }
        return(wResult);
    }

    tBool tUndoFormat::Undo() {
        tBool wResult = true;
        // Raz Control multi selection in same place
        m_ContainerCellDo.Container()->clear();
        // Important Select use temporary memory
        // Use rows instance for parsing (both instances can store cells)
        m_SaveSelectColRow_Rows.ParseSelect(m_RefRebase);
            
        UndoState(tUndoState::BeforeUndo);
        wResult = m_SaveSelectColRow_Rows.Select().CallBack(this);
        if (wResult) {
            UndoState(tUndoState::Undo);
            wResult = m_SaveSelectColRow_Rows.Select().CallBack(this);
        }
#ifdef checksp
        Sheet()->Check();
#endif
        return(wResult);
    }

    void tUndoFormat::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyFormat);
        sWriter->String(m_Format.c_str());
        if (IsUndo()) {
            sWriter->Key(kJsonKeySave);
            sWriter->StartObject();
            tSheet* wSheet=Sheet();
            // Serialize cells from rows instance (cells are stored there)
            m_SaveSelectColRow_Rows.ColRowCellRange(wSheet->ColRowCellRange());
            m_SaveSelectColRow_Rows.Json(sWriter,wSheet);
            // Serialize columns separately if they exist
            if (!m_SaveSelectColRow_Cols.VectorColRow()->empty()) {
                sWriter->Key("cols");
                sWriter->StartArray();
                for (auto wSaveColRow : *m_SaveSelectColRow_Cols.VectorColRow()) {
                    wSaveColRow->Json(sWriter);
                }
                sWriter->EndArray();
            }
            sWriter->EndObject();
        }
    }
    
    void tUndoFormat::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        m_Format = sValue[kJsonKeyFormat].GetString();
        if (sValue.HasMember(kJsonKeySave)) {
            tSheet* wSheet=Sheet();
            // Deserialize rows instance (contains cells)
            m_SaveSelectColRow_Rows.Json(sValue[kJsonKeySave],wSheet);
            // Deserialize columns if they exist
            if (sValue[kJsonKeySave].HasMember("cols")) {
                const rapidjson::Value& wColsValue = sValue[kJsonKeySave]["cols"];
                for (auto& wColValue : wColsValue.GetArray()) {
                    tSaveColRow* wSaveColRow = new tSaveColRow();
                    wSaveColRow->Json(wColValue);
                    m_SaveSelectColRow_Cols.VectorColRow()->push_back(wSaveColRow);
                }
            }
        }
    }
    
    tBool tUndoFormat::Rebase() {
        // Build rebase plan
        SetRebasePlan();
        if (!m_RebasePlan.IsEmpty()) {
            SetRebasePlan();
            
            // Rebase reference
            tSelect* wTempoSelect = new tSelect();
            if (!m_RebasePlan.m_Operations.empty()) {
                wTempoSelect->Parse(m_Ref);
                if (wTempoSelect->Rebase(m_RebasePlan)) {
                    m_RefRebase = wTempoSelect->StrRef();
                }
            }
#ifdef _debugleak
            delete(wTempoSelect);
#endif
            
            // Rebase both instances separately
            // Rows instance (contains cells and rows)
            if (!m_SaveSelectColRow_Rows.Rebase(m_RebasePlan)) {
                return false;
            }
            
            // Columns instance (contains only columns)
            if (!m_SaveSelectColRow_Cols.Rebase(m_RebasePlan)) {
                return false;
            }
        
        }
        return true;
    }

    void tUndoFormat::DeleteBeforeDo() {
        // Call parent to reset reference rebase
        tUndoSpreadSheetCallBack::DeleteBeforeDo();
        // Clear save select structures and container before Do
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.Clear();
        m_ContainerCellDo.Container()->clear();
    }

#ifdef checkfo
    void tUndoFormat::IncCheckfo(tFormatApi* sFormatApi) {
        if (m_CssSheet!=0) {
            sFormatApi->IncCheck(m_CssSheet);
        }
        m_SaveSelectColRow_Rows.IncCheckfo(sFormatApi);
        m_SaveSelectColRow_Cols.IncCheckfo(sFormatApi);
    }
#endif

    //=========================================================================
    //!  Undo inc or dec  format precision on  Celll
    tUndoPrecision::tUndoPrecision() : tUndoFormat(), m_Inc(false),m_LeastOneElement(false) {
    }

    tUndoPrecision::tUndoPrecision(tString sRef,tBool sInc,tMode sMode, tSheet* sSheet) : tUndoFormat(sRef,"",sMode,sSheet), m_Inc(sInc),m_LeastOneElement(false) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Set Precision " << sRef << ":" << sInc;
            OperationName(wStream.str());
        }
    }

    tUndoPrecision::~tUndoPrecision() {
    }

    tString tUndoPrecision::ClassName() const { return("tUndoPrecision"); }

    tBool tUndoPrecision::CallBackCell(tTempoPoint* sPoint) {
        // Get Sheet by Alllocator
        tSheet* wSheet=Sheet();
        
        tBool wOk = true;
#ifdef debugundoredo
		tString wState;

		switch (UndoState()) {
		case tUndoState::BeforeDo: wState = "BeforeDo";  break;
		case tUndoState::Do: wState = "Do";  break;
		case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
		case tUndoState::Undo: wState = "Undo";  break;
		}
		cout << "tUndoPrecision::" << wState << " CallBackCell " << sPoint->StrRef();
#endif
    
        switch (UndoState()) {
        case tUndoState::BeforeDo: tUndoFormat::CallBackCell(sPoint); break;    
        case tUndoState::Do: {
            tCell* wCell=wSheet->Cell(sPoint->Row(),sPoint->Col());
            if (wCell!=nullptr) {
               tFormatApi* wFormatApi=wSheet->WorkBook()->FormatApi();
               // After BeforeDo PassCss the live cell Css is 0; read from SaveCell snapshot.
               const tFormatRef wSourceCss=PassCssSnapshot(wSheet, m_SaveSelectColRow_Rows, wCell,
                                                           sPoint->Row(), sPoint->Col());
               m_Format=wFormatApi->CellFormat(wSourceCss);
               tString wJson=wFormatApi->Format2Json(wSourceCss);
                // Apply if number ============================================
                tVariant wVariant=wCell->Value();
                if ((wVariant.IsInt()) || (wVariant.IsDouble()) || (wVariant.IsNull()) ) {
                    tBool wOk=false;
                    rapidjson::Document document;
                    if (wJson!="") {
                        // Parse  JSON
#ifdef debugundoredo
                        cout << wSheet->WorkBook()->Uri() << ":" << wCell->StrRef(true) << endl;
                        cout << "Before apply Precision "  << wJson << endl;
#endif
                        document.Parse(wJson.c_str());
                        
                        // search
                        if (document.HasMember("t"))  {
                            if (document["t"].IsObject()) {
                                // Modify or inspect the formats object here (needs non-const refs to mutate)
                                rapidjson::Value& wValueText = document["t"];
                                if (wValueText.HasMember("f")) {
                                    if (wValueText["f"].IsObject())  {
                                        rapidjson::Value& wValueFormat = wValueText["f"];
                                        if (wValueFormat.HasMember("p")){
                                            if (wValueFormat["p"].IsInt()) {
                                                rapidjson::Value& wValuePrecision=wValueFormat["p"];
                                                tInt wPrecision=wValuePrecision.GetInt();
                                                wPrecision += m_Inc ? 1 : -1;
                                                if (wPrecision>=0) {
                                                    wValuePrecision.SetInt(wPrecision);
                                                    // excelnumber displays via pattern "c" (SkTypesClass excel branch); keep it in sync with p.
                                                    if (wValueFormat.HasMember("f") && wValueFormat["f"].IsInt() &&
                                                        static_cast<SkRoot::tFormatStringType>(wValueFormat["f"].GetInt()) == SkRoot::tFormatStringType::excelnumber &&
                                                        wValueFormat.HasMember("c") && wValueFormat["c"].IsString()) {
                                                        const tString wOldExcel = wValueFormat["c"].GetString();
                                                        SkRoot::tFormatString wFs;
                                                        const tString wNewExcel = wFs.AddDecimals(wOldExcel, m_Inc ? 1 : -1);
                                                        rapidjson::Value& wC = wValueFormat["c"];
                                                        wC.SetString(wNewExcel.c_str(),
                                                            static_cast<rapidjson::SizeType>(wNewExcel.length()),
                                                            document.GetAllocator());
                                                    }
                                                }
                                                wOk=true;
                                            }
                                        }
                                        // Legacy/compact Json omitted "p" when decimal count was 0 — cannot match HasMember("p") above.
                                        if (!wOk && wValueFormat.HasMember("f") && wValueFormat["f"].IsInt()
                                            && static_cast<SkRoot::tFormatStringType>(wValueFormat["f"].GetInt()) == SkRoot::tFormatStringType::excelnumber
                                            && wValueFormat.HasMember("c") && wValueFormat["c"].IsString()
                                            && !wValueFormat.HasMember("p")) {
                                            tInt wPrecision = 0;
                                            wPrecision += m_Inc ? 1 : -1;
                                            if (wPrecision >= 0) {
                                                wValueFormat.AddMember("p", rapidjson::Value().SetInt(wPrecision), document.GetAllocator());
                                                const tString wOldExcel = wValueFormat["c"].GetString();
                                                SkRoot::tFormatString wFs;
                                                const tString wNewExcel = wFs.AddDecimals(wOldExcel, m_Inc ? 1 : -1);
                                                rapidjson::Value& wC = wValueFormat["c"];
                                                wC.SetString(wNewExcel.c_str(),
                                                    static_cast<rapidjson::SizeType>(wNewExcel.length()),
                                                    document.GetAllocator());
                                                wOk = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        document.SetObject(); // Root is an object
                    }
                    // Generete Json nodes
                    if (!wOk) {
                        if (!document.HasMember("t") || !document["t"].IsObject()) {
                            rapidjson::Value wT(rapidjson::kObjectType);
                            document.AddMember("t", wT, document.GetAllocator());
                        }
                        rapidjson::Value& wTRef2 = document["t"];
                        if (!wTRef2.HasMember("f") || !wTRef2["f"].IsObject()) {
                            rapidjson::Value wF(rapidjson::kObjectType);
                            wTRef2.AddMember("f", wF, document.GetAllocator());
                        }
                        rapidjson::Value& wFRef2 = wTRef2["f"];

                        // Add format type  
                        if (!wFRef2.HasMember("f")) {
                            wFRef2.AddMember("f", rapidjson::Value().SetInt(tInt(tFormatStringType::numeric0)), document.GetAllocator());
                        }
                        // Add precision
                        tInt wPrecision=0;
                        if (m_Inc) wPrecision=1;
                        if (!wFRef2.HasMember("p")) {
                            wFRef2.AddMember("p", rapidjson::Value().SetInt(wPrecision), document.GetAllocator());
                        } else {
                            if (!wFRef2["p"].IsInt()) {
                                wFRef2.RemoveMember("p");
                                wFRef2.AddMember("p", rapidjson::Value().SetInt(wPrecision), document.GetAllocator());
                            } else {
                                wFRef2["p"].SetInt(wPrecision);
                            }
                        }
                        wOk=true;
                    }
                    // Convertir en string pour afficher
                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    document.Accept(writer);
                    wJson=buffer.GetString();
#ifdef debugundoredo
                    cout << "Afer apply precision "  << wJson << endl;
#endif
                    m_Format=wFormatApi->Json2Format(wJson);
#ifdef debugundoredo
                    cout << m_Format << endl;
#endif
                    m_LeastOneElement=true;
                }
                // Call Format Do method
                tUndoFormat::CallBackCell(sPoint);
            }
                
              
            break;
        }
        case tUndoState::BeforeUndo: tUndoFormat::CallBackCell(sPoint); break;
        case tUndoState::Undo: tUndoFormat::CallBackCell(sPoint); break;
        }
#ifdef debugundoredo
        cout << endl;
#endif
    return(wOk);
    }   

    tBool tUndoPrecision::Do() {
        tBool wOk=tUndoFormat::Do();
        return(wOk && m_LeastOneElement);
    }

    void tUndoPrecision::Json(Writer<StringBuffer>* sWriter) {
        tUndoFormat::Json(sWriter);
        sWriter->Key(kJsonKeyInc); sWriter->Bool(m_Inc);
    }

    void tUndoPrecision::Json(const rapidjson::Value& sValue) {
        tUndoFormat::Json(sValue);
        m_Inc=sValue[kJsonKeyInc].GetBool();
    }

    //=========================================================================
    //!  Undo to apply borderl
    tUndoBorder::tUndoBorder() : tUndoFormat("","",tMode(),nullptr), m_Border(0), m_UseCombinedCellMasks(false) {
    }

    tUndoBorder::tUndoBorder(tString sRef, tShort sBorder, tString sFormat,tMode sMode, tSheet* sSheet) : tUndoFormat(sRef,sFormat,sMode,sSheet), m_Border(sBorder), m_UseCombinedCellMasks(false) {
        if (IsUndoActif()) {
            tStringStream wStream;
            tString wFormat=BorderStr(sBorder)+":";
            wStream << "Set " << wFormat << " " << sRef << ":" << sFormat;
            OperationName(wStream.str());
        }
    }

    tUndoBorder::~tUndoBorder() {
    }

    tString tUndoBorder::ClassName() const { return("tUndoBorder"); }

    tString tUndoBorder::FormatBorder(tShort sBorderType,tString sFormat) {
        tString wBorderFormat="";
        switch (sBorderType) {
            case tBorderAll :
            case tBorderTop :
            case tBorderLeft :
            case tBorderBottom :
            case tBorderRight : {
                wBorderFormat=BorderStr(sBorderType)+":"+sFormat;
                break;
            }
            default : {
                //if ((sBorderType & tBorderAll) == tBorderAll) wBorderFormat=BorderStr(tBorderAll)+":"+sFormat;
                if ((sBorderType & tBorderTop) == tBorderTop)  wBorderFormat+=BorderStr(tBorderTop)+":"+sFormat;
                if ((sBorderType & tBorderLeft) == tBorderLeft)  wBorderFormat+=BorderStr(tBorderLeft)+":"+sFormat;
                if ((sBorderType & tBorderBottom) == tBorderBottom)  wBorderFormat+=BorderStr(tBorderBottom)+":"+sFormat;
                if ((sBorderType & tBorderRight) == tBorderRight)  wBorderFormat+=BorderStr(tBorderRight)+":"+sFormat;
                break;
            }
        }
        return(wBorderFormat);
    }

    void tUndoBorder::ApplyBorder(tTempoPoint* sPoint,tShort sBorder) {
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        
        tCell* wCell = nullptr;
        // If Not Border
        if (m_Format!="") {
            wCell = wSheet->EnsureCell(sPoint->Row(), sPoint->Col());
        } else  {
            wCell=wSheet->Cell(sPoint->Row(), sPoint->Col());
        }
#ifdef debugundoredo
        tString wState;
        switch (UndoState()) {
        case tUndoState::BeforeDo: wState = "BeforeDo";  break;
        case tUndoState::Do: wState = "Do";  break;
        case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
        case tUndoState::Undo: wState = "Undo";  break;
        }
        cout << "tUndoBorder::ApplyBorder " << wState << " " << sPoint->StrRef() << " "  << sBorder << m_Format << endl;
#endif

        switch (UndoState()) {
            case tUndoState::BeforeDo: {
                // Call tUndoFormat for save old format
                // Only pass one
                tUndoFormat::CallBackCell(sPoint);
                break;
            }
            case tUndoState::Do: {
                if (m_Format!="") {
                    tString wFormat=FormatBorder(sBorder, m_Format);
                    // After BeforeDo PassCss live Css is 0 on first visit; on overlap revisits it holds the intermediate ref.
                    tFormatRef wLiveBeforeApply=wCell->Css();
                    tWorkBook* wWorkBook=wSheet->WorkBook();
#ifdef debugundoredo
                    cout << " WorkBook:" << wWorkBook->Uri() <<  " Old Css=" << wCell->Css();
#endif
                   
                    ApplyCellFormatPassCss(wWorkBook, wCell, m_SaveSelectColRow_Rows, wSheet,
                                           sPoint->Row(), sPoint->Col(), wFormat);
                    // Revisit (overlapping ranges without combined masks): drop the intermediate live ref.
                    if (m_ContainerCellDo.Exist(tPoint(sPoint->Row(),sPoint->Col()))) {
                        if (wLiveBeforeApply!=0 && wLiveBeforeApply!=wCell->Css()) {
                            wWorkBook->DeleteCellFormat(wLiveBeforeApply);
                        }
                    }
                    m_ContainerCellDo.InsertClass(tPoint(sPoint->Row(),sPoint->Col()));
#ifdef debugundoredo
                    cout << " Css=" << wCell->Css() << endl;
#endif
                } else {
                    // Delete Format border ============================
                    if (wCell!=nullptr) {
#ifdef debugundoredo
                        cout << " DeleteBorder:" << wCell->Css() << endl;
#endif
                        tFormatRef wLiveBeforeDelete=wCell->Css();
                        tWorkBook* wWorkBook=wSheet->WorkBook();
                        const tFormatRef wSourceCss=PassCssSnapshot(wSheet, m_SaveSelectColRow_Rows, wCell,
                                                                    sPoint->Row(), sPoint->Col());
                        tFormatRef wFormatRef=wWorkBook->DeleteBorder(wSourceCss, sBorder);
                        tSaveCell* wSaveCell=FindFormatUndoSaveCell(wSheet, m_SaveSelectColRow_Rows,
                                                                    sPoint->Row(), sPoint->Col());
                        tFormatApi* wFormatApi=wWorkBook->FormatApi();
                        if (wFormatRef != 0 &&
                            (wSaveCell == nullptr || wFormatRef != wSaveCell->Css())) {
                            wCell->Css(wFormatRef);
                            // DeleteBorder noop on source ref: ApplyMerge already IncCell once; bump when reusing source.
                            if (wFormatRef == wSourceCss) {
                                wWorkBook->IncCellFormat(wFormatRef);
                            }
                        } else if (wFormatRef != 0 && wSaveCell != nullptr &&
                                   wFormatRef == wSaveCell->Css() && wFormatApi != nullptr) {
                            // No-op delete: live stays off SaveCell snapshot; give live its own slot for display.
                            AssignLiveFormatFromSaveCss(wFormatApi, wCell, wSaveCell->Css());
                        }
                        // Revisit: drop intermediate live ref from a prior delete pass on the same cell.
                        if (m_ContainerCellDo.Exist(tPoint(sPoint->Row(),sPoint->Col()))) {
                            if (wLiveBeforeDelete != 0 && wLiveBeforeDelete != wCell->Css()) {
                                wWorkBook->DeleteCellFormat(wLiveBeforeDelete);
                            }
                        }
                        m_ContainerCellDo.InsertClass(tPoint(sPoint->Row(),sPoint->Col()));
                    }

                }
                break;
            }
            case tUndoState::BeforeUndo : break;
            case tUndoState::Undo : {
                tUndoFormat::CallBackCell(sPoint);
                break;
            }
        }
    }

    //------------------------------------------------------------------
    // Excel-like border storage: top, left, right and bottom are stored
    // on the cell that owns the edge (same model as .xlsx cell styles).
    //
    // FillCellMasks builds (cell -> combined side mask); CallBack applies
    // once per cell so format refcounts stay consistent.
    //------------------------------------------------------------------
    void tUndoBorder::FillCellMasks(tIndex sRow0, tIndex sCol0,
                                    tIndex sRow1, tIndex sCol1,
                                    std::map<tPoint, tShort>& sMasks) {
        auto wAdd = [&sMasks](tIndex sRow, tIndex sCol, tShort sMask) {
            tPoint wKey(sRow, sCol);
            auto wIt = sMasks.find(wKey);
            if (wIt == sMasks.end()) {
                sMasks[wKey] = sMask;
            } else {
                wIt->second = (tShort)(wIt->second | sMask);
            }
        };

        const tShort wAllSides = (tShort)(tBorderTop + tBorderLeft + tBorderBottom + tBorderRight);

        switch (m_Border) {
            case tBorderAll: {
                for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                    for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                        wAdd(wRow, wCol, wAllSides);
                    }
                }
                break;
            }
            case tBorderOutside: {
                for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                    wAdd(sRow0, wCol, tBorderTop);
                    wAdd(sRow1, wCol, tBorderBottom);
                }
                for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                    wAdd(wRow, sCol0, tBorderLeft);
                    wAdd(wRow, sCol1, tBorderRight);
                }
                break;
            }
            case tBorderInside: {
                for (tIndex wRow = sRow0; wRow < sRow1; wRow++) {
                    for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                        wAdd(wRow, wCol, tBorderBottom);
                    }
                }
                for (tIndex wCol = sCol0; wCol < sCol1; wCol++) {
                    for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                        wAdd(wRow, wCol, tBorderRight);
                    }
                }
                break;
            }
            case tBorderHorizontal: {
                for (tIndex wRow = sRow0; wRow < sRow1; wRow++) {
                    for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                        wAdd(wRow, wCol, tBorderBottom);
                    }
                }
                break;
            }
            case tBorderVertical: {
                for (tIndex wCol = sCol0; wCol < sCol1; wCol++) {
                    for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                        wAdd(wRow, wCol, tBorderRight);
                    }
                }
                break;
            }
            case tBorderTop: {
                for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                    wAdd(sRow0, wCol, tBorderTop);
                }
                break;
            }
            case tBorderBottom: {
                for (tIndex wCol = sCol0; wCol <= sCol1; wCol++) {
                    wAdd(sRow1, wCol, tBorderBottom);
                }
                break;
            }
            case tBorderLeft: {
                for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                    wAdd(wRow, sCol0, tBorderLeft);
                }
                break;
            }
            case tBorderRight: {
                for (tIndex wRow = sRow0; wRow <= sRow1; wRow++) {
                    wAdd(wRow, sCol1, tBorderRight);
                }
                break;
            }
            default: break;
        }
    }

    void tUndoBorder::BuildCombinedCellMasks() {
        m_CombinedCellMasks.clear();
        const tSelect wSelect = m_SaveSelectColRow_Rows.Select();
        auto wMergePartial = [this](const std::map<tPoint, tShort>& sPartial) {
            for (const auto& wEntry : sPartial) {
                auto wIt = m_CombinedCellMasks.find(wEntry.first);
                if (wIt == m_CombinedCellMasks.end()) {
                    m_CombinedCellMasks[wEntry.first] = wEntry.second;
                } else {
                    wIt->second = (tShort)(wIt->second | wEntry.second);
                }
            }
        };
        for (auto wElem : *wSelect.VectorSelect()) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
            if (wRect != nullptr) {
                if (wRect->IsSheetSelect() || wRect->IsRowSelect() || wRect->IsColSelect()) {
                    continue;
                }
                std::map<tPoint, tShort> wPartial;
                FillCellMasks(wRect->Row(), wRect->Col(), wRect->Bottom(), wRect->Right(), wPartial);
                wMergePartial(wPartial);
            } else {
                std::map<tPoint, tShort> wPartial;
                FillCellMasks(wElem->Row(), wElem->Col(), wElem->Row(), wElem->Col(), wPartial);
                wMergePartial(wPartial);
            }
        }
    }

    void tUndoBorder::ApplyCombinedCellMasks() {
        for (auto& wEntry : m_CombinedCellMasks) {
            tTempoPoint wPoint(wEntry.first.Row(), wEntry.first.Col());
            ApplyBorder(&wPoint, wEntry.second);
        }
    }

    tBool tUndoBorder::CallBackCell(tTempoPoint* sPoint) {
        tBool wOk = true;
#ifdef debugundoredo
        tString wState;
        switch (UndoState()) {
        case tUndoState::BeforeDo: wState = "BeforeDo";  break;
        case tUndoState::Do: wState = "Do";  break;
        case tUndoState::BeforeUndo: wState = "BeforeUndo";  break;
        case tUndoState::Undo: wState = "Undo";  break;
        }
        cout << "tUndoBorder::" << wState << " CallBackCell " << sPoint->StrRef() << endl;
#endif

        if (m_UseCombinedCellMasks) {
            return(true);
        }

        // A single cell is just a 1x1 range; reuse FillCellMasks.
        std::map<tPoint, tShort> wMasks;
        FillCellMasks(sPoint->Row(), sPoint->Col(),
                      sPoint->Row(), sPoint->Col(), wMasks);

        switch (UndoState()) {
            case tUndoState::BeforeDo:
            case tUndoState::Do:
            case tUndoState::Undo: {
                for (auto& wEntry : wMasks) {
                    tTempoPoint wPoint(wEntry.first.Row(), wEntry.first.Col());
                    ApplyBorder(&wPoint, wEntry.second);
                }
                break;
            }
            case tUndoState::BeforeUndo: break;
        }
#ifdef debugundoredo
        cout  << endl;
#endif
        return(wOk);
    };

    tBool tUndoBorder::CallBackRange(tTempoRect* sRect) {
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
#ifdef debugundoredo
        cout << "tUndoBorder::CallBackRange " << sRect->StrRef() << endl;
#endif
        tString wBorder="";
        if (m_Format!="") {
            wBorder=BorderStr(m_Border)+":";
        }
        // Sheet ==============================================================
        if (sRect->IsSheetSelect()) {
            switch (UndoState()) {
                case tUndoState::BeforeDo: {
                    m_CssSheet=wSheet->Css();
                    break;
                }
                case tUndoState::Do: {
                    if (m_Format!="") {
                        tString wFormat=wBorder+m_Format;
                        wSheet->WorkBook()->ApplySheetFormat(wSheet,wFormat);
                    } else {
                        tFormatRef wFormatRef=wSheet->WorkBook()->DeleteBorder(m_CssSheet , m_Border);
                        wSheet->Css(wFormatRef);
                    }
                    break;
                }
                case tUndoState::BeforeUndo: break;
                case tUndoState::Undo: {
                    tUndoFormat::CallBackRange(sRect);
                }
            }
            return(true);
        }
        // Col or Row =========================================================
        if ((sRect->IsRowSelect()) || (sRect->IsColSelect())) {
            switch (UndoState()) {
                case tUndoState::BeforeDo: {
                    tUndoFormat::CallBackRange(sRect);
                    break;
                }
                case tUndoState::Do: {
                    if (m_Format!="") {
                        tString wFormat=wBorder+m_Format;
                        if (sRect->IsRowSelect()) {
                            for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                                wSheet->WorkBook()->ApplyColRowFormat(wSheet->EnsureRow(wRow),wFormat);
                            }
                        } else {
                            for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                                wSheet->WorkBook()->ApplyColRowFormat(wSheet->EnsureCol(wCol),wFormat);
                            }
                        }
                    } else {
                        if (sRect->IsRowSelect()) {
                            for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                                tColRow* wColRow=wSheet->Row(wRow);
                                tFormatRef wFormatRef=wSheet->WorkBook()->DeleteBorder(wColRow->Css(), m_Border);
                                wColRow->Css(wFormatRef);
                            }
                        } else {
                            for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                                tColRow* wColRow=wSheet->Col(wCol);
                                tFormatRef wFormatRef=wSheet->WorkBook()->DeleteBorder(wColRow->Css(), m_Border);
                                wColRow->Css(wFormatRef);
                            }
                        }
  
                    }
                    break;
                }
                case tUndoState::BeforeUndo: break;
                case tUndoState::Undo: {
                    if (sRect->IsSheetSelect()) {
                        if (wSheet->Css()!=0) {
                            wSheet->WorkBook()->DeleteCellFormat(wSheet->Css());
                        }
                        wSheet->Css(m_CssSheet);
                        m_CssSheet=0;
                    }
                    if (sRect->IsRowSelect()) {
                        for(tIndex wRow=sRect->Top();wRow<=sRect->Bottom(); wRow++) {
                            tSaveColRow* wSaveColRow=m_SaveSelectColRow_Rows.FindColRow(wRow);
                            if (wSaveColRow!=nullptr) {
                                tColRow* wColRow=wSheet->Row(wRow);
                                // Raz Old
                                if (wColRow->Css()!=0) {
                                    wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                }
                                wColRow->Css(wSaveColRow->Css());
                                wSaveColRow->Css(0);
                            }  else {
                                tColRow* wColRow=wSheet->Row(wRow);
                                if (wColRow!=nullptr) {
                                    if (wColRow->Css()!=0) {
                                        wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                        wColRow->Css(0);
                                    }
                                }
                            }
                        }
                    }
                    if (sRect->IsColSelect()) {
                        for(tIndex wCol=sRect->Left();wCol<=sRect->Right(); wCol++) {
                            tSaveColRow* wSaveColRow=m_SaveSelectColRow_Cols.FindColRow(wCol);
                            if (wSaveColRow!=nullptr) {
                                tColRow* wColRow=wSheet->Col(wCol);
                                // Raz Old
                                if (wColRow->Css()!=0) {
                                    wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                }
                                wColRow->Css(wSaveColRow->Css());
                                wSaveColRow->Css(0);
                                
                            } else {
                                tColRow* wColRow=wSheet->Col(wCol);
                                if (wColRow!=nullptr) {
                                    if (wColRow->Css()!=0) {
                                        wSheet->WorkBook()->DeleteCellFormat(wColRow->Css());
                                        wColRow->Css(0);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            return(true);
        }
        // Else CallBack Cell =================================================
        // Combined masks are applied once in Do()/Undo(); skip per-range cell apply here.
        if (m_UseCombinedCellMasks) {
            return(true);
        }
        // We precompute the final (cell -> combined mask) map so every cell
        // is visited exactly once within this range callback.
        switch (UndoState()) {
            case tUndoState::BeforeUndo : break;
            case tUndoState::BeforeDo:
            case tUndoState::Undo:
            case tUndoState::Do : {
                std::map<tPoint, tShort> wMasks;
                FillCellMasks(sRect->Row(), sRect->Col(),
                              sRect->Bottom(), sRect->Right(), wMasks);
                for (auto& wEntry : wMasks) {
                    tTempoPoint wPoint(wEntry.first.Row(), wEntry.first.Col());
                    ApplyBorder(&wPoint, wEntry.second);
                }
                break;
            }
        }
        return(true);
    }

    tBool tUndoBorder::Do() {
        m_SaveSelectColRow_Rows.Clear();
        m_SaveSelectColRow_Cols.Clear();
        m_ContainerCellDo.Container()->clear();
        m_CombinedCellMasks.clear();
        m_UseCombinedCellMasks = false;

        if (!m_SaveSelectColRow_Rows.ParseSelect(m_RefRebase)) {
            return(false);
        }

        BuildCombinedCellMasks();
        m_UseCombinedCellMasks = !m_CombinedCellMasks.empty();

        tBool wResult = false;
        tSelect wSelect = m_SaveSelectColRow_Rows.Select();

        UndoState(tUndoState::BeforeDo);
        if (m_UseCombinedCellMasks) {
            ApplyCombinedCellMasks();
        }
        wResult = wSelect.CallBack(this);
        if (wResult) {
            UndoState(tUndoState::Do);
            if (m_UseCombinedCellMasks) {
                ApplyCombinedCellMasks();
            }
            wResult = wSelect.CallBack(this);
        }

#ifdef debugundoredo
        cout << "tUndoBorder::Do" << endl;
#endif

#ifdef checksp
        Sheet()->Check();
#endif

        m_UseCombinedCellMasks = false;
        return(wResult);
    }

    tBool tUndoBorder::Undo() {
        m_ContainerCellDo.Container()->clear();
        m_CombinedCellMasks.clear();
        m_UseCombinedCellMasks = false;

        m_SaveSelectColRow_Rows.ParseSelect(m_RefRebase);
        BuildCombinedCellMasks();
        m_UseCombinedCellMasks = !m_CombinedCellMasks.empty();

        tBool wResult = true;
        tSelect wSelect = m_SaveSelectColRow_Rows.Select();

        UndoState(tUndoState::BeforeUndo);
        wResult = wSelect.CallBack(this);
        if (wResult) {
            UndoState(tUndoState::Undo);
            if (m_UseCombinedCellMasks) {
                ApplyCombinedCellMasks();
            }
            wResult = wSelect.CallBack(this);
        }

#ifdef checksp
        Sheet()->Check();
#endif

        m_UseCombinedCellMasks = false;
        return(wResult);
    }
    
    void tUndoBorder::Json(Writer<StringBuffer>* sWriter) {
        tUndoFormat::Json(sWriter);
        sWriter->Key(kJsonKeyBorder);
        sWriter->Int(m_Border);
    }

    void tUndoBorder::Json(const rapidjson::Value& sValue) {
        tUndoFormat::Json(sValue);
        m_Border = sValue[kJsonKeyBorder].GetInt();
    }
    
    
	//=========================================================================
	//! Undo to apply Paste
    tUndoPaste::tUndoPaste() : tUndoRaz("",tMode(),nullptr),m_Copy("") {
    }

    tUndoPaste::tUndoPaste(tString sRef, tMode sMode, tSheet* sSheet, tBool sLoadFromClipboard)
        : tUndoRaz(sRef, sMode, sSheet), m_Copy("") {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Paste " << sRef;
            OperationName(wStream.str());
        }
        if (sLoadFromClipboard) {
            m_Copy = tApplication::Instance()->Clipboard()->Text();
        }
 	}

    tString  tUndoPaste::ClassName() const { return("tUndoPaste"); }

    tBool tUndoPaste::TreatSelection(tBool sIsDo) {
        // If Clipboard is empty, do nothing
        if (m_Copy.empty()) {
            return(false);
        }
        // Get Sheet By Allocator
        tSheet* wSheet=Sheet();
        (void)wSheet;
        // Make TopRefPoint
        //m_SelectDest="";
        if (m_SaveSelect.ParseSelect(m_RefRebase)) {
            // Spill origins broken by paste into MatExtend cells — recalc after JsonEnd → #SPILL!.
            std::vector<tCell*> wSpillOriginsToRecalc;
            auto wBreakSpillBeforePasteWrite = [&](tCell* sCell) {
                if (sCell == nullptr || !sCell->IsMatExtend()) {
                    return;
                }
                std::vector<tCell*> wSpillCells;
                sCell->AppendCellsInSpillRange(wSpillCells);
                for (tCell* wSpillCell : wSpillCells) {
                    if (wSpillCell != nullptr) {
                        m_SaveSelect.AddCell(wSpillCell);
                    }
                }
                tCell* wOrigin = sCell->BreakSpillForUserOverwrite();
                if (wOrigin == nullptr) {
                    return;
                }
                for (tCell* wExisting : wSpillOriginsToRecalc) {
                    if (wExisting == wOrigin) {
                        return;
                    }
                }
                wSpillOriginsToRecalc.push_back(wOrigin);
            };
            tSpreadSheetContainer::Instance()->JsonBegin();
            
            // Paste
            tSelect wSelectDest = m_SaveSelect.Select();
            tTempoPoint  wTopLeftPoint= *wSelectDest.ReturnTopLeftPoint();
            Document wDocument;
#ifdef debugundoredo
            cout << endl << "Copy:" << m_Copy << endl;
#endif
            wDocument.Parse(m_Copy.c_str());
            // Read Shared
            tSpreadSheetContainer::Instance()->JsonShared(wDocument);
            
            tFormatApi* wFormatApi=wSheet->WorkBook()->FormatApi();
            if (wDocument.HasMember("f")) {
                const rapidjson::Value& wFormats= wDocument["f"];
                if (wFormatApi!=nullptr) {
                    wFormatApi->Json(wFormats);
                }
            }
           
 
            tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
            const rapidjson::Value& wValueSelect = wDocument["select"];
            tSelect* wSelectOrig = new tSelect();
            wSelectOrig->Json(wValueSelect);

            const rapidjson::Value& wCells = wDocument["cells"];
            tBool wCopyRect = false;
            // Is Rect Destination ========================================
            // For multiple copy
            if (m_SaveSelect.Select().JustOneRect()) {
                if ((wSelectOrig->JustOneCell()) ||
                    (wSelectOrig->JustOneRect())) {
                    tIndex wWOrig = 1;
                    tIndex wHOrig = 1;
                    if (wSelectOrig->JustOneRect()) {
                        wHOrig = wSelectOrig->FirstRect()->Bottom() - wSelectOrig->FirstRect()->Top()+1;
                        wWOrig = wSelectOrig->FirstRect()->Right() - wSelectOrig->FirstRect()->Left()+1;
                    }
                    tIndex wHDest = wSelectDest.FirstRect()->Bottom() - wSelectDest.FirstRect()->Top()+1;
                    tIndex wWDest = wSelectDest.FirstRect()->Right() - wSelectDest.FirstRect()->Left()+1;
                    // Look if destination contain multiple view of source
                    // Use Modulo and Div for calculate destination
                    tBool wOkRow = (wHDest % wHOrig == 0);
                    tBool wOkCol = (wWDest % wWOrig == 0);
                    if (wOkRow && wOkCol) {
                        wCopyRect=true;
                        // Loop on row
                        for (tIndex wBlockRow = 0; wBlockRow < (wHDest / wHOrig); wBlockRow++) {
                            // Loop on col
                            for (tIndex wBlockCol = 0; wBlockCol < (wWDest / wWOrig); wBlockCol++) {
                                // Loop and json value
                                for (SizeType wIndex = 0; wIndex < wCells.Size(); wIndex++) {
                                    const rapidjson::Value& wJsonCell = wCells[wIndex];
                                    // Add Block coordinate
                                    tTempoPoint wPoint(wJsonCell["c"].GetString());
                                    tIndex wIndexRow = wPoint.Row() + (wBlockRow * wHOrig);
                                    tIndex wIndexCol = wPoint.Col() + (wBlockCol * wWOrig);
                                    // Add Destination coordinate
                                    wIndexRow += wTopLeftPoint.Row();
                                    wIndexCol += wTopLeftPoint.Col();
                                    // Add Cell In List
                                    //m_ContainerCellDo.InsertClass(tPoint(wIndexRow,wIndexCol));
                                    //  Get Cell
                                    tCell* wCell = wColRowCellRange->Cell(wIndexRow, wIndexCol);
                                    
                                    if (wCell!=nullptr) {
                                        if (sIsDo) {
                                            UndoState(tUndoState::BeforeDo);
                                            CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                                        } else {
                                            UndoState(tUndoState::BeforeUndo);
                                            CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                                            UndoState(tUndoState::Undo);
                                            CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                                        }
                                    }
                                    if (sIsDo) {
                                        wCell = wColRowCellRange->EnsureCell(wIndexRow, wIndexCol);
                                        wBreakSpillBeforePasteWrite(wCell);
                                        wCell->Json(wJsonCell);
                                        m_SaveSelect.PushCalculate(wCell);
                                    } else {
                                        wCell = wColRowCellRange->Cell(wIndexRow, wIndexCol);
                                    }
                                    //  Call Json for cell
                                    if (wCell!=nullptr) {
                                        m_SaveSelect.PushCalculate(wCell);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Not multiple copy ==================================================
            if (!wCopyRect) {
                //  Just One pass
                tClassVector<tPoint>  wContainerCellDo;
                // Copy All Cell with diff with TopLeftPoint
                for (SizeType wIndex = 0; wIndex < wCells.Size(); wIndex++) {
                    const rapidjson::Value& wJsonCell = wCells[wIndex];
                    tTempoPoint wPoint(wJsonCell["c"].GetString());
                    tIndex wIndexRow = wPoint.Row();
                    tIndex wIndexCol = wPoint.Col();
                    // Get destination of cell
                    wIndexRow += wTopLeftPoint.Row();
                    wIndexCol += wTopLeftPoint.Col();
                    if (!wContainerCellDo.Exist(tPoint(wIndexRow,wIndexCol))) {
                        // Add Cell In List
                        wContainerCellDo.InsertClass(tPoint(wIndexRow,wIndexCol));
                        //  Get Cell
                        tCell* wCell = wColRowCellRange->Cell(wIndexRow, wIndexCol);
                        
                        if (wCell!=nullptr) {
                            if (sIsDo) {
                                UndoState(tUndoState::BeforeDo);
                                CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                            } else {
                                UndoState(tUndoState::BeforeUndo);
                                CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                                UndoState(tUndoState::Undo);
                                CallBackCell(new tTempoPoint(wIndexRow,wIndexCol));
                            }
                        }
                        if (sIsDo) {
                            wCell = wColRowCellRange->EnsureCell(wIndexRow, wIndexCol);
                            wBreakSpillBeforePasteWrite(wCell);
                            wCell->Json(wJsonCell);
                        } else {
                            wCell = wColRowCellRange->Cell(wIndexRow, wIndexCol);
                        }
                        //  Call Json for cell
                        if (wCell!=nullptr) {
                            m_SaveSelect.PushCalculate(wCell);
                        }
                    }
                }
            }
            if (sIsDo) {
                // Paste Do: compile/calculate destination cells only — not full JsonCompil (would recalc source).
                tSpreadSheetContainer::Instance()->JsonEndCellsOnly();
                // Origins whose spill was torn down by paste into a slave → #SPILL!.
                for (tCell* wOrigin : wSpillOriginsToRecalc) {
                    if (wOrigin != nullptr) {
                        wOrigin->Calculation();
                    }
                }
            } else {
                tSpreadSheetContainer::Instance()->JsonEndShared();
            }
#ifdef _debugleak
            delete(wSelectOrig);
#endif
            return(true);
        }
        return(false);
    }

	tBool tUndoPaste::Do() {
        if (IsJson() && IsCollaborationPastePayloadTooLarge(m_Copy.size())) {
            const tString wMessage(CollaborationPasteTooLargeMessage());
            Error(wMessage);
            tLemonInterface* wLemon = tSpreadSheetContainer::Instance()->LemonInterface();
            if (wLemon != nullptr) {
                wLemon->Error(wMessage, 0, 0);
            }
            return false;
        }
        m_SaveSelect.Clear();
        const tBool wResult = TreatSelection(true);
        // Paste Do does not call CalculateDo(); discard cell pointers queued during TreatSelection.
        m_SaveSelect.ClearCellCalculate();
        return(wResult);
	};
    
    tBool tUndoPaste::Undo() {
        // Undo may delete pasted cells; drop stale pointers left from paste Do().
        m_SaveSelect.ClearCellCalculate();
        if (!TreatSelection(false)) {
            return(false);
        }
        // Spill siblings snapshotted when paste hit a MatExtend cell may sit outside the
        // paste destination set — TreatSelection only restores dest cells.
        m_SaveSelect.UndoCellsOutsideSelect();
        CalculateDo();
        return(true);
    }

    void tUndoPaste::Json(Writer<StringBuffer>* sWriter) {
        tUndoRaz::Json(sWriter);
        sWriter->Key(kJsonKeyCopy);
        sWriter->String(m_Copy.c_str());
    }

    void tUndoPaste::Json(const rapidjson::Value& sValue) {
        tUndoRaz::Json(sValue);
        m_Copy = sValue[kJsonKeyCopy].GetString();
    }

    tBool tUndoPaste::Rebase() {
        // Call parent Rebase to rebase m_RefRebase and m_SaveSelect
        return(tUndoSpreadSheetCallBack::Rebase());
    }

    void tUndoPaste::DeleteBeforeDo() {
        // Call parent to reset reference rebase and clear save select
        tUndoRaz::DeleteBeforeDo();
    }

	//=========================================================================
	//! Undo for cut selection
    tUndoCut::tUndoCut() : tUndoRaz("", tMode(), nullptr), m_Copy("") {
    }

    tUndoCut::tUndoCut(tString sRef, tMode sMode, tSheet* sSheet)
        : tUndoRaz(sRef, sMode, sSheet), m_Copy("") {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Cut " << sRef;
            OperationName(wStream.str());
        }
    }

    tString tUndoCut::ClassName() const { return("tUndoCut"); }

    tBool tUndoCut::EnsureCopyPayload() {
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return(false);
        }
        if (!CopySelectionToJson(m_RefRebase, wSheet, m_Copy, false) || m_Copy.empty()) {
            return(false);
        }
        return(CopyJsonHasCellPayload(m_Copy));
    }

    void tUndoCut::ApplyCopyToClipboard() {
        if (!m_Copy.empty()) {
            tApplication::Instance()->Clipboard()->Text(m_Copy);
        }
    }

    tBool tUndoCut::Do() {
        if (IsJson() && IsCollaborationPastePayloadTooLarge(m_Copy.size())) {
            const tString wMessage(CollaborationPasteTooLargeMessage());
            Error(wMessage);
            tLemonInterface* wLemon = tSpreadSheetContainer::Instance()->LemonInterface();
            if (wLemon != nullptr) {
                wLemon->Error(wMessage, 0, 0);
            }
            return(false);
        }
        if (!IsJson()) {
            if (!EnsureCopyPayload()) {
                return(false);
            }
        } else if (!CopyJsonHasCellPayload(m_Copy)) {
            return(false);
        }
        ApplyCopyToClipboard();
        return(tUndoRaz::Do());
    }

    void tUndoCut::Json(Writer<StringBuffer>* sWriter) {
        tUndoRaz::Json(sWriter);
        sWriter->Key(kJsonKeyCopy);
        sWriter->String(m_Copy.c_str());
    }

    void tUndoCut::Json(const rapidjson::Value& sValue) {
        tUndoRaz::Json(sValue);
        if (sValue.HasMember(kJsonKeyCopy)) {
            m_Copy = sValue[kJsonKeyCopy].GetString();
        }
    }

    tBool tUndoCut::MatchesCutClipboard() const {
        if (m_Copy.empty() || !CopyJsonHasCellPayload(m_Copy)) {
            return(false);
        }
        return(tApplication::Instance()->Clipboard()->Text() == m_Copy);
    }

    void tUndoCut::DeleteBeforeDo() {
        tUndoRaz::DeleteBeforeDo();
        m_Copy.clear();
    }

	//=========================================================================
	//! Undo for move selection
    tUndoMove::tUndoMove() : tUndoPaste(), m_SourceRef(""), m_SourceRefRebase(""), m_DidGridRelocate(false),
                             m_GridRelocateMovedCells(false), m_SourceAlreadyCleared(false) {
    }

    tUndoMove::tUndoMove(tString sSourceRef, tString sDestRef, tMode sMode, tSheet* sSheet)
        : tUndoPaste(sDestRef, sMode, sSheet, false), m_SourceRef(sSourceRef), m_SourceRefRebase(sSourceRef),
          m_DidGridRelocate(false), m_GridRelocateMovedCells(false), m_SourceAlreadyCleared(false) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Move " << sSourceRef << " -> " << sDestRef;
            OperationName(wStream.str());
        }
    }

    void tUndoMove::AdoptCompletedCut(tUndoCut* sCut) {
        if (sCut == nullptr) {
            return;
        }
        m_Copy = sCut->CopyPayload();
        if (tSaveSelect* wCutSave = sCut->SaveSelect()) {
            m_SaveSelect.AbsorbFrom(*wCutSave);
        }
        m_SourceAlreadyCleared = true;
    }

    tBool tUndoMove::EnsureCopyPayload() {
        if (!m_Copy.empty()) {
            return(true);
        }
        if (kUndoMoveUseGridRelocate) {
            tRect wSourceRect;
            tRect wDestRect;
            if (GetSourceDestRects(wSourceRect, wDestRect)) {
                return(true);
            }
        }
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            m_Copy.clear();
            return(false);
        }
        if (!CopySelectionToJson(m_SourceRefRebase, wSheet, m_Copy, false) || m_Copy.empty()) {
            m_Copy.clear();
            return(false);
        }
        return(true);
    }

    tString tUndoMove::ClassName() const { return("tUndoMove"); }

    tBool tUndoMove::MoveSelectSameGeometry(const tSelect& sSource, const tSelect& sDest) const {
        if (!((sSource.JustOneCell() || sSource.JustOneRect()) &&
              (sDest.JustOneCell() || sDest.JustOneRect()))) {
            return(false);
        }
        tIndex wSrcH = 1;
        tIndex wSrcW = 1;
        tIndex wDstH = 1;
        tIndex wDstW = 1;
        if (sSource.JustOneRect()) {
            wSrcH = sSource.FirstRect()->Bottom() - sSource.FirstRect()->Top() + 1;
            wSrcW = sSource.FirstRect()->Right() - sSource.FirstRect()->Left() + 1;
        }
        if (sDest.JustOneRect()) {
            wDstH = sDest.FirstRect()->Bottom() - sDest.FirstRect()->Top() + 1;
            wDstW = sDest.FirstRect()->Right() - sDest.FirstRect()->Left() + 1;
        }
        return(wSrcH == wDstH && wSrcW == wDstW);
    }

    tBool tUndoMove::GetSourceDestRects(tRect& oSource, tRect& oDest) const {
        tSelect wSource;
        tSelect wDest;
        if (!wSource.Parse(m_SourceRefRebase) || !wDest.Parse(m_RefRebase)) {
            return(false);
        }
        if (!MoveSelectSameGeometry(wSource, wDest)) {
            return(false);
        }
        auto wFillRect = [](const tSelect& sSelect, tRect& oRect) -> tBool {
            if (tTempoRect* wRect = sSelect.FirstRect()) {
                oRect = *wRect;
                return(true);
            }
            if (tTempoPoint* wPoint = sSelect.FirstPoint()) {
                oRect.Row(wPoint->Row());
                oRect.Col(wPoint->Col());
                oRect.Bottom(wPoint->Row());
                oRect.Right(wPoint->Col());
                return(true);
            }
            return(false);
        };
        return(wFillRect(wSource, oSource) && wFillRect(wDest, oDest));
    }

    tBool tUndoMove::RectsOverlap(tRect& sA, tRect& sB) {
        return(RectsOverlapImpl(sA, sB));
    }

    tBool tUndoMove::DoGridRelocate(tSelect& sSource, tSelect& sDest,
                                    tRect& sSourceRect, tRect& sDestRect) {
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return(false);
        }
        tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(false);
        }

        m_SaveSelect.Clear();
        m_SaveSelect.ParseSelect(m_RefRebase);

        UndoState(tUndoState::BeforeDo);
        SaveSelectCellsForGridMove(this, sSource, false, false);
        if (SelectHasNonEmptyCell(wSheet, sSource)) {
            (void)sDest.CallBack(this);
        }

        std::vector<tCell*> wRecompilDependents;
        CollectFormulaDependentsFromRect(wColRowCellRange, sSourceRect, wRecompilDependents);
        CollectFormulaDependentsFromRect(wColRowCellRange, sDestRect, wRecompilDependents);

        m_GridRelocateMovedCells = SourceRectHasMovingCells(wColRowCellRange, sSourceRect);
        if (!wColRowCellRange->RelocateRect(sSourceRect, sDestRect)) {
            m_SaveSelect.Clear();
            m_DidGridRelocate = false;
            m_GridRelocateMovedCells = false;
            return(false);
        }

        RemapFormulaReferences(true);
        RecompilFormulaDependentCells(wSheet->WorkBook(), wRecompilDependents);
        if (!IsJson()) {
            ClearMoveSourceExteriorNeighborGhosts(wSheet, sSource, sDest);
        }
        m_SaveSelect.ClearCellCalculate();
        CalculateDo();
        m_DidGridRelocate = true;
        return(true);
    }

    tBool tUndoMove::UndoGridRelocate(tSelect& sDest) {
        (void)sDest;
        tRect wSourceRect;
        tRect wDestRect;
        if (!GetSourceDestRects(wSourceRect, wDestRect)) {
            return(false);
        }
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return(false);
        }
        tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(false);
        }

        std::vector<tCell*> wRecompilDependents;
        CollectFormulaDependentsFromRect(wColRowCellRange, wSourceRect, wRecompilDependents);
        CollectFormulaDependentsFromRect(wColRowCellRange, wDestRect, wRecompilDependents);

        if (!m_GridRelocateMovedCells) {
            RemapFormulaReferences(false);
            CalculateDo();
            return(true);
        }

        if (!wColRowCellRange->RelocateRect(wDestRect, wSourceRect)) {
            return(false);
        }

        UndoState(tUndoState::Undo);
        ClearUnsavedDestCells(this, sDest, wSourceRect, wDestRect);
        UndoState(tUndoState::BeforeUndo);
        (void)sDest.CallBack(this);
        UndoState(tUndoState::Undo);
        (void)sDest.CallBack(this);

        UndoSourceOutsideDest();
        RemapFormulaReferences(false);
        QueueUndoRestoredCellsForCalculate();
        RecompilFormulaDependentCells(wSheet->WorkBook(), wRecompilDependents);
        CalculateDo();
        return(true);
    }

    void tUndoMove::RemapFormulaReferences(tBool sForward) {
        tRect wSourceRect;
        tRect wDestRect;
        if (!GetSourceDestRects(wSourceRect, wDestRect)) {
            return;
        }

        const tIndex wDeltaRow = wDestRect.Top() - wSourceRect.Top();
        const tIndex wDeltaCol = wDestRect.Left() - wSourceRect.Left();
        if (wDeltaRow == 0 && wDeltaCol == 0) {
            return;
        }

        const tRect wRemapRect = sForward ? wSourceRect : wDestRect;
        const tIndex wApplyDeltaRow = sForward ? wDeltaRow : -wDeltaRow;
        const tIndex wApplyDeltaCol = sForward ? wDeltaCol : -wDeltaCol;

        tWorkBook* wWorkBook = Sheet()->WorkBook();
        if (wWorkBook == nullptr) {
            return;
        }

        const tAllocatorRef wMoveSheetRef = Sheet()->AllocatorRef();
        const tVectorSheet wSheets = wWorkBook->VectorPtSheet();
        std::vector<tSheet*> wAllSheets(wSheets.begin(), wSheets.end());
        if (tSheet* wAnchorSheet = wWorkBook->SheetClassAnchor()) {
            wAllSheets.push_back(wAnchorSheet);
        }
        for (tSheet* wSheet : wAllSheets) {
            if (wSheet == nullptr) {
                continue;
            }
            tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
            if (wColRowCellRange == nullptr) {
                continue;
            }
            const tIndex wLastRow = wSheet->LastRow();
            const tIndex wLastCol = wSheet->LastCol();
            if (wLastRow < 0 || wLastCol < 0) {
                continue;
            }
            auto wRemapFormulaCell = [&](tCell* sFormulaCell) {
                if (sFormulaCell == nullptr) {
                    return;
                }
                tString wRemapText;
                if (sFormulaCell->Value().HasFormula()) {
                    wRemapText = sFormulaCell->Value().Str();
                } else if (sFormulaCell->Formula() != nullptr) {
                    wRemapText = sFormulaCell->FormulaStr();
                    if (wRemapText.empty()) {
                        wRemapText = sFormulaCell->Formula()->FormulaKey();
                    }
                } else {
                    wRemapText = sFormulaCell->Value().Str();
                }
                if (wRemapText.empty()) {
                    return;
                }
                const tIndex wRow = sFormulaCell->RowIndex();
                const tIndex wCol = sFormulaCell->ColIndex();
                tUndoRedoRebaseFormula wRemapFormula(wMoveSheetRef, wRow, wCol, wRemapText);
                // Option A: only touch formulas that reference the moved block (skip e.g. D15 / AnnéeSélectionnée).
                if (!wRemapFormula.ReferencesMoveRect(wMoveSheetRef, wRemapRect)) {
                    return;
                }
                if (!wRemapFormula.RemapForMove(wMoveSheetRef,
                                                wRemapRect,
                                                wApplyDeltaRow,
                                                wApplyDeltaCol)) {
                    return;
                }
                tString wNewText = wRemapFormula.Formula();
                if (wNewText == wRemapText) {
                    return;
                }
                const tBool wIsAttribute = (sFormulaCell->Type() == tTypeItem::t_Attribute);
                if (sFormulaCell->Formula() != nullptr || wIsAttribute) {
                    if (!wNewText.empty() && wNewText[0] != '=') {
                        wNewText.insert(0, 1, '=');
                    }
                    tVariant wValue(wNewText);
                    wWorkBook->CellValue(sFormulaCell, wValue, false);
                } else {
                    tVariant wValue(wNewText);
                    wWorkBook->CellValue(sFormulaCell, wValue, false);
                }
                if (wIsAttribute) {
                    (void)sFormulaCell->Calculation();
                }
                m_SaveSelect.PushCalculate(sFormulaCell);
            };
            auto wRemapClassAttributes = [&](tCell* sCell) {
                if (sCell == nullptr) {
                    return;
                }
                if (tCellClassAttribute* wCellClass = sCell->ClassAttribute()) {
                    tVectorCellAttribute wAttributes;
                    wCellClass->CellAttribute(&wAttributes);
                    for (tCellAttribute* wAttribute : wAttributes) {
                        wRemapFormulaCell(wAttribute);
                    }
                }
            };
            // Remap grid cells and class attributes once per sheet. Do not also walk
            // CellClassContainer: the same attributes would be remapped twice (2x delta).
            for (tIndex wRow = 0; wRow <= wLastRow; wRow++) {
                for (tIndex wCol = 0; wCol <= wLastCol; wCol++) {
                    tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                    if (wCell == nullptr) {
                        continue;
                    }
                    wRemapFormulaCell(wCell);
                    wRemapClassAttributes(wCell);
                }
            }
        }
    }

    tBool tUndoMove::SelectContainsCell(const tSelect& sSelect, tIndex sRow, tIndex sCol) const {
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

    void tUndoMove::UndoSourceOutsideDest() {
        tSelect wSource;
        tSelect wDest;
        if (!wSource.Parse(m_SourceRefRebase) || !wDest.Parse(m_RefRebase)) {
            return;
        }
        UndoState(tUndoState::Undo);
        tVectorTempoPoint* wVector = wSource.VectorSelect();
        if (wVector == nullptr) {
            return;
        }
        for (auto wItem : *wVector) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
            if (wRect != nullptr) {
                for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                    for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                        if (!SelectContainsCell(wDest, wRow, wCol)) {
                            tTempoPoint wPoint(wRow, wCol);
                            CallBackCell(&wPoint);
                        }
                    }
                }
            } else if (wItem != nullptr) {
                if (!SelectContainsCell(wDest, wItem->Row(), wItem->Col())) {
                    CallBackCell(wItem);
                }
            }
        }
    }

    void tUndoMove::QueueUndoRestoredCellsForCalculate() {
        tSelect wSource;
        tSelect wDest;
        if (!wSource.Parse(m_SourceRefRebase) || !wDest.Parse(m_RefRebase)) {
            return;
        }
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
        tVectorTempoPoint* wVector = wSource.VectorSelect();
        if (wVector == nullptr) {
            return;
        }
        for (auto wItem : *wVector) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
            if (wRect != nullptr) {
                for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                    for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                        if (!SelectContainsCell(wDest, wRow, wCol)) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                m_SaveSelect.PushCalculate(wCell);
                            }
                        }
                    }
                }
            } else if (wItem != nullptr) {
                if (!SelectContainsCell(wDest, wItem->Row(), wItem->Col())) {
                    tCell* wCell = wColRowCellRange->Cell(wItem->Row(), wItem->Col());
                    if (wCell != nullptr) {
                        m_SaveSelect.PushCalculate(wCell);
                    }
                }
            }
        }
        // Destination cells restored by the paste-undo leg may also need recalc.
        wVector = wDest.VectorSelect();
        if (wVector == nullptr) {
            return;
        }
        for (auto wItem : *wVector) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
            if (wRect != nullptr) {
                for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                    for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                        tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                        if (wCell != nullptr) {
                            m_SaveSelect.PushCalculate(wCell);
                        }
                    }
                }
            } else if (wItem != nullptr) {
                tCell* wCell = wColRowCellRange->Cell(wItem->Row(), wItem->Col());
                if (wCell != nullptr) {
                    m_SaveSelect.PushCalculate(wCell);
                }
            }
        }
    }

    void tUndoMove::UndoDestWhenCopyHasNoCells() {
        if (CopyJsonHasCellPayload(m_Copy)) {
            return;
        }
        tSelect wDest;
        if (!wDest.Parse(m_RefRebase)) {
            return;
        }
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return;
        }
        tVectorTempoPoint* wVector = wDest.VectorSelect();
        if (wVector == nullptr) {
            return;
        }
        const tAllocatorRef wAllocator = wSheet->IndexAllocatorColRowCellRange();
        auto wRestoreSavedCell = [&](tIndex sRow, tIndex sCol) {
            tSaveCell* wSaveCell = m_SaveSelect.FindSaveCell(
                wAllocator, tTypeItem::t_Cell, sRow, sCol, "");
            if (wSaveCell != nullptr) {
                m_SaveSelect.UndoCell(wSaveCell);
            }
        };
        for (auto wItem : *wVector) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
            if (wRect != nullptr) {
                for (tIndex wRow = wRect->Top(); wRow <= wRect->Bottom(); wRow++) {
                    for (tIndex wCol = wRect->Left(); wCol <= wRect->Right(); wCol++) {
                        wRestoreSavedCell(wRow, wCol);
                    }
                }
            } else if (wItem != nullptr) {
                wRestoreSavedCell(wItem->Row(), wItem->Col());
            }
        }
    }

    tBool tUndoMove::Do() {
        m_DidGridRelocate = false;
        m_GridRelocateMovedCells = false;
        tSelect wSource;
        tSelect wDest;
        if (!wSource.Parse(m_SourceRefRebase) || !wDest.Parse(m_RefRebase)) {
            return(false);
        }
        if (!MoveSelectSameGeometry(wSource, wDest)) {
            return(false);
        }

        if (m_SourceAlreadyCleared) {
            if (m_Copy.empty() || !CopyJsonHasCellPayload(m_Copy)) {
                return(false);
            }
            m_SaveSelect.ParseSelect(m_RefRebase);
            if (!TreatSelection(true)) {
                return(false);
            }
            if (!IsJson()) {
                ClearMoveSourceExteriorNeighborGhosts(Sheet(), wSource, wDest);
            }
            m_SaveSelect.ClearCellCalculate();
            return(true);
        }

        if (kUndoMoveUseGridRelocate) {
            tRect wSourceRect;
            tRect wDestRect;
            if (GetSourceDestRects(wSourceRect, wDestRect) &&
                !MoveDestHasPriorContent(Sheet(), wDestRect)) {
                if (DoGridRelocate(wSource, wDest, wSourceRect, wDestRect)) {
                    return(true);
                }
            }
        }

        if (m_Copy.empty()) {
            tSheet* wSheet = Sheet();
            if (wSheet == nullptr) {
                return(false);
            }
            if (!CopySelectionToJson(m_SourceRefRebase, wSheet, m_Copy, false)) {
                m_Copy.clear();
            }
        }
        if (!EnsureCopyPayload()) {
            return(false);
        }
        if (m_Copy.empty()) {
            return(false);
        }
        if (!wSource.Parse(m_SourceRefRebase) || !wDest.Parse(m_RefRebase)) {
            return(false);
        }
        if (!MoveSelectSameGeometry(wSource, wDest)) {
            return(false);
        }
        if (SelectHasNonEmptyCell(Sheet(), wSource) && !CopyJsonHasCellPayload(m_Copy)) {
            return(false);
        }

        m_SaveSelect.Clear();
        m_SaveSelect.ParseSelect(m_RefRebase);

        UndoState(tUndoState::BeforeDo);
        wSource.CallBack(this);
        // Empty copy has no paste payload; BeforeDo PassCss on dest would strip live format
        // without TreatSelection restoring it (cells[] is empty).
        if (CopyJsonHasCellPayload(m_Copy)) {
            wDest.CallBack(this);
        }

        // Copy -> Raz(source) -> Paste(dest) so overlapping shifts stay consistent.
        UndoState(tUndoState::Do);
        wSource.CallBack(this);

        CalculateDo();
        if (CopyJsonHasCellPayload(m_Copy)) {
            if (!TreatSelection(true)) {
                return(false);
            }
            if (!IsJson()) {
                ClearMoveSourceExteriorNeighborGhosts(Sheet(), wSource, wDest);
            }
            // TreatSelection queues raw cell pointers; RemapFormulaReferences / CalculateDo
            // must not keep them (AddSelect resolves dest cells by coordinates).
            m_SaveSelect.ClearCellCalculate();
        }
        RemapFormulaReferences(true);
        CalculateDo();
        return(true);
    }

    tBool tUndoMove::Undo() {
        if (kUndoMoveUseGridRelocate && m_DidGridRelocate) {
            tRect wSourceRect;
            tRect wDestRect;
            tSelect wDest;
            if (wDest.Parse(m_RefRebase) &&
                GetSourceDestRects(wSourceRect, wDestRect)) {
                if (UndoGridRelocate(wDest)) {
                    return(true);
                }
            }
        }

        if (CopyJsonHasCellPayload(m_Copy)) {
            if (!TreatSelection(false)) {
                return(false);
            }
        } else {
            UndoDestWhenCopyHasNoCells();
        }
        UndoSourceOutsideDest();
        // Drop cell pointers queued during TreatSelection; layout is restored below.
        m_SaveSelect.ClearCellCalculate();
        RemapFormulaReferences(false);
        QueueUndoRestoredCellsForCalculate();
        CalculateDo();
        return(true);
    }

    void tUndoMove::Json(Writer<StringBuffer>* sWriter) {
        tUndoPaste::Json(sWriter);
        sWriter->Key(kJsonKeyMoveSource);
        sWriter->String(m_SourceRefRebase.c_str());
        sWriter->Key(kJsonKeyDidGridRelocate);
        sWriter->Bool(m_DidGridRelocate);
        sWriter->Key(kJsonKeyGridRelocateMoved);
        sWriter->Bool(m_GridRelocateMovedCells);
    }

    void tUndoMove::Json(const rapidjson::Value& sValue) {
        tUndoPaste::Json(sValue);
        if (sValue.HasMember(kJsonKeyMoveSource)) {
            m_SourceRef = sValue[kJsonKeyMoveSource].GetString();
            m_SourceRefRebase = m_SourceRef;
        }
        if (sValue.HasMember(kJsonKeyDidGridRelocate) && sValue[kJsonKeyDidGridRelocate].IsBool()) {
            m_DidGridRelocate = sValue[kJsonKeyDidGridRelocate].GetBool();
        }
        if (sValue.HasMember(kJsonKeyGridRelocateMoved) && sValue[kJsonKeyGridRelocateMoved].IsBool()) {
            m_GridRelocateMovedCells = sValue[kJsonKeyGridRelocateMoved].GetBool();
        }
    }

    tBool tUndoMove::Rebase() {
        if (!tUndoPaste::Rebase()) {
            return(false);
        }
        m_SourceRefRebase = m_SourceRef;
        if (!m_RebasePlan.m_Operations.empty()) {
            tSelect wSelect;
            if (!wSelect.Parse(m_SourceRef)) {
                return(false);
            }
            if (!wSelect.Rebase(m_RebasePlan)) {
                return(false);
            }
            m_SourceRefRebase = wSelect.StrRef();
        }
        return(true);
    }

    void tUndoMove::DeleteBeforeDo() {
        m_SourceAlreadyCleared = false;
        tUndoPaste::DeleteBeforeDo();
    }
    

	//=========================================================================
    tUndoAddRangeNamed::tUndoAddRangeNamed() :  tUndoSpreadSheetCallBack(0, "", tMode()), m_Name(),m_OldName(),m_RangeData(),m_OldRangeData() {}

    tUndoAddRangeNamed::tUndoAddRangeNamed(tString sName, tString sRef,tMode sMode, tSheet* sSheet) : tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), sRef, sMode), m_Name(sName),m_OldName(),m_RangeData(),m_OldRangeData() {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Add named range " << sName << ":" << sRef;
            OperationName(wStream.str());
        }
	}

	tUndoAddRangeNamed::~tUndoAddRangeNamed() {}

    tString  tUndoAddRangeNamed::ClassName() const { return("tUndoAddRangeNamed"); }


	tBool tUndoAddRangeNamed::Do() {
        tWorkBook* wWorkBook = Sheet()->WorkBook();
        tSelect wSelect;
        wSelect.Parse(m_RefRebase);

        // Collect every rectangle declared in the reference (multi-area on a
        // single sheet, e.g. "A1:B2;D5:E6"). We normalize single points into
        // 1x1 rects so the insert path only deals with tTempoRect instances.
        std::vector<tTempoRect> wRects;
        tVectorTempoPoint* wVect = wSelect.VectorSelect();
        if (wVect != nullptr) {
            for (auto wItem : *wVect) {
                if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
                    wRects.push_back(*wRect);
                } else if (tTempoPoint* wPt = dynamic_cast<tTempoPoint*>(wItem)) {
                    tTempoRect wCellRect;
                    wCellRect.Top(wPt->Row());
                    wCellRect.Left(wPt->Col());
                    wCellRect.Bottom(wPt->Row());
                    wCellRect.Right(wPt->Col());
                    wRects.push_back(wCellRect);
                }
            }
        }
        if (wRects.empty()) {
            return(false);
        }

        // Record the first clashing pre-existing named range so Undo can
        // restore it. Multi-area overwrite only captures the first collision
        // (same limitation as the previous single-area implementation).
        //
        // Overlap-aware: a different name that already covers one of these
        // areas is allowed to coexist (Test1=A1:A2;B1:B2 + Test2=A1:A2;C1:C2).
        // We only treat a clash as "replace the previous owner" when its
        // name matches m_Name (handled by the explicit DeleteRangeNamed
        // below). Other owners are left untouched.
        // tTempoRect accessors are non-const, so iterate by reference.
        for (auto& wRect : wRects) {
            tRange* wRange = Sheet()->Range(wRect.Top(),
                                            wRect.Left(),
                                            wRect.Bottom(),
                                            wRect.Right());
            if (wRange != nullptr && wRange->IsNamed()) {
                tString wExistingName = wRange->Name();
                if (wExistingName == m_Name() || wExistingName == "") continue;
                if (m_OldName() == "") {
                    // Snapshot RangeData for the clashing name (not m_Name — not registered here yet).
                    if (wRange->IsData()) {
                        tRangeData* wPrior = wWorkBook->RangeData(wExistingName);
                        if (wPrior != nullptr) {
                            m_OldRangeData = *wPrior;
                        } else {
                            m_OldRangeData.Clear();
                        }
                    }
                    // Only swallow the previous name if it would otherwise
                    // remain orphaned (no other surviving area). Otherwise
                    // keep both names registered (overlap).
                    auto wContainer = wWorkBook->RangeNamedContainer();
                    auto wOtherRanges = wContainer->Ranges(wExistingName);
                    if (wOtherRanges.size() <= 1) {
                        m_OldName = wExistingName;
                        wWorkBook->DeleteRangeNamed(m_OldName());
                    }
                }
            }
        }

        // Replace semantics on m_Name: drop any previously-registered areas
        // (including those carried over from a previous Do() call in the Undo
        // stack) before re-inserting the new multi-area selection.
        wWorkBook->DeleteRangeNamed(m_Name());

        if (!m_RangeData.IsEmpty()) {
            // Data ranges stay single-area: attach RangeData to the first
            // rectangle and keep the rest as plain named areas.
            wWorkBook->InsertRangeData(m_Name(), m_RangeData, wRects.front(), Sheet());
            for (size_t wIdx = 1; wIdx < wRects.size(); ++wIdx) {
                wWorkBook->InsertRangeNamed(m_Name(), wRects[wIdx], Sheet());
            }
        } else {
            for (auto& wRect : wRects) {
                wWorkBook->InsertRangeNamed(m_Name(), wRect, Sheet());
            }
        }
        return(true);
	}

	tBool tUndoAddRangeNamed::Undo() {
		tWorkBook* wWorkBook = Sheet()->WorkBook();
        // Erase Data if exist
        tBool wOk=wWorkBook->DeleteRangeNamed(m_Name.Str());
		if (m_OldName() != "") {
            tTempoRect wRangeRect;
            wRangeRect.ParseRef(m_RefRebase);
            if (m_OldRangeData.IsEmpty()) {
                wOk=wWorkBook->InsertRangeNamed(m_OldName(),
                                                wRangeRect,
                                                Sheet());
            } else {
                wOk=wWorkBook->InsertRangeData(m_OldName(),
                                               m_OldRangeData,
                                               wRangeRect,
                                               Sheet());
            }
		};
		return(wOk);
	}
    
    void tUndoAddRangeNamed::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        if (m_OldName() != "") {
            sWriter->Key(kJsonKeyOldName);
            sWriter->String(m_OldName().c_str());
        }
        if (!m_RangeData.IsEmpty()) {
            sWriter->Key(kJsonKeyData);
            m_RangeData.Json(sWriter);
        }
        if (!m_OldRangeData.IsEmpty()) {
            sWriter->Key(kJsonKeyOldData);
            m_OldRangeData.Json(sWriter);
        }
    }

    void tUndoAddRangeNamed::Json(const rapidjson::Value& sValue) {
         tUndoSpreadSheetCallBack::Json(sValue);
        if (sValue.HasMember(kJsonKeyName)) {
            m_Name = sValue[kJsonKeyName].GetString();
        }
        if (sValue.HasMember(kJsonKeyOldName)) {
            m_OldName = sValue[kJsonKeyOldName].GetString();
        }
        if (sValue.HasMember(kJsonKeyData)) {
            m_RangeData.Json(sValue[kJsonKeyData]);
        }
        if (sValue.HasMember(kJsonKeyOldData)) {
            m_OldRangeData.Json(sValue[kJsonKeyOldData]);
        }
    }

    
    void tUndoAddRangeNamed::DeleteBeforeDo() {
        // Clear save range named before Do
        m_OldName="";
        m_RangeData.Clear();
        m_OldRangeData.Clear();
    }

    //=========================================================================
    tUndoAddRangeData::tUndoAddRangeData() : tUndoAddRangeNamed(),m_JsonData() {
    }

    tUndoAddRangeData::tUndoAddRangeData(tString sName,tString sJsonData, tString sRef,tMode sMode, tSheet* sSheet) : tUndoAddRangeNamed(sName, sRef, sMode, sSheet) {
        m_JsonData = sJsonData;
    }

    tUndoAddRangeData::~tUndoAddRangeData() {
    }

    tString tUndoAddRangeData::ClassName() const { return("tUndoAddRangeData"); }

    tBool tUndoAddRangeData::Do() {
        tRect wBounds(m_RefRebase);
        if (!m_JsonData.empty()) {
            rapidjson::Document wDocument;
            wDocument.Parse(m_JsonData.c_str());
            if (wBounds.IsValid()) {
                m_RangeData.Json(wDocument, wBounds.Left(), wBounds.Right());
            } else {
                m_RangeData.Json(wDocument);
            }
        } else {
            tSheet* wSheet = Sheet();
            if (wSheet == nullptr) {
                Error("tUndoAddRangeData: sheet is nullptr");
                return false;
            }
            if (!wBounds.IsValid()) {
                Error("tUndoAddRangeData: empty JSON requires a valid single-area range ref");
                return false;
            }
            if (!m_RangeData.FillByRect(wSheet, wBounds)) {
                Error("tUndoAddRangeData: FillByRect failed");
                return false;
            }
        }
        if (!m_RangeData.IsEmpty()) {
            tBool wResult=tUndoAddRangeNamed::Do();
            if (wResult) {
                tRange* wRange = Sheet()->WorkBook()->FindRangeNamed(m_Name());
                if (wRange != nullptr) {
                    m_RangeData.SyncFromRange(Sheet(), wRange);
                }
                wResult=Sheet()->WorkBook()->ApplyRangeData(m_Name(),m_RangeData);
            }
            return(wResult);
        } else {
            return(false);
        }
    }
    

    tUndoDeleteRangeNamed::tUndoDeleteRangeNamed() : tUndoAddRangeData() {}

    tUndoDeleteRangeNamed::tUndoDeleteRangeNamed(tString sName, tMode sMode, tSheet* sSheet) : tUndoAddRangeData(sName, "","", sMode, sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete named range " << sName;
            OperationName(wStream.str());
        }
	}

	tUndoDeleteRangeNamed::~tUndoDeleteRangeNamed() {
	}

    tString  tUndoDeleteRangeNamed::ClassName() const { return("tUndoDeleteRangeNamed"); }

    tSaveSelect* tUndoDeleteRangeNamed::SaveSelect() { return(nullptr); }

    tString tUndoDeleteRangeNamed::RefRebase() { return(""); }

	tBool tUndoDeleteRangeNamed::Do() {
        tWorkBook* wWorkBook=Sheet()->WorkBook();
        tBool wOk=false;

        // Multi-area aware: build the full reference string joining every
        // area under the name (e.g. "A1:B2;D5:E6") so Undo can re-insert
        // them all. Dependents are only saved for the first area (legacy
        // single-range tSaveRangeNamed limitation); formulas typically bind
        // to the name and re-resolve via Range(name) which still points to
        // the first area.
        std::vector<tRange*> wRanges = wWorkBook->RangeNamedContainer()->Ranges(m_Name());
        if (!wRanges.empty()) {
            tString wJoined;
            tBool wFirst = true;
            for (tRange* wRange : wRanges) {
                if (wRange == nullptr) continue;
                if (!wFirst) wJoined += ";";
                wJoined += wRange->StrRef();
                wFirst = false;
            }
            m_Ref = wJoined;
            m_RefRebase = m_Ref;
            // Overlap-aware: SaveAndModifyDependent nulls every formula
            // reference that points at the saved range. When another name
            // still owns this range (Test1=A1:A2;B1:B2 + Test2=A1:A2;C1:C2),
            // wiping E2's A1:A2 ref would break SUM(Test2). Skip the
            // dependent invalidation in that case so formulas keep working;
            // they will simply render the name of the surviving owner.
            tRange* wFirstRange = wRanges.front();
            tAllocatorRef wSheetRef = wFirstRange->Sheet()->AllocatorRef();
            tAllocatorRef wRangeRef = wFirstRange->AllocatorRef();
            tBool wShared = wWorkBook->RangeNamedContainer()->IsRangeSharedByOtherName(
                m_Name(), wSheetRef, wRangeRef);
            if (!wShared) {
                m_SaveRangeNamed.SaveAndModifyDependent(wFirstRange);
            }
            if (wFirstRange->IsData()) {
                m_RangeData = wWorkBook->RangeData(m_Name());
            }
            wOk = wWorkBook->DeleteRangeNamed(m_Name.Str());
        }
        return(wOk);
	}

	tBool tUndoDeleteRangeNamed::Undo() {
        tBool wOk=false;
        tWorkBook* wWorkBook=Sheet()->WorkBook();
        tSelect wSelect;
        wSelect.Parse(m_RefRebase);

        // Re-insert every rectangle that was captured at Do() time.
        std::vector<tTempoRect> wRects;
        tVectorTempoPoint* wVect = wSelect.VectorSelect();
        if (wVect != nullptr) {
            for (auto wItem : *wVect) {
                if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
                    wRects.push_back(*wRect);
                } else if (tTempoPoint* wPt = dynamic_cast<tTempoPoint*>(wItem)) {
                    tTempoRect wCellRect;
                    wCellRect.Top(wPt->Row());
                    wCellRect.Left(wPt->Col());
                    wCellRect.Bottom(wPt->Row());
                    wCellRect.Right(wPt->Col());
                    wRects.push_back(wCellRect);
                }
            }
        }
        if (wRects.empty()) {
            return(false);
        }
        if (m_RangeData.IsEmpty()) {
            // tTempoRect accessors are non-const, so iterate by reference.
            for (auto& wRect : wRects) {
                wWorkBook->InsertRangeNamed(m_Name(), wRect, Sheet());
            }
            wOk = true;
        } else {
            // Data ranges stay single-area: attach RangeData to the first
            // rectangle and add the remaining ones as plain named areas.
            wWorkBook->InsertRangeData(m_Name(), m_RangeData, wRects.front(), Sheet());
            for (size_t wIdx = 1; wIdx < wRects.size(); ++wIdx) {
                wWorkBook->InsertRangeNamed(m_Name(), wRects[wIdx], Sheet());
            }
            wOk = true;
        }
        tRange* wRange = wWorkBook->FindRangeNamed(m_Name());
        if (wRange != nullptr) {
            m_SaveRangeNamed.RecupAndModifyDependent(wRange);
        }
        return(wOk);
	}

    void tUndoDeleteRangeNamed::Json(Writer<StringBuffer>* sWriter) {
        tUndoAddRangeNamed::Json(sWriter);
        // Undo messages must carry formula dependents so remote clients can
        // RecupAndModifyDependent after DeleteRangeNamed (GetMessage rebuilds
        // the undo from JSON instead of using the local undo stack).
        if (IsUndo()) {
            sWriter->Key(kJsonKeySave);
            sWriter->StartObject();
            m_SaveRangeNamed.Json(sWriter, Sheet());
            sWriter->EndObject();
        }
    }

    void tUndoDeleteRangeNamed::Json(const rapidjson::Value& sValue) {
        tUndoAddRangeNamed::Json(sValue);
        if (sValue.HasMember(kJsonKeySave)) {
            m_SaveRangeNamed.Json(sValue[kJsonKeySave], Sheet());
        }
    }
    
    void tUndoDeleteRangeNamed::DeleteBeforeDo() {}


    tBool tUndoDeleteRangeNamed::Rebase() {
        SetRebasePlan();
        if (!m_RebasePlan.IsEmpty()) {
            // Rebase the saved named range coordinates
            if (!m_SaveRangeNamed.Rebase(m_RebasePlan,Sheet())) {
                // Named range was deleted or became invalid
                return false;
            }
        }
        
        return true;
    }

    //=========================================================================
    // tUndoUpdateRangeNamed
    //
    // Atomic "rename and/or re-select" of a named range. The update keeps the
    // underlying tRange* (and therefore tAllocatorRef) for every area that
    // stays in the new selection. Concretely:
    //   - we call DeleteRangeNamed(oldName) which only touches the
    //     m_MapName / m_MapRef / IsNamed flag state (no dependent rewriting);
    //   - we then call InsertRangeNamed(newName, rect) for every area of the
    //     new selection. EnsureRange returns the exact same tRange* when the
    //     rectangle already exists on the sheet, so formulas that embed the
    //     old range pointer in their VectorRef keep evaluating. Rendering
    //     automatically switches to newName through m_MapRef ->
    //     tRange::Name() -> tFormula::ClassOrRangeStr.
    //
    // Overlap is handled by the container-level overlap awareness already in
    // DeleteRangeNamed/InsertRangeNamed: shared areas of a different name
    // survive the rename.
    //=========================================================================
    tUndoUpdateRangeNamed::tUndoUpdateRangeNamed()
        : tUndoSpreadSheetCallBack(0, "", tMode()),
          m_NewName(), m_OldName(), m_OldRef(), m_RangeData() {}

    tUndoUpdateRangeNamed::tUndoUpdateRangeNamed(tString sOldName,
                                                 tString sNewName,
                                                 tString sNewRef,
                                                 tMode sMode,
                                                 tSheet* sSheet)
        : tUndoSpreadSheetCallBack(sSheet ? sSheet->AllocatorRef() : 0,
                                   sNewRef, sMode),
          m_NewName(sNewName), m_OldName(sOldName), m_OldRef(), m_RangeData() {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Update named range " << sOldName << " -> " << sNewName
                    << ":" << sNewRef;
            OperationName(wStream.str());
        }
    }

    tUndoUpdateRangeNamed::~tUndoUpdateRangeNamed() {}

    tString tUndoUpdateRangeNamed::ClassName() const {
        return("tUndoUpdateRangeNamed");
    }

    // Shared helper: parse a ref like "A1:A2;B1:B2" (or a single point) into
    // the list of rectangles the named range should cover. Returns true if at
    // least one rectangle was produced.
    static tBool ParseNamedRangeRef(const tString& sRef,
                                    std::vector<tTempoRect>& sOutRects) {
        sOutRects.clear();
        tSelect wSelect;
        wSelect.Parse(sRef);
        tVectorTempoPoint* wVect = wSelect.VectorSelect();
        if (wVect == nullptr) return(false);
        for (auto wItem : *wVect) {
            if (tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem)) {
                sOutRects.push_back(*wRect);
            } else if (tTempoPoint* wPt = dynamic_cast<tTempoPoint*>(wItem)) {
                tTempoRect wCellRect;
                wCellRect.Top(wPt->Row());
                wCellRect.Left(wPt->Col());
                wCellRect.Bottom(wPt->Row());
                wCellRect.Right(wPt->Col());
                sOutRects.push_back(wCellRect);
            }
        }
        return(!sOutRects.empty());
    }

    tBool tUndoUpdateRangeNamed::Do() {
        tWorkBook* wWorkBook = Sheet()->WorkBook();
        if (wWorkBook == nullptr) return(false);

        // The old name must currently exist, otherwise there is nothing to
        // update.
        auto wOldRanges = wWorkBook->RangeNamedContainer()->Ranges(m_OldName());
        if (wOldRanges.empty()) return(false);

        // If we are renaming (old != new) make sure the target name is free.
        // When old == new we are only changing the selection, so a matching
        // entry is expected.
        if (m_NewName() != m_OldName()) {
            auto wClash = wWorkBook->RangeNamedContainer()->Ranges(m_NewName());
            if (!wClash.empty()) return(false);
        }

        // Capture the old selection as a joined ref so Undo() can restore it.
        // We build it from the live tRange* objects rather than re-parsing the
        // original ref because dependent insert/delete col/row operations may
        // have shifted coordinates in between.
        tString wOldRef;
        tBool wFirst = true;
        tBool wWasData = false;
        for (tRange* wRange : wOldRanges) {
            if (wRange == nullptr) continue;
            if (wRange->IsData()) {
                wWasData = true;
            }
            if (!wFirst) wOldRef += ";";
            wOldRef += wRange->StrRef();
            wFirst = false;
        }
        m_OldRef = wOldRef;

        // Preserve table metadata across rename / reselect.
        m_RangeData.Clear();
        if (wWasData) {
            tRangeData* wPrior = wWorkBook->RangeData(m_OldName());
            if (wPrior != nullptr) {
                m_RangeData = *wPrior;
            }
        }

        // Parse the desired new selection.
        std::vector<tTempoRect> wRects;
        if (!ParseNamedRangeRef(m_RefRebase, wRects)) return(false);

        // Drop the old registration. This only mutates the container state
        // (m_MapName / m_MapRef) and the per-range IsNamed flag; it does not
        // touch formula dependents. Shared areas with other names survive
        // thanks to the overlap-aware logic in
        // tRangeNamedContainer::DeleteRangeNamedByName.
        wWorkBook->DeleteRangeNamed(m_OldName());

        // Re-insert every rectangle under the new name. EnsureRange reuses
        // the existing tRange* (preserving tAllocatorRef) when the rectangle
        // is already allocated, so formulas that pointed at the old range
        // keep resolving and will now render with the new name through
        // wRange->Name() -> m_MapRef.
        // Tables (RangeData) must go through InsertRangeData so IsData +
        // column/filter/style metadata survive the rename.
        if (!m_RangeData.IsEmpty()) {
            wWorkBook->InsertRangeData(m_NewName(), m_RangeData, wRects.front(), Sheet());
            for (size_t wIdx = 1; wIdx < wRects.size(); ++wIdx) {
                wWorkBook->InsertRangeNamed(m_NewName(), wRects[wIdx], Sheet());
            }
        } else {
            for (auto& wRect : wRects) {
                wWorkBook->InsertRangeNamed(m_NewName(), wRect, Sheet());
            }
        }
        return(true);
    }

    tBool tUndoUpdateRangeNamed::Undo() {
        tWorkBook* wWorkBook = Sheet()->WorkBook();
        if (wWorkBook == nullptr) return(false);

        std::vector<tTempoRect> wRects;
        if (!ParseNamedRangeRef(m_OldRef, wRects)) return(false);

        // Symmetrical to Do(): remove the new name and reinstate the old one
        // over its original selection (restoring RangeData when it was a table).
        wWorkBook->DeleteRangeNamed(m_NewName());
        if (!m_RangeData.IsEmpty()) {
            wWorkBook->InsertRangeData(m_OldName(), m_RangeData, wRects.front(), Sheet());
            for (size_t wIdx = 1; wIdx < wRects.size(); ++wIdx) {
                wWorkBook->InsertRangeNamed(m_OldName(), wRects[wIdx], Sheet());
            }
        } else {
            for (auto& wRect : wRects) {
                wWorkBook->InsertRangeNamed(m_OldName(), wRect, Sheet());
            }
        }
        return(true);
    }

    void tUndoUpdateRangeNamed::DeleteBeforeDo() {
        m_OldRef = "";
        m_RangeData.Clear();
    }

    tBool tUndoUpdateRangeNamed::Rebase() {
        // 1) Default rebase: refreshes m_RefRebase (the new selection) from
        //    m_Ref using the current rebase plan.
        if (!tUndoSpreadSheetCallBack::Rebase()) {
            return(false);
        }

        // 2) Multi-area specific: rebase m_OldRef as well. Do() stores the
        //    original selection there so Undo() can restore it; without this
        //    rebase, col/row insert/delete operations happening between Do()
        //    and a later Undo() would leave m_OldRef pointing at stale
        //    coordinates and Undo() would reinstate the old name on the
        //    wrong cells.
        if (m_OldRef.empty()) {
            return(true);
        }
        if (m_RebasePlan.IsEmpty()) {
            return(true);
        }
        tSelect wSelect;
        wSelect.Parse(m_OldRef);
        if (!wSelect.Rebase(m_RebasePlan)) {
            // Old selection has been fully wiped out by the rebase plan
            // (e.g. every area sat inside a deleted col/row range). Signal
            // the failure so the undo stack can drop this action.
            return(false);
        }
        m_OldRef = wSelect.StrRef();
        return(true);
    }

    void tUndoUpdateRangeNamed::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_NewName().c_str());
        sWriter->Key(kJsonKeyOldName);
        sWriter->String(m_OldName().c_str());
        if (!m_OldRef.empty()) {
            sWriter->Key(kJsonKeyOldRef);
            sWriter->String(m_OldRef.c_str());
        }
        if (!m_RangeData.IsEmpty()) {
            sWriter->Key(kJsonKeyData);
            m_RangeData.Json(sWriter);
        }
    }

    void tUndoUpdateRangeNamed::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        if (sValue.HasMember(kJsonKeyName)) {
            m_NewName = sValue[kJsonKeyName].GetString();
        }
        if (sValue.HasMember(kJsonKeyOldName)) {
            m_OldName = sValue[kJsonKeyOldName].GetString();
        }
        if (sValue.HasMember(kJsonKeyOldRef)) {
            m_OldRef = sValue[kJsonKeyOldRef].GetString();
        }
        if (sValue.HasMember(kJsonKeyData)) {
            m_RangeData.Json(sValue[kJsonKeyData]);
        }
    }

    //=========================================================================
    tUndoApplyRangeData::tUndoApplyRangeData() :  tUndoAddRangeNamed(), m_JsonData() {}

    tUndoApplyRangeData::tUndoApplyRangeData(tString sName, tString sJsonData, tMode sMode, tSheet* sSheet) : tUndoAddRangeNamed(sName, "", sMode, sSheet), m_JsonData(sJsonData) {
        // Normalize sheet
        NormalizeSheet(&sSheet);
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Apply RangeData " << sName;
            OperationName(wStream.str());
        }
    }

    tUndoApplyRangeData::~tUndoApplyRangeData() {
    }

    tString tUndoApplyRangeData::ClassName() const { 
        return("tUndoApplyRangeData"); 
    }

    tBool tUndoApplyRangeData::Do() {
        tWorkBook* wWorkBook = Sheet()->WorkBook();
        tBool wOk=false;
        tRange* wRange=wWorkBook->FindRangeNamed(m_Name());
        if (wRange==nullptr) {
            Error("Range "+m_Name()+" is nullptr");
            return false;
        }

        
        rapidjson::Document wDocument;
        wDocument.Parse(m_JsonData.c_str());
        m_RangeData.Json(wDocument, wRange->LeftIndex(), wRange->RightIndex());
        if (m_RangeData.IsEmpty()) {
            Error("RangeData JSON is empty");
            return false;
        }
        m_RangeData.SyncFromRange(Sheet(), wRange);
        
        if (wRange!=nullptr) {
            // Deep-copy snapshot before mutating stored workbook RangeData (heap replaces pointer targets).
            tRangeData* wPrior = wWorkBook->RangeData(m_Name());
            if (wPrior != nullptr) {
                m_OldRangeData = *wPrior;
            } else {
                m_OldRangeData.Clear();
            }
            // Apply sort and filter if RangeData exists
            wOk=wWorkBook->ApplyRangeData(m_Name(),m_RangeData);
        }
        return(wOk);
    }

    tBool tUndoApplyRangeData::Undo() {
        if (IsError()) {
            return false;
        }
        
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            Error("Sheet is nullptr");
            return false;
        }
        
        tWorkBook* wWorkBook = wSheet->WorkBook();
        if (wWorkBook == nullptr) {
            Error("WorkBook is nullptr");
            return false;
        }
        
        // Find the range
        tRange* wRange = wWorkBook->FindRangeNamed(m_Name());
        if (wRange == nullptr) {
            Error("Range not found");
            return false;
        }
        
        // Restore saved RangeData
        if (!m_OldRangeData.IsEmpty())  {
            wWorkBook->ApplyRangeData(m_Name(), m_OldRangeData);
        } else {
            // If there was no RangeData before, just remove it
            wRange->RemoveData();
        }
        
        return true;
    }

    //=========================================================================
    // tUndoConditionalFormat
    //=========================================================================
    tUndoConditionaFormat::tUndoConditionaFormat() : tUndoSpreadSheetCallBack(0, "", tMode()),m_Type(), m_ConditionalFormat(nullptr), m_Param1(""), m_Param2(""), m_Param3(""), m_Param4(""), m_Param5(""), m_Param6(""),m_Param7(""), m_Param8(""), m_Param9(""), m_Param10(""), m_HadExistingBeforeDo(false), m_SaveParam1(""), m_SaveParam2(""), m_SaveParam3(""), m_SaveParam4(""), m_SaveParam5(""), m_SaveParam6(""), m_SaveParam7(""), m_SaveParam8(""), m_SaveParam9(""), m_SaveParam10(""), m_SaveIconType(tIconType::t_None) {}

    tUndoConditionaFormat::tUndoConditionaFormat(tString sRef,tString sType,tString sParam1, tString sParam2, tString sParam3, tString sParam4, tString sParam5, tString sParam6, tString sParam7, tString sParam8, tString sParam9, tString sParam10, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), sRef, sMode),m_ConditionalFormat(nullptr), m_StringType(sType), m_Param1(sParam1), m_Param2(sParam2), m_Param3(sParam3), m_Param4(sParam4), m_Param5(sParam5), m_Param6(sParam6), m_Param7(sParam7), m_Param8(sParam8), m_Param9(sParam9), m_Param10(sParam10), m_HadExistingBeforeDo(false), m_SaveParam1(""), m_SaveParam2(""), m_SaveParam3(""), m_SaveParam4(""), m_SaveParam5(""), m_SaveParam6(""), m_SaveParam7(""), m_SaveParam8(""), m_SaveParam9(""), m_SaveParam10(""), m_SaveIconType(tIconType::t_None) {
        // Get Type
        m_Type=StringToConditionalFormatType(m_StringType.c_str());
    
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Conditional format " << sRef;
            OperationName(wStream.str());
        }
    }

    tUndoConditionaFormat::~tUndoConditionaFormat() {}
  
    tString tUndoConditionaFormat::ClassName() const { return("tUndoConditionalFormat"); }

    tString tUndoConditionaFormat::Key() {
        return(KeyStyleRef(m_Type,m_RefRebase));
    }

    void tUndoConditionaFormat::SaveParamsFromFormat(tConditionalFormat* sConditionalFormat) {
        if (sConditionalFormat == nullptr) {
            return;
        }
        m_SaveParam1 = sConditionalFormat->Param1();
        m_SaveParam2 = sConditionalFormat->Param2();
        m_SaveParam3 = sConditionalFormat->Param3();
        m_SaveParam4 = sConditionalFormat->Param4();
        m_SaveParam5 = sConditionalFormat->Param5();
        m_SaveParam6 = sConditionalFormat->Param6();
        m_SaveParam7 = sConditionalFormat->Param7();
        m_SaveParam8 = sConditionalFormat->Param8();
        m_SaveParam9 = sConditionalFormat->Param9();
        m_SaveParam10 = sConditionalFormat->Param10();
        m_SaveIconType = sConditionalFormat->IconType();
    }

    tBool tUndoConditionaFormat::ApplyParamsToFormat(
        tConditionalFormat* sConditionalFormat,
        tConditionalFormatType sType,
        const tString& sParam1,
        const tString& sParam2,
        const tString& sParam3,
        const tString& sParam4,
        const tString& sParam5,
        const tString& sParam6,
        const tString& sParam7,
        const tString& sParam8,
        const tString& sParam9,
        const tString& sParam10,
        tIconType sIconType) {
        if (sConditionalFormat == nullptr) {
            return(false);
        }

        switch (sType) {
            case tConditionalFormatType::t_CustomFormulas:
            case tConditionalFormatType::t_HighlightCellsRules: {
                if (!sConditionalFormat->Compil(sParam1, sParam2, sParam2)) {
                    return(false);
                }
                sConditionalFormat->SetParam3(sParam3);
                sConditionalFormat->SetParam4(sParam4);
                sConditionalFormat->SetParam5(sParam5);
                sConditionalFormat->SetParam6(sParam6);
                return(true);
            }
            case tConditionalFormatType::t_DataBars: {
                sConditionalFormat->SetParam1(sParam1);
                sConditionalFormat->SetParam2(sParam2);
                sConditionalFormat->SetParam3(sParam3);
                sConditionalFormat->SetParam4(sParam4);
                sConditionalFormat->SetParam5(sParam5);
                sConditionalFormat->SetParam6(sParam6);
                return(true);
            }
            case tConditionalFormatType::t_IconSets: {
                sConditionalFormat->IconType(sIconType);
                sConditionalFormat->SetParam1(sParam1);
                sConditionalFormat->SetParam2(sParam2);
                sConditionalFormat->SetParam3(sParam3);
                sConditionalFormat->SetParam4(sParam4);
                sConditionalFormat->SetParam5(sParam5);
                sConditionalFormat->SetParam6(sParam6);
                sConditionalFormat->SetParam7(sParam7);
                sConditionalFormat->SetParam8(sParam8);
                sConditionalFormat->SetParam9(sParam9);
                sConditionalFormat->SetParam10(sParam10);
                return(true);
            }
            case tConditionalFormatType::t_ColorScales: {
                sConditionalFormat->SetParam1(sParam1);
                sConditionalFormat->SetParam2(sParam2);
                sConditionalFormat->SetParam3(sParam3);
                sConditionalFormat->SetParam4(sParam4);
                sConditionalFormat->SetParam5(sParam5);
                sConditionalFormat->SetParam6(sParam6);
                sConditionalFormat->SetParam7(sParam7);
                sConditionalFormat->SetParam8(sParam8);
                sConditionalFormat->SetParam9(sParam9);
                sConditionalFormat->SetParam10(sParam10);
                return(true);
            }
            default:
                break;
        }
        return(false);
    }

    void tUndoConditionaFormat::RefreshConditionalFormatVisuals(tConditionalFormat* sConditionalFormat) {
        if (sConditionalFormat == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = Sheet()->WorkBook()->FormatApi();
        switch (sConditionalFormat->Type()) {
            case tConditionalFormatType::t_CustomFormulas:
            case tConditionalFormatType::t_HighlightCellsRules:
                if (wFormatApi != nullptr) {
                    sConditionalFormat->Apply(tPass::t_PassApply);
                    // Mirror JsonView: PassApply creates transient ItemCF/css refs; PassFree releases them.
                    sConditionalFormat->Apply(tPass::t_PassFree);
                }
                break;
            case tConditionalFormatType::t_DataBars:
            case tConditionalFormatType::t_ColorScales:
            case tConditionalFormatType::t_IconSets:
                // PassSum recalculates min/max/mid; safe without FormatApi.
                sConditionalFormat->Apply(tPass::t_PassSum);
                if (wFormatApi != nullptr) {
                    sConditionalFormat->Apply(tPass::t_PassApply);
                    sConditionalFormat->Apply(tPass::t_PassFree);
                }
                break;
            default:
                return;
        }
    }

    tBool tUndoConditionaFormat::Do() {
        tBool wResult = true;
    
        if (m_Type==tConditionalFormatType::t_None) return(false);

        m_HadExistingBeforeDo = false;
        tConditionalFormat* wExisting = Sheet()->ConditionalFormat(m_Type, m_RefRebase);
        if (wExisting != nullptr) {
            m_HadExistingBeforeDo = true;
            SaveParamsFromFormat(wExisting);
        }
        
        m_ConditionalFormat=Sheet()->AddConditionalFormat(m_Type,m_RefRebase);
        if (m_ConditionalFormat==nullptr) return(false);

        tIconType wIconType = tIconType::t_None;
        if (m_Type == tConditionalFormatType::t_IconSets) {
            const tBool wParam4IsNumeric =
                m_Param4 != "" && tClassString(m_Param4).IsNumber();
            const tBool wParam5IsNumeric =
                m_Param5 != "" && tClassString(m_Param5).IsNumber();
            const tBool wParam6IsNumeric =
                m_Param6 != "" && tClassString(m_Param6).IsNumber();
            const tBool wFiveIcons =
                !wParam4IsNumeric && !wParam5IsNumeric && wParam6IsNumeric;
            const char* wIconTypeStr = wFiveIcons ? m_Param10.c_str() : m_Param7.c_str();
            if (wIconTypeStr && *wIconTypeStr != '\0') {
                wIconType = StringToIconType(wIconTypeStr);
            }
        }

        wResult = ApplyParamsToFormat(
            m_ConditionalFormat,
            m_Type,
            m_Param1,
            m_Param2,
            m_Param3,
            m_Param4,
            m_Param5,
            m_Param6,
            m_Param7,
            m_Param8,
            m_Param9,
            m_Param10,
            wIconType);

        if (wResult) {
            RefreshConditionalFormatVisuals(m_ConditionalFormat);
        } else {
            tString wKey=KeyStyleRef(m_Type,m_RefRebase);
            if (!m_HadExistingBeforeDo) {
                Sheet()->RemoveConditionalFormatByKey(wKey);
            } else {
                ApplyParamsToFormat(
                    m_ConditionalFormat,
                    m_Type,
                    m_SaveParam1,
                    m_SaveParam2,
                    m_SaveParam3,
                    m_SaveParam4,
                    m_SaveParam5,
                    m_SaveParam6,
                    m_SaveParam7,
                    m_SaveParam8,
                    m_SaveParam9,
                    m_SaveParam10,
                    m_SaveIconType);
                RefreshConditionalFormatVisuals(m_ConditionalFormat);
            }
        }
        return(wResult);
    }

    tBool tUndoConditionaFormat::Undo() {
        if (!m_HadExistingBeforeDo) {
            return(Sheet()->RemoveConditionalFormatByKey(Key()));
        }

        tConditionalFormat* wConditionalFormat = Sheet()->ConditionalFormat(m_Type, m_RefRebase);
        if (wConditionalFormat == nullptr) {
            return(false);
        }

        tBool wResult = ApplyParamsToFormat(
            wConditionalFormat,
            m_Type,
            m_SaveParam1,
            m_SaveParam2,
            m_SaveParam3,
            m_SaveParam4,
            m_SaveParam5,
            m_SaveParam6,
            m_SaveParam7,
            m_SaveParam8,
            m_SaveParam9,
            m_SaveParam10,
            m_SaveIconType);

        if (wResult) {
            RefreshConditionalFormatVisuals(wConditionalFormat);
        }
        return(wResult);
    }

    void tUndoConditionaFormat::Json(Writer<StringBuffer>* sWriter) {
        // Reuse tUndoFormat serialized structure (ref + optional save)
        tUndoSpreadSheetCallBack::Json(sWriter);
        
        if (m_StringType != "") {
            sWriter->Key(kJsonKeyType);
            sWriter->String(m_StringType.c_str());
        }
        if (m_Param1 != "") {
            sWriter->Key(kJsonKeyParam1);
            sWriter->String(m_Param1.c_str());
        }
        if (m_Param2 != "") {
            sWriter->Key(kJsonKeyParam2);
            sWriter->String(m_Param2.c_str());
        }
        if (m_Param3 != "") {
            sWriter->Key(kJsonKeyParam3);
            sWriter->String(m_Param3.c_str());
        }
        if (m_Param4 != "") {
            sWriter->Key(kJsonKeyParam4);
            sWriter->String(m_Param4.c_str());
        }
        if (m_Param5 != "") {
            sWriter->Key(kJsonKeyParam5);
            sWriter->String(m_Param5.c_str());
        }
        if (m_Param6 != "") {
            sWriter->Key(kJsonKeyParam6);
            sWriter->String(m_Param6.c_str());
        }
        if (m_Param7 != "") {
            sWriter->Key(kJsonKeyParam7);
            sWriter->String(m_Param7.c_str());
        }
        if (m_Param8 != "") {
            sWriter->Key(kJsonKeyParam8);
            sWriter->String(m_Param8.c_str());
        }
        if (m_Param9 != "") {
            sWriter->Key(kJsonKeyParam9);
            sWriter->String(m_Param9.c_str());
        }
        if (m_Param10 != "") {
            sWriter->Key(kJsonKeyParam10);
            sWriter->String(m_Param10.c_str());
        }
    }

    void tUndoConditionaFormat::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        
        if (sValue.HasMember(kJsonKeyType)) {
            m_StringType = sValue[kJsonKeyType].GetString();
            m_Type=StringToConditionalFormatType(m_StringType.c_str());
        }
        if (sValue.HasMember(kJsonKeyParam1)) {
            m_Param1 = sValue[kJsonKeyParam1].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam2)) {
            m_Param2 = sValue[kJsonKeyParam2].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam3)) {
            m_Param3 = sValue[kJsonKeyParam3].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam4)) {
            m_Param4 = sValue[kJsonKeyParam4].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam5)) {
            m_Param5 = sValue[kJsonKeyParam5].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam6)) {
            m_Param6 = sValue[kJsonKeyParam6].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam7)) {
            m_Param7 = sValue[kJsonKeyParam7].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam8)) {
            m_Param8 = sValue[kJsonKeyParam8].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam9)) {
            m_Param9 = sValue[kJsonKeyParam9].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam10)) {
            m_Param10 = sValue[kJsonKeyParam10].GetString();
        }
    }

    void tUndoConditionaFormat::DeleteBeforeDo() {
        // Call parent to reset reference rebase
        tUndoSpreadSheetCallBack::DeleteBeforeDo();
        // Clear conditional format pointer before Do
        m_ConditionalFormat = nullptr;
    }

    //=========================================================================
    // tUndoDeleteConditionaFormat
    //=========================================================================
    tUndoDeleteConditionaFormat::tUndoDeleteConditionaFormat() : tUndoConditionaFormat() {}

    tUndoDeleteConditionaFormat::tUndoDeleteConditionaFormat(tString sRef,tString sType,tMode sMode, tSheet* sSheet)
        : tUndoConditionaFormat(sRef, sType, "", "", "", "", "", "", "", "", "", "", sMode, sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete conditional format " << sRef;
            OperationName(wStream.str());
        }
    }

    tUndoDeleteConditionaFormat::~tUndoDeleteConditionaFormat() {}

    tString tUndoDeleteConditionaFormat::ClassName() const { return("tUndoDeleteConditionaFormat"); }

   
    tBool tUndoDeleteConditionaFormat::Do() {
        if (m_Type==tConditionalFormatType::t_None) {
            return(false);
        }
        // Save Value
       tConditionalFormat* wConditionalFormat=Sheet()->ConditionalFormat(m_Type,m_RefRebase);
       if (wConditionalFormat==nullptr) return(false);
       m_Param1=wConditionalFormat->Param1();
       m_Param2=wConditionalFormat->Param2();
       m_Param3=wConditionalFormat->Param3();
       m_Param4=wConditionalFormat->Param4();
       m_Param5=wConditionalFormat->Param5();
       m_Param6=wConditionalFormat->Param6();
       m_Param7=wConditionalFormat->Param7();
       m_Param8=wConditionalFormat->Param8();
       m_Param9=wConditionalFormat->Param9();
       m_Param10=wConditionalFormat->Param10();
        
        tString wKey=Key();
        if (wKey=="") return(false);
    
    
       return(Sheet()->RemoveConditionalFormatByKey(wKey));
    }

    tBool tUndoDeleteConditionaFormat::Undo() {
        return(tUndoConditionaFormat::Do());
    }

    tBool tUndoConditionaFormat::Rebase() {
        return(tUndoSpreadSheetCallBack::Rebase());
    }

    //=========================================================================
    // tUndoAncestorDeleteInsert
    tUndoAncestorDeleteInsert::tUndoAncestorDeleteInsert(tBool sDoesRow,tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet) : tUndoSpreadSheetCallBack(0, "", tMode()),m_DoesRow(sDoesRow), m_Position(sPosition), m_Size(sSize), m_Rect(),m_PositionRebase(sPosition),m_RectRebase() {
        if (sSheet!=nullptr) m_SheetAllocatorRef=sSheet->AllocatorRef();
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
    }

    tUndoAncestorDeleteInsert::tUndoAncestorDeleteInsert(tBool sDoesRow, tRect sRect,tMode sMode, tSheet* sSheet) :  tUndoSpreadSheetCallBack(sSheet->AllocatorRef(), "", sMode),m_DoesRow(sDoesRow), m_Position(-1), m_Size(-1),m_Rect(sRect),m_PositionRebase(-1),m_RectRebase(sRect), m_SaveSelectErase() {
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
    }

    tBool tUndoAncestorDeleteInsert::DoesRow() { return(m_DoesRow); }

    tIndex tUndoAncestorDeleteInsert::Position() { return(m_Position);}
    tIndex tUndoAncestorDeleteInsert::PositionRebase() { return(m_PositionRebase);}
    tIndex tUndoAncestorDeleteInsert::Size() { return(m_Size);}
    tRect tUndoAncestorDeleteInsert::Rect() { return(m_Rect);}
    tRect tUndoAncestorDeleteInsert::RectRebase() { return(m_RectRebase);}

    tBool tUndoAncestorDeleteInsert::RebaseCol() {
        // Default: mirror current values
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;

        SetRebasePlan();
        if (!IsJson() && !m_RebasePlan.IsEmpty()) {
         
            if (m_Position != -1) {
                auto wRebasedPos = m_RebasePlan.RebaseCol(m_Position);
                if (wRebasedPos.has_value()) {
                    m_PositionRebase = wRebasedPos.value();
                } else {
                    // Mark as invalid
                    m_PositionRebase = -1;
                    return(false);
                }
            } else {
                // Rect mode: only columns (Left/Right) are rebased here
                tIndex wLeft = m_Rect.Left();
                tIndex wRight = m_Rect.Right();
                tIndex wWidth = m_Rect.Width();
                // cout << "RebaseCol() - Rect original: Left=" << wLeft << " Right=" << wRight << " Width=" << wWidth << endl;
                auto wRebasedLeft = m_RebasePlan.RebaseCol(wLeft);
                auto wRebasedRight = m_RebasePlan.RebaseCol(wRight);
                if (wRebasedLeft.has_value() && wRebasedRight.has_value()) {
                    // For undo, we need to preserve the original width of the rect.
                    // The rebased rect should have the same dimensions as the original, but with rebased positions.
                    // So we rebase Left, but calculate Right based on Left + original Width - 1.
                    m_RectRebase.Left(wRebasedLeft.value());
                    m_RectRebase.Right(wRebasedLeft.value() + wWidth - 1);
                    // Keep rows unchanged in column rebase
                    m_RectRebase.Top(m_Rect.Top());
                    m_RectRebase.Bottom(m_Rect.Bottom());
                    // cout << "RebaseCol() - Rect rebased: Left=" << wRebasedLeft.value() << " Right=" << (wRebasedLeft.value() + wWidth - 1) << " Width=" << wWidth << endl;
                } else {
                    // Invalidate rect if mapping failed
                    m_RectRebase.Set(0, 0, -1, -1);
                    return(false);
                }
            }
        }
        return(true);
    }

    tBool tUndoAncestorDeleteInsert::RebaseRow() {
        // Default: mirror current values
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        
        SetRebasePlan();
        if (!m_RebasePlan.IsEmpty()) {
            if (m_Position != -1) {
                auto wRebasedPos = m_RebasePlan.RebaseRow(m_Position);
                if (wRebasedPos.has_value()) {
                    m_PositionRebase = wRebasedPos.value();
                } else {
                    // Mark as invalid
                    m_PositionRebase = -1;
                    return(false);
                }
            } else {
                // Rect mode: only rows (Top/Bottom) are rebased here
                tIndex wTop = m_Rect.Top();
                tIndex wBottom = m_Rect.Bottom();
                tIndex wHeight = m_Rect.Height();
                auto wRebasedTop = m_RebasePlan.RebaseRow(wTop);
                auto wRebasedBottom = m_RebasePlan.RebaseRow(wBottom);
    #ifdef debuginterface
                cout << "RebaseRow() - RebasePlan: Top=" << wTop << "->" << (wRebasedTop.has_value() ? std::to_string(wRebasedTop.value()) : "none")
                << " Bottom=" << wBottom << "->" << (wRebasedBottom.has_value() ? std::to_string(wRebasedBottom.value()) : "none") << endl;
    #endif
                if (wRebasedTop.has_value() && wRebasedBottom.has_value()) {
                    // For undo, we need to preserve the original height of the rect.
                    // The rebased rect should have the same dimensions as the original, but with rebased positions.
                    // So we rebase Top, but calculate Bottom based on Top + original Height - 1.
                    m_RectRebase.Top(wRebasedTop.value());
                    m_RectRebase.Bottom(wRebasedTop.value() + wHeight - 1);
                    // Keep columns unchanged in row rebase
                    m_RectRebase.Left(m_Rect.Left());
                    m_RectRebase.Right(m_Rect.Right());
    #ifdef debuginterface
                    cout << "RebaseRow() - After rebase: m_RectRebase=" << m_RectRebase.StrRef() << " (original m_Rect=" << m_Rect.StrRef() << ")" << endl;
    #endif
                } else {
                    // Invalidate rect if mapping failed
                    m_RectRebase.Set(0, 0, -1, -1);
                    return(false);
                }
            }
        }
        return(true);
    }

    tBool tUndoAncestorDeleteInsert::Rebase() {
        // For rect operations, m_Ref is empty in the constructor, so we need to initialize it
        // with m_Rect.StrRef() (original, not rebased) before calling the parent Rebase()
        // The parent Rebase() will rebase m_RefRebase using m_Ref, so we must use the original value
        if (m_Position == -1 && m_Ref == "") {
            m_Ref = m_Rect.StrRef();
        }
        
#ifdef debuginterface
        cout << "tUndoAncestorDeleteInsert::Rebase() - Start: m_Position=" << m_Position << " m_Rect=" << m_Rect.StrRef() << " m_RectRebase=" << m_RectRebase.StrRef() << " m_Ref=" << m_Ref << " m_RefRebase=" << m_RefRebase << endl;
#endif
        
        // Rebase m_RectRebase or m_PositionRebase first
        tBool wOk = false;
        if (m_DoesRow) {
            wOk = RebaseRow();
        } else {
            wOk = RebaseCol();
        }
        if (!wOk) {
            return false;
        }
        
#ifdef debuginterface
        cout << "tUndoAncestorDeleteInsert::Rebase() - After RebaseRow/Col: m_RectRebase=" << m_RectRebase.StrRef() << endl;
#endif
        
        // Call parent Rebase() to rebase m_RefRebase and m_SaveSelect
        wOk = tUndoSpreadSheetCallBack::Rebase();
        
#ifdef debuginterface
        cout << "tUndoAncestorDeleteInsert::Rebase() - After parent Rebase(): m_RefRebase=" << m_RefRebase << " m_Position=" << m_Position << endl;
#endif
        
        // After parent Rebase(), for rect operations, ensure m_RefRebase matches m_RectRebase.StrRef()
        // This is because m_RectRebase was rebased by RebaseRow()/RebaseCol(), and m_RefRebase was rebased by parent Rebase()
        // They should match, but we ensure consistency
        // IMPORTANT: For rect operations, we ALWAYS use m_RectRebase.StrRef() as the source of truth
        // because m_RectRebase is rebased by RebaseRow()/RebaseCol() which handles rect operations correctly
        if (wOk && m_Position == -1) {
            // Always use m_RectRebase.StrRef() as the source of truth for rect operations
            // This ensures consistency between m_RectRebase (used for Undo execution) and m_RefRebase (used for RefRebase())
            // This MUST be done after RebaseRow()/RebaseCol() has rebased m_RectRebase
            // Force update m_RefRebase with m_RectRebase.StrRef() to ensure RefRebase() returns the correct value
            tString wRectRebaseStr = m_RectRebase.StrRef();
            tString wRefRebaseBefore = m_RefRebase;
#ifdef debuginterface
            cout << "tUndoAncestorDeleteInsert::Rebase() - Before update: m_RectRebase=" << wRectRebaseStr << " m_RefRebase=" << wRefRebaseBefore << endl;
#endif
            m_RefRebase = m_RectRebase.StrRef();
            // Also update m_Ref to match, so that Json() uses the correct value
            m_Ref = m_RectRebase.StrRef();
#ifdef debuginterface
            cout << "tUndoAncestorDeleteInsert::Rebase() - After update: m_RefRebase=" << m_RefRebase << " m_Ref=" << m_Ref << endl;
#endif
        } else {
#ifdef debuginterface
            cout << "tUndoAncestorDeleteInsert::Rebase() - Skipping update: wOk=" << wOk << " m_Position=" << m_Position << endl;
#endif
        }
        
        return wOk;
    }

    void tUndoAncestorDeleteInsert::DeleteBeforeDo() {
        // Call parent to reset reference rebase
        tUndoSpreadSheetCallBack::DeleteBeforeDo();
        // Clear save select erase before Do
        m_SaveSelectErase.Clear();
    }

    void tUndoAncestorDeleteInsert::Json(Writer<StringBuffer>* sWriter) {
        if (m_Rect.Left() != -1) {
            m_Ref=m_RectRebase.StrRef();
            m_RefRebase=m_Ref;
        }
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyDoesRow);
        sWriter->Bool(m_DoesRow);
        if (m_Position != -1) {
            sWriter->Key(kJsonKeyPosition);
            sWriter->Int(m_PositionRebase);
            sWriter->Key(kJsonKeySize);
            sWriter->Int(m_Size);
        }
        if (m_Rect.Left() != -1) {
            sWriter->Key(kJsonKeyRect);
            sWriter->String(m_RectRebase.StrRef().c_str());
        }
        if (IsUndo()) {
            sWriter->Key(kJsonKeySaveSelectErase);
            m_SaveSelectErase.Json(sWriter,Sheet());
        }
    }

    void tUndoAncestorDeleteInsert::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        m_SaveSelectErase.UndoSpreadSheet(this);
        m_DoesRow = sValue[kJsonKeyDoesRow].GetBool();
        if (sValue.HasMember(kJsonKeyPosition)) {
            m_Position = sValue[kJsonKeyPosition].GetInt();
            m_PositionRebase=m_Position;
        } else {
            m_Position=-1;
            m_PositionRebase=-1;
        }
        if (sValue.HasMember(kJsonKeySize)) {
            m_Size = sValue[kJsonKeySize].GetInt();
        }
        if (sValue.HasMember(kJsonKeyRect)) {
            tTempoRect wRect;
            wRect.ParseRef(sValue[kJsonKeyRect].GetString());
            m_Rect.Set(wRect.Top(), wRect.Left(), wRect.Bottom(), wRect.Right());
            m_RectRebase=m_Rect;
            // Place Rect
            if (m_Position != static_cast<tIndex>(-1)) {
                if (m_DoesRow) {
                    m_SaveSelectErase.RectDeleteArea(Sheet()->ColRowCellRange()->GetRectDeleteAreaRow(m_PositionRebase, m_Size));
                } else {
                    m_SaveSelectErase.RectDeleteArea(Sheet()->ColRowCellRange()->GetRectDeleteAreaCol(m_PositionRebase, m_Size));
                }
            } else {
                m_SaveSelectErase.RectDeleteArea(m_RectRebase);
            }
        }
        if (sValue.HasMember(kJsonKeySaveSelectErase)) {
            m_SaveSelectErase.Json(sValue[kJsonKeySaveSelectErase],Sheet());
            // Position/count undo only: svse already carries rect+sel for by-rect undo.
            if (m_Position != static_cast<tIndex>(-1)) {
                if (m_DoesRow) {
                    m_SaveSelectErase.RectDeleteArea(Sheet()->ColRowCellRange()->GetRectDeleteAreaRow(m_PositionRebase, m_Size));
                } else {
                    m_SaveSelectErase.RectDeleteArea(Sheet()->ColRowCellRange()->GetRectDeleteAreaCol(m_PositionRebase, m_Size));
                }
            }
        }
    }

    //=========================================================================
    // tUndoInsertCol
	tUndoInsertCol::tUndoInsertCol() : tUndoAncestorDeleteInsert(false,0,0,tMode(),nullptr) {}

    tUndoInsertCol::tUndoInsertCol(tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(false,sPosition,sSize,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Insert Col " << Base10ToAlpha(sPosition) << " " << sSize << "x";
            OperationName(wStream.str());
        }
	}

    tUndoInsertCol::tUndoInsertCol(tRect sRect,tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(false,sRect,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Insert Col " << sRect.StrRef();
            OperationName(wStream.str());
        }
    };

	tUndoInsertCol::~tUndoInsertCol() {
	};

    tString  tUndoInsertCol::ClassName() const { return("tUndoInsertCol"); }

	tSaveSelect* tUndoInsertCol::SaveSelect() { return(&m_SaveSelectErase); }

	tBool tUndoInsertCol::Do() {
        m_SaveSelectErase.Clear();
        // Reset rebase values to original before using them (symmetry with Undo)
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (m_Position!=-1) {
            Sheet()->DoInsertCol(m_PositionRebase, m_Size);
        } else {
            Sheet()->DoInsertColByRect(&m_SaveSelectErase,m_RectRebase);
        }
		return(true);
	};

	tBool tUndoInsertCol::Undo() {
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (Client() && !IsJson()) {
            if (!RebaseCol()) return(false);
        }
        if (m_Position!=-1) {
            Sheet()->DoDeleteCol(&m_SaveSelectErase, m_PositionRebase, m_Size);
        } else {
            // For undo of InsertColByRect, use the original rect to determine which columns to delete,
            // but use the rebased rect to determine where to move cells.
            // The rebased rect accounts for other users' operations (e.g., User2's InsertCol).
            // This ensures cells are moved from their current positions (after rebase) to their
            // original positions (after rebase), not from their original positions to their original positions.
            Sheet()->DoDeleteColByRect(&m_SaveSelectErase, m_Rect, true, &m_RectRebase);
        }
		return(true);
	}

    tBool tUndoInsertCol::Rebase() {
        return(tUndoAncestorDeleteInsert::Rebase());
    }

#ifdef checkfo
    void tUndoInsertCol::IncCheckfo(tFormatApi* sFormatApi) {
        m_SaveSelectErase.IncCheckfo(sFormatApi);
    }
#endif


	//=========================================================================
    tUndoInsertRow::tUndoInsertRow() : tUndoAncestorDeleteInsert(true,0,0,tMode(),nullptr) {}

	tUndoInsertRow::tUndoInsertRow(tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(true,sPosition,sSize,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Insert Row " << sPosition << " " << sSize << "x";
            OperationName(wStream.str());
        }
	}

    tUndoInsertRow::tUndoInsertRow(tRect sRect,tMode sMode, tSheet* sSheet) :  tUndoAncestorDeleteInsert(true,sRect,sMode,sSheet)  {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Insert Row " << sRect.StrRef();
            OperationName(wStream.str());
        }
    };


	tUndoInsertRow::~tUndoInsertRow() {
	};


    tString  tUndoInsertRow::ClassName() const { return("tUndoInsertRow"); }

	tSaveSelect* tUndoInsertRow::SaveSelect() { return(&m_SaveSelectErase); }

	tBool tUndoInsertRow::Do() {
        m_SaveSelectErase.Clear();
        // Reset rebase values to original before using them (symmetry with Undo)
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (m_Position!=-1) {
            Sheet()->DoInsertRow(m_PositionRebase, m_Size);
            Sheet()->WorkBook()->ApplyCalculatedColumnFormulasToInsertedRows(
                Sheet(), m_PositionRebase, m_PositionRebase + m_Size - 1);
        } else {
            Sheet()->DoInsertRowByRect(&m_SaveSelectErase,m_RectRebase);
            Sheet()->WorkBook()->ApplyCalculatedColumnFormulasToInsertedRows(
                Sheet(), m_RectRebase.Top(), m_RectRebase.Bottom(),
                m_RectRebase.Left(), m_RectRebase.Right());
        }
		return(true);
	};

	tBool tUndoInsertRow::Undo() {
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (Client() && !IsJson()) {
            if (!RebaseRow()) return(false);
        }
        if (m_Position!=-1) {
            Sheet()->DoDeleteRow(&m_SaveSelectErase, m_PositionRebase, m_Size);
        } else {
            // For undo of InsertRowByRect, use the original rect to determine which rows to delete,
            // but use the rebased rect to determine where to move cells.
            // The rebased rect accounts for other users' operations.
            // This ensures cells are moved from their current positions (after rebase) to their
            // original positions (after rebase), not from their original positions to their original positions.
            Sheet()->DoDeleteRowByRect(&m_SaveSelectErase, m_Rect, true, &m_RectRebase);
        }
        return(true);
	}

    tBool tUndoInsertRow::Rebase() {
        return(tUndoAncestorDeleteInsert::Rebase());
    }

#ifdef checkfo
    void tUndoInsertRow::IncCheckfo(tFormatApi* sFormatApi) {
        m_SaveSelectErase.IncCheckfo(sFormatApi);
    }
#endif

	//=========================================================================
	tUndoInsertRowWithLabel::tUndoInsertRowWithLabel()
		: tUndoInsertRow(), m_LabelRef(), m_LabelValue() {}

	tUndoInsertRowWithLabel::tUndoInsertRowWithLabel(tRect sRect, tString sLabelRef,
													 tString sLabelValue, tMode sMode,
													 tSheet* sSheet)
		: tUndoInsertRow(sRect, sMode, sSheet),
		  m_LabelRef(sLabelRef),
		  m_LabelValue(sLabelValue) {
		if (IsUndoActif()) {
			tStringStream wStream;
			wStream << "Insert totals row " << sRect.StrRef();
			OperationName(wStream.str());
		}
	}

	tUndoInsertRowWithLabel::~tUndoInsertRowWithLabel() {}

	tString tUndoInsertRowWithLabel::ClassName() const {
		return("tUndoInsertRowWithLabel");
	}

	tBool tUndoInsertRowWithLabel::WriteLabel() {
		if (m_LabelRef().empty()) {
			return(true);
		}
		tSheet* wSheet = Sheet();
		if (wSheet == nullptr) {
			return(false);
		}
		tTempoPoint wPoint;
		if (!wPoint.ParseRef(m_LabelRef())) {
			return(false);
		}
		tCell* wCell = wSheet->EnsureCell(wPoint.Row(), wPoint.Col());
		if (wCell == nullptr) {
			return(false);
		}
		wCell->ClearFormulaAndVariant();
		wCell->Value(tVariant(m_LabelValue()));
		return(true);
	}

	tBool tUndoInsertRowWithLabel::Do() {
		if (!tUndoInsertRow::Do()) {
			return(false);
		}
		return(WriteLabel());
	}

	void tUndoInsertRowWithLabel::Json(Writer<StringBuffer>* sWriter) {
		tUndoAncestorDeleteInsert::Json(sWriter);
		sWriter->Key(kJsonKeyLabelRef);
		sWriter->String(m_LabelRef().c_str());
		sWriter->Key(kJsonKeyLabelValue);
		sWriter->String(m_LabelValue().c_str());
	}

	void tUndoInsertRowWithLabel::Json(const rapidjson::Value& sValue) {
		tUndoAncestorDeleteInsert::Json(sValue);
		if (sValue.HasMember(kJsonKeyLabelRef) && sValue[kJsonKeyLabelRef].IsString()) {
			m_LabelRef = sValue[kJsonKeyLabelRef].GetString();
		} else {
			m_LabelRef = "";
		}
		if (sValue.HasMember(kJsonKeyLabelValue) && sValue[kJsonKeyLabelValue].IsString()) {
			m_LabelValue = sValue[kJsonKeyLabelValue].GetString();
		} else {
			m_LabelValue = "";
		}
	}

	//=========================================================================
    tUndoDeleteCol::tUndoDeleteCol() : tUndoAncestorDeleteInsert(false,0,0,tMode(),nullptr) {}

	tUndoDeleteCol::tUndoDeleteCol(tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(false,sPosition,sSize,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete col " << Base10ToAlpha(sPosition) << " to " << Base10ToAlpha(sPosition+sSize-1);
            OperationName(wStream.str());
        }
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
	}

    tUndoDeleteCol::tUndoDeleteCol(tRect sRect,tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(false,sRect,sMode,sSheet)  {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete Col " << sRect.StrRef();
            OperationName(wStream.str());
        }
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
    };
	tUndoDeleteCol::~tUndoDeleteCol() {
	};

    tString  tUndoDeleteCol::ClassName() const { return("tUndoDeleteCol"); }

	tSaveSelect* tUndoDeleteCol::SaveSelect() { return(&m_SaveSelectErase); }

	tBool tUndoDeleteCol::Do() {
        m_SaveSelectErase.Clear();
        // Reset rebase values to original before using them (symmetry with Undo)
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (m_Position!=-1) {
            Sheet()->DoDeleteCol(&m_SaveSelectErase,m_PositionRebase, m_Size);
        } else {
            Sheet()->DoDeleteColByRect(&m_SaveSelectErase,m_RectRebase,false);
        }
		return(true);
	};

	tBool tUndoDeleteCol::Undo() {
        // Note: Rebase() may have already been called in tInterfaceWeb::Undo() before calling m_UndoRedoContainer->Undo()
        // In that case, m_RectRebase and m_PositionRebase are already rebased.
        // We should only rebase again if Client() is true AND the rebase hasn't been done yet.
        // Check if rebase was already done by comparing current values with original
        bool wRebaseAlreadyDone = false;
        if (m_Position != -1) {
            // Check if PositionRebase is different from Position (already rebased)
            if (m_PositionRebase != m_Position) {
                wRebaseAlreadyDone = true;
            }
        } else {
            // Check if RectRebase is different from Rect (already rebased)
            if (m_RectRebase.Top() != m_Rect.Top() || m_RectRebase.Bottom() != m_Rect.Bottom() ||
                m_RectRebase.Left() != m_Rect.Left() || m_RectRebase.Right() != m_Rect.Right()) {
                wRebaseAlreadyDone = true;
            }
        }
        
        if (!wRebaseAlreadyDone && !IsJson()) {
            // Rebase hasn't been done yet, initialize and rebase
            m_PositionRebase = m_Position;
            m_RectRebase = m_Rect;
            if (Client()) {
                if (!RebaseCol()) return(false);
            }
        }
        // else: Rebase() was already called in tInterfaceWeb::Undo(), so m_RectRebase and m_PositionRebase are already correct
        if (m_Position!=-1) {
            Sheet()->UndoDeleteCol(&m_SaveSelectErase, m_PositionRebase, m_Size);
        } else {
            // For undo of DeleteColByRect, after rebase, we need to use m_RectRebase for both:
            // - Determining where to insert the columns (m_RectRebase accounts for other users' operations)
            // - Determining where to move cells (m_RectRebase is the current position after rebase)
            // The SaveSelect has already been rebased in tInterfaceWeb::Undo() -> Rebase(),
            // so it contains cells at the rebased positions (m_RectRebase).
            // We need to insert columns at m_RectRebase and restore cells at m_RectRebase.
            Sheet()->UndoDeleteColByRect(&m_SaveSelectErase, m_RectRebase, nullptr);
        }
#ifdef checksp
        Sheet()->Check();
#endif
		return(true);
	}

    tBool tUndoDeleteCol::Rebase() {
        return(tUndoAncestorDeleteInsert::Rebase());
    }


#ifdef checkfo
    void tUndoDeleteCol::IncCheckfo(tFormatApi* sFormatApi) {
        m_SaveSelectErase.IncCheckfo(sFormatApi);
    }
#endif

    
#ifdef _DEBUGSK
    tString tUndoDeleteCol::Debug() {
        tStringStream wStream;
        wStream << tUndoSpreadSheetCallBack::Debug();
        wStream << m_SaveSelectErase.Debug();
        return(wStream.str());
    }
#endif
	//=========================================================================
	tUndoDeleteRow::tUndoDeleteRow() :  tUndoAncestorDeleteInsert(true,0,0,tMode(),nullptr)  {
   }
	
	tUndoDeleteRow::tUndoDeleteRow(tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet) : tUndoAncestorDeleteInsert(true,sPosition,sSize,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete row " << sPosition << " to " << (sPosition + sSize - 1);
            OperationName(wStream.str());
        }
	}


    tUndoDeleteRow::tUndoDeleteRow(tRect sRect,tMode sMode, tSheet* sSheet) :tUndoAncestorDeleteInsert(true,sRect,sMode,sSheet) {
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Delete Row " << sRect.StrRef();
            OperationName(wStream.str());
        }
    };
	
	tUndoDeleteRow::~tUndoDeleteRow() {
	};

    tString  tUndoDeleteRow::ClassName() const { return("tUndoDeleteRow"); }

	tSaveSelect* tUndoDeleteRow::SaveSelect() { return(&m_SaveSelectErase); }

	tBool tUndoDeleteRow::Do() {
        // Clear Selection
        m_SaveSelectErase.Clear();
        m_PositionRebase = m_Position;
        m_RectRebase = m_Rect;
        if (m_Position!=-1) {
            Sheet()->DoDeleteRow(&m_SaveSelectErase, m_PositionRebase, m_Size);
        } else {
            Sheet()->DoDeleteRowByRect(&m_SaveSelectErase,m_RectRebase,false);
        }
        return(true);
	};

	tBool tUndoDeleteRow::Undo() {
        if (Sheet() == nullptr) {
            RebindActiveSheet();
        }
        tSheet* wSheet = Sheet();
        if (wSheet == nullptr) {
            return(false);
        }
        // Note: Rebase() may have already been called in tInterfaceWeb::Undo() before calling m_UndoRedoContainer->Undo()
        // In that case, m_RectRebase and m_PositionRebase are already rebased.
        // We should only rebase again if Client() is true AND the rebase hasn't been done yet.
        // Check if rebase was already done by comparing current values with original
        bool wRebaseAlreadyDone = false;
        if (m_Position != -1) {
            // Check if PositionRebase is different from Position (already rebased)
            if (m_PositionRebase != m_Position) {
                wRebaseAlreadyDone = true;
            }
        } else {
            // Check if RectRebase is different from Rect (already rebased)
            if (m_RectRebase.Top() != m_Rect.Top() || m_RectRebase.Bottom() != m_Rect.Bottom() ||
                m_RectRebase.Left() != m_Rect.Left() || m_RectRebase.Right() != m_Rect.Right()) {
                wRebaseAlreadyDone = true;
            }
        }
        
        if (!wRebaseAlreadyDone && !IsJson()) {
            // Rebase hasn't been done yet, initialize and rebase
            m_PositionRebase = m_Position;
            m_RectRebase = m_Rect;
            if (Client()) {
                if (!RebaseRow()) return(false);
            }
        }
        // else: Rebase() was already called in tInterfaceWeb::Undo(), so m_RectRebase and m_PositionRebase are already correct
        if (m_Position!=-1) {
            wSheet->UndoDeleteRow(&m_SaveSelectErase, m_PositionRebase, m_Size);
        } else {
            // For undo of DeleteRowByRect, after rebase, we need to use m_RectRebase for both:
            // - Determining where to insert the rows (m_RectRebase accounts for other users' operations)
            // - Determining where to move cells (m_RectRebase is the current position after rebase)
            // The SaveSelect has already been rebased in tInterfaceWeb::Undo() -> Rebase(),
            // so it contains cells at the rebased positions (m_RectRebase).
            // We need to insert rows at m_RectRebase and restore cells at m_RectRebase.
            wSheet->UndoDeleteRowByRect(&m_SaveSelectErase, m_RectRebase, nullptr);
        }
#ifdef checksp
        wSheet->Check();
#endif
		return(true);
	}

    tBool tUndoDeleteRow::Rebase() {
        return(tUndoAncestorDeleteInsert::Rebase());
    }

#ifdef checkfo
    void tUndoDeleteRow::IncCheckfo(tFormatApi* sFormatApi) {
        m_SaveSelectErase.IncCheckfo(sFormatApi);
    }
#endif


#ifdef _DEBUGSK
    tString tUndoDeleteRow::Debug() {
        tStringStream wStream;
        wStream << tUndoSpreadSheetCallBack::Debug();
        wStream << m_SaveSelectErase.Debug();
        return(wStream.str());
    }
#endif

    //=========================================================================
    tUndoAddSheet::tUndoAddSheet() : tUndoSpreadSheet(tMode()), m_Name(""), m_SheetLeft("") {
    }
    
    tUndoAddSheet::tUndoAddSheet(tString sName, tString sSheetLeft,tMode sMode) : tUndoSpreadSheet(sMode) {
        m_Name=sName;
        m_SheetLeft=sSheetLeft;
        tStringStream wStream;
        if (IsUndoActif()) {
            wStream << "Add Sheet " << m_Name();
            OperationName(wStream.str());
        }
    }
    
    tUndoAddSheet::~tUndoAddSheet() {}

    tString tUndoAddSheet::ClassName() const { return("tUndoAddSheet"); }

    tSaveSelect* tUndoAddSheet::SaveSelect() { return(nullptr); }

    tString tUndoAddSheet::RefRebase() { return(""); }

    tBool tUndoAddSheet::Do() {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetAlreadyExists(wWorkBook,m_Name())) return(false);
        tSheet* wSheet = wWorkBook->Sheet(m_Name());
        if (wSheet != nullptr) { return(false); } // Return false if already exist
        wSheet=wWorkBook->AddSheet(m_Name(),m_SheetLeft());
        return(true);
    }

    tBool tUndoAddSheet::Undo()  {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_Name())) return(false);
        wWorkBook->DeleteSheet(m_Name(),!IsUndoActif());
        return(true);
    };

    tBool tUndoAddSheet::Rebase() {
        // Sheet operations don't need rebase as they operate on sheet names, not positions
        return(true);
    }

    void tUndoAddSheet::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeySheetLeft);
        sWriter->String(m_SheetLeft().c_str());
    }   

    void tUndoAddSheet::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_Name = sValue[kJsonKeyName].GetString();
        m_SheetLeft = sValue[kJsonKeySheetLeft].GetString();
    }

    void tUndoAddSheet::DeleteBeforeDo() {
        // No cleanup needed for AddSheet before Do
    }
    

    //=========================================================================
    tUndoRenameSheet::tUndoRenameSheet() : tUndoSpreadSheet(tMode()), m_Name(""), m_NewName("") {
    }

    tUndoRenameSheet::tUndoRenameSheet(tString sName,tString sNewName,tMode sMode) : tUndoSpreadSheet(sMode) {
        m_Name=sName;
        m_NewName=sNewName;
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Rename Sheet " << m_Name() << " to " << m_NewName();
            OperationName(wStream.str());
        }
    }

    tUndoRenameSheet::~tUndoRenameSheet() {}

    tString tUndoRenameSheet::ClassName() const { return("tUndoRenameSheet"); }

    tSaveSelect* tUndoRenameSheet::SaveSelect() { return(nullptr); }

    tString tUndoRenameSheet::RefRebase() { return(""); }

    tBool tUndoRenameSheet::Do() {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_Name())) return(false);
        tSheet* wSheet = wWorkBook->Sheet(m_Name());
        if (wSheet == nullptr) { return(false); } // Return false if don't exist
        wSheet->Name(m_NewName());
        return(true);
    }

    tBool tUndoRenameSheet::Undo()  {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_NewName())) return(false);
        tSheet* wSheet = wWorkBook->Sheet(m_NewName());
        wSheet->Name(m_Name());
        return(true);
    };

    tBool tUndoRenameSheet::Rebase() {
        // Sheet operations don't need rebase as they operate on sheet names, not positions
        return(true);
    }

    void tUndoRenameSheet::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyNewName);
        sWriter->String(m_NewName().c_str());
    }
    
    void tUndoRenameSheet::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_Name = sValue[kJsonKeyName].GetString();
        m_NewName = sValue[kJsonKeyNewName].GetString();
    }

    void tUndoRenameSheet::DeleteBeforeDo() {
        // No cleanup needed for RenameSheet before Do
    }

    //=========================================================================
    tUndoSwapSheet::tUndoSwapSheet() : tUndoSpreadSheet(tMode()), m_Name1(""), m_Name2(""), m_InsertAfter(false), m_OldAfter("") {
    }

    tUndoSwapSheet::tUndoSwapSheet(tString sName1,tString sName2, tMode sMode, tBool sInsertAfter)
        : tUndoSpreadSheet(sMode), m_InsertAfter(sInsertAfter), m_OldAfter("") {
        m_Name1=sName1;
        m_Name2=sName2;
        if (IsUndoActif()) {
            tStringStream wStream;
            if (m_InsertAfter) {
                wStream << "Move Sheet " << m_Name1() << " after " << m_Name2();
            } else {
                wStream << "Swap Sheet " << m_Name1() << " to " << m_Name2();
            }
            OperationName(wStream.str());
        }
    }

    tUndoSwapSheet::~tUndoSwapSheet() {}

    tString tUndoSwapSheet::ClassName() const { return("tUndoSwapSheet"); }

    tSaveSelect* tUndoSwapSheet::SaveSelect() { return(nullptr); }

    tString tUndoSwapSheet::RefRebase() { return(""); }

    tBool tUndoSwapSheet::Do() {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_Name1())) return(false);
        if (m_InsertAfter) {
            if (!m_Name2().empty() && !TestSheetExists(wWorkBook, m_Name2())) {
                return(false);
            }
            m_OldAfter = wWorkBook->SheetLeftOf(m_Name1());
            wWorkBook->MoveSheet(m_Name1(), m_Name2());
            return(true);
        }
        if (!TestSheetExists(wWorkBook,m_Name2())) return(false);
        wWorkBook->SwapSheets(m_Name1(), m_Name2());
        return(true);
    }

    tBool tUndoSwapSheet::Undo()  {
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_Name1())) return(false);
        if (m_InsertAfter) {
            wWorkBook->MoveSheet(m_Name1(), m_OldAfter());
            return(true);
        }
        if (!TestSheetExists(wWorkBook,m_Name2())) return(false);
        wWorkBook->SwapSheets(m_Name2(), m_Name1());
        return(true);
    };

    tBool tUndoSwapSheet::Rebase() {
        // Sheet operations don't need rebase as they operate on sheet names, not positions
        return(true);
    }

    void tUndoSwapSheet::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName1);
        sWriter->String(m_Name1().c_str());
        sWriter->Key(kJsonKeyName2);
        sWriter->String(m_Name2().c_str());
        if (m_InsertAfter) {
            sWriter->Key("ia");
            sWriter->Bool(m_InsertAfter);
        }
    }   

    void tUndoSwapSheet::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_Name1 = sValue[kJsonKeyName1].GetString();
        m_Name2 = sValue[kJsonKeyName2].GetString();
        m_InsertAfter = sValue.HasMember("ia") && sValue["ia"].GetBool();
        m_OldAfter = "";
    }

    void tUndoSwapSheet::DeleteBeforeDo() {
        // No cleanup needed for SwapSheet before Do
    }
    
	//=========================================================================
	tUndoDeleteSheet::tUndoDeleteSheet() : tUndoSpreadSheetCallBack(0, "", tMode()), m_Index(0), m_Redo(false), m_SaveSelectErase(),m_SheetLeft(""),m_JsonSheetAllocator(0) {
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
	}
	
	tUndoDeleteSheet::tUndoDeleteSheet(tString sName,tMode sMode) : tUndoSpreadSheetCallBack(sName, "", sMode), m_Index(0), m_Redo(false),m_SheetLeft(""),m_JsonSheetAllocator(0) {
        tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        tSheet* wSheet = wWorkBook->Sheet(m_SheetName());
        if (IsUndoActif()) {
            if (wSheet != nullptr) {
                tStringStream wStream;
                wStream << "Delete Sheet " << wSheet->Name();
                OperationName(wStream.str());
            }
        }
        // Anchor UndoSpreadSheet to m_SaveSelectErase.
        m_SaveSelectErase.UndoSpreadSheet(this);
      
        // Search Left position =============================================
        // For Undo
        m_Index=0;
        m_SheetLeft="";

		tVectorAllocatorRef* wVectorSheet = wWorkBook->VectorSheet();
        tVectorAllocatorRef::iterator wIterator=wVectorSheet->begin();
        while (wIterator!=wVectorSheet->end()) {
            tSheet* wSheet = wWorkBook->SheetByAllocator(*wIterator);
            if (wSheet->Name() == m_SheetName()) {
                m_SheetAllocatorRef=*wIterator;
                break;
            }
            m_SheetLeft=tSharedString(wSheet->Name());
            m_Index++;
            wIterator++;
        }
	}

    tUndoDeleteSheet::~tUndoDeleteSheet() {
        // Get Active WorkBook
        tWorkBook* wWorkBook=WorkBook();
    
        if ((!m_Redo) && (!IsJson()))  {
            // Delete Sheet By Allocator
            wWorkBook->DeleteSheet(m_SheetAllocatorRef);
        }
    };

    tString  tUndoDeleteSheet::ClassName() const { return("tUndoDeleteSheet"); }

    tSheet* tUndoDeleteSheet::InsertUndoSheet() {
        tWorkBook* wWorkBook = WorkBook();
        if (wWorkBook == nullptr) {
            wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        }
        if (tSheet* wOffList = wWorkBook->TakeOffListSheet(m_SheetName(), m_Index)) {
            m_SheetAllocatorRef = wOffList->AllocatorRef();
            return(wOffList);
        }
        tSheet* wSheet = wWorkBook->Sheet(m_SheetName());
        if (wSheet==nullptr) {
            wSheet = wWorkBook->AddSheet(m_SheetName(),m_SheetLeft());
            m_SheetAllocatorRef = wSheet->AllocatorRef();
        }
        return(wSheet);
    }

     void  tUndoDeleteSheet::WriteJsonSheet(Writer<StringBuffer>* sWriter,tSheet* sSheet) {
        sWriter->StartObject();
        
        tFormatApi* wFormatApi =sSheet->WorkBook()->FormatApi();
        if (wFormatApi!=nullptr) {
            // Init save Format
            wFormatApi->BeginWriteJson();
        }
       
        // Write Sheet
        sSheet->Json(sWriter);
        
        if (wFormatApi!=nullptr) {
            // save Format
            sWriter->Key("f");
            wFormatApi->Json(sWriter);
        }
        sWriter->EndObject();
    }


	tBool tUndoDeleteSheet::Do() {
        // Raz Selection - always clear for symmetry (Do() should always start fresh)
       m_SaveSelectErase.Clear();
        tBool wOk=true;
        m_Redo=false;
        tWorkBook* wWorkBook=WorkBook();
        if (!TestSheetExists(wWorkBook,m_SheetName())) return(false);
        tSheet* wSheet=wWorkBook->Sheet(m_SheetName());
        if (wSheet!=nullptr) {
            m_SheetAllocatorRef=wSheet->AllocatorRef();
        } else {
            return false;
        }
        
        // Save dependance (cells from other sheet)
		wSheet->DoDeleteSheet(&m_SaveSelectErase);
		return(wOk);
	};

	tBool tUndoDeleteSheet::Undo() {
#ifdef debugundoredo
        cout << m_SaveSelectErase.Debug() << endl;
#endif
        m_Redo=true;
        tWorkBook* wWorkBook=WorkBook();
        if (wWorkBook == nullptr) {
            wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        }
        if (!TestSheetAlreadyExists(wWorkBook,m_SheetName())) return(false);
        tSheet* wSheet=nullptr;
        if (tSheet* wOffList = wWorkBook->TakeOffListSheet(m_SheetName(), m_Index)) {
            wSheet = wOffList;
            m_SheetAllocatorRef = wSheet->AllocatorRef();
            if (IsJson()) {
                m_SaveSelectErase.Json(m_JsonErase, wSheet);
            }
        } else if (!IsJson()) {
            return(false);
        } else {
            wSheet=InsertUndoSheet();
            if (tFormatApi* wFormatApi = wWorkBook->FormatApi()) {
                wFormatApi->BeginWriteJson();
                if (m_JsonSheet.HasMember("f")) {
                    wFormatApi->Json(m_JsonSheet["f"]);
                }
            }
            wSheet->Json(m_JsonSheet);
            m_SaveSelectErase.Json(m_JsonErase, wSheet);
        }

		// Restore all link with another sheet
        wSheet->UndoDeleteSheet(&m_SaveSelectErase);
        if (tFloatingObjectContainer* wFloating = wWorkBook->FloatingObjectContainer()) {
            wFloating->RefreshTargetSheetRefs(wSheet->Name());
        }
        // Peer GetMessage (IsJson): save-based CalculateDo is not enough for cross-sheet deps.
        if (IsJson()) {
            wWorkBook->RecalculateAll();
        } else {
            wSheet->Calculate(m_SaveSelectErase.TempoRectDeleteArea());
        }
        if (tColRowCellRange* wGrid = wSheet->ColRowCellRange()) {
            wGrid->StripOrphanConditionalFormatExtension();
            wGrid->StripMergedOnConditionalFormatRanges();
        }
#ifdef checksp
        wSheet->WorkBook()->Check();
#endif
		return(true);
	}

    void tUndoDeleteSheet::Json(Writer<StringBuffer>* sWriter) {
        // Save SaveSelectErase
        tUndoSpreadSheetCallBack::Json(sWriter);
        sWriter->Key(kJsonKeyIndex);
        sWriter->Int(m_Index);
        sWriter->Key(kJsonKeySheetLeft);
        sWriter->String(m_SheetLeft().c_str());
        if (IsUndo()) {
            tWorkBook* wWorkBook = WorkBook();
            if (wWorkBook == nullptr) {
                wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
            }
            tSheet* wSheet = (wWorkBook != nullptr) ? wWorkBook->SheetByAllocator(m_SheetAllocatorRef) : nullptr;
            if (wSheet != nullptr) {
                sWriter->Key(kJsonKeySaveSelectErase);
                m_SaveSelectErase.Json(sWriter, wSheet);
                sWriter->Key(kJsonKeySheetjson);
                WriteJsonSheet(sWriter, wSheet);
            }
        }
    }

    void tUndoDeleteSheet::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheetCallBack::Json(sValue);
        if (sValue.HasMember(kJsonKeyIndex)) {
            m_Index = sValue[kJsonKeyIndex].GetInt();
        }
        if (sValue.HasMember(kJsonKeySheetLeft)) {
            m_SheetLeft = sValue[kJsonKeySheetLeft].GetString();
        }
    
        if (sValue.HasMember(kJsonKeySaveSelectErase)) {
            // Create a temporary document to properly copy the JSON value
            m_JsonErase.CopyFrom(sValue[kJsonKeySaveSelectErase], m_JsonErase.GetAllocator());
        }
        
        // Note: m_JsonValue is not used in the current implementation
        // The sheet JSON data is handled through other mechanisms
        if (sValue.HasMember(kJsonKeySheetjson)) {
            // Create a temporary document to properly copy the JSON value
            m_JsonSheet.CopyFrom(sValue[kJsonKeySheetjson], m_JsonSheet.GetAllocator());
        }
    }

#ifdef checkfo
    void tUndoDeleteSheet::IncCheckfo(tFormatApi* sFormatApi) {
        if (!m_Redo) {
            tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
            tSheet* wSheet = wWorkBook->SheetByAllocator(m_SheetAllocatorRef);
            // After do wSheet==nullptr if (Json)
            if (wSheet!=nullptr) wSheet->CheckFormat();
        }
    }
#endif

    //=========================================================================
    // Undo for Insert Formula Named
    //=========================================================================
    tUndoInsertFormulaNamed::tUndoInsertFormulaNamed() : tUndoSpreadSheet(tMode()),m_WorkBook(nullptr), m_Name(""), m_Formula(""), m_OldFormula(""), m_Row(0) {
    }

    tUndoInsertFormulaNamed::tUndoInsertFormulaNamed(tString sName,tString sFormula,tMode sMode,tSheet* sSheet) : tUndoSpreadSheet(sMode), m_WorkBook(nullptr), m_Name(sName), m_Formula(sFormula), m_OldFormula(""),m_Row(0) {
        m_WorkBook=sSheet->WorkBook();
    }

    tUndoInsertFormulaNamed::~tUndoInsertFormulaNamed() {
    }

    tString tUndoInsertFormulaNamed::ClassName() const { return("tUndoInsertFormulaNamed"); }

    tBool tUndoInsertFormulaNamed::Do() {
        tRangeNamedContainer*  wRangeNamedContainer=m_WorkBook->RangeNamedContainer();
        tFormulaNamed* wFormulaNamed=wRangeNamedContainer->FormulaNamed(m_Name());
        if (wFormulaNamed!=nullptr) {
            m_Row=wFormulaNamed->Cell()->RowIndex();
            m_OldFormula=wFormulaNamed->Cell()->FormulaWire();
        } else {
            m_OldFormula="";
        }
        const tBool wOk = wRangeNamedContainer->ApplyFormulaNamed(m_Name(), m_Formula(), true, 0);
        if (wOk) {
            tFormulaNamed* const wInserted = wRangeNamedContainer->FormulaNamed(m_Name());
            if (wInserted != nullptr) {
                m_Row = wInserted->DefinitionRow();
                // Persist US A1 for Json / peers (author may have typed locale decimals).
                const tString wWire = wInserted->Cell() != nullptr ? wInserted->Cell()->FormulaWire() : "";
                if (!wWire.empty()) {
                    m_Formula = wWire;
                }
            }
        }
        return (wOk);
    }

    tBool tUndoInsertFormulaNamed::Undo() {
        tRangeNamedContainer* wRangeNamedContainer=m_WorkBook->RangeNamedContainer();
        if (m_OldFormula()!="") {
            tLocalePush wUs("us");
            return(wRangeNamedContainer->ApplyFormulaNamed(m_Name(), m_OldFormula(),true,m_Row));
        } else {
            return(wRangeNamedContainer->DeleteFormulaNamed(m_Name(),true));
        }
        return(true);
    }

    void tUndoInsertFormulaNamed::DeleteBeforeDo() {
        // No cleanup needed for InsertFormulaNamed before Do
    }

    tSaveSelect* tUndoInsertFormulaNamed::SaveSelect() { return(nullptr); }

    tString tUndoInsertFormulaNamed::RefRebase() { return(""); }

    tBool tUndoInsertFormulaNamed::Rebase() { return(true); }

    void tUndoInsertFormulaNamed::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyFormula);
        sWriter->String(m_Formula().c_str());
        sWriter->Key(kJsonKeyOldFormula);
        sWriter->String(m_OldFormula().c_str());
    }

    void tUndoInsertFormulaNamed::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_Name = sValue[kJsonKeyName].GetString();
        m_Formula = sValue[kJsonKeyFormula].GetString();
        m_OldFormula = sValue[kJsonKeyOldFormula].GetString();
    }

    static tBool RebaseHostRowIndex(tIndex& ioRow, const tRebasePlan& sPlan) {
        if (ioRow <= 0) {
            return true;
        }
        const auto wRebasedRow = sPlan.RebaseRow(ioRow);
        if (!wRebasedRow.has_value()) {
            return false;
        }
        ioRow = wRebasedRow.value();
        return true;
    }

    /// @brief Collect cells that depend on a named-formula host cell (_$$N).
    static void CaptureFormulaNamedDependentRefs(tCell* sHostCell, tVectorString& sDependentRefs) {
        sDependentRefs.clear();
        if (sHostCell == nullptr) {
            return;
        }
        for (auto wItem : *sHostCell->ContainerCellDepend()->Container()) {
            if (tCell* wDependent = wItem->Cell(); wDependent != nullptr) {
                sDependentRefs.push_back(wDependent->StrRef(true));
            }
        }
    }

    /// @brief Recompile formula cells from persisted Sheet!ref strings.
    static void RecompilFormulaDependentRefs(tWorkBook* sWorkBook, const tVectorString& sDependentRefs) {
        if (sWorkBook == nullptr) {
            return;
        }
        for (const tString& wRef : sDependentRefs) {
            tString wSheetName;
            tString wCellRef = wRef;
            const size_t wBang = wRef.find('!');
            if (wBang != tString::npos) {
                wSheetName = wRef.substr(0, wBang);
                wCellRef = wRef.substr(wBang + 1);
            }
            tSheet* wSheet = wSheetName.empty() ? sWorkBook->ActiveSheet() : sWorkBook->Sheet(wSheetName);
            if (wSheet == nullptr) {
                continue;
            }
            tTempoPoint wPoint;
            if (!wPoint.ParseRef(wCellRef)) {
                continue;
            }
            tCell* wCell = wSheet->Cell(static_cast<tInt>(wPoint.Row()), static_cast<tInt>(wPoint.Col()));
            if (wCell == nullptr) {
                continue;
            }
            const tString wFormula = wCell->FormulaStr();
            if (wFormula.empty()) {
                continue;
            }
            (void)sWorkBook->CompilCell(wCell, wFormula.c_str());
            wCell->Calculation();
        }
    }

    /// @brief Snapshot named-formula cells on _$$N for undo/redo.
    static void CaptureFormulaNamedCells(tSaveSelect& sSaveSelect, tFormulaNamed* sFormulaNamed) {
        if (sFormulaNamed == nullptr) {
            return;
        }
        tCell* const wFormulaCell = sFormulaNamed->Cell();
        if (wFormulaCell != nullptr && sSaveSelect.FindCell(wFormulaCell) == nullptr) {
            sSaveSelect.AddCell(wFormulaCell);
        }
        tWorkBook* const wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        tSheet* const wSheet = (wWorkBook != nullptr) ? wWorkBook->SheetNamedFormula() : nullptr;
        if (wSheet == nullptr) {
            return;
        }
        tCell* const wNameCell = wSheet->Cell(
            static_cast<tInt>(sFormulaNamed->DefinitionRow()),
            static_cast<tInt>(tFormulaNamed::kFormulaNamedNameLabelCol));
        if (wNameCell != nullptr && sSaveSelect.FindCell(wNameCell) == nullptr) {
            sSaveSelect.AddCell(wNameCell);
        }
    }

    static tString FormulaNamedRowRefRebase(tWorkBook* sWorkBook, tIndex sRow) {
        if (sWorkBook == nullptr || sRow <= 0) {
            return "";
        }
        tSheet* const wSheet = sWorkBook->SheetNamedFormula();
        if (wSheet == nullptr) {
            return "";
        }
        tCell* const wCell = wSheet->Cell(static_cast<tInt>(sRow), 1);
        if (wCell == nullptr) {
            return "";
        }
        return wCell->StrRef(true);
    }

    //=========================================================================
    // Undo for Delete Formula Named
    //=========================================================================
    tUndoDeleteFormulaNamed::tUndoDeleteFormulaNamed()
        : tUndoSpreadSheet(tMode()),
          m_WorkBook(nullptr),
          m_Name(""),
          m_Formula(""),
          m_Row(0),
          m_SaveSelect() {
        m_SaveSelect.UndoSpreadSheet(this);
    }

    tUndoDeleteFormulaNamed::tUndoDeleteFormulaNamed(tString sName, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_WorkBook(nullptr),
          m_Name(sName),
          m_Formula(""),
          m_Row(0),
          m_SaveSelect() {
        m_WorkBook = sSheet->WorkBook();
        m_SaveSelect.UndoSpreadSheet(this);
        tFormulaNamed* const wFormulaNamed = m_WorkBook->RangeNamedContainer()->FormulaNamed(m_Name());
        if (wFormulaNamed != nullptr) {
            m_Row = wFormulaNamed->DefinitionRow();
            m_Formula = wFormulaNamed->Cell() != nullptr ? wFormulaNamed->Cell()->FormulaWire() : wFormulaNamed->FormulaStr();
            CaptureFormulaNamedCells(m_SaveSelect, wFormulaNamed);
            CaptureFormulaNamedDependentRefs(wFormulaNamed->Cell(), m_DependentRefs);
        }
    }

    tUndoDeleteFormulaNamed::~tUndoDeleteFormulaNamed() {
        m_SaveSelect.Clear();
    }

    tString tUndoDeleteFormulaNamed::ClassName() const { return ("tUndoDeleteFormulaNamed"); }

    tSheet* tUndoDeleteFormulaNamed::Sheet() {
        if (m_WorkBook == nullptr) {
            return nullptr;
        }
        return m_WorkBook->SheetNamedFormula();
    }

    tBool tUndoDeleteFormulaNamed::Do() {
        tRangeNamedContainer* const wRangeNamedContainer = m_WorkBook->RangeNamedContainer();
        tFormulaNamed* const wFormulaNamed = wRangeNamedContainer->FormulaNamed(m_Name());
        if (wFormulaNamed == nullptr) {
            return false;
        }
        m_Row = wFormulaNamed->DefinitionRow();
        m_Formula = wFormulaNamed->Cell() != nullptr ? wFormulaNamed->Cell()->FormulaWire() : wFormulaNamed->FormulaStr();
        CaptureFormulaNamedCells(m_SaveSelect, wFormulaNamed);
        CaptureFormulaNamedDependentRefs(wFormulaNamed->Cell(), m_DependentRefs);
        return wRangeNamedContainer->DeleteFormulaNamed(m_Name(), true);
    }

    tBool tUndoDeleteFormulaNamed::Undo() {
        tRangeNamedContainer* const wRangeNamedContainer = m_WorkBook->RangeNamedContainer();
        tLocalePush wUs("us");
        if (!wRangeNamedContainer->ApplyFormulaNamed(m_Name(), m_Formula(), false, m_Row)) {
            return false;
        }
        tSheet* const wSheet = m_WorkBook->SheetNamedFormula();
        if (wSheet == nullptr) {
            return false;
        }
        m_SaveSelect.ColRowCellRange(wSheet->ColRowCellRange());
        m_SaveSelect.UndoCells();
        tFormulaNamed* const wFormulaNamed = wRangeNamedContainer->FormulaNamed(m_Name());
        if (wFormulaNamed == nullptr) {
            return false;
        }
        if (!wFormulaNamed->Compil(m_Formula(), true)) {
            return false;
        }
        RecompilFormulaDependentRefs(m_WorkBook, m_DependentRefs);
        return true;
    }

    void tUndoDeleteFormulaNamed::DeleteBeforeDo() {
        m_SaveSelect.Clear();
    }

    tSaveSelect* tUndoDeleteFormulaNamed::SaveSelect() { return (&m_SaveSelect); }

    tString tUndoDeleteFormulaNamed::RefRebase() {
        return FormulaNamedRowRefRebase(m_WorkBook, m_Row);
    }

    tBool tUndoDeleteFormulaNamed::Rebase() {
        if (m_WorkBook == nullptr) {
            return true;
        }
        SetRebasePlan();
        if (m_RebasePlan.IsEmpty()) {
            return true;
        }
        if (!RebaseHostRowIndex(m_Row, m_RebasePlan)) {
            return false;
        }
        if (!m_SaveSelect.Rebase(m_RebasePlan)) {
            return false;
        }
        return true;
    }

    void tUndoDeleteFormulaNamed::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyFormula);
        sWriter->String(m_Formula().c_str());
        sWriter->Key(kJsonKeyHostRow);
        sWriter->Int(static_cast<int>(m_Row));
    }

    void tUndoDeleteFormulaNamed::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_WorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        m_Name = sValue[kJsonKeyName].GetString();
        m_Formula = sValue[kJsonKeyFormula].GetString();
        if (sValue.HasMember(kJsonKeyHostRow)) {
            m_Row = static_cast<tIndex>(sValue[kJsonKeyHostRow].GetInt());
        }
    }

    /// @brief Snapshot host cell on _$$A for undo/redo and rebase of cell class.
    static void CaptureFloatingObjectHostCell(tSaveSelect& sSaveSelect, tFloatingObject* sObject) {
        if (sObject == nullptr) {
            return;
        }
        tCell* const wHost = sObject->HostCell();
        if (wHost == nullptr) {
            return;
        }
        if (sSaveSelect.FindCell(wHost) == nullptr) {
            sSaveSelect.AddCell(wHost);
        }
    }

    static tRebasePlan BuildRebasePlanForSheet(tWorkBook* sWorkBook, tSheet* sSheet, tSequenceId sFromSequence) {
        tRebasePlan wPlan;
        if (sWorkBook == nullptr || sSheet == nullptr) {
            return wPlan;
        }
        const tSequenceId wCurrentSequence = sWorkBook->UndoRebaseLog().LatestSequence();
        if (wCurrentSequence > sFromSequence) {
            wPlan = sWorkBook->UndoRebaseLog().BuildPlan(sFromSequence, wCurrentSequence, sSheet->AllocatorRef());
        }
        return wPlan;
    }

    /// @brief Rebase tFloatingObjectLayout anchor row/col refs (m_AnchorRowRef/m_AnchorColRef).
    static tBool RebaseFloatingObjectLayoutOnAnchorSheet(tWorkBook* sWorkBook,
                                                         tSequenceId sFromSequence,
                                                         tFloatingObjectLayout& sLayout) {
        if (sWorkBook == nullptr) {
            return true;
        }
        tCell* const wAnchor = sLayout.AnchorCell(sWorkBook);
        if (wAnchor == nullptr || wAnchor->Sheet() == nullptr) {
            return true;
        }
        const tRebasePlan wPlan = BuildRebasePlanForSheet(sWorkBook, wAnchor->Sheet(), sFromSequence);
        if (wPlan.m_Operations.empty()) {
            return true;
        }
        return sLayout.RebaseAnchor(sWorkBook, wPlan);
    }

    /// @brief Cell ref string for host row on _$$A (used by RefRebase).
    static tString HostRowRefRebase(tWorkBook* sWorkBook, tIndex sHostRow) {
        if (sWorkBook == nullptr || sHostRow <= 0) {
            return "";
        }
        tSheet* const wHostSheet = sWorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return "";
        }
        tCell* const wCell = wHostSheet->Cell(static_cast<tInt>(sHostRow), 1);
        if (wCell == nullptr) {
            return "";
        }
        return wCell->StrRef(true);
    }

    //=========================================================================
    // Undo for Insert Floating Object
    //=========================================================================
    tUndoInsertFloatingObject::tUndoInsertFloatingObject()
        : tUndoSpreadSheet(tMode()),
          m_WorkBook(nullptr),
          m_Name(""),
          m_ClassName(""),
          m_TargetSheetName(""),
          m_HostRow(0),
          m_ExistedBefore(false),
          m_OldClassName(""),
          m_OldTargetSheetName(""),
          m_OldHostRow(0),
          m_OldLayout(),
          m_UseInitialLayout(false),
          m_InitialAnchorRef(""),
          m_InitialLayout(),
          m_SaveSelect() {
        m_SaveSelect.UndoSpreadSheet(this);
    }

    tUndoInsertFloatingObject::tUndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_WorkBook(nullptr),
          m_Name(sName),
          m_ClassName(sClassName),
          m_TargetSheetName(sTargetSheetName),
          m_HostRow(sHostRow),
          m_ExistedBefore(false),
          m_OldClassName(""),
          m_OldTargetSheetName(""),
          m_OldHostRow(0),
          m_OldLayout(),
          m_UseInitialLayout(false),
          m_InitialAnchorRef(""),
          m_InitialLayout(),
          m_SaveSelect() {
        m_WorkBook = sSheet->WorkBook();
        m_SaveSelect.UndoSpreadSheet(this);
    }

    tUndoInsertFloatingObject::tUndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName,
                                                         tIndex sHostRow, tBool sUseInitialLayout,
                                                         tString sInitialAnchorRef,
                                                         const tFloatingObjectLayout& sInitialLayout, tMode sMode,
                                                         tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_WorkBook(nullptr),
          m_Name(sName),
          m_ClassName(sClassName),
          m_TargetSheetName(sTargetSheetName),
          m_HostRow(sHostRow),
          m_ExistedBefore(false),
          m_OldClassName(""),
          m_OldTargetSheetName(""),
          m_OldHostRow(0),
          m_OldLayout(),
          m_UseInitialLayout(sUseInitialLayout),
          m_InitialAnchorRef(sInitialAnchorRef),
          m_InitialLayout(sInitialLayout),
          m_SaveSelect() {
        m_WorkBook = sSheet->WorkBook();
        m_SaveSelect.UndoSpreadSheet(this);
    }

    tUndoInsertFloatingObject::~tUndoInsertFloatingObject() {
        m_SaveSelect.Clear();
    }

    tString tUndoInsertFloatingObject::ClassName() const { return ("tUndoInsertFloatingObject"); }

    tSheet* tUndoInsertFloatingObject::Sheet() {
        if (m_WorkBook == nullptr) {
            return nullptr;
        }
        return m_WorkBook->SheetClassAnchor();
    }

    tBool tUndoInsertFloatingObject::Do() {
        tFloatingObjectContainer* const wContainer = m_WorkBook->FloatingObjectContainer();
        tFloatingObject* const wExisting = wContainer->ByName(m_Name());
        m_ExistedBefore = (wExisting != nullptr);
        if (m_ExistedBefore) {
            m_OldClassName = wExisting->ClassName();
            m_OldTargetSheetName = wExisting->TargetSheetName();
            m_OldHostRow = wExisting->HostRow();
            m_OldLayout.Assign(wExisting->Layout());
            CaptureFloatingObjectHostCell(m_SaveSelect, wExisting);
        }

        tFloatingObject* const wObject = wContainer->Apply(m_Name(), m_ClassName(), m_TargetSheetName(), m_HostRow);
        if (wObject == nullptr) {
            return false;
        }
        m_HostRow = wObject->HostRow();
        if (!EnsureFloatingObjectHostClass(m_WorkBook, wObject, m_ClassName())) {
            return false;
        }
        if (m_UseInitialLayout) {
            const tInt wAutoZ = wObject->Layout().ZIndex();
            tFloatingObjectLayout wLayout(m_InitialLayout);
            if (!m_InitialAnchorRef.empty()) {
                tCell* const wAnchor = FloatingObjectAnchorCellFromRef(m_InitialAnchorRef, wObject->HostCell());
                if (wAnchor == nullptr) {
                    return false;
                }
                wLayout.AnchorCell(wAnchor);
            }
            wObject->Layout().Assign(wLayout);
            if (wObject->Layout().ZIndex() <= 0) {
                wObject->Layout().ZIndex(wAutoZ > 0 ? wAutoZ : 1);
            }
        }
        return true;
    }

    tBool tUndoInsertFloatingObject::Undo() {
        tFloatingObjectContainer* const wContainer = m_WorkBook->FloatingObjectContainer();
        if (m_ExistedBefore) {
            tFloatingObject* const wObject = wContainer->Apply(m_Name(), m_OldClassName(), m_OldTargetSheetName(), m_OldHostRow);
            if (wObject == nullptr) {
                return false;
            }
            wObject->Layout().Assign(m_OldLayout);
            if (!EnsureFloatingObjectHostClass(m_WorkBook, wObject, m_OldClassName())) {
                return false;
            }
            m_SaveSelect.ColRowCellRange(m_WorkBook->SheetClassAnchor()->ColRowCellRange());
            m_SaveSelect.UndoCells();
            return true;
        }

        tFloatingObject* const wObject = wContainer->ByName(m_Name());
        if (wObject != nullptr) {
            CaptureFloatingObjectHostCell(m_SaveSelect, wObject);
            tCell* const wHost = wObject->HostCell();
            if (wHost != nullptr && wHost->Sheet() != nullptr) {
                wHost->Sheet()->DeleteCellClassAttributeContainer(wHost);
            }
        }
        return wContainer->DeleteByName(m_Name(), false, true);
    }

    void tUndoInsertFloatingObject::DeleteBeforeDo() {
        m_SaveSelect.Clear();
    }

    tSaveSelect* tUndoInsertFloatingObject::SaveSelect() { return (&m_SaveSelect); }

    tString tUndoInsertFloatingObject::RefRebase() {
        return HostRowRefRebase(m_WorkBook, m_HostRow);
    }

    tBool tUndoInsertFloatingObject::Rebase() {
        if (m_WorkBook == nullptr) {
            return true;
        }
        SetRebasePlan();
        if (m_RebasePlan.IsEmpty()) {
            return true;
        }
        if (!RebaseHostRowIndex(m_HostRow, m_RebasePlan)) {
            return false;
        }
        if (!RebaseHostRowIndex(m_OldHostRow, m_RebasePlan)) {
            return false;
        }
        if (!m_SaveSelect.Rebase(m_RebasePlan)) {
            return false;
        }
        return true;
    }

    void tUndoInsertFloatingObject::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyClassName);
        sWriter->String(m_ClassName().c_str());
        sWriter->Key(kJsonKeyTargetSheet);
        sWriter->String(m_TargetSheetName().c_str());
        sWriter->Key(kJsonKeyHostRow);
        sWriter->Int(static_cast<int>(m_HostRow));
        sWriter->Key(kJsonKeyExistedBefore);
        sWriter->Bool(m_ExistedBefore);
        sWriter->Key("oc");
        sWriter->String(m_OldClassName().c_str());
        sWriter->Key("ot");
        sWriter->String(m_OldTargetSheetName().c_str());
        sWriter->Key("ohr");
        sWriter->Int(static_cast<int>(m_OldHostRow));
        sWriter->Key(kJsonKeyLayoutOld);
        sWriter->StartObject();
        m_OldLayout.Json(sWriter, m_WorkBook);
        sWriter->EndObject();
        sWriter->Key("uil");
        sWriter->Bool(m_UseInitialLayout);
        if (m_UseInitialLayout) {
            sWriter->Key("iar");
            sWriter->String(m_InitialAnchorRef.c_str());
            sWriter->Key("il");
            sWriter->StartObject();
            m_InitialLayout.Json(sWriter, m_WorkBook);
            sWriter->EndObject();
        }
    }

    void tUndoInsertFloatingObject::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_WorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        m_Name = sValue[kJsonKeyName].GetString();
        m_ClassName = sValue[kJsonKeyClassName].GetString();
        m_TargetSheetName = sValue[kJsonKeyTargetSheet].GetString();
        m_HostRow = static_cast<tIndex>(sValue[kJsonKeyHostRow].GetInt());
        m_ExistedBefore = sValue[kJsonKeyExistedBefore].GetBool();
        if (sValue.HasMember("oc")) {
            m_OldClassName = sValue["oc"].GetString();
        }
        if (sValue.HasMember("ot")) {
            m_OldTargetSheetName = sValue["ot"].GetString();
        }
        if (sValue.HasMember("ohr")) {
            m_OldHostRow = static_cast<tIndex>(sValue["ohr"].GetInt());
        }
        if (sValue.HasMember(kJsonKeyLayoutOld)) {
            tSheet* wHostSheet = (m_WorkBook != nullptr) ? m_WorkBook->SheetClassAnchor() : nullptr;
            tCell* wHostCell = nullptr;
            if (wHostSheet != nullptr && m_OldHostRow > 0) {
                wHostCell = wHostSheet->Cell(static_cast<tInt>(m_OldHostRow), 1);
            }
            m_OldLayout.Json(sValue[kJsonKeyLayoutOld], m_WorkBook, wHostCell);
        }
        m_UseInitialLayout = sValue.HasMember("uil") && sValue["uil"].GetBool();
        m_InitialAnchorRef = "";
        m_InitialLayout = tFloatingObjectLayout();
        if (m_UseInitialLayout) {
            if (sValue.HasMember("iar") && sValue["iar"].IsString()) {
                m_InitialAnchorRef = sValue["iar"].GetString();
            }
            if (sValue.HasMember("il")) {
                tSheet* wHostSheet = (m_WorkBook != nullptr) ? m_WorkBook->SheetClassAnchor() : nullptr;
                tCell* wHostCell = nullptr;
                if (wHostSheet != nullptr && m_HostRow > 0) {
                    wHostCell = wHostSheet->EnsureCell(static_cast<tInt>(m_HostRow), 1);
                }
                m_InitialLayout.Json(sValue["il"], m_WorkBook, wHostCell);
            }
        }
    }

    //=========================================================================
    // Undo for Delete Floating Object
    //=========================================================================
    tUndoDeleteFloatingObject::tUndoDeleteFloatingObject()
        : tUndoSpreadSheet(tMode()),
          m_WorkBook(nullptr),
          m_Name(""),
          m_ClassName(""),
          m_TargetSheetName(""),
          m_HostRow(0),
          m_Layout(),
          m_SaveSelect() {
        m_SaveSelect.UndoSpreadSheet(this);
    }

    tUndoDeleteFloatingObject::tUndoDeleteFloatingObject(tString sName, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_WorkBook(nullptr),
          m_Name(sName),
          m_ClassName(""),
          m_TargetSheetName(""),
          m_HostRow(0),
          m_Layout(),
          m_SaveSelect() {
        m_WorkBook = sSheet->WorkBook();
        m_SaveSelect.UndoSpreadSheet(this);
        tFloatingObject* const wObject = m_WorkBook->FloatingObjectContainer()->ByName(m_Name());
        if (wObject != nullptr) {
            m_ClassName = wObject->ClassName();
            m_TargetSheetName = wObject->TargetSheetName();
            m_HostRow = wObject->HostRow();
            m_Layout.Assign(wObject->Layout());
            CaptureFloatingObjectHostCell(m_SaveSelect, wObject);
        }
    }

    tUndoDeleteFloatingObject::~tUndoDeleteFloatingObject() {
        m_SaveSelect.Clear();
    }

    tString tUndoDeleteFloatingObject::ClassName() const { return ("tUndoDeleteFloatingObject"); }

    tSheet* tUndoDeleteFloatingObject::Sheet() {
        if (m_WorkBook == nullptr) {
            return nullptr;
        }
        return m_WorkBook->SheetClassAnchor();
    }

    tBool tUndoDeleteFloatingObject::Do() {
        tFloatingObjectContainer* const wContainer = m_WorkBook->FloatingObjectContainer();
        tFloatingObject* const wObject = wContainer->ByName(m_Name());
        if (wObject == nullptr) {
            return false;
        }
        CaptureFloatingObjectHostCell(m_SaveSelect, wObject);
        return wContainer->DeleteByName(m_Name(), true);
    }

    tBool tUndoDeleteFloatingObject::Undo() {
        tFloatingObjectContainer* const wContainer = m_WorkBook->FloatingObjectContainer();
        tFloatingObject* const wObject = wContainer->Apply(m_Name(), m_ClassName(), m_TargetSheetName(), m_HostRow);
        if (wObject == nullptr) {
            return false;
        }
        wObject->Layout().Assign(m_Layout);
        if (!EnsureFloatingObjectHostClass(m_WorkBook, wObject, m_ClassName())) {
            return false;
        }
        tSheet* const wHostSheet = m_WorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return false;
        }
        m_SaveSelect.ColRowCellRange(wHostSheet->ColRowCellRange());
        m_SaveSelect.UndoCells();
        return true;
    }

    void tUndoDeleteFloatingObject::DeleteBeforeDo() {
        m_SaveSelect.Clear();
    }

    tSaveSelect* tUndoDeleteFloatingObject::SaveSelect() { return (&m_SaveSelect); }

    tString tUndoDeleteFloatingObject::RefRebase() {
        return HostRowRefRebase(m_WorkBook, m_HostRow);
    }

    tBool tUndoDeleteFloatingObject::Rebase() {
        if (m_WorkBook == nullptr) {
            return true;
        }
        SetRebasePlan();
        if (m_RebasePlan.IsEmpty()) {
            return true;
        }
        if (!RebaseHostRowIndex(m_HostRow, m_RebasePlan)) {
            return false;
        }
        if (!m_SaveSelect.Rebase(m_RebasePlan)) {
            return false;
        }
        return true;
    }

    void tUndoDeleteFloatingObject::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyClassName);
        sWriter->String(m_ClassName().c_str());
        sWriter->Key(kJsonKeyTargetSheet);
        sWriter->String(m_TargetSheetName().c_str());
        sWriter->Key(kJsonKeyHostRow);
        sWriter->Int(static_cast<int>(m_HostRow));
        sWriter->Key(kJsonKeyLayoutOld);
        sWriter->StartObject();
        m_Layout.Json(sWriter, m_WorkBook);
        sWriter->EndObject();
    }

    void tUndoDeleteFloatingObject::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_WorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        m_Name = sValue[kJsonKeyName].GetString();
        m_ClassName = sValue[kJsonKeyClassName].GetString();
        m_TargetSheetName = sValue[kJsonKeyTargetSheet].GetString();
        m_HostRow = static_cast<tIndex>(sValue[kJsonKeyHostRow].GetInt());
        if (sValue.HasMember(kJsonKeyLayoutOld)) {
            tSheet* wHostSheet = (m_WorkBook != nullptr) ? m_WorkBook->SheetClassAnchor() : nullptr;
            tCell* wHostCell = nullptr;
            if (wHostSheet != nullptr && m_HostRow > 0) {
                wHostCell = wHostSheet->Cell(static_cast<tInt>(m_HostRow), 1);
            }
            m_Layout.Json(sValue[kJsonKeyLayoutOld], m_WorkBook, wHostCell);
        }
    }

    //=========================================================================
    // Undo for Floating Object Layout
    //=========================================================================
    tUndoFloatingObjectLayout::tUndoFloatingObjectLayout()
        : tUndoSpreadSheet(tMode()),
          m_WorkBook(nullptr),
          m_Name(""),
          m_TargetSheetName(""),
          m_OldLayout(),
          m_NewLayout(),
          m_ChangeAnchor(false) {}

    tUndoFloatingObjectLayout::tUndoFloatingObjectLayout(tString sName, const tFloatingObjectLayout& sNewLayout, tBool sChangeAnchor, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_WorkBook(nullptr),
          m_Name(sName),
          m_TargetSheetName(""),
          m_OldLayout(),
          m_NewLayout(sNewLayout),
          m_ChangeAnchor(sChangeAnchor) {
        m_WorkBook = sSheet->WorkBook();
        tFloatingObject* const wObject = m_WorkBook->FloatingObjectContainer()->ByName(m_Name());
        if (wObject != nullptr) {
            m_TargetSheetName = wObject->TargetSheetName();
        }
    }

    tUndoFloatingObjectLayout::~tUndoFloatingObjectLayout() {}

    tString tUndoFloatingObjectLayout::ClassName() const { return ("tUndoFloatingObjectLayout"); }

    tSheet* tUndoFloatingObjectLayout::Sheet() {
        if (m_WorkBook == nullptr) {
            return nullptr;
        }
        if (!m_TargetSheetName().empty()) {
            return m_WorkBook->Sheet(m_TargetSheetName());
        }
        tFloatingObject* const wObject = m_WorkBook->FloatingObjectContainer()->ByName(m_Name());
        if (wObject == nullptr) {
            return nullptr;
        }
        return m_WorkBook->Sheet(wObject->TargetSheetName());
    }

    tBool tUndoFloatingObjectLayout::Do() {
        tFloatingObject* const wObject = m_WorkBook->FloatingObjectContainer()->ByName(m_Name());
        if (wObject == nullptr) {
            return false;
        }
        m_TargetSheetName = wObject->TargetSheetName();
        m_OldLayout.Assign(wObject->Layout());
        return m_WorkBook->FloatingObjectContainer()->ApplyLayout(m_Name(), m_NewLayout);
    }

    tBool tUndoFloatingObjectLayout::Undo() {
        return m_WorkBook->FloatingObjectContainer()->ApplyLayout(m_Name(), m_OldLayout);
    }

    void tUndoFloatingObjectLayout::DeleteBeforeDo() {}

    tSaveSelect* tUndoFloatingObjectLayout::SaveSelect() { return nullptr; }

    tString tUndoFloatingObjectLayout::RefRebase() {
        if (m_WorkBook == nullptr) {
            return "";
        }
        tCell* wAnchor = m_NewLayout.AnchorCell(m_WorkBook);
        if (wAnchor == nullptr) {
            wAnchor = m_OldLayout.AnchorCell(m_WorkBook);
        }
        if (wAnchor == nullptr) {
            return "";
        }
        return wAnchor->StrRef(true);
    }

    tBool tUndoFloatingObjectLayout::Rebase() {
        if (m_WorkBook == nullptr) {
            return true;
        }
        if (!RebaseFloatingObjectLayoutOnAnchorSheet(m_WorkBook, SequenceId(), m_OldLayout)) {
            return false;
        }
        if (!RebaseFloatingObjectLayoutOnAnchorSheet(m_WorkBook, SequenceId(), m_NewLayout)) {
            return false;
        }
        return true;
    }

    namespace {

        static tDouble ReadUndoLayoutDouble(const rapidjson::Value& sValue, const char* sKey, tDouble sDefault) {
            if (sValue.HasMember(sKey) && sValue[sKey].IsNumber()) {
                return sValue[sKey].GetDouble();
            }
            return sDefault;
        }

        static void ApplyCompactLayoutFromUndoJson(const rapidjson::Value& sValue, tFloatingObjectLayout& sLayout) {
            sLayout.DiffX(ReadUndoLayoutDouble(sValue, "dx", sLayout.DiffX()));
            sLayout.DiffY(ReadUndoLayoutDouble(sValue, "dy", sLayout.DiffY()));
            sLayout.Width(ReadUndoLayoutDouble(sValue, "w", sLayout.Width()));
            sLayout.Height(ReadUndoLayoutDouble(sValue, "h", sLayout.Height()));
            sLayout.Opacity(ReadUndoLayoutDouble(sValue, "op", sLayout.Opacity()));
            if (sValue.HasMember("zi") && sValue["zi"].IsInt()) {
                sLayout.ZIndex(sValue["zi"].GetInt());
            }
            sLayout.EnsureDefaults();
        }

        static tBool HasCompactLayoutUndoJson(const rapidjson::Value& sValue) {
            return sValue.HasMember("dx") || sValue.HasMember("dy") || sValue.HasMember("w")
                || sValue.HasMember("h") || sValue.HasMember("op") || sValue.HasMember("zi");
        }

    } // namespace

    void tUndoFloatingObjectLayout::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        sWriter->Key(kJsonKeyName);
        sWriter->String(m_Name().c_str());
        sWriter->Key(kJsonKeyTargetSheet);
        sWriter->String(m_TargetSheetName().c_str());
        sWriter->Key("ca");
        sWriter->Bool(m_ChangeAnchor);
        sWriter->Key(kJsonKeyLayoutOld);
        sWriter->StartObject();
        m_OldLayout.Json(sWriter, m_WorkBook);
        sWriter->EndObject();
        sWriter->Key(kJsonKeyLayoutNew);
        sWriter->StartObject();
        m_NewLayout.Json(sWriter, m_WorkBook);
        sWriter->EndObject();
        // Compact wire format for collaborative replay (same keys as JsonFloatingObjectsForSheet).
        sWriter->Key("dx");
        sWriter->Double(m_NewLayout.DiffX());
        sWriter->Key("dy");
        sWriter->Double(m_NewLayout.DiffY());
        sWriter->Key("w");
        sWriter->Double(m_NewLayout.Width());
        sWriter->Key("h");
        sWriter->Double(m_NewLayout.Height());
        sWriter->Key("op");
        sWriter->Double(m_NewLayout.Opacity());
        if (m_NewLayout.ZIndex() > 0) {
            sWriter->Key("zi");
            sWriter->Int(m_NewLayout.ZIndex());
        }
    }

    void tUndoFloatingObjectLayout::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        m_WorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        m_Name = sValue[kJsonKeyName].GetString();
        if (sValue.HasMember(kJsonKeyTargetSheet)) {
            m_TargetSheetName = sValue[kJsonKeyTargetSheet].GetString();
        }
        m_ChangeAnchor = sValue.HasMember("ca") && sValue["ca"].GetBool();
        tFloatingObject* const wObject = (m_WorkBook != nullptr) ? m_WorkBook->FloatingObjectContainer()->ByName(m_Name()) : nullptr;
        tCell* wHostCell = (wObject != nullptr) ? wObject->HostCell() : nullptr;
        if (sValue.HasMember(kJsonKeyLayoutOld)) {
            m_OldLayout.Json(sValue[kJsonKeyLayoutOld], m_WorkBook, wHostCell);
        }
        if (sValue.HasMember(kJsonKeyLayoutNew)) {
            m_NewLayout.Json(sValue[kJsonKeyLayoutNew], m_WorkBook, wHostCell);
        } else if (HasCompactLayoutUndoJson(sValue)) {
            if (wObject != nullptr) {
                m_NewLayout.Assign(wObject->Layout());
            }
            ApplyCompactLayoutFromUndoJson(sValue, m_NewLayout);
        }
    }

    // Open Close node ========================================================
    tUndoOpenCloseTree::tUndoOpenCloseTree() : tUndoSpreadSheet(tMode()), m_IsRow(false), m_Position(0),
        m_Sheet(nullptr), m_TargetOpen(true), m_HasTargetOpen(false) {
    }
    
    tUndoOpenCloseTree::tUndoOpenCloseTree(tBool sIsRow,tIndex sPosition,tMode sMode, tSheet* sSheet) : tUndoSpreadSheet(sMode),
        m_IsRow(sIsRow),
        m_Position(sPosition),
        m_Sheet(sSheet),
        m_TargetOpen(true),
        m_HasTargetOpen(false) {
            if (IsUndoActif()) {
                tStringStream wStream;
                wStream << "Change tree ";
                if (m_IsRow) {
                    wStream << "open close row "  << sPosition;
                } else {
                    wStream << "open clode column " << Base10ToAlpha(sPosition);
                }
                OperationName(wStream.str());
            }
    }

    void tUndoOpenCloseTree::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        if (m_Sheet != nullptr) {
            sWriter->Key(kJsonKeySheet);
            sWriter->String(m_Sheet->Name().c_str());
        }
        sWriter->Key(kJsonKeyIsRow);
        sWriter->Bool(m_IsRow);
        sWriter->Key(kJsonKeyPosition);
        sWriter->Int(m_Position);
        if (m_HasTargetOpen) {
            sWriter->Key(kJsonKeyTreeOpen);
            sWriter->Bool(m_TargetOpen);
        }
    }

    void tUndoOpenCloseTree::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        if (sValue.HasMember(kJsonKeySheet)) {
            tString wSheetName=sValue[kJsonKeySheet].GetString();
            m_Sheet=tSpreadSheetContainer::Instance()->ActiveWorkBook()->Sheet(wSheetName);
        }
        if (sValue.HasMember(kJsonKeyIsRow)) {
            m_IsRow = sValue[kJsonKeyIsRow].GetBool();
        }
        if (sValue.HasMember(kJsonKeyPosition)) {
            m_Position = sValue[kJsonKeyPosition].GetInt();
        }
        m_HasTargetOpen = false;
        if (sValue.HasMember(kJsonKeyTreeOpen)) {
            m_TargetOpen = sValue[kJsonKeyTreeOpen].GetBool();
            m_HasTargetOpen = true;
        }
    }

    tString tUndoOpenCloseTree::ClassName() const { return("tUndoOpenCloseTree");}

    tSheet* tUndoOpenCloseTree::Sheet() { return(m_Sheet); }

    tSaveSelect* tUndoOpenCloseTree::SaveSelect() { return(nullptr); }

    tString tUndoOpenCloseTree::RefRebase() { return(""); }
    
    tBool tUndoOpenCloseTree::Do()  {
        if (m_Sheet == nullptr) {
            return(false);
        }
        tColRowCellRange* wGrid = m_Sheet->ColRowCellRange();
        if (wGrid == nullptr) {
            return(false);
        }
        // Parent "c" without child "p" must be repaired before Renum, otherwise
        // Renum clears m_Children and OpenClose can never reopen the node.
        wGrid->SyncTreeParentRefsFromChildren(m_IsRow);
        tIndex wRoot = wGrid->GrandParent(m_Position, m_IsRow);
        if (m_IsRow) {
            wGrid->RenumRow(wRoot);
        } else {
            wGrid->RenumCol(wRoot);
        }
        tColRow* wColRow = m_IsRow ? m_Sheet->Row(m_Position) : m_Sheet->Col(m_Position);
        if (wColRow == nullptr) {
            return(false);
        }
        if (wColRow->VectorChildren()->empty()
            && wColRow->LastTreeDescendantIndex(wGrid, m_IsRow) <= wColRow->Index()) {
            return(false);
        }
        // Absolute target: first local Do toggles current; collab/Json re-apply is idempotent.
        if (!m_HasTargetOpen) {
            m_TargetOpen = !wColRow->Open();
            m_HasTargetOpen = true;
        }
        wColRow->Open(m_TargetOpen);
        return(true);
    };

    tBool tUndoOpenCloseTree::Undo()  {
        if (m_Sheet == nullptr) {
            return(false);
        }
        tColRowCellRange* wGrid = m_Sheet->ColRowCellRange();
        if (wGrid == nullptr) {
            return(false);
        }
        wGrid->SyncTreeParentRefsFromChildren(m_IsRow);
        tIndex wRoot = wGrid->GrandParent(m_Position, m_IsRow);
        if (m_IsRow) {
            wGrid->RenumRow(wRoot);
        } else {
            wGrid->RenumCol(wRoot);
        }
        tColRow* wColRow = m_IsRow ? m_Sheet->Row(m_Position) : m_Sheet->Col(m_Position);
        if (wColRow == nullptr) {
            return(false);
        }
        if (wColRow->VectorChildren()->empty()
            && wColRow->LastTreeDescendantIndex(wGrid, m_IsRow) <= wColRow->Index()) {
            return(false);
        }
        if (m_HasTargetOpen) {
            wColRow->Open(!m_TargetOpen);
        } else {
            wColRow->Open(!wColRow->Open());
        }
        return(true);
    };

    void tUndoOpenCloseTree::DeleteBeforeDo() {}

    tBool tUndoOpenCloseTree::Rebase() {
        // SetRebasePlan() requires Sheet(); without override it threw Sheet==nullptr.
        SetRebasePlan();
        if (!m_RebasePlan.IsEmpty()) {
            if (m_IsRow) {
                auto wRebasedPos = m_RebasePlan.RebaseRow(m_Position);
                if (!wRebasedPos.has_value()) {
                    return(false);
                }
                m_Position = wRebasedPos.value();
            } else {
                auto wRebasedPos = m_RebasePlan.RebaseCol(m_Position);
                if (!wRebasedPos.has_value()) {
                    return(false);
                }
                m_Position = wRebasedPos.value();
            }
        }
        return(true);
    }

    // Tree ===================================================================
    tUndoChangeTree::tUndoChangeTree() : tUndoSpreadSheet(tMode()), m_IsRow(false), m_Right(false), m_Position(0), m_Size(0), m_Sheet(nullptr), m_SaveSelectColRow(true) {
        m_SaveSelectColRow.UndoSpreadSheet(this);
    }

    tUndoChangeTree::tUndoChangeTree(tBool sIsRow,tBool sRight, tIndex sPosition,tIndex sSize,tMode sMode, tSheet* sSheet) :
        tUndoSpreadSheet(sMode),
        m_IsRow(sIsRow),
        m_Right(sRight),
        m_Position(sPosition),
        m_Size(sSize),
        m_Sheet(sSheet),
        m_SaveSelectColRow(sIsRow) {
            m_SaveSelectColRow.UndoSpreadSheet(this);
            if (IsUndoActif()) {
                tStringStream wStream;
                wStream << "Change tree ";
                if (m_IsRow) {
                    if (sSize==1) {
                        wStream << "row "  << sPosition;
                    } else {
                        wStream << "row "  << sPosition << ":" << sPosition+sSize-1;
                    }
                } else {
                    if (sSize==1) {
                        wStream << "column " << Base10ToAlpha(sPosition);
                    } else {
                        wStream << "column " << Base10ToAlpha(sPosition) << ":" << Base10ToAlpha(sPosition+sSize-1);
                    }
                }
                OperationName(wStream.str());
            }
    }

    void tUndoChangeTree::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        if (m_Sheet != nullptr) {
            sWriter->Key(kJsonKeySheet);
            sWriter->String(m_Sheet->Name().c_str());
        }
        sWriter->Key(kJsonKeyIsRow);
        sWriter->Bool(m_IsRow);
        sWriter->Key(kJsonKeyTreeRight);
        sWriter->Bool(m_Right);
        sWriter->Key(kJsonKeyPosition);
        sWriter->Int(m_Position);
        sWriter->Key(kJsonKeySize);
        sWriter->Int(m_Size);
        if (IsUndo() && m_Sheet != nullptr) {
            sWriter->Key(kJsonKeySave);
            sWriter->StartObject();
            m_SaveSelectColRow.ColRowCellRange(m_Sheet->ColRowCellRange());
            m_SaveSelectColRow.Json(sWriter, m_Sheet);
            sWriter->EndObject();
        }
    }

    void tUndoChangeTree::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        if (sValue.HasMember(kJsonKeySheet)) {
            tString wSheetName=sValue[kJsonKeySheet].GetString();
            m_Sheet=tSpreadSheetContainer::Instance()->ActiveWorkBook()->Sheet(wSheetName);
        }
        if (sValue.HasMember(kJsonKeyIsRow)) {
            m_IsRow = sValue[kJsonKeyIsRow].GetBool();
        }
        if (sValue.HasMember(kJsonKeyTreeRight)) {
            m_Right = sValue[kJsonKeyTreeRight].GetBool();
        }
        if (sValue.HasMember(kJsonKeyPosition)) {
            m_Position = sValue[kJsonKeyPosition].GetInt();
        }
        if (sValue.HasMember(kJsonKeySize)) {
            m_Size = sValue[kJsonKeySize].GetInt();
        }
        if (sValue.HasMember(kJsonKeySave) && m_Sheet != nullptr) {
            m_SaveSelectColRow.Clear();
            m_SaveSelectColRow.Json(sValue[kJsonKeySave], m_Sheet);
        }
    }

    tString tUndoChangeTree::ClassName() const { return("tUndoChangeTree"); }

    tSheet* tUndoChangeTree::Sheet() { return(m_Sheet); }

    tSaveSelect* tUndoChangeTree::SaveSelect() { return(nullptr); }

    tString tUndoChangeTree::RefRebase() { return(""); }
    
    tBool tUndoChangeTree::Do() {
        if (m_Sheet == nullptr) {
            return(false);
        }
        if (m_Right) {
            return(m_Sheet->DoTreeRight(&m_SaveSelectColRow, m_Position, m_Size, m_IsRow));
        } else {
            return(m_Sheet->DoTreeLeft(&m_SaveSelectColRow, m_Position, m_Size, m_IsRow));
        }
    };

    tBool tUndoChangeTree::Undo()  {
        if (m_Sheet == nullptr) {
            return(false);
        }
        return(m_Sheet->UndoTree(&m_SaveSelectColRow, m_Position, m_Size, m_IsRow));
    };

    tBool tUndoChangeTree::Rebase() {
        if (m_Sheet == nullptr) {
            return(false);
        }
        // Rebase the position if structural operations occurred
        tWorkBook* wWorkBook = m_Sheet->WorkBook();
        tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
        tSequenceId wSequenceId = SequenceId();
        if (wCurrentSequence > wSequenceId) {
            // Use kInvalidIndex to collect operations from all sheets for Intersheet calculations
            tRebasePlan wRebasePlan = wWorkBook->UndoRebaseLog().BuildPlan(
                wSequenceId,
                wCurrentSequence,
                static_cast<tAllocatorRef>(-1)  // Collect all sheets for Intersheet references
            );
            if (!wRebasePlan.m_Operations.empty()) {
                if (m_IsRow) {
                    auto wRebasedPos = wRebasePlan.RebaseRow(m_Position);
                    if (!wRebasedPos.has_value()) {
                        return(false);
                    }
                    m_Position = wRebasedPos.value();
                    // Rebase the saved selection
                    m_SaveSelectColRow.Rebase(wRebasePlan);
                } else {
                    auto wRebasedPos = wRebasePlan.RebaseCol(m_Position);
                    if (!wRebasedPos.has_value()) {
                        return(false);
                    }
                    m_Position = wRebasedPos.value();
                    // Rebase the saved selection
                    m_SaveSelectColRow.Rebase(wRebasePlan);
                }
            }
        }
        return(true);
    }

    void tUndoChangeTree::DeleteBeforeDo() {
        // Clear save select col row before Do
        m_SaveSelectColRow.Clear();
    }

    // Freeze pane / split view ===================================================
    void tUndoSplitView::RestoreSplitPan(tSheet* p, tIndex sColSplit, tIndex sRowSplit) {
        if (p == nullptr) {
            return;
        }
        p->SplitClear();
        if (sColSplit >= 0) {
            p->SplitV(sColSplit);
        }
        if (sRowSplit >= 0) {
            p->SplitH(sRowSplit);
        }
    }

    tUndoSplitView::tUndoSplitView()
        : tUndoSpreadSheet(tMode()),
          m_Cde(0),
          m_Position(0),
          m_Sheet(nullptr),
          m_PrevSplitV(static_cast<tIndex>(-1)),
          m_PrevSplitH(static_cast<tIndex>(-1)),
          m_PrevCaptured(false) {
    }

    tUndoSplitView::tUndoSplitView(tByte sCde, tIndex sPosition, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_Cde(sCde),
          m_Position(sPosition),
          m_Sheet(sSheet),
          m_PrevSplitV(static_cast<tIndex>(-1)),
          m_PrevSplitH(static_cast<tIndex>(-1)),
          m_PrevCaptured(false) {
        if (m_Sheet != nullptr) {
            m_SheetName = tSharedString(m_Sheet->Name());
        }
        if (IsUndoActif()) {
            tStringStream wStream;
            wStream << "Split view ";
            switch (sCde) {
            case 1:
                wStream << "freeze col " << Base10ToAlpha(sPosition);
                break;
            case 2:
                wStream << "freeze row " << static_cast<long long>(sPosition);
                break;
            case 3:
                wStream << "clear";
                break;
            default:
                wStream << "unknown";
                break;
            }
            OperationName(wStream.str());
        }
    }

    tString tUndoSplitView::ClassName() const {
        return("tUndoSplitView");
    }

    tSheet* tUndoSplitView::Sheet() {
        return(m_Sheet);
    }

    tSaveSelect* tUndoSplitView::SaveSelect() {
        return(nullptr);
    }

    tString tUndoSplitView::RefRebase() {
        return("");
    }

    tBool tUndoSplitView::Do() {
        if (m_Sheet == nullptr) {
            return(false);
        }
        m_PrevSplitV = m_Sheet->SplitV();
        m_PrevSplitH = m_Sheet->SplitH();
        m_PrevCaptured = true;
        switch (m_Cde) {
        case 1:
            m_Sheet->SplitV(m_Position);
            break;
        case 2:
            m_Sheet->SplitH(m_Position);
            break;
        case 3:
            m_Sheet->SplitClear();
            break;
        default:
            return(false);
        }
        return(true);
    }

    tBool tUndoSplitView::Undo() {
        if ((m_Sheet == nullptr) || (!m_PrevCaptured)) {
            return(false);
        }
        RestoreSplitPan(m_Sheet, m_PrevSplitV, m_PrevSplitH);
        return(true);
    }

    void tUndoSplitView::DeleteBeforeDo() {
        // Allow Redo's Do() to snapshot the restored sheet split state again.
        m_PrevCaptured = false;
    }

    tBool tUndoSplitView::Rebase() {
        SetRebasePlan();
        if (m_RebasePlan.IsEmpty()) {
            return(true);
        }

        auto rebaseCol = [&](tIndex& ioIndex) -> tBool {
            if (ioIndex < 0) {
                return(true);
            }
            const auto r = m_RebasePlan.RebaseCol(ioIndex);
            if (!r.has_value()) {
                return(false);
            }
            ioIndex = r.value();
            return(true);
        };
        auto rebaseRow = [&](tIndex& ioIndex) -> tBool {
            if (ioIndex < 0) {
                return(true);
            }
            const auto r = m_RebasePlan.RebaseRow(ioIndex);
            if (!r.has_value()) {
                return(false);
            }
            ioIndex = r.value();
            return(true);
        };

        if (m_Cde == 1) {
            if (!rebaseCol(m_Position)) {
                return(false);
            }
        } else if (m_Cde == 2) {
            if (!rebaseRow(m_Position)) {
                return(false);
            }
        }

        if (m_PrevCaptured) {
            if (!rebaseCol(m_PrevSplitV)) {
                return(false);
            }
            if (!rebaseRow(m_PrevSplitH)) {
                return(false);
            }
        }

        return(true);
    }

    void tUndoSplitView::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        UndoWriteSheetSh(sWriter, m_SheetName, m_Sheet);
        sWriter->Key(kJsonKeySplitViewCmd);
        sWriter->Uint(static_cast<unsigned>(m_Cde));
        sWriter->Key(kJsonKeyPosition);
        sWriter->Int(static_cast<int>(m_Position));
        sWriter->Key(kJsonKeySplitViewCaptured);
        sWriter->Bool(m_PrevCaptured ? true : false);
        // Persist anchors for remote rebase once Do has pinned the rollback state.
        if (m_PrevCaptured) {
            sWriter->Key(kJsonKeySplitViewPrevV);
            sWriter->Int(static_cast<int>(m_PrevSplitV));
            sWriter->Key(kJsonKeySplitViewPrevH);
            sWriter->Int(static_cast<int>(m_PrevSplitH));
        }
    }

    void tUndoSplitView::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        UndoWireSheetFromJsonSh(m_SheetName, m_Sheet, sValue, !IsJson());
        if (sValue.HasMember(kJsonKeySplitViewCmd)) {
            const auto& vc = sValue[kJsonKeySplitViewCmd];
            if (vc.IsUint()) {
                m_Cde = static_cast<tByte>(vc.GetUint());
            } else if (vc.IsInt() && (vc.GetInt() >= 0)) {
                m_Cde = static_cast<tByte>(vc.GetInt());
            } else {
                m_Cde = 0;
            }
        } else {
            m_Cde = 0;
        }
        if (sValue.HasMember(kJsonKeyPosition) && sValue[kJsonKeyPosition].IsInt()) {
            m_Position = static_cast<tIndex>(sValue[kJsonKeyPosition].GetInt());
        }
        if (sValue.HasMember(kJsonKeySplitViewCaptured) && sValue[kJsonKeySplitViewCaptured].IsBool()) {
            m_PrevCaptured = sValue[kJsonKeySplitViewCaptured].GetBool();
        } else {
            m_PrevCaptured = false;
        }
        if (sValue.HasMember(kJsonKeySplitViewPrevV) && sValue[kJsonKeySplitViewPrevV].IsInt()) {
            m_PrevSplitV = static_cast<tIndex>(sValue[kJsonKeySplitViewPrevV].GetInt());
        } else {
            m_PrevSplitV = static_cast<tIndex>(-1);
        }
        if (sValue.HasMember(kJsonKeySplitViewPrevH) && sValue[kJsonKeySplitViewPrevH].IsInt()) {
            m_PrevSplitH = static_cast<tIndex>(sValue[kJsonKeySplitViewPrevH].GetInt());
        } else {
            m_PrevSplitH = static_cast<tIndex>(-1);
        }
    }

    // Print parameters ===========================================================
    tUndoPrintParameters::tUndoPrintParameters()
        : tUndoSpreadSheet(tMode()),
          m_Sheet(nullptr),
          m_SnapshotCaptured(false) {
    }

    tUndoPrintParameters::tUndoPrintParameters(tString sJsonPrintParameters, tMode sMode, tSheet* sSheet)
        : tUndoSpreadSheet(sMode),
          m_Sheet(sSheet),
          m_SnapshotCaptured(false) {
        NormalizeSheet(&m_Sheet);
        if (m_Sheet != nullptr) {
            m_SheetName = tSharedString(m_Sheet->Name());
        }
        (void)m_PrintParameters.JsonParse(sJsonPrintParameters);
        if (IsUndoActif()) {
            OperationName("Print parameters");
        }
    }

    tString tUndoPrintParameters::ClassName() const {
        return("tUndoPrintParameters");
    }

    tSheet* tUndoPrintParameters::Sheet() {
        return(m_Sheet);
    }

    tSaveSelect* tUndoPrintParameters::SaveSelect() {
        return(nullptr);
    }

    tString tUndoPrintParameters::RefRebase() {
        return("");
    }

    tBool tUndoPrintParameters::Do() {
        NormalizeSheet(&m_Sheet);
        if (m_Sheet == nullptr) {
            return(false);
        }
        tPrintParameters wBefore;
        if (!wBefore.JsonParse(m_Sheet->JsonPrintParameters())) {
            wBefore = tPrintParameters();
        }
        m_SnapshotBefore = wBefore;
        m_SnapshotCaptured = true;
        m_Sheet->PrintParametersAssign(m_PrintParameters);
        return(true);
    }

    tBool tUndoPrintParameters::Undo() {
        NormalizeSheet(&m_Sheet);
        if ((m_Sheet == nullptr) || (!m_SnapshotCaptured)) {
            return(false);
        }
        m_Sheet->PrintParametersAssign(m_SnapshotBefore);
        return(true);
    }

    void tUndoPrintParameters::DeleteBeforeDo() {
        // Redo Do() must snapshot the sheet print layout again after Undo restored it.
        m_SnapshotCaptured = false;
    }

    tBool tUndoPrintParameters::Rebase() {
        return(true);
    }

    void tUndoPrintParameters::Json(Writer<StringBuffer>* sWriter) {
        tUndoSpreadSheet::Json(sWriter);
        UndoWriteSheetSh(sWriter, m_SheetName, m_Sheet);
        sWriter->Key(kJsonKeySheetPrintParameters);
        m_PrintParameters.JsonWrite(sWriter);
        sWriter->Key(kJsonKeySheetPrintParametersCaptured);
        sWriter->Bool(m_SnapshotCaptured ? true : false);
        if (m_SnapshotCaptured) {
            sWriter->Key(kJsonKeySheetPrintParametersPrev);
            m_SnapshotBefore.JsonWrite(sWriter);
        }
    }

    void tUndoPrintParameters::Json(const rapidjson::Value& sValue) {
        tUndoSpreadSheet::Json(sValue);
        UndoWireSheetFromJsonSh(m_SheetName, m_Sheet, sValue, !IsJson());
        if (sValue.HasMember(kJsonKeySheetPrintParameters) && sValue[kJsonKeySheetPrintParameters].IsObject()) {
            (void)m_PrintParameters.JsonRead(sValue[kJsonKeySheetPrintParameters]);
        }
        if (sValue.HasMember(kJsonKeySheetPrintParametersCaptured) && sValue[kJsonKeySheetPrintParametersCaptured].IsBool()) {
            m_SnapshotCaptured = sValue[kJsonKeySheetPrintParametersCaptured].GetBool();
        } else {
            m_SnapshotCaptured = false;
        }
        if (sValue.HasMember(kJsonKeySheetPrintParametersPrev) && sValue[kJsonKeySheetPrintParametersPrev].IsObject()) {
            (void)m_SnapshotBefore.JsonRead(sValue[kJsonKeySheetPrintParametersPrev]);
        }
    }

}; // End of namespace
