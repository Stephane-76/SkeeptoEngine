//=============================================================================
// SkSpreadSheet Html generate table HTML
//=============================================================================
#include "../include/SkJsonView.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkConditionalFormat.hpp"
#include <algorithm>

namespace SkSpreadSheet {

const tInt   Cst_SearchDeep = 40; // Nb Cell to find recovery



    tJsonView::tJsonView() : tVirtualClass(),m_ColRowCellRange(nullptr),m_ConditionalFormatContainer(nullptr),m_KeyIndex(0) {}

    void tJsonView::ColRowCellRange(tColRowCellRange* sColRowCellRange) {
        m_ColRowCellRange=sColRowCellRange;
        m_ConditionalFormatContainer=m_ColRowCellRange->ConditionalFormatContainer();
    }

    tIndex tJsonView::FirstCol(tIndex sCol) {
        tColRow* wColRow=m_ColRowCellRange->Col(sCol);
        if (wColRow!=nullptr) {
            if (wColRow->IsInClosedPath(m_ColRowCellRange, false)) {
                wColRow=wColRow->FirstParentTreeNode(m_ColRowCellRange, false);
                return(wColRow->Index());
            }
        }
        return(sCol);
    }

    tIndex tJsonView::NextCol(tIndex sCol) {
        tIndex wCol=sCol;
        tColRow* wColRow=m_ColRowCellRange->Col(sCol);
        if (wColRow!=nullptr) {
            return(wColRow->SearchNextOpen(m_ColRowCellRange, false));
        }
        if (wCol<Cst_MaxCol)  wCol++;
        return(wCol);
    }

    tIndex tJsonView::FirstRow(tIndex sRow) {
        tColRow* wColRow=m_ColRowCellRange->Row(sRow);
        if (wColRow!=nullptr) {
            if (wColRow->IsInClosedPath(m_ColRowCellRange, true)) {
                wColRow=wColRow->FirstParentTreeNode(m_ColRowCellRange, true);
                sRow=wColRow->Index();
                wColRow=m_ColRowCellRange->Row(sRow);
            }
            if (wColRow != nullptr && !wColRow->DataVisible()) {
                sRow=wColRow->SearchNextOpen(m_ColRowCellRange, true);
            }
        }
        return(sRow);
    }

    tIndex tJsonView::NextRow(tIndex sRow) {
        tIndex wRow=sRow;
        tColRow* wColRow=m_ColRowCellRange->Row(sRow);
        if (wColRow!=nullptr) {
            return(wColRow->SearchNextOpen(m_ColRowCellRange, true));
        }
        if (wRow<Cst_MaxRow)  wRow++;
        return(wRow);
    }


    tCell* tJsonView::Cell(tIndex sRow, tIndex sCol) {
        return(m_ColRowCellRange->Cell(sRow,sCol));
    }

    tColRow* tJsonView::Col(tIndex sIndex) {
        return(m_ColRowCellRange->Col(sIndex));
    }

    tColRow* tJsonView::Row(tIndex sIndex) {
        return(m_ColRowCellRange->Row(sIndex));
    }

