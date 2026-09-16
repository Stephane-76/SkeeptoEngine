//=============================================================================
// SkSpreadSheet Function Math 
//=============================================================================
#ifdef WIN32
    #define _USE_MATH_DEFINES
#endif
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>
#include <vector>
#include "../include/SkFunctionMath.hpp"
#include "../include/SkColRowCellRange.hpp"

namespace SkSpreadSheet {

	namespace {
	// Excel SUM semantics: add numeric values; ignore text; propagate errors.
	static tBool SumAccumulate(tVariant& ioSum, const tVariant& sAddend) {
		if (sAddend.IsError()) {
			ioSum = sAddend;
			return false;
		}
		if (sAddend.IsString()) {
			return true;
		}
		if (sAddend.IsNull()) {
			return true;
		}
		// Accumulate plain numbers in place: operator+ builds a temporary tVariant and
		// re-dispatches on both operand types, which shows up on million-cell ranges.
		// Promotion stays identical: int keeps int until it would overflow, any double
		// promotes the sum to double. Everything else falls through to operator+.
		const tVariantType wAddendType = sAddend.Type();
		switch (ioSum.Type()) {
		case tVariantType::t_int:
			if (wAddendType == tVariantType::t_int) {
				const tInt w1 = ioSum.Int();
				const tInt w2 = sAddend.Int();
				if (((w2 > 0) && (w1 > std::numeric_limits<tInt>::max() - w2))
					|| ((w2 < 0) && (w1 < std::numeric_limits<tInt>::min() - w2))) {
					ioSum.SetDouble(static_cast<tDouble>(w1) + static_cast<tDouble>(w2));
				} else {
					ioSum.SetInt(w1 + w2);
				}
				return true;
			}
			if (wAddendType == tVariantType::t_double) {
				ioSum.SetDouble(static_cast<tDouble>(ioSum.Int()) + sAddend.Double());
				return true;
			}
			break;
		case tVariantType::t_double:
			if (wAddendType == tVariantType::t_double) {
				ioSum.SetDouble(ioSum.Double() + sAddend.Double());
				return true;
			}
			if (wAddendType == tVariantType::t_int) {
				ioSum.SetDouble(ioSum.Double() + static_cast<tDouble>(sAddend.Int()));
				return true;
			}
			break;
		default:
			break;
		}
		ioSum = ioSum + sAddend;
		return !ioSum.IsError();
	}

