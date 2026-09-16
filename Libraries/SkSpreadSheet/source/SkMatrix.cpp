//=============================================================================
// SkSpreadSheet Matrix
//=============================================================================
#include "../include/SkMatrix.hpp"
#include "../include/SkRangeNamed.hpp"
#include "../include/SkColRow.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSheet.hpp"

#include <algorithm>

// Uncomment to trace #SPILL guard and OOXML array ref clipping (verbose on stderr).
#define _debugmatrix
#ifdef debugmatrix
#include <iostream>
#endif

namespace SkSpreadSheet {

    // Excel-style #SPILL guard: destination must be empty except for this spill's own cells (origin + MatExtend from sOrigin).
    tBool tMatrix::SpillDestinationIsClear(tCell* sOrigin, tColRowCellRange* sCr, const tTempoRect& sDestRect,
        const tTempoRect* sLogicalSpillRectBeforeClamp) {
        (void)sLogicalSpillRectBeforeClamp; // reserved for clamp-vs-logical-spill edge cases; spill+OOXML uses formula-null rule
        if (sOrigin == nullptr || sCr == nullptr) {
            return true;
        }
        tTempoRect wDest(sDestRect);
        const tIndex wTop = wDest.Top();
        const tIndex wLeft = wDest.Left();
        const tIndex wBottom = wDest.Bottom();
        const tIndex wRight = wDest.Right();
        if (wTop > wBottom || wLeft > wRight) {
            return true;
        }
#ifdef debugmatrix
        {
            tSheet* wSh = sOrigin->Sheet();
            const tString wSn = (wSh != nullptr) ? wSh->Name() : tString();
            std::cerr << "[SkMatrix::SpillDestinationIsClear] sheet=" << wSn
                      << " origin=" << sOrigin->StrRef(true) << " r=" << sOrigin->RowIndex() << " c=" << sOrigin->ColIndex()
                      << " dest=(" << wTop << "," << wLeft << ")-(" << wBottom << "," << wRight << ")"
                      << " IsSpillRange=" << (sOrigin->IsSpillRange() ? 1 : 0)
                      << " IsMatOrigin=" << (sOrigin->IsMatOrigin() ? 1 : 0) << "\n";
            if (sOrigin->IsSpillRange()) {
                tTempoRect wDbg = sOrigin->ArrayFormulaOutputRect();
                std::cerr << "  ArrayFormulaOutputRect: (" << wDbg.Top() << "," << wDbg.Left() << ")-("
                          << wDbg.Bottom() << "," << wDbg.Right() << ")"
                          << " nonEmpty=" << (wDbg.Top() <= wDbg.Bottom() && wDbg.Left() <= wDbg.Right() ? 1 : 0)
                          << "\n";
            }
        }
#endif
        for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
            for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                tCell* wCell = sCr->Cell(wRow, wCol);
                if (wCell == nullptr) {
                    continue;
                }
                // Skip spill origin: must never count as blocking. Compare by sheet coordinates, not only pointer.
                if (wRow == sOrigin->RowIndex() && wCol == sOrigin->ColIndex()) {
                    continue;
                }
                // Same spill as sOrigin: ArrayFormulaOutputRect / RangeSpill defines the ref; MatrixRange() may be null here.
                {
                    const tIndex wRr = wCell->RowIndex();
                    const tIndex wCc = wCell->ColIndex();
                    tBool wInsideOwnSpill = false;
                    tTempoRect wOut = sOrigin->ArrayFormulaOutputRect();
                    if (wOut.Top() <= wOut.Bottom() && wOut.Left() <= wOut.Right()) {
                        wInsideOwnSpill = (wRr >= wOut.Top() && wRr <= wOut.Bottom()
                            && wCc >= wOut.Left() && wCc <= wOut.Right());
                    } else if (sOrigin->IsMatOrigin()) {
                        tRange* wMr = sOrigin->MatrixRange();
                        if (wMr != nullptr) {
                            tTempoRect wMatBox(wMr->Rect());
                            wInsideOwnSpill = (wRr >= wMatBox.Top() && wRr <= wMatBox.Bottom()
                                && wCc >= wMatBox.Left() && wCc <= wMatBox.Right());
                        }
                    }
                    if (wInsideOwnSpill
                        && !(wRr == sOrigin->RowIndex() && wCc == sOrigin->ColIndex())) {
                        // Non-dynamic AFO (OOXML / .sker array formula ref): cached <v> slaves may lack
                        // MatExtend until RelinkJsonPersistedSpillSlaves — never treat them as blockers
                        // (Calendar six-block RecalculateAll / reload). Native MatDynamic uses the
                        // strict path below so user edits inside the spill yield #SPILL!.
                        if (!sOrigin->IsMatDynamic()) {
#ifdef debugmatrix
                            std::cerr << "  skip non-dynamic AFO interior " << wCell->StrRef(true) << "\n";
#endif
                            continue;
                        }
                        // Native dynamic arrays: only MatExtend of this origin or blanks may skip;
                        // user-typed values inside the spill must #SPILL! on recompute.
                        if (wCell->IsMatExtend()) {
                            tCell* wRoot = wCell->CellMatrixRoot();
                            if (wRoot != nullptr && wRoot->RowIndex() == sOrigin->RowIndex()
                                && wRoot->ColIndex() == sOrigin->ColIndex()) {
#ifdef debugmatrix
                                std::cerr << "  skip MatExtend same root (AFO) " << wCell->StrRef(true) << "\n";
#endif
                                continue;
                            }
                        } else if (wCell->Formula() == nullptr && wCell->Value().IsExcelNull()) {
#ifdef debugmatrix
                            std::cerr << "  skip blank inside AFO " << wCell->StrRef(true) << "\n";
#endif
                            continue;
                        }
                        // else: fall through — user content / foreign formula blocks (#SPILL!)
                    }
                }
                if (wCell->IsMatExtend()) {
                    tCell* wRoot = wCell->CellMatrixRoot();
                    if (wRoot != nullptr && wRoot->RowIndex() == sOrigin->RowIndex()
                        && wRoot->ColIndex() == sOrigin->ColIndex()) {
#ifdef debugmatrix
                        std::cerr << "  skip MatExtend same root " << wCell->StrRef(true) << "\n";
#endif
                        continue;
                    }
                    // Orphaned spill slave: MatExtend with no resolvable root. This happens when a matrix origin lost
                    // its range/AFO on load (legacy / partial .sker where the operator spill was saved with cached
                    // slave <v> but no "spillrange"): ClearMatrix cannot reach these cells, so they keep their cached
                    // value and would block the origin's re-spill (#SPILL! after reload). A MatExtend cell is always
                    // spill output, never user data, so an orphaned one must not block — the recompute overwrites it.
                    if (wRoot == nullptr) {
#ifdef debugmatrix
                        std::cerr << "  skip orphaned MatExtend (no root) " << wCell->StrRef(true) << "\n";
#endif
                        continue;
                    }
                }
                if (wCell->Formula() != nullptr) {
#ifdef debugmatrix
                    std::cerr << "  BLOCK spill: non-null formula at " << wCell->StrRef(true)
                              << " r=" << wRow << " c=" << wCol
                              << " formula=\"" << wCell->FormulaStr() << "\"\n";
#endif
                    return false;
                }
                // Do not use IsValueEmpty(): it requires m_Css==0. Formatted-but-blank cells must not block spill (Excel / OOXML).
                if (!wCell->Value().IsExcelNull()) {
#ifdef debugmatrix
                    std::cerr << "  BLOCK spill: non-null value at " << wCell->StrRef(true)
                              << " r=" << wRow << " c=" << wCol << "\n";
#endif
                    return false;
                }
            }
        }
