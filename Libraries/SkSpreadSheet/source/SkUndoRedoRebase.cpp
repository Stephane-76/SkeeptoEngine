#include "../include/SkUndoRedoRebase.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkWorkBook.hpp"

#include <algorithm>
#include <unordered_set>
namespace SkSpreadSheet {

namespace {

    constexpr tIndex kInvalidIndex = static_cast<tIndex>(-1);

    [[nodiscard]] tIndex RectTop(const tRect& sRect) {
        tRect wRect(sRect);
        return wRect.Top();
    }

    [[nodiscard]] tIndex RectBottom(const tRect& sRect) {
        tRect wRect(sRect);
        return wRect.Bottom();
    }

    [[nodiscard]] tIndex RectLeft(const tRect& sRect) {
        tRect wRect(sRect);
        return wRect.Left();
    }

    [[nodiscard]] tIndex RectRight(const tRect& sRect) {
        tRect wRect(sRect);
        return wRect.Right();
    }

    [[nodiscard]] tIndex RectHeight(const tRect& sRect) {
        return RectBottom(sRect) - RectTop(sRect) + 1;
    }

    [[nodiscard]] tIndex RectWidth(const tRect& sRect) {
        return RectRight(sRect) - RectLeft(sRect) + 1;
    }

    } // namespace

    bool tStructuralOp::MatchesSheet(tAllocatorRef sAllocator) const {
        if (m_SheetAllocator != kInvalidIndex && sAllocator != kInvalidIndex && m_SheetAllocator == sAllocator) {
            return true;
        }
        return false;
    }

    tBool tRebasePlan::IsEmpty() {
        return m_Operations.empty();
    }

    void tRebasePlan::Clear() {
        m_SheetAllocator = kInvalidIndex;
        m_FromSequence = 0;
        m_ToSequence = 0;
        m_Operations.clear();
        m_Operations.shrink_to_fit();
    }

