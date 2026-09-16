//=============================================================================
// SkSpreadSheet Cell
//=============================================================================

#include "../include/SkSpreadSheet.hpp"
#include "../include/SkCellClassAttribute.hpp"
#include "../include/SkMatrix.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkColRow.hpp"
#include "../include/SkRangeNamed.hpp"
#include "../include/SkCellClassUnit.hpp"
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

// Trace macros: enable with -Ddebugcalculate, -Ddebugfunction, etc. (names match #ifdef below, no leading underscore).


#define _debugcell
#define _debugcalculate
#define _debugcalculateorder
#define _debugfunction
#define _debugjson
#define _debugdependent
#define _debugpointer
#define _debugjsondb
#define _debugmatrix

namespace SkSpreadSheet {

    // Build a closure environment for sNames by resolving each name against the active scopes (innermost last,
    // so search back-to-front). Names not currently in scope are simply skipped. Used to bind an inline lambda's
    // captured outer variables at the point its value is created / it is called.
    static void BuildCapturedScope(const std::vector<tString>& sNames,
                                   const std::vector<std::map<tString, tStackElem>>& sScopes,
                                   std::map<tString, tStackElem>& sOut) {
        for (const tString& wName : sNames) {
            for (auto wIt = sScopes.rbegin(); wIt != sScopes.rend(); ++wIt) {
                auto wHit = wIt->find(wName);
                if (wHit != wIt->end()) {
                    sOut[wName] = wHit->second;
                    break;
                }
            }
        }
    }

    // Output target for matrix spill on named-formula sheet: only the definition cell on CstSheetNamed (_$$).
    static tFormulaNamed* FormulaNamedOutputForCell(tCell* sCell) {
        if (sCell == nullptr) {
            return nullptr;
        }
        tSheet* wSheet = sCell->Sheet();
        if (wSheet == nullptr || wSheet->Name() != CstSheetNamed) {
            return nullptr;
        }
        tWorkBook* wWB = wSheet->WorkBook();
        if (wWB == nullptr) {
            return nullptr;
        }
        return wWB->FindFormulaNamedByCell(sCell);
    }

    // OOXML multi-cell array formula on this cell: must keep full matrix spill (e.g. =A1:B2+10 spilling from D1).
    static tBool CellExpectsArrayFormulaMatrixSpill(tCell* sCell) {
        if (sCell == nullptr || !sCell->IsSpillRange()) {
            return false;
        }
        tTempoRect wAfo(sCell->ArrayFormulaOutputRect());
        return sCell->RowIndex() == wAfo.Top() && sCell->ColIndex() == wAfo.Left() &&
               (wAfo.Width() > 1 || wAfo.Height() > 1);
    }

    // Excel D14#: the spill footprint of the origin (dynamic array or named-formula host). nullptr → #REF!.
    static tRange* SpillRangeForHashOperator(tCell* sCell) {
        if (sCell == nullptr) {
            return nullptr;
        }
        tFormulaNamed* wFn = FormulaNamedOutputForCell(sCell);
        if (wFn != nullptr) {
            tRange* wSpill = wFn->SpillRange();
            if (wSpill != nullptr && !wSpill->IsCell()) {
                return wSpill;
            }
        }
        if (sCell->IsMatOrigin() || sCell->IsSpillRange()) {
            tRange* wSpill = sCell->SpillRange();
            if (wSpill != nullptr) {
                return wSpill;
            }
            wSpill = sCell->MatrixRange();
            if (wSpill != nullptr) {
                return wSpill;
            }
        }
        return nullptr;
    }

    // Single row or single column (not one cell, not a 2-D block). Uses indices only — no Rect() (hot path).
    static tBool RangeIs1DVectorForImplicitIntersection(tRange* sRange) {
        if (sRange == nullptr || sRange->IsCell()) {
            return false;
        }
        return (sRange->TopIndex() == sRange->BottomIndex()) || (sRange->LeftIndex() == sRange->RightIndex());
    }

    // Implicit intersection applies only to workbook range names (e.g. DuréePrêt column). Literal spills
    // ({1,2,3}+10), direct refs (A1:A6/…), and 2-D matrix blocks are not named — keep full matrix path.
    static tBool RangeUsesNamedImplicitIntersection(tRange* sRange) {
        return sRange != nullptr && sRange->IsNamed() && RangeIs1DVectorForImplicitIntersection(sRange);
    }

    // One cell from range at formula row (column range) or formula column (row range), Excel legacy style.
    static tBool RangeImplicitIntersectionValue(tCell* sFormulaCell, tRange* sRange, tVariant& outValue) {
        if (sFormulaCell == nullptr || sRange == nullptr) {
            return false;
        }
        tSheet* wSheet = sRange->Sheet();
        if (wSheet == nullptr) {
            return false;
        }
        tColRowCellRange* wColRow = wSheet->ColRowCellRange();
        if (wColRow == nullptr) {
            return false;
        }
        const tIndex wTop = sRange->TopIndex();
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wBottom = sRange->BottomIndex();
        const tIndex wRight = sRange->RightIndex();
        const tIndex wFr = sFormulaCell->RowIndex();
        const tIndex wFc = sFormulaCell->ColIndex();
        tIndex wTr = 0;
        tIndex wTc = 0;
        if (wLeft == wRight) {
            wTc = wLeft;
            wTr = wFr;
            if (wTr < wTop || wTr > wBottom) {
                return false;
            }
        } else if (wTop == wBottom) {
            wTr = wTop;
            wTc = wFc;
            if (wTc < wLeft || wTc > wRight) {
                return false;
            }
        } else {
            return false;
        }
        tCell* wCell = wColRow->Cell(wTr, wTc);
        if (wCell == nullptr) {
            outValue = tVariant();
            return true;
        }
        outValue = wCell->CalculableValue();
        return true;
    }

    // Scalar from a range ref: 1-D → implicit intersection at the formula cell; else top-left
    // (a MatOrigin spill's top-left is the origin). TEXT(C9) is t_Cell — not this path.
    // Do not Clear ioOut before reading: ioOut may alias m_Value of self (same-cell copy would wipe the value).
    static void AssignVariantFromRangeRefForCell(tCell* self, tRange* wRange, tVariant& ioOut) {
        if (self == nullptr || wRange == nullptr) {
            ioOut.Clear();
            return;
        }
        tColRowCellRange* wCr = self->ColRowCellRange();
        if (wCr == nullptr) {
            ioOut.Clear();
            return;
        }
        tVariant wAt;
        if (RangeIs1DVectorForImplicitIntersection(wRange) && RangeImplicitIntersectionValue(self, wRange, wAt)) {
            ioOut = wAt;
            return;
        }
        tCell* wCellAtTopLeft = wRange->Cell();
        if (wCellAtTopLeft == nullptr) {
            wCellAtTopLeft = wCr->EnsureCell(wRange->TopIndex(), wRange->LeftIndex());
        }
        if (self->RowIndex() == wRange->TopIndex() && self->ColIndex() == wRange->LeftIndex()) {
            wCellAtTopLeft = self;
        }
        if (wCellAtTopLeft != nullptr) {
            ioOut = tVariant(wCellAtTopLeft->CalculableValue());
        } else {
            ioOut.Clear();
        }
    }

        tCellExtend::tCellExtend() : tClass() {}

        tCellExtend::tCellExtend(const tCellExtend& sCellExtend) : tClass(sCellExtend) {}

        tCellExtend::~tCellExtend() {}

        void tCellExtend::Clear() {
          
        }


