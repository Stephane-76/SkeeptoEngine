//=============================================================================
// SkSpreadSheet Calculation path
//=============================================================================
#include <thread>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <functional>
#include <chrono>

#include "../include/SkCalculationPath.hpp"
#include "../include/SkSpreadSheet.hpp"

#define _debug_calculation
#define _debug_calculation_covered
#define _debug_calculation_recursive
#define _debug_topo_blocked


// Calculation traces: opt in with -Ddebug_calculation, -Ddebug_calculation_covered, -Ddebug_topo_blocked, etc.
#ifndef __EMSCRIPTEN__
#define thread_calculation
#endif

namespace SkSpreadSheet {

#ifdef sk_calc_order_trace
namespace {
	tInt gSkCalcOrderSeq = 0;
}
#endif

// Reset at each calculation batch; one line per InternalCalculation when sk_calc_order_trace is set at compile time.
#ifdef sk_calc_order_trace
#define SK_CALC_ORDER_RESET() do { gSkCalcOrderSeq = 0; } while (0)
#define SK_CALC_ORDER_CELL(c_ptr)                                                                                    \
	do {                                                                                                               \
		tCell* _skc = (c_ptr);                                                                                         \
		if ((_skc) != nullptr && (_skc)->Formula() != nullptr) {                                                       \
			++gSkCalcOrderSeq;                                                                                         \
			cout << "[SK_CALC_ORDER]\t" << gSkCalcOrderSeq << '\t' << (_skc)->Sheet()->Name() << "!" << (_skc)->StrRef() \
				 << '\n';                                                                                               \
		}                                                                                                              \
	} while (0)
#else
#define SK_CALC_ORDER_RESET() ((void)0)
#define SK_CALC_ORDER_CELL(c_ptr) ((void)0)
#endif

	// Defined later in this file (before Reduce); needed by ResolveBlockedAcyclicSubgraph.
	tBool SpillExtendWaitingOnOrigin(tPath* sPath);

	namespace {
	tBool PathEntryStillLive(tPath* sPath, SkAllocatorItemPath& sAllocator) {
		if (sPath == nullptr) {
			return false;
		}
		tCell* wCell = sPath->Cell();
		if (wCell == nullptr) {
			return false;
		}
		const tAllocatorRef wRef = wCell->Path();
		if (wRef == 0) {
			return false;
		}
		return sAllocator(wRef) == sPath;
	}
	} // namespace

	static tBool VariantEqualsForIteration(const tVariant& sA, const tVariant& sB) {
		if (sA.Type() != sB.Type()) {
			return(false);
		}
		switch (sA.Type()) {
			case tVariantType::t_null:
				return(true);
			case tVariantType::t_int:
				return(sA.Int() == sB.Int());
			case tVariantType::t_bool:
				return(sA.Bool() == sB.Bool());
			case tVariantType::t_double:
				return(std::fabs(sA.Double() - sB.Double()) <= 1e-12);
			case tVariantType::t_string:
				return(sA.String() == sB.String());
			case tVariantType::t_error:
				return(sA.Error().CodeInt() == sB.Error().CodeInt() && sA.Error().String() == sB.Error().String());
			case tVariantType::t_date:
				return(sA.Date() == sB.Date());
			case tVariantType::t_class:
				// Class values are mutable and heterogeneous; pointer identity is enough for convergence guard.
				return(sA.Class() == sB.Class());
		}
		return(false);
	}

	// Divergence guard for the SCC iteration loop: a true circular reference on additive aggregation
	// (e.g. D7=SUM(DUPONT) where DUPONT covers F6=SUM(ALIAN) where ALIAN covers D7) produces values
	// growing without bound across iterations. Without this guard, 40 iterations silently finish with
	// huge/overflowed values, or a transient nullptr in VectorRef makes Eval emit a spurious #REF!.
	// Detect non-finite doubles and magnitudes too large to contribute to any meaningful convergence.
	static tBool VariantIsDivergent(const tVariant& sValue) {
		constexpr double wDivergenceThreshold = 1.0e15;
		switch (sValue.Type()) {
			case tVariantType::t_double: {
				const double wD = sValue.Double();
				return(!std::isfinite(wD) || std::fabs(wD) > wDivergenceThreshold);
			}
			case tVariantType::t_int: {
				const double wI = static_cast<double>(sValue.Int());
				return(std::fabs(wI) > wDivergenceThreshold);
			}
			default:
				return(false);
		}
	}
	// Element used to calculate the calculation path
	tPath::tPath() : tClass(), m_NbDepend(0), m_Cell(nullptr), m_Next(0), m_Prev(0), m_Recovered(false) {}

	void tPath::Clear() {
#ifdef _DEBUGSK
		m_NbDepend = 0;
		m_Prev = 0;
		m_Next = 0;
		m_Cell = nullptr;
#endif
	}

	void tPath::Set(tCell* sCell) { m_Cell = sCell; }


	tCell* tPath::Cell() const {
		return(m_Cell);
	}

	void tPath::Inc() { m_NbDepend++; }

	void tPath::Dec() { m_NbDepend--; }

	void tPath::Erase(SkAllocatorItemPath* sAllocator) {
		if (m_Prev != 0) {
			tPath* wPrev = (*sAllocator)(m_Prev);
			if (wPrev != nullptr) {
				wPrev->m_Next = m_Next;
			}
		}
		if (m_Next != 0) {
			tPath* wNext = (*sAllocator)(m_Next);
			if (wNext != nullptr) {
				wNext->m_Prev = m_Prev;
			}
		}
	}

	void tPath::Insert(SkAllocatorItemPath* sAllocator, tAllocatorRef sThis, tAllocatorRef sItemPath) {
		(*sAllocator)(sItemPath)->m_Prev = sThis;

		if (m_Next != 0) {
			(*sAllocator)(m_Next)->m_Prev = sItemPath;
		}
		(*sAllocator)(sItemPath)->m_Next = m_Next;
		m_Next = sItemPath;
	}

	void tPath::Debug() const {
		if (Cell() == nullptr) {
			cout << "NULL!! ";
			return;
		}
		cout << Cell()->StrRef() << ":" << m_NbDepend << " ";
	}

	tInt tPath::NbDepend() const { return(m_NbDepend); }

	//=========================================================================
	void tContainerPath::ClearBlockedSubgraphState() {
		m_BlockedPhase = 0;
		m_BlockedCells.clear();
		m_BlockedPrev.clear();
		m_BlockedIter = 0;
		m_BlockedIndex = 0;
	}

	void tContainerPath::ResetCalculationPassGuards() {
		m_SpillSlaveFormulasEnqueued.clear();
		ClearBlockedSubgraphState();
		tWorkBook* wWb = tSpreadSheetContainer::Instance()->ActiveWorkBook();
		if (wWb != nullptr) {
			wWb->ClearNamedCallerEvalCache();
		}
	}

	tContainerPath::tContainerPath() : tClass(), m_RootItemPath(nullptr), 
		m_RootItemPathRef(0),
		m_LastNbItemPath(0),
		m_BlockedPhase(0),
		m_BlockedIter(0),
		m_BlockedIndex(0) {
	}
	tContainerPath::~tContainerPath() {
	}

	void tContainerPath::SetRecursive() {
		tPath* wItemPath = m_RootItemPath;
#ifdef debug_calculation_recursive
		Debug();
		cout << "SetRecursive ------------------" << endl;
#endif
		while (wItemPath != nullptr) {
#ifdef debug_calculation_recursive
			cout << " " << wItemPath->Cell()->StrRef(true) << "=" <<  wItemPath->Cell()->FormulaStr() << ":" << wItemPath->m_NbDepend << endl;
#endif
			tCell* wCell = wItemPath->Cell();
			wCell->Value(tVariant(tClassError(tTypeError::t_recursive, "")));
			wCell->Path(0);
			// not free in allocator
			wItemPath = m_AllocatorItemPath(wItemPath->m_Next);
		}
#ifdef debug_calculation_recursive
		cout << "-------------------------------" << endl;
#endif
	}

	void tContainerPath::ResetRootItemPath(tPath* sPath) {
		if (sPath->m_Prev != 0) {
			m_RootItemPath = m_AllocatorItemPath(sPath->m_Prev);
			m_RootItemPathRef = sPath->m_Prev;

			m_RootItemPath->m_Next = sPath->m_Next;
			while (m_RootItemPath->m_Prev != 0) {
				m_RootItemPath = m_AllocatorItemPath(m_RootItemPath->m_Prev);
				m_RootItemPathRef = m_RootItemPath->m_Prev;
			}
		}
		else {
			if (sPath->m_Next != 0) {
				m_RootItemPath = m_AllocatorItemPath(sPath->m_Next);
				m_RootItemPathRef = sPath->m_Next;
			}
			else {
				m_RootItemPathRef = 0;
				m_RootItemPath = nullptr;
			}
		}
	}

