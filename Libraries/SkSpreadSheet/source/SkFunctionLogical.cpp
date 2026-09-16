//=============================================================================
// SkSpreadSheet Function Logical 
//=============================================================================
#include "../include/SkFunctionLogical.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkTools.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#define _debugfunction

namespace SkSpreadSheet {

	// Helper: convert empty/null to false (Excel semantics for logical functions)
	static tVariant ConvertEmptyToFalse(const tVariant& sValue) {
		if (sValue.IsNull()) return tVariant(false);
		return sValue;
	}

	// Helper: Excel IF treats as FALSE: false, 0, 0.0, null, empty string, "false", "0".
	static bool IsConditionTrue(const tVariant& sValue) {
		if (sValue.IsNull()) return false;
		if (sValue.IsError()) return false;
		if (sValue.IsBool()) return sValue.Bool();
		if (sValue.IsInt()) return (sValue.Int() != 0);
		if (sValue.IsDouble()) {
			tDouble d = sValue.Double();
			return (d != 0.0 && !std::isnan(d));
		}
		if (sValue.IsString()) {
			tString s = sValue.String();
			if (s.empty()) return false;
			// Excel: "false", "0" (and often "FALSE", "0.0") are falsy
			if (s == "0" || s == "0.0") return false;
			if (s == "false" || s == "FALSE") return false;
			return true;
		}
		// Class, date, or other: treat as falsy unless Bool() says true (Excel: only TRUE/1/non-zero are truthy)
		return sValue.Bool();
	}

	// Function =============================================================
	tFunctionIf::tFunctionIf() : tFunction() {}

	tStackElem tFunctionIf::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() < 2 || wArgs.size() > 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg,""))));
		}
		// PopArgs: first popped = stack top. Parser pushes condition, value_if_true, value_if_false -> top = value_if_false (125k).
		// So wArgs[0]=value_if_false, wArgs[1]=value_if_true, wArgs[2]=condition.
		tVariant wCondition;
		tVariant wValue1;  // value_if_true
		tVariant wValue2;  // value_if_false
		
		if (wArgs.size() == 3) {
			if (!StackElemToVariant(wArgs[2], wCondition)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IF: condition must be a value or cell"))));
			}
			if (wCondition.IsError()) return(tStackElem(wCondition));
			wCondition = ConvertEmptyToFalse(wCondition);
			if (!StackElemToVariant(wArgs[1], wValue1)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IF: value_if_true must be a value or cell"))));
			}
			if (!StackElemToVariant(wArgs[0], wValue2)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IF: value_if_false must be a value or cell"))));
			}
			// Do not return early if value_if_true or value_if_false is an error: choose branch from condition first
		} else {
			if (!StackElemToVariant(wArgs[1], wCondition)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IF: condition must be a value or cell"))));
			}
			if (wCondition.IsError()) return(tStackElem(wCondition));
			wCondition = ConvertEmptyToFalse(wCondition);
			if (!StackElemToVariant(wArgs[0], wValue1)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IF: value_if_true must be a value or cell"))));
			}
			wValue2 = tVariant();
		}
#ifdef debugfunction
		const bool wCondTrue = IsConditionTrue(wCondition);
		std::cout << "IF: condition=" << wCondition << " (Type=" << static_cast<int>(wCondition.Type()) << ") value_if_true=" << wValue1 << " value_if_false=" << wValue2 << std::endl;
		std::cout << "IF: IsConditionTrue()=" << (wCondTrue ? 1 : 0) << " -> return " << (wCondTrue ? "value_if_true" : "value_if_false") << std::endl;
