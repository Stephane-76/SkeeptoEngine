//=============================================================================
// SkSpreadSheet Calculation path
/* !\mainpage Calculation path
*
* \section calculation_sec Calculation path
* 
* tPath is The tPath is anchored to the cell.
* It allows to push a cell towards the calculation when all these dependencies have been calculated
* the m_NbDepend member of tPath is used to know the number of dependent cells during path determination.
* if we have zero, we can run the calculation of the cell, for this operation push on m_StackPath of SkContainer
*/
//=============================================================================
#ifndef SkCalculationPath_hpp
#define SkCalculationPath_hpp

#include <SkApplication.hpp>
#include "SkColRowCellRange.hpp"
#include <unordered_set>
#include <vector>

using namespace SkRoot;

namespace SkSpreadSheet {

	class tPath;
	//! Allocator for ItemPath
	typedef tAllocator<tPath, tAllocatorRef, 8192> SkAllocatorItemPath;
		
	class tContainerPath;
	//! Element used to make the calculation path
	class tPath : public tClass {
	private:
		friend class tContainerPath;
		//! Number of dependant Cell
		tInt		m_NbDepend;
		//! Pointer of Cell
		tCell*		m_Cell;
		//! Next tPath
		tAllocatorRef m_Next;
		//! Previous tPath
		tAllocatorRef m_Prev;

		tBool		   m_Recovered;
	public:
		/// @brief      Constructor tPath.
		tPath();

		SkInline void Clear();
		/// @brief      Set Cell and allocator index.
		/// @param[in]	sCell tCell*
		/// @param[in]	sAllocatorIndex tInt
		SkInline void Set(tCell* sCell);

		/// @brief      Return Cell.
		/// @return		tCell*
		inline	tCell* Cell() const;

		/// @brief      Increment m_NbDepend.
		inline	void Inc();
		/// @brief      Decrement m_NbDepend.
		inline	void Dec();

		/// @brief      Erase in path.
		SkInline void Erase(SkAllocatorItemPath* sAllocator);

		/// @brief      Insert in path after sItemPath.
		/// @param[in]	sItemPath tPath*
		SkInline void Insert(SkAllocatorItemPath* sAllocator, tAllocatorRef sThis, tAllocatorRef sItemPath);

		/// @brief      Debug on console.
		void Debug() const;

		/// @brief      Return NbDepend for debug.
		/// @return  	tInt
		SkInline int NbDepend() const;

		/// @brief      Offset of member for information.
		void Offsetof() const {
#ifndef __EMSCRIPTEN__
#ifndef __APPLE__
#ifndef __GNUC__
			cout << "tPath offset................." << endl;
			cout << " m_NbDepend " << offsetof(tPath, m_NbDepend) << " :" << sizeof(tInt) << endl;
			cout << " m_Cell " << offsetof(tPath, m_Cell) << " :" << sizeof(tCell*) << endl;
			cout << " m_Next " << offsetof(tPath, m_Next) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_Prev " << offsetof(tPath, m_Prev) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_Recovered " << offsetof(tPath, m_Recovered) << " :" << sizeof(tBool) << endl;
#endif
#endif
#endif
		}

	};

	//! Stack of Path
	typedef stack<tPath*> SKStackPath;
	//! Stack of Cell
	typedef stack<tCell*> SKStackCell;

	//! Container of all path for one cell calculation
	class tContainerPath : public tClass {
	private:
		//! Root Path is the first Path ( m_RootItemPath->m_prev=nullptr)...
		tPath*				m_RootItemPath;
		tAllocatorRef		m_RootItemPathRef;

		//! Allocator Item Path
		SkAllocatorItemPath m_AllocatorItemPath;
		//! Legacy: was used by an older Reduce() stall heuristic; kept for compatibility (still reset in Calculate).
		tInt               m_LastNbItemPath;

		//! Stack of Cell with m_NbDepend !=0;
		tStackCell					m_StackRun; 
		//! For Cell with Path ready m_NbDepend == 0
		SKStackPath					m_StackPath;

		//! Formulas already re-enqueued after a spill in this pass (TEXT(C8) once, not every cycle).
		std::unordered_set<tCell*> m_SpillSlaveFormulasEnqueued;

		//! Scratch buffer for the FindRanges() call in Resolve(). Kept across cells so the
		//! reserve() inside GetIntersection stops allocating once the capacity is reached
		//! (one malloc/free per resolved cell otherwise). Only live inside Resolve().
		tColRow::tContainerRange::tResult m_ResolveRangesCovered;

		/// Cooperative blocked-subgraph pass (Kahn queue empty, remaining SUMIF/range cycles).
		/// 0 = idle, 1 = iterative InternalCalculation, 2 = teardown Resolve(skip calc).
		tInt m_BlockedPhase;
		std::vector<tCell*> m_BlockedCells;
		std::vector<tVariant> m_BlockedPrev;
		tInt m_BlockedIter;
		tInt m_BlockedIndex;

#ifdef NextPrecInc
		tAllocatorRef		m_RootItemPathRef;
#endif
		// Try to resolve remaining blocked paths by rebuilding local dependencies from VectorRef.
		// Returns true when all remaining paths were evaluated and removed.
		tBool ResolveBlockedAcyclicSubgraph();

