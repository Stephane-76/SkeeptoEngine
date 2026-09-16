#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#include <SkApplication.hpp>
#include "SkTools.hpp"

using namespace SkRoot;

namespace SkRoot {
class tUndo;
}

namespace SkSpreadSheet {

// Forward declaration
class tUndoSpreadSheet;

// Rebase =================================================================
using tSequenceId = std::uint64_t;

/// @brief Structural operation types for undo/redo rebase
enum class tStructuralOpType {
    InsertRow,
    DeleteRow,
    InsertCol,
    DeleteCol,
    InsertRect,
    DeleteRect
};

/// @brief Represents a structural operation (insert/delete row/col/rect) for rebase tracking
struct tStructuralOp {
    tSequenceId m_Sequence {0};
    tStructuralOpType m_Type {tStructuralOpType::InsertRow};
    tAllocatorRef m_SheetAllocator {tAllocatorRef(-1)};
    tIndex m_Position {0};
    tIndex m_Count {0};
    tRect m_Rect;
    /// When type is InsertRect/DeleteRect: true = row insert/delete by rect, false = column insert/delete by rect.
    bool m_DoesRow {true};
    tSequenceId m_CancelsSequence {0}; // Sequence ID of the operation this cancels (0 = none)
    std::uint64_t m_OperationId {0}; // Unique operation ID to find corresponding Do operation for an Undo

    /// @brief Check if this operation matches the given sheet allocator
    /// @param[in] sSheeetAllocator Sheet allocator to check
    /// @return true if the operation matches the sheet
    [[nodiscard]] bool MatchesSheet(tAllocatorRef sSheeetAllocator) const;
};

/// @brief Plan containing operations to rebase coordinates between two sequence IDs
struct tRebasePlan {
    tAllocatorRef m_SheetAllocator {0};
    tSequenceId m_FromSequence {0};
    tSequenceId m_ToSequence {0};
    std::vector<tStructuralOp> m_Operations;

    /// @brief Rebase a row coordinate according to the operations in this plan
    /// @param[in] sRow Row index to rebase
    /// @return Rebased row index, or nullopt if the row was deleted
    [[nodiscard]] std::optional<tIndex> RebaseRow(tIndex sRow) const;
    
    /// @brief Rebase a column coordinate according to the operations in this plan
    /// @param[in] sCol Column index to rebase
    /// @return Rebased column index, or nullopt if the column was deleted
    [[nodiscard]] std::optional<tIndex> RebaseCol(tIndex sCol) const;
    
    /// @brief Rebase a rectangle according to the operations in this plan
    /// @param[in] sRect Rectangle to rebase
    /// @return Rebased rectangle, or nullopt if the rectangle was deleted or invalid
    [[nodiscard]] std::optional<tRect> RebaseRect(const tRect& sRect) const;
    
    /// @brief Rebase a point according to the operations in this plan
    /// @param[in] sPoint Point to rebase
    /// @return Rebased point, or nullopt if the point was deleted
    [[nodiscard]] std::optional<tPoint> RebasePoint(const tPoint& sPoint) const;
    
    /// @brief Check if the rebase plan is empty
    /// @return true if there are no operations in the plan
    tBool IsEmpty();
    
    /// @brief Clear the rebase plan
    void Clear();
};

/// @brief Log of structural operations for rebasing undo/redo coordinates
class tUndoRebaseLog {
public:
    /// @brief Register an insert row operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sPosition Row position where rows are inserted
    /// @param[in] sCount Number of rows inserted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterInsertRow(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0);
    
    /// @brief Register a delete row operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sPosition Row position where rows are deleted
    /// @param[in] sCount Number of rows deleted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterDeleteRow(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0);
    
    /// @brief Register an insert column operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sPosition Column position where columns are inserted
    /// @param[in] sCount Number of columns inserted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterInsertCol(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0);
    
    /// @brief Register a delete column operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sPosition Column position where columns are deleted
    /// @param[in] sCount Number of columns deleted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterDeleteCol(tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0);
    
    /// @brief Register an insert rectangle operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sRect Rectangle that was inserted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterInsertRect(tAllocatorRef sSheetAllocator, const tRect& sRect, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0, bool sDoesRow = true);
    
    /// @brief Register a delete rectangle operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sRect Rectangle that was deleted
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @param[in] sDoesRow true for row delete-by-rect, false for column delete-by-rect
    /// @return Sequence ID assigned to this operation
    tSequenceId RegisterDeleteRect(tAllocatorRef sSheetAllocator, const tRect& sRect, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0, bool sDoesRow = true);

    /// @brief Get the latest sequence ID
    /// @return Latest sequence ID
    [[nodiscard]] tSequenceId LatestSequence() const;
    
    /// @brief Get the next sequence ID and increment the counter
    /// @return Next sequence ID
    [[nodiscard]] tSequenceId NextSequence();
    
    /// @brief Collect operations between two sequence IDs
    /// @param[in] sFromExclusive Start sequence ID (exclusive)
    /// @param[in] sToInclusive End sequence ID (inclusive)
    /// @param[in] sAllocator Sheet allocator to filter by (use kInvalidIndex for all sheets)
    /// @return Vector of structural operations in the specified range
    [[nodiscard]] std::vector<tStructuralOp> Collect(tSequenceId sFromExclusive, tSequenceId sToInclusive, tAllocatorRef sAllocator) const;
    