    void tCellExtend::Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter) {
        sWriter->Key("ext");
        sWriter->StartArray();
        sWriter->EndArray();
    }

    void tCellExtend::Json(const rapidjson::Value& sValue) {
        if (sValue.HasMember("ext") && sValue["ext"].IsArray()) {
            const rapidjson::Value& wExt = sValue["ext"];
            if (wExt.Size() >= 4) {
            }
        }
    }



	tCell::tCell() : tItem(tTypeItem::t_Cell, 0, -1, -1),
					m_Value(),
					m_Css(0),
					m_CalculationPath(0),
                    m_CellExtend(0) {}

	tCell::tCell(const tCell& sCell) : tItem(sCell) {
		m_Value = sCell.m_Value;
		m_SharedFormula = sCell.m_SharedFormula;
		m_Css = sCell.m_Css;
		m_VectorRef = sCell.m_VectorRef;
		m_VectorRefAddDependent = sCell.m_VectorRefAddDependent;
		m_CalculationPath = sCell.m_CalculationPath;
        m_CellExtend=0;
        if (sCell.m_CellExtend!=0) {
            tColRowCellRange* wColRowCellRange=ColRowCellRange();
            m_CellExtend=ColRowCellRange()->AllocCellExtend();
            tCellExtend* wCellExtendSource=wColRowCellRange->CellExtend(sCell.m_CellExtend);
            tCellExtend* wCellExtend=wColRowCellRange->CellExtend(m_CellExtend);
            *wCellExtend=*wCellExtendSource;
        }
	}

	tCell::~tCell() {}

	void tCell::Clear() {
        // While terminated program
        if (!ColRowCellRange()->ClearInProgress()) {
                //cout << "Clear::" << tItem::StrRef() << ":" << ItemRef().AllocatorRef() << endl;
                ClearVectorRefAndDeleteDependant();
                  if (m_CellExtend!=0) {
                    ColRowCellRange()->DeleteCellExtend(m_CellExtend);
                  }
        }

        // Do NOT DeleteVolatile here. Alloc-reuse calls Clear() on a recycled slot whose
        // Row/Col still point at a live grid cell — that wrongly unregisters that cell.
        // Callers that destroy a live cell (DeleteCell / ClearFormula*) unregister first.
       
        tItem::Clear();
        m_Value.Clear();
        m_SharedFormula.Clear();
		m_CalculationPath = 0;
      
        if (m_Css != 0) {
            WorkBook()->DeleteCellFormat(m_Css);
            m_Css = 0;
        }
	}

	void tCell::ClearFormulaAndVariant() {
		// Check if cell was volatile before clearing formula
		const tFormula* wFormula = Formula();
        if (StrRef()=="B15") {
        }
		if (wFormula != nullptr) {
			if (!wFormula->BitSetVolatile().Empty()) {
				ColRowCellRange()->DeleteVolatile(this);
			}
		}
		// Is Class --> recursive ClearFormulaVariant  ========================
 		if (m_Value.Type() == tVariantType::t_class) {
			tCellClassAttribute* wCellClassAttribute = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
			if (wCellClassAttribute != nullptr) {
				wCellClassAttribute->ClearFormulaVariant();
			}
		}
            
        ClearVectorRefAndDeleteDependant();
        // Raz Matrix ==================================================
        if (IsMatOrigin()) {
            ClearMatrix();
        } else if (IsMatExtend()) {
            // Excel: clearing/overwriting a spill slave tears down the whole spill (origin formula kept).
            BreakSpillForUserOverwrite();
        }
        // Spill Range: native dynamic spills (t_MatDynamic) re-anchor on each recompute, so releasing their AFO here
        // leaves the cell truly empty on undo / overwrite (e.g. Undo of B1=A1:A2 must make B1 vanish). OOXML array-formula
        // refs (no t_MatDynamic) keep their authoritative ref, which the clamp still needs.
        if (IsSpillRange() && IsMatDynamic()) {
            ClearArrayFormulaOutputRect();
            RemoveMatDynamic();
        }

		m_SharedFormula.Clear();
		m_Value.Clear();
	}

    void tCell::ClearFormula() {
		// Check if cell was volatile before clearing formula
		const tFormula* wFormula = Formula();
		if (wFormula != nullptr) {
			if (!wFormula->BitSetVolatile().Empty()) {
				ColRowCellRange()->DeleteVolatile(this);
			}
        }
        ClearVectorRefAndDeleteDependant();
        m_SharedFormula.Clear();
        // OOXML array-formula output ref is tied to the formula; clear when the formula is cleared.
        // Only for native dynamic spills (t_MatDynamic): they re-anchor on recompute, so releasing the AFO with the
        // formula is safe. OOXML refs (no t_MatDynamic) are preserved. Slaves never carry SpillRange so they are unaffected.
        if (IsSpillRange() && IsMatDynamic()) {
            ClearArrayFormulaOutputRect();
            RemoveMatDynamic();
        }
    }

	// Row Col ========================================================
	tColRow* tCell::Row() const {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_RowAllocatorRef,true));
	}

	tIndex tCell::RowIndex() {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_RowAllocatorRef,true)->Index());
	}
	
	tIndex tCell::RowIndex() const {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_RowAllocatorRef,true)->Index());
	}

	tColRow* tCell::Col() const {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_ColAllocatorRef,false));
	}

	tIndex tCell::ColIndex() {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_ColAllocatorRef,false)->Index());
	}
	
	tIndex tCell::ColIndex() const {
        return(ColRowCellRange()->ColRowByAllocatorRef(m_ColAllocatorRef,false)->Index());
	}

	tAllocatorRef tCell::Path() const { return(m_CalculationPath); }
	void tCell::Path(tAllocatorRef sPath) { m_CalculationPath = sPath; }


    void tCell::SetColRow(const tAllocatorRef sRowAllocatorRef, const tAllocatorRef sColAllocatorRef) {
        m_RowAllocatorRef = sRowAllocatorRef;
        m_ColAllocatorRef = sColAllocatorRef;
    }

	void tCell::Set(const tRefItem sRef, const tAllocatorRef sRowAllocatorRef, const tAllocatorRef sColAllocatorRef) {
        m_ColRowCellRangeRef = sRef;
		m_RowAllocatorRef = sRowAllocatorRef;
		m_ColAllocatorRef = sColAllocatorRef;
	}

    
	tAllocatorRef tCell::RowAllocatorRef() const {
		return(m_RowAllocatorRef);
	}

    tAllocatorRef tCell::ColAllocatorRef() const {
		return(m_ColAllocatorRef);
	}

	void tCell::Formula(const tFormula* sFormula) {
		m_SharedFormula = *sFormula;
	}

	tFormula* tCell::Formula() {
		return (m_SharedFormula.Formula());
	}
	
	const tFormula* tCell::Formula() const {
		return m_SharedFormula.Formula();
	}



	const tString tCell::FormulaStr(tBool sR1C1,tBool sUser) const {
        const tFormula* wFormula = m_SharedFormula.Formula();
		if (wFormula != nullptr) return wFormula->Str(this, sR1C1, sUser);
        return("");
	}

	const tString tCell::FormulaWire(tBool sR1C1, tBool sUser) const {
		tLocalePush wUs("us");
		return FormulaStr(sR1C1, sUser);
	}
	//=========================================================================
	// CALCULATION 
	//=========================================================================
    const tStackElem tCell::CallFunction(tFormula* sFormula,
                                       const tItemFormula* sItemFormula,
                                       tStackElems* sStackElem,
                                       const std::vector<std::map<tString, tStackElem>>* sLetScopes) {
		tFunctionDictionary* wFunctionDictionary = tSpreadSheetContainer::Instance()->FunctionDictionary();
        // OPTIMIZATION: Save Value() to avoid repeated calls
        tVariant wItemValue = sItemFormula->Value();
        // Get function reference (Excel-style: function names are case-insensitive)
        tString wFunctionName = wItemValue.String();
        std::transform(wFunctionName.begin(), wFunctionName.end(), wFunctionName.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		tFunctionRef wFunctionRef = wFunctionDictionary->FunctionRef(wFunctionName);
        tFunction* wFunction = wFunctionRef.Function();
        if (wFunction == nullptr) {
            // Named LAMBDA call: NAME(arg1; ...) where NAME is a user LAMBDA (built-in functions take priority).
            tStackElem wLambdaResult;
            if (CallNamedLambda(wFunctionName, sItemFormula, sStackElem, wLambdaResult, sLetScopes)) {
                return wLambdaResult;
            }
            // Unresolved callee (e.g. calling a LET variable that is not a LAMBDA: LET(v;5;v(2))).
            // CallNamedLambda left the stack untouched, so consume the pushed arguments here to keep the
            // evaluation stack balanced; otherwise Calculate() raises "Stack not empty !".
            {
                tShort wNbArgUnknown = static_cast<tShort>(wItemValue.Extra());
                for (tShort i = 0; i < wNbArgUnknown && sStackElem != nullptr && !sStackElem->empty(); ++i) {
                    sStackElem->pop();
                }
            }
            return tStackElem(tVariant(tClassError(tTypeError::t_name, wFunctionName)));
        }
        wFunction->PassByRef(this);
        // The number of argument is in Extra() see LemonInterface::PushFunctionMethod()
        tShort wNbArgFromExtra = static_cast<tShort>(wItemValue.Extra());
        tShort wNbArg = wNbArgFromExtra;
        /*
        // Legacy / round-trip: tVariant JSON used to omit "e" when Extra==0, so function arity was lost (DATE looked like 0-arg).
        // If Extra says 0 but the stack has enough operands for the dictionary arity, use the dictionary count.
        // ROW/COLUMN accept 0 args with dictionary arity 1 — do not override.
        {
            const tInt wNbArgDict = wFunctionRef.NbArg();
            const tString& wFnName = wItemValue.String();
            if (wNbArgDict >= 0 && wNbArgFromExtra == 0 && wNbArgDict > 0 &&
                wFnName != "ROW" && wFnName != "COLUMN" &&
                sStackElem != nullptr && sStackElem->size() >= static_cast<size_t>(wNbArgDict)) {
                wNbArg = static_cast<tShort>(wNbArgDict);
            }
        }
        */
		tString wName= wFunctionRef.Name();
#ifdef debugfunction
		// Call first so the stack is consumed by Call() only; then Debug from a copy for display
		tStackElems wStackCopyForDebug = *sStackElem;
#endif
		// OPTIMIZATION: Use saved function pointer instead of calling Function() again
		tStackElem wStackResult = wFunction->Call(sStackElem, wNbArg);
#ifdef debugfunction
		cout << StrRef() << ":"  << wName << "->" <<
		wFunction->Debug(wName, wStackCopyForDebug, wNbArg);
#endif
        if (wStackResult.Type()==tStackType::t_Variant) {
            tVariant wResult=wStackResult.Value();
            if (wResult.Type() == tVariantType::t_double) {
                tDouble wDouble = wResult.Double();
                if (std::isnan(wDouble) || std::isinf(wDouble)) {
                    tVariant wValue;
                    wValue.SetError(tClassError(tTypeError::t_value, ""));
                    return(tStackElem(wValue));
                }
            }
        }
#ifdef debugfunction
        cout << "  return " << wStackResult.Debug() <<  " " << endl;
#endif
		return(wStackResult);
	}

	tBool tCell::CallNamedLambda(const tString& sName,
	                             const tItemFormula* sItemFormula,
	                             tStackElems* sStackElem,
	                             tStackElem& sResult,
	                             const std::vector<std::map<tString, tStackElem>>* sLetScopes) {
		tWorkBook* wWorkBook = WorkBook();
		if (wWorkBook == nullptr) {
			return false;
		}
		tFormulaNamed* wLambda = wWorkBook->FindFormulaNamed(sName);
		// A LET-bound LAMBDA value shadows the global named-formula container: LET(f; LAMBDA(x;...); f(3))
		// stores a first-class t_Lambda in the current scope, which must be callable like a function.
		// Its closure was already captured when the value was created (LambdaRef), so use it verbatim.
		const std::map<tString, tStackElem>* wScopeCaptured = nullptr;
		if ((wLambda == nullptr || !wLambda->IsLambda()) && sLetScopes != nullptr) {
			for (auto wIt = sLetScopes->rbegin(); wIt != sLetScopes->rend(); ++wIt) {
				auto wHit = wIt->find(sName);
				if (wHit != wIt->end() && wHit->second.Type() == tStackType::t_Lambda) {
					wLambda = wHit->second.Lambda();
					wScopeCaptured = wHit->second.CapturedScope();
					break;
				}
			}
		}
		if (wLambda == nullptr || !wLambda->IsLambda()) {
			return false; // not a lambda: leave the stack untouched so the caller raises #NAME?.
		}
		// From here the name IS a lambda: we own the call and always return true (arguments are consumed).
		const tShort wNbArg = static_cast<tShort>(sItemFormula->Value().Extra());
		// Pop the arguments (top of stack is the last argument).
		std::vector<tStackElem> wArgs;
		for (tShort i = 0; i < wNbArg && sStackElem != nullptr && !sStackElem->empty(); ++i) {
			wArgs.push_back(sStackElem->top());
			sStackElem->pop();
		}
		std::reverse(wArgs.begin(), wArgs.end());
		// Closure resolution:
		//  - LET-bound lambda value: use the closure captured at value-creation time (wScopeCaptured).
		//  - inline lambda applied immediately (e.g. LET(a;5;LAMBDA(x;x+a)(3))): rebuild from the call-site
		//    LET scopes, since there is no LambdaRef value carrying a snapshot.
		std::map<tString, tStackElem> wCaptured;
		const std::map<tString, tStackElem>* wCapturedPtr = nullptr;
		if (wScopeCaptured != nullptr) {
			wCapturedPtr = wScopeCaptured;
		} else if (wLambda->HasCapturedNames() && sLetScopes != nullptr) {
			BuildCapturedScope(wLambda->CapturedNames(), *sLetScopes, wCaptured);
			wCapturedPtr = wCaptured.empty() ? nullptr : &wCaptured;
		}
		// Bind parameters and evaluate the body (arity is validated inside Invoke -> #VALUE! on mismatch).
		tBool wArityOk = true;
		tVariant wValue = wLambda->Invoke(wArgs, wArityOk, wCapturedPtr);
		sResult = tStackElem(wValue);
		return true;
	}


	const tVariant tCell::InternalCalculation(tFormula* sFormula,
	                                          const std::map<tString, tStackElem>* sInjectedScope) {
#ifdef _DEBUGSK	// for break point in debug
        if (StrRef() == "B15") {
            //cout << "DEBUG E7 E8 " << endl;
            //cout << Debug() << endl;
            //tCell* wCellE8=Sheet()->Cell(8,5);
            //cout << wCellE8->Debug();
        }
#endif
        // Reset matrix only for a full cell recalculation (sFormula == nullptr). Callers such as
        // tConditionalFormat::CallBackCell pass a separate compiled formula; ClearMatrix() would wipe
        // spill/matrix cell values (Value().Clear) and leave the grid empty after PassApply (Chrome/WASM vs native timing).
        if (sFormula == nullptr && IsMatOrigin()) {
            ClearMatrix();
        }
    
		tVariant wVariantWork;
        // Conditional Format
        tFormula* wFormula;
        if (sFormula == nullptr) {
            wFormula = m_SharedFormula.Formula();
        } else {
            wFormula = sFormula;
        }
		// if not formula return
		if (wFormula == nullptr) return(wVariantWork);

#ifdef debugcalculateorder
        cout << StrRef() << ":" << FormulaStr() << "=";
#endif

		// Stack for calcul
		tStackElems wStackElems;

		// LET local-variable scopes (innermost last). Each LetBeginScope pushes a map, LetBind fills it,
		// LetVarRef reads it (innermost-first), LetEndScope pops it. Names are uppercased (LET is
		// case-insensitive). Kept out of tStackElems so LET binding does not disturb the RPN stack.
		std::vector<std::map<tString, tStackElem>> wLetScopes;
		auto wLetUpper = [](tString s) -> tString {
			std::transform(s.begin(), s.end(), s.begin(),
			               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
			return s;
		};
		// Seed the outermost scope with injected LAMBDA parameters (already uppercased by the caller), so the
		// body's parameter references (compiled to LetVarRef) resolve to the supplied argument values.
		if (sInjectedScope != nullptr && !sInjectedScope->empty()) {
			wLetScopes.push_back(*sInjectedScope);
		}

		// Indice on m_VectorRef for cell or range (Lex order).
		tIndex wIndice = 0;
	
#ifdef debugcalculate
        cout << " Internal Calculate " << tItem::StrRef(true) << "->"  << FormulaStr() <<  endl;
        cout << " Formula ->" << Formula()->FormulaKey() << endl;
        cout << " Value ->" << m_Value << endl;
        cout << "  Ref" << endl;
        for(auto wItem : m_VectorRef) {
            if (wItem!=nullptr) {
                cout << "    ->"  << wItem->StrRef(true);
                tCell* wCell=wItem->Cell();
                if (wCell!=nullptr) cout << " Cell =" << wCell->Value();
                cout << endl;
            } else {
                cout << "    -> nullptr" << endl;
            }
        //cout << Sheet()->Name() << ":" << Sheet()->WorkBook()->Uri() << endl;
        //cout << Debug();
        }
#endif
        // Excel-style literal arrays { ... } / | ... | (comma = column, semicolon = row)
        vector<tVectorVariant> wArrayRows;
        tVectorVariant wArrayCurrentRow;
		tIndex wArrayDepth = 0;
		tVectorItemFormula* wVectorItemFormula = wFormula->VectorItemFormula();
		const tIndex wFormulaSize = static_cast<tIndex>(wVectorItemFormula->size());
		
        // Build an in-memory literal array from wArrayRows / wArrayCurrentRow (used by the LeftCurly inner loop
        // and the orphan RightCurly path). The literal is materialized as a transient t_Array on the evaluation
        // stack, NOT spilled to the grid at the formula origin. This is what lets several literals coexist in one
        // formula (e.g. SORT keys {9,8,6,1}/{-1,…} plus a SWITCH header {"TEAM",…}) and lets a nested literal be
        // consumed directly as a function argument. A top-level literal ={…} is spilled to the grid later by the
        // generic t_Array path (wSpillArrayValue), exactly like SEQUENCE / SORT / UNIQUE / FILTER.
		auto wFinalizeLiteralArray = [&](tIndex sRightCurlyIdx) -> void {
			(void)sRightCurlyIdx;
			if (wArrayDepth != 1) {
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				return;
			}
			if (!wArrayCurrentRow.empty()) {
				wArrayRows.push_back(wArrayCurrentRow);
				wArrayCurrentRow.clear();
			}
			wArrayDepth = 0;
			if (wArrayRows.empty()) {
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				return;
			}
			const tIndex wW = static_cast<tIndex>(wArrayRows[0].size());
			tBool wJagged = false;
			for (const auto& wRow : wArrayRows) {
				if (static_cast<tIndex>(wRow.size()) != wW) {
					wJagged = true;
					break;
				}
			}
			if (wJagged || wW == 0) {
				wArrayRows.clear();
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				return;
			}
			const tIndex wH = static_cast<tIndex>(wArrayRows.size());
			if (wH == 1 && wW == 1) {
				// A 1x1 literal is a plain scalar (Excel ={42} == 42): avoid a needless 1x1 spill.
				tVariant wOnly = wArrayRows[0][0];
				wArrayRows.clear();
				wStackElems.push(tStackElem(wOnly));
				return;
			}
			tArrayValue* wLit = new tArrayValue(wH, wW);
			for (tIndex rr = 0; rr < wH; rr++) {
				for (tIndex cc = 0; cc < wW; cc++) {
					wLit->At(rr, cc) = wArrayRows[rr][cc];
				}
			}
			wArrayRows.clear();
			wStackElems.push(tStackElem(wLit));
		};

		// --- Short-circuit IF support -------------------------------------------------------------------
		// Number of m_VectorRef slots an opcode consumes at eval. This MUST mirror the wIndice++ sites in the
		// Identifier / Cell / Attribute / Range / DynamicRange cases below; it lets the skip logic keep wIndice
		// aligned when an IF branch is not executed. Invariant: wIndice advances over EVERY ref in lexical order,
		// whether the ref is executed (wIndice++ in its case) or skipped (wIndice += here).
		auto wRefConsumed = [](tKind sKind) -> tIndex {
			switch (sKind) {
				case tKind::Identifier:
				case tKind::Cell:
				case tKind::SpillRef:
				case tKind::Attribute:
				case tKind::Range:
				case tKind::DynamicRangeRight:
				case tKind::DynamicRangeLeft:
					return 1;
				default:
					return 0;
			}
		};
		// Skip a not-taken IF branch starting at sFrom, advancing wIndice past its refs (including any nested
		// IF branches). Returns the index of the matching IfElse (when sStopAtElse) or the matching IfEnd, at
		// the same nesting depth.
		auto wSkipIfBranch = [&](tIndex sFrom, tBool sStopAtElse) -> tIndex {
			tIndex wDepth = 0;
			tIndex wIdx = sFrom;
			for (; wIdx < wFormulaSize; ++wIdx) {
				const tKind wK = (*wVectorItemFormula)[wIdx].Kind();
				if (wK == tKind::IfCond) {
					wDepth++;
				} else if (wK == tKind::IfEnd) {
					if (wDepth == 0) { return wIdx; }
					wDepth--;
				} else if (wK == tKind::IfElse) {
					if (wDepth == 0 && sStopAtElse) { return wIdx; }
				} else {
					wIndice += wRefConsumed(wK);
				}
			}
			return wIdx;
		};
		// Excel truthiness for an IF condition (mirrors IsConditionTrue in SkFunctionLogical.cpp).
		auto wIfConditionTrue = [](const tVariant& sValue) -> tBool {
			if (sValue.IsNull()) return false;
			if (sValue.IsError()) return false;
			if (sValue.IsBool()) return sValue.Bool();
			if (sValue.IsInt()) return (sValue.Int() != 0);
			if (sValue.IsDouble()) { tDouble d = sValue.Double(); return (d != 0.0 && !std::isnan(d)); }
			if (sValue.IsString()) {
				tString s = sValue.String();
				if (s.empty()) return false;
				if (s == "0" || s == "0.0") return false;
				if (s == "false" || s == "FALSE") return false;
				return true;
			}
			return sValue.Bool();
		};

        // Lopp on formula ====================================================
		for (tIndex wFormulaIdx = 0; wFormulaIdx < wFormulaSize; ++wFormulaIdx) {
			const tItemFormula& wItem = (*wVectorItemFormula)[wFormulaIdx];
#ifdef debugcalculate
			cout << " Token:" << wItem.Kind() << ":" << wItem.Value() << " stack=" << wStackElems.size() << endl;
#ifdef debugcalculate
            cout << " Stack" << endl;
            tIndex wIndexStack=0;
            if (!wStackElems.empty()) {
                tStackElems wStackCopy(wStackElems);
                while (!wStackCopy.empty()) {
                    cout << "     " << wIndexStack++ << ":[" << wStackCopy.top().Debug() << "]" << endl;
                    wStackCopy.pop();
                }
            }
   
#endif         
#endif
			switch (wItem.Kind()) {
			// Operator
			case tKind::Plus: 
			case tKind::Minus: 
			case tKind::Times: 
			case tKind::Divide:
            case tKind::UnaryMinus: 
			case tKind::Equal:
			case tKind::NotEqual:
			case tKind::GreaterThan:
			case tKind::GreaterThanOrEqual:
			case tKind::LessThan:
			case tKind::LessThanOrEqual:
	        case tKind::Ampersand: {
                // OPTIMIZATION: Save top value before pop to avoid double access
                tStackElem wStackElemWork=wStackElems.top();
                wVariantWork = wStackElemWork.Value();
                wStackElems.pop();
                // When matrix spill runs, do not fall through to scalar +/-/etc. on first-cell values (would push 0+0 and corrupt stack).
                tBool wMatrixOperatorDone = false;
                // Exact spill rect for this op (K1 can belong to several ranges — do not use MatrixRange() here).
                tRange* wMatrixResultRange = nullptr;
                tBool wMatrixSpillMultiCell = false;
                tBool wTopClass=false;

                tStackElem wStackElemTop;

                tVariant wTopValue; // Save top value to avoid repeated top() calls
                if (!wStackElems.empty()) {
                    wStackElemTop=wStackElems.top();
                    wTopValue = wStackElemTop.Value();
                    // Class-like only for a real variant class on stack; do NOT use Value().IsClass() for Range/Cell
                    // (Value() is first-cell value — looks like a plain number and wrongly enables the scalar path).
                    wTopClass = (wStackElemTop.Type() == tStackType::t_Variant) && wTopValue.IsClass();
                }

                // Unary minus on a scalar operand must run before the matrix(Range||Range) block.
                // Example: MATCH(25,C20:C24,-1) pushes 25, range C20:C24, then 1, then UnaryMinus.
                // After popping work=1, the stack still holds the lookup range below — the matrix guard
                // would see a Range on top and treat UnaryMinus as "matrix op scalar" on that range (wrong).
                if (wItem.Kind() == tKind::UnaryMinus) {
                    const tBool wWorkScalarUnary =
                        ((wStackElemWork.Type() == tStackType::t_Variant) && (!wVariantWork.IsClass())) ||
                        (wStackElemWork.Type() == tStackType::t_Cell) ||
                        (wStackElemWork.Type() == tStackType::t_Range &&
                         wStackElemWork.Range() != nullptr && wStackElemWork.Range()->IsCell());
                    if (wWorkScalarUnary && (!wVariantWork.IsClass())) {
                        wStackElems.push(tStackElem(wVariantWork * -1));
                        break;
                    }
                }

                // In-memory array operand ====================================
                // Element-wise arithmetic/comparison when at least one operand is an in-memory array
                // (tStackType::t_Array). This is what lets FILTER combine several boolean masks, e.g.
                // FILTER(range, (A1:A5>1)*(B1:B5<5)): each comparison already produced a t_Array here, and
                // this block multiplies/adds them element-wise into a new t_Array (TRUE*TRUE=1, AND; +, OR).
                // The scalar path below would collapse a t_Array to its first cell (Value()), so intercept it.
                {
                    const tBool wWorkIsArray =
                        (wStackElemWork.Type() == tStackType::t_Array) && (wStackElemWork.Array() != nullptr);
                    const tBool wTopIsArray =
                        (!wStackElems.empty() && wStackElemTop.Type() == tStackType::t_Array) &&
                        (wStackElemTop.Array() != nullptr);
                    if (wWorkIsArray || wTopIsArray) {
                        const tKind wOpKind = wItem.Kind();
                        const bool wIsArith =
                            (wOpKind == tKind::Plus) || (wOpKind == tKind::Minus) ||
                            (wOpKind == tKind::Times) || (wOpKind == tKind::Divide) ||
                            (wOpKind == tKind::Ampersand);
                        const bool wIsCmp =
                            (wOpKind == tKind::Equal) || (wOpKind == tKind::NotEqual) ||
                            (wOpKind == tKind::GreaterThan) || (wOpKind == tKind::GreaterThanOrEqual) ||
                            (wOpKind == tKind::LessThan) || (wOpKind == tKind::LessThanOrEqual);
                        if (wIsArith || wIsCmp) {
                            // Materialize each operand as an array view: an in-memory array uses its own storage, a
                            // multi-cell range is read into a temporary, and a scalar (variant / single cell) stays
                            // nullptr and falls back to its scalar value. One path then handles array op array (with
                            // Excel broadcasting), array op range, and array op scalar.
                            tArrayValue wLeftTmp;
                            tArrayValue wRightTmp;
                            tArrayValue* wLeftArr = nullptr;
                            tArrayValue* wRightArr = nullptr;
                            auto wAsArray = [](const tStackElem& sE, tArrayValue& sTmp, tArrayValue*& sArr) {
                                if (sE.Type() == tStackType::t_Array && sE.Array() != nullptr) {
                                    sArr = sE.Array();
                                    return;
                                }
                                if (sE.Type() == tStackType::t_Range && sE.Range() != nullptr &&
                                    !sE.Range()->IsCell()) {
                                    tRange* wR = sE.Range();
                                    tColRowCellRange* wCr = wR->ColRowCellRange();
                                    const tIndex wTop = wR->TopIndex();
                                    const tIndex wLeft = wR->LeftIndex();
                                    const tIndex wBot = wR->IterateBottom();
                                    const tIndex wRight = wR->IterateRight();
                                    if (wCr != nullptr && wBot >= wTop && wRight >= wLeft) {
                                        const tIndex wRows = wBot - wTop + 1;
                                        const tIndex wCols = wRight - wLeft + 1;
                                        sTmp = tArrayValue(wRows, wCols);
                                        for (tIndex r = 0; r < wRows; r++) {
                                            for (tIndex c = 0; c < wCols; c++) {
                                                tCell* wc = wCr->Cell(wTop + r, wLeft + c);
                                                if (wc != nullptr) sTmp.At(r, c) = wc->CalculableValue();
                                            }
                                        }
                                        sArr = &sTmp;
                                    }
                                }
                            };
                            wAsArray(wStackElemTop, wLeftTmp, wLeftArr);
                            wAsArray(wStackElemWork, wRightTmp, wRightArr);

                            const tIndex wLH = wLeftArr ? wLeftArr->m_Rows : 1;
                            const tIndex wLW = wLeftArr ? wLeftArr->m_Cols : 1;
                            const tIndex wRH = wRightArr ? wRightArr->m_Rows : 1;
                            const tIndex wRW = wRightArr ? wRightArr->m_Cols : 1;
                            const tIndex wAH = (wLH > wRH) ? wLH : wRH;
                            const tIndex wAW = (wLW > wRW) ? wLW : wRW;
                            // Excel broadcasting: each dimension of every operand must be 1 or the common maximum.
                            const tBool wDimBad =
                                !((wLH == 1 || wLH == wAH) && (wLW == 1 || wLW == wAW) &&
                                  (wRH == 1 || wRH == wAH) && (wRW == 1 || wRW == wAW));
                            if (wDimBad || wAH <= 0 || wAW <= 0) {
                                if (!wStackElems.empty()) wStackElems.pop();
                                wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                                break;
                            }
                            tArrayValue* wOut = new tArrayValue(wAH, wAW);
                            for (tIndex r = 0; r < wAH; r++) {
                                for (tIndex c = 0; c < wAW; c++) {
                                    tVariant wL = wLeftArr ? wLeftArr->At(wLH == 1 ? 0 : r, wLW == 1 ? 0 : c) : wTopValue;
                                    tVariant wR = wRightArr ? wRightArr->At(wRH == 1 ? 0 : r, wRW == 1 ? 0 : c) : wVariantWork;
                                    tVariant wV;
                                    switch (wOpKind) {
                                        case tKind::Plus:               wV = wL + wR; break;
                                        case tKind::Minus:              wV = wL - wR; break;
                                        case tKind::Times:              wV = wL * wR; break;
                                        case tKind::Divide:             wV = wL / wR; break;
                                        case tKind::Ampersand:          wV = Ampersand(wL, wR); break;
                                        case tKind::Equal:              wV = tVariant((tBool)(wL == wR)); break;
                                        case tKind::NotEqual:           wV = tVariant((tBool)!(wL == wR)); break;
                                        case tKind::GreaterThan:        wV = tVariant((tBool)(wL > wR)); break;
                                        case tKind::GreaterThanOrEqual: wV = tVariant((tBool)(wL >= wR)); break;
                                        case tKind::LessThan:           wV = tVariant((tBool)(wL < wR)); break;
                                        case tKind::LessThanOrEqual:    wV = tVariant((tBool)(wL <= wR)); break;
                                        default: break;
                                    }
                                    wOut->At(r, c) = wV;
                                }
                            }
                            if (!wStackElems.empty()) wStackElems.pop(); // pop the left operand
                            wStackElems.push(tStackElem(wOut));
                            break;
                        }
                    }
                }

                // Matrix ======================================================
                // Unary minus on a range (e.g. -A1:B2) leaves an empty stack after pop — only wStackElemWork is valid.
                // Enter this block only when at least one operand is a multi-cell Range. Single-cell Range (A1-style)
                // stays on the scalar path below — avoids OneCell/fold/matrix setup on every simple ref (perf).
                tRange* wRangeWork = (wStackElemWork.Type() == tStackType::t_Range) ? wStackElemWork.Range() : nullptr;
                tRange* wRangeTop =
                    (!wStackElems.empty() && wStackElemTop.Type() == tStackType::t_Range) ? wStackElemTop.Range() : nullptr;
                tByte wCond = 0;
                if (wRangeWork != nullptr && !wRangeWork->IsCell()) {
                    wCond++;
                }
                if (wRangeTop != nullptr && !wRangeTop->IsCell()) {
                    wCond++;
                }
                if (wCond != 0) {
                        // Element-wise comparison with a range/matrix operand -> in-memory boolean array (t_Array).
                        // Runs before the arithmetic tMatrix path so the mask stays in memory and is NOT spilled to
                        // the grid: this is what lets FILTER(range, range>scalar) receive a real boolean mask, and it
                        // also gives Excel's =A1:A5>100 (the top-level result is spilled later by wSpillArrayValue).
                        {
                            const tKind wCmpKind = wItem.Kind();
                            const bool wIsCompare =
                                (wCmpKind == tKind::Equal) || (wCmpKind == tKind::NotEqual) ||
                                (wCmpKind == tKind::GreaterThan) || (wCmpKind == tKind::GreaterThanOrEqual) ||
                                (wCmpKind == tKind::LessThan) || (wCmpKind == tKind::LessThanOrEqual);
                            if (wIsCompare) {
                                const tBool wLeftMatrix = (wRangeTop != nullptr && !wRangeTop->IsCell());
                                const tBool wRightMatrix = (wRangeWork != nullptr && !wRangeWork->IsCell());
                                tIndex wCmpH = 1;
                                tIndex wCmpW = 1;
                                if (wLeftMatrix) {
                                    wCmpH = wRangeTop->IterateBottom() - wRangeTop->TopIndex() + 1;
                                    wCmpW = wRangeTop->IterateRight() - wRangeTop->LeftIndex() + 1;
                                } else if (wRightMatrix) {
                                    wCmpH = wRangeWork->IterateBottom() - wRangeWork->TopIndex() + 1;
                                    wCmpW = wRangeWork->IterateRight() - wRangeWork->LeftIndex() + 1;
                                }
                                // Two matrices must share dimensions (no broadcasting for comparisons here).
                                tBool wDimMismatch = false;
                                if (wLeftMatrix && wRightMatrix) {
                                    const tIndex wRH = wRangeWork->IterateBottom() - wRangeWork->TopIndex() + 1;
                                    const tIndex wRW = wRangeWork->IterateRight() - wRangeWork->LeftIndex() + 1;
                                    if (wRH != wCmpH || wRW != wCmpW) wDimMismatch = true;
                                }
                                if (wDimMismatch || wCmpH <= 0 || wCmpW <= 0) {
                                    if (!wStackElems.empty()) wStackElems.pop();
                                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                                    break;
                                }
                                auto wCmpCell = [](tRange* sRange, tIndex sR, tIndex sC) -> tVariant {
                                    if (sRange == nullptr) return(tVariant());
                                    tColRowCellRange* wCr = sRange->ColRowCellRange();
                                    if (wCr == nullptr) return(tVariant());
                                    tCell* wCell = wCr->Cell(sRange->TopIndex() + sR, sRange->LeftIndex() + sC);
                                    return((wCell != nullptr) ? wCell->CalculableValue() : tVariant());
                                };
                                tArrayValue* wCmpArr = new tArrayValue(wCmpH, wCmpW);
                                for (tIndex r = 0; r < wCmpH; r++) {
                                    for (tIndex c = 0; c < wCmpW; c++) {
                                        tVariant wL = wLeftMatrix ? wCmpCell(wRangeTop, r, c) : wTopValue;
                                        tVariant wR = wRightMatrix ? wCmpCell(wRangeWork, r, c) : wVariantWork;
                                        tBool wB = false;
                                        switch (wCmpKind) {
                                            case tKind::Equal:              wB = (wL == wR); break;
                                            case tKind::NotEqual:           wB = !(wL == wR); break;
                                            case tKind::GreaterThan:        wB = (wL > wR); break;
                                            case tKind::GreaterThanOrEqual: wB = (wL >= wR); break;
                                            case tKind::LessThan:           wB = (wL < wR); break;
                                            case tKind::LessThanOrEqual:    wB = (wL <= wR); break;
                                            default: break;
                                        }
                                        wCmpArr->At(r, c) = tVariant(wB);
                                    }
                                }
                                if (!wStackElems.empty()) wStackElems.pop(); // pop the left operand
                                wStackElems.push(tStackElem(wCmpArr));
                                break;
                            }
                        }
                        // Fold named 1-D ranges only to one scalar @ formula row/column (Excel implicit intersection)
                        // so e.g. DuréePrêt-ROWS(...) does not broadcast a named column through tMatrix. Unnamed ranges
                        // (literals, A1:A6, =A1:B2+10) keep full matrix path. OOXML array output rects skip fold.
                        // Gate: at least one operand is a named 1-D vector (avoids work on pure 2-D matrix ops).
                        if (wCond!=0) {
                            const tBool wMayFold1D =
                                (wRangeWork != nullptr && RangeUsesNamedImplicitIntersection(wRangeWork)) ||
                                (wRangeTop != nullptr && RangeUsesNamedImplicitIntersection(wRangeTop));
                            if (wMayFold1D && !CellExpectsArrayFormulaMatrixSpill(this)) {
                                auto wFoldRangeToImplicit = [&](tStackElem& ioElem, tVariant& ioVal) {
                                    if (ioElem.Type() != tStackType::t_Range) {
                                        return;
                                    }
                                    tRange* wRng = ioElem.Range();
                                    if (wRng == nullptr || wRng->IsCell() || !RangeUsesNamedImplicitIntersection(wRng)) {
                                        return;
                                    }
                                    tVariant wAt;
                                    if (RangeImplicitIntersectionValue(this, wRng, wAt)) {
                                        ioElem = tStackElem(wAt);
                                        ioVal = wAt;
                                    } else {
                                        ioElem = tStackElem(tVariant(tClassError(tTypeError::t_value, "")));
                                        ioVal = ioElem.Value();
                                    }
                                };
                                wFoldRangeToImplicit(wStackElemWork, wVariantWork);
                                if (!wStackElems.empty()) {
                                    wStackElemTop = wStackElems.top();
                                    wTopValue = wStackElemTop.Value();
                                    wFoldRangeToImplicit(wStackElemTop, wTopValue);
                                    wStackElems.pop();
                                    wStackElems.push(wStackElemTop);
                                }
                                wRangeWork = wStackElemWork.Range();
                                wRangeTop = wStackElems.empty() ? nullptr : wStackElemTop.Range();
                                wCond = 0;
                                if (wRangeWork != nullptr && !wRangeWork->IsCell()) {
                                    wCond++;
                                }
                                if (wRangeTop != nullptr && !wRangeTop->IsCell()) {
                                    wCond++;
                                }
                                if (wCond == 0 && wItem.Kind() == tKind::UnaryMinus && wStackElems.empty() &&
                                    wStackElemWork.Type() == tStackType::t_Variant && !wVariantWork.IsClass()) {
                                    wStackElems.push(tStackElem(wVariantWork * -1));
                                    break;
                                }
                            }
                        }
                        // 1 or 2 Range
                        if (wCond!=0) {
                            tFormulaNamed* wFormulaNamedOut = FormulaNamedOutputForCell(this);
                            // Native dynamic matrix re-anchor: drop the previously persisted AFO before recomputing so a
                            // resized result (e.g. editing =A1:B2+C1:D2 to reference larger ranges) is not clamped to its
                            // old footprint by ClampResultRectToArrayFormulaOutput. Only for engine-native matrices
                            // (t_MatDynamic): OOXML array-formula refs never set that bit, so their authoritative ref stays.
                            if (wFormulaNamedOut == nullptr && IsMatDynamic() && IsSpillRange()) {
                                ClearArrayFormulaOutputRect();
                                RemoveMatDynamic();
                            }
                            // UnaryMinus on a single range: =-A1:B2 -> result in formula cell extent
                            if (wItem.Kind() == tKind::UnaryMinus && wStackElemWork.Type() == tStackType::t_Range && wRangeWork != nullptr) {
                                tTempoRect wSrcRect = wRangeWork->Rect();
                                tMatrix wMatrix(wSrcRect, this, wRangeWork->Sheet()->ColRowCellRange(), wFormulaNamedOut);
                                tIndex wR = RowIndex();
                                tIndex wC = ColIndex();
                                tIndex wH = wMatrix.Height();
                                tIndex wW = wMatrix.Width();
                                if (wH > 0 && wW > 0) {
                                    tTempoRect wResultRect(wR, wC, wR + wH - 1, wC + wW - 1);
                                    wMatrix.UnaryMinus(wResultRect);
                                    wMatrixSpillMultiCell = (wH > 1 || wW > 1);
                                    wMatrixResultRange = ColRowCellRange()->EnsureRange(
                                        wResultRect.Top(), wResultRect.Left(), wResultRect.Bottom(), wResultRect.Right());
                                    wMatrixOperatorDone = true;
                                }
                            } else if (wRangeWork != nullptr && wRangeTop != nullptr) {
                                // Matrix op matrix: both operands are ranges; result in formula cell extent (E1:F2 for =A1:B2+C1:D2 in E1)
                                tTempoRect wRectWork = wRangeWork->Rect();
                                tTempoRect wRectTop = wRangeTop->Rect();
#ifdef debugmatrix
                                cout << "Calcule 2 Matrix"  << endl;
                                cout << "Matrix Work->" << wRectWork.StrRef() << endl;
                                cout << "Matrix Top ->" << wRectTop.StrRef() << endl;
#endif
                                tMatrix wMatrixWork(wRectWork, this, wRangeWork->Sheet()->ColRowCellRange(), wFormulaNamedOut);
                                tMatrix wMatrixTop(wRectTop, this, wRangeTop->Sheet()->ColRowCellRange(), wFormulaNamedOut);
                                tIndex wR = RowIndex();
                                tIndex wC = ColIndex();
                                tIndex wH = 0;
                                tIndex wW = 0;
                                if (tMatrix::BroadcastResultSize(
                                        wMatrixTop.Height(), wMatrixTop.Width(),
                                        wMatrixWork.Height(), wMatrixWork.Width(),
                                        wH, wW)) {
                                    tTempoRect wResultRect(wR, wC, wR + wH - 1, wC + wW - 1);
                                    switch (wItem.Kind()) {
                                        case tKind::Plus:
                                            wMatrixTop.Plus(&wMatrixWork, wResultRect);
                                            break;
                                        case tKind::Minus:
                                            wMatrixTop.Minus(&wMatrixWork, wResultRect);
                                            break;
                                        case tKind::Times:
                                            wMatrixTop.Multiply(&wMatrixWork, wResultRect);
                                            break;
                                        case tKind::Divide:
                                            wMatrixTop.Divide(&wMatrixWork, wResultRect);
                                            break;
                                        case tKind::Ampersand:
                                            wMatrixTop.Ampersand(&wMatrixWork, wResultRect);
                                            break;
                                        default:
                                            break;
                                    }
                                    wMatrixSpillMultiCell = (wH > 1 || wW > 1);
                                    wMatrixResultRange = ColRowCellRange()->EnsureRange(
                                        wResultRect.Top(), wResultRect.Left(), wResultRect.Bottom(), wResultRect.Right());
                                    wMatrixOperatorDone = true;
                                }
                            } else if ((wRangeTop != nullptr || wRangeWork != nullptr)) {
                                // Matrix op scalar: one range, one scalar
                                tRange* wRangeMatrix = (wRangeTop != nullptr) ? wRangeTop : wRangeWork;
                                tVariant& wScalar = (wRangeTop != nullptr) ? wVariantWork : wTopValue;
                                tBool wScalarOnLeft = (wRangeTop == nullptr);
                                tTempoRect wSrcRect = wRangeMatrix->Rect();
#ifdef debugmatrix
                                cout << "Calcule 1 Matrix"  << endl;
                                cout << "Matrix Src->" << wSrcRect.StrRef() << endl;
#endif
                                tMatrix wMatrix(wSrcRect, this, wRangeMatrix->Sheet()->ColRowCellRange(), wFormulaNamedOut);
                                tIndex wR = RowIndex();
                                tIndex wC = ColIndex();
                                tIndex wH = wMatrix.Height();
                                tIndex wW = wMatrix.Width();
                                if (wH > 0 && wW > 0) {
                                    tTempoRect wResultRect(wR, wC, wR + wH - 1, wC + wW - 1);
                                    switch (wItem.Kind()) {
                                        case tKind::Plus:   wMatrix.Plus(wScalar, wResultRect); break;
                                        case tKind::Minus:  wMatrix.Minus(wScalar, wResultRect, wScalarOnLeft); break;
                                        case tKind::Times:  wMatrix.Multiply(wScalar, wResultRect); break;
                                        case tKind::Divide: wMatrix.Divide(wScalar, wResultRect, wScalarOnLeft); break;
                                        case tKind::Ampersand: wMatrix.Ampersand(wScalar, wResultRect, wScalarOnLeft); break;
                                        default: break;
                                    }
                                    wMatrixSpillMultiCell = (wH > 1 || wW > 1);
                                    wMatrixResultRange = ColRowCellRange()->EnsureRange(
                                        wResultRect.Top(), wResultRect.Left(), wResultRect.Bottom(), wResultRect.Right());
                                    wMatrixOperatorDone = true;
                                }
                            }
                        }
                }

                if (wMatrixOperatorDone) {
                    // Push spill range for chained broadcast (e.g. row + col*7) or multi-cell result; else Value() for legacy scalar behavior.
                    // Lookahead: Plus/Minus only — chained matrix+scalar keeps the range when the next op is + or -.
                    // Use the ensured result range for multi-cell detection: last op (e.g. +8) has no following +/— but must still
                    // push the spill range so the formula result is the full matrix, not Value() of the origin cell only.
                    tBool wNextTokenIsPlusOrMinus = false;
                    if (wFormulaIdx + 1 < wFormulaSize) {
                        const tKind wNextKind = (*wVectorItemFormula)[wFormulaIdx + 1].Kind();
                        wNextTokenIsPlusOrMinus = (wNextKind == tKind::Plus) || (wNextKind == tKind::Minus);
                    }
                    tBool wResultIsMultiCell = wMatrixSpillMultiCell;
                    if (wMatrixResultRange != nullptr) {
                        // tTempoRect::Width/Height are non-const; keep a mutable copy for the call.
                        tTempoRect wOutRect = wMatrixResultRange->Rect();
                        wResultIsMultiCell =
                            (wOutRect.Width() > 1 || wOutRect.Height() > 1) || wMatrixSpillMultiCell;
                    }
                    // OOXML array ref (e.g. B8:H8): next token after + is often DATE(...), not +/- — still must push Range
                    // so later -WEEKDAY(...)+1 keeps matrix broadcast; otherwise Value() breaks the chain and B8 stays empty.
                    tBool wCellExpectsArraySpill = false;
                    if (IsSpillRange()) {
                        // Copy: tTempoRect::Top/Left/Width/Height are non-const accessors.
                        tTempoRect wAfo(ArrayFormulaOutputRect());
                        wCellExpectsArraySpill =
                            (RowIndex() == wAfo.Top() && ColIndex() == wAfo.Left() &&
                             (wAfo.Width() > 1 || wAfo.Height() > 1));
                    }
                    const tBool wPushSpillRange =
                        (wMatrixResultRange != nullptr) &&
                        (wResultIsMultiCell || wNextTokenIsPlusOrMinus || wCellExpectsArraySpill);
                    if (!wStackElems.empty()) {
                        wStackElems.pop();
                    }
                    if (wPushSpillRange) {
                        wStackElems.push(tStackElem(wMatrixResultRange));
                    } else {
                        wStackElems.push(tStackElem(Value()));
                    }
                    // Persist the native matrix spill footprint (AFO) so ReadJson restores it without a full
                    // RecalculateAll. On reload the origin recomputes; without a persisted AFO, SpillDestinationIsClear
                    // treats the cached slave values as blocking and yields #SPILL!. Mirrors the dynamic-array path.
                    // t_MatDynamic marks this AFO as engine-native so a later resize re-derives it (see the pre-compute
                    // re-anchor above) instead of being clamped like an OOXML array-formula ref.
                    // Guard: never overwrite an authoritative OOXML ref (SpillRange set at import, not native/dynamic).
                    // Native cells were normalized to !IsSpillRange() by the pre-compute re-anchor, so this only skips OOXML.
                    if (wResultIsMultiCell && wMatrixResultRange != nullptr &&
                        FormulaNamedOutputForCell(this) == nullptr &&
                        !(IsSpillRange() && !IsMatDynamic())) {
                        tTempoRect wAfoRect = wMatrixResultRange->Rect();
                        if (IsSpillRange()) {
                            ClearArrayFormulaOutputRect();
                        }
                        // SetMatDynamic() before SetSpillRect(): the latter prunes below the AFO, and t_MatDynamic keeps
                        // that prune inside the AFO so a native spill never clears value cells below it.
                        SetMatDynamic();
                        SetSpillRect(wAfoRect.Top(), wAfoRect.Left(), wAfoRect.Bottom(), wAfoRect.Right());
                    }
                    break;
                }

                // Common case ================================================
                // Scalar-like operands: plain Variant, Cell ref, or single-cell Range (multi-cell Range uses matrix path above).
                auto wScalarLikeOperand = [](const tStackElem& sEl, const tVariant& sVal) -> tBool {
                    switch (sEl.Type()) {
                    case tStackType::t_Variant:
                        return !sVal.IsClass();
                    case tStackType::t_Cell:
                        return true;
                    case tStackType::t_Range: {
                        tRange* wR = sEl.Range();
                        return wR != nullptr && wR->IsCell();
                    }
                    default:
                        return false;
                    }
                };

                // Unary minus: valid cases handled above (scalar before matrix block) or matrix range path.
                if (wItem.Kind() == tKind::UnaryMinus) {
                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                    break;
                }
                if (!wStackElems.empty() &&
                    wScalarLikeOperand(wStackElemWork, wVariantWork) &&
                    wScalarLikeOperand(wStackElemTop, wTopValue) &&
                    (!wTopClass) && (!wVariantWork.IsClass())) {
                    // Binary: left = top, right = popped work (wTopValue + wVariantWork)
                    tVariant wValue;
                    switch (wItem.Kind()) {
                        case tKind::Ampersand: {
                            // Excel "&" coerces both operands to text. Routing it
                            // through operator+ would return #VALUE! for cases
                            // like "label "&(year-1) where one side is numeric.
                            wValue = Ampersand(wTopValue, wVariantWork);
                            break;
                        }
                        case tKind::Plus: {
                            wValue = wTopValue + wVariantWork;
                            break;
                        }
                        case tKind::Minus: {
                            wValue = wTopValue - wVariantWork;
                            break;
                        }
                        case tKind::Times: {
                            wValue = wTopValue * wVariantWork;
                            break;
                        }
                        case tKind::Divide: {
                            wValue = wTopValue / wVariantWork;
                            break;
                        }
                        case tKind::Equal: {
                            wValue = wTopValue == wVariantWork;
                            break;
                        }
                        case tKind::NotEqual: {
                            wValue = !(wTopValue == wVariantWork);
                            break;
                        }
                        case tKind::GreaterThan: {
                            wValue = wTopValue > wVariantWork;
                            break;
                        }
                        case tKind::GreaterThanOrEqual: {
                            wValue = wTopValue >= wVariantWork;
                            break;
                        }
                        case tKind::LessThan: {
                            wValue = wTopValue < wVariantWork;
                            break;
                        }
                        case tKind::LessThanOrEqual: {
                            wValue = wTopValue <= wVariantWork;
                            break;
                        }
                        default: { break; }
                    }
                    wStackElems.pop();
                    wStackElems.push(tStackElem(wValue));
                    break;
                }  else {
                    if (wStackElems.empty()) {
                        wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                        break;
                    }
                    // Class Left or Right ====================================
                    if (wStackElems.top().Value().IsClass()) {
                        // Save variant and class pointer before deleting stack element
                        tVariant wTopVariant = wStackElems.top().Value();
                        tVariantClass* wTopClass=dynamic_cast<tVariantClass*>(wTopVariant.Class());
                        if (wTopClass != nullptr) {
                            tVariant wValue;
                            switch (wItem.Kind()) {
                                case tKind::Ampersand: {
                                    // Operator_plus on a class returns a numeric
                                    // result, which would yield #VALUE! once
                                    // re-concatenated. Unwrap the class and run
                                    // the text-coercion path instead.
                                    tVariant* wInner = wTopClass->Value();
                                    wValue = (wInner != nullptr)
                                                 ? Ampersand(*wInner, wVariantWork)
                                                 : Ampersand(tVariant(), wVariantWork);
                                    break;
                                }
                                case tKind::Plus: {
                                    wValue = wTopClass->Operator_plus(true,wVariantWork);
                                    break;
                                }
                                case tKind::Minus: {
                                    wValue = wTopClass->Operator_minus(true,wVariantWork);
                                    break;
                                }
                                case tKind::Times: {
                                    wValue = wTopClass->Operator_multiply(true,wVariantWork);
                                    break;
                                }
                                case tKind::Divide: {
                                    wValue = wTopClass->Operator_divide(true,wVariantWork);
                                    break;
                                }
                                default: { break;  }
                            }
                            // OPTIMIZATION: Always replace for these operators (no need to check again)
                            wStackElems.pop();
                            wStackElems.push(tStackElem(wValue));
                            break;
                        }
                    } else {
                        if (wVariantWork.IsClass()) {
                            tVariantClass* wWorkClass=dynamic_cast<tVariantClass*>(wVariantWork.Class());
                            if (wWorkClass != nullptr) {
                                // Save top variant before deleting stack element
                                tVariant wTopVariant = wStackElems.top().Value();
                                tVariant wValue;
                                switch (wItem.Kind()) {
                                    case tKind::Ampersand: {
                                        // See twin branch above: "&" must
                                        // coerce to text, not delegate to "+".
                                        tVariant* wInner = wWorkClass->Value();
                                        wValue = (wInner != nullptr)
                                                     ? Ampersand(wTopVariant, *wInner)
                                                     : Ampersand(wTopVariant, tVariant());
                                        break;
                                    }
                                    case tKind::Plus: {
                                        wValue = wWorkClass->Operator_plus(false, wTopVariant);
                                        break;
                                    }
                                    case tKind::Minus: {
                                        wValue = wWorkClass->Operator_minus(false, wTopVariant);
                                        break;
                                    }
                                    case tKind::Times: {
                                        wValue = wWorkClass->Operator_multiply(false, wTopVariant);
                                        break;
                                    }
                                    case tKind::Divide: {
                                        wValue = wWorkClass->Operator_divide(false, wTopVariant);
                                        break;
                                    }
                                    default: { break;  }
                                }
                                // OPTIMIZATION: Always replace for these operators (no need to check again)
                                wStackElems.pop();
                                wStackElems.push(tStackElem(wValue));
                            }
                        }
                    }
                }
				break;
			}
			// Literal array delimiters (see SkLemonSpreadSheet.y: openbrace / openpipe, RIGHTCURLY / PIPE)
			case tKind::LeftCurly: {
				if (wArrayDepth != 0) {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
					break;
				}
				wArrayRows.clear();
				wArrayCurrentRow.clear();
				wArrayDepth = 1;
				// Walk tokens until RightCurly (see SkLemonSpreadSheet.y listliteral); skip that span in the outer loop.
				for (tIndex wInner = wFormulaIdx + 1; wInner < wFormulaSize; ++wInner) {
					const tItemFormula& wInnerItem = (*wVectorItemFormula)[wInner];
					if (wInnerItem.Kind() == tKind::RightCurly) {
						wFormulaIdx = wInner;
						wFinalizeLiteralArray(wInner);
						goto after_left_curly;
					}
					switch (wInnerItem.Kind()) {
					case tKind::Comma:
						// Next cell on same row (Excel column separator inside { ... })
						break;
					case tKind::Semicolon:
						wArrayRows.push_back(wArrayCurrentRow);
						wArrayCurrentRow.clear();
						break;
					case tKind::Integer:
					case tKind::Float:
					case tKind::Bool:
					case tKind::LabelDouble:
					case tKind::LabelSimple:
						wArrayCurrentRow.push_back(wInnerItem.Value());
						break;
					default: {
						wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
						wArrayDepth = 0;
						wArrayRows.clear();
						wArrayCurrentRow.clear();
						for (; wInner < wFormulaSize; ++wInner) {
							if ((*wVectorItemFormula)[wInner].Kind() == tKind::RightCurly) {
								wFormulaIdx = wInner;
								goto after_left_curly;
							}
						}
						wFormulaIdx = wFormulaSize - 1;
						goto after_left_curly;
					}
					}
				}
				wArrayDepth = 0;
				wArrayRows.clear();
				wArrayCurrentRow.clear();
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				wFormulaIdx = wFormulaSize - 1;
after_left_curly:
				break;
			}
			case tKind::RightCurly: {
				// Orphan closing (normal path consumes literal inside LeftCurly).
				wFinalizeLiteralArray(wFormulaIdx);
				break;
			}
			case tKind::Comma: {
				// Commas inside { ... } are consumed in the LeftCurly inner loop; any stray comma here is invalid.
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				break;
			}
			case tKind::Semicolon: {
				// Same as Comma: row separators inside literals are handled inside LeftCurly.
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				break;
			}
			// Constant
			case tKind::Integer:
			case tKind::Float:
            case tKind::Bool:
			case tKind::LabelDouble:
			case tKind::LabelSimple: {
				// Constants inside { ... } are handled only in the LeftCurly inner loop; wArrayDepth is never > 0 here.
				wStackElems.push(tStackElem(wItem.Value()));
				break;
			}
			case tKind::ErrorRef: {
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
				break;
			}
			case tKind::ErrorName: {
				wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_name, ""))));
				break;
			}
			case tKind::Identifier: {
				tString wName = wItem.Value().String();
				tItem* wItem = Ref(wIndice);
				if (wItem != nullptr) {
					tRange* wRange = wItem->Range();
					if (wRange != nullptr) {
						// Add Range as dependency to ensure it's calculated before this cell
						// This is important for named ranges with formulas (like DébutPrêt)
						wRange->AddDependent(this);
						// Also add the cell itself as a direct dependency if it's a single cell range
						// This ensures the cell is calculated before this cell
						if (wRange->IsCell() || wRange->IsCell()) {
							tCell* wRangeCell = wRange->Cell();
							if (wRangeCell != nullptr) {
								wRangeCell->AddDependent(this);
							}
						}
						wStackElems.push(tStackElem(wRange));
					}
					else {
						wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_name, ""))));
					}
				}
				else {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_name, ""))));
				}
				wIndice++;
				break;
			}
			// Cell
            case tKind::Attribute:
			case tKind::Cell: {
				if (wIndice < m_VectorRef.size()) {
#ifdef _DEBUGSK
					if (wIndice >= m_VectorRef.size()) {
						tStringStream wStream;
						wStream << "throw: On 	cell " << StrRef() << " m_VectorRef.size())=" << m_VectorRef.size() << " Index=" << wIndice << " !";
						throw(tExceptionInternalError(wStream.str()));
					}
#endif

                    // ATTRIBUTE ?

					tItem* wItem = Ref(wIndice);
					wIndice++;
					if (wItem != nullptr) {
						tCell* wCell = wItem->Cell();
						if (wCell != nullptr) {
#ifdef debugcalculate
/*
                            cout << "--> Cell(" << wCell->StrRef(true) << ")=";
                            if (wCell->Value().IsClass()) {
                                tCellClass* wCellClass=dynamic_cast<tCellClass*>(wCell->Value().Class());
                                if (wCellClass!=nullptr) {
                                    cout << wCellClass->Debug();
                                } else {
                                    cout << wCell->Value() << ":" << wItem;
                                }
                            } else {
                                cout << wCell->Value() << ":" << wItem;
                            }
*/
#endif
                            // Late-bind named-formula spill: when SkLemonInterface::PushID could not yet resolve a
                            // named formula to its multi-cell spill (typical on JSON load: dependents are compiled
                            // before lstAnnées finished computing), it falls back to pushing the FormulaNamed host
                            // cell on _$$. By the time we evaluate the dependent the named formula has run, so we
                            // upgrade the stack element from t_Cell to t_Range here. Without this, MATCH /COUNTA
                            // / SUM / ... reject the lone _$$ cell with #ARG (regression on Budget.xlsx — Apr 2026).
                            tFormulaNamed* wFn = FormulaNamedOutputForCell(wCell);
                            if (wFn != nullptr) {
                                // Excel: names that use ROW()/COLUMN() (or depend on such names) are
                                // evaluated in the calling cell. The _$$ host cell cannot hold one value
                                // per caller (loan templates: NuméroPaiement = ROW()-LigneEnTête).
                                if (!wFn->IsLambda() && wCell != this && wFn->IsCallerRelative()) {
                                    tCell* wCaller = this;
                                    tSheet* wSheet = Sheet();
                                    tWorkBook* wWB = (wSheet != nullptr) ? wSheet->WorkBook() : nullptr;
                                    if (wWB != nullptr && wWB->NamedFormulaCaller() != nullptr) {
                                        wCaller = wWB->NamedFormulaCaller();
                                    }
                                    wStackElems.push(tStackElem(wFn->EvaluateAtCaller(wCaller)));
                                    break;
                                }
                                tRange* wSpillRange = wFn->SpillRange();
                                // JSON load compiles dependents as the host cell (SpillRange still null).
                                // Force-eval the definition once so JoursEtSemaines+DATE sees the 6×7 spill.
                                // Do not retry on every dependent: scalar names never get a multi-cell
                                // SpillRange, and Budget-style MATCH(lst*) would recompute the name N times.
                                if ((wSpillRange == nullptr || wSpillRange->IsCell()) && wCell != this
                                    && wCell->Path() == 0 && !wFn->IsLambda()
                                    && !wFn->LateBindForceTried()) {
                                    wFn->SetLateBindForceTried();
                                    if (wCell->Formula() == nullptr) {
                                        (void)wFn->Compil("", false);
                                    }
                                    if (wCell->Formula() != nullptr) {
                                        wCell->InternalCalculation();
                                        wSpillRange = wFn->SpillRange();
                                    }
                                }
#ifdef diagcalc
                                std::cerr << "[diag] InternalCalc late-bind for cell="
                                          << wCell->StrRef(true) << " fn=" << wFn->Name()
                                          << " spillRange=" << (wSpillRange != nullptr ? wSpillRange->StrRef() : tString("<null>"))
                                          << std::endl;
                                std::cerr.flush();
#endif
                                if (wSpillRange != nullptr && !wSpillRange->IsCell()) {
                                    wStackElems.push(tStackElem(wSpillRange));
                                    break;
                                }
                            }
                            // Not propagate CellClass if necessary
                            wStackElems.push(tStackElem(wCell));
						}
						else {
                            //// One Cell
                            //tRange* wRange=wItem->Range();
                            //cout << wRange->StrRef() << endl;
                            //if (wRange!=nullptr) {
                            //   wStackElems.push(tStackElem(wRange));
                            //} else {
                               wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
                            //}
						}
					}
					else {
						wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
					}
				}
			    else {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
			    }
			
			break;
			}
			// Excel D14#: same VectorRef slot as Cell, expand to the origin's spill or #REF!.
			case tKind::SpillRef: {
				if (wIndice < static_cast<tIndex>(m_VectorRef.size())) {
					tItem* wItem = Ref(wIndice);
					wIndice++;
					tCell* wCell = (wItem != nullptr) ? wItem->Cell() : nullptr;
					tRange* wSpill = SpillRangeForHashOperator(wCell);
					if (wSpill != nullptr) {
						wStackElems.push(tStackElem(wSpill));
					} else {
						wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
					}
				} else {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
				}
				break;
			}
			// Range (value may be ref index when from PushID named range, else use wIndice)
			case tKind::Range: {
				tIndex wRefIndex = wIndice;
                wIndice++;
				// Invalid ref (e.g. #REF! after range delete): VectorRef may be empty; push #REF! and continue
				if (wRefIndex >= static_cast<tIndex>(m_VectorRef.size())) {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
					break;
				}
				tItem* wItem = Ref(wRefIndex);
				if (wItem != nullptr) {
					tRange* wRange = wItem->Range();
					if (wRange != nullptr) {
						wStackElems.push(tStackElem(wRange));
					}
					else {
						wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
					}
				}
 				else {
                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
				}
				break;
			}
			// Dynamic range: left = static (from ref index in value or ref at wIndice), right = from stack (e.g. A1:INDEX(...))
			case tKind::DynamicRangeRight: {
                tItem* wItem = Ref(wIndice);
                wIndice++;
				tStackElem wRight = wStackElems.top();
				wStackElems.pop();
          
    
                // Get left cell
                tCell* wCellLeft=wItem->Cell();
                tCell* wCellRight=wRight.Cell();
                tRange* wRange=Sheet()->ColRowCellRange()->EnsureRange(wCellLeft,wCellRight);
                if (wRange!=nullptr) {
                    wStackElems.push(tStackElem(wRange));
                } else {
                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
                }
                break;
            }
            case tKind::DynamicRangeLeft: {
                tItem* wItem = Ref(wIndice);
                wIndice++;
				tStackElem wLeft = wStackElems.top();
				wStackElems.pop();
                
                // Get right cell
                tCell* wCellLeft=wLeft.Cell();
                tCell* wCellRight=wItem->Cell();
                tRange* wRange=Sheet()->ColRowCellRange()->EnsureRange(wCellLeft,wCellRight);
                if (wRange!=nullptr) {
                    wStackElems.push(tStackElem(wRange));
                } else {
                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
                }
                break;
			}         
			// Dynamic range: both bounds from stack (e.g. Function1():Function2())
			case tKind::DynamicRangeBoth: {
                tStackElem wRight = wStackElems.top();
				wStackElems.pop();
                tStackElem wLeft = wStackElems.top();
				wStackElems.pop();
                tCell* wCellLeft=wLeft.Cell();
                tCell* wCellRight=wRight.Cell();
                tRange* wRange=Sheet()->ColRowCellRange()->EnsureRange(wCellLeft,wCellRight);
                if (wRange!=nullptr) {
                    wStackElems.push(tStackElem(wRange));
                } else {
                    wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
                }           
				break;
			}
			// LET local-variable scope opcodes ================================
			case tKind::LetBeginScope: {
				wLetScopes.emplace_back();
				break;
			}
			case tKind::LetBind: {
				// Bind the current stack top (the just-evaluated value expression) to the carried name.
				tString wLetName = wLetUpper(wItem.Value().String());
				if (!wStackElems.empty() && !wLetScopes.empty()) {
					tStackElem wBound = wStackElems.top();
					wStackElems.pop();
					wLetScopes.back()[wLetName] = wBound;
				}
				break;
			}
			case tKind::LetVarRef: {
				// Resolve a LET local, searching innermost scope first.
				tString wLetName = wLetUpper(wItem.Value().String());
				tBool wFound = false;
				for (auto wIt = wLetScopes.rbegin(); wIt != wLetScopes.rend(); ++wIt) {
					auto wHit = wIt->find(wLetName);
					if (wHit != wIt->end()) {
						wStackElems.push(wHit->second);
						wFound = true;
						break;
					}
				}
				if (!wFound) {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_name, ""))));
				}
				break;
			}
			case tKind::LetEndScope: {
				// Drop the local scope; the body result remains on the evaluation stack.
				if (!wLetScopes.empty()) {
					wLetScopes.pop_back();
				}
				break;
			}
			case tKind::LambdaRef: {
				// Push a first-class LAMBDA value for the carried named-lambda name (higher-order functions).
				tString wLambdaName = wLetUpper(wItem.Value().String());
				tFormulaNamed* wLambda = nullptr;
				tWorkBook* wWb = WorkBook();
				if (wWb != nullptr) {
					wLambda = wWb->FindFormulaNamed(wLambdaName);
				}
				if (wLambda != nullptr && wLambda->IsLambda()) {
					// Closure: if this (inline) lambda captures outer names, snapshot their current values now so
					// higher-order functions (MAP/REDUCE/SCAN) can bind them when they invoke the lambda later.
					if (wLambda->HasCapturedNames() && !wLetScopes.empty()) {
						std::map<tString, tStackElem> wCaptured;
						BuildCapturedScope(wLambda->CapturedNames(), wLetScopes, wCaptured);
						wStackElems.push(tStackElem(wLambda, wCaptured));
					} else {
						wStackElems.push(tStackElem(wLambda));
					}
				} else {
					wStackElems.push(tStackElem(tVariant(tClassError(tTypeError::t_name, wLambdaName))));
				}
				break;
			}
			case tKind::IfCond: {
				// Short-circuit IF: pop the condition and evaluate only the taken branch. The bytecode layout is
				// [cond] IfCond [then] IfElse [else] IfEnd (the else-branch is a Bool FALSE when IF has 2 args).
				tVariant wCond;
				if (!wStackElems.empty()) {
					wCond = wStackElems.top().Value();
					wStackElems.pop();
				}
				if (wCond.IsError()) {
					// Propagate the error and skip BOTH branches.
					tIndex wEnd = wSkipIfBranch(wFormulaIdx + 1, false);
					wStackElems.push(tStackElem(wCond));
					wFormulaIdx = wEnd; // loop ++ moves past IfEnd
				} else if (wIfConditionTrue(wCond)) {
					// Execute the then-branch normally; the IfElse case will skip the else-branch.
				} else {
					// Skip the then-branch; execution resumes at the else-branch (just after IfElse).
					tIndex wElse = wSkipIfBranch(wFormulaIdx + 1, true);
					wFormulaIdx = wElse; // loop ++ moves to the first opcode of the else-branch
				}
				break;
			}
			case tKind::IfElse: {
				// Reached during normal execution => the then-branch ran (condition true). Skip the else-branch.
				tIndex wEnd = wSkipIfBranch(wFormulaIdx + 1, false);
				wFormulaIdx = wEnd; // loop ++ moves past IfEnd
				break;
			}
			case tKind::IfEnd: {
				// End marker: the value of the chosen branch is already on the evaluation stack.
				break;
			}
			// Function
			case tKind::Function: {
				// Get number of arguments from Extra() (set by parser during formula parsing)
				// The parser counts arguments during parsing and stores the count in Extra()
#ifdef debugcalculate
				tShort wExtraNbArg = wItem.Value().Extra();
                tSize wNbArg = static_cast<tSize>(wExtraNbArg);
                cout << " Function:" << wItem.Value().String() << " stack size=" << wStackElems.size() << " NbArg=" << wNbArg << " (from Extra=" << wExtraNbArg << ")" << endl;
#endif
				wStackElems.push(CallFunction(wFormula,&wItem, &wStackElems, &wLetScopes));
				break;
			}
            case tKind::Percent: {
#ifdef debugcalculate
                std::cout <<  m_Value;
#endif
                // Push this Formula Conditional
                wStackElems.push(tStackElem(m_Value));
                break;
            }
			default: {
				std::cout << "Error " << std::setw(12) << wItem.Kind()  << endl;
				break;
			}
			}
		}
		if (wStackElems.size() != 1) {
			tStringStream wError;
			wError << "throw: tCell::Calculate () " << StrRef() << ":" << FormulaStr() << " Stack not empty ! " << endl;
            while (!wStackElems.empty()) {
                tVariant wValue = wStackElems.top().Value();
                wError << wValue << endl;
                wStackElems.pop();
            }
            
            cerr << wError.str() << endl;
            throw(tExceptionInternalError(wError.str()));
        }
        
        tStackType wTopType = wStackElems.top().Type();

        // Spill an in-memory array result (SEQUENCE, SORT, ...) into the grid at this
        // cell's origin, mirroring the literal-array path (wFinalizeLiteralArray).
        // Returns the origin (top-left) value stored in this cell, or a #SPILL!/#VALUE! error.
        auto wSpillArrayValue = [&](const tArrayValue& sArray) -> tVariant {
            const tIndex wH = sArray.m_Rows;
            const tIndex wW = sArray.m_Cols;
            if (wH <= 0 || wW <= 0 || sArray.Count() <= 0) {
                return tVariant(tClassError(tTypeError::t_value, ""));
            }
            const tIndex wR0 = RowIndex();
            const tIndex wC0 = ColIndex();
            tTempoRect wArrRect(wR0, wC0, wR0 + wH - 1, wC0 + wW - 1);
            tFormulaNamed* wFormulaNamedOut = FormulaNamedOutputForCell(this);
            // A literal {…} passed as an argument (e.g. =SORT({3;1;2})) is materialized into this cell's own
            // grid range during evaluation and flags the cell as a matrix origin. Those slave cells would then
            // block the final array spill (#SPILL!). Release the cell's own matrix so its cells never self-block.
            if (IsMatOrigin()) {
                ClearMatrix();
            }
            if (wFormulaNamedOut == nullptr && (wH > 1 || wW > 1)) {
                if (!tMatrix::SpillDestinationIsClear(this, ColRowCellRange(), wArrRect)) {
                    return tVariant(tClassError(tTypeError::t_spill, ""));
                }
            }
            if (wFormulaNamedOut != nullptr) {
                wFormulaNamedOut->ResizeSpillBuffer(wH, wW);
            }
            {
                tMatrix wMatrix(wArrRect, this, nullptr, wFormulaNamedOut);
                for (tIndex rr = 0; rr < wH; rr++) {
                    for (tIndex cc = 0; cc < wW; cc++) {
                        wMatrix.SetValue(rr, cc, sArray.At(rr, cc));
                    }
                }
            }
            tRange* wArrRange = ColRowCellRange()->EnsureRange(wR0, wC0, wR0 + wH - 1, wC0 + wW - 1);
            if (wArrRange != nullptr) {
                wArrRange->SetMatOrigin();
                if (wFormulaNamedOut != nullptr) {
                    wFormulaNamedOut->SetSpillRange(wArrRange);
                }
            }
            // Persist the spill footprint (AFO) so ReadJson restores the dynamic array on load without a
            // full RecalculateAll: SetSpillRect writes a per-cell "spillrange" and the spilled cells are
            // already cached as v/t. Re-anchor cleanly on resize by releasing any previous AFO first.
            // (No OOXML clamp concern here: dynamic arrays do not go through tMatrix::Copy.)
            // SetMatDynamic() BEFORE SetSpillRect(): SetSpillRect prunes stale cells below the AFO, and t_MatDynamic
            // must be set so that prune stays inside the AFO (a native spill has no stale rows below to clear).
            if (wFormulaNamedOut == nullptr) {
                if (IsSpillRange()) {
                    ClearArrayFormulaOutputRect();
                }
                SetMatDynamic();
                SetSpillRect(wR0, wC0, wR0 + wH - 1, wC0 + wW - 1);
            }
            return sArray.At(0, 0);
        };

    	// Is Condition formula ===============================================
        if (sFormula==nullptr) {
            switch (wTopType) {
                case tStackType::t_Range: {
                    tStackElem wStackElem=wStackElems.top();
                    tRange* wRange = wStackElem.Range();
#ifdef debugcalculate
                    if (wRange != nullptr) {
                        cout << "->StackElem 1 ->" << wRange->Debug();
                    }
#endif
                    if (wRange == nullptr) {
                        m_Value.Clear();
                        break;
                    }
                    // Named 1-D: legacy @ intersection → one scalar (not full matrix spill).
                    if (RangeUsesNamedImplicitIntersection(wRange)) {
                        tVariant wAt;
                        if (RangeImplicitIntersectionValue(this, wRange, wAt)) {
                            m_Value = wAt;
                            break;
                        }
                    }
                    // Whole formula is a multi-cell range: spill copy (e.g. B1 = A1:A2 → B1:B2) via tMatrix::Copy.
                    if (!wRange->IsCell()) {
                        tFormulaNamed* wNamedOut = FormulaNamedOutputForCell(this);
                        // Native dynamic re-anchor: drop the previously persisted AFO before recompute so a resized
                        // source range is not clamped to the old footprint. OOXML refs (no t_MatDynamic) are preserved.
                        if (wNamedOut == nullptr && IsMatDynamic() && IsSpillRange()) {
                            ClearArrayFormulaOutputRect();
                            RemoveMatDynamic();
                        }
                        tTempoRect wSrcRect = wRange->Rect();
                        tMatrix wMatrix(wSrcRect, this, wRange->Sheet()->ColRowCellRange(), wNamedOut);
                        const tIndex wH = wMatrix.Height();
                        const tIndex wW = wMatrix.Width();
                        if (wH > 0 && wW > 0) {
                            const tIndex wR = RowIndex();
                            const tIndex wC = ColIndex();
                            tTempoRect wResultRect(wR, wC, wR + wH - 1, wC + wW - 1);
                            wMatrix.Copy(wResultRect);
                            // Persist the native range-copy spill footprint (AFO) so ReadJson restores it without a full
                            // RecalculateAll (same reload #SPILL! fix as the matrix-operator path). t_MatDynamic marks it
                            // engine-native so a later resize re-derives instead of clamping to the old footprint.
                            // Guard: never overwrite an authoritative OOXML ref (SpillRange set at import, not dynamic);
                            // native cells were normalized to !IsSpillRange() by the re-anchor above, so this only skips OOXML.
                            if ((wH > 1 || wW > 1) && wNamedOut == nullptr &&
                                !(IsSpillRange() && !IsMatDynamic())) {
                                if (IsSpillRange()) {
                                    ClearArrayFormulaOutputRect();
                                }
                                // SetMatDynamic() before SetSpillRect(): keeps the internal prune inside the AFO so a
                                // native range-copy spill never clears value cells below it.
                                SetMatDynamic();
                                SetSpillRect(wR, wC, wR + wH - 1, wC + wW - 1);
                            }
                            break;
                        }
                    }
                    AssignVariantFromRangeRefForCell(this, wRange, m_Value);
                    break;
                }
                case tStackType::t_Attribute:
                case tStackType::t_Cell: {
                    tStackElem wStackElem=wStackElems.top();
                    m_Value = wStackElem.Value();
                    break;
                }
                case tStackType::t_Array: {
                    tArrayValue* wArray = wStackElems.top().Array();
                    if (wArray != nullptr) {
                        m_Value = wSpillArrayValue(*wArray);
                    } else {
                        m_Value.Clear();
                    }
                    break;
                }
                case tStackType::t_Variant:
                    m_Value=wStackElems.top().Value();
                    break;
                default: break;
            }
            wStackElems.pop();
            // Check for NaN values and convert them to errors before storing
            if (m_Value.Type() == tVariantType::t_double) {
                tDouble wDouble = m_Value.Double();
                if (std::isnan(wDouble) || std::isinf(wDouble)) {
                    m_Value.SetError(tClassError(tTypeError::t_value, ""));
                }
            }
        } else { // sFormula!=nullptr
            tVariant wResult;
        	switch (wTopType) {
                case tStackType::t_Range: {
                    tStackElem wStackElem=wStackElems.top();
                    tRange* wRange = wStackElem.Range();
                    if (wRange == nullptr) {
                        wResult.Clear();
                        break;
                    }
                    if (RangeUsesNamedImplicitIntersection(wRange)) {
                        tVariant wAt;
                        if (RangeImplicitIntersectionValue(this, wRange, wAt)) {
                            wResult = wAt;
                            break;
                        }
                    }
                    if (!wRange->IsCell()) {
                        tFormulaNamed* wNamedOut = FormulaNamedOutputForCell(this);
                        tTempoRect wSrcRect = wRange->Rect();
                        tMatrix wMatrix(wSrcRect, this, wRange->Sheet()->ColRowCellRange(), wNamedOut);
                        const tIndex wH = wMatrix.Height();
                        const tIndex wW = wMatrix.Width();
                        if (wH > 0 && wW > 0) {
                            const tIndex wR = RowIndex();
                            const tIndex wC = ColIndex();
                            tTempoRect wResultRect(wR, wC, wR + wH - 1, wC + wW - 1);
                            wResult = wMatrix.Copy(wResultRect);
                            break;
                        }
                    }
                    AssignVariantFromRangeRefForCell(this, wRange, wResult);
                    break;
                }
                case tStackType::t_Attribute:
                case tStackType::t_Cell: {
                    tStackElem wStackElem=wStackElems.top();
                    wResult = wStackElem.Value();
                    break;
                }
                case tStackType::t_Array: {
                    tArrayValue* wArray = wStackElems.top().Array();
                    if (wArray != nullptr) {
                        wResult = wSpillArrayValue(*wArray);
                    } else {
                        wResult.Clear();
                    }
                    break;
                }
                case tStackType::t_Variant:
                    wResult=wStackElems.top().Value();
                    break;
                default : break;
            }
             wStackElems.pop();
            //wStackElems.pop();
            // Check for NaN values and convert them to errors before returning
            if (wResult.Type() == tVariantType::t_double) {
                tDouble wDouble = wResult.Double();
                if (std::isnan(wDouble) || std::isinf(wDouble)) {
                    wResult.SetError(tClassError(tTypeError::t_value, ""));
                }
            }
#ifdef debugcalculateorder
                    cout <<  m_Value  << "=" << FormulaStr() << "/";
                    cout.flush();
#endif

            return(wResult);
        }
      
		

		// Is Class Root on cell ==============================================
		if (m_Value.Type() == tVariantType::t_class) {
			tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
			if (wCellClass != nullptr) wCellClass->Rooted(this);
		}
        if (tSpreadSheetContainer::IsOnCellChange()) {
            tSpreadSheetContainer::OnCellChange(this, m_Value);
        }
