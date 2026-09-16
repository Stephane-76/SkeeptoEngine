//=============================================================================
// SkCalculationPathRect
//=============================================================================
#include <SkApplication.hpp>
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCalculationPathRect.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkSpreadSheet.hpp"
#include <set>
using namespace SkRoot;

namespace SkSpreadSheet {

    tContainerPathRect::tContainerPathRect() : tClass() {
        m_CurrentRect = nullptr;
        m_ColRowCellRange = nullptr;
    }

    tContainerPathRect::~tContainerPathRect() {
    }

    void tContainerPathRect::BeginCalculate() {
        m_VectorCell.clear();
        m_VisitedCells.clear();
        m_CurrentRect = nullptr;
        m_ColRowCellRange = nullptr;
    }

    void tContainerPathRect::AddRect(tColRowCellRange* sColRowCellRange, tTempoRect* sRect) {
        m_ColRowCellRange = sColRowCellRange;
        m_CurrentRect = sRect;
        m_VisitedCells.clear(); // Clear visited set for each new rectangle
        
        // Add only cells within the rectangle (not external dependencies)
        // Process in order: first resolve dependencies, then add the cell itself
        for (tIndex wRow = sRect->Top(); wRow <= sRect->Bottom(); wRow++) {
            for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
                tCell* wCell = sColRowCellRange->Cell(wRow, wCol);
                if (wCell != nullptr) {
                    // Resolve dependencies first (adds dependencies before the cell)
                    // This ensures calculation order: dependencies are calculated before dependents
                    Resolve(wCell, true);
                }
            }
        }
    }

    void tContainerPathRect::Resolve(tCell* sCell, tBool sAddCell) {
        if (sCell == nullptr || m_CurrentRect == nullptr) {
            return;
        }
        
        // Check if cell is within rectangle bounds
        tIndex wRow = sCell->RowIndex();
        tIndex wCol = sCell->ColIndex();
        if (wRow < m_CurrentRect->Top() || wRow > m_CurrentRect->Bottom() ||
            wCol < m_CurrentRect->Col() || wCol > m_CurrentRect->Right()) {
            return; // Cell is outside rectangle, skip it
        }
        
        // Use visited set only to prevent infinite recursion during dependency resolution
        // Check visited to avoid infinite recursion in circular dependencies
        bool wWasVisited = (m_VisitedCells.find(sCell) != m_VisitedCells.end());
        
        if (!wWasVisited) {
            // Mark as visited for this resolution pass (to prevent infinite recursion)
            m_VisitedCells.insert(sCell);
            
            // First, resolve all dependencies (they will be added before this cell)
            // This ensures calculation order: dependencies are calculated before dependents
            
            // Get cell dependencies
            tItem::tContainerCell* wVectorDepend = sCell->ContainerCellDepend();
            for (tCell* wCellDepedent : *wVectorDepend->Container()) {
                if (wCellDepedent != nullptr) {
                    // Recursively resolve dependencies first (adds them before current cell)
                    Resolve(wCellDepedent, true);
                }
            }
            
            // Handle range dependencies - resolve them first too
            tColRow::tContainerRange::tResult wContainerRange;
            tColRowCellRange* wColRowCellRange = sCell->ColRowCellRange();
            wColRowCellRange->FindRanges(sCell, &wContainerRange);
            for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
                tRange* wRange = wColRowCellRange->Range(wRangeAllocatorRef);
                if (wRange != nullptr) {
                    tRange::tContainerCell* wDependCell = wRange->ContainerCellDepend();
                    for (tCell* wCellFormulaDepend : *wDependCell->Container()) {
                        if (wCellFormulaDepend != nullptr) {
                            // Recursively resolve dependencies first (adds them before current cell)
                            Resolve(wCellFormulaDepend, true);
                        }
                    }
                }
            }
            
            // Remove from visited set after resolving dependencies
            // This allows the cell to be processed again if referenced from another path
            // (which enables duplicates in the final list)
            m_VisitedCells.erase(sCell);
        }
        
        // Add the cell itself if requested (even if already visited)
        // This allows cells to appear multiple times if referenced multiple times
        if (sAddCell) {
            m_VectorCell.push_back(sCell);
        }
    }

    void tContainerPathRect::EndCalculate() {
        // EndCalculate can be used for cleanup if needed
    }

    void tContainerPathRect::Debug() {
        cout << "tContainerPathRect: " << m_VectorCell.size() << " cells to calculate" << endl;
        for (tCell* wCell : m_VectorCell) {
            if (wCell != nullptr) {
                cout << "  " << wCell->StrRef() << endl;
            }
        }
    }   

    tBool tContainerPathRect::GetListOfCalculateCell(tVectorCell& sVectorCell) {
        sVectorCell = m_VectorCell;
        return(true);
    }
}