    std::optional<tIndex> tRebasePlan::RebaseRow(tIndex sRow) const {
        std::optional<tIndex> wRow = sRow;
        // Track InsertRow operations that shift rows (for cancellation detection)
        struct tInsertRowInfo {
            tSequenceId m_Sequence;   // Sequence ID of the InsertRow operation
            tIndex m_OriginalRow;     // Row position before being shifted by this InsertRow
        };
        std::vector<tInsertRowInfo> wInsertRowInfos;
        
        // Track InsertRect operations that shift rows (for cancellation detection)
        struct tInsertRectInfo {
            tSequenceId m_Sequence;   // Sequence ID of the InsertRect operation
            tIndex m_OriginalRow;      // Row position before being shifted by this InsertRect
        };
        std::vector<tInsertRectInfo> wInsertRectInfos;

        // Collect the sequences actually present in this plan. An Insert that cancels
        // a Delete only neutralizes its shift when that Delete is inside the current
        // plan window (a Do/Undo pair). When the cancelled Delete lives before the
        // window (e.g. Redo of a previously undone structural op), its effect is
        // already baked into the incoming coordinate, so the Insert must shift like a
        // normal insert. Without this, Redo of an undone Delete by rect resets the
        // coordinate and Rebase() fails.
        std::unordered_set<tSequenceId> wPlanSequences;
        wPlanSequences.reserve(m_Operations.size());
        for (const auto& wSeqOp : m_Operations) {
            wPlanSequences.insert(wSeqOp.m_Sequence);
        }

        for (const auto& wOp : m_Operations) {
            if (!wRow.has_value()) {
                break;
            }
            // Skip operations with count=0 (invalidated operations)
            if (wOp.m_Count == 0) {
                continue;
            }
            switch (wOp.m_Type) {
                case tStructuralOpType::InsertRow: {
                    // Check if this InsertRow is undoing a previous DeleteRow via m_CancelsSequence
                    bool wIsUndoingDeleteRow = (wOp.m_CancelsSequence != 0 &&
                                                wPlanSequences.count(wOp.m_CancelsSequence) != 0);
                    
                    tIndex wOriginalRow = wRow.value();
                    if (!wIsUndoingDeleteRow && wRow.value() >= wOp.m_Position) {
                        // Normal InsertRow: shift rows down
                        wRow = wRow.value() + wOp.m_Count;
                        // Track this InsertRow if it shifted our row (for cancellation detection)
                        if (wOriginalRow >= wOp.m_Position) {
                            tInsertRowInfo wInsertInfo;
                            wInsertInfo.m_Sequence = wOp.m_Sequence;
                            wInsertInfo.m_OriginalRow = wOriginalRow;
                            wInsertRowInfos.push_back(wInsertInfo);
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteRow: {
                    const tIndex wStart = wOp.m_Position;
                    const tIndex wEnd = wOp.m_Position + wOp.m_Count - 1;
                    
                    // Check if this DeleteRow is undoing a previous InsertRow via m_CancelsSequence
                    bool wIsUndoingInsertRow = false;
                    if (wOp.m_CancelsSequence != 0) {
                        // Find the InsertRow operation with the matching sequence ID
                        for (auto it = wInsertRowInfos.begin(); it != wInsertRowInfos.end(); ++it) {
                            if (it->m_Sequence == wOp.m_CancelsSequence) {
                                wIsUndoingInsertRow = true;
                                // Restore the row to its position before the InsertRow shift
                                wRow = it->m_OriginalRow;
                                // Remove this info as it's been undone
                                wInsertRowInfos.erase(it);
                                break;
                            }
                        }
                    }
                    
                    if (!wIsUndoingInsertRow) {
                        if (wRow.value() < wStart) {
                            // No impact.
                        } else if (wRow.value() > wEnd) {
                            wRow = wRow.value() - wOp.m_Count;
                        } else {
                            // Position is within deletion zone
                            // For Undo operations (e.g., Undo of InsertRow becomes DeleteRow),
                            // use the position just before deletion instead of invalidating
                            // This allows the operation to proceed even if the position was deleted
                            wRow = wStart;
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertRect: {
                    if (!wOp.m_DoesRow) {
                        break;
                    }
                    // Skip invalidated rects (invalidated rects have Bottom = -1)
                    if (RectBottom(wOp.m_Rect) == static_cast<tIndex>(-1)) {
                        break;
                    }
                    // Check if this InsertRect is undoing a previous DeleteRect via m_CancelsSequence
                    bool wIsUndoingDeleteRect = (wOp.m_CancelsSequence != 0 &&
                                                 wPlanSequences.count(wOp.m_CancelsSequence) != 0);
                    
                    tIndex wOriginalRow = wRow.value();
                    if (!wIsUndoingDeleteRect && wRow.value() >= RectTop(wOp.m_Rect)) {
                        // Normal InsertRect: shift rows down
                        wRow = wRow.value() + RectHeight(wOp.m_Rect);
                        // Track this InsertRect if it shifted our row (for cancellation detection)
                        if (wOriginalRow >= RectTop(wOp.m_Rect)) {
                            tInsertRectInfo wInsertInfo;
                            wInsertInfo.m_Sequence = wOp.m_Sequence;
                            wInsertInfo.m_OriginalRow = wOriginalRow;
                            wInsertRectInfos.push_back(wInsertInfo);
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteRect: {
                    if (!wOp.m_DoesRow) {
                        break;
                    }
                    // Skip invalidated rects (invalidated rects have Bottom = -1)
                    if (RectBottom(wOp.m_Rect) == static_cast<tIndex>(-1)) {
                        break;
                    }
                    const tIndex wTop = RectTop(wOp.m_Rect);
                    const tIndex wBottom = RectBottom(wOp.m_Rect);
                    
                    // Check if this DeleteRect is undoing a previous InsertRect via m_CancelsSequence
                    bool wIsUndoingInsertRect = false;
                    if (wOp.m_CancelsSequence != 0) {
                        // Find the InsertRect operation with the matching sequence ID
                        for (auto it = wInsertRectInfos.begin(); it != wInsertRectInfos.end(); ++it) {
                            if (it->m_Sequence == wOp.m_CancelsSequence) {
                                wIsUndoingInsertRect = true;
                                // Restore the row to its position before the InsertRect shift
                                wRow = it->m_OriginalRow;
                                // Remove this info as it's been undone
                                wInsertRectInfos.erase(it);
                                break;
                            }
                        }
                    }
                    
                    if (!wIsUndoingInsertRect) {
                        if (wRow.value() < wTop) {
                            // No impact.
                        } else if (wRow.value() > wBottom) {
                            wRow = wRow.value() - RectHeight(wOp.m_Rect);
                        } else {
                            wRow.reset();
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertCol:
                case tStructuralOpType::DeleteCol: 
                    // Column operations do not impact row coordinates directly.
                    break;
            }
        }
        return wRow;
    }

    std::optional<tIndex> tRebasePlan::RebaseCol(tIndex sCol) const {
        std::optional<tIndex> wCol = sCol;
        // Track InsertCol operations that shift columns (for cancellation detection)
        struct tInsertColInfo {
            tSequenceId m_Sequence;   // Sequence ID of the InsertCol operation
            tIndex m_OriginalCol;     // Column position before being shifted by this InsertCol
        };
        std::vector<tInsertColInfo> wInsertColInfos;
        
        // Track InsertRect operations that shift columns (for cancellation detection)
        struct tInsertRectInfo {
            tSequenceId m_Sequence;   // Sequence ID of the InsertRect operation
            tIndex m_OriginalCol;      // Column position before being shifted by this InsertRect
        };
        std::vector<tInsertRectInfo> wInsertRectInfos;

        // See RebaseRow(): an Insert only neutralizes its shift when the cancelled
        // Delete is inside this plan window; otherwise it must shift normally.
        std::unordered_set<tSequenceId> wPlanSequences;
        wPlanSequences.reserve(m_Operations.size());
        for (const auto& wSeqOp : m_Operations) {
            wPlanSequences.insert(wSeqOp.m_Sequence);
        }

        for (const auto& wOp : m_Operations) {
            if (!wCol.has_value()) {
                break;
            }
            // Skip operations with count=0 (invalidated operations)
            if (wOp.m_Count == 0) {
                continue;
            }
            switch (wOp.m_Type) {
                case tStructuralOpType::InsertCol: {
                    // Check if this InsertCol is undoing a previous DeleteCol via m_CancelsSequence
                    bool wIsUndoingDeleteCol = (wOp.m_CancelsSequence != 0 &&
                                                wPlanSequences.count(wOp.m_CancelsSequence) != 0);
                    
                    tIndex wOriginalCol = wCol.value();
                    if (!wIsUndoingDeleteCol && wCol.value() >= wOp.m_Position) {
                        // Normal InsertCol: shift columns to the right
                        wCol = wCol.value() + wOp.m_Count;
                        // Track this InsertCol if it shifted our column (for cancellation detection)
                        if (wOriginalCol >= wOp.m_Position) {
                            tInsertColInfo wInsertInfo;
                            wInsertInfo.m_Sequence = wOp.m_Sequence;
                            wInsertInfo.m_OriginalCol = wOriginalCol;
                            wInsertColInfos.push_back(wInsertInfo);
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteCol: {
                    const tIndex wStart = wOp.m_Position;
                    const tIndex wEnd = wOp.m_Position + wOp.m_Count - 1;
                    
                    // Check if this DeleteCol is undoing a previous InsertCol via m_CancelsSequence
                    bool wIsUndoingInsertCol = false;
                    if (wOp.m_CancelsSequence != 0) {
                        // Find the InsertCol operation with the matching sequence ID
                        for (auto it = wInsertColInfos.begin(); it != wInsertColInfos.end(); ++it) {
                            if (it->m_Sequence == wOp.m_CancelsSequence) {
                                wIsUndoingInsertCol = true;
                                // Restore the column to its position before the InsertCol shift
                                wCol = it->m_OriginalCol;
                                // Remove this info as it's been undone
                                wInsertColInfos.erase(it);
                                break;
                            }
                        }
                    }
                    
                    if (!wIsUndoingInsertCol) {
                        if (wCol.value() < wStart) {
                            // No impact - column is before deletion zone
                        } else if (wCol.value() > wEnd) {
                            // Column is after deletion zone - shift left by deletion count
                            wCol = wCol.value() - wOp.m_Count;
                        } else {
                            // Position is within deletion zone
                            // For Undo operations (e.g., Undo of InsertCol becomes DeleteCol),
                            // use the position just before deletion instead of invalidating
                            // This allows the operation to proceed even if the position was deleted
                            // When a column is deleted, columns that were inserted at that position
                            // by previous InsertCol operations should be mapped to the deletion start position
                            // This ensures that undo/redo sequences work correctly
                            // Note: wStart represents the original position before any shifts,
                            // so mapping to wStart correctly handles the case where a column
                            // was shifted by a previous InsertCol and then falls within deletion zone
                            wCol = wStart;
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertRect: {
                    if (wOp.m_DoesRow) {
                        break;
                    }
                    // Skip invalidated rects (invalidated rects have Right = -1)
                    if (RectRight(wOp.m_Rect) == static_cast<tIndex>(-1)) {
                        break;
                    }
                    // Check if this InsertRect is undoing a previous DeleteRect via m_CancelsSequence
                    bool wIsUndoingDeleteRect = (wOp.m_CancelsSequence != 0 &&
                                                 wPlanSequences.count(wOp.m_CancelsSequence) != 0);
                    
                    tIndex wOriginalCol = wCol.value();
                    if (!wIsUndoingDeleteRect && wCol.value() >= RectLeft(wOp.m_Rect)) {
                        // Normal InsertRect: shift columns to the right
                        wCol = wCol.value() + RectWidth(wOp.m_Rect);
                        // Track this InsertRect if it shifted our column (for cancellation detection)
                        if (wOriginalCol >= RectLeft(wOp.m_Rect)) {
                            tInsertRectInfo wInsertInfo;
                            wInsertInfo.m_Sequence = wOp.m_Sequence;
                            wInsertInfo.m_OriginalCol = wOriginalCol;
                            wInsertRectInfos.push_back(wInsertInfo);
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteRect: {
                    if (wOp.m_DoesRow) {
                        break;
                    }
                    // Skip invalidated rects (invalidated rects have Right = -1)
                    if (RectRight(wOp.m_Rect) == static_cast<tIndex>(-1)) {
                        break;
                    }
                    const tIndex wLeft = RectLeft(wOp.m_Rect);
                    const tIndex wRight = RectRight(wOp.m_Rect);
                    
                    // Check if this DeleteRect is undoing a previous InsertRect via m_CancelsSequence
                    bool wIsUndoingInsertRect = false;
                    if (wOp.m_CancelsSequence != 0) {
                        // Find the InsertRect operation with the matching sequence ID
                        for (auto it = wInsertRectInfos.begin(); it != wInsertRectInfos.end(); ++it) {
                            if (it->m_Sequence == wOp.m_CancelsSequence) {
                                wIsUndoingInsertRect = true;
                                // Restore the column to its position before the InsertRect shift
                                wCol = it->m_OriginalCol;
                                // Remove this info as it's been undone
                                wInsertRectInfos.erase(it);
                                break;
                            }
                        }
                    }
                    
                    if (!wIsUndoingInsertRect) {
                        if (wCol.value() < wLeft) {
                            // No impact.
                        } else if (wCol.value() > wRight) {
                            wCol = wCol.value() - RectWidth(wOp.m_Rect);
                        } else {
                            wCol.reset();
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertRow:
                case tStructuralOpType::DeleteRow:
                    // Row operations do not impact column coordinates directly.
                    break;
            }
        }
        return wCol;
    }

    std::optional<tRect> tRebasePlan::RebaseRect(const tRect& sRect) const {
        auto wTop = RebaseRow(RectTop(sRect));
        auto wBottom = RebaseRow(RectBottom(sRect));
        auto wLeft = RebaseCol(RectLeft(sRect));
        auto wRight = RebaseCol(RectRight(sRect));

        if (!wTop.has_value() || !wBottom.has_value() || !wLeft.has_value() || !wRight.has_value()) {
            return std::nullopt;
        }

        if (wTop.value() > wBottom.value() || wLeft.value() > wRight.value()) {
            return std::nullopt;
        }

        tRect wRebased;
        wRebased.Set(wTop.value(), wLeft.value(), wBottom.value(), wRight.value());
        return wRebased;
    }

    std::optional<tPoint> tRebasePlan::RebasePoint(const tPoint& sPoint) const {
        tIndex wOriginalRow = sPoint.Row();
        tIndex wOriginalCol = sPoint.Col();
        auto wRebasedRow = RebaseRow(wOriginalRow);
        auto wRebasedCol = RebaseCol(wOriginalCol);
        if (!wRebasedRow.has_value() || !wRebasedCol.has_value()) {
            return std::nullopt;
        }
        return tPoint(wRebasedRow.value(), wRebasedCol.value());
    }
        
    tUndoRebaseLog::tUndoRebaseLog()
        : m_SequenceCounter(0) {
        m_Operations.reserve(1024);
    }

    tUndoRebaseLog::tUndoRebaseLog(const tUndoRebaseLog& sOther)
        : m_SequenceCounter(sOther.m_SequenceCounter),
        m_Operations(sOther.m_Operations) {
        // Mutex is not copied (default constructed), which is correct for a copy
        m_Operations.reserve(1024);
    }

    tUndoRebaseLog& tUndoRebaseLog::operator=(const tUndoRebaseLog& sOther) {
        if (this != &sOther) {
            std::lock_guard<std::mutex> wLock(m_Mutex);
            m_SequenceCounter = sOther.m_SequenceCounter;
            m_Operations = sOther.m_Operations;
        }
        return *this;
    }

    tSequenceId tUndoRebaseLog::RegisterInsertRow(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence, std::uint64_t sOperationId) {
        return Register(tStructuralOpType::InsertRow, sSheetAllocator,  sPosition, sCount, tRect(), false, sCancelsSequence, sOperationId);
    }

    tSequenceId tUndoRebaseLog::RegisterDeleteRow(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence, std::uint64_t sOperationId) {
        return Register(tStructuralOpType::DeleteRow, sSheetAllocator, sPosition, sCount, tRect(), false, sCancelsSequence, sOperationId);
    }

    tSequenceId tUndoRebaseLog::RegisterInsertCol(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence, std::uint64_t sOperationId) {
        return Register(tStructuralOpType::InsertCol, sSheetAllocator, sPosition, sCount, tRect(), false, sCancelsSequence, sOperationId);
    }

    tSequenceId tUndoRebaseLog::RegisterDeleteCol(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence, std::uint64_t sOperationId) {
        return Register(tStructuralOpType::DeleteCol, sSheetAllocator, sPosition, sCount, tRect(), false, sCancelsSequence, sOperationId);
    }

    tSequenceId tUndoRebaseLog::RegisterInsertRect(tAllocatorRef sSheetAllocator, const tRect& sRect, tSequenceId sCancelsSequence, std::uint64_t sOperationId, bool sDoesRow) {
        tRect wRect=sRect;
        return Register(tStructuralOpType::InsertRect, sSheetAllocator, wRect.Top(), RectHeight(sRect), sRect, true, sCancelsSequence, sOperationId, sDoesRow);
    }

    tSequenceId tUndoRebaseLog::RegisterDeleteRect(tAllocatorRef sSheetAllocator, const tRect& sRect, tSequenceId sCancelsSequence, std::uint64_t sOperationId, bool sDoesRow) {
        tRect wRect=sRect;
        return Register(tStructuralOpType::DeleteRect, sSheetAllocator,  wRect.Top(), RectHeight(sRect), sRect, true, sCancelsSequence, sOperationId, sDoesRow);
    }

    tSequenceId tUndoRebaseLog::Register(tStructuralOpType sType, tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, const tRect& sRect, bool sIsRect, tSequenceId sCancelsSequence, std::uint64_t sOperationId, bool sDoesRow) {
        std::lock_guard<std::mutex> wLock(m_Mutex);

        // Rebase existing operations on the same sheet before adding the new one
        for (auto& wExistingOp : m_Operations) {
            // Only rebase operations on the same sheet
            if (!wExistingOp.MatchesSheet(sSheetAllocator)) {
                continue;
            }

            // Rebase based on operation type
            switch (sType) {
                case tStructuralOpType::InsertRow: {
                    // Rebase row operations
                    // When inserting rows at position sPosition, all rows at position >= sPosition are shifted down by sCount
                    // So existing operations at position >= sPosition must be rebased
                    if (wExistingOp.m_Type == tStructuralOpType::InsertRow || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteRow) {
                        if (wExistingOp.m_Position >= sPosition) {
                            // Shift the position down by sCount (insertion shifts everything down)
                            wExistingOp.m_Position += sCount;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingTop = RectTop(wExistingOp.m_Rect);
                        if (wExistingTop >= sPosition) {
                            // Shift the entire rect down by sCount
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top() + sCount, 
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom() + sCount,
                                            wRebasedRect.Right());
                            wExistingOp.m_Rect = wRebasedRect;
                            wExistingOp.m_Position = wRebasedRect.Top();
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertRect: {
                    tIndex wNewTop = RectTop(sRect);
                    tIndex wNewLeft = RectLeft(sRect);
                    tIndex wNewHeight = RectHeight(sRect);
                    tIndex wNewWidth = RectWidth(sRect);
                    
                    // Rebase row operations
                    if (wExistingOp.m_Type == tStructuralOpType::InsertRow || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteRow) {
                        if (wExistingOp.m_Position >= wNewTop) {
                            wExistingOp.m_Position += wNewHeight;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertCol || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteCol) {
                        // Rebase column operations
                        if (wExistingOp.m_Position >= wNewLeft) {
                            wExistingOp.m_Position += wNewWidth;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingTop = RectTop(wExistingOp.m_Rect);
                        tIndex wExistingLeft = RectLeft(wExistingOp.m_Rect);
                        tRect wRebasedRect = wExistingOp.m_Rect;
                        bool wRebased = false;
                        
                        // Rebase rows
                        if (wExistingTop >= wNewTop) {
                            wRebasedRect.Set(wRebasedRect.Top() + wNewHeight, 
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom() + wNewHeight,
                                            wRebasedRect.Right());
                            wRebased = true;
                        }
                        // Rebase columns
                        if (wExistingLeft >= wNewLeft) {
                            wRebasedRect.Set(wRebasedRect.Top(), 
                                            wRebasedRect.Left() + wNewWidth,
                                            wRebasedRect.Bottom(),
                                            wRebasedRect.Right() + wNewWidth);
                            wRebased = true;
                        }
                        if (wRebased) {
                            wExistingOp.m_Rect = wRebasedRect;
                            wExistingOp.m_Position = wRebasedRect.Top();
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteRow: {
                    // When deleting rows from sPosition to sPosition+sCount-1, all rows after the deleted range are shifted up by sCount
                    // So existing operations at position > (sPosition + sCount - 1) must be rebased
                    tIndex wDeleteEnd = sPosition + sCount - 1;
                    
                    // Rebase row operations
                    if (wExistingOp.m_Type == tStructuralOpType::InsertRow || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteRow) {
                        tIndex wExistingEnd = wExistingOp.m_Position + wExistingOp.m_Count - 1;
                        
                        if (wExistingOp.m_Position > wDeleteEnd) {
                            // Operation is completely after deletion: shift position up by sCount
                            wExistingOp.m_Position -= sCount;
                        } else if (wExistingOp.m_Position >= sPosition && wExistingOp.m_Position <= wDeleteEnd) {
                            // Operation starts within deletion zone
                            if (wExistingEnd > wDeleteEnd) {
                                // Operation extends beyond deletion: rebase to start after deletion
                                // The part after deletion is shifted up by sCount
                                wExistingOp.m_Position = sPosition;
                                wExistingOp.m_Count = wExistingEnd - wDeleteEnd;
                            } else {
                                // Operation ends at or before deletion end
                                // Check if operation is completely within deletion zone
                                if (wExistingOp.m_Position >= sPosition && wExistingEnd <= wDeleteEnd) {
                                    // Entire operation is within deletion zone
                                    // Special case: if InsertRow exactly matches DeleteRow (same position and count),
                                    // they are opposite operations that should coexist and cancel naturally during rebase
                                    // Do NOT invalidate InsertRow in this case - let them cancel naturally
                                    if (wExistingOp.m_Type == tStructuralOpType::InsertRow && 
                                        wExistingOp.m_Position == sPosition && wExistingOp.m_Count == sCount) {
                                        // InsertRow exactly matches DeleteRow - keep it as is, they'll cancel during rebase
                                        // No change needed
                                    } else {
                                        // Operation is completely within deletion zone but doesn't exactly match
                                        // Invalidate it
                                        wExistingOp.m_Count = 0;
                                    }
                                } else {
                                    // This case shouldn't happen given the outer condition, but handle it
                                    wExistingOp.m_Count = 0;
                                }
                            }
                        } else if (wExistingOp.m_Position < sPosition && wExistingEnd >= sPosition) {
                            // Operation starts before deletion but overlaps with it
                            // This is a complex case: the operation partially overlaps with deletion
                            // For InsertRow: the inserted rows that fall in the deletion zone are effectively deleted
                            // For DeleteRow: this creates a conflict that's hard to resolve
                            // Simplest approach: reduce count by the overlapping portion
                            tIndex wOverlapStart = sPosition;
                            tIndex wOverlapEnd = (wExistingEnd < wDeleteEnd) ? wExistingEnd : wDeleteEnd;
                            tIndex wOverlapCount = wOverlapEnd - wOverlapStart + 1;
                            
                            if (wOverlapCount >= wExistingOp.m_Count) {
                                // Entire operation overlaps - invalidate it
                                wExistingOp.m_Count = 0;
                            } else {
                                // Reduce count by overlap amount
                                wExistingOp.m_Count -= wOverlapCount;
                                // Position stays the same since it's before deletion
                            }
                        }
                        // If operation ends before deletion start, no rebase needed
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingTop = RectTop(wExistingOp.m_Rect);
                        tIndex wExistingBottom = RectBottom(wExistingOp.m_Rect);
                        
                        if (wExistingTop > wDeleteEnd) {
                            // Rect is completely after deletion: shift entire rect up by sCount
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top() - sCount, 
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom() - sCount,
                                            wRebasedRect.Right());
                            wExistingOp.m_Rect = wRebasedRect;
                            wExistingOp.m_Position = wRebasedRect.Top();
                        } else if (wExistingTop >= sPosition && wExistingTop <= wDeleteEnd) {
                            // Rect starts within deletion zone
                            if (wExistingBottom > wDeleteEnd) {
                                // Rect extends beyond deletion: shift the part after deletion up
                                tRect wRebasedRect = wExistingOp.m_Rect;
                                tIndex wNewTop = sPosition;
                                tIndex wNewBottom = wExistingBottom - sCount;
                                wRebasedRect.Set(wNewTop,
                                                wRebasedRect.Left(),
                                                wNewBottom,
                                                wRebasedRect.Right());
                                wExistingOp.m_Rect = wRebasedRect;
                                wExistingOp.m_Position = wRebasedRect.Top();
                            } else {
                                // Entire rect is within deletion zone - invalidate it
                                wExistingOp.m_Rect.Set(0, 0, -1, -1);
                                wExistingOp.m_Position = 0;
                            }
                        } else if (wExistingTop < sPosition && wExistingBottom >= sPosition) {
                            // Rect starts before deletion but overlaps with it
                            // Truncate bottom to just before deletion
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top(),
                                            wRebasedRect.Left(),
                                            sPosition - 1,
                                            wRebasedRect.Right());
                            wExistingOp.m_Rect = wRebasedRect;
                            wExistingOp.m_Position = wRebasedRect.Top();
                        }
                        // If rect ends before deletion start, no rebase needed
                    }
                    break;
                }
                case tStructuralOpType::DeleteRect: {
                    tIndex wDeleteTop = RectTop(sRect);
                    tIndex wDeleteLeft = RectLeft(sRect);
                    tIndex wDeleteHeight = RectHeight(sRect);
                    tIndex wDeleteWidth = RectWidth(sRect);
                    tIndex wDeleteTopEnd = wDeleteTop + wDeleteHeight - 1;
                    tIndex wDeleteLeftEnd = wDeleteLeft + wDeleteWidth - 1;
                    
                    // Rebase row operations
                    if (wExistingOp.m_Type == tStructuralOpType::InsertRow || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteRow) {
                        if (wExistingOp.m_Position > wDeleteTopEnd) {
                            wExistingOp.m_Position -= wDeleteHeight;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertCol || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteCol) {
                        // Rebase column operations
                        if (wExistingOp.m_Position > wDeleteLeftEnd) {
                            wExistingOp.m_Position -= wDeleteWidth;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingTop = RectTop(wExistingOp.m_Rect);
                        tIndex wExistingBottom = RectBottom(wExistingOp.m_Rect);
                        tIndex wExistingLeft = RectLeft(wExistingOp.m_Rect);
                        tIndex wExistingRight = RectRight(wExistingOp.m_Rect);
                        tRect wRebasedRect = wExistingOp.m_Rect;
                        bool wRebased = false;
                        bool wInvalidated = false;
                        
                        // Check if rect is completely within deletion zone
                        bool wCompletelyWithin = (wExistingTop >= wDeleteTop && wExistingBottom <= wDeleteTopEnd &&
                                                 wExistingLeft >= wDeleteLeft && wExistingRight <= wDeleteLeftEnd);
                        
                        // Special case: if InsertRect exactly matches DeleteRect, they should coexist
                        if (wCompletelyWithin && wExistingOp.m_Type == tStructuralOpType::InsertRect) {
                            tRect wDeleteRect = sRect;
                            if (wExistingTop == RectTop(wDeleteRect) && wExistingBottom == RectBottom(wDeleteRect) &&
                                wExistingLeft == RectLeft(wDeleteRect) && wExistingRight == RectRight(wDeleteRect)) {
                                // InsertRect exactly matches DeleteRect - keep it as is, they'll cancel during rebase
                                // No change needed
                                break;
                            }
                        }
                        
                        // Handle row rebasing
                        if (wExistingTop > wDeleteTopEnd) {
                            // Rect is completely below deletion zone: shift up by deletion height
                            wRebasedRect.Set(wRebasedRect.Top() - wDeleteHeight, 
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom() - wDeleteHeight,
                                            wRebasedRect.Right());
                            wRebased = true;
                        } else if (wExistingTop >= wDeleteTop && wExistingTop <= wDeleteTopEnd) {
                            // Rect starts within deletion zone (rows)
                            if (wExistingBottom > wDeleteTopEnd) {
                                // Rect extends below deletion: shift the part below deletion up
                                tIndex wNewTop = wDeleteTop;
                                tIndex wNewBottom = wExistingBottom - wDeleteHeight;
                                wRebasedRect.Set(wNewTop,
                                                wRebasedRect.Left(),
                                                wNewBottom,
                                                wRebasedRect.Right());
                                wRebased = true;
                            } else if (wExistingBottom <= wDeleteTopEnd) {
                                // Rect is completely within deletion zone (rows)
                                // Check if also completely within columns
                                if (wExistingLeft >= wDeleteLeft && wExistingRight <= wDeleteLeftEnd) {
                                    // Completely within deletion zone - invalidate it
                                    wInvalidated = true;
                                } else {
                                    // Only rows are within, columns extend beyond - truncate rows
                                    wRebasedRect.Set(wRebasedRect.Top(),
                                                    wRebasedRect.Left(),
                                                    wDeleteTop - 1,
                                                    wRebasedRect.Right());
                                    wRebased = true;
                                }
                            }
                        } else if (wExistingTop < wDeleteTop && wExistingBottom >= wDeleteTop) {
                            // Rect starts above deletion but overlaps with it (rows)
                            if (wExistingBottom > wDeleteTopEnd) {
                                // Rect extends below deletion: truncate to exclude deletion zone
                                // Keep top part, remove middle (deletion), shift bottom part up
                                // This is complex - simplify by truncating bottom to just before deletion
                                wRebasedRect.Set(wRebasedRect.Top(),
                                                wRebasedRect.Left(),
                                                wDeleteTop - 1,
                                                wRebasedRect.Right());
                                wRebased = true;
                            } else {
                                // Rect overlaps but doesn't extend beyond: truncate bottom
                                wRebasedRect.Set(wRebasedRect.Top(),
                                                wRebasedRect.Left(),
                                                wDeleteTop - 1,
                                                wRebasedRect.Right());
                                wRebased = true;
                            }
                        }
                        
                        // Handle column rebasing (only if not already invalidated)
                        if (!wInvalidated) {
                            if (wExistingLeft > wDeleteLeftEnd) {
                                // Rect is completely to the right of deletion zone: shift left by deletion width
                                wRebasedRect.Set(wRebasedRect.Top(), 
                                                wRebasedRect.Left() - wDeleteWidth,
                                                wRebasedRect.Bottom(),
                                                wRebasedRect.Right() - wDeleteWidth);
                                wRebased = true;
                            } else if (wExistingLeft >= wDeleteLeft && wExistingLeft <= wDeleteLeftEnd) {
                                // Rect starts within deletion zone (columns)
                                if (wExistingRight > wDeleteLeftEnd) {
                                    // Rect extends to the right of deletion: shift the part to the right left
                                    tIndex wNewLeft = wDeleteLeft;
                                    tIndex wNewRight = wExistingRight - wDeleteWidth;
                                    wRebasedRect.Set(wRebasedRect.Top(),
                                                    wNewLeft,
                                                    wRebasedRect.Bottom(),
                                                    wNewRight);
                                    wRebased = true;
                                } else if (wExistingRight <= wDeleteLeftEnd) {
                                    // Rect is completely within deletion zone (columns)
                                    // Check if also completely within rows (already checked above)
                                    if (wExistingTop >= wDeleteTop && wExistingBottom <= wDeleteTopEnd) {
                                        // Completely within deletion zone - invalidate it
                                        wInvalidated = true;
                                    } else {
                                        // Only columns are within, rows extend beyond - truncate columns
                                        wRebasedRect.Set(wRebasedRect.Top(),
                                                        wRebasedRect.Left(),
                                                        wRebasedRect.Bottom(),
                                                        wDeleteLeft - 1);
                                        wRebased = true;
                                    }
                                }
                            } else if (wExistingLeft < wDeleteLeft && wExistingRight >= wDeleteLeft) {
                                // Rect starts to the left of deletion but overlaps with it (columns)
                                if (wExistingRight > wDeleteLeftEnd) {
                                    // Rect extends to the right of deletion: truncate to exclude deletion zone
                                    wRebasedRect.Set(wRebasedRect.Top(),
                                                    wRebasedRect.Left(),
                                                    wRebasedRect.Bottom(),
                                                    wDeleteLeft - 1);
                                    wRebased = true;
                                } else {
                                    // Rect overlaps but doesn't extend beyond: truncate right
                                    wRebasedRect.Set(wRebasedRect.Top(),
                                                    wRebasedRect.Left(),
                                                    wRebasedRect.Bottom(),
                                                    wDeleteLeft - 1);
                                    wRebased = true;
                                }
                            }
                        }
                        
                        if (wInvalidated) {
                            // Invalidate the rect
                            wExistingOp.m_Rect.Set(0, 0, -1, -1);
                            wExistingOp.m_Position = 0;
                        } else if (wRebased) {
                            wExistingOp.m_Rect = wRebasedRect;
                            wExistingOp.m_Position = wRebasedRect.Top();
                        }
                    }
                    break;
                }
                case tStructuralOpType::InsertCol: {
                    // Rebase column operations
                    // When inserting columns at position sPosition, all columns at position >= sPosition are shifted right by sCount
                    // So existing operations at position >= sPosition must be rebased
                    if (wExistingOp.m_Type == tStructuralOpType::InsertCol || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteCol) {
                        if (wExistingOp.m_Position >= sPosition) {
                            // Shift the position right by sCount (insertion shifts everything right)
                            wExistingOp.m_Position += sCount;
                        }
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingLeft = RectLeft(wExistingOp.m_Rect);
                        tIndex wExistingRight = RectRight(wExistingOp.m_Rect);
                        if (wExistingLeft >= sPosition) {
                            // Rect is completely after insertion: shift entire rect right by sCount
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top(), 
                                            wRebasedRect.Left() + sCount,
                                            wRebasedRect.Bottom(),
                                            wRebasedRect.Right() + sCount);
                            wExistingOp.m_Rect = wRebasedRect;
                        } else if (wExistingRight >= sPosition) {
                            // Rect overlaps insertion: widen the rect by sCount
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top(), 
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom(),
                                            wRebasedRect.Right() + sCount);
                            wExistingOp.m_Rect = wRebasedRect;
                        }
                    }
                    break;
                }
                case tStructuralOpType::DeleteCol: {
                    // When deleting columns from sPosition to sPosition+sCount-1, all columns after the deleted range are shifted left by sCount
                    // So existing operations at position > (sPosition + sCount - 1) must be rebased
                    tIndex wDeleteEnd = sPosition + sCount - 1;
                    
                    // Rebase column operations
                    if (wExistingOp.m_Type == tStructuralOpType::InsertCol || 
                        wExistingOp.m_Type == tStructuralOpType::DeleteCol) {
                        tIndex wExistingEnd = wExistingOp.m_Position + wExistingOp.m_Count - 1;
                        
                        if (wExistingOp.m_Position > wDeleteEnd) {
                            // Operation is completely after deletion: shift position left by sCount
                            wExistingOp.m_Position -= sCount;
                        } else if (wExistingOp.m_Position >= sPosition && wExistingOp.m_Position <= wDeleteEnd) {
                            // Operation starts within deletion zone
                            if (wExistingEnd > wDeleteEnd) {
                                // Operation extends beyond deletion: rebase to start after deletion
                                // The part after deletion is shifted left by sCount
                                wExistingOp.m_Position = sPosition;
                                wExistingOp.m_Count = wExistingEnd - wDeleteEnd;
                            } else {
                                // Entire operation is within deletion zone
                                // Special case: if InsertCol exactly matches DeleteCol (same position and count),
                                // they are opposite operations that should coexist and cancel naturally during rebase
                                // Do NOT invalidate InsertCol in this case - let them cancel naturally
                                if (wExistingOp.m_Type == tStructuralOpType::InsertCol && 
                                    wExistingOp.m_Position == sPosition && wExistingOp.m_Count == sCount) {
                                    // InsertCol exactly matches DeleteCol - keep it as is, they'll cancel during rebase
                                    // No change needed
                                } else {
                                    // Operation is completely within deletion zone but doesn't exactly match
                                    // Invalidate it
                                    wExistingOp.m_Count = 0;
                                }
                            }
                        } else if (wExistingOp.m_Position < sPosition && wExistingEnd >= sPosition) {
                            // Operation starts before deletion but overlaps with it
                            // This is a complex case: the operation partially overlaps with deletion
                            // For InsertCol: the inserted columns that fall in the deletion zone are effectively deleted
                            // For DeleteCol: this creates a conflict that's hard to resolve
                            // Simplest approach: reduce count by the overlapping portion
                            tIndex wOverlapStart = sPosition;
                            tIndex wOverlapEnd = (wExistingEnd < wDeleteEnd) ? wExistingEnd : wDeleteEnd;
                            tIndex wOverlapCount = wOverlapEnd - wOverlapStart + 1;
                            
                            if (wOverlapCount >= wExistingOp.m_Count) {
                                // Entire operation overlaps - invalidate it
                                wExistingOp.m_Count = 0;
                            } else {
                                // Reduce count by overlap amount
                                wExistingOp.m_Count -= wOverlapCount;
                                // Position stays the same since it's before deletion
                            }
                        }
                        // If operation ends before deletion start, no rebase needed
                    } else if (wExistingOp.m_Type == tStructuralOpType::InsertRect || 
                            wExistingOp.m_Type == tStructuralOpType::DeleteRect) {
                        tIndex wExistingLeft = RectLeft(wExistingOp.m_Rect);
                        tIndex wExistingRight = RectRight(wExistingOp.m_Rect);
                        
                        if (wExistingLeft > wDeleteEnd) {
                            // Rect is completely after deletion: shift entire rect left by sCount
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top(), 
                                            wRebasedRect.Left() - sCount,
                                            wRebasedRect.Bottom(),
                                            wRebasedRect.Right() - sCount);
                            wExistingOp.m_Rect = wRebasedRect;
                        } else if (wExistingLeft >= sPosition && wExistingLeft <= wDeleteEnd) {
                            // Rect starts within deletion zone
                            if (wExistingRight > wDeleteEnd) {
                                // Rect extends beyond deletion: shift the part after deletion left
                                tRect wRebasedRect = wExistingOp.m_Rect;
                                tIndex wNewLeft = sPosition;
                                tIndex wNewRight = wExistingRight - sCount;
                                wRebasedRect.Set(wRebasedRect.Top(),
                                                wNewLeft,
                                                wRebasedRect.Bottom(),
                                                wNewRight);
                                wExistingOp.m_Rect = wRebasedRect;
                            } else {
                                // Entire rect is within deletion zone - invalidate it
                                wExistingOp.m_Rect.Set(0, 0, -1, -1);
                                wExistingOp.m_Position = 0;
                            }
                        } else if (wExistingLeft < sPosition && wExistingRight >= sPosition) {
                            // Rect starts before deletion but overlaps with it
                            // Truncate right to just before deletion
                            tRect wRebasedRect = wExistingOp.m_Rect;
                            wRebasedRect.Set(wRebasedRect.Top(),
                                            wRebasedRect.Left(),
                                            wRebasedRect.Bottom(),
                                            sPosition - 1);
                            wExistingOp.m_Rect = wRebasedRect;
                        }
                        // If rect ends before deletion start, no rebase needed
                    }
                    break;
                }
            }
        }

        // Create and add the new operation
        tStructuralOp wOp;
        wOp.m_Sequence = ++m_SequenceCounter;
        wOp.m_Type = sType;
        wOp.m_SheetAllocator = sSheetAllocator;
        wOp.m_Position = sPosition;
        wOp.m_Count = sCount;
        wOp.m_CancelsSequence = sCancelsSequence;
        wOp.m_OperationId = sOperationId;
        wOp.m_DoesRow = sDoesRow;
        if (sIsRect) {
            wOp.m_Rect = sRect;
        }

        m_Operations.emplace_back(wOp);
        return wOp.m_Sequence;
    }

    tSequenceId tUndoRebaseLog::LatestSequence() const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        return m_SequenceCounter;
    }

    tSequenceId tUndoRebaseLog::NextSequence() {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        tSequenceId wReturn = ++m_SequenceCounter;
        return wReturn;
    }

    std::vector<tStructuralOp> tUndoRebaseLog::Collect(tSequenceId sFromExclusive, tSequenceId sToInclusive, tAllocatorRef sSheetAllocator) const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        std::vector<tStructuralOp> wResult;
        if (sFromExclusive >= sToInclusive || m_Operations.empty()) {
            return wResult;
        }
        // Optimization
        wResult.reserve(16);
        // If sSheetAllocator is kInvalidIndex, collect operations from all sheets
        bool wCollectAllSheets = (sSheetAllocator == static_cast<tAllocatorRef>(kInvalidIndex));
        
        for (const auto& wOp : m_Operations) {
            if (wOp.m_Sequence <= sFromExclusive) {
                continue;
            }
            if (wOp.m_Sequence > sToInclusive) {
                break;
            }
            // If collecting all sheets, keep all operations; otherwise keep only operations from other sheets
            if (wCollectAllSheets) {
                wResult.emplace_back(wOp);
            } else {
                // Very Important Same Allocator Sheet 
                if (wOp.MatchesSheet(sSheetAllocator)) wResult.emplace_back(wOp);
            }
        }
        return wResult;
    }

    tRebasePlan tUndoRebaseLog::BuildPlan(tSequenceId sFromExclusive, tSequenceId sToInclusive, tAllocatorRef sSheetAllocator) const {
        tRebasePlan wPlan;
        wPlan.m_SheetAllocator=sSheetAllocator;
        wPlan.m_FromSequence = sFromExclusive;
        wPlan.m_ToSequence = sToInclusive;
        wPlan.m_Operations = Collect(sFromExclusive, sToInclusive, sSheetAllocator);
        return wPlan;
    }

    tSequenceId tUndoRebaseLog::FindLastOperation(tStructuralOpType sType, tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount) const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        
        // Search backwards through operations to find the last matching one
        for (auto it = m_Operations.rbegin(); it != m_Operations.rend(); ++it) {
            const auto& wOp = *it;
            
            // Check if operation matches the type
            if (wOp.m_Type != sType) {
                continue;
            }
            
            // Check if operation matches the sheet
            if (!wOp.MatchesSheet(sSheetAllocator)) {
                continue;
            }
            
            // Check if operation matches position (if specified)
            if (sPosition != static_cast<tIndex>(-1) && wOp.m_Position != sPosition) {
                continue;
            }
            
            // Check if operation matches count (if specified)
            if (sCount != static_cast<tIndex>(-1) && wOp.m_Count != sCount) {
                continue;
            }
            
            // Skip invalidated operations
            if (wOp.m_Count == 0) {
                continue;
            }
            
            // Found a match
            return wOp.m_Sequence;
        }
        
        return 0; // Not found
    }
    
    tSequenceId tUndoRebaseLog::FindLastOperation(tStructuralOpType sType, tAllocatorRef sSheetAllocator, const tRect& sRect, tInt sDoesRow) const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        
        // Search backwards through operations to find the last matching one
        for (auto it = m_Operations.rbegin(); it != m_Operations.rend(); ++it) {
            const auto& wOp = *it;
            
            // Check if operation matches the type
            if (wOp.m_Type != sType) {
                continue;
            }
            
            // Check if operation matches the sheet
            if (!wOp.MatchesSheet(sSheetAllocator)) {
                continue;
            }
            
            // Check the row/col discriminant when requested (a column rect and a row
            // rect can share identical bounds; without this, a column undo could cancel
            // the row insert instead of the column insert).
            if (sDoesRow != -1 && wOp.m_DoesRow != (sDoesRow != 0)) {
                continue;
            }
            
            // Check if operation matches the rectangle
            // Compare rectangle properties individually since operator== may not work with const references
            if (RectTop(wOp.m_Rect) != RectTop(sRect) ||
                RectBottom(wOp.m_Rect) != RectBottom(sRect) ||
                RectLeft(wOp.m_Rect) != RectLeft(sRect) ||
                RectRight(wOp.m_Rect) != RectRight(sRect)) {
                continue;
            }
            
            // Skip invalidated operations (invalidated rects have Bottom = -1 or Right = -1)
            if (RectBottom(wOp.m_Rect) == static_cast<tIndex>(-1) || RectRight(wOp.m_Rect) == static_cast<tIndex>(-1)) {
                continue;
            }
            
            // Found a match
            return wOp.m_Sequence;
        }
        
        return 0; // Not found
    }
    
    const tStructuralOp* tUndoRebaseLog::FindOperationByOperationId(std::uint64_t sOperationId) const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        
        if (sOperationId == 0) {
            return nullptr; // Invalid operation ID
        }
        
        // Search backwards through operations to find the most recent match
        for (auto it = m_Operations.rbegin(); it != m_Operations.rend(); ++it) {
            if (it->m_OperationId == sOperationId) {
                return &(*it);
            }
        }
        
        return nullptr; // Not found
    }

    tRebasePlan tUndoRebaseLog::BuildPlanAnotherSheet(const tRebasePlan& sRebasePlan, tAllocatorRef sSheetAllocator) const {
        tRebasePlan wPlan;
        wPlan.m_SheetAllocator=sSheetAllocator;
        wPlan.m_FromSequence = sRebasePlan.m_FromSequence;
        wPlan.m_ToSequence = sRebasePlan.m_ToSequence;
        wPlan.m_Operations = Collect(sRebasePlan.m_FromSequence, sRebasePlan.m_ToSequence, sSheetAllocator);
        return wPlan;
    }


    tString tUndoRebaseLog::DebugOperations() const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        tStringStream wStream;
        
        wStream << "UndoRebaseLog Debug - Total Operations: " << m_Operations.size() << std::endl;
        wStream << "Sequence Counter: " << m_SequenceCounter << std::endl;
        wStream << "----------------------------------------" << std::endl;
        
        if (m_Operations.empty()) {
            wStream << "No operations registered." << std::endl;
            return wStream.str();
        }
        
        for (const auto& wOp : m_Operations) {
            wStream << "Seq: " << wOp.m_Sequence << " | ";
            
            // Format operation type
            switch (wOp.m_Type) {
                case tStructuralOpType::InsertRow:
                    wStream << "InsertRow";
                    break;
                case tStructuralOpType::DeleteRow:
                    wStream << "DeleteRow";
                    break;
                case tStructuralOpType::InsertCol:
                    wStream << "InsertCol";
                    break;
                case tStructuralOpType::DeleteCol:
                    wStream << "DeleteCol";
                    break;
                case tStructuralOpType::InsertRect:
                    wStream << "InsertRect";
                    break;
                case tStructuralOpType::DeleteRect:
                    wStream << "DeleteRect";
                    break;
            }
            
            wStream << " | Sheet: ";
            if (wOp.m_SheetAllocator!=kInvalidIndex) {
                tSheet* wSheet= tSpreadSheetContainer::Instance()->ActiveWorkBook()->SheetByAllocator(wOp.m_SheetAllocator);
                if (wSheet!=nullptr) {
                    wStream << wSheet->Name();
                } else {
                    wStream << "N/A";
                }
            } else {
                wStream << "N/A";
            }
            
            wStream << " (Alloc: " << wOp.m_SheetAllocator << ")";
            
            // Display OperationId if present
            if (wOp.m_OperationId != 0) {
                wStream << " | OpId: " << wOp.m_OperationId;
            }
            
            // Display CancelsSequence if present
            if (wOp.m_CancelsSequence != 0) {
                wStream << " | Cancels: " << wOp.m_CancelsSequence;
            }
            
            // Format position and count for row/col operations
            if (wOp.m_Type == tStructuralOpType::InsertRow || 
                wOp.m_Type == tStructuralOpType::DeleteRow ||
                wOp.m_Type == tStructuralOpType::InsertCol ||
                wOp.m_Type == tStructuralOpType::DeleteCol) {
                wStream << " | Pos: " << wOp.m_Position << " | Count: " << wOp.m_Count;
            }
            
            // Format rect for rect operations
            if (wOp.m_Type == tStructuralOpType::InsertRect || 
                wOp.m_Type == tStructuralOpType::DeleteRect) {
                tRect wRect = wOp.m_Rect;
                wStream << " | Rect: [" << RectTop(wRect) << "," << RectLeft(wRect) 
                        << ":" << RectBottom(wRect) << "," << RectRight(wRect) << "]";
                wStream << " (H:" << RectHeight(wRect) << " W:" << RectWidth(wRect) << ")";
            }
            
            wStream << std::endl;
        }
        
        return wStream.str();
    }

    tString tUndoRebaseLog::JsonDebugOperations() const {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        
        wWriter.StartObject();
        
        // Total operations count
        wWriter.Key("totalOperations");
        wWriter.Int64(static_cast<std::int64_t>(m_Operations.size()));
        
        // Sequence counter
        wWriter.Key("sequenceCounter");
        wWriter.Int64(static_cast<std::int64_t>(m_SequenceCounter));
        
        // Operations array
        wWriter.Key("operations");
        wWriter.StartArray();
        
        for (const auto& wOp : m_Operations) {
            wWriter.StartObject();
            
            // Sequence ID
            wWriter.Key("sequence");
            wWriter.Int64(static_cast<std::int64_t>(wOp.m_Sequence));
            
            // Operation type
            wWriter.Key("type");
            switch (wOp.m_Type) {
                case tStructuralOpType::InsertRow:
                    wWriter.String("InsertRow");
                    break;
                case tStructuralOpType::DeleteRow:
                    wWriter.String("DeleteRow");
                    break;
                case tStructuralOpType::InsertCol:
                    wWriter.String("InsertCol");
                    break;
                case tStructuralOpType::DeleteCol:
                    wWriter.String("DeleteCol");
                    break;
                case tStructuralOpType::InsertRect:
                    wWriter.String("InsertRect");
                    break;
                case tStructuralOpType::DeleteRect:
                    wWriter.String("DeleteRect");
                    break;
            }
            
            // Sheet allocator
            wWriter.Key("sheetAllocator");
            wWriter.Int64(static_cast<std::int64_t>(wOp.m_SheetAllocator));
            
            // Sheet name
            wWriter.Key("sheetName");
            if (wOp.m_SheetAllocator != kInvalidIndex) {
                tSheet* wSheet = tSpreadSheetContainer::Instance()->ActiveWorkBook()->SheetByAllocator(wOp.m_SheetAllocator);
                if (wSheet != nullptr) {
                    wWriter.String(wSheet->Name().c_str());
                } else {
                    wWriter.Null();
                }
            } else {
                wWriter.Null();
            }
            
            // Position and count for row/col operations
            if (wOp.m_Type == tStructuralOpType::InsertRow || 
                wOp.m_Type == tStructuralOpType::DeleteRow ||
                wOp.m_Type == tStructuralOpType::InsertCol ||
                wOp.m_Type == tStructuralOpType::DeleteCol) {
                wWriter.Key("position");
                wWriter.Int64(static_cast<std::int64_t>(wOp.m_Position));
                
                wWriter.Key("count");
                wWriter.Int64(static_cast<std::int64_t>(wOp.m_Count));
            }
            
            // Rect for rect operations
            if (wOp.m_Type == tStructuralOpType::InsertRect || 
                wOp.m_Type == tStructuralOpType::DeleteRect) {
                tRect wRect = wOp.m_Rect;
                tIndex wTop = RectTop(wRect);
                tIndex wLeft = RectLeft(wRect);
                tIndex wBottom = RectBottom(wRect);
                tIndex wRight = RectRight(wRect);
                
                // Check if rect is invalidated (Bottom = -1)
                if (wBottom == static_cast<tIndex>(-1)) {
                    wWriter.Key("rect");
                    wWriter.Null();
                } else {
                    wWriter.Key("rect");
                    wWriter.StartObject();
                    
                    wWriter.Key("top");
                    wWriter.Int64(static_cast<std::int64_t>(wTop));
                    
                    wWriter.Key("left");
                    wWriter.Int64(static_cast<std::int64_t>(wLeft));
                    
                    wWriter.Key("bottom");
                    wWriter.Int64(static_cast<std::int64_t>(wBottom));
                    
                    wWriter.Key("right");
                    wWriter.Int64(static_cast<std::int64_t>(wRight));
                    
                    wWriter.Key("height");
                    wWriter.Int64(static_cast<std::int64_t>(RectHeight(wRect)));
                    
                    wWriter.Key("width");
                    wWriter.Int64(static_cast<std::int64_t>(RectWidth(wRect)));
                    
                    wWriter.EndObject();
                }
            }
            
            wWriter.EndObject();
        }
        
        wWriter.EndArray();
        wWriter.EndObject();
        
        return wBuffer.GetString();
    }

    tString tUndoRebaseLog::DebugUndoOperations(tUndoSpreadSheet* sUndo) const {
        if (sUndo == nullptr) {
            return "Error: Undo is nullptr";
        }
        
        // Try to get Sheet from tUndoSpreadSheetCallBack (most common case)
        tUndoSpreadSheetCallBack* wUndoCallBack = dynamic_cast<tUndoSpreadSheetCallBack*>(sUndo);
        tSheet* wSheet = nullptr;
        
        if (wUndoCallBack != nullptr) {
            wSheet = wUndoCallBack->Sheet();
        } else {
            // For Undo that inherit directly from tUndoSpreadSheet (like tUndoChangeSize),
            // we need to get the sheet differently. For now, return an error.
            return "Error: Cannot access Sheet from this Undo type (not a tUndoSpreadSheetCallBack)";
        }
        
        if (wSheet == nullptr) {
            return "Error: Sheet is nullptr";
        }
        
        tWorkBook* wWorkBook = wSheet->WorkBook();
        if (wWorkBook == nullptr) {
            return "Error: WorkBook is nullptr";
        }
        
        tSequenceId wUndoSequenceId = sUndo->SequenceId();
        tUndoRebaseLog& wRebaseLog = wWorkBook->UndoRebaseLog();
        tSequenceId wCurrentSequence = wRebaseLog.LatestSequence();
        
        tStringStream wStream;
        wStream << "UndoRebaseLog Debug - Operations since Undo creation" << std::endl;
        wStream << "Undo SequenceId: " << wUndoSequenceId << std::endl;
        wStream << "Current SequenceId: " << wCurrentSequence << std::endl;
        wStream << "Sheet: " << wSheet->Name() << " (Alloc: " << wSheet->AllocatorRef() << ")" << std::endl;
        wStream << "----------------------------------------" << std::endl;
        
        if (wCurrentSequence <= wUndoSequenceId) {
            wStream << "No operations performed since Undo creation." << std::endl;
            return wStream.str();
        }
        
        // Collect operations between Undo SequenceId and current SequenceId
        // Use kInvalidIndex to collect operations from all sheets for Intersheet references
        std::vector<tStructuralOp> wOperations = wRebaseLog.Collect(
            wUndoSequenceId,
            wCurrentSequence,
            static_cast<tAllocatorRef>(-1)  // Collect all sheets for Intersheet references
        );
        
        if (wOperations.empty()) {
            wStream << "No structural operations found in the specified range." << std::endl;
            return wStream.str();
        }
        
        // Filter operations for this sheet vs other sheets
        std::vector<tStructuralOp> wThisSheetOps;
        std::vector<tStructuralOp> wOtherSheetOps;
        tAllocatorRef wThisSheetAllocator = wSheet->AllocatorRef();
        
        for (const auto& wOp : wOperations) {
            if (wOp.MatchesSheet(wThisSheetAllocator)) {
                wThisSheetOps.push_back(wOp);
            } else {
                wOtherSheetOps.push_back(wOp);
            }
        }
        
        if (wThisSheetOps.empty() && wOtherSheetOps.empty()) {
            wStream << "No structural operations found in the specified range." << std::endl;
            return wStream.str();
        }
        
        if (!wThisSheetOps.empty()) {
            wStream << "Found " << wThisSheetOps.size() << " operation(s) on this sheet (" << wSheet->Name() << "):" << std::endl;
            wStream << std::endl;
            for (const auto& wOp : wThisSheetOps) {
                wStream << "Seq: " << wOp.m_Sequence << " | ";
                
                // Format operation type
                switch (wOp.m_Type) {
                    case tStructuralOpType::InsertRow:
                        wStream << "InsertRow";
                        break;
                    case tStructuralOpType::DeleteRow:
                        wStream << "DeleteRow";
                        break;
                    case tStructuralOpType::InsertCol:
                        wStream << "InsertCol";
                        break;
                    case tStructuralOpType::DeleteCol:
                        wStream << "DeleteCol";
                        break;
                    case tStructuralOpType::InsertRect:
                        wStream << "InsertRect";
                        break;
                    case tStructuralOpType::DeleteRect:
                        wStream << "DeleteRect";
                        break;
                }
                
                // Format position and count for row/col operations
                if (wOp.m_Type == tStructuralOpType::InsertRow || 
                    wOp.m_Type == tStructuralOpType::DeleteRow ||
                    wOp.m_Type == tStructuralOpType::InsertCol ||
                    wOp.m_Type == tStructuralOpType::DeleteCol) {
                    wStream << " | Pos: " << wOp.m_Position << " | Count: " << wOp.m_Count;
                }
                
                // Format rect for rect operations
                if (wOp.m_Type == tStructuralOpType::InsertRect || 
                    wOp.m_Type == tStructuralOpType::DeleteRect) {
                    tRect wRect = wOp.m_Rect;
                    wStream << " | Rect: [" << RectTop(wRect) << "," << RectLeft(wRect) 
                            << ":" << RectBottom(wRect) << "," << RectRight(wRect) << "]";
                    wStream << " (H:" << RectHeight(wRect) << " W:" << RectWidth(wRect) << ")";
                }
                
                wStream << std::endl;
            }
            wStream << std::endl;
        }
        
        if (!wOtherSheetOps.empty()) {
            wStream << "Found " << wOtherSheetOps.size() << " operation(s) on other sheets (affecting Intersheet references):" << std::endl;
            wStream << std::endl;
            for (const auto& wOp : wOtherSheetOps) {
                wStream << "Seq: " << wOp.m_Sequence << " | ";
                
                // Get sheet name
                tSheet* wOpSheet = tSpreadSheetContainer::Instance()->ActiveWorkBook()->SheetByAllocator(wOp.m_SheetAllocator);
                if (wOpSheet != nullptr) {
                    wStream << "Sheet: " << wOpSheet->Name() << " | ";
                } else {
                    wStream << "Sheet: N/A | ";
                }
                
                // Format operation type
                switch (wOp.m_Type) {
                    case tStructuralOpType::InsertRow:
                        wStream << "InsertRow";
                        break;
                    case tStructuralOpType::DeleteRow:
                        wStream << "DeleteRow";
                        break;
                    case tStructuralOpType::InsertCol:
                        wStream << "InsertCol";
                        break;
                    case tStructuralOpType::DeleteCol:
                        wStream << "DeleteCol";
                        break;
                    case tStructuralOpType::InsertRect:
                        wStream << "InsertRect";
                        break;
                    case tStructuralOpType::DeleteRect:
                        wStream << "DeleteRect";
                        break;
                }
                
                // Format position and count for row/col operations
                if (wOp.m_Type == tStructuralOpType::InsertRow || 
                    wOp.m_Type == tStructuralOpType::DeleteRow ||
                    wOp.m_Type == tStructuralOpType::InsertCol ||
                    wOp.m_Type == tStructuralOpType::DeleteCol) {
                    wStream << " | Pos: " << wOp.m_Position << " | Count: " << wOp.m_Count;
                }
                
                // Format rect for rect operations
                if (wOp.m_Type == tStructuralOpType::InsertRect || 
                    wOp.m_Type == tStructuralOpType::DeleteRect) {
                    tRect wRect = wOp.m_Rect;
                    wStream << " | Rect: [" << RectTop(wRect) << "," << RectLeft(wRect) 
                            << ":" << RectBottom(wRect) << "," << RectRight(wRect) << "]";
                    wStream << " (H:" << RectHeight(wRect) << " W:" << RectWidth(wRect) << ")";
                }
                
                wStream << std::endl;
            }
            return wStream.str();
        }
        
        // This should not happen, but handle it anyway
        return wStream.str();
        
        wStream << "Found " << wOperations.size() << " operation(s):" << std::endl;
        wStream << std::endl;
        
        for (const auto& wOp : wOperations) {
            wStream << "Seq: " << wOp.m_Sequence << " | ";
            
            // Format operation type
            switch (wOp.m_Type) {
                case tStructuralOpType::InsertRow:
                    wStream << "InsertRow";
                    break;
                case tStructuralOpType::DeleteRow:
                    wStream << "DeleteRow";
                    break;
                case tStructuralOpType::InsertCol:
                    wStream << "InsertCol";
                    break;
                case tStructuralOpType::DeleteCol:
                    wStream << "DeleteCol";
                    break;
                case tStructuralOpType::InsertRect:
                    wStream << "InsertRect";
                    break;
                case tStructuralOpType::DeleteRect:
                    wStream << "DeleteRect";
                    break;
            }
            
            // Format position and count for row/col operations
            if (wOp.m_Type == tStructuralOpType::InsertRow || 
                wOp.m_Type == tStructuralOpType::DeleteRow ||
                wOp.m_Type == tStructuralOpType::InsertCol ||
                wOp.m_Type == tStructuralOpType::DeleteCol) {
                wStream << " | Pos: " << wOp.m_Position << " | Count: " << wOp.m_Count;
            }
            
            // Format rect for rect operations
            if (wOp.m_Type == tStructuralOpType::InsertRect || 
                wOp.m_Type == tStructuralOpType::DeleteRect) {
                tRect wRect = wOp.m_Rect;
                wStream << " | Rect: [" << RectTop(wRect) << "," << RectLeft(wRect) 
                        << ":" << RectBottom(wRect) << "," << RectRight(wRect) << "]";
                wStream << " (H:" << RectHeight(wRect) << " W:" << RectWidth(wRect) << ")";
            }
            
            wStream << std::endl;
        }
        
        return wStream.str();
    }
    void tUndoRebaseLog::Clear() {
        std::lock_guard<std::mutex> wLock(m_Mutex);
        m_Operations.clear();
        m_SequenceCounter = 0;
    }

    tSequenceId RegisterCollaborationStructuralOp(tUndoRebaseLog& sRebaseLog, SkRoot::tUndo* sUndo, const tString& sDoRedo) {
        if (sUndo == nullptr) {
            return 0;
        }
        tUndoSpreadSheet* wUndoSpreadSheet = dynamic_cast<tUndoSpreadSheet*>(sUndo);
        if (wUndoSpreadSheet == nullptr) {
            return 0;
        }
        tSequenceId wSequenceId = 0;
        const tString wClassName = sUndo->ClassName();
        if (wClassName == "tUndoInsertRow") {
            tUndoInsertRow* wUndoInsertRow = dynamic_cast<tUndoInsertRow*>(sUndo);
            if (wUndoInsertRow == nullptr) {
                return 0;
            }
            if (wUndoInsertRow->Position() != static_cast<tIndex>(-1)) {
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::InsertRow &&
                            wDoOp->MatchesSheet(wUndoInsertRow->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::InsertRow,
                            wUndoInsertRow->Sheet()->AllocatorRef(),
                            wUndoInsertRow->Position(),
                            wUndoInsertRow->Size());
                    }
                    wSequenceId = sRebaseLog.RegisterDeleteRow(
                        wUndoInsertRow->Sheet()->AllocatorRef(),
                        wUndoInsertRow->Position(),
                        wUndoInsertRow->Size(),
                        wCancelsSequence);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterInsertRow(
                        wUndoInsertRow->Sheet()->AllocatorRef(),
                        wUndoInsertRow->Position(),
                        wUndoInsertRow->Size(),
                        0,
                        wOperationId);
                }
            } else {
                const tRect wRectToUse = wUndoInsertRow->RectRebase();
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::InsertRect &&
                            wDoOp->MatchesSheet(wUndoInsertRow->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::InsertRect,
                            wUndoInsertRow->Sheet()->AllocatorRef(),
                            wRectToUse,
                            1);
                    }
                    wSequenceId = sRebaseLog.RegisterDeleteRect(
                        wUndoInsertRow->Sheet()->AllocatorRef(), wRectToUse, wCancelsSequence);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterInsertRect(
                        wUndoInsertRow->Sheet()->AllocatorRef(), wRectToUse, 0, wOperationId);
                }
            }
        } else if (wClassName == "tUndoDeleteRow") {
            tUndoDeleteRow* wUndoDeleteRow = dynamic_cast<tUndoDeleteRow*>(sUndo);
            if (wUndoDeleteRow == nullptr) {
                return 0;
            }
            if (wUndoDeleteRow->Position() != static_cast<tIndex>(-1)) {
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::DeleteRow &&
                            wDoOp->MatchesSheet(wUndoDeleteRow->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::DeleteRow,
                            wUndoDeleteRow->Sheet()->AllocatorRef(),
                            wUndoDeleteRow->Position(),
                            wUndoDeleteRow->Size());
                    }
                    wSequenceId = sRebaseLog.RegisterInsertRow(
                        wUndoDeleteRow->Sheet()->AllocatorRef(),
                        wUndoDeleteRow->Position(),
                        wUndoDeleteRow->Size(),
                        wCancelsSequence);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterDeleteRow(
                        wUndoDeleteRow->Sheet()->AllocatorRef(),
                        wUndoDeleteRow->Position(),
                        wUndoDeleteRow->Size(),
                        0,
                        wOperationId);
                }
            } else {
                const tRect wRectToUse = wUndoDeleteRow->RectRebase();
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::DeleteRect &&
                            wDoOp->MatchesSheet(wUndoDeleteRow->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::DeleteRect,
                            wUndoDeleteRow->Sheet()->AllocatorRef(),
                            wRectToUse,
                            1);
                    }
                    wSequenceId = sRebaseLog.RegisterInsertRect(
                        wUndoDeleteRow->Sheet()->AllocatorRef(), wRectToUse, wCancelsSequence);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterDeleteRect(
                        wUndoDeleteRow->Sheet()->AllocatorRef(), wRectToUse, 0, wOperationId);
                }
            }
        } else if (wClassName == "tUndoInsertCol") {
            tUndoInsertCol* wUndoInsertCol = dynamic_cast<tUndoInsertCol*>(sUndo);
            if (wUndoInsertCol == nullptr) {
                return 0;
            }
            if (wUndoInsertCol->Position() != static_cast<tIndex>(-1)) {
                if (sDoRedo == "Undo") {
                    const tSequenceId wCancelsSequence = sRebaseLog.FindLastOperation(
                        tStructuralOpType::InsertCol,
                        wUndoInsertCol->Sheet()->AllocatorRef(),
                        wUndoInsertCol->Position(),
                        wUndoInsertCol->Size());
                    wSequenceId = sRebaseLog.RegisterDeleteCol(
                        wUndoInsertCol->Sheet()->AllocatorRef(),
                        wUndoInsertCol->Position(),
                        wUndoInsertCol->Size(),
                        wCancelsSequence);
                } else {
                    wSequenceId = sRebaseLog.RegisterInsertCol(
                        wUndoInsertCol->Sheet()->AllocatorRef(),
                        wUndoInsertCol->Position(),
                        wUndoInsertCol->Size());
                }
            } else {
                const tRect wRectToUse = wUndoInsertCol->RectRebase();
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::InsertRect &&
                            wDoOp->MatchesSheet(wUndoInsertCol->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::InsertRect,
                            wUndoInsertCol->Sheet()->AllocatorRef(),
                            wRectToUse,
                            0);
                    }
                    wSequenceId = sRebaseLog.RegisterDeleteRect(
                        wUndoInsertCol->Sheet()->AllocatorRef(), wRectToUse, wCancelsSequence, 0, false);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterInsertRect(
                        wUndoInsertCol->Sheet()->AllocatorRef(), wRectToUse, 0, wOperationId, false);
                }
            }
        } else if (wClassName == "tUndoDeleteCol") {
            tUndoDeleteCol* wUndoDeleteCol = dynamic_cast<tUndoDeleteCol*>(sUndo);
            if (wUndoDeleteCol == nullptr) {
                return 0;
            }
            if (wUndoDeleteCol->Position() != static_cast<tIndex>(-1)) {
                if (sDoRedo == "Undo") {
                    const tSequenceId wCancelsSequence = sRebaseLog.FindLastOperation(
                        tStructuralOpType::DeleteCol,
                        wUndoDeleteCol->Sheet()->AllocatorRef(),
                        wUndoDeleteCol->Position(),
                        wUndoDeleteCol->Size());
                    wSequenceId = sRebaseLog.RegisterInsertCol(
                        wUndoDeleteCol->Sheet()->AllocatorRef(),
                        wUndoDeleteCol->Position(),
                        wUndoDeleteCol->Size(),
                        wCancelsSequence);
                } else {
                    wSequenceId = sRebaseLog.RegisterDeleteCol(
                        wUndoDeleteCol->Sheet()->AllocatorRef(),
                        wUndoDeleteCol->Position(),
                        wUndoDeleteCol->Size());
                }
            } else {
                const tRect wRectToUse = wUndoDeleteCol->RectRebase();
                if (sDoRedo == "Undo") {
                    tSequenceId wCancelsSequence = 0;
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    if (wOperationId != 0) {
                        const tStructuralOp* wDoOp = sRebaseLog.FindOperationByOperationId(wOperationId);
                        if (wDoOp != nullptr &&
                            wDoOp->m_Type == tStructuralOpType::DeleteRect &&
                            wDoOp->MatchesSheet(wUndoDeleteCol->Sheet()->AllocatorRef())) {
                            wCancelsSequence = wDoOp->m_Sequence;
                        }
                    }
                    if (wCancelsSequence == 0) {
                        wCancelsSequence = sRebaseLog.FindLastOperation(
                            tStructuralOpType::DeleteRect,
                            wUndoDeleteCol->Sheet()->AllocatorRef(),
                            wRectToUse,
                            0);
                    }
                    wSequenceId = sRebaseLog.RegisterInsertRect(
                        wUndoDeleteCol->Sheet()->AllocatorRef(), wRectToUse, wCancelsSequence, 0, false);
                } else {
                    const std::uint64_t wOperationId = wUndoSpreadSheet->OperationId();
                    wSequenceId = sRebaseLog.RegisterDeleteRect(
                        wUndoDeleteCol->Sheet()->AllocatorRef(), wRectToUse, 0, wOperationId, false);
                }
            }
        }
        if (wSequenceId != 0) {
            wUndoSpreadSheet->SequenceId(wSequenceId);
        }
        return wSequenceId;
    }

} // namespace SkSpreadSheet