#ifdef debugcalculate
        cout << "  Result -->[" << m_Value << "]" << endl;
#endif

#ifdef _DEBUGSK
       if ((StrRef() == "E13")) {
           //cout << "=[" << m_Value << "]" << endl;
        }
#endif
#ifdef debugcalculateorder
        cout <<  m_Value << "/";
        cout.flush();
#endif
 		return(m_Value);
	}

	void FunctCalculate(tContainerPath* wCellCalculationPath,tCell* sCell) {
		wCellCalculationPath->Calculate(sCell);
	}

	void tCell::Calculation() {
		tContainerPath wCalculationPath;
		wCalculationPath.Calculate(this);
	}

	const tString tCell::StrRef(tBool sSheetName) const {
		tStringStream wStream;
        if (sSheetName) wStream << Sheet()->Name()<<"!";
#ifdef _DEBUGSK
        if (Col()!=nullptr) wStream << Base10ToAlpha(ColIndex());  else wStream << "#REF!";
        if (Row()!=nullptr) wStream << RowIndex();  else wStream << "#REF!";
#else
		wStream << Base10ToAlpha(ColIndex()) << RowIndex();
#endif
		return(wStream.str());
	}

	// Value ==================================================================
    void tCell::Value(const tVariant& sValue) {
        m_Value = sValue;
    }
    tVariant& tCell::Value() {
        return(m_Value);
    }

    const tVariant& tCell::Value() const {
        return(m_Value);
    }

    // For optimization don't copy ============================================
    tVariant* tCell::PtValue() { return(&m_Value); }

    const tVariant* tCell::PtValue() const { return(&m_Value); }

    tVariant& tCell::CalculableValue() {
        if (m_Value.Type() == tVariantType::t_class) {
            tCellClassAttribute* wAttrClass =
                dynamic_cast<tCellClassAttribute*>(m_Value.Class());
            if (wAttrClass != nullptr && !wAttrClass->IsCalculationPropagation()) {
                return(wAttrClass->CalculableValue());
            }
            tCellClass* wCellClass = dynamic_cast<tCellClass*>(m_Value.Class());
            if (wCellClass != nullptr && !wCellClass->IsCalculationPropagation()) {
                tVariant* wInner = m_Value.Class()->Value();
                if (wInner != nullptr) {
                    return(*wInner);
                }
            }
        }
        return(m_Value);
    }

    tVariant tCell::CalculableScalarFromVariant(const tVariant& sCalculable) {
        if (sCalculable.Type() != tVariantType::t_class) {
            return sCalculable;
        }
        const tCellClassUnit* wUnit =
            dynamic_cast<const tCellClassUnit*>(sCalculable.Class());
        if (wUnit != nullptr) {
            const tVariant* wInner = sCalculable.Class()->Value();
            if (wInner != nullptr) {
                return *wInner;
            }
        }
        return sCalculable;
    }

    tVariant tCell::CalculableScalarValue() const {
        return CalculableScalarFromVariant(CalculableValue());
    }

    const tVariant& tCell::CalculableValue() const {
        if (m_Value.Type() == tVariantType::t_class) {
            const tCellClassAttribute* wAttrClass =
                dynamic_cast<const tCellClassAttribute*>(m_Value.Class());
            if (wAttrClass != nullptr && !wAttrClass->IsCalculationPropagation()) {
                return(wAttrClass->CalculableValue());
            }
            const tCellClass* wCellClass = dynamic_cast<const tCellClass*>(m_Value.Class());
            if (wCellClass != nullptr && !wCellClass->IsCalculationPropagation()) {
                const tVariant* wValue = m_Value.Class()->Value();
                if (wValue != nullptr) {
                    return(*wValue);
                }
            }
        }
        return(m_Value);
    }

    tCellClass* tCell::Class() const {
        if (m_Value.Type() == tVariantType::t_class) {
            tCellClass* wCellClass = dynamic_cast<tCellClass*>(m_Value.Class());
            return(wCellClass);
        }
        return(nullptr);

    }

	tCellClassAttribute* tCell::ClassAttribute() const {
		if (m_Value.Type() == tVariantType::t_class) {
			tCellClassAttribute* wCellClassAttribute = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
			return(wCellClassAttribute);
		}
		return(nullptr);
	}

	tCellAttribute* tCell::CellAttribute(tString sName) const {
		if (m_Value.Type() == tVariantType::t_class) {
			tCellClassAttribute* wCellClassAttribute = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
			return(wCellClassAttribute->CellAttribute(sName));
		}
		return(nullptr);
	}


	// Css ====================================================================
	tFormatRef tCell::Css() const {
		return(m_Css);
	}

	void tCell::Css(tFormatRef sFormatRef) {
        if (m_Css==-1) {
            return;
        }
		m_Css = sFormatRef;
	}

    tBool tCell::Cover() const {
        if (m_Value.IsString()) return(true);
        // JsonView uses Cover() for horizontal spill/clips into empty adjacent cells,
        // same as typical spreadsheet rules for formatted numbers and dates — not only strings.
        if (m_Value.IsNumeric()) return(true);
        if (m_Value.IsBool()) return(true);
        if (m_Value.IsDate()) return(true);
        if (m_Value.IsClass()) {
            const tCellClass* wCellClass=dynamic_cast<const tCellClass*>(m_Value.Class());
            if (wCellClass!=nullptr) {
                return(wCellClass->Cover());
            }
        }
        return(false);
    }
    
    tRange* tCell::MatrixRange() {
        if (IsMatOrigin()) {
            tWorkBook* wWB = Sheet()->WorkBook();
            if (wWB != nullptr) {
                tFormulaNamed* wFn = wWB->FindFormulaNamedByCell(this);
                if (wFn != nullptr && wFn->SpillRange() != nullptr) {
                    return wFn->SpillRange();
                }
            }
            // Prefer OOXML / .sker spill range before first FindRanges() hit: overlapping rects (e.g. CF B8:G8 vs spill B8:H8)
            // leave container order undefined — wrong extent breaks ClearMatrix() and spill recalculation (Calendar B8).
            tRange* wSpill = SpillRange();
            if (wSpill != nullptr) {
                return wSpill;
            }
        }
        if ((IsMatOrigin()) || (IsMatExtend())) {
           tColRowCellRange* wColRowCellRange=ColRowCellRange();
           tColRow::tContainerRange::tResult wContainerRange;
           ColRowCellRange()->FindRanges(this,&wContainerRange);
			for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
				tRange* wRange = wColRowCellRange->Range(wRangeAllocatorRef);
                    if (wRange!=nullptr) {
                        return(wRange);
                    }
           }
        }
        return(nullptr);
    }
         
	tCell* tCell::CellMatrixRoot()  {
        if (IsMatOrigin()) {
            return(this);
        }
        
        if (IsMatExtend()) {
            tRange* wRange=MatrixRange();
            if (wRange!=nullptr) {
                return(ColRowCellRange()->Cell(wRange->TopIndex(),wRange->LeftIndex()));
            }
        }
		return nullptr;
	}

    // When clearing this origin's spill footprint, skip cells that belong to another spill (nested block origin or
    // extend cells tied to another root) so multi-block sheets (e.g. OOXML calendar rows) are not wiped.
    static tBool SkipClearSpillCellForAnotherOrigin(tCell* sThisOrigin, tCell* sCell) {
        if (sCell == nullptr || sThisOrigin == nullptr) {
            return false;
        }
        if (sCell == sThisOrigin) {
            return false;
        }
        if (sCell->IsMatOrigin()) {
            return true;
        }
        tCell* wRoot = sCell->CellMatrixRoot();
        return wRoot != nullptr && wRoot != sThisOrigin;
    }
    
    tCell* tCell::BreakSpillForUserOverwrite() {
        // Only spill slaves: overwriting the origin replaces the formula (normal edit).
        if (!IsMatExtend()) {
            return nullptr;
        }
        tCell* wOrigin = CellMatrixRoot();
        if (wOrigin == nullptr || wOrigin == this) {
            RemoveMatExtend();
            return nullptr;
        }
        // Clear spill values + MatOrigin/MatExtend; keep the origin formula.
        wOrigin->ClearMatrix();
        // Drop AFO so SpillDestinationIsClear no longer blanket-skips OOXML interiors, and
        // native MatDynamic re-anchors on the next successful spill. User cell then blocks → #SPILL!.
        if (wOrigin->IsSpillRange()) {
            wOrigin->ClearArrayFormulaOutputRect();
        }
        if (wOrigin->IsMatDynamic()) {
            wOrigin->RemoveMatDynamic();
        }
        if (IsMatExtend()) {
            RemoveMatExtend();
        }
        return wOrigin;
    }

    void tCell::ClearMatrix() {
        tRange* wRange=MatrixRange();
        if (wRange == nullptr) {
            return;
        }
        tCell* wOrigin = CellMatrixRoot();
        if (wOrigin != nullptr) {
            tWorkBook* wWB = Sheet()->WorkBook();
            if (wWB != nullptr) {
                tFormulaNamed* wFn = wWB->FindFormulaNamedByCell(wOrigin);
                if (wFn != nullptr) {
                    wFn->ClearSpillBuffer();
                }
            }
        }
        // Cross-sheet spill (FormulaNamed whose definition resolves to a real-sheet range, e.g. lstMesures
        // -> 'Entrées des données financières'!B4:B17). The range is owned by the source sheet's allocator
        // and indexed in source-sheet coords. Iterating with this->ColRowCellRange() (the host's _$$ sheet)
        // would address foreign cells, and DeleteRangeByAllocatorRef would dereference a foreign allocator
        // slot — that is the WASM "memory access out of bounds" trap on Budget.sker recalculation. The
        // source range belongs to real data on Entrées and must NOT be torn down here; just unbind the
        // FormulaNamed spill (already done above) and leave the range intact.
        tColRowCellRange* wHostColRow = ColRowCellRange();
        tColRowCellRange* wRangeColRow = (wRange->Sheet() != nullptr) ? wRange->Sheet()->ColRowCellRange() : nullptr;
        if (wRangeColRow != nullptr && wRangeColRow != wHostColRow) {
            // Detach the FormulaNamed pointer to the source range so future MatrixRange() calls don't
            // re-resolve to it (a fresh Calculation() will SetSpillRange() again with EnsureRange).
            tWorkBook* wWB = Sheet()->WorkBook();
            if (wWB != nullptr) {
                tFormulaNamed* wFn = wWB->FindFormulaNamedByCell(this);
                if (wFn != nullptr) {
                    wFn->SetSpillRange(nullptr);
                }
            }
            return;
        }
        tCell* const wThisOrigin = this;
        tBool wIsFirst=true;
        for (tIndex wRow=wRange->TopIndex(); wRow<=wRange->IterateBottom(); wRow++) {
            for (tIndex wCol=wRange->LeftIndex(); wCol<=wRange->IterateRight(); wCol++) {
                tCell* wCell=wHostColRow->Cell(wRow,wCol);
                if (wCell!=nullptr) {
                    if (SkipClearSpillCellForAnotherOrigin(wThisOrigin, wCell)) {
                        wIsFirst = false;
                        continue;
                    }
                    if (wIsFirst) {
                        wCell->RemoveMatOrigin();
                    } else {
                        wCell->RemoveMatExtend();
                    }
                    // Spill only writes where destination cells are blank (SpillDestinationIsClear). When tearing down
                    // the spill, reset values to null so the range matches that blank state—including undo replaying
                    // the null that was saved before the spill was applied.
                    wCell->Value().Clear();
                }
                wIsFirst=false;
            }
        }
        wRange->RemoveMatOrigin();
        if (wRange->IsEmpty()) {
            wHostColRow->DeleteRangeByAllocatorRef(wRange->AllocatorRef());
        }
    }

    void tCell::ClearMatrixSpillValues() {
        tRange* wRange = MatrixRange();
        if (wRange == nullptr) {
            return;
        }
        // Cross-sheet spill (lstMesures -> 'Entrées des données financières'!B4:B17): the source range is
        // owned by the source sheet and contains real data — do not clear those values, just reset the
        // FormulaNamed spill buffer. See ClearMatrix() for the symmetric protection.
        tColRowCellRange* wHostColRow = ColRowCellRange();
        tColRowCellRange* wRangeColRow = (wRange->Sheet() != nullptr) ? wRange->Sheet()->ColRowCellRange() : nullptr;
        if (wRangeColRow != nullptr && wRangeColRow != wHostColRow) {
            tWorkBook* wWB = Sheet()->WorkBook();
            if (wWB != nullptr) {
                tFormulaNamed* wFn = wWB->FindFormulaNamedByCell(this);
                if (wFn != nullptr) {
                    wFn->ClearSpillBufferValuesToNull();
                }
            }
            return;
        }
        tCell* wOrigin = CellMatrixRoot();
        if (wOrigin == nullptr) {
            wOrigin = wHostColRow->Cell(wRange->TopIndex(), wRange->LeftIndex());
        }
        const tVariant wNull;
        for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
            for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                tCell* wCell = wHostColRow->Cell(wRow, wCol);
                if (wCell != nullptr) {
                    if (wOrigin != nullptr && SkipClearSpillCellForAnotherOrigin(wOrigin, wCell)) {
                        continue;
                    }
                    if (wOrigin != nullptr && wCell != wOrigin) {
                        wCell->ClearFormula();
                    }
                    wCell->Value(wNull);
                }
            }
        }
        if (wOrigin != nullptr) {
            tWorkBook* wWB = Sheet()->WorkBook();
            if (wWB != nullptr) {
                tFormulaNamed* wFn = wWB->FindFormulaNamedByCell(wOrigin);
                if (wFn != nullptr) {
                    wFn->ClearSpillBufferValuesToNull();
                }
            }
        }
    }

    // True if (sRow,sCol) lies inside another matrix origin's OOXML array output rect (not sThisOrigin).
    // Without this check, pruning a higher block can clear extend cells that belong to a lower block's ref
    // when MatExtend metadata still points at the upper origin after a full recalc.
    static tBool CellInsideOtherArrayFormulaOutputRect(tColRowCellRange* sColRowCellRange, tIndex sRowIndex,
                                                       tIndex sColIndex, tCell* sThisOrigin) {
        const tIndex wLastCol = sColRowCellRange->LastCol();
        for (tIndex wCol = 1; wCol <= wLastCol; ++wCol) {
            tCell* wOrigin = sColRowCellRange->Cell(sRowIndex, wCol);
            if (wOrigin == nullptr || !wOrigin->IsSpillRange()) {
                continue;
            }
            if (wOrigin == sThisOrigin) {
                continue;
            }
            tTempoRect wArOther = wOrigin->ArrayFormulaOutputRect();
            if (!wArOther.IsValid()) {
                continue;
            }
            if (wOrigin->RowIndex() != wArOther.Top() || wOrigin->ColIndex() != wArOther.Left()) {
                continue;
            }
            const tIndex oTop = wArOther.Top();
            const tIndex oBottom = wArOther.Bottom();
            const tIndex oLeft = wArOther.Left();
            const tIndex oRight = wArOther.Right();
            if (sRowIndex >= oTop && sRowIndex <= oBottom && sColIndex >= oLeft && sColIndex <= oRight) {
                return true;
            }
        }
        return false;
    }
   

    void tCell::PruneMatrixSpillOutsideArrayFormulaOutput() {
        if (!IsSpillRange()) {
            return;
        }
        tTempoRect wAr = ArrayFormulaOutputRect();
        const tIndex sTop = wAr.Top();
        const tIndex sLeft = wAr.Left();
        const tIndex sBottom = wAr.Bottom();
        const tIndex sRight = wAr.Right();
        tColRowCellRange* wColRowCellRange = ColRowCellRange();
        // First evaluation (e.g. CompilCell / UndoCellValue) may spill the full matrix before this ref exists.
        // RecalculateAll clamps writes via ClampResultRectToArrayFormulaOutput but can leave stale values in extend
        // cells outside the OOXML ref — clear spill extend cells outside the ref (also needed after RecalculateAll).
        tRange* wRange = SpillRange();
        if (wRange == nullptr) {
            return;
        }
        const tIndex wR0 = wRange->TopIndex();
        const tIndex wC0 = wRange->LeftIndex();
        const tIndex wR1 = wRange->BottomIndex();
        const tIndex wC1 = wRange->RightIndex();
        // ClampResultRectToArrayFormulaOutput can shrink the stored matrix range to the OOXML ref (often one row).
        // Stale spill cells below that ref still belong to this origin until cleared — extend scan rows so we visit them.
        // Native dynamic spills (t_MatDynamic) have AFO == full result: there is nothing stale below, so limiting the
        // scan to the AFO avoids wiping unrelated value cells below it (e.g. on ReadJson, a cell one row under a small
        // =A1:B2+C1:D2 spill whose formula is not yet recompiled would otherwise be cleared).
        static constexpr tIndex kMaxMatrixSpillScanRows = 64;
        const tIndex wLastRow = wColRowCellRange->LastRow();
        tIndex wR1Scan = wR1;
        if (!IsMatDynamic()) {
            const tIndex wTailRow = sTop + kMaxMatrixSpillScanRows - 1;
            wR1Scan = wR1 > wTailRow ? wR1 : wTailRow;
            if (wR1Scan > wLastRow) {
                wR1Scan = wLastRow;
            }
            if (wR1Scan < wR1) {
                wR1Scan = wR1;
            }
        }
        const tIndex wC0Scan = wC0 < sLeft ? wC0 : sLeft;
        const tIndex wC1Scan = wC1 > sRight ? wC1 : sRight;
        for (tIndex wR = wR0; wR <= wR1Scan; ++wR) {
            for (tIndex wC = wC0Scan; wC <= wC1Scan; ++wC) {
                if (wR >= sTop && wR <= sBottom && wC >= sLeft && wC <= sRight) {
                    continue;
                }
                tCell* wCell = wColRowCellRange->Cell(wR, wC);
                if (wCell == nullptr) {
                    continue;
                }
                if (CellInsideOtherArrayFormulaOutputRect(wColRowCellRange, wR, wC, this)) {
                    continue;
                }
                if (wCell->IsMatExtend()) {
                    tCell* wRoot = wCell->CellMatrixRoot();
                    if (wRoot != this) {
                        continue;
                    }
                    wCell->Value(tVariant());
                    wCell->RemoveMatExtend();
                } else if (!wCell->IsMatOrigin() && wCell->Formula() == nullptr
                           && !wCell->Value().IsExcelNull()) {
                    // Clamp can skip writes outside AFO but older spills may leave values without MatExtend.
                    wCell->Value(tVariant());
                }
            }
        }
    }
    
     
    tRange* tCell::SpillRange() const {
        tColRowCellRange* wColRowCellRange = ColRowCellRange();
        tColRow::tContainerRange::tResult wContainerRange;
        // FindRanges takes tCell* (legacy); lookup does not mutate the cell.
        wColRowCellRange->FindRanges(const_cast<tCell*>(this), &wContainerRange);
        // FindRanges may list non-spill ranges too; among spill rects anchored at this cell, prefer the largest
        // area (tie-break: wider). Row B8 can have CF B8:G8 and dynamic spill B8:H8; a narrower spill must not win
        // or SpillDestinationIsClear / ArrayFormulaOutputRect miss column H and block reload from cached <v>.
        tRange* wBest = nullptr;
        tIndex wBestArea = 0;
        for (tAllocatorRef wRangeAllocatorRef : wContainerRange) {
            tRange* wRange = wColRowCellRange->Range(wRangeAllocatorRef);

            if (wRange == nullptr) {
                continue;
            }
            if ((wRange->TopIndex() == RowIndex()) && (wRange->LeftIndex() == ColIndex()) && (wRange->IsSpillRange())) {
                const tIndex wH = wRange->BottomIndex() - wRange->TopIndex() + 1;
                const tIndex wW = wRange->RightIndex() - wRange->LeftIndex() + 1;
                const tIndex wArea = wH * wW;
                if (wBest == nullptr) {
                    wBest = wRange;
                    wBestArea = wArea;
                } else if (wArea > wBestArea) {
                    wBest = wRange;
                    wBestArea = wArea;
                } else if (wArea == wBestArea) {
                    const tIndex wBestW = wBest->RightIndex() - wBest->LeftIndex() + 1;
                    if (wW > wBestW) {
                        wBest = wRange;
                    }
                }
#ifdef debugmatrix
                cout <<  StrRef() << " Range Spill candidate->" << wRange->Debug() << endl;
#endif
            }
        }
#ifdef debugmatrix
        if (wBest == nullptr) {
            cout <<  StrRef() << " Range Spill-> nullptr" << endl;
        }
#endif
        return wBest;
    }

    void tCell::SetSpillRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
        tColRowCellRange* wColRowCellRange=ColRowCellRange();
        tRange* wRangeSpill=wColRowCellRange->EnsureRange(sTop,sLeft,sBottom,sRight);
        this->SetSpillRange();
        wRangeSpill->SetSpillRange();