    tDouble tJsonView::SizeRow(tIndex sRow,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->SizeRow(sRow, sUnit));
    }


    tDouble tJsonView::SizeCol(tIndex sRow,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->SizeCol(sRow, sUnit));
    }

    tRange* tJsonView::MergedRange(tIndex sRow, tIndex sCol) {
        return(m_ColRowCellRange->MergedRange(sRow, sCol));
    }


    // For optimization (get param col for cell)
    class tColRowPos : public tClass {
        public:
        tIndex     m_Index;
        tDouble    m_Pos;
        tDouble    m_Lenght;
        tFormatRef m_Format;
        
        tColRowPos(tIndex sIndex,tDouble sPos, tDouble sLength,tFormatRef sFormatRef) : tClass(),
            m_Index(sIndex),
            m_Pos(sPos),
            m_Lenght(sLength),
            m_Format(sFormatRef) {}
    };

    typedef std::vector<tColRowPos> tVectorColRowPos;
    typedef std::map<tString,tRange*> tMapOfRecoveredRange;

    /** Merge sheet rect + viewport pixel box for grid paint (avoids ReturnRangeMerged from JS). */
    struct tJsonViewMergeGridRect {
        tIndex m_Top;
        tIndex m_Left;
        tIndex m_Bottom;
        tIndex m_Right;
        tDouble m_PxLeft;
        tDouble m_PxTop;
        tDouble m_PxWidth;
        tDouble m_PxHeight;
    };

    /** Multi-cell spill / dynamic-array output rect in sheet cell coordinates (for canvas overlay). */
    struct tJsonViewSpillSheetRect {
        tIndex m_Top;
        tIndex m_Left;
        tIndex m_Bottom;
        tIndex m_Right;
    };

    tBool AddRecovedRange(tMapOfRecoveredRange* wMap,tRange* sRange) {
        tMapOfRecoveredRange::iterator wIterator;
        wIterator=wMap->find(sRange->StrRef());
        if (wIterator==wMap->end()) {
            (*wMap)[sRange->StrRef()]=sRange;
            return(true);
        }
        return(false);
    }

    /// Collect a multi-cell spill once per View (origin, extend, or OOXML spill flag).
    static void TryCollectSpillRange(tCell* sCell,
                                     tMapOfRecoveredRange* sMap,
                                     std::vector<tJsonViewSpillSheetRect>* sOut) {
        if (sCell == nullptr || sMap == nullptr || sOut == nullptr) {
            return;
        }
        tCell* wOrigin = nullptr;
        tRange* wSpill = nullptr;
        if (sCell->IsMatOrigin()) {
            wOrigin = sCell;
            wSpill = sCell->MatrixRange();
        } else if (sCell->IsMatExtend()) {
            wOrigin = sCell->CellMatrixRoot();
            if (wOrigin != nullptr) {
                wSpill = wOrigin->MatrixRange();
            }
        } else if (sCell->IsSpillRange()) {
            // Anchor-only: SpillRange() requires Top/Left == this cell.
            wOrigin = sCell;
            wSpill = sCell->SpillRange();
        }
        if (wSpill == nullptr || wSpill->IsCell()) {
            return;
        }
        // Excel: no spill outline while the origin shows #SPILL!.
        if (wOrigin != nullptr && wOrigin->Value().IsError()
            && wOrigin->Value().Error().Code() == tTypeError::t_spill) {
            return;
        }
        if (!AddRecovedRange(sMap, wSpill)) {
            return;
        }
        sOut->push_back(tJsonViewSpillSheetRect{
            wSpill->TopIndex(),
            wSpill->LeftIndex(),
            wSpill->BottomIndex(),
            wSpill->RightIndex(),
        });
    }

    tBool IsRecovedRange(tMapOfRecoveredRange* wMap,tIndex sRow,tIndex sCol, tBool& sFirst) {
        for(auto wRangeIterator : *wMap) {
            tRange* wRange=wRangeIterator.second;
            if (wRange->CoveredCell(sRow,sCol)) {
                sFirst=((wRange->TopIndex()==sRow) &&
                        (wRange->LeftIndex()==sCol));
                return(true);
            }
        }
        return(false);
    }

    // Build the ordered format-ref vector (sheet -> col -> row -> cell).
    static tBool _BuildCellFormatVector(tSheet* sSheet, tIndex sRow, tIndex sCol,
                                        tVectorFormatRef& sVector) {
        sVector.clear();
        tFormatRef wFormatSheet = sSheet->Css();
        if (wFormatSheet != 0) sVector.push_back(wFormatSheet);
        tColRow* wCol = sSheet->Col(sCol);
        if (wCol != nullptr) {
            tFormatRef wFormatCol = wCol->Css();
            if (wFormatCol != 0) sVector.push_back(wFormatCol);
        }
        tColRow* wRow = sSheet->Row(sRow);
        if (wRow != nullptr) {
            tFormatRef wFormatRow = wRow->Css();
            if (wFormatRow != 0) sVector.push_back(wFormatRow);
        }
        tCell* wCell = sSheet->Cell(sRow, sCol);
        if (wCell != nullptr) {
            tFormatRef wFormatCell = wCell->Css();
            if (wFormatCell != 0) sVector.push_back(wFormatCell);
        }
        return(!sVector.empty());
    }

    static void EmitMergedEdgeBorder(tSheet* sSheet, tFormatApi* sFormatApi,
                                     tIndex sRow, tIndex sCol, tShort sBorderSide,
                                     const tChar* sKey, tBool sCss,
                                     Writer<StringBuffer>* sWriter) {
        tVectorFormatRef wVector;
        if (!_BuildCellFormatVector(sSheet, sRow, sCol, wVector)) {
            return;
        }
        sFormatApi->JsonJavaScriptBorderSide(&wVector, sBorderSide, sKey, sCss, sWriter);
    }

    static tBool MergedEdgeHasBorder(tSheet* sSheet, tIndex sRow, tIndex sCol, tShort sBorderSide) {
        tVectorFormatRef wVector;
        if (!_BuildCellFormatVector(sSheet, sRow, sCol, wVector)) {
            return(false);
        }
        tFormatApi* wFormatApi = sSheet->WorkBook()->FormatApi();
        if (wFormatApi == nullptr) {
            return(false);
        }
        return((wFormatApi->CellBorder(&wVector) & sBorderSide) != 0);
    }

    void tJsonView::EmitMergedBorders(tRange* sRange, tBool sCss, Writer<StringBuffer>* sWriter) {
        if (sRange == nullptr) return;
        tSheet* wSheet = m_ColRowCellRange->Sheet();
        tFormatApi* wFormatApi = wSheet->WorkBook()->FormatApi();
        if (wFormatApi == nullptr) return;

        // Owner-model: outer right/bottom live on edge cells of the merge, not on neighbors.
        const tIndex wTop = sRange->TopIndex();
        const tIndex wBottom = sRange->BottomIndex();
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wRight = sRange->RightIndex();

        if (MergedEdgeHasBorder(wSheet, wTop, wRight, tBorderRight)) {
            EmitMergedEdgeBorder(
                wSheet, wFormatApi, wTop, wRight, tBorderRight, "f_bor", sCss, sWriter);
        } else if (wBottom > wTop &&
                   MergedEdgeHasBorder(wSheet, wBottom, wRight, tBorderRight)) {
            EmitMergedEdgeBorder(
                wSheet, wFormatApi, wBottom, wRight, tBorderRight, "f_bor", sCss, sWriter);
        }

        if (MergedEdgeHasBorder(wSheet, wBottom, wLeft, tBorderBottom)) {
            EmitMergedEdgeBorder(
                wSheet, wFormatApi, wBottom, wLeft, tBorderBottom, "f_bob", sCss, sWriter);
        } else if (wRight > wLeft &&
                   MergedEdgeHasBorder(wSheet, wBottom, wRight, tBorderBottom)) {
            EmitMergedEdgeBorder(
                wSheet, wFormatApi, wBottom, wRight, tBorderBottom, "f_bob", sCss, sWriter);
        }
    }

    tShort tJsonView::CellBorder(tCell* sCell) {
        tSheet* wSheet=m_ColRowCellRange->Sheet();
        tFormatRef wFormatSheet=wSheet->Css();
        tFormatRef wFormatCol=0;
        tFormatRef wFormatRow=0;
        tFormatRef wFormatCell=sCell->Css();
       
        tColRow* wCol=wSheet->Col(sCell->ColIndex());
        if (wCol!=nullptr) wFormatCol=wCol->Css();

        tColRow* wRow=wSheet->Row(sCell->Row()->Index());
        if (wRow!=nullptr) wFormatRow=wRow->Css();
        
        tFormatApi* wFormatApi=wSheet->WorkBook()->FormatApi();
        if (wFormatApi==nullptr) return(0);
        // Optimize non format on sheet , col & row
        if ((wFormatSheet==0) && (wFormatCol==0) && (wFormatRow==0)) {
            return(wFormatApi->CellBorder(wFormatCell));
        } else {
            tVectorFormatRef wVector;
            if (wFormatSheet!=0) wVector.push_back(wFormatSheet);
            if (wFormatRow!=0) wVector.push_back(wFormatRow);
            if (wFormatCol!=0) wVector.push_back(wFormatCol);
            if (wFormatCell!=0) wVector.push_back(wFormatCell);
            return(wFormatApi->CellBorder(&wVector));
        }
    }

    tBool tJsonView::IsEmptyLeft(tCell* sCell) {
        // Excel: overflow is blocked by a neighboring *value* (or fill), not by a border.
        // A thin left/right edge on an empty indent column must not clip labels
        // (e.g. ACTIF!C19 "Terrains" spilling across D:F).
        return(sCell->Value().Type()==tVariantType::t_null);
    }
    
    tBool tJsonView::IsEmptyRight(tCell* sCell) {
        return(sCell->Value().Type()==tVariantType::t_null);
    }
        
    tDouble tJsonView::GetLeftClip(tIndex sRow, tIndex sCol,tUnitMetrics sUnit,tDouble sPos) {
        tDouble wResult=sPos;
        tIndex wCol=sCol;
        wCol--;
        tIndex wSearchDelta=Cst_SearchDeep;
        if (wCol>0) {
            tCell* wCell=Cell(sRow,wCol);
            while  ((wCol>0) && ((wSearchDelta--)>0)) {
                tColRow* wColRow=m_ColRowCellRange->Col(wCol);
                if (wColRow!=nullptr) {
                    if (wColRow->IsInClosedPath(m_ColRowCellRange,false)) return(wResult);
                }
                if (wCell!=nullptr) {
                    if (IsEmptyLeft(wCell)) {
                        const tShort wMask = CellBorder(wCell);
                        // Never extend clip_l onto a left-border cell (table frame on B).
                        if ((wMask & tBorderLeft) == tBorderLeft) {
                            return(wResult);
                        }
                        wResult-=SizeCol(wCol, sUnit);
                        if ((wMask & tBorderRight) == tBorderRight) {
                            return(wResult);
                        }
                    } else {
                        return(wResult);
                    }
                } else {
                    wResult-=SizeCol(wCol, sUnit);
                }
                wCol--;
                wCell=Cell(sRow,wCol);
            }
        }
        return(wResult);
    }

    tDouble tJsonView::GetRightClip(tIndex sRow, tIndex sCol,tUnitMetrics sUnit,tDouble sPos) {
        tDouble wResult=sPos;
        tIndex wCol=sCol;
        wCol++;
        tIndex wSearchDelta=Cst_SearchDeep;
        tBool wFirst = true;
        if (wCol<Cst_MaxCol) {
            tCell* wCell=Cell(sRow,wCol);
            while ((wCol<Cst_MaxCol) && ((wSearchDelta--)>0)) {
                tColRow* wColRow=m_ColRowCellRange->Col(wCol);
                if (wColRow!=nullptr) {
                    if (wColRow->IsInClosedPath(m_ColRowCellRange,false)) return(wResult);
                }
                if (wCell!=nullptr) {
                    if (IsEmptyRight(wCell)) {
                        const tShort wMask = CellBorder(wCell);
                        // Immediate neighbor may carry the source's projected right edge
                        // (C19 "Terrains" → D19). Later solid left walls (G number columns)
                        // must stop the clip or category rows lose G–J verticals.
                        if ((wMask & tBorderLeft) == tBorderLeft && !wFirst) {
                            return(wResult);
                        }
                        wResult+=SizeCol(wCol, sUnit);
                        wFirst = false;
                        if ((wMask & tBorderRight) == tBorderRight) {
                            return(wResult);
                        }
                    } else {
                        return(wResult); // End
                    }
                } else {
                    wResult+=SizeCol(wCol, sUnit);
                    wFirst = false;
                }
                wCol++;
                wCell=Cell(sRow,wCol);
            }
        }
        return(wResult);
    }

    tCellConditionalFormat* tJsonView::CellConditionalFormat(tCell* sCell) {
        if (m_ConditionalFormatContainer!=nullptr) {
            return(m_ConditionalFormatContainer->CellIConditionalFormat(sCell));
        }
        return(nullptr);
    }


    void tJsonView::View(Writer<StringBuffer>* sWriter, tIndex sRow, tIndex sCol, tUnitMetrics sUnit, tDouble sViewHeight, tDouble sViewWidth,tDouble sDiffY, tDouble sDiffX,tBool sCss) {
        std::lock_guard<std::mutex> wLock(m_Mutex);
    #ifdef  debugwasm
        tStringStream wStream;
        wStream << "tJsonView::JsonView(" << SkRoot::SkMetrics::UnitShortName(sUnit) << ":" << sRow << "," << sCol << "," << sViewHeight << "," << sViewWidth << ")";
        cout << wStream.str() << endl;
        
        assert(m_ColRowCellRange!=nullptr);
    #endif
        
     
        tSheet* wSheet=m_ColRowCellRange->Sheet();
        if (wSheet==nullptr) return;
        tWorkBook* wWorkBook=wSheet->WorkBook();
        wWorkBook->BeginJsonViewFormatTable();

    #ifdef  debugwasm
        wStream.clear();
        wStream << " URI:" <<  wWorkBook->Uri();
        wStream << " Sheet:" << wSheet->Name();
        cout << wStream.str() << endl;
    #endif
        tVectorColRowPos wVectorCol;

        // Precision Pixel
        sWriter->SetMaxDecimalPlaces(2);

        // Cell band positions accumulate from 0; sDiffX/sDiffY are added to every c_x/c_y at emit.
        // c_w/c_h use row/column sizes only (no diff trim on dimensions).

        // Sheet ==============================================================
        sWriter->Key("sheet");
        sWriter->String(wSheet->Name().c_str());

        // TopRow TopCol + sheet pixel offsets for scrollbar thumb (avoids extra SumPixel* WASM calls from JS).
        const tIndex wTopRow = FirstRow(sRow);
        const tIndex wTopCol = FirstCol(sCol);
        sWriter->Key("toprow"); sWriter->Int(wTopRow);
        sWriter->Key("topcol"); sWriter->Int(wTopCol);
        tDouble wScrollTopPx = 0;
        tDouble wScrollLeftPx = 0;
        if (wTopRow > 1) {
            wScrollTopPx = m_ColRowCellRange->SumHeight(1, wTopRow - 1, sUnit);
        }
        if (wTopCol > 1) {
            wScrollLeftPx = m_ColRowCellRange->SumWidth(1, wTopCol - 1, sUnit);
        }
        sWriter->Key("scrollTopPx");
        sWriter->Double(wScrollTopPx);
        sWriter->Key("scrollLeftPx");
        sWriter->Double(wScrollLeftPx);
      
        // 1 Col ==============================================================
        sWriter->Key("cols");
        sWriter->StartArray();
        tIndex wCol = FirstCol(sCol);
        tIndex wLastGoodCol = wCol;
        tDouble wPos=0;
        tBool wOvertake=false;

        do {
            tColRow* wColRow = Col(wCol);
            sWriter->StartObject();
            sWriter->Key("i"); sWriter->Int(wCol);
            tFormatRef wFormatRef=0;
            tDouble wSize =SizeCol(wCol,sUnit);
            if (wColRow != nullptr) {
                wFormatRef = wColRow->Css();
            }
            const tDouble wBandX = wPos + sDiffX;
            // Include partially visible columns (emitted to cols[] below).
            if (wBandX < sViewWidth) {
                wLastGoodCol = wCol;
            }
            if (wBandX + wSize > sViewWidth) wOvertake=true;
            sWriter->Key("s"); sWriter->Double(wSize);
            // Tree
            if (wColRow != nullptr) {
                if (!wColRow->Open()) { sWriter->Key("o"); sWriter->Bool(false); }
                if (wColRow->NbChildren()!=0) { sWriter->Key("c"); sWriter->Int(tInt(wColRow->NbChildren())); }
                tInt wDeep=wColRow->Deep(m_ColRowCellRange, false);
                if (wDeep!=0) { sWriter->Key("d"); sWriter->Int(wDeep); }
            }
            sWriter->EndObject();
            wVectorCol.push_back(tColRowPos(wCol,wPos,wSize,wFormatRef));
            wPos += wSize;
            const tIndex wColBeforeAdvance = wCol;
            wCol=NextCol(wCol);
            if (wCol <= wColBeforeAdvance) break;
        } while ((wPos + sDiffX < sViewWidth) && (!wOvertake));

        sWriter->EndArray();

        // Conditional Format — skip entire pass when the sheet has no CF rules.
        if (m_ConditionalFormatContainer!=nullptr && !m_ConditionalFormatContainer->IsEmpty()) {
            // Make intersect rect list
            // Search Bottom Row
            tIndex wRow=sRow;
            tDouble wHeight=0;
            while (wHeight<sViewHeight) {
                wHeight+=SizeRow(wRow, sUnit);
                if (wHeight>sViewHeight) break;
                wRow++;
            }
            tRect wRect;
            wRect.Top(wTopRow);
            wRect.Left(wTopCol);
            wRect.Bottom(wRow);
            wRect.Right(wLastGoodCol);
            // Make intersect rect list
            m_ConditionalFormatContainer->MakeIntersectRectList(&wRect);
            
            // Load Conditional Format
            m_ConditionalFormatContainer->ApplyJsonView();
        }
        // 2 Rows  && cells ==================================================
        tMapOfRecoveredRange wMapRecovered;
        tMapOfRecoveredRange wMapSpillRanges;
        std::vector<tJsonViewMergeGridRect> wMergeRectsForGrid;
        std::vector<tJsonViewSpillSheetRect> wSpillRectsForGrid;
        tFormatRef wSheetFormat=wSheet->Css();

        sWriter->Key("rows");
        sWriter->StartArray();
        tIndex wRow = FirstRow(sRow);
        tIndex wLastGoodRow = wRow;
        wPos=0;
        do {
            tColRow* wColRow = Row(wRow);
            sWriter->StartObject();
            sWriter->Key("i"); sWriter->Int(wRow);
            tFormatRef wRowFormat=0;
            tDouble wSize=SizeRow(wRow, sUnit);
            if (wColRow != nullptr) {
                wRowFormat = wColRow->Css();
            }
            // Include partially visible rows (emitted to rows[] below).
            if (wPos + sDiffY < sViewHeight) wLastGoodRow = wRow;

            sWriter->Key("s"); sWriter->Double(wSize);
            // Tree
            if (wColRow != nullptr) {
                if (!wColRow->Open()) { sWriter->Key("o"); sWriter->Bool(false); }
                if (wColRow->NbChildren()!=0) { sWriter->Key("c"); sWriter->Int(tInt(wColRow->NbChildren())); }
                tInt wDeep=wColRow->Deep(m_ColRowCellRange, true);
                if (wDeep!=0) { sWriter->Key("d"); sWriter->Int(wDeep); }
            }
            sWriter->Key("cells");
            sWriter->StartArray();
         
            
            // Loop on col ====================================================
            for(auto wColIterator : wVectorCol ) {
    #ifdef  debugwasm
                wStream.str("");
                wStream << "JsonView Cell : " << Base10ToAlpha(wColIterator.m_Index) << wRow;
                cout << wStream.str() << endl;
    #endif
                tDouble wColPos=wColIterator.m_Pos;
                tDouble wWidth=wColIterator.m_Lenght;
                tFormatRef wColFormat=wColIterator.m_Format;
                wCol=wColIterator.m_Index;

                const tBool wIsFirstVisibleCol = (wCol == FirstCol(sCol));

                // To Draw rect of cell (Precision)
                tDouble wRecoverDelta=0;
                
                // 1 is Merged
                tBool wIsMerged=false;
                tBool wIsVisible=true;
                tRange* wRange=MergedRange(wRow,wCol);
                if (wRange!=nullptr) {
                    if (AddRecovedRange(&wMapRecovered,wRange)) {
                        wIsMerged=true;
                    } else {
                        tBool wIsFirst=false;
                        wIsVisible=!IsRecovedRange(&wMapRecovered,wRow,wCol,wIsFirst);
                    }
                }
                // KeyIndex for unique key in React (Class on)
                tStringStream wStreamKey;
                wStreamKey << m_KeyIndex++;
                if (m_KeyIndex>10000000) m_KeyIndex=0;
               
                if (wIsMerged) {
    #ifdef  debugwasm
                    wStream.str("");
                    wStream << "JsonView Range Merged : " << wRange->StrRef() << wRow;
                    cout << wStream.str() << endl;
    #endif
                    // Calculate Pos and Size (physical SumWidth/Height — independent of sCol / tree).
                    const tIndex wMergeEmitCol = wCol;
                    const tIndex wFirstVisCol = FirstCol(sCol);
                    const tIndex wFirstVisRow = FirstRow(sRow);

                    tDouble wPosY = wPos;
                    if (wRange->TopIndex() < sRow) {
                        const tIndex wHideRowEnd = std::min(sRow - 1, wRange->BottomIndex());
                        if (wHideRowEnd >= wRange->TopIndex()) {
                            wPosY -= m_ColRowCellRange->SumHeight(
                                wRange->TopIndex(), wHideRowEnd, sUnit);
                        }
                    }
                    tDouble wRangeHeight = m_ColRowCellRange->SumHeight(
                        wRange->TopIndex(), wRange->BottomIndex(), sUnit);

                    tDouble wPosX = wColPos;
                    if (wRange->LeftIndex() < wMergeEmitCol) {
                        wPosX -= m_ColRowCellRange->SumWidth(
                            wRange->LeftIndex(), wMergeEmitCol - 1, sUnit);
                    }
                    tDouble wRangeWidth = m_ColRowCellRange->SumWidth(
                        wRange->LeftIndex(), wRange->RightIndex(), sUnit);

                    tDouble wCellTop = wPosY + sDiffY;
                    tDouble wCellH = wRangeHeight;
                    tDouble wCellLeft = wPosX + sDiffX;
                    tDouble wCellW = wRangeWidth;

                    // Clamp merged ink to this JsonView viewport (split panes); sizes stay SumWidth/Height.
                    tDouble wEmitLeft = wCellLeft;
                    tDouble wEmitTop = wCellTop;
                    tDouble wEmitW = wCellW;
                    tDouble wEmitH = wCellH;
                    if (wEmitW > 0 && wEmitLeft < sViewWidth) {
                        const tDouble wMergeRight = wEmitLeft + wEmitW;
                        if (wMergeRight > sViewWidth) {
                            wEmitW = sViewWidth - wEmitLeft;
                            if (wEmitW < 0) {
                                wEmitW = 0;
                            }
                        }
                    }
                    if (wEmitH > 0 && wEmitTop < sViewHeight) {
                        const tDouble wMergeBottom = wEmitTop + wEmitH;
                        if (wMergeBottom > sViewHeight) {
                            wEmitH = sViewHeight - wEmitTop;
                            if (wEmitH < 0) {
                                wEmitH = 0;
                            }
                        }
                    }

                    tIndex wRowRange=wRange->TopIndex();
                    tIndex wColRange=wRange->LeftIndex();
                    tCell* wCell = Cell(wRowRange, wColRange);
                    TryCollectSpillRange(wCell, &wMapSpillRanges, &wSpillRectsForGrid);

                    wMergeRectsForGrid.push_back(tJsonViewMergeGridRect{
                        wRange->TopIndex(),
                        wRange->LeftIndex(),
                        wRange->BottomIndex(),
                        wRange->RightIndex(),
                        wEmitLeft,
                        wEmitTop,
                        wEmitW,
                        wEmitH,
                    });

                    // A merged anchor must ALWAYS be emitted, even when empty of value
                    // and format. The frontend relies on c_w / c_h for paint, cursor, and
                    // hit-test in this view.
                    // Start Cell =================================================
                    sWriter->StartObject();
                    sWriter->Key("c_k"); sWriter->String(wStreamKey.str().c_str());
                    sWriter->Key("c_r"); sWriter->Int64(wRow);
                    sWriter->Key("c_c"); sWriter->Int64(wCol);
                    sWriter->Key("c_x"); sWriter->Double(wCellLeft);
                    sWriter->Key("c_y"); sWriter->Double(wCellTop);
                    sWriter->Key("c_w"); sWriter->Double(wCellW);
                    sWriter->Key("c_h"); sWriter->Double(wCellH);

                    // Cross-pane merge text: c_ox / c_oy encode hidden span when viewport clamp shrinks visible slice.
                    const tDouble wLayoutOx = wEmitW - wCellW;
                    if (wLayoutOx != 0) {
                        sWriter->Key("c_ox"); sWriter->Double(wLayoutOx);
                    }
                    const tDouble wLayoutOy = wEmitH - wCellH;
                    if (wLayoutOy != 0) {
                        sWriter->Key("c_oy"); sWriter->Double(wLayoutOy);
                    }
                    sWriter->Key("c_mg"); sWriter->Bool(true);
                    const tBool wMergeInkClampedLeft = (wRange->LeftIndex() < wFirstVisCol);
                    sWriter->Key("c_ml"); sWriter->Bool(wMergeInkClampedLeft);
                    if (wMergeInkClampedLeft) {
                        sWriter->Key("c_ax"); sWriter->Double(wCellLeft);
                    }
                    const tBool wMergeInkClampedTop = (wRange->TopIndex() < wFirstVisRow);
                    sWriter->Key("c_mt"); sWriter->Bool(wMergeInkClampedTop);
                    if (wMergeInkClampedTop) {
                        sWriter->Key("c_ay"); sWriter->Double(wCellTop);
                    }
                    if (wCell != nullptr) {
                        wCell->Value().JsonJavaScript(sWriter);
                        // Formula strings (c_f) omitted in Wasm JsonView: payload size / parse cost; JS uses GetFormula when editing.
    #ifndef __EMSCRIPTEN__
                        if (wCell->Formula() != nullptr) {
                            sWriter->String("c_f"); sWriter->String(wCell->FormulaStr().c_str());
                        }
    #endif
                    }

                    // Write Format and FormatString Cell after
                    wWorkBook->JsonFormatJavaScript(wSheet,wRowRange, wColRange,sCss,CellConditionalFormat(wCell),sWriter);
                    // Outer right/bottom for merged anchor (see EmitMergedBorders).
                    EmitMergedBorders(wRange, sCss, sWriter);
                    sWriter->EndObject();
                } else {
                    if (wIsVisible) {
                        tCell* wCell = Cell(wRow, wCol);
                        TryCollectSpillRange(wCell, &wMapSpillRanges, &wSpillRectsForGrid);
                        const tBool wFirstColEmpty =
                            (wCell == nullptr) ||
                            (wCell != nullptr &&
                             wCell->Value().Type() == tVariantType::t_null);
                        // Left spill: first visible column empty, recover text from cells left of viewport.
                        tBool wLeftSpillEmitted = false;
                        if (wIsFirstVisibleCol && wFirstColEmpty) {
    #ifdef debugwasm
                                cout << "Test Left " << Base10ToAlpha(wCol) << wRow << endl;
    #endif
                                tIndex wSearchCol = wCol;
                                wSearchCol--;
                                tByte wSearchDelta = Cst_SearchDeep;
                                while ((wSearchCol > 0) && ((wSearchDelta--)>0)) {
                                    tCell* wSearchCell = Cell(wRow, wSearchCol);
                                    if (wSearchCell != nullptr) {
                                        tShort wMaskBorder = CellBorder(wSearchCell);
                                        tBool wBorderRight =
                                            (wMaskBorder & tBorderRight) == tBorderRight;
                                        if ((wSearchCell->Value().Type() != tVariantType::t_null) &&
                                            (!wBorderRight)) {
    #ifdef debugwasm
                                            cout << "Find Left " << Base10ToAlpha(wSearchCol) << wRow << endl;
    #endif
                                            const tDouble wSpillW = SizeCol(wSearchCol, sUnit);
                                            // Pane X of source anchor: sheet offset minus viewport origin.
                                            tDouble wAnchorSheetX = 0;
                                            if (wSearchCol > 1) {
                                                wAnchorSheetX = m_ColRowCellRange->SumWidth(
                                                    1, wSearchCol - 1, sUnit);
                                            }
                                            tDouble wViewOriginSheetX = wScrollLeftPx;
                                            tDouble wSpillX = wAnchorSheetX - wViewOriginSheetX + sDiffX;
                                            sWriter->StartObject();
                                            sWriter->Key("c__l"); sWriter->Bool(true);
                                            sWriter->Key("c_k"); sWriter->String(wStreamKey.str().c_str());
                                            sWriter->Key("c_r"); sWriter->Int64(wRow);
                                            sWriter->Key("c_c"); sWriter->Int64(wSearchCol);
                                            sWriter->Key("c_x"); sWriter->Double(wSpillX - wRecoverDelta);
                                            sWriter->Key("c_y"); sWriter->Double(wPos + sDiffY - wRecoverDelta);
                                            sWriter->Key("c_w"); sWriter->Double(wSpillW + wRecoverDelta);
                                            sWriter->Key("c_h"); sWriter->Double(wSize + wRecoverDelta);
                                            wSearchCell->Value().JsonJavaScript(sWriter);
    #ifndef __EMSCRIPTEN__
                                            if (wSearchCell->Formula() != nullptr) {
                                                sWriter->String("c_f");
                                                sWriter->String(wSearchCell->FormulaStr().c_str());
                                            }
    #endif
                                            wWorkBook->JsonFormatJavaScript(
                                                wSheet, wRow, wSearchCol, sCss,
                                                CellConditionalFormat(wSearchCell), sWriter);
                                            if (wSearchCell->Cover()) {
                                                tDouble wRightClip = wSpillX + wSpillW;
                                                tDouble wWork = GetRightClip(
                                                    wRow, wSearchCol, sUnit, wRightClip);
                                                if (wRightClip != wWork) {
                                                    sWriter->Key("clip_r");
                                                    sWriter->Double(wWork + wRecoverDelta);
                                                }
                                                if (wSpillX < 0) {
                                                    sWriter->Key("clip_l");
                                                    sWriter->Double(wRecoverDelta);
                                                }
                                            }
                                            sWriter->EndObject();
                                            wLeftSpillEmitted = true;
                                            break;
                                        }
                                    }
                                    wSearchCol--;
                                }
                        }

                        // Empty cells inside a styled table must still be emitted so
                        // JsonFormatJavaScript can apply table-style overlays (banded rows, etc.).
                        tBool wInTableData = false;
                        if (wCell == nullptr && wSheetFormat == 0 && wRowFormat == 0 &&
                            wColFormat == 0) {
                            tString wTableName;
                            tRange* wTableRange = nullptr;
                            std::tie(wTableName, wTableRange) =
                                wSheet->FindRangeDataCovered(wRow, wCol);
                            wInTableData =
                                (wTableRange != nullptr && wTableRange->IsData());
                        }

                        // Search cell Overflow (right spill on sentinel past last good col)
                        if (wCell==nullptr) {
                            // Search Cell in right position ==================
                            if (wCol==wLastGoodCol+1) {
    #ifdef debugwasm
                                cout << "Test Right " << Base10ToAlpha(wCol) << wRow << endl;
    #endif
                                tIndex wSearchCol=wCol;
                                tDouble wSearchWidth=0;
                                tDouble wSearchColPos=wColPos;
                                tIndex wSearchDelta=Cst_SearchDeep;
                                while((wSearchCol<=Cst_MaxCol) && ((wSearchDelta--)>0)) {
                                    wSearchColPos+=wSearchWidth;
                                    wSearchWidth=SizeCol(wSearchCol,sUnit);
                                    tCell* wSearchCell=Cell(wRow,wSearchCol);
                                    if (wSearchCell!=nullptr) {
                                        tShort wMaskBorder=CellBorder(wSearchCell);
                                        tBool wBorderLeft=(wMaskBorder & tBorderLeft) ==tBorderLeft;
                                        if ((wSearchCell->Value().Type()!=tVariantType::t_null) &&
                                            (!wBorderLeft)) {
    #ifdef debugwasm
                                            cout << "Find Right " << Base10ToAlpha(wSearchCol) << wRow << endl;
    #endif
                                            // Start Cell =========================
                                            sWriter->StartObject();
                                            sWriter->Key("c__r"); sWriter->Bool(true);
                                            sWriter->Key("c_k"); sWriter->String(wStreamKey.str().c_str());
                                            sWriter->Key("c_r"); sWriter->Int64(wRow);
                                            sWriter->Key("c_c"); sWriter->Int64(wSearchCol);
                                            sWriter->Key("c_x"); sWriter->Double(wSearchColPos + sDiffX - wRecoverDelta);
                                            sWriter->Key("c_y"); sWriter->Double(wPos + sDiffY - wRecoverDelta);
                                            sWriter->Key("c_w"); sWriter->Double(wSearchWidth + wRecoverDelta);
                                            sWriter->Key("c_h"); sWriter->Double(wSize + wRecoverDelta);
                                            wSearchCell->Value().JsonJavaScript(sWriter);
    #ifndef __EMSCRIPTEN__
                                            if (wSearchCell->Formula() != nullptr) {
                                                sWriter->String("c_f"); sWriter->String(wSearchCell->FormulaStr().c_str());
                                            }
    #endif
                                            // Write Format and FormatString Cell after (CF from source cell; outer wCell is null here).
                                            wWorkBook->JsonFormatJavaScript(wSheet,wRow,wSearchCol,sCss,CellConditionalFormat(wSearchCell),sWriter);
                                            if (wSearchCell->Cover()) {
                                                tDouble wLeftClip=wSearchColPos;
                                                tDouble wWork=GetLeftClip(wRow,wSearchCol,sUnit,wLeftClip);
                                                if (wLeftClip!=wWork) {
                                                    sWriter->Key("clip_l"); sWriter->Double(wWork+wRecoverDelta);
                                                }
                                            }
                                            sWriter->EndObject();
                                            break;
                                        }
                                    }
                                    wSearchCol++;
                                }
                            }
                        }
            
                        const tBool wNeedViewportCell =
                            (wCell != nullptr) || (wSheetFormat != 0) ||
                            (wRowFormat != 0) || (wColFormat != 0) || wInTableData;
                        // c__l is overflow text ink only — still emit this column's styled viewport cell.
                        if (wNeedViewportCell) {
                            if (wLeftSpillEmitted) {
                                wStreamKey.str("");
                                wStreamKey << m_KeyIndex++;
                                if (m_KeyIndex > 10000000) {
                                    m_KeyIndex = 0;
                                }
                            }
                            // Start Cell =================================================
                            sWriter->StartObject();
                            sWriter->Key("c_k"); sWriter->String(wStreamKey.str().c_str());
                            sWriter->Key("c_r"); sWriter->Int64(wRow);
                            sWriter->Key("c_c"); sWriter->Int64(wCol);
                            sWriter->Key("c_x"); sWriter->Double(wColPos + sDiffX - wRecoverDelta);
                            sWriter->Key("c_y"); sWriter->Double(wPos + sDiffY - wRecoverDelta);
                            sWriter->Key("c_w"); sWriter->Double(wWidth + wRecoverDelta);
                            sWriter->Key("c_h"); sWriter->Double(wSize + wRecoverDelta);
                            if (wCell != nullptr) {
                                wCell->Value().JsonJavaScript(sWriter);
    #ifndef __EMSCRIPTEN__
                                if (wCell->Formula() != nullptr) {
                                    sWriter->String("c_f"); sWriter->String(wCell->FormulaStr().c_str());
                                }
    #endif
                            }
                            // Write Format and FormatString Cell after
                            wWorkBook->JsonFormatJavaScript(wSheet,wRow,wCol,sCss,CellConditionalFormat(wCell),sWriter);
                            // Get Left && Right Clip
                            if (wCell!=nullptr) {
                                if (wCell->Value().Type()!=tVariantType::t_null) {
                                    // Does the cell cover the others ?
                                    if (wCell->Cover()) {
                                        tShort wMaskBorder=CellBorder(wCell);
                                        tBool wBorderLeft= (wMaskBorder & tBorderLeft) == tBorderLeft;
                                        tBool wBorderRight= (wMaskBorder & tBorderRight) == tBorderRight;
                                        
                                        if (!wBorderLeft) {
                                            tDouble wLeftClip=wColPos + sDiffX;
                                            tDouble wWork=GetLeftClip(wRow,wCol,sUnit,wLeftClip);
                                            if (wLeftClip!=wWork) {
                                                tDouble wEmitClipL = wWork + wRecoverDelta;
                                                if (wEmitClipL < 0) {
                                                    wEmitClipL = 0;
                                                }
                                                sWriter->Key("clip_l"); sWriter->Double(wEmitClipL);
                                            }
                                        }
                                        // Excel General: numbers/dates/bools are right-aligned and
                                        // spill left, not right. clip_r on I would include J (empty
                                        // or adjacency left wall) and the painter would drop J's
                                        // border-left — the I|J grid of this bilan.
                                        if (!wBorderRight && wCell->Value().IsString()) {
                                            tDouble wRightClip=wColPos + sDiffX + wWidth;
                                            tDouble wWork=GetRightClip(wRow,wCol,sUnit,wRightClip);
                                            if (wRightClip!=wWork) {
                                                sWriter->Key("clip_r"); sWriter->Double(wWork+wRecoverDelta);
                                            }
                                        }
                                    }
                                }
                            }
                            
                            sWriter->EndObject();
                        }
                    }
                   
                }
               
            } // end for col
            
            sWriter->EndArray(); // of cell
                        
            sWriter->EndObject(); // of Row


            // Advance by row height (diff is on c_y, not on row band stepping).
            const tIndex wRowBeforeAdvance = wRow;
            wPos += wSize;
            wRow=NextRow(wRow);
            // No progress (closed tree / end of sheet) — avoid duplicating the same row forever.
            if (wRow <= wRowBeforeAdvance) break;
        } while (wPos + sDiffY < sViewHeight);
        
        
        sWriter->EndArray(); //Rows

        if (!wMergeRectsForGrid.empty()) {
            sWriter->Key("merges");
            sWriter->StartArray();
            for (const tJsonViewMergeGridRect& wMergeEntry : wMergeRectsForGrid) {
                sWriter->StartObject();
                sWriter->Key("r_t"); sWriter->Int64(wMergeEntry.m_Top);
                sWriter->Key("r_l"); sWriter->Int64(wMergeEntry.m_Left);
                sWriter->Key("r_b"); sWriter->Int64(wMergeEntry.m_Bottom);
                sWriter->Key("r_r"); sWriter->Int64(wMergeEntry.m_Right);
                sWriter->Key("p_l"); sWriter->Double(wMergeEntry.m_PxLeft);
                sWriter->Key("p_t"); sWriter->Double(wMergeEntry.m_PxTop);
                sWriter->Key("p_w"); sWriter->Double(wMergeEntry.m_PxWidth);
                sWriter->Key("p_h"); sWriter->Double(wMergeEntry.m_PxHeight);
                sWriter->EndObject();
            }
            sWriter->EndArray();
        }

        // Dynamic-array / matrix spill ranges intersecting this viewport (sheet cell coords).
        if (!wSpillRectsForGrid.empty()) {
            sWriter->Key("spills");
            sWriter->StartArray();
            for (const tJsonViewSpillSheetRect& wSpillEntry : wSpillRectsForGrid) {
                sWriter->StartObject();
                sWriter->Key("r_t"); sWriter->Int64(wSpillEntry.m_Top);
                sWriter->Key("r_l"); sWriter->Int64(wSpillEntry.m_Left);
                sWriter->Key("r_b"); sWriter->Int64(wSpillEntry.m_Bottom);
                sWriter->Key("r_r"); sWriter->Int64(wSpillEntry.m_Right);
                sWriter->EndObject();
            }
            sWriter->EndArray();
        }

        // Viewport origin offsets (same values folded into every c_x / c_y).
        sWriter->Key("dY");
        sWriter->Double(sDiffY);

        sWriter->Key("dX");
        sWriter->Double(sDiffX);

        sWriter->Key("lastrow"); sWriter->Int(wLastGoodRow);
        sWriter->Key("lastcol"); sWriter->Int(wLastGoodCol);

        wWorkBook->WriteJsonViewFormatTable(sWriter);

        // Free Conditional Format
        if (m_ConditionalFormatContainer!=nullptr && !m_ConditionalFormatContainer->IsEmpty()) {
            m_ConditionalFormatContainer->ApplyFree();
        }
    #ifdef  debugwasm
        wStream.str("");
        wStream << "JsonView Last Row : " << wLastGoodRow << endl;
        wStream << "JsonView Last Col : " << wLastGoodCol;

        cout << wStream.str() << endl;
    #endif
    };

    void tJsonView::ReturnRightJustify(Writer<StringBuffer>* sWriter, tIndex sCol, tUnitMetrics sUnit,tDouble sViewWidth) {
       tIndex wCol=sCol;
       tDouble wPos=sViewWidth;
       while ((wCol>0) && (wPos>0)) {
           tDouble wSize = SizeCol(wCol,sUnit);
           wPos -= wSize;
           wCol--;
       };
       sWriter->StartObject();
       sWriter->Key("col");
       sWriter->Int(wCol+1);
       sWriter->Key("diff");
       sWriter->Double(wPos);
       sWriter->EndObject();
    }

    void tJsonView::ReturnBottomJustify(Writer<StringBuffer>* sWriter, tIndex sRow, tUnitMetrics sUnit,tDouble sViewHeight) {
    tIndex wRow=sRow;
        tDouble wPos=sViewHeight;
    while ((wRow>0) && (wPos>0)) {
        tDouble wSize = SizeRow(wRow,sUnit);
        wPos -= wSize;
        wRow--;
    };
    sWriter->StartObject();
    sWriter->Key("row");
    sWriter->Int(wRow+1);
    sWriter->Key("diff");
    sWriter->Double(wPos);
    sWriter->EndObject();
    }
    
} // end of namepsace