	void tContainerPath::Remove(tPath* sPath) {
#ifdef debug_calculation
		if (sPath->Cell() != nullptr) {
			cout << "***********Remove " << sPath->Cell()->StrRef() << " Nb " << sPath->m_NbDepend << endl;
			Debug();
		}
#endif

		// If m_RootItemPath change 
		if (sPath == m_RootItemPath) {
			ResetRootItemPath(sPath);
		}
		sPath->Erase(&m_AllocatorItemPath);
#ifdef debug_calculation
		cout << "***********After " << endl;
		Debug();
#endif
	}

	void tContainerPath::Resolve(tPath* sPath) {
		Resolve(sPath, false);
	}

	void tContainerPath::Resolve(tPath* sPath, tBool sSkipInternalCalculation) {
#ifdef debug_calculation
		if (sPath->Cell() != nullptr) {
			cout << "Resolve " << sPath->Cell()->StrRef() << " Nb " << sPath->m_NbDepend << endl;
		}
#endif 
		tCell* wCell = sPath->Cell();
#ifdef diagcalc
		// Diagnostic for WASM "memory access out of bounds" on Budget.sker (Apr 2026): the trap loses any
		// stack info under Node, so emit each cell as we resolve so the last printed line points at the
		// crashing cell. flush() ensures the line reaches stderr before the trap aborts the module.
		if (wCell != nullptr) {
			std::cerr << "[diag] Resolve cell=" << wCell->StrRef(true)
			          << " hasFormula=" << (wCell->Formula() != nullptr ? "Y" : "N")
			          << " formulaStr=\"" << wCell->FormulaStr() << "\"" << std::endl;
			std::cerr.flush();
		}
#endif
		if (wCell == nullptr || !PathEntryStillLive(sPath, m_AllocatorItemPath)) {
			return;
		}
		if (wCell != nullptr) {
#ifdef _DEBUGSK
            if ((wCell->Row()==nullptr) || (wCell->Col()==nullptr)) cout << wCell->StrRef() << endl;
            assert(wCell->Row()!=nullptr);
            assert(wCell->Col()!=nullptr);
#endif
			// 1 Dec cell dependent ===========================================
			// NOTE: Don't cache cell dependencies - they can change during calculation
			// as formulas are evaluated and new dependencies are discovered
			tItem::tContainerCell* wVectorDepend = wCell->ContainerCellDepend();
			for (tCell* wCellDepedent : *wVectorDepend->Container()) {
				if (wCellDepedent->Path() != 0) {
					tPath* wPath = m_AllocatorItemPath(wCellDepedent->Path());
#ifdef debug_calculation
					cout << "Dec  " <<  wPath->Cell()->StrRef() << " Nb " << wPath->m_NbDepend << endl;
					Debug();
#endif
                    if (wPath!=nullptr) {
                        wPath->Dec();
                        // if wPath->m_NbDepend ==0, Ok for Calculate
                        if (wPath->m_NbDepend == 0) {
                            m_StackPath.push(wPath);
                        }
                    }
				}
			}

#ifdef debug_calculation_covered
			tStringStream wStream;
			wStream  << wCell->StrRef() << " Resolve range covered ->";
#endif

			// 2 Dec cell range dependent  ======================================
			// Cache disabled - ranges can become stale during calculation
			// when cells are recalculated and their dependencies change.
			// The ranges returned by FindRanges() are correct at call time,
			// but caching them can lead to using stale data when dependencies update.
			// Reused buffer: FindRanges() clears it, and nothing between here and the loop
			// below re-enters Resolve(). Avoids a malloc/free per resolved cell.
			tColRow::tContainerRange::tResult& wContainerRange = m_ResolveRangesCovered;
			tColRowCellRange* wColRowCellRange = wCell->ColRowCellRange();
			wColRowCellRange->FindRanges(wCell, &wContainerRange);
			for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
				tRange* wRange = wColRowCellRange->Range(wRangeAllocatorRef);
                    if (wRange!=nullptr) {
#ifdef debug_calculation_covered
                        wStream << ", " << wRange->StrRef();
#endif
                        tRange::tContainerCell* wDependCell = wRange->ContainerCellDepend();
                        // Loop an dependent Cell (with formula point on this range).
                        for (tCell* wCellFormulaDepend : *wDependCell->Container()) {
                                // Symmetric with RangeCovered(): a cell that aggregates a range containing
                                // itself never incremented its own m_NbDepend, so it must not decrement it
                                // here either (would underflow the count).
                                if (wCellFormulaDepend == wCell) {
                                    continue;
                                }
                                // Check if path exists before accessing it
                                // Path may have been removed by another Resolve() call
                                if (wCellFormulaDepend->Path() == 0) {
                                    // Path was removed, skip this cell
                                    continue;
                                }
                                
                                tPath* wPath = m_AllocatorItemPath(wCellFormulaDepend->Path());
                                if (wPath != nullptr) {
#ifdef debug_calculation
                                    cout << " Dec  " << wCell->StrRef() << " Nb " << wPath->m_NbDepend;
#endif
                                    wPath->Dec();
                                    if (wPath->m_NbDepend == 0) {
                                        m_StackPath.push(wPath);
                                    }
                                } else {
                                    // Path was removed between check and access - this can happen in multi-threaded scenarios
                                    // or when paths are being removed concurrently. Skip this cell.
#ifdef debug_calculation
                                    cout << "Warning: Path for cell " << wCellFormulaDepend->StrRef(true) << " was removed, skipping" << endl;
#endif
                                    continue;
                                }
                            } // Loop FormulaDependent
                        } // End loop CellDependent
                        } //End wRange!=nullptr
                        
        #ifdef debug_calculation_covered
                    cout << wStream.str() << endl;
        #endif
                 
         

			// AFter Internal Calculation apply Cell->Path(0);
			tPath* wCellPath = m_AllocatorItemPath(wCell->Path());
			if (wCellPath == nullptr || wCellPath != sPath) {
				return;
			}
			
			// Calculate Cell =================================================
			if (!sSkipInternalCalculation) {
#ifdef debug_calculation
				cout << "----------------Calculate ->" << wCell->StrRef() << endl;
#endif
				SK_CALC_ORDER_CELL(wCell);
				wCell->InternalCalculation();
				// Do not Add() slaves here: after ClearPersistedSpillSlaves they are not MatExtend,
				// so they resolve empty before the origin and RangeCovered Inc's the week rows
				// (Calendar RecalculateAll → column B only). Enqueue the formulas that read
				// those slaves (TEXT(C9), LEFT(C9)) now that the spill values exist.
				if (wCell->IsMatOrigin() || wCell->IsSpillRange()) {
					AddSpillSlaveFormulaDependents(wCell);
				}
			}

			// Delete CellPath on path
			// Save path reference before Remove() which may modify wCell->Path()
			tAllocatorRef wPathRef = wCell->Path();
			Remove(wCellPath);
#ifdef debug_calculation
			cout << "Delete path in allocator ->" << wPathRef << " " << wCell->StrRef();
#endif
			// Allocator ref 0 is invalid; Delete(0) can corrupt the path pool.
			if (wPathRef != 0) {
				m_AllocatorItemPath.Delete(wPathRef);
			}

			// Cell available for the next path calculation ===================
			// very important
			wCell->Path(0);
		} // end of loop of recovered range
		
	}

	tBool tContainerPath::ResolveBlockedAcyclicSubgraph() {
		// Match Reduce(): after Resolve(), dependents with m_NbDepend==0 are pushed on m_StackPath and must be resolved too.
		auto wDrainStackAfterResolve = [this]() {
			while (!m_StackPath.empty()) {
				tPath* wPath = m_StackPath.top();
				m_StackPath.pop();
				if (wPath == nullptr || wPath->m_NbDepend != 0 || SpillExtendWaitingOnOrigin(wPath)) {
					continue;
				}
				if (!PathEntryStillLive(wPath, m_AllocatorItemPath)) {
					continue;
				}
				tCell* wCell = wPath->Cell();
				if (wCell == nullptr) {
					continue;
				}
				Resolve(wPath);
			}
		};

		std::vector<tCell*> wCells;
		wCells.reserve(256);
		for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
			tCell* wCell = wP->Cell();
			if (wCell == nullptr) {
				continue;
			}
			wCells.push_back(wCell);
		}
		const tInt wN = static_cast<tInt>(wCells.size());
		if (wN <= 0) {
			return(false);
		}
		tBool wAllPathBlockedAtEntry = true;
		for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
			if (wP->m_NbDepend == 0) {
				wAllPathBlockedAtEntry = false;
				break;
			}
		}
#ifdef debug_topo_blocked
		cout << "[topo_blocked] topo_nodes=" << wN << endl;