#ifdef debugmatrix
        cout << "SetArrayFormulaOutputRect " << StrRef() << " " << wRangeSpill->Debug() << endl;
#endif
        PruneMatrixSpillOutsideArrayFormulaOutput();
    }
    
    void tCell::ClearArrayFormulaOutputRect() {
        tRange* wRangeSpill = SpillRange();
        this->RemoveSpillRange();
        if (wRangeSpill != nullptr) {
            wRangeSpill->RemoveSpillRange();
            if (wRangeSpill->IsEmpty()) {
                ColRowCellRange()->DeleteRangeByAllocatorRef(wRangeSpill->AllocatorRef());
            }
        }
    }
    
    
    tTempoRect tCell::ArrayFormulaOutputRect() const {
        static const tTempoRect s_Empty(0, 0, 0, 0);
        // Do not require IsSpillRange() on the cell — after .sker load or ClearMatrix order, the flag can be stale while
        // FindRanges still resolves the spill tRange; SpillDestinationIsClear must see the same AFO as ClampResultRectToArrayFormulaOutput.
        tRange* wRangeSpill = SpillRange();
        if (wRangeSpill != nullptr) {
#ifdef debugmatrix
            cout << "ArrayFormulaOutputRect " << StrRef() << " " << wRangeSpill->Debug() << endl;
#endif
            return wRangeSpill->Rect();
        }
#ifdef debugmatrix
        cout << "ArrayFormulaOutputRect " << StrRef() << " empty " << endl;
#endif
        return s_Empty;
    }

    void tCell::AppendCellsInSpillRange(std::vector<tCell*>& sOut) {
        tCell* wOrigin = CellMatrixRoot();
        if (wOrigin == nullptr) {
            if (IsSpillRange()) {
                tRange* wSpill = SpillRange();
                if (wSpill != nullptr && RowIndex() == wSpill->TopIndex() && ColIndex() == wSpill->LeftIndex()) {
                    wOrigin = this;
                }
            }
        }
        if (wOrigin == nullptr) {
            return;
        }
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        tBool wHaveBounds = false;
        if (wOrigin->IsSpillRange()) {
            if (tRange* wSpill = wOrigin->SpillRange()) {
                wTop = wSpill->TopIndex();
                wLeft = wSpill->LeftIndex();
                wBottom = wSpill->BottomIndex();
                wRight = wSpill->RightIndex();
                wHaveBounds = true;
            }
        }
        if (!wHaveBounds) {
            if (tRange* wMr = wOrigin->MatrixRange()) {
                wTop = wMr->TopIndex();
                wLeft = wMr->LeftIndex();
                wBottom = wMr->BottomIndex();
                wRight = wMr->RightIndex();
                wHaveBounds = true;
            }
        }
        if (!wHaveBounds) {
            return;
        }
        tColRowCellRange* wColRow = wOrigin->ColRowCellRange();
        for (tIndex wR = wTop; wR <= wBottom; ++wR) {
            for (tIndex wC = wLeft; wC <= wRight; ++wC) {
                if (tCell* wCell = wColRow->Cell(wR, wC)) {
                    sOut.push_back(wCell);
                }
            }
        }
    }

	void tCell::PushRef(tItem* sItem,tBool sDependent) {
        // sItem may be tCell or tRange 
#ifdef debugdependent
        cout << "Push ";
        if (sItem!=nullptr) cout << " on Cell " << this << " PushRef(" << StrRef() << ")" << endl;
#endif
        // Uses by calcul (reference of formula)
        m_VectorRef.push_back(sItem);
        const tBool wLinked = (sItem != nullptr) && sDependent;
        m_VectorRefAddDependent.push_back(wLinked);
       // Set Dependent Cell ==================================================
       // See SkCalculationPath (calculate all cell(s) and renge(s)
       // ByRef functions pass sDependent=false: keep VectorRef for eval, no graph edge.
		if (wLinked) {
			sItem->AddDependent(this);
		}
        // minimum of size Optimization size (compil time)
        m_VectorRef.shrink_to_fit();
        m_VectorRefAddDependent.shrink_to_fit();
	}
    
    tVectorItem* tCell::VectorItem() { return(&m_VectorRef); };
    

	void tCell::ClearVectorRefAndDeleteDependant() {
		// Delete cell dependent of cell in vector 
        // Unique item with unique pointer.
        // for multiple item with same pointer A1,A1,A2 A1 as One Dependent
        tClassVector<tItem*> wContainerItem;
		for (auto wItem : m_VectorRef) {
            // Push unique item with unique pointer
            if (wItem != nullptr) {
                (void)wContainerItem.InsertClass(wItem);
            }
        }
        for (auto wItem : *wContainerItem.Container()) {
            // Delete Dependanr
            wItem->DeleteDependent(this);
            tRange* wRange = wItem->Range();
            // Is Range not empty delete an detach Range
            if (wRange != nullptr) {
                if (wRange->IsEmpty()) {
                    tSheet* wSheet=wRange->Sheet();
    #ifdef debugcell
                    cout << "tCell::ClearVectorRefAndDeleteDependant()->" <<  StrRef() << endl;
    #endif
                    // If delete All wRange->Sheet can be Null
                    if (wSheet!=nullptr) {
    #ifdef debugcell
                        cout << " Range->" << wRange->Debug();
    #endif
                        // Don't test attach
                        wSheet->ColRowCellRange()->DeleteRangeByAllocatorRef(wRange->AllocatorRef(),true);
                    } else {
                        tStringStream wStream;
                        wStream << "throw  tCell::ClearVectorRefAndDeleteDependant(() " <<  StrRef() << "<<  wRange with sheet==nullptr  ";
                        
                        cerr <<  wStream.str();
                        throw(tExceptionInternalError(wStream.str()));
                    }
                }
              }
            }
		m_VectorRef.clear();
		m_VectorRefAddDependent.clear();
	}

	void tCell::PruneStaleInverseDependents() {
		// Conservative cleanup on THIS cell's ContainerCellDepend only (not VectorRef, not other cells).
		// Safe to call after restore/recompile when stale formula cells may still list this source.
		std::vector<tCell*> wRemove;
		for (auto wItemDep : *m_ContainerCellDepend.Container()) {
			if (wItemDep == nullptr) {
				continue;
			}
			tCell* wFormulaCell = wItemDep->Cell();
			if (wFormulaCell == nullptr) {
				continue;
			}
			tBool wStill = false;
			for (auto wItemRef : *wFormulaCell->VectorRef()) {
				if (wItemRef == nullptr) {
					continue;
				}
				if (wItemRef == static_cast<tItem*>(this)) {
					wStill = true;
					break;
				}
				tRange* wRange = wItemRef->Range();
				if (wRange != nullptr && Sheet() == wRange->Sheet() &&
					wRange->CoveredCell(RowIndex(), ColIndex())) {
					wStill = true;
					break;
				}
			}
			if (!wStill) {
				wRemove.push_back(wFormulaCell);
			}
		}
		for (tCell* wF : wRemove) {
			DeleteDependent(wF);
		}
	}

	void tCell::AddDependant() {
		// Add cell dependent on other cell (skip ByRef VectorRef slots).
		if (m_VectorRefAddDependent.size() != m_VectorRef.size()) {
			m_VectorRefAddDependent.assign(m_VectorRef.size(), true);
		}
		for (size_t wI = 0; wI < m_VectorRef.size(); ++wI) {
			tItem* wItem = m_VectorRef[wI];
			if (wItem != nullptr && m_VectorRefAddDependent[wI]) {
				wItem->AddDependent(this);
			}
		}
	}


	tVectorItem* tCell::VectorRef() {
		return(&m_VectorRef);
	}

	const tVectorItem* tCell::VectorRef() const {
		return(&m_VectorRef);
	}

    void tCell::SwapVectorRef(tVectorItem& sOther, std::vector<tBool>& sOtherAddDependent) {
        m_VectorRef.swap(sOther);
        m_VectorRefAddDependent.swap(sOtherAddDependent);
    }

    tRange* tCell::MergedRange() const {
        return(ColRowCellRange()->MergedRange(RowAllocatorRef(), ColAllocatorRef()));
    }

    tSheet* tCell::Sheet() {
        return tItem::Sheet();
    }
    
    tSheet* tCell::Sheet() const {
        return tItem::Sheet();
    }

	// Json ===============================================================
	void tCell::Json(Writer<StringBuffer>* sWriter, tBool sR1C1, tPoint* sDiff) {
#ifdef debugjson
        cout << "Write Json tCell::" << StrRef() << endl;
        if (StrRef()=="G6") {
        }
        cout << FormulaStr(sR1C1).c_str() << endl;
#endif
		tIndex wRow = RowIndex();
		tIndex wCol = ColIndex();
		if (sDiff != nullptr) {
			wRow -= sDiff->Row();
			wCol -= sDiff->Col();
		}
        tWorkBook* wWorkBook=WorkBook();
		tTempoPoint wPoint(wRow, wCol);
        tString wRef=wPoint.StrRef();
        sWriter->Key("c"); sWriter->String(wRef.c_str());
        
        // Write Formula ======================================================
        if (Formula() != nullptr) {
            tJsonSharedString* wJsonSharedFormula=tSpreadSheetContainer::Instance()->JsonSharedFormula();
            if (wJsonSharedFormula->IsActif()) {
                sWriter->Key(wJsonSharedFormula->Key().c_str());
                sWriter->Int(wJsonSharedFormula->AddString(FormulaStr(sR1C1).c_str()));
            } else {
                sWriter->Key("f");
                sWriter->String(FormulaStr(sR1C1).c_str());
            }
            // Persist display value alongside formula (e.g. Excel import .sker); ReadJson restores v/t without EndCalculate.
            if (!m_Value.IsExcelNull()) {
                tBool wMutualize = false;
                if (m_Value.IsString()) {
                    tJsonSharedString* wJsonSharedString = tSpreadSheetContainer::Instance()->JsonSharedString();
                    if (wJsonSharedString->IsActif()) {
                        sWriter->Key(kJsonKeySharedString);
                        sWriter->Int(wJsonSharedString->AddString(m_Value.String()));
                        wMutualize = true;
                    }
                }
                if (!wMutualize) {
                    m_Value.Json(sWriter);
                }
            }
        } else {
            // Value-only cells (including matrix spill extends): persist v/t so ReadJson can restore
            // the grid without EndCalculate on the full dependency graph.
            if (!IsMatExtend() || !m_Value.IsExcelNull()) {
                tBool wMutualize=false;
                // If String Mutualize
                if (m_Value.IsString()) {
                    tJsonSharedString* wJsonSharedString=tSpreadSheetContainer::Instance()->JsonSharedString();
                    if (wJsonSharedString->IsActif()) {
                        sWriter->Key(kJsonKeySharedString);
                        sWriter->Int(wJsonSharedString->AddString(m_Value.String()));
                        wMutualize=true;
                    }
                }
                if (!wMutualize) {
                    // Write value (Variant->Int,Double,String,Class)
                    if (!m_Value.IsExcelNull())  {
                        m_Value.Json(sWriter);
                    }
                }
            }
        }
        // Format Mutualize
        if (m_Css!=0) {
            tFormatApi* wFormatApi=wWorkBook->FormatApi();
            if (wFormatApi!=nullptr) {
                tSize wIndex=wFormatApi->WriteJsonAddFormat(m_Css);
                sWriter->Key("fo"); sWriter->Int(tInt(wIndex));
            }
        }
        // CellClass Attribute ==============================================
        if (m_Value.IsClass()) {
            tVirtualClass* wClass=m_Value.Class();
           
            // CellClass Attribute
            tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wClass);
            if (wCellClassAttribute!=nullptr) {
                sWriter->Key("class");
                tString wClassName = wCellClassAttribute->ClassName();
                if (wClassName.empty() && wCellClassAttribute->ModelClass() != nullptr) {
                    wClassName = wCellClassAttribute->ModelClass()->ClassName();
                }
                sWriter->String(wClassName.c_str());
                sWriter->Key("cl");
                wCellClassAttribute->JsonCell(sWriter,sR1C1,sDiff);
            }
        }
        // CellExtend ========================================================
        if (m_CellExtend!=0) {
            sWriter->Key("extend");
            tCellExtend* wCellExtend=ColRowCellRange()->CellExtend(m_CellExtend);
            wCellExtend->Json(sWriter);
        }
        // Matrix spill metadata: needed so RecalculateAll treats persisted extend cells as own spill.
        if (IsMatExtend() || IsMatOrigin() || IsSpillRange() || IsMatDynamic()) {
            sWriter->Key("ex");
            sWriter->Int(static_cast<int>(m_Extension.BitSet()));
        }
        if (tRange* wSpillRange = SpillRange(); wSpillRange != nullptr) {
            sWriter->Key("spillrange");
            sWriter->String(wSpillRange->StrRef().c_str());
        }
	}

	void tCell::Json(const rapidjson::Value& sValue) {
          // Get WorkBook
        tWorkBook* wWorkBook=WorkBook();
        
      
        // Get Value ========================================================
        tSpreadSheetContainer::Instance()->CurrentJsonCell(this);
#ifdef debugjson
        cout << "Json tCell::" << StrRef();
#endif
        // Is String Index
        if (sValue.HasMember(kJsonKeySharedString)) {
            tIndex wIndex = sValue[kJsonKeySharedString].GetInt();
            tJsonSharedString* wJsonSharedString=tSpreadSheetContainer::Instance()->JsonSharedString();
            m_Value=wJsonSharedString->GetString(wIndex);
        } else {
            if (sValue.HasMember("t")) {
                m_Value.Json(sValue);
            }
        }
#ifdef debugjson
        cout << "=" << m_Value;
#endif
        // Formula ==============================================================
        tString wFormulaStr="";
        tVariant wCachedDisplayValue;
        tBool wHasCachedDisplayValue = false;
        // Is String Index
        if (sValue.HasMember(kJsonKeySharedFormula)) {
             tIndex wIndex = sValue[kJsonKeySharedFormula].GetInt();
               tJsonSharedString* wJsonSharedFormula=tSpreadSheetContainer::Instance()->JsonSharedFormula();
             wFormulaStr=wJsonSharedFormula->GetString(wIndex);
        } else {
            if (sValue.HasMember("f")) {
                wFormulaStr = sValue["f"].GetString();
            }
        }
        // Set Formula
        if (wFormulaStr != "") {
            if (sValue.HasMember(kJsonKeySharedString) || sValue.HasMember("t") || sValue.HasMember("v")) {
                wCachedDisplayValue = m_Value;
                wHasCachedDisplayValue = !wCachedDisplayValue.IsExcelNull();
            }
            Value(wFormulaStr);
#ifdef debugjson
            cout <<  ":" <<  wFormulaStr;
#endif
            // Calculation at the end of process json
            if (wHasCachedDisplayValue) {
                tSpreadSheetContainer::Instance()->PushJsonCell(this, &wCachedDisplayValue);
            } else {
                tSpreadSheetContainer::Instance()->PushJsonCell(this);
            }
        }
        // Get Format
        if (sValue.HasMember("fo")) {
            tFormatApi* wFormatApi = wWorkBook->FormatApi();
            if (wFormatApi != nullptr) {
                const tSize wIndex = static_cast<tSize>(sValue["fo"].GetInt());
                const tString wFormat = wFormatApi->ReadJsonGetFormat(wIndex);
                if (!wFormat.empty()) {
                    const tFormatRef wCss = wFormatApi->ApplyCellFormat(wFormat);
                    if (wCss != 0) {
                        m_Css = wCss;
                    }
                }
            }
        }
       
        if (sValue.HasMember("class")) {
            if (m_Value.IsClass()) {
                tString wClassName=sValue["class"].GetString();
                tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(m_Value.Class());
                if (wCellClassAttribute!=nullptr) {
                    wCellClassAttribute->SheetIndice(Sheet()->IndexAllocatorColRowCellRange());
                    
                    // ClassName before Json(cl): tCellClassAttribute::Json uses Get(ClassName()) for ModelClass.
                    wCellClassAttribute->ClassName(wClassName);
                    const rapidjson::Value& wValue=sValue["cl"];
                    wCellClassAttribute->Json(wValue);
                    // For Paste If Name exist change
                    tColRowCellRange* wColRowCellRange=Sheet()->ColRowCellRange();
                    tCellClassContainer* wCellClassContainer=wColRowCellRange->CellClassContainer();
                    tString wRefName=wCellClassAttribute->RefName();
                    wRefName=wCellClassContainer->GetNextName(wRefName);
  
                    wCellClassAttribute->RefName(wRefName);
                    wCellClassAttribute->CellRootRef(wColRowCellRange,RowIndex(),ColIndex());
                    tModelClass* wModelClass=tClassFactory::Instance()->Get(wClassName);
                    wCellClassAttribute->SetModelClass(wModelClass);
                    wCellClassAttribute->Rooted(this);
                    // Insert Class in container
                    wColRowCellRange->InsertCellClassAttributeContainer(this);
                }
            }
        }
        // CellExtend: pass whole cell JSON — tCellExtend::Json reads member "afo" (not the raw array alone).
        if (sValue.HasMember("extend")) {
            if (m_CellExtend == 0) {
                m_CellExtend = ColRowCellRange()->AllocCellExtend();
            }
            tCellExtend* wCellExtend = ColRowCellRange()->CellExtend(m_CellExtend);
            wCellExtend->Json(sValue);
        }
        if (sValue.HasMember("ex")) {
            const rapidjson::Value& wEx = sValue["ex"];
            tUShort wBits = 0;
            if (wEx.IsUint()) {
                wBits = static_cast<tUShort>(wEx.GetUint());
            } else if (wEx.IsInt()) {
                wBits = static_cast<tUShort>(wEx.GetInt());
            }
            Extension(tExtension(wBits));
        }
        // Persisted spill bounds (e.g. after RecalculateAll): "A10:G15" — same format as tTempoRect::ParseRef / tRange::StrRef.
        if (sValue.HasMember("spillrange")) {
            const tString wRefRange = sValue["spillrange"].GetString();
            tTempoRect wParsed;
            if (wParsed.ParseRef(wRefRange)) {
                SetSpillRect(wParsed.Top(), wParsed.Left(), wParsed.Bottom(), wParsed.Right());
                // A restored spill origin must own a MatOrigin range like an in-session spill, not only a SpillRange.
                // Otherwise a later edit clears the SpillRange (tCell::ClearFormula in tUndoCellValue::Do) and, with no
                // MatOrigin left, the range becomes empty and is deleted BEFORE ClearMatrix (InternalCalculation) can
                // release the cached slaves — those orphaned slaves then block the new spill (#SPILL! after load
                // without a RecalculateAll). Re-tagging keeps ClearMatrix able to find and clear the slave footprint.
                // ("ex" is read just above, so IsMatOrigin() already reflects the persisted flags here.)
                if (IsMatOrigin()) {
                    tRange* wSpill = SpillRange();
                    if (wSpill != nullptr) {
                        wSpill->SetMatOrigin();
                    }
                }
            }
        }
        // Reconcile stale spill flags from legacy / partial saves. With the current engine a real spill origin ALWAYS
        // persists its "spillrange" (AFO) — the block above materializes its range. Older files (e.g. matrix-operator
        // spills =N19:O20+Q19:R20 saved before AFO/slaves persistence) reload the origin as a scalar carrying only the
        // MatOrigin/SpillRange flags, with no materialized range and no slave cells. Those orphaned flags make a later
        // edit trip #SPILL! (ClearMatrix finds no range to clear; the stale flags confuse SpillDestinationIsClear).
        // Dropping them here lets the edit re-spill as a fresh matrix formula. Never touch slaves (MatExtend) — they
        // legitimately have no spillrange and point back to their origin.
        if ((IsSpillRange() || IsMatOrigin()) && !IsMatExtend() && SpillRange() == nullptr) {
            RemoveSpillRange();
            RemoveMatDynamic();
            if (IsMatOrigin() && MatrixRange() == nullptr) {
                RemoveMatOrigin();
            }
        }
#ifdef debugjson
        cout << endl;
#endif
	}
   
	tBool tCell::IsEmpty() const {
		return(
			(m_Value.Type() == tVariantType::t_null) &&
			(m_ContainerCellDepend.Container()->empty()) &&
			(Formula() == nullptr) &&
			(m_VectorRef.empty()) &&
            (!IsSpillRange()) &&
			(m_Css == 0)
			);
	}
        
    tBool tCell::IsValueEmpty() const {
        return(
            (m_Value.Type() == tVariantType::t_null) &&
            (Formula() == nullptr) &&
            (m_Css == 0)
        );
    }
   

	tBool tCell::HasDependent() const {
		return(!(m_ContainerCellDepend.Container()->empty()));
	}

	/*
	tCell& tCell::operator = (const tCell& sCell) {
		m_Type = sCell.m_Type;
		m_Extension = sCell.m_Extension;
		m_RowAllocatorRef = sCell.m_RowAllocatorRef;
		m_ColAllocatorRef = sCell.m_ColAllocatorRef;
		m_Path = sCell.m_Path;
		m_ContainerCellDepend = sCell.m_ContainerCellDepend;
		m_IndiceAllocator = sCell.m_IndiceAllocator;

		m_Value = sCell.m_Value;
		// FormulaOrSpill union copy would go here if operator= were enabled
		m_VectorRef = sCell.m_VectorRef;
		return(*this);
	}
	*/
	tBool  tCell::operator == (const tCell& sCell) const {
		return(
			(Type() == sCell.Type()) &&
			(m_Extension == sCell.m_Extension) &&
			(Col() == sCell.Col()) &&
			(Row() == sCell.Row()) &&
			(m_ContainerCellDepend == sCell.m_ContainerCellDepend) &&
			(m_ColRowCellRangeRef == sCell.m_ColRowCellRangeRef) &&
			(m_Value == sCell.m_Value) &&
			(m_SharedFormula == sCell.m_SharedFormula)
		);
	}