    /// @brief Find the last operation of a specific type matching the given criteria
    /// @param[in] sType Type of operation to find
    /// @param[in] sSheetAllocator Sheet allocator to filter by
    /// @param[in] sPosition Position to match (or -1 to ignore)
    /// @param[in] sCount Count to match (or -1 to ignore)
    /// @return Sequence ID of the found operation, or 0 if not found
    [[nodiscard]] tSequenceId FindLastOperation(tStructuralOpType sType, tAllocatorRef sSheetAllocator, tIndex sPosition = static_cast<tIndex>(-1), tIndex sCount = static_cast<tIndex>(-1)) const;
    
    /// @brief Find the last operation of a specific type matching the given rectangle
    /// @param[in] sType Type of operation to find (InsertRect or DeleteRect)
    /// @param[in] sSheetAllocator Sheet allocator to filter by
    /// @param[in] sRect Rectangle to match
    /// @param[in] sDoesRow Row/col discriminant filter: 1 = row rect, 0 = column rect,
    ///            -1 = ignore (any). Prevents a column op from matching a row rect that
    ///            shares the same rectangle (both InsertRect with identical bounds).
    /// @return Sequence ID of the found operation, or 0 if not found
    [[nodiscard]] tSequenceId FindLastOperation(tStructuralOpType sType, tAllocatorRef sSheetAllocator, const tRect& sRect, tInt sDoesRow = -1) const;
    
    /// @brief Find an operation by its unique OperationId
    /// @param[in] sOperationId Unique operation ID to find
    /// @return Pointer to the operation if found, nullptr otherwise
    [[nodiscard]] const tStructuralOp* FindOperationByOperationId(std::uint64_t sOperationId) const;
    
    /// @brief Build a rebase plan for operations between two sequence IDs
    /// @param[in] sFromExclusive Start sequence ID (exclusive)
    /// @param[in] sToInclusive End sequence ID (inclusive)
    /// @param[in] sSheetAllocator Sheet allocator to filter by (use kInvalidIndex for all sheets)
    /// @return Rebase plan containing operations to apply
    [[nodiscard]] tRebasePlan BuildPlan(tSequenceId sFromExclusive, tSequenceId sToInclusive, tAllocatorRef sSheetAllocator) const;
    
    /// @brief Build a rebase plan for another sheet based on an existing rebase plan
    /// @param[in] sRebasePlan Existing rebase plan
    /// @param[in] sSheetAllocator Sheet allocator for the new plan
    /// @return Rebase plan for the specified sheet
    [[nodiscard]] tRebasePlan BuildPlanAnotherSheet(const tRebasePlan& sRebasePlan, tAllocatorRef sSheetAllocator) const;
  
    /// @brief Debug method to return a string representation of all operations
    /// @return String containing debug information about all operations
    [[nodiscard]] tString DebugOperations() const;
    
    /// @brief Debug method to return a JSON string representation of all operations
    /// @return JSON string containing debug information about all operations
    [[nodiscard]] tString JsonDebugOperations() const;
    
    /// @brief Debug method to return a string representation of operations performed since an Undo was created
    /// @param[in] sUndo Undo object to check operations for
    /// @return String containing debug information about operations since undo creation
    [[nodiscard]] tString DebugUndoOperations(tUndoSpreadSheet* sUndo) const;

public:
    /// @brief Constructor
    tUndoRebaseLog();
    
    /// @brief Copy constructor for allocator compatibility
    /// @param[in] sOther Other rebase log to copy from
    tUndoRebaseLog(const tUndoRebaseLog& sOther);
    
    /// @brief Assignment operator for allocator compatibility
    /// @param[in] sOther Other rebase log to assign from
    /// @return Reference to this object
    tUndoRebaseLog& operator=(const tUndoRebaseLog& sOther);

    /// @brief Register a structural operation (internal method)
    /// @param[in] sType Type of structural operation
    /// @param[in] sSheetAllocator Sheet allocator reference
    /// @param[in] sPosition Position of the operation
    /// @param[in] sCount Count for row/col operations
    /// @param[in] sRect Rectangle for rect operations
    /// @param[in] sIsRect true if this is a rect operation
    /// @param[in] sCancelsSequence Optional sequence ID of the operation this cancels (0 = none)
    /// @param[in] sOperationId Optional unique operation ID to find corresponding Do operation (0 = none)
    /// @return Sequence ID assigned to this operation
    tSequenceId Register(tStructuralOpType sType, tAllocatorRef sSheetAllocator, tIndex sPosition, tIndex sCount, const tRect& sRect, bool sIsRect, tSequenceId sCancelsSequence = 0, std::uint64_t sOperationId = 0, bool sDoesRow = true);

    /// @brief Clear all operations and reset the sequence counter
    void Clear();
private:
    mutable std::mutex m_Mutex;
    tSequenceId m_SequenceCounter;
    std::vector<tStructuralOp> m_Operations;
};

/// Register insert/delete row/col in the collaboration rebase log (local Do/Undo/Redo or GetMessage).
/// @return New sequence id when an operation was registered, otherwise 0.
tSequenceId RegisterCollaborationStructuralOp(tUndoRebaseLog& sLog, SkRoot::tUndo* sUndo, const tString& sDoRedo);

} // namespace SkSpreadSheet