#endif
		// Excel: if condition is TRUE return value_if_true, else return value_if_false
		if (IsConditionTrue(wCondition)) {
			return(tStackElem(wValue1));
		}
		return(tStackElem(wValue2));
	};

	// IFERROR / IFNA =======================================================
	tFunctionIfError::tFunctionIfError(tBool sNaOnly) : tFunction(), m_NaOnly(sNaOnly) {}

	tStackElem tFunctionIfError::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		// IFNA: exactly 2 args. IFERROR: 2 or 3 (legacy 3-arg form kept for compatibility).
		if (m_NaOnly) {
			if (wArgs.size() != 2) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFNA requires exactly 2 arguments"))));
			}
		} else if (wArgs.size() < 2 || wArgs.size() > 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFERROR requires 2 or 3 arguments"))));
		}
		// PopArgs: top first → wArgs[0]=value_if_*, wArgs[1]=value (2-arg).
		// 3-arg IFERROR: wArgs[0]=unused third, wArgs[1]=value_if_error, wArgs[2]=value.
		tStackElem* wArgValueIf;
		tStackElem* wArgValue;
		if (wArgs.size() == 2) {
			wArgValueIf = &wArgs[0];
			wArgValue = &wArgs[1];
		} else {
			wArgValueIf = &wArgs[1];
			wArgValue = &wArgs[2];
		}

		tVariant wValue;
		tVariant wValueIf;
		if (!StackElemToVariant(*wArgValue, wValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFERROR/IFNA: first argument must be a value or cell"))));
		}
		if (!StackElemToVariant(*wArgValueIf, wValueIf)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFERROR/IFNA: second argument must be a value or cell"))));
		}
#ifdef debugfunction
		std::cout << (m_NaOnly ? "IFNA" : "IFERROR") << ": value=" << wValue
		          << " value_if=" << wValueIf << " IsError()=" << (wValue.IsError() ? 1 : 0) << std::endl;
#endif
		const tBool wTrap = m_NaOnly
			? (wValue.IsError() && wValue.Error().Code() == tTypeError::t_na)
			: wValue.IsError();
		if (wTrap) {
			return(tStackElem(wValueIf));
		}
		return(tStackElem(wValue));
	};

	// Function Ifs =============================================================
	tFunctionIfs::tFunctionIfs() : tFunction() {}

	tStackElem tFunctionIfs::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() < 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS requires at least 2 arguments"))));
		}
		
		// IFS takes pairs: (condition1, value1, condition2, value2, ...)
		// In RPN: condition1 value1 condition2 value2 ... IFS
		// After PopArgs: wArgs[0]=value3, wArgs[1]=condition3, wArgs[2]=value2, wArgs[3]=condition2, wArgs[4]=value1, wArgs[5]=condition1, ...
		// Process pairs in reverse order: (condition1, value1) then (condition2, value2) then (condition3, value3)
		
		// Check if odd number of arguments (means last is else_value)
		tBool wHasElseValue = (wArgs.size() % 2 == 1);
		tVariant wElseValue;
		
		if (wHasElseValue) {
			if (!StackElemToVariant(wArgs[0], wElseValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS: else value must be a value or cell"))));
			}
			if (wElseValue.IsError()) return(tStackElem(wElseValue));
		}
		
		// Process pairs: (condition, value)
		// Start from the end (which is the beginning in RPN order)
		// Use int to avoid underflow issues with size_t
		int wStartIndex = wHasElseValue ? static_cast<int>(wArgs.size()) - 2 : static_cast<int>(wArgs.size()) - 1;
		
		for (int i = wStartIndex; i >= 1; i -= 2) {
			size_t wIdx = static_cast<size_t>(i);
			if (wIdx >= wArgs.size() || wIdx < 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS: invalid argument count"))));
			}
			
			// After PopArgs, pairs are reversed: wArgs[i]=condition, wArgs[i-1]=value
			// For IFS(condition1, value1, condition2, value2):
			//   After PopArgs: wArgs[0]=value2, wArgs[1]=condition2, wArgs[2]=value1, wArgs[3]=condition1
			//   So for i=3: condition=wArgs[3]=condition1, value=wArgs[2]=value1
			//   For i=1: condition=wArgs[1]=condition2, value=wArgs[0]=value2
			// Get condition (at index i) and value (at index i-1)
			tVariant wCondition;
			tVariant wValue;
			if (!StackElemToVariant(wArgs[wIdx], wCondition)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS: condition must be a value or cell"))));
			}
			if (wCondition.IsError()) return(tStackElem(wCondition));
			wCondition = ConvertEmptyToFalse(wCondition);
			if (!StackElemToVariant(wArgs[wIdx - 1], wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS: value must be a value or cell"))));
			}
			if (wValue.IsError()) return(tStackElem(wValue));
			
			// Check condition
			if (wCondition.Bool()) {
				// Clean up before returning
				return(tStackElem(wValue));
			}
		}
		
		// Clean up
		
		// No condition was true
		if (wHasElseValue) {
			return(tStackElem(wElseValue));
		}
		
		return(tStackElem(tVariant(tClassError(tTypeError::t_na, "IFS: no condition was true"))));
	};

	// Function Switch ==========================================================
	tFunctionSwitch::tFunctionSwitch() : tFunction() {}

	tStackElem tFunctionSwitch::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		const int wN = static_cast<int>(wArgs.size());
		// Excel: SWITCH(expression, value1, result1, [value2, result2, ...], [default]) — at least 3 args.
		if (wN < 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH requires at least 3 arguments"))));
		}

		// Array expression: SWITCH broadcasts element-wise (Excel dynamic-array behaviour), e.g.
		// SWITCH({"a","b"}, "a", 1, "b", 2) -> {1,2}. Results/default may themselves be arrays, in
		// which case the output is the broadcast of the expression and every operand (each dimension
		// must be 1 or the common maximum). This powers league-table style SWITCH over a header row.
		{
			tArrayValue wExprArr;
			const bool wExprIsArray =
				(wArgs[wN - 1].Type() == tStackType::t_Array ||
				 wArgs[wN - 1].Type() == tStackType::t_Range) &&
				tFunction::StackElemToArray(wArgs[wN - 1], wExprArr) && wExprArr.Count() > 1;
			if (wExprIsArray) {
				const int wArrAfterExpr = wN - 1;
				const bool wArrHasDefault = (wArrAfterExpr % 2) == 1;
				const int wArrPairs = wArrAfterExpr / 2;

				std::vector<tArrayValue> wValues(static_cast<size_t>(wArrPairs));
				std::vector<tArrayValue> wResults(static_cast<size_t>(wArrPairs));
				for (int k = 0; k < wArrPairs; ++k) {
					const int wSrcValue = 1 + 2 * k;
					const int wSrcResult = 2 + 2 * k;
					if (!tFunction::StackElemToArray(wArgs[wN - 1 - wSrcValue], wValues[k]) ||
						!tFunction::StackElemToArray(wArgs[wN - 1 - wSrcResult], wResults[k])) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: bad argument"))));
					}
				}
				tArrayValue wDefaultArr;
				if (wArrHasDefault) {
					if (!tFunction::StackElemToArray(wArgs[0], wDefaultArr)) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: bad default"))));
					}
				}

				// Broadcast shape: max rows/cols across every operand.
				tIndex wRows = wExprArr.m_Rows;
				tIndex wCols = wExprArr.m_Cols;
				auto wAccum = [&](const tArrayValue& sA) {
					if (sA.m_Rows > wRows) wRows = sA.m_Rows;
					if (sA.m_Cols > wCols) wCols = sA.m_Cols;
				};
				for (int k = 0; k < wArrPairs; ++k) { wAccum(wValues[k]); wAccum(wResults[k]); }
				if (wArrHasDefault) wAccum(wDefaultArr);

				auto wCompatible = [&](const tArrayValue& sA) -> bool {
					return((sA.m_Rows == 1 || sA.m_Rows == wRows) &&
						   (sA.m_Cols == 1 || sA.m_Cols == wCols));
				};
				bool wShapeOk = wCompatible(wExprArr) && (!wArrHasDefault || wCompatible(wDefaultArr));
				for (int k = 0; wShapeOk && k < wArrPairs; ++k) {
					wShapeOk = wCompatible(wValues[k]) && wCompatible(wResults[k]);
				}
				if (!wShapeOk || wRows <= 0 || wCols <= 0) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SWITCH: incompatible array shapes"))));
				}

				// Broadcasting index: collapse a size-1 dimension to index 0.
				auto wAt = [](const tArrayValue& sA, tIndex sR, tIndex sC) -> tVariant {
					return(sA.At(sA.m_Rows == 1 ? 0 : sR, sA.m_Cols == 1 ? 0 : sC));
				};

				tArrayValue* wOut = new tArrayValue(wRows, wCols);
				for (tIndex r = 0; r < wRows; r++) {
					for (tIndex c = 0; c < wCols; c++) {
						tVariant wE = wAt(wExprArr, r, c);
						if (wE.IsError()) { wOut->At(r, c) = wE; continue; }
						bool wMatched = false;
						for (int k = 0; k < wArrPairs; ++k) {
							tVariant wV = wAt(wValues[k], r, c);
							if (!wV.IsError() && wE == wV) {
								wOut->At(r, c) = wAt(wResults[k], r, c);
								wMatched = true;
								break;
							}
						}
						if (!wMatched) {
							wOut->At(r, c) = wArrHasDefault ? wAt(wDefaultArr, r, c)
														   : tVariant(tClassError(tTypeError::t_na, ""));
						}
					}
				}
				return(tStackElem(wOut));
			}
		}

		// PopArgs reverses source order: wArgs[wN-1] is the expression (first source arg) and wArgs[0] is the
		// last source arg. Source index j maps to wArgs[wN-1-j]: j=0 expression, then (value,result) pairs,
		// and an optional trailing default when the count after the expression is odd.
		tVariant wExpr;
		if (!StackElemToVariant(wArgs[wN - 1], wExpr)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: expression must be a value or cell"))));
		}
		// Excel surfaces an error expression (it can never equal any value).
		if (wExpr.IsError()) return(tStackElem(wExpr));

		const int wAfterExpr = wN - 1;                 // args following the expression
		const bool wHasDefault = (wAfterExpr % 2) == 1; // odd tail => last arg is the default
		const int wPairs = wAfterExpr / 2;

		// First matching value wins, evaluated in source order.
		for (int k = 0; k < wPairs; ++k) {
			const int wSrcValue = 1 + 2 * k;   // source index of value_k
			const int wSrcResult = 2 + 2 * k;  // source index of result_k
			tVariant wValue;
			if (!StackElemToVariant(wArgs[wN - 1 - wSrcValue], wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: value must be a value or cell"))));
			}
			// Match uses the engine equality (same semantics as the "=" operator: case-sensitive for text).
			// An error value never matches (an error expression was already returned above).
			if (!wValue.IsError() && wExpr == wValue) {
				tVariant wResult;
				if (!StackElemToVariant(wArgs[wN - 1 - wSrcResult], wResult)) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: result must be a value or cell"))));
				}
				// Return the matched result verbatim, even if it is itself an error (Excel propagates it).
				return(tStackElem(wResult));
			}
		}
		// No match: return the default if present, else #N/A (Excel).
		if (wHasDefault) {
			tVariant wDefault;
			if (!StackElemToVariant(wArgs[0], wDefault)) { // last source arg
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SWITCH: default must be a value or cell"))));
			}
			return(tStackElem(wDefault));
		}
		return(tStackElem(tVariant(tClassError(tTypeError::t_na, "SWITCH: no value matched and no default"))));
	};

	// Function Choose ==========================================================
	tFunctionChoose::tFunctionChoose() : tFunction() {}

	tStackElem tFunctionChoose::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		const int wN = static_cast<int>(wArgs.size());
		// Excel: CHOOSE(index, value1, value2, ...) — at least an index plus one value.
		if (wN < 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CHOOSE requires an index and at least one value"))));
		}
		// PopArgs reverses source order: wArgs[wN-1] is the index (first source arg) and wArgs[0] is the
		// last value. A value at 1-based position p maps to source index p, i.e. wArgs[wN-1-p].
		const int wNbValues = wN - 1;
		auto wIndexToInt = [](const tVariant& sV, int& oOut) -> bool {
			if (sV.IsError()) {
				return false;
			}
			if (sV.IsInt()) {
				oOut = sV.Int();
				return true;
			}
			if (sV.IsDouble()) {
				oOut = static_cast<int>(sV.Double());
				return true;
			}
			if (sV.IsBool()) {
				oOut = sV.Bool() ? 1 : 0;
				return true;
			}
			return false;
		};

		// Array index: CHOOSE({1,2,3}, col1, col2, col3) stitches the selected args (Excel pre-HSTACK).
		// League-Table Part B Table B2: SORT(CHOOSE({1,2,...,9}, D14#, E14#, ...), {9,8,6,1}, {-1,-1,-1,1}).
		tArrayValue wIndexArr;
		const bool wIndexIsArray =
			(wArgs[wN - 1].Type() == tStackType::t_Array ||
			 wArgs[wN - 1].Type() == tStackType::t_Range) &&
			tFunction::StackElemToArray(wArgs[wN - 1], wIndexArr) &&
			wIndexArr.Count() > 1;
		if (wIndexIsArray) {
			std::vector<tArrayValue> wPicked;
			wPicked.reserve(static_cast<size_t>(wIndexArr.Count()));
			for (tIndex r = 0; r < wIndexArr.m_Rows; ++r) {
				for (tIndex c = 0; c < wIndexArr.m_Cols; ++c) {
					int wIndex = 0;
					if (!wIndexToInt(wIndexArr.At(r, c), wIndex)) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CHOOSE: index must be numeric"))));
					}
					if (wIndex < 1 || wIndex > wNbValues) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CHOOSE: index out of range"))));
					}
					tArrayValue wArr;
					if (!tFunction::StackElemToArray(wArgs[wN - 1 - wIndex], wArr) || wArr.Count() <= 0) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
					}
					wPicked.push_back(std::move(wArr));
				}
			}

			tBool wAllScalar = true;
			for (const tArrayValue& wA : wPicked) {
				if (wA.m_Rows != 1 || wA.m_Cols != 1) {
					wAllScalar = false;
					break;
				}
			}
			if (wAllScalar) {
				tArrayValue* wOut = new tArrayValue(wIndexArr.m_Rows, wIndexArr.m_Cols);
				tIndex wK = 0;
				for (tIndex r = 0; r < wIndexArr.m_Rows; ++r) {
					for (tIndex c = 0; c < wIndexArr.m_Cols; ++c) {
						wOut->At(r, c) = wPicked[static_cast<size_t>(wK)].At(0, 0);
						++wK;
					}
				}
				return(tStackElem(wOut));
			}

			const tBool wHStack = (wIndexArr.m_Rows == 1);
			const tBool wVStack = (wIndexArr.m_Cols == 1);
			if (!wHStack && !wVStack) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CHOOSE: incompatible array index"))));
			}

			tIndex wOutRows = 0;
			tIndex wOutCols = 0;
			if (wHStack) {
				for (const tArrayValue& wA : wPicked) {
					if (wA.m_Rows > wOutRows) {
						wOutRows = wA.m_Rows;
					}
					wOutCols += wA.m_Cols;
				}
			} else {
				for (const tArrayValue& wA : wPicked) {
					if (wA.m_Cols > wOutCols) {
						wOutCols = wA.m_Cols;
					}
					wOutRows += wA.m_Rows;
				}
			}
			if (wOutRows < 1 || wOutCols < 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
			}

			const tVariant wNa(tClassError(tTypeError::t_na, ""));
			tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
			if (wHStack) {
				tIndex wColOffset = 0;
				for (const tArrayValue& wA : wPicked) {
					for (tIndex r = 0; r < wOutRows; ++r) {
						for (tIndex c = 0; c < wA.m_Cols; ++c) {
							wOut->At(r, wColOffset + c) =
								(r < wA.m_Rows) ? wA.At(r, c) : wNa;
						}
					}
					wColOffset += wA.m_Cols;
				}
			} else {
				tIndex wRowOffset = 0;
				for (const tArrayValue& wA : wPicked) {
					for (tIndex r = 0; r < wA.m_Rows; ++r) {
						for (tIndex c = 0; c < wOutCols; ++c) {
							wOut->At(wRowOffset + r, c) =
								(c < wA.m_Cols) ? wA.At(r, c) : wNa;
						}
					}
					wRowOffset += wA.m_Rows;
				}
			}
			return(tStackElem(wOut));
		}

		tVariant wIndexVar;
		if (!StackElemToVariant(wArgs[wN - 1], wIndexVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CHOOSE: index must be a value or cell"))));
		}
		if (wIndexVar.IsError()) return(tStackElem(wIndexVar));
		int wIndex = 0;
		if (!wIndexToInt(wIndexVar, wIndex)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CHOOSE: index must be numeric"))));
		}
		if (wIndex < 1 || wIndex > wNbValues) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CHOOSE: index out of range"))));
		}
		// Return the selected argument verbatim (keeps ranges/cells intact, e.g. SUM(CHOOSE(2, A1:A3, B1:B3))).
		return(wArgs[wN - 1 - wIndex]);
	};

	// Function =============================================================
	tFunctionAnd::tFunctionAnd() : tFunction() {}

	tStackElem tFunctionAnd::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		tVariant wValue(true);
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tVariant wArgValue;
			if (!StackElemToVariant(*it, wArgValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AND: argument must be a value or cell"))));
			}
			if (wArgValue.IsError()) return(tStackElem(wArgValue));
			wArgValue = ConvertEmptyToFalse(wArgValue);
			if (!wArgValue.Bool()) {
				wValue = false;
				break;
			}
		}
		return(tStackElem(wValue));
	};

	// Function =============================================================
	tFunctionOr::tFunctionOr() : tFunction() {}

	tStackElem tFunctionOr::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		tVariant wValue(false);
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tVariant wArgValue;
			if (!StackElemToVariant(*it, wArgValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "OR: argument must be a value or cell"))));
			}
			if (wArgValue.IsError()) return(tStackElem(wArgValue));
			wArgValue = ConvertEmptyToFalse(wArgValue);
			if (wArgValue.Bool()) {
				wValue = true;
				break;
			}
		}
		return(tStackElem(wValue));
	};

	// Function XOR =========================================================
	tFunctionXor::tFunctionXor() : tFunction() {}

	tStackElem tFunctionXor::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XOR requires at least 1 argument"))));
		}
		tInt wTrueCount = 0;
		// Formula order (PopArgs is LIFO).
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tVariant wArgValue;
			if (!StackElemToVariant(*it, wArgValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XOR: argument must be a value or cell"))));
			}
			if (wArgValue.IsError()) return(tStackElem(wArgValue));
			wArgValue = ConvertEmptyToFalse(wArgValue);
			if (wArgValue.Bool()) {
				++wTrueCount;
			}
		}
		return(tStackElem(tVariant((wTrueCount % 2) != 0)));
	};

	// Function =============================================================
	tFunctionNot::tFunctionNot() : tFunction() {}

	tStackElem tFunctionNot::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
		}
		tVariant wArgValue;
		if (!StackElemToVariant(wArgs[0], wArgValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NOT: argument must be a value or cell"))));
		}
		if (wArgValue.IsError()) return(tStackElem(wArgValue));
		wArgValue = ConvertEmptyToFalse(wArgValue);
		return(tStackElem(tVariant(!wArgValue.Bool())));
	};
	

	// Function =============================================================
	tFunctionIsBlank::tFunctionIsBlank() : tFunction() {}
	
	tStackElem tFunctionIsBlank::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ISBLANK requires exactly 1 argument"))));
		}
		
		// ISBLANK(value) - 1 argument; In RPN: wArgs[0]=value
		tStackElem* wArg = &wArgs[0];
		if (wArg->Type() == tStackType::t_Range) {
			// Range: true only if all cells in the range are blank
			tRange* wRange = wArg->Range();
			if (wRange == nullptr) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ISBLANK: invalid range"))));
			}
			tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
			if (wColRowCellRange == nullptr) {
				return(tStackElem(tVariant(true)));
			}
			bool wAllBlank = true;
			for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
				for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
					tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
					if (wCell != nullptr) {
						tVariant wCellValue = wCell->Value();
						if (!wCellValue.IsNull()) {
							wAllBlank = false;
							break;
						}
					}
				}
				if (!wAllBlank) break;
			}
			return(tStackElem(tVariant(wAllBlank)));
		}
		tVariant wValue;
		if (!StackElemToVariant(*wArg, wValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ISBLANK: argument must be a value or cell"))));
		}
		return(tStackElem(tVariant(wValue.IsNull())));
	};

	// Function =============================================================
	tFunctionNa::tFunctionNa() : tFunction() {}

	tStackElem tFunctionNa::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		// Clean up (even though NA doesn't use arguments)
		return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
	};

	// TRUE() / FALSE() =====================================================
	tFunctionTrueFalse::tFunctionTrueFalse(tBool sValue) : tFunction(), m_Value(sValue) {}

	tStackElem tFunctionTrueFalse::Call(tStackElems* sStackElems, tShort sNbArg) {
		PopArgs(sStackElems, sNbArg);
		if (sNbArg != 0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRUE/FALSE take no arguments"))));
		}
		return(tStackElem(tVariant(m_Value)));
	};

	// ISNA / ISERR / ISERROR / ISNUMBER / ISLOGICAL / ISTEXT / ISNONTEXT /
	// ISREF / ISFORMULA ========================================================
	tFunctionIsType::tFunctionIsType(tIsTypeKind sKind) : tFunction(), m_Kind(sKind) {}

	tStackElem tFunctionIsType::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IS* requires exactly 1 argument"))));
		}

		tStackElem* wArg = &wArgs[0];

		// ISREF: inspect the stack element itself (Cell/Range), not the referenced value.
		// Literals, arrays, and error values (#REF! as a value) are FALSE.
		if (m_Kind == tIsTypeKind::IsRef) {
			tBool wIsRef = false;
			if (wArg->Type() == tStackType::t_Cell) {
				wIsRef = (wArg->Cell() != nullptr);
			} else if (wArg->Type() == tStackType::t_Range) {
				wIsRef = (wArg->Range() != nullptr);
			}
			return(tStackElem(tVariant(wIsRef)));
		}

		// ISFORMULA: requires a reference; TRUE when that cell owns a formula.
		if (m_Kind == tIsTypeKind::IsFormula) {
			tCell* wCell = nullptr;
			if (wArg->Type() == tStackType::t_Cell) {
				wCell = wArg->Cell();
			} else if (wArg->Type() == tStackType::t_Range) {
				tRange* wRange = wArg->Range();
				if (wRange != nullptr) {
					// Multi-cell: top-left (Excel 365 can spill; Aggregate keeps one result).
					wCell = wRange->EnsureCell();
				}
			}
			if (wCell == nullptr) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			return(tStackElem(tVariant(wCell->Formula() != nullptr)));
		}

		if (wArg->Type() == tStackType::t_Range) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IS*: argument must be a single value"))));
		}

		tVariant wValue;
		if (!StackElemToVariant(*wArg, wValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IS*: argument must be a value or cell"))));
		}

		tBool wResult = false;
		switch (m_Kind) {
			case tIsTypeKind::IsNa:
				wResult = wValue.IsError() && (wValue.Error().Code() == tTypeError::t_na);
				break;
			case tIsTypeKind::IsErr:
				wResult = wValue.IsError() && (wValue.Error().Code() != tTypeError::t_na);
				break;
			case tIsTypeKind::IsError:
				wResult = wValue.IsError();
				break;
			case tIsTypeKind::IsNumber:
				// Excel: dates are numeric (serials); errors / text / bool / blank are not.
				wResult = wValue.IsNumeric() || wValue.IsDate();
				break;
			case tIsTypeKind::IsLogical:
				wResult = wValue.IsBool();
				break;
			case tIsTypeKind::IsText:
				wResult = wValue.IsString();
				break;
			case tIsTypeKind::IsNonText:
				// Excel: TRUE for blank, numbers, bools, errors — only text is FALSE.
				wResult = !wValue.IsString();
				break;
			case tIsTypeKind::IsRef:
			case tIsTypeKind::IsFormula:
				// Handled above; keep switch exhaustive.
				break;
		}
		return(tStackElem(tVariant(wResult)));
	};

	// ISEVEN / ISODD ========================================================
	tFunctionIsParity::tFunctionIsParity(tBool sEven) : tFunction(), m_Even(sEven) {}

	tStackElem tFunctionIsParity::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ISEVEN/ISODD require exactly 1 argument"))));
		}
		if (wArgs[0].Type() == tStackType::t_Range) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		tVariant wValue;
		if (!StackElemToVariant(wArgs[0], wValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wValue.IsError()) {
			return(tStackElem(wValue));
		}
		tDouble wX = 0.0;
		if (wValue.IsNumeric()) {
			wX = wValue.Numeric();
		} else if (wValue.IsDate()) {
			tClassDate wDate(wValue.Date());
			wX = static_cast<tDouble>(wDate.Value()) / 86400.0 + 25569.0;
		} else {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		// Excel: truncate toward zero, then test parity of the integer.
		const tLong wN = static_cast<tLong>(std::trunc(wX));
		const tBool wIsEven = ((wN % 2) == 0);
		return(tStackElem(tVariant(m_Even ? wIsEven : !wIsEven)));
	};

	tFunctionSpillKind tFunctionIf::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionIfError::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionIfs::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionSwitch::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionChoose::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionAnd::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionOr::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionXor::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionNot::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	tFunctionIsOmitted::tFunctionIsOmitted() : tFunction() {}

	tStackElem tFunctionIsOmitted::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ISOMITTED requires exactly 1 argument"))));
		}
		tStackElem* wArg = &wArgs[0];
		if (wArg->Type() == tStackType::t_Range) {
			return(tStackElem(tVariant(false)));
		}
		tVariant wValue;
		if (!StackElemToVariant(*wArg, wValue)) {
			return(tStackElem(tVariant(false)));
		}
		// Only the omitted sentinel (bound to a missing trailing LAMBDA parameter) is TRUE.
		if (wValue.IsError() && wValue.Error().Code() == tTypeError::t_omitted) {
			return(tStackElem(tVariant(true)));
		}
		return(tStackElem(tVariant(false)));
	};

	tFunctionSpillKind tFunctionIsBlank::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionNa::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionTrueFalse::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionIsType::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionIsParity::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionIsOmitted::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// N ====================================================================
	tFunctionN::tFunctionN() : tFunction() {}

	tStackElem tFunctionN::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "N requires exactly 1 argument"))));
		}
		tVariant wValue;
		if (wArgs[0].Type() == tStackType::t_Array) {
			// Implicit intersection: top-left of an array.
			wValue = wArgs[0].Value();
		} else if (!StackElemToVariant(wArgs[0], wValue)) {
			return(tStackElem(tVariant(0.0)));
		}
		if (wValue.IsError()) {
			return(tStackElem(wValue));
		}
		if (wValue.IsNumeric()) {
			return(tStackElem(tVariant(wValue.Numeric())));
		}
		if (wValue.IsDate()) {
			tClassDate wDate(wValue.Date());
			const tDouble wSerial =
				static_cast<tDouble>(wDate.Value()) / 86400.0 + 25569.0;
			return(tStackElem(tVariant(wSerial)));
		}
		if (wValue.IsBool()) {
			return(tStackElem(tVariant(wValue.Bool() ? 1.0 : 0.0)));
		}
		// Text / blank / other → 0
		return(tStackElem(tVariant(0.0)));
	};

	tFunctionSpillKind tFunctionN::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// TYPE =================================================================
	tFunctionType::tFunctionType() : tFunction() {}

	tStackElem tFunctionType::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TYPE requires exactly 1 argument"))));
		}
		// Excel TYPE: 64 = array (in-memory t_Array, or a multi-cell range).
		if (wArgs[0].Type() == tStackType::t_Array) {
			return(tStackElem(tVariant(64)));
		}
		if (wArgs[0].Type() == tStackType::t_Range) {
			tRange* wRange = wArgs[0].Range();
			if (wRange != nullptr) {
				const tIndex wRows = wRange->BottomIndex() - wRange->TopIndex() + 1;
				const tIndex wCols = wRange->RightIndex() - wRange->LeftIndex() + 1;
				if (wRows * wCols > 1) {
					return(tStackElem(tVariant(64)));
				}
			}
		}
		tVariant wValue;
		if (!StackElemToVariant(wArgs[0], wValue)) {
			return(tStackElem(tVariant(1))); // blank / unresolved → number-ish 1 in Excel for empty
		}
		if (wValue.IsError()) {
			return(tStackElem(tVariant(16)));
		}
		if (wValue.IsBool()) {
			return(tStackElem(tVariant(4)));
		}
		if (wValue.IsString()) {
			return(tStackElem(tVariant(2)));
		}
		// number, date, blank → 1
		return(tStackElem(tVariant(1)));
	};

	tFunctionSpillKind tFunctionType::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// ERROR.TYPE / ERROR_TYPE ==============================================
	tFunctionErrorType::tFunctionErrorType() : tFunction() {}

	tStackElem tFunctionErrorType::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ERROR_TYPE requires 1 argument"))));
		}
		tVariant wValue;
		if (!StackElemToVariant(wArgs[0], wValue)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		if (!wValue.IsError()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		// Excel ERROR.TYPE codes (extended for #SPILL! / #CALC!).
		tInt wCode = 0;
		switch (wValue.Error().Code()) {
			case tTypeError::t_div0:   wCode = 2; break;
			case tTypeError::t_value:  wCode = 3; break;
			case tTypeError::t_arg:    wCode = 3; break;
			case tTypeError::t_class:  wCode = 3; break;
			case tTypeError::t_omitted:wCode = 3; break;
			case tTypeError::t_ref:    wCode = 4; break;
			case tTypeError::t_name:   wCode = 5; break;
			case tTypeError::t_num:    wCode = 6; break;
			case tTypeError::t_recursive: wCode = 6; break;
			case tTypeError::t_matrix: wCode = 6; break;
			case tTypeError::t_na:     wCode = 7; break;
			case tTypeError::t_spill:  wCode = 9; break;
			case tTypeError::t_calc:   wCode = 14; break;
			default:
				return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		return(tStackElem(tVariant(wCode)));
	}

	tFunctionSpillKind tFunctionErrorType::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// Introspection helpers (FORMULATEXT / CELL / INFO / SHEET / SHEETS) =====
	namespace {

		static tString UpperAscii(tString sText) {
			for (char& ch : sText) {
				ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			}
			return sText;
		}

		static tCell* CellFromStackRef(const tStackElem& sArg) {
			if (sArg.Type() == tStackType::t_Cell) {
				return sArg.Cell();
			}
			if (sArg.Type() == tStackType::t_Range) {
				tRange* wRange = sArg.Range();
				if (wRange != nullptr) {
					return wRange->EnsureCell();
				}
			}
			return nullptr;
		}

		static tSheet* SheetFromStackRef(const tStackElem& sArg) {
			if (sArg.Type() == tStackType::t_Cell) {
				tCell* wCell = sArg.Cell();
				return (wCell != nullptr) ? wCell->Sheet() : nullptr;
			}
			if (sArg.Type() == tStackType::t_Range) {
				tRange* wRange = sArg.Range();
				return (wRange != nullptr) ? wRange->Sheet() : nullptr;
			}
			return nullptr;
		}

		static tSheet* SheetFromCaller(tItem* sItemRef) {
			if (sItemRef == nullptr) return nullptr;
			tCell* wCell = sItemRef->Cell();
			if (wCell != nullptr) return wCell->Sheet();
			tRange* wRange = sItemRef->Range();
			if (wRange != nullptr) return wRange->Sheet();
			return sItemRef->Sheet();
		}

		static tInt SheetIndex1Based(tSheet* sSheet) {
			if (sSheet == nullptr) return 0;
			tWorkBook* wBook = sSheet->WorkBook();
			if (wBook == nullptr) return 0;
			const tVectorSheet wSheets = wBook->VectorPtSheet();
			for (size_t i = 0; i < wSheets.size(); ++i) {
				if (wSheets[i] == sSheet) {
					return static_cast<tInt>(i) + 1;
				}
			}
			for (size_t i = 0; i < wSheets.size(); ++i) {
				if (wSheets[i] != nullptr && wSheets[i]->Name() == sSheet->Name()) {
					return static_cast<tInt>(i) + 1;
				}
			}
			return 0;
		}

		static tInt SheetCount(tWorkBook* sBook) {
			if (sBook == nullptr) return 0;
			tVectorAllocatorRef* wVec = sBook->VectorSheet();
			return (wVec != nullptr) ? static_cast<tInt>(wVec->size()) : 0;
		}

		static tString AbsoluteA1(tCell* sCell) {
			tString wOut = "$";
			wOut += Base10ToAlpha(sCell->ColIndex());
			wOut += "$";
			wOut += std::to_string(static_cast<long long>(sCell->RowIndex()));
			return wOut;
		}

		static tString DirectoryFromUri(const tString& sUri) {
			if (sUri.empty()) return tString();
			const size_t wSlash = sUri.find_last_of("/\\");
			if (wSlash == tString::npos) return tString();
			return sUri.substr(0, wSlash + 1);
		}

	} // namespace

	// FORMULATEXT ==========================================================
	tFunctionFormulaText::tFunctionFormulaText() : tFunction() {}

	tStackElem tFunctionFormulaText::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FORMULATEXT requires 1 argument"))));
		}
		tCell* wCell = CellFromStackRef(wArgs[0]);
		if (wCell == nullptr) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wCell->Formula() == nullptr) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		tString wText = wCell->FormulaStr();
		if (wText.empty() || wText[0] != '=') {
			wText = tString("=") + wText;
		}
		return(tStackElem(tVariant(wText)));
	}

	tFunctionSpillKind tFunctionFormulaText::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// CELL =================================================================
	tFunctionCellInfo::tFunctionCellInfo() : tFunction(), m_ItemRef(nullptr) {}

	tStackElem tFunctionCellInfo::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() < 1 || wArgs.size() > 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CELL requires 1 or 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());

		tVariant wInfoVar;
		if (!StackElemToVariant(wArgs[0], wInfoVar) || !wInfoVar.IsString()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tString wInfo = UpperAscii(wInfoVar.String());

		tCell* wCell = nullptr;
		if (wArgs.size() == 2) {
			wCell = CellFromStackRef(wArgs[1]);
			if (wCell == nullptr) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		} else {
			if (m_ItemRef == nullptr) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wCell = m_ItemRef->Cell();
			if (wCell == nullptr) {
				tRange* wRange = m_ItemRef->Range();
				if (wRange != nullptr) wCell = wRange->EnsureCell();
			}
			if (wCell == nullptr) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}

		if (wInfo == "ADDRESS") {
			return(tStackElem(tVariant(AbsoluteA1(wCell))));
		}
		if (wInfo == "COL") {
			return(tStackElem(tVariant(static_cast<tInt>(wCell->ColIndex()))));
		}
		if (wInfo == "ROW") {
			return(tStackElem(tVariant(static_cast<tInt>(wCell->RowIndex()))));
		}
		if (wInfo == "CONTENTS") {
			return(tStackElem(wCell->CalculableValue()));
		}
		if (wInfo == "TYPE") {
			const tVariant& wVal = wCell->CalculableValue();
			if (wVal.IsNull() || (wVal.IsString() && wVal.String().empty())) {
				return(tStackElem(tVariant(tString("b"))));
			}
			if (wVal.IsString()) {
				return(tStackElem(tVariant(tString("l"))));
			}
			return(tStackElem(tVariant(tString("v"))));
		}
		if (wInfo == "FILENAME") {
			tWorkBook* wBook = wCell->WorkBook();
			return(tStackElem(tVariant(wBook != nullptr ? wBook->Uri() : tString())));
		}
		// Unsupported Excel info_types for now (format/protect/width/…).
		if (wInfo == "FORMAT" || wInfo == "COLOR" || wInfo == "PARENTHESES" ||
			wInfo == "PREFIX" || wInfo == "PROTECT" || wInfo == "WIDTH") {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
	}

	tBool tFunctionCellInfo::ByRef() { return true; }

	void tFunctionCellInfo::PassByRef(tItem* sItem) { m_ItemRef = sItem; }

	tFunctionSpillKind tFunctionCellInfo::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// INFO =================================================================
	tFunctionInfo::tFunctionInfo() : tFunction() {}

	tStackElem tFunctionInfo::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "INFO requires 1 argument"))));
		}
		tVariant wTypeVar;
		if (!StackElemToVariant(wArgs[0], wTypeVar) || !wTypeVar.IsString()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tString wType = UpperAscii(wTypeVar.String());
		tWorkBook* wBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();

		if (wType == "DIRECTORY") {
			const tString wUri = (wBook != nullptr) ? wBook->Uri() : tString();
			const tString wDir = DirectoryFromUri(wUri);
			if (wDir.empty()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
			}
			return(tStackElem(tVariant(wDir)));
		}
		if (wType == "NUMFILE") {
			tInt wCount = 0;
			tVectorWorkBookClass wBooks;
			tSpreadSheetContainer::Instance()->WorkBooksList(wBooks);
			for (tWorkBook* wB : wBooks) {
				wCount += SheetCount(wB);
			}
			return(tStackElem(tVariant(wCount)));
		}
		if (wType == "ORIGIN") {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		if (wType == "OSVERSION") {
#if defined(__APPLE__)
			return(tStackElem(tVariant(tString("macOS"))));
#elif defined(_WIN32)
			return(tStackElem(tVariant(tString("Windows"))));
#else
			return(tStackElem(tVariant(tString("Linux"))));
#endif
		}
		if (wType == "RECALC") {
			return(tStackElem(tVariant(tString("Automatic"))));
		}
		if (wType == "RELEASE") {
			return(tStackElem(tVariant(tString("sker"))));
		}
		if (wType == "SYSTEM") {
#if defined(__APPLE__)
			return(tStackElem(tVariant(tString("mac"))));
#else
			return(tStackElem(tVariant(tString("pcdos"))));
#endif
		}
		return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
	}

	tFunctionSpillKind tFunctionInfo::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// SHEET ================================================================
	tFunctionSheetNum::tFunctionSheetNum() : tFunction(), m_ItemRef(nullptr) {}

	tStackElem tFunctionSheetNum::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() > 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SHEET requires 0 or 1 argument"))));
		}

		tSheet* wSheet = nullptr;
		if (wArgs.empty()) {
			wSheet = SheetFromCaller(m_ItemRef);
		} else if (wArgs[0].Type() == tStackType::t_Cell || wArgs[0].Type() == tStackType::t_Range) {
			wSheet = SheetFromStackRef(wArgs[0]);
		} else {
			tVariant wVal;
			if (!StackElemToVariant(wArgs[0], wVal)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
			}
			if (wVal.IsError()) return(tStackElem(wVal));
			if (wVal.IsString()) {
				tWorkBook* wBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
				if (wBook == nullptr) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
				}
				wSheet = wBook->Sheet(wVal.String());
			} else {
				// Non-ref / non-name → #N/A (Excel).
				return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
			}
		}

		const tInt wIndex = SheetIndex1Based(wSheet);
		if (wIndex < 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		return(tStackElem(tVariant(wIndex)));
	}

	void tFunctionSheetNum::PassByRef(tItem* sItem) { m_ItemRef = sItem; }
	tBool tFunctionSheetNum::ByRef() { return true; }
	tFunctionSpillKind tFunctionSheetNum::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// SHEETS ===============================================================
	tFunctionSheets::tFunctionSheets() : tFunction(), m_ItemRef(nullptr) {}

	tStackElem tFunctionSheets::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() > 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SHEETS requires 0 or 1 argument"))));
		}

		tWorkBook* wBook = nullptr;
		if (wArgs.empty()) {
			tSheet* wSheet = SheetFromCaller(m_ItemRef);
			wBook = (wSheet != nullptr) ? wSheet->WorkBook()
										: tSpreadSheetContainer::Instance()->ActiveWorkBook();
		} else if (wArgs[0].Type() == tStackType::t_Cell || wArgs[0].Type() == tStackType::t_Range) {
			tSheet* wSheet = SheetFromStackRef(wArgs[0]);
			wBook = (wSheet != nullptr) ? wSheet->WorkBook() : nullptr;
		} else {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wBook == nullptr) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		return(tStackElem(tVariant(SheetCount(wBook))));
	}

	void tFunctionSheets::PassByRef(tItem* sItem) { m_ItemRef = sItem; }
	tBool tFunctionSheets::ByRef() { return true; }
	tFunctionSpillKind tFunctionSheets::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// LET registration stub: compiled to scope opcodes, so this is never invoked in practice. Returning
	// #VALUE! makes any unexpected direct call visible rather than silently wrong.
	tFunctionLet::tFunctionLet() : tFunction() {}

	tStackElem tFunctionLet::Call(tStackElems* sStackElems, tShort sNbArg) {
		PopArgs(sStackElems, sNbArg);
		return(tStackElem(tVariant(tClassError(tTypeError::t_value, "LET is handled at compile time"))));
	};

	tFunctionSpillKind tFunctionLet::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

} // end of namespace