#ifdef checksp
	void tCell::Check() {
		// 1 Chech NbCell =====================================================
		if (Type() != tTypeItem::t_Attribute) {
			Row()->m_NbCellsCheck++;
			Col()->m_NbCellsCheck++;
		}
		// 2 Is Class --> recursive check =====================================
		if (m_Value.Type() == tVariantType::t_class) {
            tCellClass* wCellClass=wCellClass = dynamic_cast<tCellClass*>(m_Value.Class());
            if (wCellClass!=nullptr) {
                wCellClass->Check();
            } else {
                tCellClassAttribute* wCellClassAttribute = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
                if (wCellClassAttribute != nullptr) {
                    wCellClassAttribute->Check();
                }
            }
		}

		// 3 Verify Cell Dependent ============================================
		for (auto wItemDepend : *m_ContainerCellDepend.Container()) {
			if (wItemDepend == nullptr) {
				tStringStream wStream;
				wStream << "Check error on cell " << tItem::StrRef(true) << " item nullptr on m_ContainerCellDepend !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
#ifdef debugdependent
			cout << "  Depend->" << ":"  << tItem::StrRef() << ":" << FormulaStr() << endl;
#endif
			tCell* wCellDepend = wItemDepend->Cell();
			if (wCellDepend != nullptr) {
#ifdef debugdependent
				cout << "         >" << wCellDepend->tItem::StrRef(true) << endl;
#endif
				tBool wOk = false;
				// Search wItem on m_VectorRed
				for (auto wItemRef : wCellDepend->m_VectorRef) {
#ifdef debugdependent
					tCell* wCellRef = nullptr;
                    if (wItemRef!=nullptr)
					  wCellRef = wItemRef->Cell();
					if (wCellRef != nullptr) {
						cout << " Depend " << wCellRef->tItem::StrRef(true) << endl;
					}
#endif

					if (wItemRef == this) {

						wOk = true;
						break;
					}
				}
				if (!wOk) {
					tStringStream wStream;
					wStream << "throw: Check error on cell " << tItem::StrRef(true) << "=" << FormulaStr() << ":";
					wStream << " Cell " << wCellDepend->tItem::StrRef(true) << " not in m_ContainerCellDepend !";
                    cerr << wStream.str() << endl;
					throw(tExceptionInternalError(wStream.str()));
				}
			}
			tRange* wRangeDepend = wItemDepend->Range();
			if (wRangeDepend != nullptr) {
				tStringStream wStream;
				wStream << "throw: Check error on cell " << StrRef(true) << " Range " << wRangeDepend->StrRef(true) << " on m_ContainerCellDepend !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
		}
		// 4 Verify Formula =========================================================
		tBool wHasVolatile = false;
		if (Formula() != nullptr) {
			Formula()->Check();
            if (!Formula()->BitSetVolatile().Empty()) {
				wHasVolatile = true;
                // Note: Some volatile formulas may not be in m_VectorVolatileCell during Check()
                // This is expected behavior and not an error
                if (!ColRowCellRange()->IsInVolatileCells(this)) {
                     tStringStream wStream;
                     wStream << "throw: Check error on cell " << StrRef() << " Volatile " << FormulaStr()  << " not  in m_VectorVolatileCell !";
                     cerr << wStream.str() << endl;
                }
            }
		}
		
		// Skip dependency check for volatile formulas
		if (!wHasVolatile) {
		// Keep flags aligned with VectorRef (legacy / swap edge cases).
		if (m_VectorRefAddDependent.size() != m_VectorRef.size()) {
			m_VectorRefAddDependent.assign(m_VectorRef.size(), true);
		}
		for (size_t wI = 0; wI < m_VectorRef.size(); ++wI) {
			tItem* wItemRef = m_VectorRef[wI];
			// ByRef args: VectorRef kept for eval, no AddDependent → skip inverse check.
			if (!m_VectorRefAddDependent[wI]) {
				continue;
			}
			if (wItemRef != nullptr) {
#ifdef debugdependent
				tCell* wCell = wItemRef->Cell();
				if (wCell != nullptr) {
					cout << wItemRef->tItem::StrRef() << " Formula->" << ":" << wCell->FormulaStr() << endl;
				}
				else {
					cout << wItemRef->tItem::StrRef() << " Formula->" << endl;
				}
#endif
				tCell* wCellRef = wItemRef->Cell();
				if (wCellRef != nullptr) {
					tBool wOk = false;
                    for (auto wItemDepend : *wItemRef->ContainerCellDepend()->Container()) {
                        if (wItemDepend == this) {
                            wOk = true;
                            break;
                        }
                    }
        
                if (!wOk) {
                    tStringStream wStream;
                    wStream << "Check error on cell " << Sheet()->Name() << "!" << tItem::StrRef() << "=" << FormulaStr() << ":";
                    wStream << " Cell " << wCellRef->Sheet()->Name() << "!" <<  wCellRef->tItem::StrRef() << " not in m_ContainerCellDepend !";
                    cout << "Throw: " << wStream.str() << endl;
                    cout << wStream.str() << endl;
                    cout << Debug();
                    cout << wCellRef->Debug();
                    throw(tExceptionInternalError(wStream.str()));
                }
            }
			}
		}
		} // End of !wHasVolatile check
        // 5 Verify Class
        if (ClassAttribute()!=nullptr) {
            tCell* wCellClass=ColRowCellRange()->CellClassContainer()->CellByName(ClassAttribute()->RefName());
            if (wCellClass==nullptr) {
                tStringStream wStream;
                wStream << "Check error on cell " << Sheet()->Name() << "!" << tItem::StrRef() << "=" << FormulaStr() << ":";
                wStream << " Class  " << ClassAttribute()->RefName() << " not in container !";
                cout << "Throw: " << wStream.str() << endl;
                cout << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
        }
    }
#endif

	tString tCell::Debug() const {
        tStringStream wStream;
        tString wType="";
        switch(Type()) {
            case tTypeItem::t_Cell : wType="Cell"; break;
            case tTypeItem::t_Range : wType="Range"; break;
            case tTypeItem::t_Attribute : wType="Attribute"; break;
        }
        wStream << tItem::StrRef() << "=" << FormulaStr() << " Value=" << m_Value << " Css=" << m_Css;
        if (IsMerged()) wStream << " Merged";
        if (IsNamed()) wStream << " Named";
        if (IsConditionalFormat()) wStream << " Conditional Format";
        if (IsMatOrigin()) wStream << " MatOrigin";
        if (IsMatExtend()) wStream << " MatExtend";
        if (IsSpillRange()) wStream << " SpillRange";
        
#ifdef _DEBUGSK
		const tCellClassAttribute* wClass = ClassAttribute();
		if (wClass != nullptr) {
			// Debug() is not const in the base class hierarchy, so we need const_cast
			wStream << const_cast<tCellClassAttribute*>(wClass)->Debug();
		}
#endif
#ifdef debugpointer
		wStream << " Pt:" << this << ":";
#endif // debugpointer
		if ( ContainerCellDepend() != nullptr) {
            if (ContainerCellDepend()->Container()->size()!=0) {
                wStream << endl << " Depend:" << endl;
                for (tItem* wItem : *(ContainerCellDepend()->Container())) {
                        if (wItem!=nullptr) {
                            switch (wItem->Type()) {
                                case tTypeItem::t_Cell : {
                                    wStream << " Cell -> " << wItem->StrRef();
                                    tString wFormula = wItem->Cell()->FormulaStr();
                                    if (wFormula != "") wStream << " Formula=" << wFormula;
                                    break;
                                }
                                case tTypeItem::t_Attribute: {
                                    wStream << " Attribute ->" << wItem->StrRef();
                                    tString wFormula = wItem->Cell()->FormulaStr();
                                    if (wFormula != "") wStream << " Formula=" << wFormula;
                                    break;
                                }
                                case tTypeItem::t_Range: {
                                    wStream << " Range -> " << wItem->StrRef();
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
#ifdef debugpointer
                        wStream << " Pt " << wItem;
#endif // debugpointer
                        wStream  << endl;
                    }
                }
		}
        if (m_VectorRef.size() !=0) {
            if (Formula()!=nullptr) wStream << " =" << Formula()->Debug() << endl;
            
            wStream << "Formula  VectorRef  --------------->" << endl;
            tIndex wInd=0;
  
            for (tItem* wItem : m_VectorRef) {

                wStream << "[" << wInd++ << "] ";
                if (wItem != nullptr) {
                    switch (wItem->Type()) {
                        case tTypeItem::t_Cell: {
                            wStream << " Cell -------->" << wItem->StrRef(true);
                            tString wFormula= wItem->Cell()->FormulaStr();
                            if (wFormula!="") wStream << " Formula=" << wFormula << endl;;
                            break;
                        }
                        case tTypeItem::t_Attribute: {
                            wStream << " Attribute --->" << wItem->StrRef(true);
                            tString wFormula = wItem->Cell()->FormulaStr();
                            if (wFormula != "") wStream << " wFormula=" << wFormula << endl;
                            break;
                        }
                        case tTypeItem::t_Range: {
                            wStream << " Range ------->" << wItem->Range()->StrRef(true) << endl;
                            break;
                        }
                        default:
                            wStream << " Unknown ----->"  << endl;
                            break;
                    }
#ifdef debugpointer
                    wStream << " Pt " << wItem;
#endif // debugpointer
                } else {
                    wStream << " Nullptr" << endl;;
                }
                wStream << endl;
            }
		}
        
#ifdef _DEBUGSK
		if (m_Value.Type() == tVariantType::t_class) {
			tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(m_Value.Class());
            wStream <<  " ";
			if (wCellClass != nullptr) wCellClass->Debug();
		}
#endif
        wStream << endl;
        return(wStream.str());
	}
} // End of namespace
