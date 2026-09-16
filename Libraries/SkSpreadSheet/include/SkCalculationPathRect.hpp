//=============================================================================
// SkCalculationPathRect
//=============================================================================
#ifndef SkCalculationPathRect_hpp
#define SkCalculationPathRect_hpp

#include <SkApplication.hpp>
#include "SkColRowCellRange.hpp"
#include <set>

using namespace SkRoot;

namespace SkSpreadSheet {

    class tContainerPathRect : public tClass {
    private:
        //! Vector to store cells that need to be calculated (in calculation order, with duplicates allowed)
        tVectorCell m_VectorCell;
        //! Current rectangle being processed
        tTempoRect* m_CurrentRect;
        //! Reference to ColRowCellRange for cell access
        tColRowCellRange* m_ColRowCellRange;
        //! Set to track visited cells during dependency resolution (to avoid infinite recursion)
        std::set<tCell*> m_VisitedCells;
        
        /// @brief      Resolve dependencies for a cell and add them to the list in calculation order
        /// @param[in]  sCell tCell* Cell to resolve
        /// @param[in]  sAddCell tBool If true, add the cell itself after its dependencies
        void Resolve(tCell* sCell, tBool sAddCell = true);
        
    public:
        tContainerPathRect();
        ~tContainerPathRect();

        void BeginCalculate();
        void AddRect(tColRowCellRange* sColRowCellRange, tTempoRect* sRect);
        void EndCalculate();
        
        tBool GetListOfCalculateCell(tVectorCell& sVectorCell);
        void Debug();
    };
} // end of name Space

#endif