#endif

		std::unordered_map<tCell*, tInt> wIndexByCell;
		wIndexByCell.reserve(static_cast<tSize>(wN * 2));
		for (tInt i = 0; i < wN; ++i) {
			wIndexByCell[wCells[i]] = i;
		}

		std::vector<std::vector<tInt>> wAdj(static_cast<tSize>(wN));
		std::vector<tInt> wIndeg(static_cast<tSize>(wN), 0);
		// O(1) edge dedup: a linear std::find over wAdj[wTo] is O(degree) per insert and becomes
		// O(degree^2) on hub cells referenced by thousands of formulas (main freeze on large books).
		std::vector<std::unordered_set<tInt>> wAdjSeen(static_cast<tSize>(wN));
		auto wAddEdge = [&](tInt sFrom, tCell* sToCell) {
			if (sToCell == nullptr) return;
			auto it = wIndexByCell.find(sToCell);
			if (it == wIndexByCell.end()) return;
			const tInt wTo = it->second;
			if (wTo == sFrom) return;
			if (wAdjSeen[static_cast<tSize>(wTo)].insert(sFrom).second) {
				wAdj[static_cast<tSize>(wTo)].push_back(sFrom);
				wIndeg[static_cast<tSize>(sFrom)]++;
			}
		};
		auto wAddRangeEdges = [&](tInt sFrom, tCell* sFormulaCell, tRange* sRange) {
			if (sRange == nullptr) return;
			if (sRange->IsCell()) {
				wAddEdge(sFrom, sRange->Cell());
				return;
			}
			// For blocked-graph fallback only: avoid expanding huge ranges into all cells.
			tColRowCellRange* wCr = sRange->ColRowCellRange();
			if (wCr == nullptr) {
				return;
			}
			const tIndex wTop = sRange->TopIndex();
			const tIndex wBottom = sRange->BottomIndex();
			const tIndex wLeft = sRange->LeftIndex();
			const tIndex wRight = sRange->RightIndex();
			// Single-column vector ranges (SUMIF G:G / V:V): one implicit-intersection edge per ref.
			// Do not apply row intersection on C6:F6-style bands — D7=SUM(DUPONT) must see F6 on the path.
			if (sFormulaCell != nullptr && wLeft == wRight) {
				const tIndex wRow = sFormulaCell->RowIndex();
				tIndex wTargetRow = wRow;
				if (wTargetRow < wTop) wTargetRow = wTop;
				if (wTargetRow > wBottom) wTargetRow = wBottom;
				wAddEdge(sFrom, wCr->Cell(wTargetRow, wLeft));
				return;
			}
			// 2-D ranges: any blocked-path cell inside the range is a real dependency edge
			// (e.g. SUM across C6:F6 when F6 is also on the path).
			// Cost guard: scanning every blocked node per range ref is O(N) and, summed over all range
			// refs, O(N^2) — ~5 s of pure adjacency build on a 4000-cell blocked graph. Iterate whichever
			// is smaller: the range rectangle area (typical bands like C6:F6 are tiny) or the blocked set.
			// Both paths add exactly the same edges (each blocked cell inside the rect on the range sheet).
			tSheet* wRangeSheet = wCr->Sheet();
			tBool wAddedFromPath = false;
			const tLongLong wRangeArea = (static_cast<tLongLong>(wBottom) - static_cast<tLongLong>(wTop) + 1)
									* (static_cast<tLongLong>(wRight) - static_cast<tLongLong>(wLeft) + 1);
			if (wRangeArea > 0 && wRangeArea <= static_cast<tLongLong>(wN)) {
				for (tIndex wR = wTop; wR <= wBottom; ++wR) {
					for (tIndex wC = wLeft; wC <= wRight; ++wC) {
						tCell* wCandidate = wCr->Cell(wR, wC);
						if (wCandidate == nullptr) continue;
						if (wIndexByCell.find(wCandidate) == wIndexByCell.end()) continue;
						wAddEdge(sFrom, wCandidate);
						wAddedFromPath = true;
					}
				}
			} else {
				for (const auto& wPair : wIndexByCell) {
					tCell* wCandidate = wPair.first;
					if (wCandidate == nullptr) continue;
					if (wRangeSheet != nullptr && wCandidate->Sheet() != wRangeSheet) continue;
					const tIndex wCandRow = wCandidate->RowIndex();
					const tIndex wCandCol = wCandidate->ColIndex();
					if (wCandRow < wTop || wCandRow > wBottom) continue;
					if (wCandCol < wLeft || wCandCol > wRight) continue;
					wAddEdge(sFrom, wCandidate);
					wAddedFromPath = true;
				}
			}
			if (wAddedFromPath) {
				return;
			}
			if (sFormulaCell == nullptr) {
				return;
			}
			// 2-D range fallback: top-left cell only.
			wAddEdge(sFrom, wCr->Cell(wTop, wLeft));
		};

		for (tInt i = 0; i < wN; ++i) {
			tCell* wCell = wCells[static_cast<tSize>(i)];
			const tVectorItem* wRefs = wCell->VectorRef();
			if (wRefs == nullptr) {
				continue;
			}
			for (tItem* wRefItem : *wRefs) {
				if (wRefItem == nullptr) continue;
				tCell* wRefCell = wRefItem->Cell();
				if (wRefCell != nullptr) {
					wAddEdge(i, wRefCell);
				}
				tRange* wRefRange = wRefItem->Range();
				if (wRefRange != nullptr) {
					wAddRangeEdges(i, wCell, wRefRange);
				}
			}
		}

		std::queue<tInt> wQueue;
		for (tInt i = 0; i < wN; ++i) {
			if (wIndeg[static_cast<tSize>(i)] == 0) {
				wQueue.push(i);
			}
		}
		std::vector<tInt> wOrder;
		wOrder.reserve(static_cast<tSize>(wN));
		while (!wQueue.empty()) {
			const tInt wU = wQueue.front();
			wQueue.pop();
			wOrder.push_back(wU);
			for (tInt wV : wAdj[static_cast<tSize>(wU)]) {
				tInt& wDeg = wIndeg[static_cast<tSize>(wV)];
				wDeg--;
				if (wDeg == 0) {
					wQueue.push(wV);
				}
			}
		}
		const tInt wTopoOrderSize = static_cast<tInt>(wOrder.size());
		// Small fully-blocked subgraphs with a full Kahn order are under-connected (e.g. D7=SUM(DUPONT)
		// vs F6=SUM(ALIAN): topo sees F6->D7 only, numeric result instead of #RECURSIVE). Large books
		// (Ref SUMIF columns) still use the fast topo path when every node had m_NbDepend>0 but edges
		// are correct after single-column implicit intersection.
		constexpr tInt Cst_MaxForceSccBlockedNodes = 64;
		const tBool wForceSccDespiteTopo =
			(wTopoOrderSize == wN) && wAllPathBlockedAtEntry && wN > 1
			&& wN <= Cst_MaxForceSccBlockedNodes;
		if (wTopoOrderSize != wN || wForceSccDespiteTopo) {
			if (wForceSccDespiteTopo) {
#ifdef debug_topo_blocked
				cout << "[topo_blocked] topo_order=" << wTopoOrderSize << " force_scc=1" << endl;
#endif
			} else {
#ifdef debug_topo_blocked
			cout << "[topo_blocked] topo_order=" << wTopoOrderSize << " topo_success=0" << endl;
			tInt wPrinted = 0;
			for (tInt i = 0; i < wN && wPrinted < 20; ++i) {
				if (wIndeg[static_cast<tSize>(i)] <= 0) {
					continue;
				}
				tCell* wCell = wCells[static_cast<tSize>(i)];
				if (wCell == nullptr) {
					continue;
				}
				const tVectorItem* wRefs = wCell->VectorRef();
				const tInt wRefCount = (wRefs != nullptr) ? static_cast<tInt>(wRefs->size()) : 0;
				cout << "[topo_blocked] indeg_cell " << wCell->StrRef(true)
					 << " indegree=" << wIndeg[static_cast<tSize>(i)]
					 << " refs=" << wRefCount << endl;
				tInt wRefPrinted = 0;
				if (wRefs != nullptr) {
					for (tItem* wRefItem : *wRefs) {
						if (wRefItem == nullptr || wRefPrinted >= 5) continue;
						tCell* wRefCell = wRefItem->Cell();
						if (wRefCell != nullptr) {
							cout << "  [topo_blocked] ref CELL " << wRefCell->StrRef(true) << endl;
							wRefPrinted++;
						}
						tRange* wRefRange = wRefItem->Range();
						if (wRefRange != nullptr && wRefPrinted < 5) {
							cout << "  [topo_blocked] ref RANGE " << wRefRange->StrRef()
								 << " isCell=" << (wRefRange->IsCell() ? 1 : 0) << endl;
							wRefPrinted++;
						}
					}
				}
				wPrinted++;
			}
#endif
			}
			// Topological ordering failed: remaining graph contains strongly connected components.
			// Try local SCC iterative evaluation before declaring recursive.
			std::vector<tInt> wTarjanIndex(static_cast<tSize>(wN), -1);
			std::vector<tInt> wTarjanLow(static_cast<tSize>(wN), 0);
			std::vector<tBool> wTarjanOnStack(static_cast<tSize>(wN), false);
			std::vector<tInt> wTarjanStack;
			wTarjanStack.reserve(static_cast<tSize>(wN));
			std::vector<tInt> wCompOfNode(static_cast<tSize>(wN), -1);
			std::vector<std::vector<tInt>> wCompNodes;
			tInt wTarjanTime = 0;

			std::function<void(tInt)> wStrongConnect = [&](tInt sV) {
				wTarjanIndex[static_cast<tSize>(sV)] = wTarjanTime;
				wTarjanLow[static_cast<tSize>(sV)] = wTarjanTime;
				wTarjanTime++;
				wTarjanStack.push_back(sV);
				wTarjanOnStack[static_cast<tSize>(sV)] = true;
				for (tInt wNext : wAdj[static_cast<tSize>(sV)]) {
					if (wTarjanIndex[static_cast<tSize>(wNext)] < 0) {
						wStrongConnect(wNext);
						wTarjanLow[static_cast<tSize>(sV)] = std::min(wTarjanLow[static_cast<tSize>(sV)],
																		wTarjanLow[static_cast<tSize>(wNext)]);
					} else if (wTarjanOnStack[static_cast<tSize>(wNext)]) {
						wTarjanLow[static_cast<tSize>(sV)] = std::min(wTarjanLow[static_cast<tSize>(sV)],
																		wTarjanIndex[static_cast<tSize>(wNext)]);
					}
				}
				if (wTarjanLow[static_cast<tSize>(sV)] == wTarjanIndex[static_cast<tSize>(sV)]) {
					std::vector<tInt> wNodes;
					while (!wTarjanStack.empty()) {
						tInt wNode = wTarjanStack.back();
						wTarjanStack.pop_back();
						wTarjanOnStack[static_cast<tSize>(wNode)] = false;
						wCompOfNode[static_cast<tSize>(wNode)] = static_cast<tInt>(wCompNodes.size());
						wNodes.push_back(wNode);
						if (wNode == sV) {
							break;
						}
					}
					wCompNodes.push_back(std::move(wNodes));
				}
			};
			for (tInt i = 0; i < wN; ++i) {
				if (wTarjanIndex[static_cast<tSize>(i)] < 0) {
					wStrongConnect(i);
				}
			}

			const tInt wCompCount = static_cast<tInt>(wCompNodes.size());
			std::vector<std::vector<tInt>> wCompAdj(static_cast<tSize>(wCompCount));
			std::vector<tInt> wCompIndeg(static_cast<tSize>(wCompCount), 0);
			for (tInt u = 0; u < wN; ++u) {
				const tInt wCompU = wCompOfNode[static_cast<tSize>(u)];
				for (tInt v : wAdj[static_cast<tSize>(u)]) {
					const tInt wCompV = wCompOfNode[static_cast<tSize>(v)];
					if (wCompU == wCompV) continue;
					std::vector<tInt>& wOut = wCompAdj[static_cast<tSize>(wCompU)];
					if (std::find(wOut.begin(), wOut.end(), wCompV) == wOut.end()) {
						wOut.push_back(wCompV);
						wCompIndeg[static_cast<tSize>(wCompV)]++;
					}
				}
			}

#ifdef debug_topo_blocked
			cout << "[topo_blocked] scc_components=" << wCompCount << endl;
#endif

			const tInt Cst_MaxSccIter = 40;
			std::queue<tInt> wCompQueue;
			for (tInt c = 0; c < wCompCount; ++c) {
				if (wCompIndeg[static_cast<tSize>(c)] == 0) {
					wCompQueue.push(c);
				}
			}
			tInt wCompVisited = 0;
			std::vector<tInt> wCompVisitOrder;
			wCompVisitOrder.reserve(static_cast<tSize>(wCompCount));
			while (!wCompQueue.empty()) {
				const tInt wComp = wCompQueue.front();
				wCompQueue.pop();
				wCompVisitOrder.push_back(wComp);
				wCompVisited++;
				std::vector<tInt>& wNodes = wCompNodes[static_cast<tSize>(wComp)];
				const tBool wIsCyclicComp = (static_cast<tInt>(wNodes.size()) > 1);
				if (!wIsCyclicComp) {
					const tInt wNode = wNodes[0];
					tCell* wCell = wCells[static_cast<tSize>(wNode)];
					if (wCell != nullptr && wCell->Formula() != nullptr) {
						SK_CALC_ORDER_CELL(wCell);
						wCell->InternalCalculation();
					}
				} else {
#ifdef debug_topo_blocked
					cout << "[topo_blocked] iterating_scc size=" << wNodes.size() << endl;
#endif
					std::vector<tVariant> wPrev;
					wPrev.reserve(wNodes.size());
					for (tInt wNode : wNodes) {
						tCell* wCell = wCells[static_cast<tSize>(wNode)];
						wPrev.push_back((wCell != nullptr) ? wCell->Value() : tVariant());
					}
					tBool wSccConverged = false;
					for (tInt iter = 0; iter < Cst_MaxSccIter; ++iter) {
						for (tInt wNode : wNodes) {
							tCell* wCell = wCells[static_cast<tSize>(wNode)];
							if (wCell != nullptr && wCell->Formula() != nullptr) {
								SK_CALC_ORDER_CELL(wCell);
								wCell->InternalCalculation();
							}
						}
						tBool wStable = true;
						tBool wDivergent = false;
						for (tSize k = 0; k < wNodes.size(); ++k) {
							tCell* wCell = wCells[static_cast<tSize>(wNodes[k])];
							const tVariant wNow = (wCell != nullptr) ? wCell->Value() : tVariant();
							if (!VariantEqualsForIteration(wNow, wPrev[k])) {
								wStable = false;
							}
							if (VariantIsDivergent(wNow)) {
								wDivergent = true;
							}
							wPrev[k] = wNow;
						}
						if (wDivergent) {
#ifdef debug_topo_blocked
							cout << "[topo_blocked] iterating_scc divergent_iter=" << (iter + 1) << endl;
#endif
							return(false);
						}
						if (wStable) {
#ifdef debug_topo_blocked
							cout << "[topo_blocked] iterating_scc converged_iter=" << (iter + 1) << endl;
#endif
							wSccConverged = true;
							break;
						}
					}
					if (!wSccConverged) {
#ifdef debug_topo_blocked
						cout << "[topo_blocked] iterating_scc not_converged max_iter=" << Cst_MaxSccIter << endl;
#endif
						return(false);
					}
				}
				for (tInt wCompNext : wCompAdj[static_cast<tSize>(wComp)]) {
					tInt& wDeg = wCompIndeg[static_cast<tSize>(wCompNext)];
					wDeg--;
					if (wDeg == 0) {
						wCompQueue.push(wCompNext);
					}
				}
			}
			if (wCompVisited != wCompCount) {
#ifdef debug_topo_blocked
				cout << "[topo_blocked] scc_visit_incomplete=" << wCompVisited << "/" << wCompCount << endl;
#endif
				return(false);
			}

			// Tear down: SCC iteration updated cell values but never ran Resolve(), so m_NbDepend is still
			// from the blocked snapshot. Use Resolve(..., true) to Dec dependents and Remove without
			// InternalCalculation (avoids corrupting NbDepend by calling full Resolve while still "blocked").
			// Order: condensation DAG visit order, then each node in the component (any order inside a cycle).
			{
				for (tInt wCompIdx : wCompVisitOrder) {
					std::vector<tInt>& wNodes = wCompNodes[static_cast<tSize>(wCompIdx)];
					for (tInt wNode : wNodes) {
						tCell* wCellTc = wCells[static_cast<tSize>(wNode)];
						if (wCellTc == nullptr) {
							continue;
						}
						const tAllocatorRef wPref = wCellTc->Path();
						if (wPref == 0) {
							continue;
						}
						tPath* wPathTeardown = m_AllocatorItemPath(wPref);
						if (wPathTeardown == nullptr) {
							continue;
						}
						if (SpillExtendWaitingOnOrigin(wPathTeardown)) {
							return(false);
						}
						Resolve(wPathTeardown, true);
						wDrainStackAfterResolve();
					}
				}
				tInt wStragGuard = 0;
				const tInt wStragMax = wN * wN + 64;
				while (m_RootItemPath != nullptr && wStragGuard < wStragMax) {
					++wStragGuard;
					tBool wDid = false;
					for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
						if (wP->Cell() == nullptr) {
							Remove(wP);
							wDid = true;
							break;
						}
						if (SpillExtendWaitingOnOrigin(wP)) {
							return(false);
						}
						if (wP->m_NbDepend == 0) {
							Resolve(wP);
						} else {
							Resolve(wP, true);
						}
						wDrainStackAfterResolve();
						wDid = true;
						break;
					}
					if (!wDid) {
						return(false);
					}
				}
				if (m_RootItemPath != nullptr) {
					return(false);
				}
			}