		/// @brief Time-budgeted blocked-subgraph work for cooperative ReduceStep.
		/// @return false = yield (state kept); true = finished (graph empty or failed → SetRecursive).
		tBool StepBlockedSubgraph(tInt sMaxMs);

		void ClearBlockedSubgraphState();

		void ResetCalculationPassGuards();

	public:
		/// @brief  Constructor SkContainerPath.
		tContainerPath();

		/// @brief      Destructor SkContainerPath.
		~tContainerPath();

		/// @brief      Mark cells still present as recursive.
		void SetRecursive();

		/// @brief      Set m_RootItemPath at the begining of the list chained of SkStack.
		/// @param[in]	sPath tPath*
		SkInline void ResetRootItemPath(tPath* sPath);

		/// @brief      Remove sPath on path.
		/// @param[in]	sPath tPath*
		SkInline void Remove(tPath* sPath);

		/// @brief      Resolve: Dec dependents, optional InternalCalculation, Remove path.
		/// @param[in]	sPath tPath*
		/// @param[in]	sSkipInternalCalculation when true, skip InternalCalculation (e.g. after SCC iteration).
		void Resolve(tPath* sPath, tBool sSkipInternalCalculation);
		/// @brief      Resolve with full calculation (same as Resolve(sPath, false)).
		SkInline void Resolve(tPath* sPath);

		/// @brief      Reduce. Kahn work-queue: seed ready paths, Resolve, drain (no recursion).
		SkInline void Reduce();

		/// @brief      Count paths still queued on the linked list (progress / diagnostics).
		tInt CountPathsInList();

		/// @brief      0..100 of the cooperative blocked-subgraph pass; 0 when idle.
		tInt BlockedSubgraphProgressPercent() const;

		/// @brief      Run Reduce until the time budget is exhausted or the graph is empty.
		/// @param[in]  sMaxMs milliseconds; <= 0 means run to completion (same as Reduce).
		/// @return     true when the calculation graph is fully reduced.
		tBool ReduceStep(tInt sMaxMs);

		/// @brief      Search range recover for sCell and add the cells in path whose formulas point to these ranges.
		SkInline void RangeCovered(tPath* sPath);

		/// @brief      AllocPath on allocator item path for sCell.
		/// @param[in]	sCell tCell*
		/// @return		tPath*
		SkInline tPath* AllocPath(tCell* sCell);

        /// @brief Enqueue MatExtend slaves of sCell (they wait on the origin via SpillExtendWaitingOnOrigin).
        /// @param[in]	sCell tCell* spill origin
        SkInline void AddMatrixDepend(tCell* sCell);

        /// @brief After the origin spilled: enqueue formulas that reference a slave (TEXT(C9)), not the slaves.
        /// @param[in]	sCell tCell* spill origin
        SkInline void AddSpillSlaveFormulaDependents(tCell* sCell);
    
		/// @brief      Make path for sCell.
		/// @param[in]	sCell tCell*
		SkInline void MakePath(tCell* sCell);

		/// @brief      Calculate launch calcul for sCell.
		/// @param[in]	sCell tCell*
		void Calculate(tCell* sCell);

		// Calculate by rect

		/// @brief      Calculate Range of cell.
		/// @param[in]	sColRowCellRange tColRowCellRange*
		/// @param[in]	sRect SkRect
		void Calculate(tColRowCellRange* sColRowCellRange,tTempoRect* sRect);

		/// @brief      Calculate Select.
		/// @param[in]	sColRowCellRange tColRowCellRange*
		/// @param[in]	sSelect const tSelect&
		void Calculate(tColRowCellRange* sColRowCellRange, const tSelect& sSelect);

		/// @brief      Begin calculate for Delete Row & Col.
		void BeginCalculate();

		/// @brief      Add Cell in path.
		/// @param[in]	sCell tCell*
		
		// Used In SkUndoRedo.cpp
#ifndef __EMSCRIPTEN__
		//inline
#endif			
		/// @brief Add Cell in path.
		/// @param[in] sCell tCell*
		void Add(tCell* sCell);

		/// @brief      Add Range of cell.
		/// @param[in]	sColRowCellRange tColRowCellRange*
		/// @param[in]	sRect tTempoRect*
		void AddRect(tColRowCellRange* sColRowCellRange, tTempoRect* sRect);

		/// @brief      Add every formula cell on the sheet (sparse scan; no FindRanges recovery).
		void AddAllFormulaCells(tColRowCellRange* sColRowCellRange);

		/// @brief      Add Select.
		/// @param[in]	sColRowCellRange tColRowCellRange*
		/// @param[in]	sSelect const tSelect&
		void AddSelect(tColRowCellRange* sColRowCellRange, const tSelect& sSelect);

		/// @brief      End calculate for Delete Row & Col.
		void EndCalculate();

		/// @brief      Debug.
		void Debug() const;
	};

} // end of namespace

#endif
