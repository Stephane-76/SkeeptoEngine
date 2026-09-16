//=============================================================================
// SkSpreadSheet Function Array (dynamic arrays: SEQUENCE, SORT, ...)
//=============================================================================
#include "../include/SkFunctionArray.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkRangeNamed.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace SkSpreadSheet {

    namespace {

        // Produce an integer variant when the value is whole (Excel shows SEQUENCE
        // as integers unless start/step are fractional), otherwise a double.
        tVariant MakeNumberVariant(tDouble sValue) {
            if (std::isfinite(sValue) && sValue == std::floor(sValue) &&
                std::fabs(sValue) < 9.0e15) {
                return tVariant(static_cast<tInt>(sValue));
            }
            return tVariant(sValue);
        }

        // Flatten a function argument (scalar, range, literal array) into a row-major
        // list of integers. Used by SORT for multi-key sort_index / sort_order arrays.
        tBool ReadStackElemToIntVector(const tStackElem& sArg, std::vector<tInt>& oOut) {
            tArrayValue wArr;
            if (!tFunction::StackElemToArray(sArg, wArr) || wArr.Count() <= 0) {
                return(false);
            }
            oOut.clear();
            for (tIndex r = 0; r < wArr.m_Rows; r++) {
                for (tIndex c = 0; c < wArr.m_Cols; c++) {
                    tVariant wV = wArr.At(r, c);
                    if (wV.IsNumeric()) {
                        oOut.push_back(static_cast<tInt>(wV.Numeric()));
                    } else {
                        return(false);
                    }
                }
            }
            return(!oOut.empty());
        }

        // Read a square numeric matrix; blank/null → 0. On failure sets oError.
        tBool ReadSquareMatrix(const tStackElem& sArg,
                               std::vector<tDouble>& oMat,
                               tIndex& oN,
                               tVariant& oError) {
            tArrayValue wArr;
            if (!tFunction::StackElemToArray(sArg, wArr) || wArr.Count() <= 0) {
                oError = tVariant(tClassError(tTypeError::t_value, ""));
                return(false);
            }
            if (wArr.m_Rows != wArr.m_Cols) {
                oError = tVariant(tClassError(tTypeError::t_value, ""));
                return(false);
            }
            oN = wArr.m_Rows;
            oMat.assign(static_cast<size_t>(oN) * static_cast<size_t>(oN), 0.0);
            for (tIndex r = 0; r < oN; ++r) {
                for (tIndex c = 0; c < oN; ++c) {
                    const tVariant& wV = wArr.At(r, c);
                    if (wV.IsError()) {
                        oError = wV;
                        return(false);
                    }
                    if (wV.IsNull()) {
                        oMat[static_cast<size_t>(r) * oN + c] = 0.0;
                    } else if (wV.IsNumeric()) {
                        oMat[static_cast<size_t>(r) * oN + c] = wV.Numeric();
                    } else {
                        oError = tVariant(tClassError(tTypeError::t_value, ""));
                        return(false);
                    }
                }
            }
            return(true);
        }

        // Determinant via Gaussian elimination with partial pivoting.
        // Returns false if the matrix is numerically singular (det ≈ 0 beyond tolerance
        // is still returned as 0.0 — Excel MDETERM of singular is 0).
        tDouble MatrixDeterminant(std::vector<tDouble> sMat, tIndex sN) {
            if (sN == 0) return(0.0);
            tDouble wDet = 1.0;
            for (tIndex k = 0; k < sN; ++k) {
                tIndex wPivot = k;
                tDouble wMax = std::fabs(sMat[static_cast<size_t>(k) * sN + k]);
                for (tIndex i = k + 1; i < sN; ++i) {
                    const tDouble wAbs = std::fabs(sMat[static_cast<size_t>(i) * sN + k]);
                    if (wAbs > wMax) {
                        wMax = wAbs;
                        wPivot = i;
                    }
                }
                if (wMax < 1.0e-14) {
                    return(0.0);
                }
                if (wPivot != k) {
                    for (tIndex c = k; c < sN; ++c) {
                        std::swap(sMat[static_cast<size_t>(k) * sN + c],
                                  sMat[static_cast<size_t>(wPivot) * sN + c]);
                    }
                    wDet = -wDet;
                }
                const tDouble wDiag = sMat[static_cast<size_t>(k) * sN + k];
                wDet *= wDiag;
                for (tIndex i = k + 1; i < sN; ++i) {
                    const tDouble wFactor =
                        sMat[static_cast<size_t>(i) * sN + k] / wDiag;
                    for (tIndex j = k; j < sN; ++j) {
                        sMat[static_cast<size_t>(i) * sN + j] -=
                            wFactor * sMat[static_cast<size_t>(k) * sN + j];
                    }
                }
            }
            return(wDet);
        }

        // Gauss–Jordan inverse of [A|I] → [I|A⁻¹]. Returns false if singular.
        tBool MatrixInverse(std::vector<tDouble> sMat, tIndex sN, std::vector<tDouble>& oInv) {
            const size_t wN = static_cast<size_t>(sN);
            std::vector<tDouble> wAug(wN * wN * 2, 0.0);
            for (tIndex r = 0; r < sN; ++r) {
                for (tIndex c = 0; c < sN; ++c) {
                    wAug[static_cast<size_t>(r) * (wN * 2) + c] =
                        sMat[static_cast<size_t>(r) * wN + c];
                }
                wAug[static_cast<size_t>(r) * (wN * 2) + wN + static_cast<size_t>(r)] = 1.0;
            }
            const tIndex wCols = sN * 2;
            for (tIndex k = 0; k < sN; ++k) {
                tIndex wPivot = k;
                tDouble wMax = std::fabs(wAug[static_cast<size_t>(k) * wCols + k]);
                for (tIndex i = k + 1; i < sN; ++i) {
                    const tDouble wAbs = std::fabs(wAug[static_cast<size_t>(i) * wCols + k]);
                    if (wAbs > wMax) {
                        wMax = wAbs;
                        wPivot = i;
                    }
                }
                if (wMax < 1.0e-14) {
                    return(false);
                }
                if (wPivot != k) {
                    for (tIndex c = 0; c < wCols; ++c) {
                        std::swap(wAug[static_cast<size_t>(k) * wCols + c],
                                  wAug[static_cast<size_t>(wPivot) * wCols + c]);
                    }
                }
                const tDouble wDiag = wAug[static_cast<size_t>(k) * wCols + k];
                for (tIndex c = 0; c < wCols; ++c) {
                    wAug[static_cast<size_t>(k) * wCols + c] /= wDiag;
                }
                for (tIndex i = 0; i < sN; ++i) {
                    if (i == k) continue;
                    const tDouble wFactor = wAug[static_cast<size_t>(i) * wCols + k];
                    for (tIndex c = 0; c < wCols; ++c) {
                        wAug[static_cast<size_t>(i) * wCols + c] -=
                            wFactor * wAug[static_cast<size_t>(k) * wCols + c];
                    }
                }
            }
            oInv.resize(wN * wN);
            for (tIndex r = 0; r < sN; ++r) {
                for (tIndex c = 0; c < sN; ++c) {
                    oInv[static_cast<size_t>(r) * wN + c] =
                        wAug[static_cast<size_t>(r) * wCols + wN + static_cast<size_t>(c)];
                }
            }
            return(true);
        }

    } // anonymous namespace

    // SEQUENCE ===============================================================
    tFunctionSequence::tFunctionSequence() {}

    tStackElem tFunctionSequence::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // PopArgs yields last formula arg first; reverse to formula order (rows, columns, start, step).
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        // Propagate an error argument (e.g. SEQUENCE(#REF!)).
        for (const tStackElem& wArg : wArgs) {
            tVariant wProbe;
            if (StackElemToVariant(wArg, wProbe) && wProbe.IsError()) {
                return(tStackElem(wProbe));
            }
        }

        tInt wRows = 0;
        tInt wCols = 1;
        tDouble wStart = 1.0;
        tDouble wStep = 1.0;
        if (!StackElemToInt(wArgs[0], wRows)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wArgs.size() >= 2) {
            tInt wC;
            if (StackElemToInt(wArgs[1], wC)) wCols = wC;
        }
        if (wArgs.size() >= 3) {
            tVariant wV;
            if (StackElemToVariant(wArgs[2], wV) && wV.IsNumeric()) wStart = wV.Numeric();
        }
        if (wArgs.size() >= 4) {
            tVariant wV;
            if (StackElemToVariant(wArgs[3], wV) && wV.IsNumeric()) wStep = wV.Numeric();
        }

        if (wRows < 1 || wCols < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        // Guard against pathological spill sizes.
        if (static_cast<tDouble>(wRows) * static_cast<tDouble>(wCols) > 4.0e6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }

        tArrayValue* wArray = new tArrayValue(static_cast<tIndex>(wRows), static_cast<tIndex>(wCols));
        tIndex wK = 0;
        for (tIndex r = 0; r < static_cast<tIndex>(wRows); r++) {
            for (tIndex c = 0; c < static_cast<tIndex>(wCols); c++) {
                const tDouble wVal = wStart + static_cast<tDouble>(wK) * wStep;
                wArray->At(r, c) = MakeNumberVariant(wVal);
                wK++;
            }
        }
        return(tStackElem(wArray));
    }

    // RANDARRAY ==============================================================
    tFunctionRandArray::tFunctionRandArray() {}

    tStackElem tFunctionRandArray::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // PopArgs yields last formula arg first; reverse to formula order.
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        for (const tStackElem& wArg : wArgs) {
            tVariant wProbe;
            if (StackElemToVariant(wArg, wProbe) && wProbe.IsError()) {
                return(tStackElem(wProbe));
            }
        }

        tInt wRows = 1;
        tInt wCols = 1;
        tDouble wMin = 0.0;
        tDouble wMax = 1.0;
        tBool wInteger = false;

        if (!wArgs.empty()) {
            if (!StackElemToInt(wArgs[0], wRows)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        if (wArgs.size() >= 2) {
            if (!StackElemToInt(wArgs[1], wCols)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        if (wArgs.size() >= 3) {
            tVariant wV;
            if (!StackElemToVariant(wArgs[2], wV) || !wV.IsNumeric()) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wMin = wV.Numeric();
        }
        if (wArgs.size() >= 4) {
            tVariant wV;
            if (!StackElemToVariant(wArgs[3], wV) || !wV.IsNumeric()) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wMax = wV.Numeric();
        }
        if (wArgs.size() >= 5) {
            if (!StackElemToBool(wArgs[4], wInteger)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        if (wRows < 1 || wCols < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wMin > wMax) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (static_cast<tDouble>(wRows) * static_cast<tDouble>(wCols) > 4.0e6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }

        tArrayValue* wArray = new tArrayValue(static_cast<tIndex>(wRows), static_cast<tIndex>(wCols));
        const tDouble wSpan = wMax - wMin;
        for (tIndex r = 0; r < static_cast<tIndex>(wRows); r++) {
            for (tIndex c = 0; c < static_cast<tIndex>(wCols); c++) {
                if (wInteger) {
                    // Inclusive integer range (like RANDBETWEEN).
                    const tInt wLo = static_cast<tInt>(std::ceil(wMin));
                    const tInt wHi = static_cast<tInt>(std::floor(wMax));
                    if (wLo > wHi) {
                        delete wArray;
                        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                    }
                    const tInt wRange = wHi - wLo + 1;
                    const tInt wRnd = wLo + (rand() % wRange);
                    wArray->At(r, c) = MakeNumberVariant(static_cast<tDouble>(wRnd));
                } else {
                    const tDouble wU = static_cast<tDouble>(rand()) / static_cast<tDouble>(RAND_MAX);
                    wArray->At(r, c) = MakeNumberVariant(wMin + wU * wSpan);
                }
            }
        }
        return(tStackElem(wArray));
    }

    // SORT ===================================================================
    tFunctionSort::tFunctionSort() {}

    tStackElem tFunctionSort::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order (array, sort_index, sort_order, by_col).
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        // sort_index and sort_order may each be a single value OR an array of values,
        // enabling multi-key sorts (Excel: SORT(array, {9,8,6,1}, {-1,-1,-1,1})). Keys are
        // applied in order: the first key decides; ties fall through to the next key.
        std::vector<tInt> wSortIndex;
        std::vector<tInt> wSortOrder;
        tBool wByCol = false;
        if (wArgs.size() >= 2) {
            ReadStackElemToIntVector(wArgs[1], wSortIndex);
        }
        if (wSortIndex.empty()) {
            wSortIndex.push_back(1);
        }
        if (wArgs.size() >= 3) {
            ReadStackElemToIntVector(wArgs[2], wSortOrder);
        }
        if (wArgs.size() >= 4) {
            tBool wV;
            if (StackElemToBool(wArgs[3], wV)) wByCol = wV;
        }

        // Validate every 1-based key against the dimension being sorted along; store 0-based.
        const tInt wKeyMax = wByCol ? static_cast<tInt>(wSource.m_Rows)
                                    : static_cast<tInt>(wSource.m_Cols);
        std::vector<tIndex> wKeys;
        wKeys.reserve(wSortIndex.size());
        for (tInt wIdx : wSortIndex) {
            if (wIdx < 1 || wIdx > wKeyMax) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wKeys.push_back(static_cast<tIndex>(wIdx - 1));
        }

        // Per-key descending flag: matching sort_order entry, else the last one supplied, else ascending.
        auto wIsDescending = [&](size_t sK) -> bool {
            if (wSortOrder.empty()) return(false);
            if (sK < wSortOrder.size()) return(wSortOrder[sK] < 0);
            return(wSortOrder.back() < 0);
        };

        // Lexicographic comparison across all keys.
        auto wLess = [&](tIndex sA, tIndex sB) -> bool {
            for (size_t k = 0; k < wKeys.size(); k++) {
                const tIndex wKey = wKeys[k];
                const tBool wDesc = wIsDescending(k);
                const tVariant& wVa = wByCol ? wSource.At(wKey, sA) : wSource.At(sA, wKey);
                const tVariant& wVb = wByCol ? wSource.At(wKey, sB) : wSource.At(sB, wKey);
                if (wVa < wVb) return(!wDesc);
                if (wVb < wVa) return(wDesc);
            }
            return(false);
        };

        tArrayValue* wOut = new tArrayValue(wSource.m_Rows, wSource.m_Cols);

        if (!wByCol) {
            // Sort rows by the key column(s).
            std::vector<tIndex> wOrder(static_cast<size_t>(wSource.m_Rows));
            for (tIndex i = 0; i < wSource.m_Rows; i++) wOrder[i] = i;
            std::stable_sort(wOrder.begin(), wOrder.end(), wLess);
            for (tIndex r = 0; r < wSource.m_Rows; r++) {
                for (tIndex c = 0; c < wSource.m_Cols; c++) {
                    wOut->At(r, c) = wSource.At(wOrder[r], c);
                }
            }
        } else {
            // Sort columns by the key row(s).
            std::vector<tIndex> wOrder(static_cast<size_t>(wSource.m_Cols));
            for (tIndex i = 0; i < wSource.m_Cols; i++) wOrder[i] = i;
            std::stable_sort(wOrder.begin(), wOrder.end(), wLess);
            for (tIndex r = 0; r < wSource.m_Rows; r++) {
                for (tIndex c = 0; c < wSource.m_Cols; c++) {
                    wOut->At(r, c) = wSource.At(r, wOrder[c]);
                }
            }
        }

        return(tStackElem(wOut));
    }

    // UNIQUE ==================================================================
    tFunctionUnique::tFunctionUnique() : tFunction() {}

    tStackElem tFunctionUnique::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order (array, by_col, exactly_once).
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tBool wByCol = false;
        tBool wExactlyOnce = false;
        if (wArgs.size() >= 2) {
            tBool wV;
            if (StackElemToBool(wArgs[1], wV)) wByCol = wV;
        }
        if (wArgs.size() >= 3) {
            tBool wV;
            if (StackElemToBool(wArgs[2], wV)) wExactlyOnce = wV;
        }

        // Two vectors (row or column indices) are equal when every element matches
        // (engine equality, same as the "=" operator).
        auto wRowsEqual = [&](tIndex sA, tIndex sB) -> bool {
            for (tIndex c = 0; c < wSource.m_Cols; c++) {
                if (!(wSource.At(sA, c) == wSource.At(sB, c))) return(false);
            }
            return(true);
        };
        auto wColsEqual = [&](tIndex sA, tIndex sB) -> bool {
            for (tIndex r = 0; r < wSource.m_Rows; r++) {
                if (!(wSource.At(r, sA) == wSource.At(r, sB))) return(false);
            }
            return(true);
        };

        // Walk the source along the chosen axis, keeping first-occurrence order and a per-group
        // occurrence count so exactly_once can drop everything seen more than once.
        const tIndex wCount = wByCol ? wSource.m_Cols : wSource.m_Rows;
        std::vector<tIndex> wFirst;   // index of the first occurrence of each distinct group
        std::vector<tInt> wOccur;     // number of occurrences of that group
        for (tIndex i = 0; i < wCount; i++) {
            int wHit = -1;
            for (size_t k = 0; k < wFirst.size(); k++) {
                const bool wSame = wByCol ? wColsEqual(i, wFirst[k]) : wRowsEqual(i, wFirst[k]);
                if (wSame) { wHit = static_cast<int>(k); break; }
            }
            if (wHit < 0) { wFirst.push_back(i); wOccur.push_back(1); }
            else { wOccur[static_cast<size_t>(wHit)]++; }
        }

        std::vector<tIndex> wSelected;
        for (size_t k = 0; k < wFirst.size(); k++) {
            if (!wExactlyOnce || wOccur[k] == 1) wSelected.push_back(wFirst[k]);
        }
        // Empty result (e.g. exactly_once with all values duplicated): Excel returns #CALC!.
        if (wSelected.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        tArrayValue* wOut = nullptr;
        if (!wByCol) {
            wOut = new tArrayValue(static_cast<tIndex>(wSelected.size()), wSource.m_Cols);
            for (size_t r = 0; r < wSelected.size(); r++) {
                for (tIndex c = 0; c < wSource.m_Cols; c++) {
                    wOut->At(static_cast<tIndex>(r), c) = wSource.At(wSelected[r], c);
                }
            }
        } else {
            wOut = new tArrayValue(wSource.m_Rows, static_cast<tIndex>(wSelected.size()));
            for (tIndex r = 0; r < wSource.m_Rows; r++) {
                for (size_t c = 0; c < wSelected.size(); c++) {
                    wOut->At(r, static_cast<tIndex>(c)) = wSource.At(r, wSelected[c]);
                }
            }
        }
        return(tStackElem(wOut));
    }

    // FILTER =================================================================
    namespace {
        // Excel truthiness for a FILTER mask element: TRUE/non-zero number keeps the row/column.
        // Text, null and errors are treated as FALSE (errors are excluded rather than propagated).
        tBool MaskElementIsTrue(const tVariant& sValue) {
            if (sValue.IsBool()) return(sValue.Bool());
            if (sValue.IsInt()) return(sValue.Int() != 0);
            if (sValue.IsDouble()) return(sValue.Double() != 0.0);
            return(false);
        }
    }

    tFunctionFilter::tFunctionFilter() : tFunction() {}

    tStackElem tFunctionFilter::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order (array, include, if_empty).
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tArrayValue wData;
        if (!StackElemToArray(wArgs[0], wData) || wData.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tArrayValue wMask;
        if (!StackElemToArray(wArgs[1], wMask) || wMask.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        // The mask decides the axis: a column vector (height == array height) filters rows,
        // a row vector (width == array width) filters columns.
        const tBool wRowMode = (wMask.m_Cols == 1 && wMask.m_Rows == wData.m_Rows);
        const tBool wColMode = (wMask.m_Rows == 1 && wMask.m_Cols == wData.m_Cols);
        if (!wRowMode && !wColMode) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        std::vector<tIndex> wKeep;
        if (wRowMode) {
            for (tIndex r = 0; r < wData.m_Rows; r++) {
                if (MaskElementIsTrue(wMask.At(r, 0))) wKeep.push_back(r);
            }
        } else {
            for (tIndex c = 0; c < wData.m_Cols; c++) {
                if (MaskElementIsTrue(wMask.At(0, c))) wKeep.push_back(c);
            }
        }

        // Nothing matched: return if_empty when supplied, otherwise #CALC! (Excel).
        if (wKeep.empty()) {
            if (wArgs.size() >= 3) {
                tVariant wIfEmpty;
                if (!StackElemToVariant(wArgs[2], wIfEmpty)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
                }
                return(tStackElem(wIfEmpty));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        tArrayValue* wOut = nullptr;
        if (wRowMode) {
            wOut = new tArrayValue(static_cast<tIndex>(wKeep.size()), wData.m_Cols);
            for (size_t r = 0; r < wKeep.size(); r++) {
                for (tIndex c = 0; c < wData.m_Cols; c++) {
                    wOut->At(static_cast<tIndex>(r), c) = wData.At(wKeep[r], c);
                }
            }
        } else {
            wOut = new tArrayValue(wData.m_Rows, static_cast<tIndex>(wKeep.size()));
            for (tIndex r = 0; r < wData.m_Rows; r++) {
                for (size_t c = 0; c < wKeep.size(); c++) {
                    wOut->At(r, static_cast<tIndex>(c)) = wData.At(r, wKeep[c]);
                }
            }
        }
        return(tStackElem(wOut));
    }

    // ---- Higher-order functions (take a first-class LAMBDA argument) --------------------------------------

    tFunctionMap::tFunctionMap() : tFunction() {}

    tStackElem tFunctionMap::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order: array1, [array2, ...], lambda.
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tFormulaNamed* wLambda = wArgs.back().Lambda();
        if (wLambda == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        // Closure environment captured when the lambda value was created (nullptr for non-closure lambdas).
        const std::map<tString, tStackElem>* wCaptured = wArgs.back().CapturedScope();
        const tSize wNbArrays = wArgs.size() - 1;
        if (wLambda->LambdaParams().size() != wNbArrays) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        // Read every source array; they must share the same shape (Excel requirement).
        std::vector<tArrayValue> wArrays(wNbArrays);
        for (tSize k = 0; k < wNbArrays; k++) {
            if (!StackElemToArray(wArgs[k], wArrays[k]) || wArrays[k].Count() <= 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (wArrays[k].m_Rows != wArrays[0].m_Rows || wArrays[k].m_Cols != wArrays[0].m_Cols) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        const tIndex wRows = wArrays[0].m_Rows;
        const tIndex wCols = wArrays[0].m_Cols;
        tArrayValue* wOut = new tArrayValue(wRows, wCols);
        for (tIndex r = 0; r < wRows; r++) {
            for (tIndex c = 0; c < wCols; c++) {
                std::vector<tStackElem> wCall;
                wCall.reserve(wNbArrays);
                for (tSize k = 0; k < wNbArrays; k++) {
                    wCall.push_back(tStackElem(wArrays[k].At(r, c)));
                }
                tBool wArityOk = true;
                wOut->At(r, c) = wLambda->Invoke(wCall, wArityOk, wCaptured);
            }
        }
        return(tStackElem(wOut));
    }

    tFunctionReduce::tFunctionReduce() : tFunction() {}

    tStackElem tFunctionReduce::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order: initial_value, array, lambda.
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tFormulaNamed* wLambda = wArgs[2].Lambda();
        if (wLambda == nullptr || wLambda->LambdaParams().size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const std::map<tString, tStackElem>* wCaptured = wArgs[2].CapturedScope();
        tArrayValue wData;
        if (!StackElemToArray(wArgs[1], wData)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tStackElem wAcc = wArgs[0];
        for (tIndex r = 0; r < wData.m_Rows; r++) {
            for (tIndex c = 0; c < wData.m_Cols; c++) {
                std::vector<tStackElem> wCall;
                wCall.push_back(wAcc);
                wCall.push_back(tStackElem(wData.At(r, c)));
                tBool wArityOk = true;
                wAcc = tStackElem(wLambda->Invoke(wCall, wArityOk, wCaptured));
            }
        }
        return(wAcc);
    }

    tFunctionScan::tFunctionScan() : tFunction() {}

    tStackElem tFunctionScan::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // Reverse to formula order: initial_value, array, lambda.
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tFormulaNamed* wLambda = wArgs[2].Lambda();
        if (wLambda == nullptr || wLambda->LambdaParams().size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const std::map<tString, tStackElem>* wCaptured = wArgs[2].CapturedScope();
        tArrayValue wData;
        if (!StackElemToArray(wArgs[1], wData) || wData.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tStackElem wAcc = wArgs[0];
        tArrayValue* wOut = new tArrayValue(wData.m_Rows, wData.m_Cols);
        for (tIndex r = 0; r < wData.m_Rows; r++) {
            for (tIndex c = 0; c < wData.m_Cols; c++) {
                std::vector<tStackElem> wCall;
                wCall.push_back(wAcc);
                wCall.push_back(tStackElem(wData.At(r, c)));
                tBool wArityOk = true;
                tVariant wRes = wLambda->Invoke(wCall, wArityOk, wCaptured);
                wAcc = tStackElem(wRes);
                wOut->At(r, c) = wRes;
            }
        }
        return(tStackElem(wOut));
    }

    // BYROW / BYCOL ==========================================================
    tFunctionByRowCol::tFunctionByRowCol(tBool sByRow) : tFunction(), m_ByRow(sByRow) {}

    tStackElem tFunctionByRowCol::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // BYROW/BYCOL(array, lambda)
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tFormulaNamed* wLambda = wArgs[1].Lambda();
        if (wLambda == nullptr || wLambda->LambdaParams().size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const std::map<tString, tStackElem>* wCaptured = wArgs[1].CapturedScope();

        tArrayValue wData;
        if (!StackElemToArray(wArgs[0], wData) || wData.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        if (m_ByRow) {
            tArrayValue* wOut = new tArrayValue(wData.m_Rows, 1);
            for (tIndex r = 0; r < wData.m_Rows; ++r) {
                tArrayValue* wSlice = new tArrayValue(1, wData.m_Cols);
                for (tIndex c = 0; c < wData.m_Cols; ++c) {
                    wSlice->At(0, c) = wData.At(r, c);
                }
                std::vector<tStackElem> wCall;
                wCall.push_back(tStackElem(wSlice));
                tBool wArityOk = true;
                wOut->At(r, 0) = wLambda->Invoke(wCall, wArityOk, wCaptured);
            }
            return(tStackElem(wOut));
        }

        tArrayValue* wOut = new tArrayValue(1, wData.m_Cols);
        for (tIndex c = 0; c < wData.m_Cols; ++c) {
            tArrayValue* wSlice = new tArrayValue(wData.m_Rows, 1);
            for (tIndex r = 0; r < wData.m_Rows; ++r) {
                wSlice->At(r, 0) = wData.At(r, c);
            }
            std::vector<tStackElem> wCall;
            wCall.push_back(tStackElem(wSlice));
            tBool wArityOk = true;
            wOut->At(0, c) = wLambda->Invoke(wCall, wArityOk, wCaptured);
        }
        return(tStackElem(wOut));
    }

    // MAKEARRAY ==============================================================
    tFunctionMakeArray::tFunctionMakeArray() {}

    tStackElem tFunctionMakeArray::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // MAKEARRAY(rows, cols, lambda)
        std::reverse(wArgs.begin(), wArgs.end());
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tInt wRows = 0;
        tInt wCols = 0;
        if (!StackElemToInt(wArgs[0], wRows) || !StackElemToInt(wArgs[1], wCols)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wRows < 1 || wCols < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        if (static_cast<tDouble>(wRows) * static_cast<tDouble>(wCols) > 4.0e6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tFormulaNamed* wLambda = wArgs[2].Lambda();
        if (wLambda == nullptr || wLambda->LambdaParams().size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const std::map<tString, tStackElem>* wCaptured = wArgs[2].CapturedScope();

        tArrayValue* wOut = new tArrayValue(static_cast<tIndex>(wRows), static_cast<tIndex>(wCols));
        for (tInt r = 0; r < wRows; ++r) {
            for (tInt c = 0; c < wCols; ++c) {
                std::vector<tStackElem> wCall;
                // Excel MAKEARRAY indices are 1-based.
                wCall.push_back(tStackElem(tVariant(r + 1)));
                wCall.push_back(tStackElem(tVariant(c + 1)));
                tBool wArityOk = true;
                wOut->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) =
                    wLambda->Invoke(wCall, wArityOk, wCaptured);
            }
        }
        return(tStackElem(wOut));
    }

    // MUNIT ==================================================================
    tFunctionMUnit::tFunctionMUnit() {}

    tStackElem tFunctionMUnit::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MUNIT requires 1 argument"))));
        }
        tVariant wProbe;
        if (StackElemToVariant(wArgs[0], wProbe) && wProbe.IsError()) {
            return(tStackElem(wProbe));
        }
        tInt wN = 0;
        if (!StackElemToInt(wArgs[0], wN)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wN < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (static_cast<tDouble>(wN) * static_cast<tDouble>(wN) > 4.0e6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tArrayValue* wOut = new tArrayValue(static_cast<tIndex>(wN), static_cast<tIndex>(wN));
        for (tIndex r = 0; r < static_cast<tIndex>(wN); ++r) {
            for (tIndex c = 0; c < static_cast<tIndex>(wN); ++c) {
                wOut->At(r, c) = tVariant((r == c) ? 1 : 0);
            }
        }
        return(tStackElem(wOut));
    }

    // TRIMRANGE ==============================================================
    tFunctionTrimRange::tFunctionTrimRange() {}

    tStackElem tFunctionTrimRange::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty() || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRIMRANGE requires 1 to 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        // Modes: 0 none, 1 leading, 2 trailing, 3 both (Excel default).
        tInt wRowMode = 3;
        tInt wColMode = 3;
        if (wArgs.size() >= 2) {
            if (!StackElemToInt(wArgs[1], wRowMode) || wRowMode < 0 || wRowMode > 3) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        if (wArgs.size() >= 3) {
            if (!StackElemToInt(wArgs[2], wColMode) || wColMode < 0 || wColMode > 3) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        auto wIsBlank = [](const tVariant& sVal) -> tBool {
            return(sVal.IsNull() || (sVal.IsString() && sVal.String().empty()));
        };
        auto wRowBlank = [&](tIndex sRow) -> tBool {
            for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                if (!wIsBlank(wSource.At(sRow, c))) return(false);
            }
            return(true);
        };
        auto wColBlank = [&](tIndex sCol) -> tBool {
            for (tIndex r = 0; r < wSource.m_Rows; ++r) {
                if (!wIsBlank(wSource.At(r, sCol))) return(false);
            }
            return(true);
        };

        tInt wRowStart = 0;
        tInt wRowEnd = static_cast<tInt>(wSource.m_Rows) - 1;
        tInt wColStart = 0;
        tInt wColEnd = static_cast<tInt>(wSource.m_Cols) - 1;

        if (wRowMode == 1 || wRowMode == 3) {
            while (wRowStart <= wRowEnd && wRowBlank(static_cast<tIndex>(wRowStart))) {
                ++wRowStart;
            }
        }
        if (wRowMode == 2 || wRowMode == 3) {
            while (wRowEnd >= wRowStart && wRowBlank(static_cast<tIndex>(wRowEnd))) {
                --wRowEnd;
            }
        }
        if (wColMode == 1 || wColMode == 3) {
            while (wColStart <= wColEnd && wColBlank(static_cast<tIndex>(wColStart))) {
                ++wColStart;
            }
        }
        if (wColMode == 2 || wColMode == 3) {
            while (wColEnd >= wColStart && wColBlank(static_cast<tIndex>(wColEnd))) {
                --wColEnd;
            }
        }

        if (wRowStart > wRowEnd || wColStart > wColEnd) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        const tIndex wOutRows = static_cast<tIndex>(wRowEnd - wRowStart + 1);
        const tIndex wOutCols = static_cast<tIndex>(wColEnd - wColStart + 1);
        tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
        for (tIndex r = 0; r < wOutRows; ++r) {
            for (tIndex c = 0; c < wOutCols; ++c) {
                wOut->At(r, c) = wSource.At(static_cast<tIndex>(wRowStart) + r,
                                             static_cast<tIndex>(wColStart) + c);
            }
        }
        return(tStackElem(wOut));
    }

    // TRANSPOSE ==============================================================
    tFunctionTranspose::tFunctionTranspose() {}

    tStackElem tFunctionTranspose::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tArrayValue* wOut = new tArrayValue(wSource.m_Cols, wSource.m_Rows);
        for (tIndex r = 0; r < wSource.m_Rows; ++r) {
            for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                wOut->At(c, r) = wSource.At(r, c);
            }
        }
        return(tStackElem(wOut));
    }

    // MMULT ==================================================================
    tFunctionMMult::tFunctionMMult() {}

    tStackElem tFunctionMMult::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MMULT requires 2 arguments"))));
        }
        // RPN: array2 array1 MMULT
        tArrayValue wA;
        tArrayValue wB;
        if (!StackElemToArray(wArgs[1], wA) || !StackElemToArray(wArgs[0], wB) ||
            wA.Count() <= 0 || wB.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wA.m_Cols != wB.m_Rows) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "MMULT: incompatible dimensions"))));
        }
        tArrayValue* wOut = new tArrayValue(wA.m_Rows, wB.m_Cols);
        for (tIndex i = 0; i < wA.m_Rows; ++i) {
            for (tIndex j = 0; j < wB.m_Cols; ++j) {
                tDouble wSum = 0.0;
                tBool wHasError = false;
                tVariant wErr;
                for (tIndex k = 0; k < wA.m_Cols; ++k) {
                    const tVariant& wAv = wA.At(i, k);
                    const tVariant& wBv = wB.At(k, j);
                    if (wAv.IsError()) { wErr = wAv; wHasError = true; break; }
                    if (wBv.IsError()) { wErr = wBv; wHasError = true; break; }
                    if (!wAv.IsNumeric() || !wBv.IsNumeric()) {
                        delete wOut;
                        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                    }
                    wSum += wAv.Numeric() * wBv.Numeric();
                }
                if (wHasError) {
                    delete wOut;
                    return(tStackElem(wErr));
                }
                if (std::isfinite(wSum) && wSum == std::floor(wSum) && std::fabs(wSum) < 9.0e15) {
                    wOut->At(i, j) = tVariant(static_cast<tInt>(wSum));
                } else {
                    wOut->At(i, j) = tVariant(wSum);
                }
            }
        }
        return(tStackElem(wOut));
    }

    // MDETERM ================================================================
    tFunctionMDeterm::tFunctionMDeterm() {}

    tStackElem tFunctionMDeterm::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MDETERM requires 1 argument"))));
        }
        std::vector<tDouble> wMat;
        tIndex wN = 0;
        tVariant wError;
        if (!ReadSquareMatrix(wArgs[0], wMat, wN, wError)) {
            return(tStackElem(wError));
        }
        const tDouble wDet = MatrixDeterminant(std::move(wMat), wN);
        if (!std::isfinite(wDet)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        return(tStackElem(MakeNumberVariant(wDet)));
    }

    // MINVERSE ===============================================================
    tFunctionMInverse::tFunctionMInverse() {}

    tStackElem tFunctionMInverse::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MINVERSE requires 1 argument"))));
        }
        std::vector<tDouble> wMat;
        tIndex wN = 0;
        tVariant wError;
        if (!ReadSquareMatrix(wArgs[0], wMat, wN, wError)) {
            return(tStackElem(wError));
        }
        std::vector<tDouble> wInv;
        if (!MatrixInverse(std::move(wMat), wN, wInv)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tArrayValue* wOut = new tArrayValue(wN, wN);
        for (tIndex r = 0; r < wN; ++r) {
            for (tIndex c = 0; c < wN; ++c) {
                const tDouble wV = wInv[static_cast<size_t>(r) * wN + c];
                if (!std::isfinite(wV)) {
                    delete wOut;
                    return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
                }
                wOut->At(r, c) = MakeNumberVariant(wV);
            }
        }
        return(tStackElem(wOut));
    }

    // TOCOL / TOROW ==========================================================
    tFunctionToColRow::tFunctionToColRow(tBool sToCol) : tFunction(), m_ToCol(sToCol) {}

    tStackElem tFunctionToColRow::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // TOCOL/TOROW(array, [ignore], [scan_by_col])
        if (wArgs.size() < 1 || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tInt wIgnore = 0;
        if (wArgs.size() >= 2) {
            if (!StackElemToInt(wArgs[1], wIgnore) || wIgnore < 0 || wIgnore > 3) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        tBool wScanByCol = false;
        if (wArgs.size() >= 3) {
            tBool wV = false;
            if (!StackElemToBool(wArgs[2], wV)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wScanByCol = wV;
        }

        const tBool wSkipBlank = (wIgnore == 1 || wIgnore == 3);
        const tBool wSkipError = (wIgnore == 2 || wIgnore == 3);

        std::vector<tVariant> wFlat;
        wFlat.reserve(static_cast<size_t>(wSource.Count()));
        auto wPush = [&](const tVariant& sVal) {
            if (wSkipBlank && (sVal.IsNull() || (sVal.IsString() && sVal.String().empty()))) return;
            if (wSkipError && sVal.IsError()) return;
            wFlat.push_back(sVal);
        };

        if (wScanByCol) {
            for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                for (tIndex r = 0; r < wSource.m_Rows; ++r) {
                    wPush(wSource.At(r, c));
                }
            }
        } else {
            for (tIndex r = 0; r < wSource.m_Rows; ++r) {
                for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                    wPush(wSource.At(r, c));
                }
            }
        }

        if (wFlat.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        tArrayValue* wOut = nullptr;
        if (m_ToCol) {
            wOut = new tArrayValue(static_cast<tIndex>(wFlat.size()), 1);
            for (tSize i = 0; i < wFlat.size(); ++i) {
                wOut->At(static_cast<tIndex>(i), 0) = wFlat[i];
            }
        } else {
            wOut = new tArrayValue(1, static_cast<tIndex>(wFlat.size()));
            for (tSize i = 0; i < wFlat.size(); ++i) {
                wOut->At(0, static_cast<tIndex>(i)) = wFlat[i];
            }
        }
        return(tStackElem(wOut));
    }

    // CHOOSECOLS / CHOOSEROWS ================================================
    tFunctionChooseDim::tFunctionChooseDim(tBool sByCol) : tFunction(), m_ByCol(sByCol) {}

    tStackElem tFunctionChooseDim::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // CHOOSECOLS(array, col1, [col2], ...) / CHOOSEROWS(array, row1, ...)
        if (wArgs.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        const tInt wDim = m_ByCol ? static_cast<tInt>(wSource.m_Cols)
                                  : static_cast<tInt>(wSource.m_Rows);
        std::vector<tIndex> wPicks;
        wPicks.reserve(wArgs.size() - 1);
        for (tSize i = 1; i < wArgs.size(); ++i) {
            tInt wIdx = 0;
            if (!StackElemToInt(wArgs[i], wIdx) || wIdx == 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            // Negative: count from the end (-1 = last).
            if (wIdx < 0) {
                wIdx = wDim + wIdx + 1;
            }
            if (wIdx < 1 || wIdx > wDim) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wPicks.push_back(static_cast<tIndex>(wIdx - 1));
        }

        tArrayValue* wOut = nullptr;
        if (m_ByCol) {
            wOut = new tArrayValue(wSource.m_Rows, static_cast<tIndex>(wPicks.size()));
            for (tSize k = 0; k < wPicks.size(); ++k) {
                for (tIndex r = 0; r < wSource.m_Rows; ++r) {
                    wOut->At(r, static_cast<tIndex>(k)) = wSource.At(r, wPicks[k]);
                }
            }
        } else {
            wOut = new tArrayValue(static_cast<tIndex>(wPicks.size()), wSource.m_Cols);
            for (tSize k = 0; k < wPicks.size(); ++k) {
                for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                    wOut->At(static_cast<tIndex>(k), c) = wSource.At(wPicks[k], c);
                }
            }
        }
        return(tStackElem(wOut));
    }

    // EXPAND =================================================================
    tFunctionExpand::tFunctionExpand() {}

    tStackElem tFunctionExpand::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // EXPAND(array, rows, [columns], [pad_with])
        if (wArgs.size() < 2 || wArgs.size() > 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        const tInt wSrcRows = static_cast<tInt>(wSource.m_Rows);
        const tInt wSrcCols = static_cast<tInt>(wSource.m_Cols);

        // Omitted args become 0 via PushEmptyFunctionArg; treat non-positive as "keep source".
        tInt wOutRows = wSrcRows;
        tInt wRowsArg = 0;
        if (!StackElemToInt(wArgs[1], wRowsArg)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wRowsArg >= 1) {
            wOutRows = wRowsArg;
        }

        tInt wOutCols = wSrcCols;
        if (wArgs.size() >= 3) {
            tInt wColsArg = 0;
            if (!StackElemToInt(wArgs[2], wColsArg)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (wColsArg >= 1) {
                wOutCols = wColsArg;
            }
        }

        // Excel: cannot shrink either dimension.
        if (wOutRows < wSrcRows || wOutCols < wSrcCols) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tVariant wPad(tClassError(tTypeError::t_na, ""));
        if (wArgs.size() >= 4) {
            if (!StackElemToVariant(wArgs[3], wPad)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        tArrayValue* wOut = new tArrayValue(static_cast<tIndex>(wOutRows),
                                            static_cast<tIndex>(wOutCols));
        for (tInt r = 0; r < wOutRows; ++r) {
            for (tInt c = 0; c < wOutCols; ++c) {
                if (r < wSrcRows && c < wSrcCols) {
                    wOut->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) =
                        wSource.At(static_cast<tIndex>(r), static_cast<tIndex>(c));
                } else {
                    wOut->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) = wPad;
                }
            }
        }
        return(tStackElem(wOut));
    }

    // WRAPROWS / WRAPCOLS ====================================================
    tFunctionWrap::tFunctionWrap(tBool sByRow) : tFunction(), m_ByRow(sByRow) {}

    tStackElem tFunctionWrap::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // WRAPROWS/WRAPCOLS(vector, wrap_count, [pad_with])
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        // Excel requires a row or column vector.
        if (wSource.m_Rows != 1 && wSource.m_Cols != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tInt wWrap = 0;
        if (!StackElemToInt(wArgs[1], wWrap) || wWrap < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }

        tVariant wPad(tClassError(tTypeError::t_na, ""));
        if (wArgs.size() == 3) {
            if (!StackElemToVariant(wArgs[2], wPad)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        std::vector<tVariant> wElems;
        wElems.reserve(static_cast<size_t>(wSource.Count()));
        if (wSource.m_Rows == 1) {
            for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                wElems.push_back(wSource.At(0, c));
            }
        } else {
            for (tIndex r = 0; r < wSource.m_Rows; ++r) {
                wElems.push_back(wSource.At(r, 0));
            }
        }
        const tInt wN = static_cast<tInt>(wElems.size());

        tIndex wOutRows = 0;
        tIndex wOutCols = 0;
        if (m_ByRow) {
            // WRAPROWS: wrap_count columns; one row when wrap_count >= N.
            wOutCols = static_cast<tIndex>(wWrap);
            wOutRows = static_cast<tIndex>((wN + wWrap - 1) / wWrap);
            if (wWrap >= wN) {
                wOutRows = 1;
                wOutCols = static_cast<tIndex>(wN);
            }
        } else {
            // WRAPCOLS: wrap_count rows; one column when wrap_count >= N.
            wOutRows = static_cast<tIndex>(wWrap);
            wOutCols = static_cast<tIndex>((wN + wWrap - 1) / wWrap);
            if (wWrap >= wN) {
                wOutRows = static_cast<tIndex>(wN);
                wOutCols = 1;
            }
        }

        tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
        tInt wIdx = 0;
        if (m_ByRow) {
            for (tIndex r = 0; r < wOutRows; ++r) {
                for (tIndex c = 0; c < wOutCols; ++c) {
                    wOut->At(r, c) = (wIdx < wN) ? wElems[static_cast<size_t>(wIdx++)] : wPad;
                }
            }
        } else {
            for (tIndex c = 0; c < wOutCols; ++c) {
                for (tIndex r = 0; r < wOutRows; ++r) {
                    wOut->At(r, c) = (wIdx < wN) ? wElems[static_cast<size_t>(wIdx++)] : wPad;
                }
            }
        }
        return(tStackElem(wOut));
    }

    // TAKE / DROP ============================================================
    tFunctionTakeDrop::tFunctionTakeDrop(tBool sDrop) : tFunction(), m_Drop(sDrop) {}

    tStackElem tFunctionTakeDrop::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // TAKE(array, rows, [cols]) / DROP(array, [rows], [cols])
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tInt wRowsArg = 0;
        if (!StackElemToInt(wArgs[1], wRowsArg)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tBool wHasCols = (wArgs.size() == 3);
        // TAKE defaults omitted cols to "all columns"; DROP defaults omitted to 0 (drop none).
        tInt wColsArg = m_Drop ? 0 : static_cast<tInt>(wSource.m_Cols);
        if (wHasCols) {
            if (!StackElemToInt(wArgs[2], wColsArg)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }

        const tInt wSrcRows = static_cast<tInt>(wSource.m_Rows);
        const tInt wSrcCols = static_cast<tInt>(wSource.m_Cols);
        tInt wRowStart = 0;
        tInt wColStart = 0;
        tInt wOutRows = 0;
        tInt wOutCols = 0;

        if (!m_Drop) {
            // TAKE: keep first/last N (clamp); 0 -> #CALC!
            if (wRowsArg == 0 || (wHasCols && wColsArg == 0)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
            }
            wOutRows = wRowsArg;
            if (wOutRows < 0) {
                wOutRows = -wOutRows;
                if (wOutRows > wSrcRows) wOutRows = wSrcRows;
                wRowStart = wSrcRows - wOutRows;
            } else if (wOutRows > wSrcRows) {
                wOutRows = wSrcRows;
            }
            wOutCols = wHasCols ? wColsArg : wSrcCols;
            if (wHasCols) {
                if (wOutCols < 0) {
                    wOutCols = -wOutCols;
                    if (wOutCols > wSrcCols) wOutCols = wSrcCols;
                    wColStart = wSrcCols - wOutCols;
                } else if (wOutCols > wSrcCols) {
                    wOutCols = wSrcCols;
                }
            }
        } else {
            // DROP: exclude first/last N; oversize / empty keep -> #CALC!
            wOutRows = wSrcRows;
            if (wRowsArg != 0) {
                tInt wDropRows = (wRowsArg < 0) ? -wRowsArg : wRowsArg;
                if (wDropRows >= wSrcRows) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
                }
                wOutRows = wSrcRows - wDropRows;
                wRowStart = (wRowsArg > 0) ? wDropRows : 0;
            }
            wOutCols = wSrcCols;
            if (wHasCols && wColsArg != 0) {
                tInt wDropCols = (wColsArg < 0) ? -wColsArg : wColsArg;
                if (wDropCols >= wSrcCols) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
                }
                wOutCols = wSrcCols - wDropCols;
                wColStart = (wColsArg > 0) ? wDropCols : 0;
            }
        }

        if (wOutRows < 1 || wOutCols < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        tArrayValue* wOut = new tArrayValue(static_cast<tIndex>(wOutRows), static_cast<tIndex>(wOutCols));
        for (tInt r = 0; r < wOutRows; ++r) {
            for (tInt c = 0; c < wOutCols; ++c) {
                wOut->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) =
                    wSource.At(static_cast<tIndex>(wRowStart + r), static_cast<tIndex>(wColStart + c));
            }
        }
        return(tStackElem(wOut));
    }

    // HSTACK / VSTACK ========================================================
    tFunctionStack::tFunctionStack(tBool sHorizontal) : tFunction(), m_Horizontal(sHorizontal) {}

    tStackElem tFunctionStack::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        std::vector<tArrayValue> wArrays;
        wArrays.reserve(wArgs.size());
        tIndex wOutRows = 0;
        tIndex wOutCols = 0;
        for (const tStackElem& wArg : wArgs) {
            tArrayValue wArr;
            if (!StackElemToArray(wArg, wArr) || wArr.Count() <= 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (m_Horizontal) {
                if (wArr.m_Rows > wOutRows) wOutRows = wArr.m_Rows;
                wOutCols += wArr.m_Cols;
            } else {
                if (wArr.m_Cols > wOutCols) wOutCols = wArr.m_Cols;
                wOutRows += wArr.m_Rows;
            }
            wArrays.push_back(std::move(wArr));
        }
        if (wOutRows < 1 || wOutCols < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_calc, ""))));
        }

        const tVariant wNa(tClassError(tTypeError::t_na, ""));
        tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
        if (m_Horizontal) {
            tIndex wColOffset = 0;
            for (const tArrayValue& wArr : wArrays) {
                for (tIndex r = 0; r < wOutRows; ++r) {
                    for (tIndex c = 0; c < wArr.m_Cols; ++c) {
                        wOut->At(r, wColOffset + c) =
                            (r < wArr.m_Rows) ? wArr.At(r, c) : wNa;
                    }
                }
                wColOffset += wArr.m_Cols;
            }
        } else {
            tIndex wRowOffset = 0;
            for (const tArrayValue& wArr : wArrays) {
                for (tIndex r = 0; r < wArr.m_Rows; ++r) {
                    for (tIndex c = 0; c < wOutCols; ++c) {
                        wOut->At(wRowOffset + r, c) =
                            (c < wArr.m_Cols) ? wArr.At(r, c) : wNa;
                    }
                }
                wRowOffset += wArr.m_Rows;
            }
        }
        return(tStackElem(wOut));
    }

    // SORTBY =================================================================
    tFunctionSortBy::tFunctionSortBy() {}

    tStackElem tFunctionSortBy::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // SORTBY(array, by_array1, [sort_order1], [by_array2, sort_order2], ...)
        if (wArgs.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tArrayValue wSource;
        if (!StackElemToArray(wArgs[0], wSource) || wSource.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        struct tSortKey {
            tArrayValue m_Keys; // flattened to length == source rows
            tBool m_Descending = false;
        };
        std::vector<tSortKey> wSortKeys;

        size_t wI = 1;
        while (wI < wArgs.size()) {
            tArrayValue wBy;
            if (!StackElemToArray(wArgs[wI], wBy) || wBy.Count() <= 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            ++wI;

            // by_array must be a vector whose length equals the number of rows.
            const tBool wIsCol = (wBy.m_Cols == 1);
            const tBool wIsRow = (wBy.m_Rows == 1);
            if (!wIsCol && !wIsRow) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SORTBY: by_array must be a single row or column"))));
            }
            const tIndex wKeyLen = wIsCol ? wBy.m_Rows : wBy.m_Cols;
            if (wKeyLen != wSource.m_Rows) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SORTBY: by_array length must match array rows"))));
            }

            tSortKey wKey;
            wKey.m_Keys = tArrayValue(wKeyLen, 1);
            for (tIndex k = 0; k < wKeyLen; ++k) {
                wKey.m_Keys.At(k, 0) = wIsCol ? wBy.At(k, 0) : wBy.At(0, k);
            }

            // Optional sort_order: scalar 1 / -1 (0 from omitted arg => ascending).
            // A Range/Array argument starts the next by_array pair instead.
            if (wI < wArgs.size()) {
                const tStackType wNextType = wArgs[wI].Type();
                if (wNextType != tStackType::t_Range && wNextType != tStackType::t_Array) {
                    tInt wOrder = 1;
                    if (StackElemToInt(wArgs[wI], wOrder)) {
                        if (wOrder != 0 && wOrder != 1 && wOrder != -1) {
                            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                        }
                        wKey.m_Descending = (wOrder < 0);
                        ++wI;
                    }
                }
            }
            wSortKeys.push_back(std::move(wKey));
        }

        if (wSortKeys.empty()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        auto wLess = [&](tIndex sA, tIndex sB) -> bool {
            for (const tSortKey& wKey : wSortKeys) {
                const tVariant& wVa = wKey.m_Keys.At(sA, 0);
                const tVariant& wVb = wKey.m_Keys.At(sB, 0);
                if (wVa < wVb) return(!wKey.m_Descending);
                if (wVb < wVa) return(wKey.m_Descending);
            }
            return(false);
        };

        std::vector<tIndex> wOrder(static_cast<size_t>(wSource.m_Rows));
        for (tIndex i = 0; i < wSource.m_Rows; ++i) wOrder[i] = i;
        std::stable_sort(wOrder.begin(), wOrder.end(), wLess);

        tArrayValue* wOut = new tArrayValue(wSource.m_Rows, wSource.m_Cols);
        for (tIndex r = 0; r < wSource.m_Rows; ++r) {
            for (tIndex c = 0; c < wSource.m_Cols; ++c) {
                wOut->At(r, c) = wSource.At(wOrder[r], c);
            }
        }
        return(tStackElem(wOut));
    }

}; // end of namespace