#ifdef debug_topo_blocked
			cout << "[topo_blocked] scc_iterative_resolve_done" << endl;
#endif
			return(true);
		}
#ifdef debug_topo_blocked
		cout << "[topo_blocked] topo_order=" << wOrder.size() << " topo_success=1" << endl;
#endif

		// Same rule as Reduce(): only Resolve paths with m_NbDepend==0. Repeat passes over wOrder until
		// no progress — one topo pass is not enough when order vs stack drain unlocks cells out of sequence.
		{
			tBool wProgress = true;
			tInt wTopoGuard = 0;
			const tInt wTopoGuardMax = wN * wN + 64;
			while (wProgress && m_RootItemPath != nullptr && wTopoGuard < wTopoGuardMax) {
				++wTopoGuard;
				wProgress = false;
				for (tInt wIdx : wOrder) {
					tCell* wCell = wCells[static_cast<tSize>(wIdx)];
					if (wCell == nullptr) {
						continue;
					}
					const tAllocatorRef wPathRef = wCell->Path();
					if (wPathRef == 0) {
						continue;
					}
					tPath* wPath = m_AllocatorItemPath(wPathRef);
					if (wPath == nullptr) {
						continue;
					}
					if (wPath->m_NbDepend != 0) {
						continue;
					}
					if (SpillExtendWaitingOnOrigin(wPath)) {
						continue;
					}
					Resolve(wPath);
					wDrainStackAfterResolve();
					wProgress = true;
				}
			}
		}
		// VectorRef DAG can be acyclic while range/covered deps keep m_NbDepend > 0. When the normal
		// resolve loop above stalls with cells still blocked, those cells are stuck on a range-covered
		// cycle (e.g. many SUMIF total-rows each summing their whole column, so each is a member of the
		// range the others aggregate). The topo graph is acyclic (self-edges dropped) and too large for
		// wForceSccDespiteTopo, so the SCC-iteration branch was skipped. Excel still computes these:
		// each cell's own/peer contribution uses the current value (0 when the criterion row doesn't
		// match). Compute them here with a bounded iterative pass (like the SCC branch) BEFORE the
		// teardown, otherwise the teardown below runs Resolve(..., true) with skipInternalCalculation and
		// leaves every stuck cell empty (t_none) instead of its Excel value.
		if (m_RootItemPath != nullptr) {
			std::vector<tCell*> wStuck;
			wStuck.reserve(static_cast<tSize>(wN));
			for (tInt wIdx : wOrder) {
				tCell* wCell = wCells[static_cast<tSize>(wIdx)];
				if (wCell == nullptr || wCell->Path() == 0 || wCell->Formula() == nullptr) {
					continue;
				}
				wStuck.push_back(wCell);
			}
			if (!wStuck.empty()) {
				const tInt Cst_MaxStuckIter = 40;
				std::vector<tVariant> wPrev;
				wPrev.reserve(wStuck.size());
				for (tCell* wCell : wStuck) {
					wPrev.push_back(wCell->Value());
				}
				for (tInt wIter = 0; wIter < Cst_MaxStuckIter; ++wIter) {
					for (tCell* wCell : wStuck) {
						SK_CALC_ORDER_CELL(wCell);
						wCell->InternalCalculation();
					}
					tBool wStable = true;
					tBool wDivergent = false;
					for (tSize wK = 0; wK < wStuck.size(); ++wK) {
						const tVariant wNow = wStuck[wK]->Value();
						if (!VariantEqualsForIteration(wNow, wPrev[wK])) {
							wStable = false;
						}
						if (VariantIsDivergent(wNow)) {
							wDivergent = true;
						}
						wPrev[wK] = wNow;
					}
					if (wDivergent) {
						return(false);
					}
					if (wStable) {
						break;
					}
				}
			}
		}
		// Values are now computed (or were already consistent); finish teardown without recalculating.
		if (m_RootItemPath != nullptr) {
			tInt wForceGuard = 0;
			const tInt wForceMax = wN * wN + 64;
			while (m_RootItemPath != nullptr && wForceGuard < wForceMax) {
				++wForceGuard;
				tBool wForceProgress = false;
				for (tInt wIdx : wOrder) {
					tCell* wCell = wCells[static_cast<tSize>(wIdx)];
					if (wCell == nullptr) {
						continue;
					}
					const tAllocatorRef wPathRef = wCell->Path();
					if (wPathRef == 0) {
						continue;
					}
					tPath* wPath = m_AllocatorItemPath(wPathRef);
					if (wPath == nullptr) {
						continue;
					}
					if (SpillExtendWaitingOnOrigin(wPath)) {
						return(false);
					}
					Resolve(wPath, true);
					wDrainStackAfterResolve();
					wForceProgress = true;
				}
				if (!wForceProgress) {
					break;
				}
			}
		}
		return(m_RootItemPath == nullptr);
	}

    tBool SpillExtendWaitingOnOrigin(tPath* sPath) {
        if (sPath == nullptr) {
            return false;
        }
        tCell* wCell = sPath->Cell();
        if (wCell == nullptr || !wCell->IsMatExtend()) {
            return false;
        }
        tCell* wRoot = wCell->CellMatrixRoot();
        return wRoot != nullptr && wRoot->Path() != 0;
    }

    tInt tContainerPath::CountPathsInList() {
        tInt wCount = 0;
        for (tPath* wItemPath = m_RootItemPath; wItemPath != nullptr;
             wItemPath = m_AllocatorItemPath(wItemPath->m_Next)) {
            ++wCount;
        }
        return wCount;
    }

	tInt tContainerPath::BlockedSubgraphProgressPercent() const {
		if (m_BlockedPhase <= 0) {
			return 0;
		}
		const tInt wN = static_cast<tInt>(m_BlockedCells.size());
		if (wN <= 0) {
			return (m_BlockedPhase >= 2) ? 90 : 0;
		}
		constexpr tInt Cst_MaxStuckIter = 40;
		if (m_BlockedPhase == 1) {
			const tInt wTotal = Cst_MaxStuckIter * wN;
			const tInt wDone = m_BlockedIter * wN + m_BlockedIndex;
			if (wTotal <= 0) {
				return 0;
			}
			const tInt wPct = (wDone * 80) / wTotal;
			return (wPct < 80) ? wPct : 80;
		}
		// Teardown: list still holds remaining paths until each Resolve.
		const tInt wTeardown = (m_BlockedIndex * 20) / wN;
		const tInt wPct = 80 + wTeardown;
		return (wPct < 99) ? wPct : 99;
	}

	tBool tContainerPath::StepBlockedSubgraph(tInt sMaxMs) {
		using tClock = std::chrono::steady_clock;
		const tBool wUnlimited = (sMaxMs <= 0);
		const auto wDeadline = wUnlimited
			? tClock::time_point::max()
			: (tClock::now() + std::chrono::milliseconds(sMaxMs));
		auto wShouldYield = [&]() {
			return !wUnlimited && tClock::now() >= wDeadline;
		};
		auto wDrainStackAfterResolve = [this]() {
			while (!m_StackPath.empty()) {
				tPath* wPath = m_StackPath.top();
				m_StackPath.pop();
				if (wPath == nullptr || wPath->m_NbDepend != 0 || SpillExtendWaitingOnOrigin(wPath)) {
					continue;
				}
				if (!PathEntryStillLive(wPath, m_AllocatorItemPath)) {
					continue;
				}
				if (wPath->Cell() == nullptr) {
					continue;
				}
				Resolve(wPath);
			}
		};

		constexpr tInt Cst_MaxStuckIter = 40;
		constexpr tInt Cst_SmallBlockedUseFullSolver = 64;

		if (m_BlockedPhase == 0) {
			tInt wRemaining = 0;
			for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
				if (wP->Cell() != nullptr) {
					++wRemaining;
				}
			}
			// Small leftover graphs need the SCC / DUPONT-ALIAN solver.
			if (wRemaining <= Cst_SmallBlockedUseFullSolver) {
				(void)ResolveBlockedAcyclicSubgraph();
				ClearBlockedSubgraphState();
				return true;
			}
			m_BlockedCells.clear();
			m_BlockedCells.reserve(static_cast<tSize>(wRemaining));
			for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
				tCell* wCell = wP->Cell();
				if (wCell != nullptr) {
					m_BlockedCells.push_back(wCell);
				}
			}
			m_BlockedPrev.clear();
			m_BlockedPrev.reserve(m_BlockedCells.size());
			for (tCell* wCell : m_BlockedCells) {
				m_BlockedPrev.push_back(wCell->Value());
			}
			m_BlockedIter = 0;
			m_BlockedIndex = 0;
			m_BlockedPhase = 1;
			// Yield so the UI can paint the last % before the heavy SUMIF iterations.
			if (!wUnlimited) {
				return false;
			}
		}

		if (m_BlockedPhase == 1) {
			const tInt wN = static_cast<tInt>(m_BlockedCells.size());
			while (m_BlockedIter < Cst_MaxStuckIter) {
				while (m_BlockedIndex < wN) {
					if (wShouldYield()) {
						return false;
					}
					tCell* wCell = m_BlockedCells[static_cast<tSize>(m_BlockedIndex)];
					if (wCell != nullptr && wCell->Formula() != nullptr) {
						SK_CALC_ORDER_CELL(wCell);
						wCell->InternalCalculation();
					}
					++m_BlockedIndex;
				}
				tBool wStable = true;
				tBool wDivergent = false;
				for (tSize wK = 0; wK < m_BlockedCells.size(); ++wK) {
					tCell* wCell = m_BlockedCells[wK];
					const tVariant wNow = (wCell != nullptr) ? wCell->Value() : tVariant();
					if (!VariantEqualsForIteration(wNow, m_BlockedPrev[wK])) {
						wStable = false;
					}
					if (VariantIsDivergent(wNow)) {
						wDivergent = true;
					}
					m_BlockedPrev[wK] = wNow;
				}
				if (wDivergent) {
					ClearBlockedSubgraphState();
					return true;
				}
				if (wStable) {
					break;
				}
				++m_BlockedIter;
				m_BlockedIndex = 0;
			}
			m_BlockedPhase = 2;
			m_BlockedIndex = 0;
			if (wShouldYield()) {
				return false;
			}
		}

		if (m_BlockedPhase == 2) {
			const tInt wN = static_cast<tInt>(m_BlockedCells.size());
			while (m_BlockedIndex < wN) {
				if (wShouldYield()) {
					return false;
				}
				tCell* wCell = m_BlockedCells[static_cast<tSize>(m_BlockedIndex)];
				++m_BlockedIndex;
				if (wCell == nullptr || wCell->Path() == 0) {
					continue;
				}
				tPath* wPath = m_AllocatorItemPath(wCell->Path());
				if (wPath == nullptr) {
					continue;
				}
				if (SpillExtendWaitingOnOrigin(wPath)) {
					ClearBlockedSubgraphState();
					return true;
				}
				if (wPath->m_NbDepend == 0) {
					Resolve(wPath);
				} else {
					Resolve(wPath, true);
				}
				wDrainStackAfterResolve();
			}
			tInt wStragGuard = 0;
			const tInt wStragMax = wN * 2 + 64;
			while (m_RootItemPath != nullptr && wStragGuard < wStragMax) {
				if (wShouldYield()) {
					return false;
				}
				++wStragGuard;
				tPath* wP = m_RootItemPath;
				if (wP->Cell() == nullptr) {
					Remove(wP);
					continue;
				}
				if (SpillExtendWaitingOnOrigin(wP)) {
					ClearBlockedSubgraphState();
					return true;
				}
				if (wP->m_NbDepend == 0) {
					Resolve(wP);
				} else {
					Resolve(wP, true);
				}
				wDrainStackAfterResolve();
			}
			ClearBlockedSubgraphState();
			return true;
		}

		ClearBlockedSubgraphState();
		return true;
	}

	void tContainerPath::Reduce() {
        while (!ReduceStep(0)) {
        }
    }

	tBool tContainerPath::ReduceStep(tInt sMaxMs) {
        using tClock = std::chrono::steady_clock;
        const tBool wUnlimited = (sMaxMs <= 0);
        const auto wDeadline = wUnlimited
            ? tClock::time_point::max()
            : (tClock::now() + std::chrono::milliseconds(sMaxMs));
        // Only yield the time budget once this step has resolved at least one cell. A full scan that
        // resolves nothing must run to completion so the blocked-subgraph / cycle detection below can
        // fire; otherwise a large genuine cycle would repeatedly hit the budget mid-scan and the path
        // count would never shrink (progress frozen, e.g. stuck near the end forever).
        tBool wStepResolvedAny = false;
        auto wShouldYield = [&]() {
            return wStepResolvedAny && !wUnlimited && tClock::now() >= wDeadline;
        };

		// Kahn work-queue: seed every ready path (m_NbDepend==0, not spill-waiting) onto m_StackPath,
		// then Resolve + drain. Each cell is resolved once. The previous scan restarted at the list
		// head after every Resolve (O(n^2) on long acyclic chains).
		while (m_RootItemPath != nullptr) {
            if (!wUnlimited && m_BlockedPhase != 0) {
                if (!StepBlockedSubgraph(sMaxMs)) {
                    return false;
                }
                if (m_RootItemPath == nullptr) {
                    return true;
                }
                SetRecursive();
                return true;
            }
            if (wShouldYield()) {
                return false;
            }
#ifdef debug_calculation
			cout << "Reduce -------------------------------------------------------" << endl;
			Debug();
#endif
			for (tPath* wItemPath = m_RootItemPath; wItemPath != nullptr;
				 wItemPath = m_AllocatorItemPath(wItemPath->m_Next)) {
				if (wItemPath->m_NbDepend == 0 && !SpillExtendWaitingOnOrigin(wItemPath)) {
					m_StackPath.push(wItemPath);
				}
			}

			if (m_StackPath.empty()) {
				// True cycle: every remaining cell is still waiting on at least one dependency.
				tBool wAllStillBlocked = true;
				for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
					if (wP->m_NbDepend == 0) {
						wAllStillBlocked = false;
						break;
					}
				}
				if (wAllStillBlocked) {
#ifdef debug_topo_blocked
					tInt wBlockedCount = 0;
					tInt wBlockedSum = 0;
					for (tPath* wP = m_RootItemPath; wP != nullptr; wP = m_AllocatorItemPath(wP->m_Next)) {
						wBlockedCount++;
						wBlockedSum += wP->m_NbDepend;
					}
					cout << "[topo_blocked] blocked_path_count=" << wBlockedCount
						 << " blocked_depend_sum=" << wBlockedSum << endl;
					tInt wPreview = 0;
					for (tPath* wP = m_RootItemPath; wP != nullptr && wPreview < 10; wP = m_AllocatorItemPath(wP->m_Next)) {
						tCell* wCell = wP->Cell();
						if (wCell != nullptr) {
							cout << "[topo_blocked] blocked_cell " << wCell->StrRef(true) << ":" << wP->m_NbDepend << endl;
							wPreview++;
						}
					}
#endif
#ifdef debug_calculation_recursive
					cout << "Blocked subgraph: trying local topological resolve." << endl;
#endif
					tBool wBlockedDone = false;
					if (wUnlimited) {
						wBlockedDone = ResolveBlockedAcyclicSubgraph();
					} else {
						// Large leftover (SUMIF column totals, range-covered cycles) used to run
						// ResolveBlockedAcyclicSubgraph in one step — UI frozen at ~88% for seconds.
						if (!StepBlockedSubgraph(sMaxMs)) {
							return false;
						}
						wBlockedDone = (m_RootItemPath == nullptr);
					}
					if (wBlockedDone) {
#ifdef debug_topo_blocked
						cout << "[topo_blocked] fallback topo resolved all -> skip SetRecursive" << endl;
#endif
						return true;
					}
#ifdef debug_topo_blocked
					cout << "[topo_blocked] fallback topo failed -> SetRecursive" << endl;
#endif
#ifdef debug_calculation_recursive
					cout << "Circular dependency: marking recursive error." << endl;
					Debug();
#endif
					SetRecursive();
				}
				return true;
			}

			while (!m_StackPath.empty()) {
				if (wShouldYield()) {
					return false;
				}
				tPath* wPath = m_StackPath.top();
				m_StackPath.pop();
				if (wPath == nullptr || wPath->m_NbDepend != 0 || SpillExtendWaitingOnOrigin(wPath)
					|| !PathEntryStillLive(wPath, m_AllocatorItemPath)) {
					continue;
				}
				if (wPath->Cell() == nullptr) {
					continue;
				}
#ifdef debug_calculation
				cout << " In Loop Reduce " << wPath->Cell()->StrRef() << ":" << wPath->m_NbDepend << endl;
#endif
				Resolve(wPath);
				wStepResolvedAny = true;
#ifdef debug_calculation
				cout << "After Resolve ";
				Debug();
#endif
			}
		}
        return true;
	};

	tPath* tContainerPath::AllocPath(tCell* sCell) {
		tAllocatorRef wAllocatorPath = 0;
		tPath* wItemPath;
		wItemPath = m_AllocatorItemPath(sCell->Path());
		if (wItemPath == nullptr) {
			// Alloc ItemPath
			tie(wAllocatorPath, wItemPath) = m_AllocatorItemPath.Alloc();
			//cout << "AllocPath -->" << wAllocatorPath << endl;
			wItemPath->Set(sCell);
			// Anchor path to cell ============================================
			sCell->Path(wAllocatorPath);
			// Drop stale #RECURSIVE from a prior calc pass before dependents read this cell.
			if (sCell->Formula() != nullptr) {
				tVariant& wVal = sCell->Value();
				if (wVal.Type() == tVariantType::t_error
					&& wVal.Error().Code() == tTypeError::t_recursive) {
					sCell->Value(tVariant());
				}
			}
			//=================================================================
			// Place in path
			if (m_RootItemPath == nullptr) {
				m_RootItemPath = wItemPath;
				m_RootItemPathRef = wAllocatorPath;
			} else {
				m_RootItemPath->Insert(&m_AllocatorItemPath, m_RootItemPathRef,wAllocatorPath);
			}
		}
		return(wItemPath);
	}

	void tContainerPath::RangeCovered(tPath* sPath) {
		if (sPath == nullptr) return;
		if (sPath->m_Recovered) return;
		sPath->m_Recovered = true;
		tCell* wCell = sPath->Cell();
		if (wCell == nullptr) return;
		// Get ColRowCellRange for this Cell
		tColRowCellRange* wColRowCellRange = wCell->ColRowCellRange();
		if (wColRowCellRange == nullptr) return;


		// Find range recover work with intersection and SkColRow (row & Col)
		tColRow::tContainerRange::tResult wContainerRange;
		wColRowCellRange->FindRanges(wCell, &wContainerRange);
#ifdef debug_calculation_covered
		tStringStream wStream; 
		wStream << "Search Range Covered -->" << wCell->Sheet()->Name() << "!" << wCell->StrRef() << ":";
#endif

		// Loop an all recovered ranges =============================================
		// For place cell formula dependend on path
		for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
			tRange* wRange = wColRowCellRange->Range(wRangeAllocatorRef);
            if (wRange!=nullptr) {
			//==========================================================================
			// If wRange IsMerged or IsNamed  is possible there are no dependent Cell
			//==========================================================================
#ifdef debug_calculation
			cout << sPath->Cell()->StrRef() << " Range Covered(" << wRange->AllocatorRef() << ")" << wRange->StrRef() << ":";
#endif
			tRange::tContainerCell* wDependCell = wRange->ContainerCellDepend();
			// Loop an dependent Cell (with formula point on this range).
                for (tCell* wCellFormulaDepend : *wDependCell->Container()) {
                        // Self-inclusion via range (e.g. T27=SUMIF(...,$T$1:$T$2644) where T27 is inside
                        // the summed range): Excel treats the cell's own contribution as its current value
                        // (0 when empty) instead of a hard circular reference. Do not create a self-dependency
                        // here, otherwise m_NbDepend can never reach 0 and the cell is torn down empty. Genuine
                        // multi-cell cycles (T27<->T31) keep their cross edges and still fall to SCC iteration.
                        if (wCellFormulaDepend == wCell) {
                            continue;
                        }
#ifdef debug_calculation
                        cout << " " << wCellFormulaDepend->StrRef();
#endif
                        // Search Path for this cell ==========================
                        tPath* wPathDepend = m_AllocatorItemPath(wCellFormulaDepend->Path());
                        // Alloc if not exist =================================
                        if (wPathDepend == nullptr) {
                            m_StackRun.push(wCellFormulaDepend);
                            // Alloc wPathDepand ==============================
                            wPathDepend = AllocPath(wCellFormulaDepend);
                        }
                        // Increment path of cell with formula contains this range ====
                        wPathDepend->Inc();
                        // Push Cell formula depandent for this recover cell ===========
                    }
#ifdef debug_calculation
                    cout << endl;
#endif
#ifdef debug_calculation_covered
                    wStream << ", " << wRange->StrRef();
#endif
                }
		}