#ifdef debugmatrix
        std::cerr << "[SkMatrix::SpillDestinationIsClear] OK dest=(" << wTop << "," << wLeft << ")-(" << wBottom << ","
                  << wRight << ")\n";
#endif
        return true;
    }

    // Clip result bounds to OOXML array/dynamic formula ref when stored on the evaluating cell (same rules as SkCell clamp).
    static void ClampResultRectToArrayFormulaOutput(tCell* sCell, tTempoRect& sIoRect) {
        if (sCell == nullptr) {
            return;
        }
        // Do not gate on IsSpillRange() on the cell — ClearMatrix / load order can clear it while RangeSpill() / AFO persists.
        tTempoRect wAR = sCell->ArrayFormulaOutputRect();
        if (wAR.Top() > wAR.Bottom() || wAR.Left() > wAR.Right()) {
            return;
        }
        if (sCell->RowIndex() != wAR.Top() || sCell->ColIndex() != wAR.Left()) {
#ifdef debugmatrix
            std::cerr << "[ClampResultRectToArrayFormulaOutput] skip " << sCell->StrRef(true)
                      << " not top-left of AFO cell=(" << sCell->RowIndex() << "," << sCell->ColIndex() << ") afoTL=("
                      << wAR.Top() << "," << wAR.Left() << ")\n";
#endif
            return;
        }
        // OOXML ref is authoritative — e.g. Calendar.xlsx uses one row per block
        // (ref B8:H8, B10:H10, …) while JoursEtSemaines+… evaluates to a larger matrix; clip to ref or spill
        // overwrites the next row’s area and repeats values (duplicate weeks).
        const tIndex wBeforeB = sIoRect.Bottom();
        const tIndex wBeforeR = sIoRect.Right();
        (void)wBeforeB;
        (void)wBeforeR;
        if (sIoRect.Bottom() > wAR.Bottom()) {
            sIoRect.Bottom(wAR.Bottom());
        }
        if (sIoRect.Right() > wAR.Right()) {
            sIoRect.Right(wAR.Right());
        }
#ifdef debugmatrix
        if (wBeforeB != sIoRect.Bottom() || wBeforeR != sIoRect.Right()) {
            std::cerr << "[ClampResultRectToArrayFormulaOutput] " << sCell->StrRef(true)
                      << " clipped logical (" << sIoRect.Top() << "," << sIoRect.Left() << ")-(" << wBeforeB << "," << wBeforeR
                      << ") -> AFO (" << sIoRect.Top() << "," << sIoRect.Left() << ")-(" << sIoRect.Bottom() << ","
                      << sIoRect.Right() << ") refAFO=(" << wAR.Top() << "," << wAR.Left() << ")-(" << wAR.Bottom() << ","
                      << wAR.Right() << ")\n";
        }
#endif
    }


    tMatrix::tMatrix(const tTempoRect& sRect, tCell* sCell)
        : tMatrix(sRect, sCell, nullptr, nullptr) {}

    tMatrix::tMatrix(const tTempoRect& sRect, tCell* sCell, tColRowCellRange* sReadColRow)
        : tMatrix(sRect, sCell, sReadColRow, nullptr) {}

    tMatrix::tMatrix(const tTempoRect& sRect, tCell* sCell, tColRowCellRange* sReadColRow, tFormulaNamed* sFormulaNamedOutput)
        : m_Cell(sCell),
          m_Rect(sRect),
          m_ColRowCellRange(sCell != nullptr ? sCell->ColRowCellRange() : nullptr),
          m_ReadColRowCellRange(sReadColRow != nullptr ? sReadColRow : (sCell != nullptr ? sCell->ColRowCellRange() : nullptr)),
          m_FormulaNamedOutput(sFormulaNamedOutput),
          m_Value() {}

    tMatrix::~tMatrix() {}

    void tMatrix::Clear() {}

    const tTempoRect& tMatrix::GetRect() const { return m_Rect; }

    void tMatrix::InitLiteralStorage(tIndex sH, tIndex sW) {
        m_LiteralStorage.clear();
        m_LiteralStorage.resize(static_cast<std::size_t>(sH));
        for (tIndex r = 0; r < sH; ++r) {
            m_LiteralStorage[static_cast<std::size_t>(r)].resize(static_cast<std::size_t>(sW));
        }
    }

    void tMatrix::SetLiteralValue(tIndex sRow, tIndex sCol, const tVariant& sValue) {
        if (sRow >= static_cast<tIndex>(m_LiteralStorage.size())) return;
        if (sCol >= static_cast<tIndex>(m_LiteralStorage[static_cast<std::size_t>(sRow)].size())) return;
        m_LiteralStorage[static_cast<std::size_t>(sRow)][static_cast<std::size_t>(sCol)] = sValue;
    }

    tVariant& tMatrix::MatrixValue(tIndex sRow, tIndex sCol) {
        if (!m_LiteralStorage.empty()) {
            if (sRow < static_cast<tIndex>(m_LiteralStorage.size()) &&
                sCol < static_cast<tIndex>(m_LiteralStorage[static_cast<std::size_t>(sRow)].size())) {
                m_Value = m_LiteralStorage[static_cast<std::size_t>(sRow)][static_cast<std::size_t>(sCol)];
                return m_Value;
            }
            m_Value.Clear();
            return m_Value;
        }
        if (m_ReadColRowCellRange == nullptr) {
            m_Value.Clear();
            return m_Value;
        }
        tIndex wRow = sRow + m_Rect.Top();
        tIndex wCol = sCol + m_Rect.Left();
        tSheet* wReadSheet = m_ReadColRowCellRange->Sheet();
        if (wReadSheet != nullptr && wReadSheet->Name() == CstSheetNamed) {
            tWorkBook* wWB = wReadSheet->WorkBook();
                if (wWB != nullptr) {
                tVariant wBuf;
                // Operand rectangle disambiguates which named formula spill to read when several exist on _$$.
                // Pass full size so definitions at column 1 match refs like A1:G6 (Left 0) as well as B1:H6.
                const tIndex wOpH = static_cast<tIndex>(m_Rect.Height());
                const tIndex wOpW = static_cast<tIndex>(m_Rect.Width());
                if (wWB->TryNamedFormulaSpillBufferAt(
                        wRow, wCol, wBuf, m_Rect.Top(), m_Rect.Left(), wOpH, wOpW)) {
                    m_Value = wBuf;
                    return m_Value;
                }
            }
        }
        tCell* wCell = m_ReadColRowCellRange->EnsureCell(wRow, wCol);
        // Prefer m_Cell when coordinates match (evaluating cell may differ from grid lookup).
        if (m_Cell != nullptr && m_Cell->RowIndex() == wRow && m_Cell->ColIndex() == wCol) {
            wCell = m_Cell;
        }
        if (wCell != nullptr) {
            return wCell->Value();
        }
        m_Value.Clear();
        return m_Value;
    }

    tVariant tMatrix::Copy(const tTempoRect& sResultRect) {
#ifdef debugmatrix
        cout << "tMatrix::Copy " << m_Rect.StrRef() << endl;
#endif
        const tIndex h = Height();
        const tIndex w = Width();
        if (h == 0 || w == 0 || m_ColRowCellRange == nullptr) {
            m_Value.Clear();
            return m_Value;
        }
        const tSize wCount = static_cast<tSize>(h) * static_cast<tSize>(w);
        tVariant* wSnap = new tVariant[wCount];
        for (tIndex rr = 0; rr < h; ++rr) {
            for (tIndex cc = 0; cc < w; ++cc) {
                wSnap[static_cast<tSize>(rr) * static_cast<tSize>(w) + static_cast<tSize>(cc)] = MatrixValue(rr, cc);
            }
        }

        tTempoRect wClampedRect(sResultRect);
        ClampResultRectToArrayFormulaOutput(m_Cell, wClampedRect);

        if (m_FormulaNamedOutput == nullptr &&
            !SpillDestinationIsClear(m_Cell, m_ColRowCellRange, wClampedRect, &sResultRect)) {
            delete[] wSnap;
            if (m_Cell != nullptr) {
                m_Cell->Value(tVariant(tClassError(tTypeError::t_spill, "")));
            }
            m_Value.SetError(tClassError(tTypeError::t_spill, ""));
            return m_Value;
        }

        if (m_Cell != nullptr && m_Cell->IsMatOrigin()) {
            tRange* wOldRange = m_Cell->MatrixRange();
            if (wOldRange != nullptr) {
                tTempoRect wOldRect = wOldRange->Rect();
                const tIndex wOldH = static_cast<tIndex>(wOldRect.Height());
                const tIndex wOldW = static_cast<tIndex>(wOldRect.Width());
                if (wOldH < h || wOldW < w) {
                    m_Cell->ClearMatrix();
                }
            }
        }

        tMatrix wResultMatrix(wClampedRect, m_Cell, nullptr, m_FormulaNamedOutput);
        const tIndex wLimitH = std::min(h, wResultMatrix.Height());
        const tIndex wLimitW = std::min(w, wResultMatrix.Width());
#ifdef debugmatrix
        {
            tTempoRect wLogResult(sResultRect);
            tTempoRect wLogClamped(wClampedRect);
            std::cerr << "[tMatrix::Copy] origin=" << (m_Cell != nullptr ? m_Cell->StrRef(true) : tString("null"))
                      << " operandHxW=" << h << "x" << w
                      << " sResult=(" << wLogResult.Top() << "," << wLogResult.Left() << ")-(" << wLogResult.Bottom()
                      << "," << wLogResult.Right() << ")"
                      << " clamped=(" << wLogClamped.Top() << "," << wLogClamped.Left() << ")-(" << wLogClamped.Bottom()
                      << "," << wLogClamped.Right() << ")"
                      << " wResultHxW=" << wResultMatrix.Height() << "x" << wResultMatrix.Width()
                      << " wLimitHxW=" << wLimitH << "x" << wLimitW << "\n";
        }
#endif
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->ResizeSpillBuffer(wLimitH, wLimitW);
        }
        tVariant wFirstResult;
        tBool wFirstSet = false;
        for (tIndex wRow = 0; wRow < wLimitH; ++wRow) {
            for (tIndex wCol = 0; wCol < wLimitW; ++wCol) {
                const tVariant wVal =
                    wSnap[static_cast<tSize>(wRow) * static_cast<tSize>(w) + static_cast<tSize>(wCol)];
                wResultMatrix.SetValue(wRow, wCol, wVal);
                if (!wFirstSet) {
                    wFirstResult = wVal;
                    wFirstSet = true;
                }
            }
        }
        delete[] wSnap;

        tTempoRect wRect(wClampedRect);
        // Named formulas whose definition resolves to a real-sheet range (OFFSET / INDEX / INDIRECT…) keep
        // m_ReadColRowCellRange on that real sheet (e.g. 'Entrées des données financières'), while the
        // host cell of the named formula lives on the synthetic CstSheetNamed sheet (`_$$`). A range
        // synthesized on `_$$` is fine for tMatrix::MatrixValue (it redirects to the FormulaNamed spill
        // buffer when reading), but tColRowCellRange visitors used by aggregate functions (COUNTA / SUM /
        // MATCH / …) iterate raw cells on `_$$` and see them empty, returning bogus #ARG / 0 results
        // (Excel "Budget" lstAnnées / lstMesures regression). When the source is a real sheet, register
        // it directly so dependents read the actual cells. Literal arrays {1,2;…} keep ReadColRow ==
        // ColRow on `_$$` and fall through to the synthetic range below.
        if (m_FormulaNamedOutput != nullptr
            && m_ReadColRowCellRange != nullptr
            && m_ReadColRowCellRange != m_ColRowCellRange) {
            tSheet* wReadSheet = m_ReadColRowCellRange->Sheet();
            if (wReadSheet != nullptr && wReadSheet->Name() != CstSheetNamed) {
#ifdef diagcalc
                std::cerr << "[diag] tMatrix::Copy cross-sheet spill: srcSheet=\""
                          << wReadSheet->Name() << "\" rect=("
                          << m_Rect.Top() << "," << m_Rect.Left() << ")-("
                          << m_Rect.Bottom() << "," << m_Rect.Right() << ")" << std::endl;
                std::cerr.flush();
#endif
                tRange* wSourceRange = m_ReadColRowCellRange->EnsureRange(
                    m_Rect.Top(), m_Rect.Left(), m_Rect.Bottom(), m_Rect.Right());
                if (wSourceRange != nullptr) {
                    // Do not call SetMatOrigin() on the source range: it isn't owned by this formula and
                    // marking it as a matrix origin would conflict with its real role on the source sheet.
                    m_FormulaNamedOutput->SetSpillRange(wSourceRange);
#ifdef diagcalc
                    std::cerr << "[diag] tMatrix::Copy registered SpillRange="
                              << wSourceRange->StrRef() << std::endl;
                    std::cerr.flush();
#endif
                    return (wFirstSet ? wFirstResult : m_Value);
                }
            }
        }
        tRange* wMatrixRange =
            m_ColRowCellRange->EnsureRange(wRect.Top(), wRect.Left(), wRect.Bottom(), wRect.Right());
        wMatrixRange->SetMatOrigin();
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->SetSpillRange(wMatrixRange);
        }
        return (wFirstSet ? wFirstResult : m_Value);
    }

    void tMatrix::SetValue(tIndex sRow, tIndex sCol, const tVariant& sValue) {
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->SetSpillBufferValue(sRow, sCol, sValue);
            if (sRow == 0 && sCol == 0 && m_Cell != nullptr) {
                m_Cell->Value(sValue);
                m_Cell->SetMatOrigin();
            }
            return;
        }
        if (m_ColRowCellRange == nullptr) return;
        tIndex wRow = sRow + m_Rect.Top();
        tIndex wCol = sCol + m_Rect.Left();
        tCell* wCell = m_ColRowCellRange->EnsureCell(wRow, wCol);
        const tBool wIsOrigin = (wRow == m_Rect.Top()) && (wCol == m_Rect.Left());
        // Same (row,col) can map to a different tCell* than the formula cell (m_Cell); always write the origin
        // matrix value onto m_Cell when the spill target is that cell so InternalCalculation sees the result.
        if (m_Cell != nullptr && m_Cell->RowIndex() == wRow && m_Cell->ColIndex() == wCol) {
            wCell = m_Cell;
        }
        if (wCell != nullptr) {
            // OOXML spill slaves may still carry an empty array formula (e.g. []) after import or .sker load.
            // Value() alone does not remove tFormula; dependents then see formula result instead of spilled value.
#ifdef debugmatrix
            const void* const wFpBefore = wCell->Formula();
            const tSize wFsLenBefore = wCell->FormulaStr().size();
#endif
            if (!wIsOrigin && wCell->Formula() != nullptr) {
                wCell->ClearFormula();
            }
            wCell->Value(sValue);
            // Set Flags
            if (wIsOrigin) {
                wCell->SetMatOrigin();
            } else {
                wCell->SetMatExtend();
            }
#ifdef debugmatrix
            std::cerr << "[tMatrix::SetValue] " << wCell->StrRef(true) << " local=(" << sRow << "," << sCol << ") abs=("
                      << wRow << "," << wCol << ")"
                      << " origin=" << (wIsOrigin ? 1 : 0) << " fpBefore=" << wFpBefore << " fpAfter=" << wCell->Formula()
                      << " fStrLen before/after=" << wFsLenBefore << "/" << wCell->FormulaStr().size()
                      << " valIsExcelNull=" << (wCell->Value().IsExcelNull() ? 1 : 0) << " MatOr=" << (wCell->IsMatOrigin() ? 1 : 0)
                      << " MatExt=" << (wCell->IsMatExtend() ? 1 : 0) << "\n";
#endif
        }
    }

    tIndex tMatrix::Height() {
        return static_cast<tIndex>(m_Rect.Height());
    }

    tIndex tMatrix::Width() {
        return static_cast<tIndex>(m_Rect.Width());
    }

    tBool tMatrix::BroadcastResultSize(tIndex h1, tIndex w1, tIndex h2, tIndex w2, tIndex& outH, tIndex& outW) {
        if (h1 == 0 || w1 == 0 || h2 == 0 || w2 == 0) {
            return false;
        }
        if (h1 == h2) {
            outH = h1;
        } else if (h1 == 1) {
            outH = h2;
        } else if (h2 == 1) {
            outH = h1;
        } else {
            return false;
        }
        if (w1 == w2) {
            outW = w1;
        } else if (w1 == 1) {
            outW = w2;
        } else if (w2 == 1) {
            outW = w1;
        } else {
            return false;
        }
        return true;
    }

    tVariant tMatrix::ApplyBinaryOperation(tVariant& sVariant, const tTempoRect& sResultRect, tBinaryOperation sOperation, tBool sScalarOnLeft) {
#ifdef debugmatrix
        cout << "tMatrix::ApplyBinaryOperation Variant " << m_Rect.StrRef() <<    "/" << sVariant << " " << sOperation << endl;
#endif
        tTempoRect wClampedRect(sResultRect);
        ClampResultRectToArrayFormulaOutput(m_Cell, wClampedRect);
        if (m_FormulaNamedOutput == nullptr &&
            !SpillDestinationIsClear(m_Cell, m_ColRowCellRange, wClampedRect, &sResultRect)) {
            if (m_Cell != nullptr) {
                m_Cell->Value(tVariant(tClassError(tTypeError::t_spill, "")));
            }
            m_Value.SetError(tClassError(tTypeError::t_spill, ""));
            return m_Value;
        }
        tMatrix wResultMatrix(wClampedRect, m_Cell, nullptr, m_FormulaNamedOutput);
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->ResizeSpillBuffer(wResultMatrix.Height(), wResultMatrix.Width());
        }
        tVariant wFirstResult;
        tBool wFirstSet = false;
        // Loop on Matrix
        for (tIndex wRow = 0; wRow < Height() && wRow < wResultMatrix.Height(); wRow++) {
            for (tIndex wCol = 0; wCol < Width() && wCol < wResultMatrix.Width(); wCol++) {
                tVariant wSource = MatrixValue(wRow, wCol);
                tVariant wResult;
                tVariant& wLeft = sScalarOnLeft ? sVariant : wSource;
                tVariant& wRight = sScalarOnLeft ? wSource : sVariant;

                switch (sOperation) {
                    case tOp_Add:    wResult = wLeft + wRight; break;
                    case tOp_Subtract: wResult = wLeft - wRight; break;
                    case tOp_Multiply: wResult = wLeft * wRight; break;
                    case tOp_Divide:  wResult = wLeft / wRight; break;
                    default:
                        m_Value.Clear();
                        return m_Value;
                }

                wResultMatrix.SetValue(wRow, wCol, wResult);
                if (!wFirstSet) {
                    wFirstResult = wResult;
                    wFirstSet = true;
                }
            }
        }
        // Ensure Matrix Range (non-const copy to call Top/Left/Bottom/Right; tTempoRect getters are non-const)
        tTempoRect wRect(wClampedRect);
        tRange* wMatrixRange = m_ColRowCellRange->EnsureRange(wRect.Top(), wRect.Left(), wRect.Bottom(), wRect.Right());
        wMatrixRange->SetMatOrigin();
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->SetSpillRange(wMatrixRange);
        }
        return (wFirstSet ? wFirstResult : m_Value);
    }

    tVariant tMatrix::ApplyBinaryOperation(tMatrix* sMatrix, const tTempoRect& sResultRect, tBinaryOperation sOperation) {
#ifdef debugmatrix
        cout << "tMatrix::ApplyBinaryOperation Matrix " << m_Rect.StrRef() <<   "/" << sOperation << endl;
#endif
        if (sMatrix == nullptr) {
            m_Value.Clear();
            return m_Value;
        }
        const tIndex h1 = Height();
        const tIndex w1 = Width();
        const tIndex h2 = sMatrix->Height();
        const tIndex w2 = sMatrix->Width();
        tIndex wOutH = 0;
        tIndex wOutW = 0;
        if (!BroadcastResultSize(h1, w1, h2, w2, wOutH, wOutW)) {
            m_Value.Clear();
            m_Value.SetError(tClassError(tTypeError::t_value, ""));
            return m_Value;
        }
        // Snapshot operand values before writing the result: result may overlap an operand range.
        const tSize wCount1 = static_cast<tSize>(h1) * static_cast<tSize>(w1);
        const tSize wCount2 = static_cast<tSize>(h2) * static_cast<tSize>(w2);
        tVariant* wSnap1 = new tVariant[wCount1];
        tVariant* wSnap2 = new tVariant[wCount2];
        for (tIndex rr = 0; rr < h1; ++rr) {
            for (tIndex cc = 0; cc < w1; ++cc) {
                wSnap1[static_cast<tSize>(rr) * static_cast<tSize>(w1) + static_cast<tSize>(cc)] = MatrixValue(rr, cc);
            }
        }
        for (tIndex rr = 0; rr < h2; ++rr) {
            for (tIndex cc = 0; cc < w2; ++cc) {
                wSnap2[static_cast<tSize>(rr) * static_cast<tSize>(w2) + static_cast<tSize>(cc)] = sMatrix->MatrixValue(rr, cc);
            }
        }

        tTempoRect wClampedRect(sResultRect);
        ClampResultRectToArrayFormulaOutput(m_Cell, wClampedRect);

        if (m_FormulaNamedOutput == nullptr &&
            !SpillDestinationIsClear(m_Cell, m_ColRowCellRange, wClampedRect, &sResultRect)) {
            delete[] wSnap2;
            delete[] wSnap1;
            if (m_Cell != nullptr) {
                m_Cell->Value(tVariant(tClassError(tTypeError::t_spill, "")));
            }
            m_Value.SetError(tClassError(tTypeError::t_spill, ""));
            return m_Value;
        }

        if (m_Cell != nullptr && m_Cell->IsMatOrigin()) {
            tRange* wOldRange = m_Cell->MatrixRange();
            if (wOldRange != nullptr) {
                tTempoRect wOldRect = wOldRange->Rect();
                const tIndex wOldH = static_cast<tIndex>(wOldRect.Height());
                const tIndex wOldW = static_cast<tIndex>(wOldRect.Width());
                if (wOldH < wOutH || wOldW < wOutW) {
                    m_Cell->ClearMatrix();
                }
            }
        }

        tMatrix wResultMatrix(wClampedRect, m_Cell, nullptr, m_FormulaNamedOutput);
        const tIndex wLimitH = std::min(wOutH, wResultMatrix.Height());
        const tIndex wLimitW = std::min(wOutW, wResultMatrix.Width());
#ifdef debugmatrix
        {
            tTempoRect wLogResult(sResultRect);
            tTempoRect wLogClamped(wClampedRect);
            std::cerr << "[tMatrix::ApplyBinaryOperation Matrix] origin="
                      << (m_Cell != nullptr ? m_Cell->StrRef(true) : tString("null"))
                      << " h1xw1=" << h1 << "x" << w1 << " h2xw2=" << h2 << "x" << w2 << " outHxW=" << wOutH << "x" << wOutW
                      << " sResult=(" << wLogResult.Top() << "," << wLogResult.Left() << ")-(" << wLogResult.Bottom()
                      << "," << wLogResult.Right() << ")"
                      << " clamped=(" << wLogClamped.Top() << "," << wLogClamped.Left() << ")-(" << wLogClamped.Bottom()
                      << "," << wLogClamped.Right() << ")"
                      << " wResultHxW=" << wResultMatrix.Height() << "x" << wResultMatrix.Width()
                      << " wLimitHxW=" << wLimitH << "x" << wLimitW << " op=" << static_cast<int>(sOperation) << "\n";
        }
#endif
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->ResizeSpillBuffer(wLimitH, wLimitW);
        }
        tVariant wFirstResult;
        tBool wFirstSet = false;

        for (tIndex wRow = 0; wRow < wLimitH; ++wRow) {
            for (tIndex wCol = 0; wCol < wLimitW; ++wCol) {
                const tIndex i1 = (h1 == 1) ? 0 : wRow;
                const tIndex j1 = (w1 == 1) ? 0 : wCol;
                const tIndex i2 = (h2 == 1) ? 0 : wRow;
                const tIndex j2 = (w2 == 1) ? 0 : wCol;
                tVariant wVal1 = wSnap1[static_cast<tSize>(i1) * static_cast<tSize>(w1) + static_cast<tSize>(j1)];
                tVariant wVal2 = wSnap2[static_cast<tSize>(i2) * static_cast<tSize>(w2) + static_cast<tSize>(j2)];
                tVariant wResult;
                switch (sOperation) {
                    case tOp_Add:    wResult = wVal1 + wVal2; break;
                    case tOp_Subtract: wResult = wVal1 - wVal2; break;
                    case tOp_Multiply: wResult = wVal1 * wVal2; break;
                    case tOp_Divide:  wResult = wVal1 / wVal2; break;
                    default:
                        delete[] wSnap2;
                        delete[] wSnap1;
                        m_Value.Clear();
                        m_Value.SetError(tClassError(tTypeError::t_value, ""));
                        return m_Value;
                }
                wResultMatrix.SetValue(wRow, wCol, wResult);
                if (!wFirstSet) {
                    wFirstResult = wResult;
                    wFirstSet = true;
                }
            }
        }
        delete[] wSnap2;
        delete[] wSnap1;
        tTempoRect wRect(wClampedRect);
        tRange* wMatrixRange = m_ColRowCellRange->EnsureRange(wRect.Top(), wRect.Left(), wRect.Bottom(), wRect.Right());
        wMatrixRange->SetMatOrigin();
        if (m_FormulaNamedOutput != nullptr) {
            m_FormulaNamedOutput->SetSpillRange(wMatrixRange);
        }
        return (wFirstSet ? wFirstResult : m_Value);
    }

    tVariant tMatrix::Plus(tVariant& sVariant, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sVariant, sResultRect, tOp_Add);
    }

    tVariant tMatrix::Minus(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft) {
        return ApplyBinaryOperation(sVariant, sResultRect, tOp_Subtract, sScalarOnLeft);
    }

    tVariant tMatrix::Multiply(tVariant& sVariant, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sVariant, sResultRect, tOp_Multiply, false);
    }

    tVariant tMatrix::Divide(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft) {
        return ApplyBinaryOperation(sVariant, sResultRect, tOp_Divide, sScalarOnLeft);
    }

    tVariant tMatrix::Plus(tMatrix* sMatrix, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sMatrix, sResultRect, tOp_Add);
    }

    tVariant tMatrix::Minus(tMatrix* sMatrix, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sMatrix, sResultRect, tOp_Subtract);
    }

    tVariant tMatrix::Multiply(tMatrix* sMatrix, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sMatrix, sResultRect, tOp_Multiply);
    }

    tVariant tMatrix::Divide(tMatrix* sMatrix, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sMatrix, sResultRect, tOp_Divide);
    }

    tVariant tMatrix::Ampersand(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft) {
        return ApplyBinaryOperation(sVariant, sResultRect, tOp_Add, sScalarOnLeft);
    }

    tVariant tMatrix::Ampersand(tMatrix* sMatrix, const tTempoRect& sResultRect) {
        return ApplyBinaryOperation(sMatrix, sResultRect, tOp_Add);
    }

    tVariant tMatrix::UnaryMinus(const tTempoRect& sResultRect) {
        tVariant wMinusOne;
        wMinusOne.SetInt(-1);
        return Multiply(wMinusOne, sResultRect);
    }

    tMatrix& tMatrix::operator +=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator -=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator *=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator /=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator %=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator ^=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator &=(tMatrix* sMatrix) { return *this; }
    tMatrix& tMatrix::operator |=(tMatrix* sMatrix) { return *this; }
}