	// Excel ROUNDDOWN/TRUNC: toward zero at num_digits precision.
	static tVariant RoundTowardZero(tDouble sValue, tInt sDecimals) {
		tVariant wValue;
		if (sDecimals >= 0) {
			const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(sDecimals));
			const tDouble wScaled = sValue * wFactor;
			wValue = std::trunc(wScaled) / wFactor;
		} else {
			const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(-sDecimals));
			const tDouble wScaled = sValue / wFactor;
			wValue = std::trunc(wScaled) * wFactor;
		}
		return wValue;
	}

	// Excel ROUNDUP: away from zero at num_digits precision.
	static tVariant RoundAwayFromZero(tDouble sValue, tInt sDecimals) {
		auto wAwayFromZero = [](tDouble sScaled) -> tDouble {
			const tDouble wTruncated = std::trunc(sScaled);
			if (sScaled == wTruncated) {
				return wTruncated;
			}
			return (sScaled > 0.0) ? (wTruncated + 1.0) : (wTruncated - 1.0);
		};
		tVariant wValue;
		if (sDecimals >= 0) {
			const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(sDecimals));
			const tDouble wScaled = sValue * wFactor;
			wValue = wAwayFromZero(wScaled) / wFactor;
		} else {
			const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(-sDecimals));
			const tDouble wScaled = sValue / wFactor;
			wValue = wAwayFromZero(wScaled) * wFactor;
		}
		return wValue;
	}
	} // namespace

	// CallBack for function average ========================================
	tCallBackRangeAverage::tCallBackRangeAverage(tColRowCellRange* sColRowCellRange)
		: tCallBackRangeFunction(sColRowCellRange), m_Count(0) {
		m_Value = tVariant(0.0);
	}

	tBool tCallBackRangeAverage::CallBack(tAllocatorRef sAllocatorRef) {
		tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell == nullptr) {
			return true;
		}
		const tVariant& wVal = wCell->CalculableValue();
		if (wVal.IsError()) {
			m_Value = wVal;
			return false;
		}
		if (wVal.IsDouble() || wVal.IsInt()) {
			SumAccumulate(m_Value, wVal);
			m_Count++;
		}
		return true;
	}

	// CallBack for function sum ============================================
	tCallBackRangeSum::tCallBackRangeSum(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange) {}


	tBool tCallBackRangeSum::CallBack(tAllocatorRef sAllocatorRef) {
		tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell == nullptr) {
			return(true);
		}
		return SumAccumulate(m_Value, wCell->CalculableValue());
	};

	// Function =============================================================
	tFunctionSum::tFunctionSum() : tFunction() {}

	tStackElem tFunctionSum::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		tVariant wValue;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			if (wArg.Type() == tStackType::t_Range) {
				tRange* wRange = wArg.Range();
				if (wRange != nullptr) {
					tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
					tCallBackRangeSum wCallBackSum(wColRowCellRange);
					wColRowCellRange->VisitorRange(wRange, &wCallBackSum);
					if (!SumAccumulate(wValue, wCallBackSum.Value())) {
						return(tStackElem(wValue));
					}
				}
			} else if (wArg.Type() == tStackType::t_Array) {
				// Needed by BYROW/BYCOL: LAMBDA receives the row/column as an in-memory array.
				tArrayValue* wArray = wArg.Array();
				if (wArray == nullptr) continue;
				for (tIndex r = 0; r < wArray->m_Rows; ++r) {
					for (tIndex c = 0; c < wArray->m_Cols; ++c) {
						if (!SumAccumulate(wValue, wArray->At(r, c))) {
							return(tStackElem(wValue));
						}
					}
				}
			} else {
				tVariant wArgVal;
				if (!StackElemToVariant(wArg, wArgVal)) continue;
				if (!SumAccumulate(wValue, wArgVal)) {
					return(tStackElem(wValue));
				}
			}
		}
		return(tStackElem(wValue));
	};

    // Function =============================================================
    tFunctionAverage::tFunctionAverage() : tFunction() {}

    tStackElem tFunctionAverage::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        tInt wNb=0;
        
        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tStackElem& wArg = *it;
            if (wArg.Type() == tStackType::t_Range) {
                tRange* wRange = wArg.Range();
                if (wRange == nullptr) {
                    continue;
                }
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                tCallBackRangeAverage wCallBackAverage(wColRowCellRange);
                wColRowCellRange->VisitorRange(wRange, &wCallBackAverage);
                if (wCallBackAverage.Value().IsError()) {
                    return(tStackElem(wCallBackAverage.Value()));
                }
                wValue = wValue + wCallBackAverage.Value();
                wNb += wCallBackAverage.Count();
            } else if (wArg.Type() == tStackType::t_Array) {
                tArrayValue* wArray = wArg.Array();
                if (wArray == nullptr) continue;
                for (tIndex r = 0; r < wArray->m_Rows; ++r) {
                    for (tIndex c = 0; c < wArray->m_Cols; ++c) {
                        const tVariant& wArgVal = wArray->At(r, c);
                        if (wArgVal.IsError()) return(tStackElem(wArgVal));
                        if (wArgVal.IsDouble() || wArgVal.IsInt()) {
                            wValue = wValue + wArgVal;
                            wNb++;
                        }
                    }
                }
            } else {
                tVariant wArgVal;
                if (!StackElemToVariant(wArg, wArgVal)) continue;
                if (wArgVal.IsError()) return(tStackElem(tVariant(wArgVal)));
                if (wArgVal.IsDouble() || wArgVal.IsInt()) {
                    wValue = wValue + wArgVal;
                    wNb++;
                }
            }
        }
        if (wNb == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
        }
        return(tStackElem(wValue / wNb));
    };

	namespace {
		// Excel serial for date values (same mapping as tClassDate operator+).
		static tBool MedianPushNumber(std::vector<tDouble>& sOut, const tVariant& sValue) {
			if (sValue.IsError()) {
				return false;
			}
			if (sValue.IsNumeric()) {
				sOut.push_back(sValue.Numeric());
				return true;
			}
			if (sValue.IsDate()) {
				tClassDate wDate(sValue.Date());
				const tDouble wSerial =
					static_cast<tDouble>(wDate.Value()) / 86400.0 + 25569.0;
				sOut.push_back(wSerial);
				return true;
			}
			return true; // ignore text / bool / blank (Excel MEDIAN range rules)
		}

		static tStackElem MakeWholeOrDouble(tDouble sValue) {
			if (std::isfinite(sValue) && sValue == std::floor(sValue) &&
				std::fabs(sValue) < 9.0e15) {
				return(tStackElem(tVariant(static_cast<tInt>(sValue))));
			}
			return(tStackElem(tVariant(sValue)));
		}

		// Derived so helpers may call protected tFunction::StackElemToVariant.
		// Shared by MEDIAN / STDEV* / VAR* / LARGE / SMALL / MODE / RANK.
		struct tNumericArgCollector : tFunction {
			static tBool Append(const tStackElem& sArg,
								std::vector<tDouble>& oOut,
								tVariant& oError) {
				if (sArg.Type() == tStackType::t_Range) {
					tRange* wRange = sArg.Range();
					if (wRange == nullptr) {
						return true;
					}
					tColRowCellRange* wCr = wRange->ColRowCellRange();
					if (wCr == nullptr) {
						return true;
					}
					for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); ++wRow) {
						for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); ++wCol) {
							tCell* wCell = wCr->Cell(wRow, wCol);
							tVariant wVal = (wCell != nullptr) ? wCell->CalculableValue() : tVariant();
							if (wVal.IsError()) {
								oError = wVal;
								return false;
							}
							if (!MedianPushNumber(oOut, wVal)) {
								oError = wVal;
								return false;
							}
						}
					}
					return true;
				}
				if (sArg.Type() == tStackType::t_Array) {
					tArrayValue* wArray = sArg.Array();
					if (wArray == nullptr) {
						return true;
					}
					for (tIndex r = 0; r < wArray->m_Rows; ++r) {
						for (tIndex c = 0; c < wArray->m_Cols; ++c) {
							const tVariant& wVal = wArray->At(r, c);
							if (wVal.IsError()) {
								oError = wVal;
								return false;
							}
							if (!MedianPushNumber(oOut, wVal)) {
								oError = wVal;
								return false;
							}
						}
					}
					return true;
				}
				tVariant wArgVal;
				if (!StackElemToVariant(sArg, wArgVal)) {
					return true;
				}
				if (wArgVal.IsError()) {
					oError = wArgVal;
					return false;
				}
				if (!MedianPushNumber(oOut, wArgVal)) {
					oError = wArgVal;
					return false;
				}
				return true;
			}

			// PopArgs order is last-arg-first; reverse-iterate for formula order.
			static tBool Collect(const std::vector<tStackElem>& sArgs,
								 std::vector<tDouble>& oOut,
								 tVariant& oError) {
				for (auto it = sArgs.rbegin(); it != sArgs.rend(); ++it) {
					if (!Append(*it, oOut, oError)) {
						return false;
					}
				}
				return true;
			}
		};

		// Excel *A coercion: number as-is; TRUE→1; FALSE→0; blank skip;
		// text in range/array → 0; text as direct arg → #VALUE!.
		static tBool AverageAPush(std::vector<tDouble>& oOut,
								  const tVariant& sValue,
								  tBool sFromRangeOrArray,
								  tVariant& oError) {
			if (sValue.IsError()) {
				oError = sValue;
				return(false);
			}
			if (sValue.IsNull()) return(true);
			if (sValue.IsNumeric()) {
				oOut.push_back(sValue.Numeric());
				return(true);
			}
			if (sValue.IsDate()) {
				tClassDate wDate(sValue.Date());
				const tDouble wSerial =
					static_cast<tDouble>(wDate.Value()) / 86400.0 + 25569.0;
				oOut.push_back(wSerial);
				return(true);
			}
			if (sValue.IsBool()) {
				oOut.push_back(sValue.Bool() ? 1.0 : 0.0);
				return(true);
			}
			if (sValue.IsString()) {
				if (sFromRangeOrArray) {
					oOut.push_back(0.0);
					return(true);
				}
				oError = tVariant(tClassError(tTypeError::t_value, ""));
				return(false);
			}
			return(true);
		}

		struct tAverageAArgCollector : tFunction {
			static tBool Append(const tStackElem& sArg,
								std::vector<tDouble>& oOut,
								tVariant& oError) {
				if (sArg.Type() == tStackType::t_Range) {
					tRange* wRange = sArg.Range();
					if (wRange == nullptr) return(true);
					tColRowCellRange* wCr = wRange->ColRowCellRange();
					if (wCr == nullptr) return(true);
					for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); ++wRow) {
						for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); ++wCol) {
							tCell* wCell = wCr->Cell(wRow, wCol);
							tVariant wVal = (wCell != nullptr) ? wCell->CalculableValue() : tVariant();
							if (!AverageAPush(oOut, wVal, true, oError)) return(false);
						}
					}
					return(true);
				}
				if (sArg.Type() == tStackType::t_Array) {
					tArrayValue* wArray = sArg.Array();
					if (wArray == nullptr) return(true);
					for (tIndex r = 0; r < wArray->m_Rows; ++r) {
						for (tIndex c = 0; c < wArray->m_Cols; ++c) {
							if (!AverageAPush(oOut, wArray->At(r, c), true, oError)) return(false);
						}
					}
					return(true);
				}
				tVariant wArgVal;
				if (!StackElemToVariant(sArg, wArgVal)) return(true);
				return(AverageAPush(oOut, wArgVal, false, oError));
			}

			static tBool Collect(const std::vector<tStackElem>& sArgs,
								 std::vector<tDouble>& oOut,
								 tVariant& oError) {
				for (auto it = sArgs.rbegin(); it != sArgs.rend(); ++it) {
					if (!Append(*it, oOut, oError)) return(false);
				}
				return(true);
			}
		};
	} // namespace

	tFunctionMedian::tFunctionMedian() : tFunction() {}

	tStackElem tFunctionMedian::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		std::sort(wNumbers.begin(), wNumbers.end());
		const tSize wN = wNumbers.size();
		tDouble wMedian = 0.0;
		if ((wN % 2) == 1) {
			wMedian = wNumbers[wN / 2];
		} else {
			wMedian = (wNumbers[wN / 2 - 1] + wNumbers[wN / 2]) / 2.0;
		}
		return(MakeWholeOrDouble(wMedian));
	};

	tFunctionVariance::tFunctionVariance(tVarianceKind sKind) : tFunction(), m_Kind(sKind) {}

	tStackElem tFunctionVariance::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		const tBool wA =
			(m_Kind == tVarianceKind::StdevA || m_Kind == tVarianceKind::StdevPA ||
			 m_Kind == tVarianceKind::VarA || m_Kind == tVarianceKind::VarPA);
		if (wA) {
			if (!tAverageAArgCollector::Collect(wArgs, wNumbers, wError)) {
				return(tStackElem(wError));
			}
		} else if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		const tSize wN = wNumbers.size();
		const tBool wSample =
			(m_Kind == tVarianceKind::StdevS || m_Kind == tVarianceKind::VarS ||
			 m_Kind == tVarianceKind::StdevA || m_Kind == tVarianceKind::VarA);
		const tSize wMinN = wSample ? 2 : 1;
		if (wN < wMinN) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) {
			wSum += wV;
		}
		const tDouble wMean = wSum / static_cast<tDouble>(wN);
		tDouble wSq = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wD = wV - wMean;
			wSq += wD * wD;
		}
		const tDouble wDenom = wSample ? static_cast<tDouble>(wN - 1) : static_cast<tDouble>(wN);
		const tDouble wVar = wSq / wDenom;
		const tBool wSqrt =
			(m_Kind == tVarianceKind::StdevS || m_Kind == tVarianceKind::StdevP ||
			 m_Kind == tVarianceKind::StdevA || m_Kind == tVarianceKind::StdevPA);
		return(tStackElem(tVariant(wSqrt ? std::sqrt(wVar) : wVar)));
	};

	// AVERAGEA =============================================================
	tFunctionAverageA::tFunctionAverageA() : tFunction() {}

	tStackElem tFunctionAverageA::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tAverageAArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) wSum += wV;
		return(tStackElem(tVariant(wSum / static_cast<tDouble>(wNumbers.size()))));
	}

	tFunctionSpillKind tFunctionAverageA::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// SKEW / SKEW_P ========================================================
	tFunctionSkew::tFunctionSkew(tBool sPopulation) : tFunction(), m_Population(sPopulation) {}

	tStackElem tFunctionSkew::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		const tSize wN = wNumbers.size();
		if (wN < 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) wSum += wV;
		const tDouble wMean = wSum / static_cast<tDouble>(wN);
		tDouble wSq = 0.0, wCu = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wD = wV - wMean;
			wSq += wD * wD;
			wCu += wD * wD * wD;
		}
		const tDouble wNDbl = static_cast<tDouble>(wN);
		if (m_Population) {
			// Moment coefficient: m3 / m2^(3/2)
			const tDouble wM2 = wSq / wNDbl;
			if (wM2 <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
			}
			const tDouble wM3 = wCu / wNDbl;
			return(tStackElem(tVariant(wM3 / std::pow(wM2, 1.5))));
		}
		// Sample skewness (Excel SKEW)
		const tDouble wS2 = wSq / (wNDbl - 1.0);
		if (wS2 <= 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		const tDouble wS = std::sqrt(wS2);
		tDouble wSumZ3 = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wZ = (wV - wMean) / wS;
			wSumZ3 += wZ * wZ * wZ;
		}
		return(tStackElem(tVariant(wNDbl / ((wNDbl - 1.0) * (wNDbl - 2.0)) * wSumZ3)));
	}

	tFunctionSpillKind tFunctionSkew::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// KURT =================================================================
	tFunctionKurt::tFunctionKurt() : tFunction() {}

	tStackElem tFunctionKurt::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		const tSize wN = wNumbers.size();
		if (wN < 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) wSum += wV;
		const tDouble wNDbl = static_cast<tDouble>(wN);
		const tDouble wMean = wSum / wNDbl;
		tDouble wSq = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wD = wV - wMean;
			wSq += wD * wD;
		}
		const tDouble wS2 = wSq / (wNDbl - 1.0);
		if (wS2 <= 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		const tDouble wS = std::sqrt(wS2);
		tDouble wSumZ4 = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wZ = (wV - wMean) / wS;
			wSumZ4 += wZ * wZ * wZ * wZ;
		}
		const tDouble wTerm1 =
			(wNDbl * (wNDbl + 1.0)) / ((wNDbl - 1.0) * (wNDbl - 2.0) * (wNDbl - 3.0)) * wSumZ4;
		const tDouble wTerm2 =
			(3.0 * (wNDbl - 1.0) * (wNDbl - 1.0)) / ((wNDbl - 2.0) * (wNDbl - 3.0));
		return(tStackElem(tVariant(wTerm1 - wTerm2)));
	}

	tFunctionSpillKind tFunctionKurt::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// STANDARDIZE ==========================================================
	tFunctionStandardize::tFunctionStandardize() : tFunction() {}

	tStackElem tFunctionStandardize::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "STANDARDIZE requires 3 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wX, wMean, wSd;
		if (!StackElemToVariant(wArgs[0], wX) || !StackElemToVariant(wArgs[1], wMean) ||
			!StackElemToVariant(wArgs[2], wSd)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wX.IsError()) return(tStackElem(wX));
		if (wMean.IsError()) return(tStackElem(wMean));
		if (wSd.IsError()) return(tStackElem(wSd));
		if (!wX.IsNumeric() || !wMean.IsNumeric() || !wSd.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wStd = wSd.Numeric();
		if (wStd <= 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		return(tStackElem(tVariant((wX.Numeric() - wMean.Numeric()) / wStd)));
	}

	// FISHER / FISHERINV ===================================================
	tFunctionFisher::tFunctionFisher(tBool sInverse) : tFunction(), m_Inverse(sInverse) {}

	tStackElem tFunctionFisher::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FISHER/FISHERINV requires 1 argument"))));
		}
		tVariant wVar;
		if (!StackElemToVariant(wArgs[0], wVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wVar.IsError()) return(tStackElem(wVar));
		if (!wVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wX = wVar.Numeric();
		if (m_Inverse) {
			const tDouble wE = std::exp(2.0 * wX);
			if (!std::isfinite(wE)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			return(tStackElem(tVariant((wE - 1.0) / (wE + 1.0))));
		}
		if (wX <= -1.0 || wX >= 1.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		return(tStackElem(tVariant(0.5 * std::log((1.0 + wX) / (1.0 - wX)))));
	}

	// PHI ==================================================================
	tFunctionPhi::tFunctionPhi() : tFunction() {}

	tStackElem tFunctionPhi::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PHI requires 1 argument"))));
		}
		tVariant wVar;
		if (!StackElemToVariant(wArgs[0], wVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wVar.IsError()) return(tStackElem(wVar));
		if (!wVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wX = wVar.Numeric();
		const tDouble wY = (1.0 / std::sqrt(2.0 * M_PI)) * std::exp(-0.5 * wX * wX);
		return(tStackElem(tVariant(wY)));
	}

	// GAUSS ================================================================
	tFunctionGauss::tFunctionGauss() : tFunction() {}

	tStackElem tFunctionGauss::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "GAUSS requires 1 argument"))));
		}
		tVariant wVar;
		if (!StackElemToVariant(wArgs[0], wVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wVar.IsError()) return(tStackElem(wVar));
		if (wVar.IsNull()) wVar.SetDouble(0.0);
		if (!wVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		// NORMSDIST(x) − 0.5 = 0.5 * erf(x / √2)
		return(tStackElem(tVariant(0.5 * std::erf(wVar.Numeric() / std::sqrt(2.0)))));
	}

	namespace {
		static tDouble StdNormPdf(tDouble sZ) {
			return((1.0 / std::sqrt(2.0 * M_PI)) * std::exp(-0.5 * sZ * sZ));
		}

		static tDouble StdNormCdf(tDouble sZ) {
			return(0.5 * (1.0 + std::erf(sZ / std::sqrt(2.0))));
		}

		// Inverse standard normal CDF (Acklam rational approximation).
		static tDouble StdNormInv(tDouble sP) {
			static const tDouble a1 = -3.969683028665376e+01;
			static const tDouble a2 = 2.209460984245205e+02;
			static const tDouble a3 = -2.759285104469687e+02;
			static const tDouble a4 = 1.383577518672690e+02;
			static const tDouble a5 = -3.066479806614871e+01;
			static const tDouble a6 = 2.506628277459239e+00;
			static const tDouble b1 = -5.447609879822406e+01;
			static const tDouble b2 = 1.615858368580409e+02;
			static const tDouble b3 = -1.556989798598866e+02;
			static const tDouble b4 = 6.680131188771972e+01;
			static const tDouble b5 = -1.328068155288572e+01;
			static const tDouble c1 = -7.784894002430293e-03;
			static const tDouble c2 = -3.223964580411365e-01;
			static const tDouble c3 = -2.400758277161838e+00;
			static const tDouble c4 = -2.549732539343734e+00;
			static const tDouble c5 = 4.374664141464968e+00;
			static const tDouble c6 = 2.938163982698783e+00;
			static const tDouble d1 = 7.784695709041462e-03;
			static const tDouble d2 = 3.224671290700398e-01;
			static const tDouble d3 = 2.445134137142996e+00;
			static const tDouble d4 = 3.754408661907416e+00;
			static const tDouble pLow = 0.02425;
			static const tDouble pHigh = 1.0 - pLow;

			tDouble wQ, wR;
			if (sP < pLow) {
				wQ = std::sqrt(-2.0 * std::log(sP));
				return(((((c1 * wQ + c2) * wQ + c3) * wQ + c4) * wQ + c5) * wQ + c6) /
					   ((((d1 * wQ + d2) * wQ + d3) * wQ + d4) * wQ + 1.0);
			}
			if (sP <= pHigh) {
				wQ = sP - 0.5;
				wR = wQ * wQ;
				return((((((a1 * wR + a2) * wR + a3) * wR + a4) * wR + a5) * wR + a6) * wQ) /
					   (((((b1 * wR + b2) * wR + b3) * wR + b4) * wR + b5) * wR + 1.0);
			}
			wQ = std::sqrt(-2.0 * std::log(1.0 - sP));
			return(-(((((c1 * wQ + c2) * wQ + c3) * wQ + c4) * wQ + c5) * wQ + c6) /
				   ((((d1 * wQ + d2) * wQ + d3) * wQ + d4) * wQ + 1.0));
		}

		// Derived so helpers may call protected tFunction::StackElemToVariant.
		struct tNormArg : tFunction {
			static tBool CoerceNumericScalar(const tStackElem& sArg, tVariant& oOut) {
				if (!StackElemToVariant(sArg, oOut)) return(false);
				if (oOut.IsError()) return(true);
				if (oOut.IsNull()) {
					oOut.SetDouble(0.0);
					return(true);
				}
				return(oOut.IsNumeric());
			}
		};
	}

	// NORM.S.DIST / NORMSDIST ==============================================
	tFunctionNormSDist::tFunctionNormSDist(tBool sLegacy) : tFunction(), m_Legacy(sLegacy) {}

	tStackElem tFunctionNormSDist::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (m_Legacy) {
			if (wArgs.size() != 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NORMSDIST requires 1 argument"))));
			}
		} else if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NORM_S_DIST requires 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wZ;
		if (!tNormArg::CoerceNumericScalar(wArgs[0], wZ)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wZ.IsError()) return(tStackElem(wZ));
		tBool wCumulative = true;
		if (!m_Legacy) {
			if (!StackElemToBool(wArgs[1], wCumulative)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}
		const tDouble wX = wZ.Numeric();
		if (wCumulative) {
			return(tStackElem(tVariant(StdNormCdf(wX))));
		}
		return(tStackElem(tVariant(StdNormPdf(wX))));
	}

	// NORM.DIST / NORMDIST =================================================
	tFunctionNormDist::tFunctionNormDist() : tFunction() {}

	tStackElem tFunctionNormDist::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NORM_DIST requires 4 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wX, wMean, wSd;
		if (!tNormArg::CoerceNumericScalar(wArgs[0], wX) || !tNormArg::CoerceNumericScalar(wArgs[1], wMean) ||
			!tNormArg::CoerceNumericScalar(wArgs[2], wSd)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wX.IsError()) return(tStackElem(wX));
		if (wMean.IsError()) return(tStackElem(wMean));
		if (wSd.IsError()) return(tStackElem(wSd));
		tBool wCumulative = false;
		if (!StackElemToBool(wArgs[3], wCumulative)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wStd = wSd.Numeric();
		if (wStd <= 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		const tDouble wZ = (wX.Numeric() - wMean.Numeric()) / wStd;
		if (wCumulative) {
			return(tStackElem(tVariant(StdNormCdf(wZ))));
		}
		return(tStackElem(tVariant(StdNormPdf(wZ) / wStd)));
	}

	// NORM.S.INV / NORMSINV / NORM.INV / NORMINV ===========================
	tFunctionNormInv::tFunctionNormInv(tBool sStandard) : tFunction(), m_Standard(sStandard) {}

	tStackElem tFunctionNormInv::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (m_Standard) {
			if (wArgs.size() != 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NORM_S_INV requires 1 argument"))));
			}
		} else if (wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NORM_INV requires 3 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wP, wMean, wSd;
		if (!tNormArg::CoerceNumericScalar(wArgs[0], wP)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wP.IsError()) return(tStackElem(wP));
		tDouble wMeanVal = 0.0;
		tDouble wSdVal = 1.0;
		if (!m_Standard) {
			if (!tNormArg::CoerceNumericScalar(wArgs[1], wMean) || !tNormArg::CoerceNumericScalar(wArgs[2], wSd)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wMean.IsError()) return(tStackElem(wMean));
			if (wSd.IsError()) return(tStackElem(wSd));
			wMeanVal = wMean.Numeric();
			wSdVal = wSd.Numeric();
			if (wSdVal <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		const tDouble wProb = wP.Numeric();
		if (wProb <= 0.0 || wProb >= 1.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		return(tStackElem(tVariant(wMeanVal + wSdVal * StdNormInv(wProb))));
	}

	// GAMMA / GAMMALN ======================================================
	tFunctionGamma::tFunctionGamma(tBool sLn) : tFunction(), m_Ln(sLn) {}

	tStackElem tFunctionGamma::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "GAMMA/GAMMALN requires 1 argument"))));
		}
		tVariant wVar;
		if (!StackElemToVariant(wArgs[0], wVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wVar.IsError()) return(tStackElem(wVar));
		if (!wVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wX = wVar.Numeric();
		if (m_Ln) {
			if (wX <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			const tDouble wY = std::lgamma(wX);
			if (!std::isfinite(wY)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			return(tStackElem(tVariant(wY)));
		}
		// GAMMA: #NUM! for 0 and negative integers
		if (wX < 0.0) {
			const tDouble wNear = std::nearbyint(wX);
			if (std::fabs(wX - wNear) < 1.0e-12) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		if (wX == 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		const tDouble wY = std::tgamma(wX);
		if (!std::isfinite(wY)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		return(tStackElem(tVariant(wY)));
	}

	namespace {
		// Pair two equal-shaped arrays; skip non-numeric cells; propagate errors.
		static tBool CollectPairedNumeric(const tStackElem& sX, const tStackElem& sY,
										  std::vector<tDouble>& oX,
										  std::vector<tDouble>& oY,
										  tVariant& oError) {
			tArrayValue wX;
			tArrayValue wY;
			if (!tFunction::StackElemToArray(sX, wX) || !tFunction::StackElemToArray(sY, wY) ||
				wX.Count() <= 0 || wY.Count() <= 0) {
				oError = tVariant(tClassError(tTypeError::t_value, ""));
				return(false);
			}
			if (wX.m_Rows != wY.m_Rows || wX.m_Cols != wY.m_Cols) {
				oError = tVariant(tClassError(tTypeError::t_na, ""));
				return(false);
			}
			oX.clear();
			oY.clear();
			oX.reserve(static_cast<size_t>(wX.Count()));
			oY.reserve(static_cast<size_t>(wY.Count()));
			for (tIndex r = 0; r < wX.m_Rows; ++r) {
				for (tIndex c = 0; c < wX.m_Cols; ++c) {
					const tVariant& wXv = wX.At(r, c);
					const tVariant& wYv = wY.At(r, c);
					if (wXv.IsError()) { oError = wXv; return(false); }
					if (wYv.IsError()) { oError = wYv; return(false); }
					if (!wXv.IsNumeric() || !wYv.IsNumeric()) continue;
					oX.push_back(wXv.Numeric());
					oY.push_back(wYv.Numeric());
				}
			}
			return(true);
		}

		struct tLinRegMoments {
			tSize m_N = 0;
			tDouble m_SumX = 0.0;
			tDouble m_SumY = 0.0;
			tDouble m_SumXY = 0.0;
			tDouble m_SumX2 = 0.0;
			tDouble m_SumY2 = 0.0;
		};

		static tLinRegMoments LinRegMoments(const std::vector<tDouble>& sX,
											const std::vector<tDouble>& sY) {
			tLinRegMoments wM;
			wM.m_N = sX.size();
			for (tSize i = 0; i < wM.m_N; ++i) {
				wM.m_SumX += sX[i];
				wM.m_SumY += sY[i];
				wM.m_SumXY += sX[i] * sY[i];
				wM.m_SumX2 += sX[i] * sX[i];
				wM.m_SumY2 += sY[i] * sY[i];
			}
			return(wM);
		}

		static tBool LinRegSlopeIntercept(const tLinRegMoments& sM,
										  tDouble& oSlope,
										  tDouble& oIntercept) {
			if (sM.m_N < 2) return(false);
			const tDouble wN = static_cast<tDouble>(sM.m_N);
			const tDouble wDen = wN * sM.m_SumX2 - sM.m_SumX * sM.m_SumX;
			if (std::fabs(wDen) < 1.0e-15) return(false);
			oSlope = (wN * sM.m_SumXY - sM.m_SumX * sM.m_SumY) / wDen;
			oIntercept = (sM.m_SumY - oSlope * sM.m_SumX) / wN;
			return(true);
		}

		// PERCENTILE.INC / .EXC on a sorted ascending copy of sNumbers.
		static tBool PercentileValue(std::vector<tDouble> sNumbers,
									 tDouble sK,
									 tBool sExclusive,
									 tDouble& oOut,
									 tTypeError& oErr) {
			if (sNumbers.empty()) {
				oErr = tTypeError::t_num;
				return(false);
			}
			std::sort(sNumbers.begin(), sNumbers.end());
			const tSize wN = sNumbers.size();
			if (!sExclusive) {
				if (sK < 0.0 || sK > 1.0) {
					oErr = tTypeError::t_num;
					return(false);
				}
				if (wN == 1 || sK == 0.0) { oOut = sNumbers.front(); return(true); }
				if (sK == 1.0) { oOut = sNumbers.back(); return(true); }
				const tDouble wPos = sK * static_cast<tDouble>(wN - 1);
				const tSize wI = static_cast<tSize>(std::floor(wPos));
				const tDouble wF = wPos - static_cast<tDouble>(wI);
				if (wI + 1 >= wN) { oOut = sNumbers.back(); return(true); }
				oOut = sNumbers[wI] + wF * (sNumbers[wI + 1] - sNumbers[wI]);
				return(true);
			}
			// Exclusive: k in (0,1); position = k*(n+1) must be in [1, n].
			if (sK <= 0.0 || sK >= 1.0 || wN < 2) {
				oErr = tTypeError::t_num;
				return(false);
			}
			const tDouble wPos = sK * static_cast<tDouble>(wN + 1);
			if (wPos < 1.0 || wPos > static_cast<tDouble>(wN)) {
				oErr = tTypeError::t_num;
				return(false);
			}
			const tSize wI = static_cast<tSize>(std::floor(wPos));
			const tDouble wF = wPos - static_cast<tDouble>(wI);
			if (wI < 1) { oErr = tTypeError::t_num; return(false); }
			if (wF == 0.0) {
				oOut = sNumbers[wI - 1];
				return(true);
			}
			if (wI >= wN) { oOut = sNumbers.back(); return(true); }
			oOut = sNumbers[wI - 1] + wF * (sNumbers[wI] - sNumbers[wI - 1]);
			return(true);
		}
	} // namespace

	// AVEDEV ===============================================================
	tFunctionAveDev::tFunctionAveDev() : tFunction() {}

	tStackElem tFunctionAveDev::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) wSum += wV;
		const tDouble wMean = wSum / static_cast<tDouble>(wNumbers.size());
		tDouble wAbs = 0.0;
		for (tDouble wV : wNumbers) wAbs += std::fabs(wV - wMean);
		return MakeWholeOrDouble(wAbs / static_cast<tDouble>(wNumbers.size()));
	}

	tFunctionSpillKind tFunctionAveDev::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// DEVSQ ================================================================
	tFunctionDevSq::tFunctionDevSq() : tFunction() {}

	tStackElem tFunctionDevSq::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSum = 0.0;
		for (tDouble wV : wNumbers) wSum += wV;
		const tDouble wMean = wSum / static_cast<tDouble>(wNumbers.size());
		tDouble wSq = 0.0;
		for (tDouble wV : wNumbers) {
			const tDouble wD = wV - wMean;
			wSq += wD * wD;
		}
		return MakeWholeOrDouble(wSq);
	}

	tFunctionSpillKind tFunctionDevSq::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// CORREL / PEARSON =====================================================
	tFunctionCorrel::tFunctionCorrel() : tFunction() {}

	tStackElem tFunctionCorrel::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CORREL/PEARSON require 2 arguments"))));
		}
		// RPN: array2, array1
		std::vector<tDouble> wX, wY;
		tVariant wError;
		if (!CollectPairedNumeric(wArgs[1], wArgs[0], wX, wY, wError)) {
			return(tStackElem(wError));
		}
		const tLinRegMoments wM = LinRegMoments(wX, wY);
		if (wM.m_N < 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		const tDouble wN = static_cast<tDouble>(wM.m_N);
		const tDouble wNum = wN * wM.m_SumXY - wM.m_SumX * wM.m_SumY;
		const tDouble wDenX = wN * wM.m_SumX2 - wM.m_SumX * wM.m_SumX;
		const tDouble wDenY = wN * wM.m_SumY2 - wM.m_SumY * wM.m_SumY;
		if (wDenX <= 0.0 || wDenY <= 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		return(tStackElem(tVariant(wNum / std::sqrt(wDenX * wDenY))));
	}

	tFunctionSpillKind tFunctionCorrel::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// COVARIANCE_P / COVARIANCE_S ==========================================
	tFunctionCovariance::tFunctionCovariance(tBool sSample) : tFunction(), m_Sample(sSample) {}

	tStackElem tFunctionCovariance::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COVARIANCE requires 2 arguments"))));
		}
		std::vector<tDouble> wX, wY;
		tVariant wError;
		if (!CollectPairedNumeric(wArgs[1], wArgs[0], wX, wY, wError)) {
			return(tStackElem(wError));
		}
		const tSize wN = wX.size();
		const tSize wMin = m_Sample ? 2 : 1;
		if (wN < wMin) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		tDouble wSumX = 0.0, wSumY = 0.0;
		for (tSize i = 0; i < wN; ++i) {
			wSumX += wX[i];
			wSumY += wY[i];
		}
		const tDouble wMeanX = wSumX / static_cast<tDouble>(wN);
		const tDouble wMeanY = wSumY / static_cast<tDouble>(wN);
		tDouble wCov = 0.0;
		for (tSize i = 0; i < wN; ++i) {
			wCov += (wX[i] - wMeanX) * (wY[i] - wMeanY);
		}
		const tDouble wDenom = m_Sample ? static_cast<tDouble>(wN - 1) : static_cast<tDouble>(wN);
		return(tStackElem(tVariant(wCov / wDenom)));
	}

	tFunctionSpillKind tFunctionCovariance::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// PERCENTILE_INC / PERCENTILE_EXC ======================================
	tFunctionPercentile::tFunctionPercentile(tBool sExclusive) : tFunction(), m_Exclusive(sExclusive) {}

	tStackElem tFunctionPercentile::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PERCENTILE requires 2 arguments"))));
		}
		// RPN: k, array
		tVariant wKVar;
		if (!StackElemToVariant(wArgs[0], wKVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wKVar.IsError()) return(tStackElem(wKVar));
		if (!wKVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wNumbers, wError)) {
			return(tStackElem(wError));
		}
		tDouble wOut = 0.0;
		tTypeError wErr = tTypeError::t_num;
		if (!PercentileValue(std::move(wNumbers), wKVar.Numeric(), m_Exclusive, wOut, wErr)) {
			return(tStackElem(tVariant(tClassError(wErr, ""))));
		}
		return MakeWholeOrDouble(wOut);
	}

	tFunctionSpillKind tFunctionPercentile::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// QUARTILE_INC / QUARTILE_EXC ==========================================
	tFunctionQuartile::tFunctionQuartile(tBool sExclusive) : tFunction(), m_Exclusive(sExclusive) {}

	tStackElem tFunctionQuartile::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "QUARTILE requires 2 arguments"))));
		}
		// RPN: quart, array
		tInt wQ = 0;
		if (!StackElemToInt(wArgs[0], wQ)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		tDouble wK = 0.0;
		if (m_Exclusive) {
			if (wQ < 1 || wQ > 3) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wK = static_cast<tDouble>(wQ) * 0.25;
		} else {
			if (wQ < 0 || wQ > 4) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wK = static_cast<tDouble>(wQ) * 0.25;
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wNumbers, wError)) {
			return(tStackElem(wError));
		}
		tDouble wOut = 0.0;
		tTypeError wErr = tTypeError::t_num;
		if (!PercentileValue(std::move(wNumbers), wK, m_Exclusive, wOut, wErr)) {
			return(tStackElem(tVariant(tClassError(wErr, ""))));
		}
		return MakeWholeOrDouble(wOut);
	}

	tFunctionSpillKind tFunctionQuartile::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// SLOPE / INTERCEPT / RSQ / STEYX ======================================
	tFunctionLinReg::tFunctionLinReg(tLinRegKind sKind) : tFunction(), m_Kind(sKind) {}

	tStackElem tFunctionLinReg::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SLOPE/INTERCEPT/RSQ/STEYX require 2 arguments"))));
		}
		// RPN: known_x, known_y
		std::vector<tDouble> wX, wY;
		tVariant wError;
		if (!CollectPairedNumeric(wArgs[0], wArgs[1], wX, wY, wError)) {
			return(tStackElem(wError));
		}
		const tLinRegMoments wM = LinRegMoments(wX, wY);
		if (m_Kind == tLinRegKind::Rsq) {
			if (wM.m_N < 2) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
			}
			const tDouble wN = static_cast<tDouble>(wM.m_N);
			const tDouble wNum = wN * wM.m_SumXY - wM.m_SumX * wM.m_SumY;
			const tDouble wDenX = wN * wM.m_SumX2 - wM.m_SumX * wM.m_SumX;
			const tDouble wDenY = wN * wM.m_SumY2 - wM.m_SumY * wM.m_SumY;
			if (wDenX <= 0.0 || wDenY <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
			}
			const tDouble wR = wNum / std::sqrt(wDenX * wDenY);
			return(tStackElem(tVariant(wR * wR)));
		}
		tDouble wSlope = 0.0, wIntercept = 0.0;
		if (!LinRegSlopeIntercept(wM, wSlope, wIntercept)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		if (m_Kind == tLinRegKind::Steyx) {
			if (wM.m_N < 3) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
			}
			tDouble wSse = 0.0;
			for (tSize i = 0; i < wM.m_N; ++i) {
				const tDouble wResid = wY[i] - (wIntercept + wSlope * wX[i]);
				wSse += wResid * wResid;
			}
			return(tStackElem(tVariant(std::sqrt(wSse / static_cast<tDouble>(wM.m_N - 2)))));
		}
		return(tStackElem(tVariant(m_Kind == tLinRegKind::Slope ? wSlope : wIntercept)));
	}

	tFunctionSpillKind tFunctionLinReg::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// FORECAST / FORECAST_LINEAR ===========================================
	tFunctionForecast::tFunctionForecast() : tFunction() {}

	tStackElem tFunctionForecast::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FORECAST requires 3 arguments"))));
		}
		// RPN: known_x, known_y, x
		tVariant wXVar;
		if (!StackElemToVariant(wArgs[2], wXVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wXVar.IsError()) return(tStackElem(wXVar));
		if (!wXVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		std::vector<tDouble> wX, wY;
		tVariant wError;
		if (!CollectPairedNumeric(wArgs[0], wArgs[1], wX, wY, wError)) {
			return(tStackElem(wError));
		}
		const tLinRegMoments wM = LinRegMoments(wX, wY);
		tDouble wSlope = 0.0, wIntercept = 0.0;
		if (!LinRegSlopeIntercept(wM, wSlope, wIntercept)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		return(tStackElem(tVariant(wIntercept + wSlope * wXVar.Numeric())));
	}

	tFunctionSpillKind tFunctionForecast::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	namespace {
		// Collect numeric cells in row-major order; propagate errors; ignore non-numeric.
		static tBool CollectFlatNumeric(const tStackElem& sArg,
										std::vector<tDouble>& oOut,
										tVariant& oError) {
			tArrayValue wArr;
			if (!tFunction::StackElemToArray(sArg, wArr) || wArr.Count() <= 0) {
				oError = tVariant(tClassError(tTypeError::t_value, ""));
				return(false);
			}
			oOut.clear();
			oOut.reserve(static_cast<size_t>(wArr.Count()));
			for (tIndex r = 0; r < wArr.m_Rows; ++r) {
				for (tIndex c = 0; c < wArr.m_Cols; ++c) {
					const tVariant& wV = wArr.At(r, c);
					if (wV.IsError()) { oError = wV; return(false); }
					if (!wV.IsNumeric()) continue;
					oOut.push_back(wV.Numeric());
				}
			}
			return(true);
		}

		static tBool LinRegSlopeInterceptConst(const tLinRegMoments& sM,
											   tBool sForceZeroIntercept,
											   tDouble& oSlope,
											   tDouble& oIntercept) {
			if (sForceZeroIntercept) {
				if (sM.m_N < 1 || std::fabs(sM.m_SumX2) < 1.0e-15) return(false);
				oSlope = sM.m_SumXY / sM.m_SumX2;
				oIntercept = 0.0;
				return(true);
			}
			return(LinRegSlopeIntercept(sM, oSlope, oIntercept));
		}
	}

	// TREND =================================================================
	tFunctionTrend::tFunctionTrend() : tFunction() {}

	tStackElem tFunctionTrend::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty() || wArgs.size() > 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TREND requires 1 to 4 arguments"))));
		}
		// RPN → natural: known_y, [known_x], [new_x], [const]
		std::reverse(wArgs.begin(), wArgs.end());

		tBool wConst = true; // TRUE/omitted → free intercept; FALSE → through origin
		if (wArgs.size() == 4) {
			if (!StackElemToBool(wArgs[3], wConst)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}

		std::vector<tDouble> wKnownX, wKnownY;
		tVariant wError;
		tArrayValue wNewShape; // shape of returned predictions
		tBool wHaveNewShape = false;

		if (wArgs.size() == 1) {
			// known_x = {1..n}; new_x = known_x; shape follows known_y
			tArrayValue wYArr;
			if (!StackElemToArray(wArgs[0], wYArr) || wYArr.Count() <= 0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (!CollectFlatNumeric(wArgs[0], wKnownY, wError)) {
				return(tStackElem(wError));
			}
			if (wKnownY.empty()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wKnownX.reserve(wKnownY.size());
			for (tSize i = 0; i < wKnownY.size(); ++i) {
				wKnownX.push_back(static_cast<tDouble>(i + 1));
			}
			wNewShape = wYArr;
			wHaveNewShape = true;
		} else {
			// known_y + known_x (paired, same shape)
			if (!CollectPairedNumeric(wArgs[1], wArgs[0], wKnownX, wKnownY, wError)) {
				return(tStackElem(wError));
			}
			if (wArgs.size() >= 3) {
				if (!StackElemToArray(wArgs[2], wNewShape) || wNewShape.Count() <= 0) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				}
				wHaveNewShape = true;
			} else {
				// new_x omitted → same as known_x
				if (!StackElemToArray(wArgs[1], wNewShape) || wNewShape.Count() <= 0) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				}
				wHaveNewShape = true;
			}
		}
		(void)wHaveNewShape;

		const tLinRegMoments wM = LinRegMoments(wKnownX, wKnownY);
		tDouble wSlope = 0.0, wIntercept = 0.0;
		if (!LinRegSlopeInterceptConst(wM, !wConst, wSlope, wIntercept)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}

		// Default new_x values when only known_y was supplied: 1..n in known_y shape.
		if (wArgs.size() == 1) {
			tArrayValue* wOut = new tArrayValue(wNewShape.m_Rows, wNewShape.m_Cols);
			tSize wIdx = 0;
			for (tIndex r = 0; r < wNewShape.m_Rows; ++r) {
				for (tIndex c = 0; c < wNewShape.m_Cols; ++c) {
					const tVariant& wCell = wNewShape.At(r, c);
					if (wCell.IsError()) {
						delete wOut;
						return(tStackElem(wCell));
					}
					if (!wCell.IsNumeric()) {
						// Skip non-numeric cells in known_y: leave blank in output.
						continue;
					}
					if (wIdx >= wKnownX.size()) {
						delete wOut;
						return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
					}
					wOut->At(r, c) = tVariant(wIntercept + wSlope * wKnownX[wIdx]);
					++wIdx;
				}
			}
			return(tStackElem(wOut));
		}

		tArrayValue* wOut = new tArrayValue(wNewShape.m_Rows, wNewShape.m_Cols);
		for (tIndex r = 0; r < wNewShape.m_Rows; ++r) {
			for (tIndex c = 0; c < wNewShape.m_Cols; ++c) {
				const tVariant& wXv = wNewShape.At(r, c);
				if (wXv.IsError()) {
					delete wOut;
					return(tStackElem(wXv));
				}
				if (!wXv.IsNumeric()) {
					delete wOut;
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				}
				wOut->At(r, c) = tVariant(wIntercept + wSlope * wXv.Numeric());
			}
		}
		return(tStackElem(wOut));
	}

	// GROWTH ================================================================
	tFunctionGrowth::tFunctionGrowth() : tFunction() {}

	tStackElem tFunctionGrowth::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty() || wArgs.size() > 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "GROWTH requires 1 to 4 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());

		tBool wConst = true;
		if (wArgs.size() == 4) {
			if (!StackElemToBool(wArgs[3], wConst)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}

		std::vector<tDouble> wKnownX, wKnownY;
		tVariant wError;
		tArrayValue wNewShape;

		if (wArgs.size() == 1) {
			tArrayValue wYArr;
			if (!StackElemToArray(wArgs[0], wYArr) || wYArr.Count() <= 0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (!CollectFlatNumeric(wArgs[0], wKnownY, wError)) {
				return(tStackElem(wError));
			}
			if (wKnownY.empty()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wKnownX.reserve(wKnownY.size());
			for (tSize i = 0; i < wKnownY.size(); ++i) {
				wKnownX.push_back(static_cast<tDouble>(i + 1));
			}
			wNewShape = wYArr;
		} else {
			if (!CollectPairedNumeric(wArgs[1], wArgs[0], wKnownX, wKnownY, wError)) {
				return(tStackElem(wError));
			}
			if (wArgs.size() >= 3) {
				if (!StackElemToArray(wArgs[2], wNewShape) || wNewShape.Count() <= 0) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				}
			} else if (!StackElemToArray(wArgs[1], wNewShape) || wNewShape.Count() <= 0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}

		// Exponential fit: OLS on (x, ln(y)); all known_y must be > 0.
		std::vector<tDouble> wLnY;
		wLnY.reserve(wKnownY.size());
		for (tDouble wY : wKnownY) {
			if (wY <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wLnY.push_back(std::log(wY));
		}
		const tLinRegMoments wM = LinRegMoments(wKnownX, wLnY);
		tDouble wLnM = 0.0, wLnB = 0.0;
		if (!LinRegSlopeInterceptConst(wM, !wConst, wLnM, wLnB)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		// Predict y = exp(lnB + lnM·x) = b·m^x (const FALSE → b=1).

		auto wPredict = [&](tDouble sX) -> tDouble {
			return(std::exp(wLnB + wLnM * sX));
		};

		if (wArgs.size() == 1) {
			tArrayValue* wOut = new tArrayValue(wNewShape.m_Rows, wNewShape.m_Cols);
			tSize wIdx = 0;
			for (tIndex r = 0; r < wNewShape.m_Rows; ++r) {
				for (tIndex c = 0; c < wNewShape.m_Cols; ++c) {
					const tVariant& wCell = wNewShape.At(r, c);
					if (wCell.IsError()) {
						delete wOut;
						return(tStackElem(wCell));
					}
					if (!wCell.IsNumeric()) continue;
					if (wIdx >= wKnownX.size()) {
						delete wOut;
						return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
					}
					wOut->At(r, c) = tVariant(wPredict(wKnownX[wIdx]));
					++wIdx;
				}
			}
			return(tStackElem(wOut));
		}

		tArrayValue* wOut = new tArrayValue(wNewShape.m_Rows, wNewShape.m_Cols);
		for (tIndex r = 0; r < wNewShape.m_Rows; ++r) {
			for (tIndex c = 0; c < wNewShape.m_Cols; ++c) {
				const tVariant& wXv = wNewShape.At(r, c);
				if (wXv.IsError()) {
					delete wOut;
					return(tStackElem(wXv));
				}
				if (!wXv.IsNumeric()) {
					delete wOut;
					return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
				}
				wOut->At(r, c) = tVariant(wPredict(wXv.Numeric()));
			}
		}
		return(tStackElem(wOut));
	}

	// LINEST ================================================================
	tFunctionLinEst::tFunctionLinEst() : tFunction() {}

	tStackElem tFunctionLinEst::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty() || wArgs.size() > 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LINEST requires 1 to 4 arguments"))));
		}
		// RPN → natural: known_y, [known_x], [const], [stats]
		std::reverse(wArgs.begin(), wArgs.end());

		tBool wConst = true;
		tBool wStats = false;
		if (wArgs.size() >= 3) {
			if (!StackElemToBool(wArgs[2], wConst)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}
		if (wArgs.size() >= 4) {
			if (!StackElemToBool(wArgs[3], wStats)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
		}

		std::vector<tDouble> wKnownX, wKnownY;
		tVariant wError;
		if (wArgs.size() == 1) {
			if (!CollectFlatNumeric(wArgs[0], wKnownY, wError)) {
				return(tStackElem(wError));
			}
			if (wKnownY.empty()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wKnownX.reserve(wKnownY.size());
			for (tSize i = 0; i < wKnownY.size(); ++i) {
				wKnownX.push_back(static_cast<tDouble>(i + 1));
			}
		} else {
			if (!CollectPairedNumeric(wArgs[1], wArgs[0], wKnownX, wKnownY, wError)) {
				return(tStackElem(wError));
			}
		}

		const tLinRegMoments wM = LinRegMoments(wKnownX, wKnownY);
		tDouble wSlope = 0.0, wIntercept = 0.0;
		if (!LinRegSlopeInterceptConst(wM, !wConst, wSlope, wIntercept)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}

		const tIndex wRows = wStats ? 5 : 1;
		tArrayValue* wOut = new tArrayValue(wRows, 2);
		wOut->At(0, 0) = tVariant(wSlope);
		wOut->At(0, 1) = tVariant(wIntercept);
		if (!wStats) {
			return(tStackElem(wOut));
		}

		const tDouble wN = static_cast<tDouble>(wM.m_N);
		const tDouble wDf = wConst ? (wN - 2.0) : (wN - 1.0);
		if (wDf < 1.0) {
			delete wOut;
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}

		tDouble wSse = 0.0;
		tDouble wSst = 0.0;
		const tDouble wMeanY = wM.m_SumY / wN;
		for (tSize i = 0; i < wM.m_N; ++i) {
			const tDouble wFit = wIntercept + wSlope * wKnownX[i];
			const tDouble wResid = wKnownY[i] - wFit;
			wSse += wResid * wResid;
			const tDouble wDy = wKnownY[i] - (wConst ? wMeanY : 0.0);
			wSst += wDy * wDy;
		}
		const tDouble wSsr = wSst - wSse;
		const tDouble wSey = std::sqrt(wSse / wDf);

		// Sxx = Σ(x − x̄)² (free intercept) or Σx² (through origin).
		tDouble wSxx = 0.0;
		if (wConst) {
			const tDouble wMeanX = wM.m_SumX / wN;
			wSxx = wM.m_SumX2 - wN * wMeanX * wMeanX;
		} else {
			wSxx = wM.m_SumX2;
		}
		tDouble wSeM = 0.0;
		tDouble wSeB = 0.0;
		if (wSxx > 1.0e-15) {
			wSeM = wSey / std::sqrt(wSxx);
			if (wConst) {
				const tDouble wMeanX = wM.m_SumX / wN;
				wSeB = wSey * std::sqrt(1.0 / wN + (wMeanX * wMeanX) / wSxx);
			}
		}

		tDouble wR2 = 0.0;
		if (wSst > 1.0e-15) {
			wR2 = 1.0 - wSse / wSst;
		} else if (wSse <= 1.0e-15) {
			wR2 = 1.0;
		}

		tDouble wF = 0.0;
		if (wR2 < 1.0 && (1.0 - wR2) > 1.0e-15) {
			wF = wR2 / ((1.0 - wR2) / wDf);
		} else if (wR2 >= 1.0 - 1.0e-15) {
			wF = 1.0e308; // Perfect fit — Excel shows a huge F
		}

		wOut->At(1, 0) = tVariant(wSeM);
		if (wConst) {
			wOut->At(1, 1) = tVariant(wSeB);
		} else {
			wOut->At(1, 1) = tVariant(tClassError(tTypeError::t_na, ""));
		}
		wOut->At(2, 0) = tVariant(wR2);
		wOut->At(2, 1) = tVariant(wSey);
		wOut->At(3, 0) = tVariant(wF);
		wOut->At(3, 1) = tVariant(wDf);
		wOut->At(4, 0) = tVariant(wSsr);
		wOut->At(4, 1) = tVariant(wSse);
		return(tStackElem(wOut));
	}

	namespace {
		// Round percentage rank to `significance` digits after the decimal (Excel "0.xxx" default 3).
		static tDouble RoundPercentRank(tDouble sValue, tInt sSignificance) {
			const tDouble wPow = std::pow(10.0, static_cast<tDouble>(sSignificance));
			return(std::floor(sValue * wPow + 0.5) / wPow);
		}

		static tBool PercentRankValue(std::vector<tDouble> sNumbers,
									  tDouble sX,
									  tBool sExclusive,
									  tDouble& oOut,
									  tTypeError& oErr) {
			if (sNumbers.empty()) {
				oErr = tTypeError::t_num;
				return(false);
			}
			std::sort(sNumbers.begin(), sNumbers.end());
			const tSize wN = sNumbers.size();
			if (sX < sNumbers.front() || sX > sNumbers.back()) {
				oErr = tTypeError::t_na;
				return(false);
			}
			if (!sExclusive) {
				if (wN == 1) {
					oOut = 1.0;
					return(true);
				}
				for (tSize i = 0; i < wN; ++i) {
					if (sNumbers[i] == sX) {
						oOut = static_cast<tDouble>(i) / static_cast<tDouble>(wN - 1);
						return(true);
					}
					if (i + 1 < wN && sNumbers[i] < sX && sX < sNumbers[i + 1]) {
						const tDouble wFrac = (sX - sNumbers[i]) / (sNumbers[i + 1] - sNumbers[i]);
						oOut = (static_cast<tDouble>(i) + wFrac) / static_cast<tDouble>(wN - 1);
						return(true);
					}
				}
			} else {
				if (wN < 2) {
					oErr = tTypeError::t_num;
					return(false);
				}
				for (tSize i = 0; i < wN; ++i) {
					if (sNumbers[i] == sX) {
						oOut = static_cast<tDouble>(i + 1) / static_cast<tDouble>(wN + 1);
						return(true);
					}
					if (i + 1 < wN && sNumbers[i] < sX && sX < sNumbers[i + 1]) {
						const tDouble wFrac = (sX - sNumbers[i]) / (sNumbers[i + 1] - sNumbers[i]);
						oOut = (static_cast<tDouble>(i + 1) + wFrac) / static_cast<tDouble>(wN + 1);
						return(true);
					}
				}
			}
			oErr = tTypeError::t_na;
			return(false);
		}
	}

	// PERCENTRANK_INC / PERCENTRANK_EXC ====================================
	tFunctionPercentRank::tFunctionPercentRank(tBool sExclusive) : tFunction(), m_Exclusive(sExclusive) {}

	tStackElem tFunctionPercentRank::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2 && wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PERCENTRANK requires 2 or 3 arguments"))));
		}
		// RPN: [significance,] x, array
		std::reverse(wArgs.begin(), wArgs.end());
		tInt wSignificance = 3;
		if (wArgs.size() == 3) {
			if (!StackElemToInt(wArgs[2], wSignificance)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wSignificance < 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		tVariant wXVar;
		if (!StackElemToVariant(wArgs[1], wXVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wXVar.IsError()) return(tStackElem(wXVar));
		if (!wXVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[0], wNumbers, wError)) {
			return(tStackElem(wError));
		}
		tDouble wOut = 0.0;
		tTypeError wErr = tTypeError::t_num;
		if (!PercentRankValue(std::move(wNumbers), wXVar.Numeric(), m_Exclusive, wOut, wErr)) {
			return(tStackElem(tVariant(tClassError(wErr, ""))));
		}
		return(tStackElem(tVariant(RoundPercentRank(wOut, wSignificance))));
	}

	tFunctionSpillKind tFunctionPercentRank::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// PERMUT / PERMUTATIONA ================================================
	tFunctionPermut::tFunctionPermut(tBool sWithRep) : tFunction(), m_WithRep(sWithRep) {}

	tStackElem tFunctionPermut::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PERMUT/PERMUTATIONA requires 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wNVar, wKVar;
		if (!StackElemToVariant(wArgs[0], wNVar) || !StackElemToVariant(wArgs[1], wKVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wNVar.IsError()) return(tStackElem(wNVar));
		if (wKVar.IsError()) return(tStackElem(wKVar));
		if (!wNVar.IsNumeric() || !wKVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tInt wN = static_cast<tInt>(std::trunc(wNVar.Numeric()));
		const tInt wK = static_cast<tInt>(std::trunc(wKVar.Numeric()));
		if (wN < 0 || wK < 0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		if (m_WithRep) {
			// PERMUTATIONA(n,k) = n^k
			if (wK == 0) return MakeWholeOrDouble(1.0);
			if (wN == 0) return MakeWholeOrDouble(0.0);
			tDouble wP = 1.0;
			for (tInt i = 0; i < wK; ++i) {
				wP *= static_cast<tDouble>(wN);
				if (!std::isfinite(wP)) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
				}
			}
			return MakeWholeOrDouble(std::round(wP));
		}
		// PERMUT(n,k) = n! / (n-k)!
		if (wK > wN) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tDouble wP = 1.0;
		for (tInt i = 0; i < wK; ++i) {
			wP *= static_cast<tDouble>(wN - i);
			if (!std::isfinite(wP)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		return MakeWholeOrDouble(std::round(wP));
	}

	// GEOMEAN ==============================================================
	tFunctionGeoMean::tFunctionGeoMean() : tFunction() {}

	tStackElem tFunctionGeoMean::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tDouble wLogSum = 0.0;
		for (tDouble wV : wNumbers) {
			if (wV <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wLogSum += std::log(wV);
		}
		return(tStackElem(tVariant(std::exp(wLogSum / static_cast<tDouble>(wNumbers.size())))));
	}

	tFunctionSpillKind tFunctionGeoMean::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// HARMEAN ==============================================================
	tFunctionHarMean::tFunctionHarMean() : tFunction() {}

	tStackElem tFunctionHarMean::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tDouble wInvSum = 0.0;
		for (tDouble wV : wNumbers) {
			if (wV <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wInvSum += 1.0 / wV;
		}
		return(tStackElem(tVariant(static_cast<tDouble>(wNumbers.size()) / wInvSum)));
	}

	tFunctionSpillKind tFunctionHarMean::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// TRIMMEAN =============================================================
	tFunctionTrimMean::tFunctionTrimMean() : tFunction() {}

	tStackElem tFunctionTrimMean::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRIMMEAN requires 2 arguments"))));
		}
		// RPN: percent, array
		tVariant wPctVar;
		if (!StackElemToVariant(wArgs[0], wPctVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wPctVar.IsError()) return(tStackElem(wPctVar));
		if (!wPctVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wPercent = wPctVar.Numeric();
		if (wPercent < 0.0 || wPercent >= 1.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		std::sort(wNumbers.begin(), wNumbers.end());
		const tSize wN = wNumbers.size();
		tSize wDropTotal = static_cast<tSize>(std::floor(wPercent * static_cast<tDouble>(wN)));
		wDropTotal -= (wDropTotal % 2); // round down to even
		const tSize wEachEnd = wDropTotal / 2;
		if (wEachEnd * 2 >= wN) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tDouble wSum = 0.0;
		const tSize wLast = wN - wEachEnd;
		for (tSize i = wEachEnd; i < wLast; ++i) {
			wSum += wNumbers[i];
		}
		const tSize wCount = wN - 2 * wEachEnd;
		return(tStackElem(tVariant(wSum / static_cast<tDouble>(wCount))));
	}

	tFunctionSpillKind tFunctionTrimMean::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// FREQUENCY ============================================================
	tFunctionFrequency::tFunctionFrequency() : tFunction() {}

	tStackElem tFunctionFrequency::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FREQUENCY requires 2 arguments"))));
		}
		// RPN: bins_array, data_array
		std::vector<tDouble> wData;
		std::vector<tDouble> wBins;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wData, wError)) {
			return(tStackElem(wError));
		}
		if (!tNumericArgCollector::Append(wArgs[0], wBins, wError)) {
			return(tStackElem(wError));
		}
		std::sort(wBins.begin(), wBins.end());
		const tIndex wRows = static_cast<tIndex>(wBins.size() + 1);
		tArrayValue* wOut = new tArrayValue(wRows, 1);
		std::vector<tInt> wCounts(static_cast<tSize>(wRows), 0);
		for (tDouble wV : wData) {
			tBool wPlaced = false;
			for (tSize b = 0; b < wBins.size(); ++b) {
				if (wV <= wBins[b]) {
					++wCounts[b];
					wPlaced = true;
					break;
				}
			}
			if (!wPlaced) {
				++wCounts[wBins.size()];
			}
		}
		for (tIndex r = 0; r < wRows; ++r) {
			wOut->At(r, 0) = tVariant(wCounts[static_cast<tSize>(r)]);
		}
		return(tStackElem(wOut));
	}

	// LARGE / SMALL ========================================================
	tFunctionNth::tFunctionNth(tBool sLarge) : tFunction(), m_Large(sLarge) {}

	tStackElem tFunctionNth::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		// LARGE/SMALL(array, k): PopArgs → [k, array]
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LARGE/SMALL require 2 arguments"))));
		}
		tInt wK = 0;
		if (!StackElemToInt(wArgs[0], wK)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		// Excel truncates a fractional k toward zero (via int coercion).
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty() || wK < 1 || static_cast<tSize>(wK) > wNumbers.size()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		if (m_Large) {
			std::sort(wNumbers.begin(), wNumbers.end(), std::greater<tDouble>());
		} else {
			std::sort(wNumbers.begin(), wNumbers.end());
		}
		return(MakeWholeOrDouble(wNumbers[static_cast<tSize>(wK) - 1]));
	};

	// MODE_SNGL / MODE / MODE_MULT =========================================
	tFunctionMode::tFunctionMode(tBool sMulti) : tFunction(), m_Multi(sMulti) {}

	tStackElem tFunctionMode::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}

		// Count frequencies in first-appearance order (Excel MODE* tie-break).
		std::vector<tDouble> wUnique;
		std::vector<tSize> wCounts;
		wUnique.reserve(wNumbers.size());
		wCounts.reserve(wNumbers.size());
		tSize wMaxCount = 0;
		for (tDouble wV : wNumbers) {
			tSize wIdx = wUnique.size();
			for (tSize i = 0; i < wUnique.size(); ++i) {
				if (wUnique[i] == wV) {
					wIdx = i;
					break;
				}
			}
			if (wIdx == wUnique.size()) {
				wUnique.push_back(wV);
				wCounts.push_back(1);
			} else {
				++wCounts[wIdx];
			}
			if (wCounts[wIdx] > wMaxCount) {
				wMaxCount = wCounts[wIdx];
			}
		}
		if (wMaxCount < 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		if (!m_Multi) {
			for (tSize i = 0; i < wUnique.size(); ++i) {
				if (wCounts[i] == wMaxCount) {
					return(MakeWholeOrDouble(wUnique[i]));
				}
			}
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}
		// MODE_MULT: vertical spill of every mode (same max frequency).
		std::vector<tDouble> wModes;
		for (tSize i = 0; i < wUnique.size(); ++i) {
			if (wCounts[i] == wMaxCount) {
				wModes.push_back(wUnique[i]);
			}
		}
		tArrayValue* wOut = new tArrayValue(static_cast<tIndex>(wModes.size()), 1);
		for (tSize i = 0; i < wModes.size(); ++i) {
			const tDouble wV = wModes[i];
			if (std::isfinite(wV) && wV == std::floor(wV) && std::fabs(wV) < 9.0e15) {
				wOut->At(static_cast<tIndex>(i), 0) = tVariant(static_cast<tInt>(wV));
			} else {
				wOut->At(static_cast<tIndex>(i), 0) = tVariant(wV);
			}
		}
		return(tStackElem(wOut));
	};

	// RANK_EQ / RANK / RANK_AVG ============================================
	tFunctionRankEq::tFunctionRankEq(tBool sAverage) : tFunction(), m_Average(sAverage) {}

	tStackElem tFunctionRankEq::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		// RANK_EQ(number, ref, [order]) — PopArgs last-first
		if (wArgs.size() != 2 && wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
		}
		std::reverse(wArgs.begin(), wArgs.end());

		tVariant wNumberVar;
		if (!StackElemToVariant(wArgs[0], wNumberVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wNumberVar.IsError()) {
			return(tStackElem(wNumberVar));
		}
		std::vector<tDouble> wNumberOnly;
		if (!MedianPushNumber(wNumberOnly, wNumberVar) || wNumberOnly.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tDouble wNumber = wNumberOnly[0];

		std::vector<tDouble> wRef;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wRef, wError)) {
			return(tStackElem(wError));
		}
		if (wRef.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
		}

		// order: 0/omitted = descending; nonzero = ascending.
		tBool wAscending = false;
		if (wArgs.size() == 3) {
			tInt wOrder = 0;
			if (!StackElemToInt(wArgs[2], wOrder)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wAscending = (wOrder != 0);
		}

		tInt wBetter = 0;
		tInt wEqual = 0;
		for (tDouble wV : wRef) {
			if (wV == wNumber) {
				++wEqual;
			} else if (wAscending) {
				if (wV < wNumber) ++wBetter;
			} else if (wV > wNumber) {
				++wBetter;
			}
		}
		// RANK.EQ / absent value: rank = (# better) + 1.
		// RANK.AVG with ties: average of ranks from (better+1) .. (better+equal).
		if (m_Average && wEqual > 1) {
			const tDouble wFirst = static_cast<tDouble>(wBetter + 1);
			const tDouble wLast = static_cast<tDouble>(wBetter + wEqual);
			return(tStackElem(tVariant((wFirst + wLast) / 2.0)));
		}
		return(tStackElem(tVariant(wBetter + 1)));
	};

	// MAXA / MINA ==========================================================
	tFunctionMinMaxA::tFunctionMinMaxA(tBool sMax) : tFunction(), m_Max(sMax) {}

	tStackElem tFunctionMinMaxA::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tAverageAArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return MakeWholeOrDouble(0.0);
		}
		tDouble wExt = wNumbers[0];
		for (tSize i = 1; i < wNumbers.size(); ++i) {
			if (m_Max) {
				if (wNumbers[i] > wExt) wExt = wNumbers[i];
			} else if (wNumbers[i] < wExt) {
				wExt = wNumbers[i];
			}
		}
		return MakeWholeOrDouble(wExt);
	}

	tFunctionSpillKind tFunctionMinMaxA::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// CallBack for function Min ============================================
	tCallBackRangeMin::tCallBackRangeMin(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange), m_FirstCall(true){}

	tBool tCallBackRangeMin::CallBack(tAllocatorRef sAllocatorRef) {
		tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell == nullptr) {
			return(true);
		}
		const tVariant& wVal = wCell->CalculableValue();
		if (m_FirstCall) {
			m_Value = wVal;
			m_FirstCall = false;
		} else if ((wVal.Type() != tVariantType::t_null) && (wVal < m_Value)) {
			m_Value = wVal;
     	}
		return(true); // false for Stop
	};

	// Function =============================================================
	tFunctionMin::tFunctionMin() : tFunction() {}

	tStackElem tFunctionMin::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		tVariant wValue;
		tBool wFirst = true;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			if (wArg.Type() == tStackType::t_Range) {
				tRange* wRange = wArg.Range();
				if (wRange != nullptr) {
					tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
					tCallBackRangeMin wCallBackMin(wColRowCellRange);
					wColRowCellRange->VisitorRange(wRange, &wCallBackMin);
					if (wFirst) { wValue = wCallBackMin.Value(); wFirst = false; }
					else { tVariant wMinVal = wCallBackMin.Value(); if ((wMinVal.Type()!=tVariantType::t_null) && (wMinVal < wValue)) wValue = wMinVal; }
				}
			} else if (wArg.Type() == tStackType::t_Array) {
				tArrayValue* wArray = wArg.Array();
				if (wArray == nullptr) continue;
				for (tIndex r = 0; r < wArray->m_Rows; ++r) {
					for (tIndex c = 0; c < wArray->m_Cols; ++c) {
						const tVariant& wArgVal = wArray->At(r, c);
						if (wArgVal.IsError()) continue;
						if (wFirst) { wValue = wArgVal; wFirst = false; }
						else if ((wArgVal.Type()!=tVariantType::t_null) && (wArgVal < wValue)) wValue = wArgVal;
					}
				}
			} else {
				tVariant wArgVal;
				if (StackElemToVariant(wArg, wArgVal) && !wArgVal.IsError()) {
					if (wFirst) { wValue = wArgVal; wFirst = false; }
					else if ((wArgVal.Type()!=tVariantType::t_null) && (wArgVal < wValue)) wValue = wArgVal;
				}
			}
		}
		return(tStackElem(wValue));
	};


	// CallBack for function Max ============================================
	tCallBackRangeMax::tCallBackRangeMax(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange), m_FirstCall(true) {}

	tBool tCallBackRangeMax::CallBack(tAllocatorRef sAllocatorRef) {
		tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
		if (wCell == nullptr) {
			return(true);
		}
		const tVariant& wVal = wCell->CalculableValue();
		if (m_FirstCall) {
			m_Value = wVal;
			m_FirstCall = false;
		} else if ((wVal.Type() != tVariantType::t_null) && (wVal > m_Value)) {
			m_Value = wVal;
		}
		return(true); // false for Stop
	};

	// Function Max  =========================================================
	tFunctionMax::tFunctionMax() : tFunction() {}

	tStackElem tFunctionMax::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		tVariant wValue;
		tBool wFirst = true;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			if (wArg.Type() == tStackType::t_Range) {
				tRange* wRange = wArg.Range();
				if (wRange != nullptr) {
					tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
					tCallBackRangeMax wCallBackMax(wColRowCellRange);
					wColRowCellRange->VisitorRange(wRange, &wCallBackMax);
					if (wFirst) { wValue = wCallBackMax.Value(); wFirst = false; }
					else { tVariant wMaxVal = wCallBackMax.Value(); if ((wMaxVal.Type()!=tVariantType::t_null) && (wMaxVal > wValue)) wValue = wMaxVal; }
				}
			} else if (wArg.Type() == tStackType::t_Array) {
				tArrayValue* wArray = wArg.Array();
				if (wArray == nullptr) continue;
				for (tIndex r = 0; r < wArray->m_Rows; ++r) {
					for (tIndex c = 0; c < wArray->m_Cols; ++c) {
						const tVariant& wArgVal = wArray->At(r, c);
						if (wArgVal.IsError()) continue;
						if (wFirst) { wValue = wArgVal; wFirst = false; }
						else if ((wArgVal.Type()!=tVariantType::t_null) && (wArgVal > wValue)) wValue = wArgVal;
					}
				}
			} else {
				tVariant wArgVal;
				if (StackElemToVariant(wArg, wArgVal) && !wArgVal.IsError()) {
					if (wFirst) { wValue = wArgVal; wFirst = false; }
					else if ((wArgVal.Type()!=tVariantType::t_null) && (wArgVal > wValue)) wValue = wArgVal;
				}
			}
		}
		return(tStackElem(wValue));
	};

    // Function Round  =========================================================
    tFunctionRound::tFunctionRound() : tFunction() {}

    tStackElem tFunctionRound::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if ((wArgs.size()==0) || (wArgs.size()>2)) {
            // Clean up before returning error
            wValue.SetError(tClassError(tTypeError::t_arg,""));
            return(tStackElem(wValue));
        }
        // Arguments: value [, decimals] - in RPN: decimals value ROUND
        // After PopArgs: wArgs[0]=decimals (if present), wArgs[1]=value
        tDouble wCalculatedValue = 0.0;
        tInt wNbDecimals = 0; // default: truncate to integer
        
        if (wArgs.size() == 2) {
            tVariant wDecimalsValue;
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wDecimalsValue) || !StackElemToVariant(wArgs[1], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wDecimalsValue.IsError()) return(tStackElem(wDecimalsValue));
            if (wValueArg.IsError()) return(tStackElem(wValueArg));
            if (wDecimalsValue.IsInt()) {
                wNbDecimals = wDecimalsValue.Int();
            } else if (wDecimalsValue.IsDouble()) {
                wNbDecimals = static_cast<tInt>(wDecimalsValue.Double());
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
        } else {
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wValueArg.IsError()) return(tStackElem(wValueArg));
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
        }
        
        // Clean up

        // Compute rounding with support for negative decimals and negative values
        if (wNbDecimals >= 0) {
            tDouble wFactor = std::pow(10.0, (tDouble)wNbDecimals);
            tDouble wScaled = wCalculatedValue * wFactor;
            tDouble wRounded = std::round(wScaled);
            wValue = wRounded / wFactor;
        } else {
            tDouble wFactor = std::pow(10.0, (tDouble)(-wNbDecimals));
            tDouble wScaled = wCalculatedValue / wFactor;
            tDouble wRounded = std::round(wScaled);
            wValue = wRounded * wFactor;
        }
        return (wValue);
    };
    

    // Function Trunc  =========================================================
    tFunctionTrunc::tFunctionTrunc() : tFunction() {}

    tStackElem tFunctionTrunc::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if ((wArgs.size()==0) || (wArgs.size()>2)) {
            wValue.SetError(tClassError(tTypeError::t_arg,""));
            return(tStackElem(wValue));
        }
        // Arguments: value [, decimals] - in RPN: decimals value TRUNC
        // After PopArgs: wArgs[0]=decimals (if present), wArgs[1]=value
        tDouble wCalculatedValue = 0.0;
        tInt wNbDecimals = 0; // default: truncate to integer
        
        if (wArgs.size() == 2) {
            tVariant wDecimalsValue;
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wDecimalsValue) || !StackElemToVariant(wArgs[1], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wDecimalsValue.IsError()) return(tStackElem(wDecimalsValue));
            if (wValueArg.IsError()) return(tStackElem(wValueArg));
            if (wDecimalsValue.IsInt()) {
                wNbDecimals = wDecimalsValue.Int();
            } else if (wDecimalsValue.IsDouble()) {
                wNbDecimals = static_cast<tInt>(wDecimalsValue.Double());
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
        } else {
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
            if (wValueArg.IsError()) return(tStackElem(wValueArg));
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
                return(tStackElem(wValue));
            }
        }
        
        // Clean up

        // Compute truncation with support for negative decimals and negatives values
        return RoundTowardZero(wCalculatedValue, wNbDecimals);
    };

    // Function RoundDown ======================================================
    tFunctionRoundDown::tFunctionRoundDown() : tFunction() {}

    tStackElem tFunctionRoundDown::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if ((wArgs.size() == 0) || (wArgs.size() > 2)) {
            wValue.SetError(tClassError(tTypeError::t_arg, ""));
            return tStackElem(wValue);
        }
        tDouble wCalculatedValue = 0.0;
        tInt wNbDecimals = 0;
        if (wArgs.size() == 2) {
            tVariant wDecimalsValue;
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wDecimalsValue) || !StackElemToVariant(wArgs[1], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wDecimalsValue.IsError()) return tStackElem(wDecimalsValue);
            if (wValueArg.IsError()) return tStackElem(wValueArg);
            if (wDecimalsValue.IsInt()) {
                wNbDecimals = wDecimalsValue.Int();
            } else if (wDecimalsValue.IsDouble()) {
                wNbDecimals = static_cast<tInt>(wDecimalsValue.Double());
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
        } else {
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wValueArg.IsError()) return tStackElem(wValueArg);
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
        }
        return RoundTowardZero(wCalculatedValue, wNbDecimals);
    }

    // Function RoundUp ========================================================
    tFunctionRoundUp::tFunctionRoundUp() : tFunction() {}

    tStackElem tFunctionRoundUp::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if ((wArgs.size() == 0) || (wArgs.size() > 2)) {
            wValue.SetError(tClassError(tTypeError::t_arg, ""));
            return tStackElem(wValue);
        }
        tDouble wCalculatedValue = 0.0;
        tInt wNbDecimals = 0;
        if (wArgs.size() == 2) {
            tVariant wDecimalsValue;
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wDecimalsValue) || !StackElemToVariant(wArgs[1], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wDecimalsValue.IsError()) return tStackElem(wDecimalsValue);
            if (wValueArg.IsError()) return tStackElem(wValueArg);
            if (wDecimalsValue.IsInt()) {
                wNbDecimals = wDecimalsValue.Int();
            } else if (wDecimalsValue.IsDouble()) {
                wNbDecimals = static_cast<tInt>(wDecimalsValue.Double());
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
        } else {
            tVariant wValueArg;
            if (!StackElemToVariant(wArgs[0], wValueArg)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
            if (wValueArg.IsError()) return tStackElem(wValueArg);
            if (wValueArg.IsInt()) {
                wCalculatedValue = static_cast<tDouble>(wValueArg.Int());
            } else if (wValueArg.IsDouble()) {
                wCalculatedValue = wValueArg.Double();
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return tStackElem(wValue);
            }
        }
        return RoundAwayFromZero(wCalculatedValue, wNbDecimals);
    }

    // CEILING.MATH / FLOOR.MATH / legacy CEILING / FLOOR =====================
    tFunctionCeilingFloorMath::tFunctionCeilingFloorMath(tCeilingFloorKind sKind)
        : tFunction(), m_Kind(sKind) {}

    tStackElem tFunctionCeilingFloorMath::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        const tBool wLegacy =
            (m_Kind == tCeilingFloorKind::CeilingLegacy ||
             m_Kind == tCeilingFloorKind::FloorLegacy);
        const tBool wPrecise =
            (m_Kind == tCeilingFloorKind::CeilingPrecise ||
             m_Kind == tCeilingFloorKind::FloorPrecise);
        // MATH: number, [significance=1], [mode=0]
        // PRECISE / ISO.CEILING: number, [significance=1]
        // Legacy: number, significance (both required).
        if (wLegacy) {
            if (wArgs.size() != 2) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
        } else if (wPrecise) {
            if (wArgs.size() < 1 || wArgs.size() > 2) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
        } else if (wArgs.size() < 1 || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wNumVar;
        if (!StackElemToVariant(wArgs[0], wNumVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wNumVar.IsError()) return(tStackElem(wNumVar));
        if (!wNumVar.IsNumeric()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tDouble wNumber = wNumVar.Numeric();

        tDouble wSignificance = 1.0;
        if (wArgs.size() >= 2) {
            tVariant wSigVar;
            if (!StackElemToVariant(wArgs[1], wSigVar)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (wSigVar.IsError()) return(tStackElem(wSigVar));
            if (!wSigVar.IsNumeric()) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wSignificance = wSigVar.Numeric();
        }
        if (wSignificance == 0.0) {
            // Legacy / PRECISE → #DIV/0!; MATH variants → #NUM!.
            const tBool wDiv0 = wLegacy || wPrecise;
            return(tStackElem(tVariant(tClassError(
                wDiv0 ? tTypeError::t_div0 : tTypeError::t_num, ""))));
        }

        if (wLegacy) {
            // Different signs → #NUM! (Excel compatibility CEILING/FLOOR).
            if ((wNumber > 0.0 && wSignificance < 0.0) ||
                (wNumber < 0.0 && wSignificance > 0.0)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
            }
            const tDouble wQuotient = wNumber / wSignificance;
            const tDouble wMult = (m_Kind == tCeilingFloorKind::CeilingLegacy)
                ? std::ceil(wQuotient)
                : std::floor(wQuotient);
            return MakeWholeOrDouble(wMult * wSignificance);
        }

        // MATH / PRECISE / ISO.CEILING use the absolute value of significance.
        wSignificance = std::fabs(wSignificance);
        const tDouble wQuotient = wNumber / wSignificance;

        if (wPrecise) {
            const tDouble wMult = (m_Kind == tCeilingFloorKind::CeilingPrecise)
                ? std::ceil(wQuotient)   // always toward +∞
                : std::floor(wQuotient); // always toward −∞
            return MakeWholeOrDouble(wMult * wSignificance);
        }

        tInt wMode = 0;
        if (wArgs.size() >= 3) {
            if (!StackElemToInt(wArgs[2], wMode)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        tDouble wMult = 0.0;
        if (m_Kind == tCeilingFloorKind::CeilingMath) {
            // Positive: toward +inf. Negative mode=0: toward +inf; mode!=0: away from zero.
            if (wNumber >= 0.0 || wMode == 0) {
                wMult = std::ceil(wQuotient);
            } else {
                wMult = std::floor(wQuotient);
            }
        } else {
            // Floor: positive toward -inf. Negative mode=0: toward zero; mode!=0: away from zero.
            if (wNumber >= 0.0) {
                wMult = std::floor(wQuotient);
            } else if (wMode == 0) {
                wMult = std::ceil(wQuotient);
            } else {
                wMult = std::floor(wQuotient);
            }
        }
        return MakeWholeOrDouble(wMult * wSignificance);
    }

    // MROUND =================================================================
    tFunctionMRound::tFunctionMRound() : tFunction() {}

    tStackElem tFunctionMRound::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wNumVar;
        tVariant wMultVar;
        if (!StackElemToVariant(wArgs[0], wNumVar) || !StackElemToVariant(wArgs[1], wMultVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wNumVar.IsError()) return(tStackElem(wNumVar));
        if (wMultVar.IsError()) return(tStackElem(wMultVar));
        if (!wNumVar.IsNumeric() || !wMultVar.IsNumeric()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tDouble wNumber = wNumVar.Numeric();
        const tDouble wMultiple = wMultVar.Numeric();
        if (wMultiple == 0.0) {
            return MakeWholeOrDouble(0.0);
        }
        // Same-sign rule (Excel MROUND); zero number is allowed with any multiple.
        if (wNumber != 0.0 &&
            ((wNumber > 0.0 && wMultiple < 0.0) || (wNumber < 0.0 && wMultiple > 0.0))) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        // std::round: halfway cases away from zero (Excel MROUND).
        return MakeWholeOrDouble(std::round(wNumber / wMultiple) * wMultiple);
    }

    // GCD / LCM ==============================================================
    namespace {
        tLong LongGcd(tLong sA, tLong sB) {
            sA = (sA < 0) ? -sA : sA;
            sB = (sB < 0) ? -sB : sB;
            while (sB != 0) {
                const tLong wT = sA % sB;
                sA = sB;
                sB = wT;
            }
            return sA;
        }

        tBool SafeLcm(tLong sA, tLong sB, tLong& oOut) {
            sA = (sA < 0) ? -sA : sA;
            sB = (sB < 0) ? -sB : sB;
            if (sA == 0 || sB == 0) {
                oOut = 0;
                return true;
            }
            const tLong wG = LongGcd(sA, sB);
            const tLong wAg = sA / wG;
            if (wAg > 0 && sB > 0 && wAg > (std::numeric_limits<tLong>::max() / sB)) {
                return false;
            }
            oOut = wAg * sB;
            return true;
        }

        tBool TruncDoubleToLong(tDouble sValue, tLong& oOut) {
            if (!std::isfinite(sValue)) {
                return false;
            }
            const tDouble wT = std::trunc(sValue);
            if (wT < static_cast<tDouble>(std::numeric_limits<tLong>::min()) ||
                wT > static_cast<tDouble>(std::numeric_limits<tLong>::max())) {
                return false;
            }
            oOut = static_cast<tLong>(wT);
            return true;
        }
    } // namespace

    tFunctionGcdLcm::tFunctionGcdLcm(tBool sLcm) : tFunction(), m_Lcm(sLcm) {}

    tStackElem tFunctionGcdLcm::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        std::vector<tDouble> wNumbers;
        tVariant wError;
        if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
            return(tStackElem(wError));
        }
        if (wNumbers.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        std::vector<tLong> wInts;
        wInts.reserve(wNumbers.size());
        for (tDouble wN : wNumbers) {
            tLong wI = 0;
            if (!TruncDoubleToLong(wN, wI)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
            }
            wInts.push_back(wI);
        }

        tLong wAcc = wInts[0];
        if (m_Lcm) {
            for (size_t i = 1; i < wInts.size(); ++i) {
                tLong wNext = 0;
                if (!SafeLcm(wAcc, wInts[i], wNext)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
                }
                wAcc = wNext;
            }
        } else {
            for (size_t i = 1; i < wInts.size(); ++i) {
                wAcc = LongGcd(wAcc, wInts[i]);
            }
            if (wAcc < 0) wAcc = -wAcc;
        }
        return MakeWholeOrDouble(static_cast<tDouble>(wAcc));
    }

    tFunctionSpillKind tFunctionGcdLcm::SpillKind() const {
        return(tFunctionSpillKind::Aggregate);
    }

    // Function Int  =========================================================
    tFunctionInt::tFunctionInt() : tFunction() {}

    tStackElem tFunctionInt::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size() != 1) {
            wValue.SetError(tClassError(tTypeError::t_arg, "INT requires 1 argument"));
            return(tStackElem(wValue));
        }
        tVariant wArgValue;
        if (!StackElemToVariant(wArgs[0], wArgValue)) {
            wValue.SetError(tClassError(tTypeError::t_arg, ""));
            return(tStackElem(wValue));
        }
        if (wArgValue.IsError()) return(tStackElem(wArgValue));
        // Excel: blank cell → 0.
        if (wArgValue.IsNull()) return(tStackElem(tVariant(0)));
        // INT already returns integers unchanged.
        if (wArgValue.IsInt()) return(tStackElem(wArgValue));
        if (!wArgValue.IsDouble()) {
            wValue.SetError(tClassError(tTypeError::t_arg, "INT: argument must be numeric"));
            return(tStackElem(wValue));
        }
        // Excel INT rounds toward negative infinity (floor).
        tDouble wFloored = std::floor(wArgValue.Double());
        // Preserve precision for magnitudes beyond 32-bit tInt (e.g. trillions) by keeping a double.
        if (wFloored >= static_cast<tDouble>(std::numeric_limits<tInt>::min()) &&
            wFloored <= static_cast<tDouble>(std::numeric_limits<tInt>::max())) {
            wValue.SetInt(static_cast<tInt>(wFloored));
        } else {
            wValue.SetDouble(wFloored);
        }
        return(tStackElem(wValue));
    }

    // Function Abs  =========================================================
    tFunctionAbs::tFunctionAbs() : tFunction() {}

    tStackElem tFunctionAbs::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size()!=1) {
            wValue.SetError(tClassError(tTypeError::t_arg,""));
            return(tStackElem(wValue));
        }
        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tStackElem& wArg = *it;
            // Variant or Range
            if (StackElemToVariant(wArg, wValue)) {
                switch (wValue.Type()) {
                    case tVariantType::t_null : {
                        // Excel: blank cell → 0.
                        wValue.SetInt(0);
                        break;
                    }
                    case tVariantType::t_int : {
                        if (wValue.Int()<0) wValue.SetInt(wValue.Int()*-1);
                        break;
                    }
                    case tVariantType::t_double : {
                        if (wValue.Double()<0) wValue.SetDouble(wValue.Double()*-1);
                        break;
                    }
                    default: {
                        wValue.SetError(tClassError(tTypeError::t_arg,""));
                        break;
                    }
                }
            } else {
                wValue.SetError(tClassError(tTypeError::t_arg,""));
            }
            // Clean up handled automatically (no delete needed)
        }
        return(tStackElem(wValue));
    };

    // Unary math (SIN/COS/…/SIGN/SQRTPI) =====================================
    tFunctionUnaryMath::tFunctionUnaryMath(tUnaryMathKind sKind) : tFunction(), m_Kind(sKind) {}

    tStackElem tFunctionUnaryMath::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "unary math requires exactly 1 argument"))));
        }
        tVariant wVal;
        if (!StackElemToVariant(wArgs[0], wVal)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wVal.IsError()) {
            return(tStackElem(wVal));
        }
        // Excel: blank cell → 0 for SIN/COS/… (empty string stays #VALUE!/arg).
        if (wVal.IsNull()) {
            wVal.SetDouble(0.0);
        }
        if (!wVal.IsInt() && !wVal.IsDouble()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        const tDouble wX = wVal.IsDouble() ? wVal.Double() : static_cast<tDouble>(wVal.Int());
        tDouble wY = 0.0;
        switch (m_Kind) {
            case tUnaryMathKind::Cos:     wY = std::cos(wX); break;
            case tUnaryMathKind::Sin:     wY = std::sin(wX); break;
            case tUnaryMathKind::Tan:     wY = std::tan(wX); break;
            case tUnaryMathKind::Acos:    wY = std::acos(wX); break;
            case tUnaryMathKind::Asin:    wY = std::asin(wX); break;
            case tUnaryMathKind::Atan:    wY = std::atan(wX); break;
            case tUnaryMathKind::Sqrt:    wY = std::sqrt(wX); break;
            case tUnaryMathKind::Log:     wY = std::log(wX); break;
            case tUnaryMathKind::Log10:   wY = std::log10(wX); break;
            case tUnaryMathKind::Exp:     wY = std::exp(wX); break;
            case tUnaryMathKind::Ln:      wY = std::log(wX); break;
            case tUnaryMathKind::Radians: wY = wX * M_PI / 180.0; break;
            case tUnaryMathKind::Degrees: wY = wX * 180.0 / M_PI; break;
            case tUnaryMathKind::Cosh:    wY = std::cosh(wX); break;
            case tUnaryMathKind::Sinh:    wY = std::sinh(wX); break;
            case tUnaryMathKind::Tanh:    wY = std::tanh(wX); break;
            case tUnaryMathKind::Acosh:   wY = std::acosh(wX); break;
            case tUnaryMathKind::Asinh:   wY = std::asinh(wX); break;
            case tUnaryMathKind::Atanh:   wY = std::atanh(wX); break;
            case tUnaryMathKind::Cot:     wY = std::cos(wX) / std::sin(wX); break;
            case tUnaryMathKind::Coth:    wY = std::cosh(wX) / std::sinh(wX); break;
            case tUnaryMathKind::Csc:     wY = 1.0 / std::sin(wX); break;
            case tUnaryMathKind::Csch:    wY = 1.0 / std::sinh(wX); break;
            case tUnaryMathKind::Sec:     wY = 1.0 / std::cos(wX); break;
            case tUnaryMathKind::Sech:    wY = 1.0 / std::cosh(wX); break;
            // Excel ACOT: result in (0, π]; atan2(1, x) matches that range.
            case tUnaryMathKind::Acot:    wY = std::atan2(1.0, wX); break;
            case tUnaryMathKind::Acoth:   wY = std::atanh(1.0 / wX); break;
            case tUnaryMathKind::Sign:
                wY = (wX > 0.0) ? 1.0 : ((wX < 0.0) ? -1.0 : 0.0);
                break;
            case tUnaryMathKind::SqrtPi:  wY = std::sqrt(wX * M_PI); break;
        }
        return(tStackElem(tVariant(wY)));
    };

    // Function Pi  ==========================================================
    tFunctionPi::tFunctionPi() : tFunction() {}
    
    tStackElem tFunctionPi::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        wValue.SetDouble(M_PI);
        return(tStackElem(wValue));
    };

    // ATAN2 / QUOTIENT =====================================================
    tFunctionBinaryMath::tFunctionBinaryMath(tBinaryMathKind sKind) : tFunction(), m_Kind(sKind) {}

    tStackElem tFunctionBinaryMath::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "binary math requires 2 arguments"))));
        }
        // PopArgs: [arg2, arg1]. ATAN2(x, y) → atan2(y, x). QUOTIENT(num, den) → trunc(num/den).
        tVariant wA;
        tVariant wB;
        if (!StackElemToVariant(wArgs[1], wA) || !StackElemToVariant(wArgs[0], wB)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wA.IsError()) return(tStackElem(wA));
        if (wB.IsError()) return(tStackElem(wB));
        // Excel: blank cell → 0 for ATAN2 / QUOTIENT.
        if (wA.IsNull()) wA.SetDouble(0.0);
        if (wB.IsNull()) wB.SetDouble(0.0);
        if ((!wA.IsInt() && !wA.IsDouble()) || (!wB.IsInt() && !wB.IsDouble())) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tDouble wX = wA.IsDouble() ? wA.Double() : static_cast<tDouble>(wA.Int());
        const tDouble wY = wB.IsDouble() ? wB.Double() : static_cast<tDouble>(wB.Int());
        if (m_Kind == tBinaryMathKind::Atan2) {
            if (wX == 0.0 && wY == 0.0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
            }
            return(tStackElem(tVariant(std::atan2(wY, wX))));
        }
        // Quotient
        if (wY == 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
        }
        return(tStackElem(tVariant(std::trunc(wX / wY))));
    };

    // FACT / FACTDOUBLE =====================================================
    tFunctionFact::tFunctionFact(tBool sDouble) : tFunction(), m_Double(sDouble) {}

    tStackElem tFunctionFact::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FACT/FACTDOUBLE requires exactly 1 argument"))));
        }
        tVariant wVal;
        if (!StackElemToVariant(wArgs[0], wVal)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wVal.IsError()) return(tStackElem(wVal));
        if (!wVal.IsInt() && !wVal.IsDouble()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tDouble wX = wVal.IsDouble() ? wVal.Double() : static_cast<tDouble>(wVal.Int());
        if (!std::isfinite(wX)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        const tInt wN = static_cast<tInt>(std::trunc(wX));

        if (!m_Double) {
            if (wN < 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
            }
            // Excel FACT(170) is finite; FACT(171) overflows to #NUM!.
            if (wN > 170) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
            }
            tDouble wFact = 1.0;
            for (tInt i = 2; i <= wN; ++i) {
                wFact *= static_cast<tDouble>(i);
            }
            return MakeWholeOrDouble(wFact);
        }

        // FACTDOUBLE: n!! ; negative even → #NUM!; negative odd allowed.
        // Excel: (-1)!!=1, (-3)!!=-1, (-5)!!=3, (-7)!!=-15
        // i.e. for n=-(2k+1): (-1)^k * (2k-1)!! with (-1)!!:=1.
        if (wN < 0 && ((wN % 2) == 0)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tDouble wFact = 1.0;
        if (wN > 0) {
            for (tInt i = wN; i >= 2; i -= 2) {
                wFact *= static_cast<tDouble>(i);
            }
        } else if (wN < 0) {
            const tInt wK = (-wN - 1) / 2;
            if (wK >= 1) {
                for (tInt i = 2 * wK - 1; i >= 2; i -= 2) {
                    wFact *= static_cast<tDouble>(i);
                }
            }
            if ((wK % 2) != 0) {
                wFact = -wFact;
            }
        }
        if (!std::isfinite(wFact)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        return MakeWholeOrDouble(wFact);
    };

    // EVEN / ODD =============================================================
    tFunctionEvenOdd::tFunctionEvenOdd(tBool sEven) : tFunction(), m_Even(sEven) {}

    tStackElem tFunctionEvenOdd::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wVal;
        if (!StackElemToVariant(wArgs[0], wVal)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wVal.IsError()) return(tStackElem(wVal));
        if (!wVal.IsNumeric()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tDouble wX = wVal.Numeric();
        if (!std::isfinite(wX)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        // Round away from zero to an integer, then adjust parity.
        tDouble wAway = (wX >= 0.0) ? std::ceil(wX) : std::floor(wX);
        tLong wI = static_cast<tLong>(wAway);
        const tBool wIsEven = ((wI % 2) == 0);
        if (m_Even) {
            if (!wIsEven) wI += (wX >= 0.0) ? 1 : -1;
        } else {
            // ODD(0) → 1
            if (wIsEven) wI += (wX >= 0.0) ? 1 : -1;
        }
        return MakeWholeOrDouble(static_cast<tDouble>(wI));
    }

    // COMBIN / COMBINA =======================================================
    tFunctionCombin::tFunctionCombin(tBool sWithRep) : tFunction(), m_WithRep(sWithRep) {}

    tStackElem tFunctionCombin::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COMBIN/COMBINA requires 2 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());
        tVariant wNVar, wKVar;
        if (!StackElemToVariant(wArgs[0], wNVar) || !StackElemToVariant(wArgs[1], wKVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wNVar.IsError()) return(tStackElem(wNVar));
        if (wKVar.IsError()) return(tStackElem(wKVar));
        if (!wNVar.IsNumeric() || !wKVar.IsNumeric()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tInt wN = static_cast<tInt>(std::trunc(wNVar.Numeric()));
        const tInt wK = static_cast<tInt>(std::trunc(wKVar.Numeric()));
        if (wN < 0 || wK < 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        // COMBINA(n,k) = C(n+k-1, k); COMBINA(0,0)=1; COMBINA(0,k>0)=0.
        tInt wTotal = wN;
        tInt wChoose = wK;
        if (m_WithRep) {
            if (wN == 0) {
                return MakeWholeOrDouble((wK == 0) ? 1.0 : 0.0);
            }
            wTotal = wN + wK - 1;
            wChoose = wK;
        } else if (wK > wN) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        // Multiplicative formula; use the smaller of k and n-k.
        tInt wR = wChoose;
        if (wR > wTotal - wR) wR = wTotal - wR;
        tDouble wC = 1.0;
        for (tInt i = 1; i <= wR; ++i) {
            wC = wC * static_cast<tDouble>(wTotal - wR + i) / static_cast<tDouble>(i);
        }
        if (!std::isfinite(wC)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        return MakeWholeOrDouble(std::round(wC));
    }

	namespace {
		static const char kBaseDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

		static tInt DigitValue(char sCh) {
			if (sCh >= '0' && sCh <= '9') return(sCh - '0');
			if (sCh >= 'A' && sCh <= 'Z') return(10 + (sCh - 'A'));
			if (sCh >= 'a' && sCh <= 'z') return(10 + (sCh - 'a'));
			return(-1);
		}

		// Excel engineering conversions: fixed 10-digit two's-complement widths.
		static tInt EngBitWidth(tInt sRadix) {
			if (sRadix == 2) return(10);
			if (sRadix == 8) return(30);
			if (sRadix == 16) return(40);
			return(0);
		}

		static tInt EngMaxDigits(tInt /*sRadix*/) { return(10); }

		static tBool EngRangeForRadix(tInt sRadix, long long sValue) {
			const tInt wBits = EngBitWidth(sRadix);
			if (wBits <= 0) return(false);
			const long long wHalf = 1LL << (wBits - 1);
			return(sValue >= -wHalf && sValue < wHalf);
		}

		// Parse bin/oct/hex text (≤10 digits). Full-width strings use two's complement.
		static tBool EngParseSigned(const tString& sText, tInt sRadix, long long& oOut) {
			const tInt wBits = EngBitWidth(sRadix);
			const tInt wMaxDigits = EngMaxDigits(sRadix);
			if (wBits <= 0) return(false);
			tSize wBegin = 0;
			while (wBegin < sText.size() && (sText[wBegin] == ' ' || sText[wBegin] == '\t')) ++wBegin;
			tSize wEnd = sText.size();
			while (wEnd > wBegin && (sText[wEnd - 1] == ' ' || sText[wEnd - 1] == '\t')) --wEnd;
			if (wBegin >= wEnd) return(false);
			const tSize wLen = wEnd - wBegin;
			if (static_cast<tInt>(wLen) > wMaxDigits) return(false);
			unsigned long long wUnsigned = 0;
			for (tSize i = wBegin; i < wEnd; ++i) {
				const tInt wD = DigitValue(sText[i]);
				if (wD < 0 || wD >= sRadix) return(false);
				wUnsigned = wUnsigned * static_cast<unsigned long long>(sRadix) +
							static_cast<unsigned long long>(wD);
			}
			if (static_cast<tInt>(wLen) == wMaxDigits) {
				const unsigned long long wMod = 1ULL << wBits;
				const unsigned long long wSign = 1ULL << (wBits - 1);
				if (wUnsigned >= wSign) {
					oOut = static_cast<long long>(wUnsigned - wMod);
					return(true);
				}
			}
			oOut = static_cast<long long>(wUnsigned);
			return(true);
		}

		// Format signed value to bin/oct/hex. Negatives always use full digit width.
		static tBool EngFormatSigned(long long sValue, tInt sRadix, tInt sPlaces, tString& oOut) {
			const tInt wBits = EngBitWidth(sRadix);
			const tInt wMaxDigits = EngMaxDigits(sRadix);
			if (wBits <= 0 || !EngRangeForRadix(sRadix, sValue)) return(false);
			unsigned long long wU;
			tInt wWidth;
			if (sValue < 0) {
				wU = static_cast<unsigned long long>(sValue + (1LL << wBits));
				wWidth = wMaxDigits;
			} else {
				wU = static_cast<unsigned long long>(sValue);
				if (sPlaces > 0) {
					if (sPlaces > wMaxDigits) return(false);
					wWidth = sPlaces;
				} else {
					wWidth = 1;
				}
			}
			tString wDigits;
			if (wU == 0) {
				wDigits = "0";
			} else {
				unsigned long long wN = wU;
				while (wN > 0) {
					wDigits.push_back(kBaseDigits[static_cast<tSize>(wN % static_cast<unsigned long long>(sRadix))]);
					wN /= static_cast<unsigned long long>(sRadix);
				}
				std::reverse(wDigits.begin(), wDigits.end());
			}
			if (static_cast<tInt>(wDigits.size()) > wMaxDigits) return(false);
			if (sValue >= 0 && sPlaces > 0 && static_cast<tInt>(wDigits.size()) > sPlaces) {
				return(false);
			}
			if (static_cast<tInt>(wDigits.size()) < wWidth) {
				wDigits.insert(wDigits.begin(),
							   static_cast<tSize>(wWidth - static_cast<tInt>(wDigits.size())), '0');
			}
			oOut = wDigits;
			return(true);
		}

		// Excel BIT*: non-negative integers ≤ 2^48 − 1.
		static tBool CoerceBitInt(const tVariant& sVar, unsigned long long& oOut) {
			if (!sVar.IsNumeric()) return(false);
			const tDouble wX = sVar.Numeric();
			if (!std::isfinite(wX) || wX < 0.0) return(false);
			const tDouble wTrunc = std::trunc(wX);
			if (wTrunc != wX) return(false);
			const tDouble wMax = static_cast<tDouble>((1ULL << 48) - 1ULL);
			if (wTrunc > wMax) return(false);
			oOut = static_cast<unsigned long long>(wTrunc);
			return(true);
		}

		// Excel ROMAN form tables (classic → simplified), same as Excel-compatible engines.
		struct tRomanPair { tInt m_Value; const char* m_Symbol; };
		static const tRomanPair kRomanForm0[] = {
			{1000,"M"},{900,"CM"},{500,"D"},{400,"CD"},{100,"C"},{90,"XC"},
			{50,"L"},{40,"XL"},{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
		};
		static const tRomanPair kRomanForm1[] = {
			{1000,"M"},{950,"LM"},{900,"CM"},{500,"D"},{450,"LD"},{400,"CD"},
			{100,"C"},{95,"VC"},{90,"XC"},{50,"L"},{45,"VL"},{40,"XL"},
			{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
		};
		static const tRomanPair kRomanForm2[] = {
			{1000,"M"},{990,"XM"},{950,"LM"},{900,"CM"},{500,"D"},{490,"XD"},
			{450,"LD"},{400,"CD"},{100,"C"},{99,"IC"},{90,"XC"},{50,"L"},
			{45,"VL"},{40,"XL"},{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
		};
		static const tRomanPair kRomanForm3[] = {
			{1000,"M"},{995,"VM"},{990,"XM"},{950,"LM"},{900,"CM"},{500,"D"},
			{495,"VD"},{490,"XD"},{450,"LD"},{400,"CD"},{100,"C"},{99,"IC"},
			{90,"XC"},{50,"L"},{45,"VL"},{40,"XL"},{10,"X"},{9,"IX"},{5,"V"},
			{4,"IV"},{1,"I"}
		};
		static const tRomanPair kRomanForm4[] = {
			{1000,"M"},{999,"IM"},{995,"VM"},{990,"XM"},{950,"LM"},{900,"CM"},
			{500,"D"},{499,"ID"},{495,"VD"},{490,"XD"},{450,"LD"},{400,"CD"},
			{100,"C"},{99,"IC"},{90,"XC"},{50,"L"},{45,"VL"},{40,"XL"},
			{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
		};

		static tString ToRoman(tInt sNumber, tInt sForm) {
			const tRomanPair* wTable = kRomanForm0;
			tSize wCount = sizeof(kRomanForm0) / sizeof(kRomanForm0[0]);
			switch (sForm) {
				case 1: wTable = kRomanForm1; wCount = sizeof(kRomanForm1) / sizeof(kRomanForm1[0]); break;
				case 2: wTable = kRomanForm2; wCount = sizeof(kRomanForm2) / sizeof(kRomanForm2[0]); break;
				case 3: wTable = kRomanForm3; wCount = sizeof(kRomanForm3) / sizeof(kRomanForm3[0]); break;
				case 4: wTable = kRomanForm4; wCount = sizeof(kRomanForm4) / sizeof(kRomanForm4[0]); break;
				default: break;
			}
			tString wOut;
			tInt wN = sNumber;
			for (tSize i = 0; i < wCount; ++i) {
				while (wN >= wTable[i].m_Value) {
					wOut += wTable[i].m_Symbol;
					wN -= wTable[i].m_Value;
				}
			}
			return(wOut);
		}

		static tInt RomanCharValue(char sCh) {
			switch (sCh) {
				case 'I': case 'i': return(1);
				case 'V': case 'v': return(5);
				case 'X': case 'x': return(10);
				case 'L': case 'l': return(50);
				case 'C': case 'c': return(100);
				case 'D': case 'd': return(500);
				case 'M': case 'm': return(1000);
				default: return(-1);
			}
		}

		static tBool ParseArabic(const tString& sText, tInt& oOut) {
			tString wTrim;
			wTrim.reserve(sText.size());
			for (char wCh : sText) {
				if (wCh == ' ' || wCh == '\t') continue;
				wTrim += wCh;
			}
			if (wTrim.empty()) {
				oOut = 0;
				return(true);
			}
			tBool wNeg = false;
			tSize wStart = 0;
			if (wTrim[0] == '-') {
				wNeg = true;
				wStart = 1;
				if (wStart >= wTrim.size()) return(false);
			}
			tInt wSum = 0;
			for (tSize i = wStart; i < wTrim.size(); ++i) {
				const tInt wCur = RomanCharValue(wTrim[i]);
				if (wCur < 0) return(false);
				tInt wNext = 0;
				if (i + 1 < wTrim.size()) {
					wNext = RomanCharValue(wTrim[i + 1]);
					if (wNext < 0) return(false);
				}
				if (i + 1 < wTrim.size() && wCur < wNext) wSum -= wCur;
				else wSum += wCur;
			}
			oOut = wNeg ? -wSum : wSum;
			return(true);
		}
	} // namespace

	// BASE ===================================================================
	tFunctionBase::tFunctionBase() : tFunction() {}

	tStackElem tFunctionBase::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2 && wArgs.size() != 3) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "BASE requires 2 or 3 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wNumVar, wRadixVar;
		if (!StackElemToVariant(wArgs[0], wNumVar) || !StackElemToVariant(wArgs[1], wRadixVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wNumVar.IsError()) return(tStackElem(wNumVar));
		if (wRadixVar.IsError()) return(tStackElem(wRadixVar));
		if (!wNumVar.IsNumeric() || !wRadixVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const long long wNumber = static_cast<long long>(std::trunc(wNumVar.Numeric()));
		const tInt wRadix = static_cast<tInt>(std::trunc(wRadixVar.Numeric()));
		tInt wMinLen = 0;
		if (wArgs.size() == 3) {
			tVariant wMinVar;
			if (!StackElemToVariant(wArgs[2], wMinVar)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wMinVar.IsError()) return(tStackElem(wMinVar));
			if (!wMinVar.IsNumeric()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wMinLen = static_cast<tInt>(std::trunc(wMinVar.Numeric()));
		}
		const long long wMax = 1LL << 53; // Excel: number < 2^53
		if (wNumber < 0 || wNumber >= wMax || wRadix < 2 || wRadix > 36 ||
			wMinLen < 0 || wMinLen > 255) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tString wDigits;
		long long wN = wNumber;
		if (wN == 0) {
			wDigits = "0";
		} else {
			while (wN > 0) {
				wDigits.push_back(kBaseDigits[static_cast<tSize>(wN % wRadix)]);
				wN /= wRadix;
			}
			std::reverse(wDigits.begin(), wDigits.end());
		}
		if (static_cast<tInt>(wDigits.size()) < wMinLen) {
			wDigits.insert(wDigits.begin(), static_cast<tSize>(wMinLen - static_cast<tInt>(wDigits.size())), '0');
		}
		return(tStackElem(tVariant(wDigits)));
	}

	// DECIMAL ================================================================
	tFunctionDecimal::tFunctionDecimal() : tFunction() {}

	tStackElem tFunctionDecimal::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DECIMAL requires 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wTextVar, wRadixVar;
		if (!StackElemToVariant(wArgs[0], wTextVar) || !StackElemToVariant(wArgs[1], wRadixVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wTextVar.IsError()) return(tStackElem(wTextVar));
		if (wRadixVar.IsError()) return(tStackElem(wRadixVar));
		if (!wRadixVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tInt wRadix = static_cast<tInt>(std::trunc(wRadixVar.Numeric()));
		if (wRadix < 2 || wRadix > 36) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tString wText;
		if (wTextVar.IsString()) {
			wText = wTextVar.String();
		} else if (wTextVar.IsNumeric()) {
			// Excel coerces numbers to their decimal text form (truncated).
			const long long wAsInt = static_cast<long long>(std::trunc(wTextVar.Numeric()));
			wText = std::to_string(wAsInt);
		} else {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		// Trim spaces.
		tSize wBegin = 0;
		while (wBegin < wText.size() && (wText[wBegin] == ' ' || wText[wBegin] == '\t')) ++wBegin;
		tSize wEnd = wText.size();
		while (wEnd > wBegin && (wText[wEnd - 1] == ' ' || wText[wEnd - 1] == '\t')) --wEnd;
		if (wBegin >= wEnd) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		tDouble wValue = 0.0;
		for (tSize i = wBegin; i < wEnd; ++i) {
			const tInt wD = DigitValue(wText[i]);
			if (wD < 0 || wD >= wRadix) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wValue = wValue * static_cast<tDouble>(wRadix) + static_cast<tDouble>(wD);
			if (!std::isfinite(wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		return MakeWholeOrDouble(wValue);
	}

	// BITAND / BITOR / BITXOR / BITLSHIFT / BITRSHIFT ========================
	tFunctionBitOp::tFunctionBitOp(tBitOpKind sKind) : tFunction(), m_Kind(sKind) {}

	tStackElem tFunctionBitOp::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "BIT* requires 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wA, wB;
		if (!StackElemToVariant(wArgs[0], wA) || !StackElemToVariant(wArgs[1], wB)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wA.IsError()) return(tStackElem(wA));
		if (wB.IsError()) return(tStackElem(wB));
		if (wA.IsNull()) wA.SetDouble(0.0);
		if (wB.IsNull()) wB.SetDouble(0.0);
		if (!wA.IsNumeric() || !wB.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}

		const tBool wIsShift =
			(m_Kind == tBitOpKind::LShift || m_Kind == tBitOpKind::RShift);
		unsigned long long wNum = 0;
		if (!CoerceBitInt(wA, wNum)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}

		if (wIsShift) {
			const tDouble wShiftD = wB.Numeric();
			if (!std::isfinite(wShiftD) || std::trunc(wShiftD) != wShiftD) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			long long wShift = static_cast<long long>(std::trunc(wShiftD));
			if (wShift > 53 || wShift < -53) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			tBool wLeft = (m_Kind == tBitOpKind::LShift);
			if (wShift < 0) {
				wLeft = !wLeft;
				wShift = -wShift;
			}
			unsigned long long wOut = wNum;
			if (wShift == 0) {
				// keep
			} else if (wLeft) {
				// Result must stay within Excel's 48-bit non-negative domain.
				if (wShift >= 48) {
					if (wNum != 0) {
						return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
					}
					wOut = 0;
				} else if (wNum > (((1ULL << 48) - 1ULL) >> static_cast<unsigned>(wShift))) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
				} else {
					wOut = wNum << static_cast<unsigned>(wShift);
				}
			} else {
				wOut = (wShift >= 64) ? 0ULL : (wNum >> static_cast<unsigned>(wShift));
			}
			return MakeWholeOrDouble(static_cast<tDouble>(wOut));
		}

		unsigned long long wOther = 0;
		if (!CoerceBitInt(wB, wOther)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		unsigned long long wOut = 0;
		switch (m_Kind) {
			case tBitOpKind::And: wOut = wNum & wOther; break;
			case tBitOpKind::Or: wOut = wNum | wOther; break;
			case tBitOpKind::Xor: wOut = wNum ^ wOther; break;
			default: break;
		}
		return MakeWholeOrDouble(static_cast<tDouble>(wOut));
	}

	// DELTA / GESTEP =========================================================
	tFunctionEngStep::tFunctionEngStep(tEngStepKind sKind) : tFunction(), m_Kind(sKind) {}

	tStackElem tFunctionEngStep::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty() || wArgs.size() > 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DELTA/GESTEP requires 1 or 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wN1, wN2;
		if (!StackElemToVariant(wArgs[0], wN1)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wN1.IsError()) return(tStackElem(wN1));
		if (wN1.IsNull()) wN1.SetDouble(0.0);
		if (!wN1.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		tDouble wSecond = 0.0;
		if (wArgs.size() == 2) {
			if (!StackElemToVariant(wArgs[1], wN2)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wN2.IsError()) return(tStackElem(wN2));
			if (wN2.IsNull()) wN2.SetDouble(0.0);
			if (!wN2.IsNumeric()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wSecond = wN2.Numeric();
		}
		const tDouble wFirst = wN1.Numeric();
		tDouble wOut = 0.0;
		if (m_Kind == tEngStepKind::Delta) {
			wOut = (wFirst == wSecond) ? 1.0 : 0.0;
		} else {
			wOut = (wFirst >= wSecond) ? 1.0 : 0.0;
		}
		return MakeWholeOrDouble(wOut);
	}

	// BIN2* / DEC2* / HEX2* / OCT2* ==========================================
	tFunctionEngBaseConvert::tFunctionEngBaseConvert(tInt sFromRadix, tInt sToRadix)
		: tFunction(), m_FromRadix(sFromRadix), m_ToRadix(sToRadix) {}

	tStackElem tFunctionEngBaseConvert::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		const tBool wToDec = (m_ToRadix == 10);
		if (wToDec) {
			if (wArgs.size() != 1) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ENG *2DEC requires 1 argument"))));
			}
		} else if (wArgs.size() != 1 && wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ENG convert requires 1 or 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());

		tVariant wNumberVar;
		if (!StackElemToVariant(wArgs[0], wNumberVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wNumberVar.IsError()) return(tStackElem(wNumberVar));

		tInt wPlaces = 0;
		if (!wToDec && wArgs.size() == 2) {
			tVariant wPlacesVar;
			if (!StackElemToVariant(wArgs[1], wPlacesVar)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wPlacesVar.IsError()) return(tStackElem(wPlacesVar));
			if (wPlacesVar.IsNull()) wPlacesVar.SetDouble(0.0);
			if (!wPlacesVar.IsNumeric()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			const tDouble wP = wPlacesVar.Numeric();
			if (!std::isfinite(wP) || std::trunc(wP) != wP || wP <= 0.0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wPlaces = static_cast<tInt>(std::trunc(wP));
		}

		long long wValue = 0;
		if (m_FromRadix == 10) {
			if (wNumberVar.IsNull()) wNumberVar.SetDouble(0.0);
			if (!wNumberVar.IsNumeric()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			const tDouble wX = wNumberVar.Numeric();
			if (!std::isfinite(wX) || std::trunc(wX) != wX) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			wValue = static_cast<long long>(std::trunc(wX));
			if (!EngRangeForRadix(m_ToRadix, wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		} else {
			tString wText;
			if (wNumberVar.IsString()) {
				wText = wNumberVar.String();
			} else if (wNumberVar.IsNumeric()) {
				const long long wAsInt = static_cast<long long>(std::trunc(wNumberVar.Numeric()));
				if (wAsInt < 0) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
				}
				wText = std::to_string(wAsInt);
			} else if (wNumberVar.IsNull()) {
				wText = "0";
			} else {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (!EngParseSigned(wText, m_FromRadix, wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			if (!wToDec && !EngRangeForRadix(m_ToRadix, wValue)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}

		if (wToDec) {
			return MakeWholeOrDouble(static_cast<tDouble>(wValue));
		}
		tString wOut;
		if (!EngFormatSigned(wValue, m_ToRadix, wPlaces, wOut)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
		}
		return(tStackElem(tVariant(wOut)));
	}

	// ROMAN ==================================================================
	tFunctionRoman::tFunctionRoman() : tFunction() {}

	tStackElem tFunctionRoman::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1 && wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ROMAN requires 1 or 2 arguments"))));
		}
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wNumVar;
		if (!StackElemToVariant(wArgs[0], wNumVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wNumVar.IsError()) return(tStackElem(wNumVar));
		if (!wNumVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		const tInt wNumber = static_cast<tInt>(std::trunc(wNumVar.Numeric()));
		tInt wForm = 0;
		if (wArgs.size() == 2) {
			tVariant wFormVar;
			if (!StackElemToVariant(wArgs[1], wFormVar)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			if (wFormVar.IsError()) return(tStackElem(wFormVar));
			if (!wFormVar.IsNumeric()) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
			}
			wForm = static_cast<tInt>(std::trunc(wFormVar.Numeric()));
		}
		if (wNumber < 0 || wNumber > 3999 || wForm < 0 || wForm > 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		return(tStackElem(tVariant(ToRoman(wNumber, wForm))));
	}

	// ARABIC =================================================================
	tFunctionArabic::tFunctionArabic() : tFunction() {}

	tStackElem tFunctionArabic::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 1) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ARABIC requires 1 argument"))));
		}
		tVariant wTextVar;
		if (!StackElemToVariant(wArgs[0], wTextVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wTextVar.IsError()) return(tStackElem(wTextVar));
		tString wText;
		if (wTextVar.IsString()) {
			wText = wTextVar.String();
		} else if (wTextVar.IsNull()) {
			wText = "";
		} else {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		tInt wOut = 0;
		if (!ParseArabic(wText, wOut)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		return(tStackElem(tVariant(wOut)));
	}

	// MULTINOMIAL ============================================================
	tFunctionMultinomial::tFunctionMultinomial() : tFunction() {}

	tStackElem tFunctionMultinomial::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.empty()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MULTINOMIAL requires at least 1 argument"))));
		}
		std::vector<tDouble> wNumbers;
		tVariant wError;
		if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
			return(tStackElem(wError));
		}
		if (wNumbers.empty()) {
			return MakeWholeOrDouble(1.0);
		}
		// Iterative: result *= rising product over each ni (avoids huge intermediate factorials).
		tDouble wResult = 1.0;
		tInt wSum = 0;
		for (tDouble wV : wNumbers) {
			const tInt wNi = static_cast<tInt>(std::trunc(wV));
			if (wNi < 0) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
			for (tInt j = 1; j <= wNi; ++j) {
				++wSum;
				wResult = wResult * static_cast<tDouble>(wSum) / static_cast<tDouble>(j);
				if (!std::isfinite(wResult)) {
					return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
				}
			}
		}
		return MakeWholeOrDouble(std::round(wResult));
	}

	tFunctionSpillKind tFunctionMultinomial::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

	// SERIESSUM ==============================================================
	tFunctionSeriesSum::tFunctionSeriesSum() : tFunction() {}

	tStackElem tFunctionSeriesSum::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 4) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SERIESSUM requires 4 arguments"))));
		}
		// RPN: coefficients, m, n, x
		std::reverse(wArgs.begin(), wArgs.end());
		tVariant wXVar, wNVar, wMVar;
		if (!StackElemToVariant(wArgs[0], wXVar) || !StackElemToVariant(wArgs[1], wNVar) ||
			!StackElemToVariant(wArgs[2], wMVar)) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		if (wXVar.IsError()) return(tStackElem(wXVar));
		if (wNVar.IsError()) return(tStackElem(wNVar));
		if (wMVar.IsError()) return(tStackElem(wMVar));
		if (!wXVar.IsNumeric() || !wNVar.IsNumeric() || !wMVar.IsNumeric()) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
		}
		std::vector<tDouble> wCoef;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[3], wCoef, wError)) {
			return(tStackElem(wError));
		}
		if (wCoef.empty()) {
			return MakeWholeOrDouble(0.0);
		}
		const tDouble wX = wXVar.Numeric();
		const tDouble wN = wNVar.Numeric();
		const tDouble wM = wMVar.Numeric();
		tDouble wSum = 0.0;
		for (tSize i = 0; i < wCoef.size(); ++i) {
			const tDouble wPow = wN + static_cast<tDouble>(i) * wM;
			wSum += wCoef[i] * std::pow(wX, wPow);
			if (!std::isfinite(wSum)) {
				return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
			}
		}
		return(tStackElem(tVariant(wSum)));
	}

	// PERCENTOF ==============================================================
	tFunctionPercentOf::tFunctionPercentOf() : tFunction() {}

	tStackElem tFunctionPercentOf::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
		if (wArgs.size() != 2) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PERCENTOF requires 2 arguments"))));
		}
		// RPN: data_all, data_subset
		std::vector<tDouble> wSub, wAll;
		tVariant wError;
		if (!tNumericArgCollector::Append(wArgs[1], wSub, wError)) {
			return(tStackElem(wError));
		}
		if (!tNumericArgCollector::Append(wArgs[0], wAll, wError)) {
			return(tStackElem(wError));
		}
		tDouble wSumSub = 0.0, wSumAll = 0.0;
		for (tDouble wV : wSub) wSumSub += wV;
		for (tDouble wV : wAll) wSumAll += wV;
		if (wSumAll == 0.0) {
			return(tStackElem(tVariant(tClassError(tTypeError::t_div0, ""))));
		}
		return(tStackElem(tVariant(wSumSub / wSumAll)));
	}

	tFunctionSpillKind tFunctionPercentOf::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    // SUMSQ ==================================================================
    tFunctionSumSq::tFunctionSumSq() : tFunction() {}

    tStackElem tFunctionSumSq::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        std::vector<tDouble> wNumbers;
        tVariant wError;
        if (!tNumericArgCollector::Collect(wArgs, wNumbers, wError)) {
            return(tStackElem(wError));
        }
        tDouble wSum = 0.0;
        for (tDouble wX : wNumbers) {
            wSum += wX * wX;
        }
        return MakeWholeOrDouble(wSum);
    }

    tFunctionSpillKind tFunctionSumSq::SpillKind() const {
        return(tFunctionSpillKind::Aggregate);
    }

    // SUMX2MY2 / SUMX2PY2 / SUMXMY2 ==========================================
    tFunctionSumX::tFunctionSumX(tSumXKind sKind) : tFunction(), m_Kind(sKind) {}

    tStackElem tFunctionSumX::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUMX* requires 2 arguments"))));
        }
        // RPN: array_y, array_x
        tArrayValue wY;
        tArrayValue wX;
        if (!StackElemToArray(wArgs[0], wY) || !StackElemToArray(wArgs[1], wX) ||
            wX.Count() <= 0 || wY.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wX.m_Rows != wY.m_Rows || wX.m_Cols != wY.m_Cols) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }
        tDouble wSum = 0.0;
        for (tIndex r = 0; r < wX.m_Rows; ++r) {
            for (tIndex c = 0; c < wX.m_Cols; ++c) {
                const tVariant& wXv = wX.At(r, c);
                const tVariant& wYv = wY.At(r, c);
                if (wXv.IsError()) return(tStackElem(wXv));
                if (wYv.IsError()) return(tStackElem(wYv));
                // Excel: skip non-numeric pairs.
                if (!wXv.IsNumeric() || !wYv.IsNumeric()) continue;
                const tDouble wA = wXv.Numeric();
                const tDouble wB = wYv.Numeric();
                switch (m_Kind) {
                    case tSumXKind::X2MinusY2: wSum += (wA * wA) - (wB * wB); break;
                    case tSumXKind::X2PlusY2:  wSum += (wA * wA) + (wB * wB); break;
                    case tSumXKind::XMinusY2: {
                        const tDouble wD = wA - wB;
                        wSum += wD * wD;
                        break;
                    }
                }
            }
        }
        return MakeWholeOrDouble(wSum);
    }

    tFunctionSpillKind tFunctionSumX::SpillKind() const {
        return(tFunctionSpillKind::Aggregate);
    }

    // Function Mod  ==========================================================
    tFunctionMod::tFunctionMod() : tFunction() {}
    
    tStackElem tFunctionMod::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size() != 2) {
            wValue.SetError(tClassError(tTypeError::t_arg, "MOD requires 2 arguments"));
            return(tStackElem(wValue));
        }
        
        // MOD(number, divisor); In RPN: wArgs[0]=divisor, wArgs[1]=number
        tVariant wNumber;
        tVariant wDivisor;
        if (!StackElemToVariant(wArgs[1], wNumber) || !StackElemToVariant(wArgs[0], wDivisor)) {
            wValue.SetError(tClassError(tTypeError::t_arg, "MOD requires numeric arguments"));
            return(tStackElem(wValue));
        }
        if (wNumber.IsError()) return(tStackElem(wNumber));
        if (wDivisor.IsError()) return(tStackElem(wDivisor));
        
        // Validate arguments are numeric
        if (!wNumber.IsInt() && !wNumber.IsDouble()) {
            wValue.SetError(tClassError(tTypeError::t_arg, "MOD: number must be numeric"));
            return(tStackElem(wValue));
        }
        if (!wDivisor.IsInt() && !wDivisor.IsDouble()) {
            wValue.SetError(tClassError(tTypeError::t_arg, "MOD: divisor must be numeric"));
            return(tStackElem(wValue));
        }
        
        // Check divisor is not zero
        if ((wDivisor.IsInt() && wDivisor.Int() == 0) || 
            (wDivisor.IsDouble() && wDivisor.Double() == 0.0)) {
            wValue.SetError(tClassError(tTypeError::t_div0, ""));
            return(tStackElem(wValue));
        }
        
        // Calculate modulo
        if (wNumber.IsInt() && wDivisor.IsInt()) {
            wValue.SetInt(wNumber.Int() % wDivisor.Int());
        } else {
            tDouble wNum = wNumber.IsInt() ? static_cast<tDouble>(wNumber.Int()) : wNumber.Double();
            tDouble wDiv = wDivisor.IsInt() ? static_cast<tDouble>(wDivisor.Int()) : wDivisor.Double();
            wValue.SetDouble(fmod(wNum, wDiv));
        }
        
        return(tStackElem(wValue));
    };

    // Function Power  ==========================================================
    tFunctionPower::tFunctionPower() : tFunction() {}
    
    tStackElem tFunctionPower::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size() != 2) {
            wValue.SetError(tClassError(tTypeError::t_arg, "POWER requires 2 arguments"));
            return(tStackElem(wValue));
        }
        
        tVariant wBase;
        tVariant wExponent;
        tInt wIndex = 0;
        
        // Inverse loop (stack) - parser reverses arguments
        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tVariant wArgVal;
            if (!StackElemToVariant(*it, wArgVal)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return(tStackElem(wValue));
            }
            if (wArgVal.IsError()) return(tStackElem(wArgVal));
            if (wIndex == 0) wBase = wArgVal;
            else if (wIndex == 1) wExponent = wArgVal;
            wIndex++;
        }
        
        // Calculate power
        tDouble wBaseVal = wBase.IsInt() ? static_cast<tDouble>(wBase.Int()) : wBase.Double();
        tDouble wExpVal = wExponent.IsInt() ? static_cast<tDouble>(wExponent.Int()) : wExponent.Double();
        tDouble wResult = pow(wBaseVal, wExpVal);
        
        // Check for errors (infinity, NaN)
        if (!isfinite(wResult)) {
            wValue.SetError(tClassError(tTypeError::t_num, ""));
            return(tStackElem(wValue));
        }
        
        // Return integer if both inputs were integers and result is whole number
        if (wBase.IsInt() && wExponent.IsInt() && wResult == floor(wResult)) {
            wValue.SetInt(static_cast<tInt>(wResult));
        } else {
            wValue.SetDouble(wResult);
        }
        
        return(tStackElem(wValue));
    };

    // Helper function to convert string to number if possible (Excel behavior)
    static tVariant TryConvertStringToNumber(const tVariant& sVariant) {
        if (sVariant.IsInt() || sVariant.IsDouble()) {
            return sVariant; // Already numeric
        }
        if (sVariant.Type() == tVariantType::t_string) {
            tString wStr = sVariant.String();
            // Remove leading/trailing whitespace
            tClassString wClassString(wStr);
            wStr = wClassString.Trim();
            
            if (wStr.empty()) {
                return tVariant(); // Return null variant for empty string
            }
            
            // Try to parse as integer first
            tStringStream wStreamInt(wStr);
            tInt wInt;
            if (wStreamInt >> wInt) {
                tChar wPeek = wStreamInt.peek();
                if (wStreamInt.eof() || wPeek == EOF || wPeek == '\0') {
                    return tVariant(wInt);
                }
            }
            
            // Try to parse as double
            tStringStream wStreamDouble(wStr);
            tDouble wDouble;
            if (wStreamDouble >> wDouble) {
                tChar wPeek = wStreamDouble.peek();
                if (wStreamDouble.eof() || wPeek == EOF || wPeek == '\0') {
                    return tVariant(wDouble);
                }
            }
        }
        return tVariant(); // Return null variant if cannot convert
    }

    // CallBack for function product ============================================
    tCallBackRangeProduct::tCallBackRangeProduct(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange) {
        m_Value = tVariant(1); // Initialize to 1 for multiplication
    }

    tBool tCallBackRangeProduct::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell != nullptr) {
            const tVariant& wCellValue = wCell->CalculableValue();
            // Try to convert string to number if possible (Excel behavior)
            tVariant wNumericValue = TryConvertStringToNumber(wCellValue);
            // PRODUCT multiplies numeric values (ignores text that can't be converted, errors, null)
            if (wNumericValue.IsInt() || wNumericValue.IsDouble()) {
                m_Value = m_Value * wNumericValue;
            }
        }
        return(true); // false for Stop
    };

    // Function Product =============================================================
    tFunctionProduct::tFunctionProduct() : tFunction() {}

    tStackElem tFunctionProduct::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue(1); // Start with 1 for multiplication
        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tStackElem& wArg = *it;
            if (wArg.Type() == tStackType::t_Range) {
                tRange* wRange = wArg.Range();
                if (wRange != nullptr) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    tCallBackRangeProduct wCallBackProduct(wColRowCellRange);
                    wColRowCellRange->VisitorRange(wRange, &wCallBackProduct);
                    wValue = wValue * wCallBackProduct.Value();
                }
            } else if (wArg.Type() == tStackType::t_Array) {
                tArrayValue* wArray = wArg.Array();
                if (wArray == nullptr) continue;
                for (tIndex r = 0; r < wArray->m_Rows; ++r) {
                    for (tIndex c = 0; c < wArray->m_Cols; ++c) {
                        const tVariant& wVariant = wArray->At(r, c);
                        if (wVariant.IsError()) continue;
                        tVariant wNumericValue = TryConvertStringToNumber(wVariant);
                        if (wNumericValue.IsInt() || wNumericValue.IsDouble()) {
                            wValue = wValue * wNumericValue;
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(wArg, wVariant) && !wVariant.IsError()) {
                    tVariant wNumericValue = TryConvertStringToNumber(wVariant);
                    if (wNumericValue.IsInt() || wNumericValue.IsDouble()) {
                        wValue = wValue * wNumericValue;
                    }
                }
            }
        }
        return(tStackElem(wValue));
    };

    namespace {
    struct tSumProductArg {
        tRange* m_Range;
        tVariant m_Scalar;
        tBool m_IsRange;
    };

    // Excel SUMPRODUCT: coerce to number; text that cannot convert counts as 0; errors propagate.
    static tBool SumProductToNumber(const tVariant& sVariant, tVariant& sOut) {
        if (sVariant.IsError()) {
            sOut = sVariant;
            return false;
        }
        if (sVariant.IsInt() || sVariant.IsDouble()) {
            sOut = sVariant;
            return true;
        }
        if (sVariant.IsBool()) {
            sOut = sVariant.Bool() ? tVariant(1) : tVariant(0);
            return true;
        }
        if (sVariant.IsNull()) {
            sOut = tVariant(0);
            return true;
        }
        tVariant wNumericValue = TryConvertStringToNumber(sVariant);
        if (wNumericValue.IsInt() || wNumericValue.IsDouble()) {
            sOut = wNumericValue;
            return true;
        }
        sOut = tVariant(0);
        return true;
    }
    } // namespace

    // Function SumProduct =============================================================
    tFunctionSumProduct::tFunctionSumProduct() : tFunction() {}

    tStackElem tFunctionSumProduct::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUMPRODUCT requires at least 1 argument"))));
        }

        std::vector<tSumProductArg> wProductArgs;
        wProductArgs.reserve(wArgs.size());

        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tStackElem& wArg = *it;
            if (wArg.Type() == tStackType::t_Range) {
                tRange* wRange = wArg.Range();
                if (wRange == nullptr) {
                    continue;
                }
                // Single-cell refs broadcast like scalars (Excel SUMPRODUCT(A:A, B1)).
                if (wRange->IsCell()) {
                    tCell* wCell = wRange->EnsureCell();
                    tVariant wCellValue = (wCell != nullptr) ? wCell->CalculableValue() : tVariant(0);
                    wProductArgs.push_back({ nullptr, wCellValue, false });
                } else {
                    wProductArgs.push_back({ wRange, tVariant(), true });
                }
            } else {
                tVariant wArgVal;
                if (!StackElemToVariant(wArg, wArgVal)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SUMPRODUCT invalid argument"))));
                }
                wProductArgs.push_back({ nullptr, wArgVal, false });
            }
        }

        if (wProductArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUMPRODUCT requires at least 1 argument"))));
        }

        tRange* wDriverRange = nullptr;
        for (const tSumProductArg& wArg : wProductArgs) {
            if (wArg.m_IsRange) {
                wDriverRange = wArg.m_Range;
                break;
            }
        }

        if (wDriverRange == nullptr) {
            tVariant wProduct(1);
            for (const tSumProductArg& wArg : wProductArgs) {
                tVariant wNum;
                if (!SumProductToNumber(wArg.m_Scalar, wNum)) {
                    return(tStackElem(wNum));
                }
                wProduct = wProduct * wNum;
                if (wProduct.IsError()) {
                    return(tStackElem(wProduct));
                }
            }
            return(tStackElem(wProduct));
        }

        const tIndex hLogical = wDriverRange->BottomIndex() - wDriverRange->TopIndex();
        const tIndex wLogical = wDriverRange->RightIndex() - wDriverRange->LeftIndex();
        auto sameShape = [&](tRange* sRange) -> tBool {
            return (sRange->BottomIndex() - sRange->TopIndex() == hLogical)
                && (sRange->RightIndex() - sRange->LeftIndex() == wLogical);
        };

        for (const tSumProductArg& wArg : wProductArgs) {
            if (wArg.m_IsRange && !sameShape(wArg.m_Range)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SUMPRODUCT ranges must be the same size"))));
            }
        }

        const tIndex h = wDriverRange->IterateBottom() - wDriverRange->TopIndex();
        const tIndex w = wDriverRange->IterateRight() - wDriverRange->LeftIndex();
        tVariant wSum(0);
        tBool wContinue = true;

        for (tIndex dRow = 0; wContinue && dRow <= h; ++dRow) {
            for (tIndex dCol = 0; wContinue && dCol <= w; ++dCol) {
                tVariant wProduct(1);
                for (const tSumProductArg& wArg : wProductArgs) {
                    tVariant wNum;
                    if (wArg.m_IsRange) {
                        tRange* wRange = wArg.m_Range;
                        tIndex wRow = wRange->TopIndex() + dRow;
                        tIndex wCol = wRange->LeftIndex() + dCol;
                        tCell* wCell = wRange->ColRowCellRange()->Cell(wRow, wCol);
                        tVariant wCellValue = (wCell != nullptr) ? wCell->CalculableValue() : tVariant(0);
                        if (!SumProductToNumber(wCellValue, wNum)) {
                            wContinue = false;
                            wSum = wNum;
                            break;
                        }
                    } else if (!SumProductToNumber(wArg.m_Scalar, wNum)) {
                        wContinue = false;
                        wSum = wNum;
                        break;
                    }
                    wProduct = wProduct * wNum;
                    if (wProduct.IsError()) {
                        wContinue = false;
                        wSum = wProduct;
                        break;
                    }
                }
                if (wContinue) {
                    wSum = wSum + wProduct;
                    if (wSum.IsError()) {
                        wContinue = false;
                    }
                }
            }
        }

        return(tStackElem(wSum));
    };

    // Function Rand ==========================================================
    tFunctionRand::tFunctionRand() : tFunction() {}

    tStackElem tFunctionRand::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size() != 0) {
            wValue.SetError(tClassError(tTypeError::t_arg, "RAND requires no arguments"));
            return(tStackElem(wValue));
        }
        
        // Generate random number between 0 and 1
        tDouble wRandom = static_cast<tDouble>(rand()) / static_cast<tDouble>(RAND_MAX);
        wValue.SetDouble(wRandom);
        return(tStackElem(wValue));
    };

    // Function RandBetween ==========================================================
    tFunctionRandBetween::tFunctionRandBetween() : tFunction() {}

    tStackElem tFunctionRandBetween::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        if (wArgs.size() != 2) {
            wValue.SetError(tClassError(tTypeError::t_arg, "RANDBETWEEN requires 2 arguments"));
            return(tStackElem(wValue));
        }
        
        tVariant wBottom;
        tVariant wTop;
        tInt wIndex = 0;
        
        // Inverse loop (stack) - parser reverses arguments
        // Process arguments in reverse order (first argument was last on stack)
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tVariant wArgVal;
            if (!StackElemToVariant(*it, wArgVal)) {
                wValue.SetError(tClassError(tTypeError::t_arg, ""));
                return(tStackElem(wValue));
            }
            if (wArgVal.IsError()) return(tStackElem(wArgVal));
            if (wIndex == 0) wBottom = wArgVal;
            else if (wIndex == 1) wTop = wArgVal;
            wIndex++;
        }
        
        // Convert to integers
        tInt wBottomInt = wBottom.IsInt() ? wBottom.Int() : static_cast<tInt>(wBottom.Double());
        tInt wTopInt = wTop.IsInt() ? wTop.Int() : static_cast<tInt>(wTop.Double());
        
        // Validate range
        if (wBottomInt > wTopInt) {
            wValue.SetError(tClassError(tTypeError::t_arg, "RANDBETWEEN: bottom must be <= top"));
            return(tStackElem(wValue));
        }
        
        // Generate random integer between bottom and top (inclusive)
        tInt wRange = wTopInt - wBottomInt + 1;
        tInt wRandom = wBottomInt + (rand() % wRange);
        wValue.SetInt(wRandom);
        
        return(tStackElem(wValue));
    };

	// SpillKind (range aggregation vs element-wise spill; see tFunctionSpillKind)
	tFunctionSpillKind tFunctionSum::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionAverage::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionMedian::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionVariance::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionNth::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionMode::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionRankEq::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionMin::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionMax::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionProduct::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionSumProduct::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
	tFunctionSpillKind tFunctionRand::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

} // End of namespace