#ifdef debug_calculation_covered
		cout << wStream.str() << endl;
#endif
		}

    // Spill footprint of an origin: MatrixRange (MatOrigin), then persisted SpillRange / AFO
    // (OOXML / .sker often has IsSpillRange without t_MatOrigin until the first SetValue).
    static tBool SpillFootprintRect(tCell* sOrigin, tIndex& oTop, tIndex& oLeft, tIndex& oBottom, tIndex& oRight) {
        if (sOrigin == nullptr) {
            return false;
        }
        tRange* wRange = sOrigin->MatrixRange();
        if (wRange == nullptr) {
            wRange = sOrigin->SpillRange();
        }
        if (wRange != nullptr) {
            oTop = wRange->TopIndex();
            oLeft = wRange->LeftIndex();
            oBottom = wRange->IterateBottom();
            oRight = wRange->IterateRight();
            return (oBottom >= oTop && oRight >= oLeft);
        }
        tTempoRect wAfo = sOrigin->ArrayFormulaOutputRect();
        if (!wAfo.IsValid()) {
            return false;
        }
        oTop = wAfo.Top();
        oLeft = wAfo.Left();
        oBottom = wAfo.Bottom();
        oRight = wAfo.Right();
        return (oBottom >= oTop && oRight >= oLeft);
    }

    void tContainerPath::AddMatrixDepend(tCell* sCell) {
        if (sCell == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = sCell->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return;
        }
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!SpillFootprintRect(sCell, wTop, wLeft, wBottom, wRight)) {
            return;
        }
        const tIndex wOriginRow = sCell->RowIndex();
        const tIndex wOriginCol = sCell->ColIndex();
        // Only live MatExtend slaves: SpillExtendWaitingOnOrigin keeps them behind the origin.
        // After RecalculateAll, ClearPersistedSpillSlaves drops MatExtend — adding those empty
        // cells lets them resolve first and RangeCovered Inc's the calendar week origins
        // (column B only, C–H stay blank). Formulas that read a slave are enqueued after
        // the origin spills (AddSpillSlaveFormulaDependents).
        for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
            for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                if (wRow == wOriginRow && wCol == wOriginCol) {
                    continue;
                }
                tCell* wCellExtend = wColRowCellRange->Cell(wRow, wCol);
                if (wCellExtend == nullptr || !wCellExtend->IsMatExtend()) {
                    continue;
                }
                tItem::tContainerCell* wDep = wCellExtend->ContainerCellDepend();
                if (wDep == nullptr || wDep->Container() == nullptr || wDep->Container()->empty()) {
                    continue;
                }
                Add(wCellExtend);
            }
        }
    }

    void tContainerPath::AddSpillSlaveFormulaDependents(tCell* sCell) {
        if (sCell == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = sCell->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return;
        }
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!SpillFootprintRect(sCell, wTop, wLeft, wBottom, wRight)) {
            return;
        }
        const tIndex wOriginRow = sCell->RowIndex();
        const tIndex wOriginCol = sCell->ColIndex();
        for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
            for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                if (wRow == wOriginRow && wCol == wOriginCol) {
                    continue;
                }
                tCell* wSlave = wColRowCellRange->Cell(wRow, wCol);
                if (wSlave == nullptr) {
                    continue;
                }
                tItem::tContainerCell* wDep = wSlave->ContainerCellDepend();
                if (wDep == nullptr || wDep->Container() == nullptr) {
                    continue;
                }
                for (tCell* wFormula : *wDep->Container()) {
                    if (wFormula == nullptr || wFormula == sCell || wFormula->Formula() == nullptr) {
                        continue;
                    }
                    if (wFormula->Path() != 0) {
                        continue;
                    }
                    // One re-enqueue per formula per pass: a cycle C7=C8 / B8=C7+… would otherwise
                    // Add(C7) after every origin Resolve and never empty the graph.
                    if (!m_SpillSlaveFormulasEnqueued.insert(wFormula).second) {
                        continue;
                    }
                    Add(wFormula);
                }
            }
        }
    }

	void tContainerPath::MakePath(tCell* sCell) {
		if (sCell == nullptr) return;
#ifdef debug_calculation
		cout << "MakePath " << sCell->StrRef() << endl;
#endif
		// Live MatExtend slaves only (they wait on this origin). Cleared slaves after
		// RecalculateAll are handled by AddSpillSlaveFormulaDependents after the spill.
		if (sCell->IsMatOrigin() || sCell->IsSpillRange()) {
			AddMatrixDepend(sCell);
		}
		tItem::tContainerCell* wDependContainer = sCell->ContainerCellDepend();
        // If not dependant cell return
		if (wDependContainer == nullptr || wDependContainer->Container() == nullptr) return;
		for (tCell* wCell : *wDependContainer->Container()) {
			if (wCell == sCell) {
				// Self-reference detected: cell references itself in its formula
#ifdef debug_calculation_recursive
				cout << "Self-reference detected for " << sCell->StrRef() << ". Setting recursive error." << endl;
#endif
				sCell->Value(tVariant(tClassError(tTypeError::t_recursive, "")));
				return;
			}
		}
		for (tCell* wCell : *wDependContainer->Container()) {
			tPath* wPath = m_AllocatorItemPath(wCell->Path());
            // If null add new path
			if (wPath == nullptr) {
#ifdef debug_calculation
				cout << "Depend " << wCell->StrRef() << endl;
#endif
				// Alloc path =====================================
				wPath = AllocPath(wCell);
				if (wPath == nullptr) continue; // Allocator failed, skip this dependency
				// Is range recover cell ==========================
				RangeCovered(wPath);

				// Recursive MakePath
#ifdef debug_calculation
				cout << "Inc wItem " << wCell->StrRef() << " Nb " << wPath->m_NbDepend << endl;
#endif
				wPath->Inc();
				m_StackRun.push(wCell);
			}
			else {
#ifdef debug_calculation
				cout << "Inc wCell " << wCell->StrRef() << " Nb " << wPath->NbDepend() << endl;
#endif
				m_AllocatorItemPath(wCell->Path())->Inc();
			}
		}
		
		// Alloc path =====================================
		tPath* wPath = AllocPath(sCell);
		if (wPath == nullptr) return;
		// Is range recover cell ==========================
		RangeCovered(wPath);
	};

	void tContainerPath::Calculate(tCell* sCell) {
		SK_CALC_ORDER_RESET();
		m_RootItemPath = nullptr;
		m_RootItemPathRef = 0;
		m_LastNbItemPath = 0;
		ResetCalculationPassGuards();

#ifdef thread_calculation
		std::thread threadObj([&, this] {
			Add(sCell);
			// At the end reduce Path
			Reduce();
			});
		//SetThreadPriority(threadObj.native_handle(), THREAD_PRIORITY_HIGHEST);
		threadObj.join();
#else
		Add(sCell);
		// At the end reduce Path
		Reduce();
#endif
	}

	void tContainerPath::Calculate(tColRowCellRange* sColRowCellRange, tTempoRect* sRect) {
		SK_CALC_ORDER_RESET();
		m_RootItemPath = nullptr;
		m_RootItemPathRef = 0;
		m_LastNbItemPath = 0;
		ResetCalculationPassGuards();
//#if defined(_WIN32) && defined(thread_calculation)
#ifdef thread_calculation
		std::thread threadObj([&, this] {
			AddRect(sColRowCellRange, sRect);
			// At the end reduce Path
			Reduce();
			});
		//SetThreadPriority(threadObj.native_handle(), THREAD_PRIORITY_HIGHEST);
		threadObj.join();
#else
		AddRect(sColRowCellRange, sRect);
		// At the end reduce Path
		Reduce();
#endif
	}

	void tContainerPath::Calculate(tColRowCellRange* sColRowCellRange, const tSelect& sSelect) {
		SK_CALC_ORDER_RESET();
		m_RootItemPath = nullptr;
		m_RootItemPathRef = 0;
		m_LastNbItemPath = 0;
		ResetCalculationPassGuards();
 
//#if defined(__EMSCRIPTEN__) && defined(thread_calculation)
#ifdef thread_calculation
		// Capture sSelect by reference (const reference is safe in thread since object lifetime is guaranteed)
		std::thread threadObj([sColRowCellRange, &sSelect, this] {
			AddSelect(sColRowCellRange, sSelect);
			// At the end reduce Path
			Reduce();
			});
		//SetThreadPriority(threadObj.native_handle(), THREAD_PRIORITY_HIGHEST);
		threadObj.join();
#else
		AddSelect(sColRowCellRange, sSelect);
		// At the end reduce Path
		Reduce();
#endif
	}

	void tContainerPath::Debug() const {
		if (m_RootItemPath != nullptr) {
			if (m_RootItemPath->Cell() != nullptr) cout << "Root " << m_RootItemPath->Cell()->StrRef() << " ";
		}
		cout << "List Path -->";
		const tPath* wItemPath = m_RootItemPath;

		if (wItemPath != nullptr) {
			while (wItemPath->m_Prev != 0) wItemPath = m_AllocatorItemPath(wItemPath->m_Prev);
			while (wItemPath != nullptr) {
				wItemPath->Debug();
				wItemPath = m_AllocatorItemPath(wItemPath->m_Next);
			}
			cout << endl;
		}
	};

	/// @brief      Begin calculate for Delete Row & Col.
	void tContainerPath::BeginCalculate() {
		SK_CALC_ORDER_RESET();
		m_RootItemPath = nullptr;
		m_LastNbItemPath = 0;
		ResetCalculationPassGuards();
	}

	void tContainerPath::Add(tCell* sCell) {
		if (sCell == nullptr) {
			return;
		}
		if (sCell->Path() == 0) {
    
#ifdef debug_calculation
            if (sCell->StrRef()=="B8") {
            }
			cout << "Add .." << sCell->StrRef() << ":" << sCell->FormulaStr() << endl;
#endif

			// Stale #RECURSIVE is cleared in AllocPath() when a new path is created
			AllocPath(sCell);
			MakePath(sCell);

			// After make treat stack of depend cell
			while (!m_StackRun.empty()) {
#ifdef debug_calculation
				cout << m_StackRun.size() << ".";
#endif
				tCell* wCell = m_StackRun.top();
#ifdef debug_calculation
				cout << "Stack " << wCell->StrRef() << endl;
#endif
				m_StackRun.pop();
				MakePath(wCell);
			}
		}
	}

	void  tContainerPath::AddRect(tColRowCellRange* sColRowCellRange, tTempoRect* sRect) {
        tBool wIsRecover=false;
		for (tIndex wRow = sRect->Top(); wRow <= sRect->Bottom(); wRow++) {
			for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
				tCell* wCell = sColRowCellRange->Cell(wRow, wCol);
				if (wCell != nullptr) {
#ifdef debugpath
                  if (wCell->Path()==0) {
                            cout << "     ---->"<<  wCell->StrRef(true)  << endl;
                  }
#endif
					Add(wCell);
                } else {
                    // if  Null Search Dependant by recover range
                    wIsRecover=true;
                }
			}
		}
        // Seach all cell dependent ===========================================
        if (wIsRecover) {
            tRect wRect=tRect(*sRect);
            tVectorRange wVectorRange;
            sColRowCellRange->FindRanges(wRect, &wVectorRange);
            for(auto wRange : wVectorRange ) {
#ifdef debugpath
                cout << "   AddRect(" << wRange->StrRef() << ")" << endl;
#endif
                tRange::tContainerCell* wDependCell = wRange->ContainerCellDepend();
                // Loop an dependent Cell (with formula point on this range).
                for (tCell* wCellFormulaDepend : *wDependCell->Container()) {
#ifdef debugpath
                        if (wCellFormulaDepend->Path()==0) {
                            cout << "    depend range -->"<<  wCellFormulaDepend->StrRef(true)  << endl;
                        }
#endif
                        Add(wCellFormulaDepend);
                    }
            }

        }
	}

	void tContainerPath::AddAllFormulaCells(tColRowCellRange* sColRowCellRange) {
		class tCallBackAddFormulaCell : public tSparseArrayCallBack<tAllocatorRef> {
			tColRowCellRange* m_ColRowCellRange;
			tContainerPath* m_ContainerPath;
		public:
			tCallBackAddFormulaCell(tColRowCellRange* sColRowCellRange, tContainerPath* sContainerPath)
				: m_ColRowCellRange(sColRowCellRange), m_ContainerPath(sContainerPath) {}
			tBool CallBack(tAllocatorRef sAllocatorRef) override {
				tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
				if (wCell != nullptr && wCell->Formula() != nullptr) {
					m_ContainerPath->Add(wCell);
				}
				return(true);
			}
		};
		tCallBackAddFormulaCell wCallBack(sColRowCellRange, this);
		sColRowCellRange->CallBackAllCell(&wCallBack);
	}

	void  tContainerPath::AddSelect(tColRowCellRange* sColRowCellRange, const tSelect& sSelect) {
        for (auto wPoint : *sSelect.VectorSelect()) {
            if (wPoint!=nullptr) {
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wPoint);
                if (wRect != nullptr) {
                    AddRect(sColRowCellRange, wRect);
                } else {
                    tCell* wCell = sColRowCellRange->Cell(wPoint->Row(), wPoint->Col());
                    if (wCell != nullptr) {
                        Add(wCell);
                    } else {
                        // Cell was deleted (e.g. Raz on a plain value cell): we must
                        // still recalc any cell whose formula references a range
                        // covering this point. Promote to a 1x1 rect and reuse
                        // AddRect's recovery logic.
                        tTempoRect wFallback(wPoint->Row(), wPoint->Col(), wPoint->Row(), wPoint->Col());
                        AddRect(sColRowCellRange, &wFallback);
                    }
                }
            }
		}
	}

	void tContainerPath::EndCalculate() {
		Reduce();
	}


} // end of namespace
 
