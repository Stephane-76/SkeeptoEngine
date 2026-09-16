//=============================================================================
// SkSpreadSheet Function SpreadSheet
//=============================================================================
#include "../include/SkFunctionSpreadSheet.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkTools.hpp"
#include "../include/SkLexerSpreadSheet.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSheet.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#define _sumifdebug

namespace SkSpreadSheet {

    namespace {

    tCell* NamedFormulaCallerFromItem(tItem* sItem) {
        if (sItem == nullptr) {
            return nullptr;
        }
        tSheet* wSheet = sItem->Sheet();
        if (wSheet == nullptr || wSheet->WorkBook() == nullptr) {
            return nullptr;
        }
        return wSheet->WorkBook()->NamedFormulaCaller();
    }

    // Resolve OFFSET first argument (single cell or range) to sheet grid and dimensions.
    static tBool OffsetParseRef(const tStackElem& sElem, tColRowCellRange*& outCr, long& outTop, long& outLeft, long& outH, long& outW) {
        if (sElem.Type() == tStackType::t_Range) {
            tRange* wR = sElem.Range();
            if (wR == nullptr) {
                return false;
            }
            outCr = wR->ColRowCellRange();
            if (outCr == nullptr) {
                return false;
            }
            outTop = static_cast<long>(wR->TopIndex());
            outLeft = static_cast<long>(wR->LeftIndex());
            outH = static_cast<long>(wR->BottomIndex() - wR->TopIndex() + 1);
            outW = static_cast<long>(wR->RightIndex() - wR->LeftIndex() + 1);
            return true;
        }
        if (sElem.Type() == tStackType::t_Cell) {
            tCell* wC = sElem.Cell();
            if (wC == nullptr) {
                return false;
            }
            outCr = wC->ColRowCellRange();
            if (outCr == nullptr) {
                return false;
            }
            outTop = static_cast<long>(wC->RowIndex());
            outLeft = static_cast<long>(wC->ColIndex());
            outH = 1;
            outW = 1;
            return true;
        }
        return false;
    }

    // Shared VLOOKUP/HLOOKUP argument parsing and lookup helpers (Excel-compatible).
    static tBool LookupParseIntArg(const tStackElem& sArg, tInt& oOut, tVariant* oError = nullptr) {
        switch (sArg.Type()) {
        case tStackType::t_Variant: {
            tVariant wVal = sArg.Variant();
            if (wVal.IsError()) {
                if (oError != nullptr) {
                    *oError = wVal;
                }
                return false;
            }
            if (wVal.IsDouble()) {
                oOut = static_cast<tInt>(wVal.Double());
                return true;
            }
            if (wVal.IsInt()) {
                oOut = wVal.Int();
                return true;
            }
            return false;
        }
        case tStackType::t_Cell: {
            tCell* wCell = sArg.Cell();
            if (wCell == nullptr) {
                return false;
            }
            tVariant wVal = wCell->Value();
            if (wVal.IsInt()) {
                oOut = wVal.Int();
                return true;
            }
            if (wVal.IsDouble()) {
                oOut = static_cast<tInt>(wVal.Double());
                return true;
            }
            return false;
        }
        case tStackType::t_Range: {
            tRange* wRange = sArg.Range();
            if (wRange == nullptr) {
                return false;
            }
            tCell* wCell = wRange->EnsureCell();
            if (wCell == nullptr) {
                return false;
            }
            tVariant wVal = wCell->Value();
            if (wVal.IsInt()) {
                oOut = wVal.Int();
                return true;
            }
            if (wVal.IsDouble()) {
                oOut = static_cast<tInt>(wVal.Double());
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }

    static tBool LookupParseSearchValue(const tStackElem& sArg, tVariant& oOut) {
        switch (sArg.Type()) {
        case tStackType::t_Variant:
            oOut = sArg.Variant();
            return true;
        case tStackType::t_Cell: {
            tCell* wCell = sArg.Cell();
            if (wCell == nullptr) {
                return false;
            }
            oOut = wCell->Value();
            return true;
        }
        case tStackType::t_Range: {
            tRange* wRange = sArg.Range();
            if (wRange == nullptr) {
                return false;
            }
            tCell* wCell = wRange->EnsureCell();
            if (wCell == nullptr) {
                return false;
            }
            oOut = wCell->Value();
            return true;
        }
        default:
            return false;
        }
    }

    // Excel range_lookup: omitted or TRUE -> approximate; FALSE/0 -> exact.
    static tBool LookupParseRangeLookup(const tStackElem* sArg, tBool sHasFourthArg, tBool& oExactMatch, tVariant* oError = nullptr) {
        if (!sHasFourthArg) {
            oExactMatch = false;
            return true;
        }
        if (sArg == nullptr) {
            return false;
        }
        tVariant wRangeLookup;
        if (!LookupParseSearchValue(*sArg, wRangeLookup)) {
            return false;
        }
        if (wRangeLookup.IsError()) {
            if (oError != nullptr) {
                *oError = wRangeLookup;
            }
            return false;
        }
        if (!wRangeLookup.IsBool() && !wRangeLookup.IsInt() && !wRangeLookup.IsDouble()) {
            return false;
        }
        if (wRangeLookup.IsBool()) {
            oExactMatch = !wRangeLookup.Bool();
        } else if (wRangeLookup.IsInt()) {
            oExactMatch = (wRangeLookup.Int() == 0);
        } else {
            oExactMatch = (wRangeLookup.Double() == 0.0);
        }
        return true;
    }

    static tIndex VLookupFindRowExact(tColRowCellRange* sCr, tRange* sRange, const tVariant& sSearch) {
        const tIndex wSearchCol = sRange->LeftIndex();
        for (tIndex wRow = sRange->TopIndex(); wRow <= sRange->IterateBottom(); ++wRow) {
            tCell* wCell = sCr->Cell(wRow, wSearchCol);
            if (wCell != nullptr && wCell->CalculableValue() == sSearch) {
                return wRow;
            }
        }
        return 0;
    }

    // Approximate match: first column sorted ascending; largest key <= lookup (Excel range_lookup TRUE).
    static tIndex VLookupFindRowApprox(tColRowCellRange* sCr, tRange* sRange, const tVariant& sSearch) {
        const tIndex wSearchCol = sRange->LeftIndex();
        tIndex wBestRow = 0;
        for (tIndex wRow = sRange->TopIndex(); wRow <= sRange->IterateBottom(); ++wRow) {
            tCell* wCell = sCr->Cell(wRow, wSearchCol);
            if (wCell == nullptr) {
                continue;
            }
            const tVariant wVal = wCell->CalculableValue();
            if (wVal.IsError()) {
                continue;
            }
            if (wVal <= sSearch) {
                wBestRow = wRow;
            } else {
                break;
            }
        }
        return wBestRow;
    }

    static tIndex HLookupFindColExact(tColRowCellRange* sCr, tRange* sRange, const tVariant& sSearch) {
        const tIndex wSearchRow = sRange->TopIndex();
        for (tIndex wCol = sRange->LeftIndex(); wCol <= sRange->IterateRight(); ++wCol) {
            tCell* wCell = sCr->Cell(wSearchRow, wCol);
            if (wCell != nullptr && wCell->CalculableValue() == sSearch) {
                return wCol;
            }
        }
        return 0;
    }

    static tIndex HLookupFindColApprox(tColRowCellRange* sCr, tRange* sRange, const tVariant& sSearch) {
        const tIndex wSearchRow = sRange->TopIndex();
        tIndex wBestCol = 0;
        for (tIndex wCol = sRange->LeftIndex(); wCol <= sRange->IterateRight(); ++wCol) {
            tCell* wCell = sCr->Cell(wSearchRow, wCol);
            if (wCell == nullptr) {
                continue;
            }
            const tVariant wVal = wCell->CalculableValue();
            if (wVal.IsError()) {
                continue;
            }
            if (wVal <= sSearch) {
                wBestCol = wCol;
            } else {
                break;
            }
        }
        return wBestCol;
    }

    static tStackElem VLookupReturnFromRow(tColRowCellRange* sCr, tRange* sRange, tIndex sRow, tInt sColIndex) {
        if (sColIndex < 1) {
            return tStackElem(tVariant(tClassError(tTypeError::t_value, "")));
        }
        const tIndex wNumCols = sRange->RightIndex() - sRange->LeftIndex() + 1;
        if (static_cast<tIndex>(sColIndex) > wNumCols) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }
        const tIndex wReturnCol = sRange->LeftIndex() + static_cast<tIndex>(sColIndex) - 1;
        tCell* wCellReturn = sCr->Cell(sRow, wReturnCol);
        if (wCellReturn != nullptr) {
            tVariant wReturn = wCellReturn->CalculableValue();
            if (wReturn.IsError()) {
                return tStackElem(wReturn);
            }
            return tStackElem(wReturn);
        }
        return tStackElem(tVariant());
    }

    static tStackElem HLookupReturnFromCol(tColRowCellRange* sCr, tRange* sRange, tIndex sCol, tInt sRowIndex) {
        if (sRowIndex < 1) {
            return tStackElem(tVariant(tClassError(tTypeError::t_value, "")));
        }
        const tIndex wNumRows = sRange->BottomIndex() - sRange->TopIndex() + 1;
        if (static_cast<tIndex>(sRowIndex) > wNumRows) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }
        const tIndex wReturnRow = sRange->TopIndex() + static_cast<tIndex>(sRowIndex) - 1;
        tCell* wCellReturn = sCr->Cell(wReturnRow, sCol);
        if (wCellReturn != nullptr) {
            tVariant wReturn = wCellReturn->CalculableValue();
            if (wReturn.IsError()) {
                return tStackElem(wReturn);
            }
            return tStackElem(wReturn);
        }
        return tStackElem(tVariant());
    }

    // Approximate match in a single-row or single-column vector (sorted ascending):
    // largest key <= search. Returns 0-based index, or npos if none.
    static tIndex FindApproxInLookupVector(const tArrayValue& sLookup,
                                           const tVariant& sSearch,
                                           tBool sLookupIsCol) {
        const tIndex wLen = sLookupIsCol ? sLookup.m_Rows : sLookup.m_Cols;
        tIndex wBest = static_cast<tIndex>(-1);
        for (tIndex i = 0; i < wLen; ++i) {
            const tVariant& wKey = sLookupIsCol ? sLookup.At(i, 0) : sLookup.At(0, i);
            if (wKey.IsError()) {
                continue;
            }
            if (wKey <= sSearch) {
                wBest = i;
            } else {
                break;
            }
        }
        return wBest;
    }

    // Exact match in a single-row or single-column lookup array.
    // search_mode 1 = first-to-last, -1 = last-to-first. Returns 0-based index or npos.
    static tIndex FindExactInLookupVector(const tArrayValue& sLookup,
                                          const tVariant& sSearch,
                                          tBool sLookupIsCol,
                                          tInt sSearchMode) {
        const tIndex wLen = sLookupIsCol ? sLookup.m_Rows : sLookup.m_Cols;
        if (sSearchMode == 1) {
            for (tIndex i = 0; i < wLen; ++i) {
                const tVariant& wKey = sLookupIsCol ? sLookup.At(i, 0) : sLookup.At(0, i);
                if (wKey == sSearch) {
                    return i;
                }
            }
        } else {
            for (tIndex i = wLen; i-- > 0; ) {
                const tVariant& wKey = sLookupIsCol ? sLookup.At(i, 0) : sLookup.At(0, i);
                if (wKey == sSearch) {
                    return i;
                }
            }
        }
        return static_cast<tIndex>(-1);
    }

    // Build A1 or A1:B2 ref text from a formula range/cell argument.
    static tBool StackElemToRangeRefString(const tStackElem& sElem, tString& oRef) {
        oRef.clear();
        switch (sElem.Type()) {
        case tStackType::t_Cell: {
            tCell* const wCell = sElem.Cell();
            if (wCell == nullptr) {
                return false;
            }
            oRef = wCell->StrRef(false);
            return !oRef.empty();
        }
        case tStackType::t_Range: {
            tRange* const wRange = sElem.Range();
            if (wRange == nullptr) {
                return false;
            }
            if (wRange->IsCell()) {
                tCell* const wCell = wRange->Cell();
                if (wCell == nullptr) {
                    return false;
                }
                oRef = wCell->StrRef(false);
            } else {
                oRef = wRange->StrRef(false);
            }
            return !oRef.empty();
        }
        default:
            return false;
        }
    }

    static void JsonAppendEscapedString(tStringStream& sStream, const tString& sText) {
        sStream << "\"";
        for (const tChar wChar : sText) {
            if (wChar == '\\' || wChar == '"') {
                sStream << '\\';
            }
            sStream << wChar;
        }
        sStream << "\"";
    }

    static tString JsonArrayFromRangeRefs(const std::vector<tString>& sRefs) {
        tStringStream wStream;
        wStream << "[";
        for (size_t wIndex = 0; wIndex < sRefs.size(); ++wIndex) {
            if (wIndex > 0) {
                wStream << ",";
            }
            JsonAppendEscapedString(wStream, sRefs[wIndex]);
        }
        wStream << "]";
        return wStream.str();
    }

    static tString CellCalculableString(tCell* sCell) {
        if (sCell == nullptr) {
            return "";
        }
        const tVariant wValue = sCell->CalculableValue();
        if (wValue.IsNull()) {
            return "";
        }
        return wValue.Str();
    }

    // Append JSON array items from one range/cell argument (row-major).
    static tBool StackElemAppendJsonItems(const tStackElem& sElem, tStringStream& oStream, tBool& ioFirst) {
        auto wAppendScalar = [&](const tString& sText) {
            if (sText.empty()) {
                return;
            }
            if (!ioFirst) {
                oStream << ",";
            }
            ioFirst = false;
            JsonAppendEscapedString(oStream, sText);
        };

        auto wAppendPair = [&](const tString& sValue, const tString& sLabel) {
            if (sValue.empty() && sLabel.empty()) {
                return;
            }
            if (!ioFirst) {
                oStream << ",";
            }
            ioFirst = false;
            oStream << "{\"value\":";
            JsonAppendEscapedString(oStream, sValue);
            oStream << ",\"label\":";
            JsonAppendEscapedString(oStream, sLabel);
            oStream << "}";
        };

        if (sElem.Type() == tStackType::t_Cell) {
            wAppendScalar(CellCalculableString(sElem.Cell()));
            return true;
        }
        if (sElem.Type() != tStackType::t_Range) {
            return false;
        }

        tRange* const wRange = sElem.Range();
        if (wRange == nullptr) {
            return false;
        }
        if (wRange->IsCell()) {
            wAppendScalar(CellCalculableString(wRange->Cell()));
            return true;
        }

        tColRowCellRange* const wColRowCellRange = wRange->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return false;
        }

        const tIndex wLeft = wRange->LeftIndex();
        const tIndex wRight = wRange->IterateRight();
        const tIndex wTop = wRange->TopIndex();
        const tIndex wBottom = wRange->IterateBottom();
        const tIndex wWidth = wRight - wLeft + 1;

        if (wWidth == 2) {
            for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
                const tString wValue = CellCalculableString(wColRowCellRange->Cell(wRow, wLeft));
                const tString wLabel = CellCalculableString(wColRowCellRange->Cell(wRow, wLeft + 1));
                wAppendPair(wValue, wLabel);
            }
            return true;
        }

        for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
            for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                wAppendScalar(CellCalculableString(wColRowCellRange->Cell(wRow, wCol)));
            }
        }
        return true;
    }

    // Excel SUMIF/SUMIFS sum_range semantics: add numeric values; ignore text; propagate errors.
    static tBool SumIfAccumulate(tVariant& ioSum, const tVariant& sAddend) {
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
        ioSum = ioSum + sAddend;
        return !ioSum.IsError();
    }

    // Excel MAXIFS/MINIFS value_range: ignore text/blank/logicals; keep numeric/date extremum;
    // propagate errors. Returns false when an error should stop the scan.
    static tBool MaxMinIfAccumulate(tVariant& ioVal, tBool& ioHas, const tVariant& sAddend, tBool sIsMax) {
        if (sAddend.IsError()) {
            ioVal = sAddend;
            return false;
        }
        if (sAddend.IsString() || sAddend.IsNull()) {
            return true;
        }
        if (!(sAddend.IsNumeric() || sAddend.IsDate())) {
            return true;
        }
        if (!ioHas) {
            ioVal = sAddend;
            ioHas = true;
            return true;
        }
        if (sIsMax) {
            if (sAddend > ioVal) ioVal = sAddend;
        } else {
            if (sAddend < ioVal) ioVal = sAddend;
        }
        return true;
    }
    } // namespace

    //=========================================================================
    //! Criteria parser for optimized SUMIF/COUNTIF
    struct tCriteriaParser {
        enum class tCriteriaType {
            t_Exact,        // Simple equality
            t_Numeric,      // Numeric comparison (>, <, >=, <=, =)
            t_Wildcard,     // Wildcard pattern (*, ?)
            t_String        // String comparison
        };
        
        tCriteriaType m_Type;
        tString m_Operator;     // For numeric: ">", "<", ">=", "<=", "="
        tDouble m_NumericValue; // For numeric comparisons
        tString m_Pattern;      // For wildcards
        tVariant m_OriginalCriteria;
        tBool m_Negate = false; // True when the criteria is a "<>" (not-equal) test

        //=========================================================================
        //! Return true when the pattern must be handled by the wildcard matcher:
        //! it contains a '*' or '?' wildcard, or a '~' escape sequence (~*, ~?, ~~).
        //! A lone '~' is harmless (matched literally by MatchWildcard), so we route
        //! any pattern containing '~' through the matcher to honour the escapes.
        static tBool HasWildcardOrEscape(const tString& sPattern) {
            return(sPattern.find('*') != std::string::npos ||
                   sPattern.find('?') != std::string::npos ||
                   sPattern.find('~') != std::string::npos);
        }

        //=========================================================================
        //! Non-throwing numeric parse. Returns true (and sets sOut) when the string
        //! starts with a valid number. Emscripten/WASM builds disable C++ exception
        //! catching by default, so std::stod's throw on non-numeric input would abort
        //! the whole process instead of being caught — never rely on exceptions for
        //! control flow here. When sRequireFull is true, the entire string (except
        //! trailing whitespace) must be numeric; otherwise a numeric prefix is enough.
        static tBool TryParseDouble(const tString& sStr, tDouble& sOut, tBool sRequireFull) {
            if (sStr.empty()) {
                return false;
            }
            const tChar* wBegin = sStr.c_str();
            tChar* wEnd = nullptr;
            const tDouble wVal = std::strtod(wBegin, &wEnd);
            if (wEnd == wBegin) {
                return false; // no conversion
            }
            if (sRequireFull) {
                while (*wEnd == ' ' || *wEnd == '\t') {
                    ++wEnd;
                }
                if (*wEnd != '\0') {
                    return false;
                }
            }
            sOut = wVal;
            return true;
        }

        
        //=========================================================================
        //! Helper function to match wildcards (* and ?) like Excel
        tBool MatchWildcard(const tString& sText, const tString& sPattern) const {
            size_t wTextPos = 0;
            size_t wPatternPos = 0;
            size_t wTextLen = sText.length();
            size_t wPatternLen = sPattern.length();
            
            // Handle empty pattern
            if (wPatternLen == 0) {
                return (wTextLen == 0);
            }
            
            // Handle empty text
            if (wTextLen == 0) {
                // Only match if pattern is all * wildcards
                for (size_t i = 0; i < wPatternLen; i++) {
                    if (sPattern[i] != '*') {
                        return false;
                    }
                }
                return true;
            }
            
            // Simple recursive matching algorithm
            while (wTextPos < wTextLen && wPatternPos < wPatternLen) {
                if (sPattern[wPatternPos] == '~' && (wPatternPos + 1) < wPatternLen &&
                    (sPattern[wPatternPos + 1] == '*' || sPattern[wPatternPos + 1] == '?' || sPattern[wPatternPos + 1] == '~')) {
                    // Escaped literal wildcard (~*, ~?) or escaped tilde (~~)
                    if (sText[wTextPos] == sPattern[wPatternPos + 1]) {
                        wTextPos++;
                        wPatternPos += 2;
                    } else {
                        return false;
                    }
                } else if (sPattern[wPatternPos] == '*') {
                    // Skip consecutive * wildcards
                    while (wPatternPos < wPatternLen && sPattern[wPatternPos] == '*') {
                        wPatternPos++;
                    }
                    
                    // If we've reached the end of pattern, match everything
                    if (wPatternPos >= wPatternLen) {
                        return true;
                    }
                    
                    // Try to match the rest of the pattern from current text position
                    for (size_t i = wTextPos; i <= wTextLen; i++) {
                        if (MatchWildcard(sText.substr(i), sPattern.substr(wPatternPos))) {
                            return true;
                        }
                    }
                    return false;
                } else if (sPattern[wPatternPos] == '?') {
                    // ? matches any single character
                    wTextPos++;
                    wPatternPos++;
                } else if (sText[wTextPos] == sPattern[wPatternPos]) {
                    // Exact character match
                    wTextPos++;
                    wPatternPos++;
                } else {
                    // No match
                    return false;
                }
            }
            
            // If we've reached the end of both text and pattern, it's a match
            return (wTextPos >= wTextLen && wPatternPos >= wPatternLen);
        }

        
        tCriteriaParser(tVariant& sCriteria) : m_OriginalCriteria(sCriteria) {
            // Try to get string representation first (works for both string and numeric types)
            tString wCriteriaStr;
            bool wIsStringType = false;
            
            if (sCriteria.IsString()) {
                wCriteriaStr = sCriteria.String();
                wIsStringType = true;
            } else {
                // Convert to string for pattern matching
                wCriteriaStr = sCriteria.Str();
            }
            
            // Detect a leading comparison operator: >=, <=, <>, >, <, =
            tString wOp;
            if (wCriteriaStr.length() >= 2 &&
                (wCriteriaStr.substr(0, 2) == ">=" || wCriteriaStr.substr(0, 2) == "<=" ||
                 wCriteriaStr.substr(0, 2) == "<>")) {
                wOp = wCriteriaStr.substr(0, 2);
            } else if (!wCriteriaStr.empty() &&
                       (wCriteriaStr[0] == '>' || wCriteriaStr[0] == '<' || wCriteriaStr[0] == '=')) {
                wOp = wCriteriaStr.substr(0, 1);
            }

            // Ordered numeric comparison: >, <, >=, <= (existing behavior)
            if (wOp == ">" || wOp == "<" || wOp == ">=" || wOp == "<=") {
                m_Type = tCriteriaType::t_Numeric;
                m_Operator = wOp;

                tString wValueStr = wCriteriaStr.substr(m_Operator.length());
                // Trim leading whitespace
                while (!wValueStr.empty() && (wValueStr[0] == ' ' || wValueStr[0] == '\t')) {
                    wValueStr = wValueStr.substr(1);
                }

                // Non-throwing parse (like std::stod, a numeric prefix is enough); fall back to exact match.
                if (!TryParseDouble(wValueStr, m_NumericValue, /*sRequireFull*/ false)) {
                    m_Type = tCriteriaType::t_Exact;
                }
            }
            // Equality / inequality with explicit operator: =X or <>X
            else if (wOp == "=" || wOp == "<>") {
                m_Negate = (wOp == "<>");
                tString wRest = wCriteriaStr.substr(wOp.length());

                if (HasWildcardOrEscape(wRest)) {
                    m_Type = tCriteriaType::t_Wildcard;
                    m_Pattern = wRest;
                } else {
                    // Compare against the stripped value (numeric only if the WHOLE token is a number, else text).
                    tDouble wParsed = 0.0;
                    if (TryParseDouble(wRest, wParsed, /*sRequireFull*/ true)) {
                        m_Type = tCriteriaType::t_Numeric;
                        m_Operator = "=";
                        m_NumericValue = wParsed;
                    } else {
                        m_Type = tCriteriaType::t_Exact;
                        m_OriginalCriteria = tVariant(wRest);
                    }
                }
            }
            // Check for wildcards (only if it's a string type)
            else if (wIsStringType && HasWildcardOrEscape(wCriteriaStr)) {
                m_Type = tCriteriaType::t_Wildcard;
                m_Pattern = wCriteriaStr;
            }
            // Check if numeric exact match
            else if (sCriteria.IsDouble() || sCriteria.IsInt()) {
                m_Type = tCriteriaType::t_Numeric;
                m_Operator = "=";
                m_NumericValue = sCriteria.IsDouble() ? sCriteria.Double() : sCriteria.Int();
            }
            // Default string comparison
            else {
                m_Type = tCriteriaType::t_Exact;
            }
        }
        
        // Optimized match function
        tBool Match(tVariant& sCellValue) const {
            // Create non-const copies to call non-const tVariant helpers safely
            tVariant wCell = sCellValue;
            tVariant wCrit = m_OriginalCriteria;
            switch (m_Type) {
                case tCriteriaType::t_Numeric: {
                    if (!(wCell.IsDouble() || wCell.IsInt())) {
                        // A non-numeric cell is never equal to a number, so "<>" matches it
                        return m_Negate;
                    }
                    tDouble wCellDouble = wCell.IsDouble() ? wCell.Double() : wCell.Int();

                    tBool wResult = false;
                    if (m_Operator == ">") wResult = (wCellDouble > m_NumericValue);
                    else if (m_Operator == "<") wResult = (wCellDouble < m_NumericValue);
                    else if (m_Operator == ">=") wResult = (wCellDouble >= m_NumericValue);
                    else if (m_Operator == "<=") wResult = (wCellDouble <= m_NumericValue);
                    else if (m_Operator == "=") wResult = (wCellDouble == m_NumericValue);
                    return m_Negate ? !wResult : wResult;
                }
                
                case tCriteriaType::t_Wildcard: {
                    tString wCellStr = sCellValue.Str();
                    tBool wResult = MatchWildcard(wCellStr, m_Pattern);
                    return m_Negate ? !wResult : wResult;
                }
                
                case tCriteriaType::t_Exact: {
                    tBool wResult;
                    // Handle numeric comparison properly
                    if ((wCell.IsDouble() || wCell.IsInt()) && 
                        (wCrit.IsDouble() || wCrit.IsInt())) {
                        tDouble wCellDouble = wCell.IsDouble() ? wCell.Double() : wCell.Int();
                        tDouble wCriteriaDouble = wCrit.IsDouble() ? wCrit.Double() : wCrit.Int();
                        wResult = (wCellDouble == wCriteriaDouble);
                    } else if (wCell.IsString() && wCrit.IsString()) {
                        // Create temporary copies to call String()
                        tVariant wTempCell = wCell;
                        tVariant wTempCriteria = wCrit;
                        wResult = (wTempCell.String() == wTempCriteria.String());
                    } else if (wCell.IsString() && (wCrit.IsDouble() || wCrit.IsInt())) {
                        // Try to convert string cell to number for comparison (non-throwing).
                        tVariant wTempCell = wCell;
                        tDouble wCellDouble = 0.0;
                        if (TryParseDouble(wTempCell.String(), wCellDouble, /*sRequireFull*/ false)) {
                            tDouble wCriteriaDouble = wCrit.IsDouble() ? wCrit.Double() : wCrit.Int();
                            wResult = (wCellDouble == wCriteriaDouble);
                        } else {
                            wResult = false;
                        }
                    } else if ((wCell.IsDouble() || wCell.IsInt()) && wCrit.IsString()) {
                        // Try to convert string criteria to number for comparison (non-throwing).
                        tVariant wTempCriteria = wCrit;
                        tDouble wCriteriaDouble = 0.0;
                        if (TryParseDouble(wTempCriteria.String(), wCriteriaDouble, /*sRequireFull*/ false)) {
                            tDouble wCellDouble = wCell.IsDouble() ? wCell.Double() : wCell.Int();
                            wResult = (wCellDouble == wCriteriaDouble);
                        } else {
                            wResult = false;
                        }
                    } else {
                        wResult = (wCell == wCrit);
                    }
                    return m_Negate ? !wResult : wResult;
                }
                
                default:
                    return false;
            }
        }
    };

    //=========================================================================
    //! A single *IF(S) criterion argument, materialized so it can be either a scalar or an
    //! array (Excel dynamic-array behaviour). When the criterion is an in-memory array — e.g.
    //! COUNTIFS(range, TEAM, ...) where TEAM = UNIQUE(...) is a spilled column — the result of
    //! the whole function spills to the array's shape (one aggregate per criterion element).
    struct tCriterionArg {
        tBool m_IsArray = false;
        tIndex m_Rows = 1;
        tIndex m_Cols = 1;
        std::vector<tVariant> m_Values; // row-major; size == m_Rows * m_Cols (>= 1)

        //! Broadcasting access: a size-1 dimension is reused for every output index.
        tVariant At(tIndex sRow, tIndex sCol) const {
            const tIndex wR = (m_Rows == 1) ? 0 : sRow;
            const tIndex wC = (m_Cols == 1) ? 0 : sCol;
            return m_Values[static_cast<size_t>(wR) * static_cast<size_t>(m_Cols) +
                            static_cast<size_t>(wC)];
        }
    };

    //! Materialize a criterion stack element. Scalars (Variant/Cell/single-cell Range) stay 1x1;
    //! an in-memory array (t_Array) or a bounded multi-cell range (D14# spill, D14:D33) becomes
    //! an array criterion so COUNTIFS/SUMIFS spill one aggregate per element (League-Table Part B).
    //! Open-ended whole rows/columns stay a single implicit-intersection value (not a million-row spill).
    static tBool ReadCriterionArg(tStackElem* sArg, tCriterionArg& oOut, tString& oError) {
        switch (sArg->Type()) {
            case tStackType::t_Variant:
                oOut.m_Values.push_back(sArg->Variant());
                return true;
            case tStackType::t_Cell: {
                tCell* wCell = sArg->Cell();
                if (wCell == nullptr) { oError = "Invalid criteria cell"; return false; }
                oOut.m_Values.push_back(wCell->Value());
                return true;
            }
            case tStackType::t_Array: {
                tArrayValue* wArr = sArg->Array();
                if (wArr == nullptr || wArr->Count() <= 0) { oError = "Invalid criteria array"; return false; }
                oOut.m_Rows = wArr->m_Rows;
                oOut.m_Cols = wArr->m_Cols;
                oOut.m_IsArray = (wArr->Count() > 1);
                oOut.m_Values.reserve(static_cast<size_t>(wArr->Count()));
                for (tIndex r = 0; r < wArr->m_Rows; ++r) {
                    for (tIndex c = 0; c < wArr->m_Cols; ++c) {
                        oOut.m_Values.push_back(wArr->At(r, c));
                    }
                }
                return true;
            }
            case tStackType::t_Range: {
                tRange* wR = sArg->Range();
                if (wR == nullptr) { oError = "Invalid criteria range"; return false; }
                const tBool wOpenEnded =
                    wR->BottomIndex() >= Cst_MaxRow || wR->RightIndex() >= Cst_MaxCol;
                if (wR->IsCell() || wOpenEnded) {
                    tCell* wCell = wR->EnsureCell();
                    if (wCell == nullptr) { oError = "Invalid criteria range"; return false; }
                    oOut.m_Values.push_back(wCell->CalculableValue());
                    return true;
                }
                tArrayValue wArr;
                if (!tFunction::StackElemToArray(*sArg, wArr) || wArr.Count() <= 0) {
                    oError = "Invalid criteria range";
                    return false;
                }
                if (wArr.Count() == 1) {
                    oOut.m_Values.push_back(wArr.At(0, 0));
                    return true;
                }
                oOut.m_Rows = wArr.m_Rows;
                oOut.m_Cols = wArr.m_Cols;
                oOut.m_IsArray = true;
                oOut.m_Values.reserve(static_cast<size_t>(wArr.Count()));
                for (tIndex r = 0; r < wArr.m_Rows; ++r) {
                    for (tIndex c = 0; c < wArr.m_Cols; ++c) {
                        oOut.m_Values.push_back(wArr.At(r, c));
                    }
                }
                return true;
            }
            default:
                oError = "Criteria must be a value";
                return false;
        }
    }

    //! Common broadcast shape across every criterion (each dimension must be 1 or the max).
    static tBool BroadcastCriteriaShape(const std::vector<tCriterionArg>& sArgs,
                                        tIndex& oRows, tIndex& oCols) {
        oRows = 1;
        oCols = 1;
        for (const tCriterionArg& wA : sArgs) {
            if (wA.m_Rows > oRows) oRows = wA.m_Rows;
            if (wA.m_Cols > oCols) oCols = wA.m_Cols;
        }
        for (const tCriterionArg& wA : sArgs) {
            if (!((wA.m_Rows == 1 || wA.m_Rows == oRows) &&
                  (wA.m_Cols == 1 || wA.m_Cols == oCols))) {
                return false;
            }
        }
        return true;
    }

    //=========================================================================
    //! Call back for function count
    tCallBackRangeCount::tCallBackRangeCount(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange) {
        m_Value=0;
    }

    tBool tCallBackRangeCount::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell=m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell!=nullptr) {
            if (!wCell->Value().IsNull())
                m_Value=m_Value+1;
        }
        return(true);
    }

    //=========================================================================
    //! Function count
    tFunctionCount::tFunctionCount() : tFunction() {}


    tStackElem tFunctionCount::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue=0;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			// Variant, Range, or Cell
			switch(wArg.Type()) {
				case tStackType::t_Variant: {
					tVariant wVariant=wArg.Variant();
					if (!wVariant.IsNull())  wValue = wValue + 1;
					break;
				}
				case tStackType::t_Attribute:
				case tStackType::t_Cell: {
					tCell* wCell = wArg.Cell();
					if (wCell != nullptr) {
						tVariant wVariant = wCell->Value();
						if (!wVariant.IsNull())  wValue = wValue + 1;
					}
					break;
				}
				case tStackType::t_Range: {
					tRange* wRange = wArg.Range();
					tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
					tCallBackRangeCount wCallBackCount(wColRowCellRange);
					wColRowCellRange->VisitorRange(wRange, &wCallBackCount);
					wValue = wValue + wCallBackCount.Value();
					break;
				}
				default:
					// Unknown type, skip
					break;
			}
			// Clean up handled automatically (no delete needed)
		}
		return(tStackElem(wValue));
    }

    //=========================================================================
    //! Call back for function counta
    tCallBackRangeCountA::tCallBackRangeCountA(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange) {
        m_Value=0;
    }

    tBool tCallBackRangeCountA::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell=m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell!=nullptr) {
            // COUNTA counts all non-empty cells (including text, errors, empty strings)
            // Empty string "" is considered non-empty by COUNTA (Excel behavior)
            tVariant wCellValue = wCell->Value();
            if (wCellValue.Type() == tVariantType::t_string) {
                // Always count strings, even empty ones
                m_Value=m_Value+1;
            } else if (!wCellValue.IsNull()) {
                // Count non-null non-string values
                m_Value=m_Value+1;
            }
        }
        return(true);
    }

    //=========================================================================
    //! Function CountA
    tFunctionCountA::tFunctionCountA() : tFunction() {}

    tStackElem tFunctionCountA::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue=0;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			// Variant, Range, or Cell
			switch(wArg.Type()) {
              case tStackType::t_Variant: {
                tVariant wVariant=wArg.Variant();
                // COUNTA counts all non-empty values (including text, errors, empty strings)
                // Empty string "" is considered non-empty by COUNTA (Excel behavior)
                if (wVariant.Type() == tVariantType::t_string) {
                    // Always count strings, even empty ones
                    wValue = wValue + 1;
                } else if (!wVariant.IsNull()) {
                    // Count non-null non-string values
                    wValue = wValue + 1;
                }
                break;
			  }
              case tStackType::t_Attribute:
              case tStackType::t_Cell: {
                    tCell* wCell = wArg.Cell();
                    if (wCell != nullptr) {
                        tVariant wVariant = wCell->Value();
                        // COUNTA counts all non-empty values (including text, errors, empty strings)
                        // Empty string "" is considered non-empty by COUNTA (Excel behavior)
                        if (wVariant.Type() == tVariantType::t_string) {
                            // Always count strings, even empty ones
                            wValue = wValue + 1;
                        } else if (!wVariant.IsNull()) {
                            // Count non-null non-string values
                            wValue = wValue + 1;
                        }
                    }
                    break;
              }
              case tStackType::t_Range: {
				tRange* wRange = wArg.Range();
				tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
				tCallBackRangeCountA wCallBackCountA(wColRowCellRange);
				wColRowCellRange->VisitorRange(wRange, &wCallBackCountA);
				wValue = wValue + wCallBackCountA.Value();
                break;
                }
              default:
                  // Unknown type, skip
                  break;
            }
			// Clean up handled automatically (no delete needed)
		}
		return(tStackElem(wValue));
    }

    //=========================================================================
    //! Call back for function countblank
    tCallBackRangeCountBlank::tCallBackRangeCountBlank(tColRowCellRange* sColRowCellRange) : tCallBackRangeFunction(sColRowCellRange) {
        m_Value=0;
    }

    tBool tCallBackRangeCountBlank::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell=m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell==nullptr) {
            // Cell doesn't exist = blank
            m_Value=m_Value+1;
        } else {
            // COUNTBLANK counts only truly empty cells (not cells with empty string "")
            // A cell is blank if its variant IsNull()
            tVariant wCellValue = wCell->Value();
            if (wCellValue.IsNull()) {
                // Only count truly null (empty) values as blank
                m_Value=m_Value+1;
            }
        }
        return(true);
    }

    //=========================================================================
    //! Function CountBlank
    tFunctionCountBlank::tFunctionCountBlank() : tFunction() {}

    tStackElem tFunctionCountBlank::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue=0;
		// Process arguments in reverse order (first argument was last on stack)
		for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
			tStackElem& wArg = *it;
			// Variant or Range
			switch(wArg.Type()) {
				case tStackType::t_Variant: {
					tVariant wVariant=wArg.Variant();
					// COUNTBLANK counts only truly empty values
					// A cell is blank if its variant IsNull()
					if (wVariant.IsNull()) {
						// Only count truly null (empty) values as blank
						wValue = wValue + 1;
					}
					break;
				}
				case tStackType::t_Attribute:
				case tStackType::t_Cell: {
					tCell* wCell = wArg.Cell();
					if (wCell != nullptr) {
						tVariant wVariant = wCell->Value();
						if (wVariant.IsNull()) {
							wValue = wValue + 1;
						}
					}
					break;
				}
				case tStackType::t_Range: {
					tRange* wRange = wArg.Range();
					tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
					tCallBackRangeCountBlank wCallBackCountBlank(wColRowCellRange);
					wColRowCellRange->VisitorRange(wRange, &wCallBackCountBlank);
					wValue = wValue + wCallBackCountBlank.Value();
					break;
				}
				default:
					// Unknown type, skip
					break;
			}
			// Clean up handled automatically (no delete needed)
		}
		return(tStackElem(wValue));
    }

    //=========================================================================
    //! Function Index
    tFunctionIndex::tFunctionIndex() : tFunction() {}
    
    tStackElem tFunctionIndex::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        tVectorInt wVectorArg;
        tRange* wRange=nullptr;
        tIndex wRow=1;
        tIndex wCol=1;

        // Excel error propagation: INDEX(range, #N/A, ...) -> #N/A.
        // This is the canonical pattern in real workbooks where a MATCH that
        // does not find its key feeds INDEX:
        //     IFERROR(INDEX(range, MATCH(x, keys, 0), col), NA())
        // StackElemToIndex returns false silently for an error operand and
        // leaves wRow/wCol at their defaults (1), which would produce a bogus
        // first-cell value and bypass the surrounding IFERROR.
        auto wPropagateIfError = [](tStackElem& sArg, tVariant& sOut) -> tBool {
            tVariant wTmp;
            if (StackElemToVariant(sArg, wTmp) && wTmp.IsError()) {
                sOut = wTmp;
                return true;
            }
            return false;
        };

        if (wArgs.size() == 2) {
            // INDEX(range, row_num): In RPN last pushed = top = row_num, so wArgs[0]=row_num, wArgs[1]=range
            tStackElem& wArgRow = wArgs[0];  // second formula arg (row index)
            tStackElem& wArgRange = wArgs[1]; // first formula arg (range)
            tVariant wErr;
            if (wPropagateIfError(wArgRow, wErr)) {
                return(tStackElem(wErr));
            }
            StackElemToIndex(wArgRow, wRow);
            if (wArgRange.Type() == tStackType::t_Range) {
                wRange=wArgRange.Range();
            }
        }
        else {
            if (wArgs.size() == 3) {
                tStackElem& wArg1 = wArgs[0]; // col
                tStackElem& wArg2 = wArgs[1]; // row (e.g. cell J1)
                tStackElem& wArg3 = wArgs[2]; // range
                tVariant wErr;
                // Check both index args; Excel's left-to-right evaluation
                // yields the row error first when both are #N/A, but for
                // single-arg failures we want either side to short-circuit.
                if (wPropagateIfError(wArg2, wErr) || wPropagateIfError(wArg1, wErr)) {
                    return(tStackElem(wErr));
                }
                StackElemToIndex(wArg1, wCol);
                StackElemToIndex(wArg2, wRow);
                if (wArg3.Type() == tStackType::t_Range) {
                    wRange=wArg3.Range();
                }
            }
            else {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
        }
        
        if ((wRange!=nullptr)) {
            tColRowCellRange* wColRowCellRange=wRange->ColRowCellRange();
            if (wColRowCellRange == nullptr) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
            }
            wRow=wRange->TopIndex()+wRow-1;
            wCol=wRange->LeftIndex()+wCol-1;
            // Use EnsureCell so we always have a cell reference to return (INDEX must return t_Cell in StackElem for dynamic range and display).
            tCell* wCell=wColRowCellRange->EnsureCell(wRow,wCol);
            if (wCell!=nullptr) {
                return(tStackElem(wCell));
            }
            // Return explicit #REF! so IFERROR (and similar) can catch it; empty variant would not be treated as error.
            return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
        }

        return(tStackElem(wValue));
    }   

    //=========================================================================
    //! Function DATARANGE — DataRange(A1:A10;B1:B10;C2) -> ["A1:A10","B1:B10","C2"]
    tFunctionDataRange::tFunctionDataRange() : tFunction() {}

    tStackElem tFunctionDataRange::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty()) {
            return tStackElem(tVariant(tClassError(tTypeError::t_arg, "DataRange requires at least one range")));
        }

        std::vector<tString> wRefs;
        wRefs.reserve(wArgs.size());
        // PopArgs is LIFO; reverse to preserve source argument order.
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            tString wRef;
            if (!StackElemToRangeRefString(*it, wRef)) {
                return tStackElem(tVariant(tClassError(tTypeError::t_ref, "DataRange expects cell or range references")));
            }
            wRefs.push_back(wRef);
        }

        return tStackElem(tVariant(JsonArrayFromRangeRefs(wRefs)));
    }

    //=========================================================================
    //! Function JSON — Json(B2:B7) -> ["ALLEZ","DUPONT",…]; Json(A2:B7) -> [{"value":"…","label":"…"},…]
    tFunctionJson::tFunctionJson() : tFunction() {}

    tStackElem tFunctionJson::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty()) {
            return tStackElem(tVariant(tClassError(tTypeError::t_arg, "JSON requires at least one range")));
        }

        tStringStream wStream;
        wStream << "[";
        tBool wFirst = true;
        // PopArgs is LIFO; reverse to preserve source argument order.
        for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
            if (!StackElemAppendJsonItems(*it, wStream, wFirst)) {
                return tStackElem(tVariant(tClassError(tTypeError::t_ref, "JSON expects cell or range references")));
            }
        }
        wStream << "]";
        return tStackElem(tVariant(wStream.str()));
    }

    //=========================================================================
    //! Function OFFSET — Excel: OFFSET(reference, rows, cols, [height], [width])
    // RPN pop order (last formula arg first): width, height, cols, rows, ref for 5 args;
    // height, cols, rows, ref for 4 args; cols, rows, ref for 3 args.
    tFunctionOffset::tFunctionOffset() : tFunction() {}

    tStackElem tFunctionOffset::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tColRowCellRange* wCr = nullptr;
        long wBaseTop = 0;
        long wBaseLeft = 0;
        long wRefH = 0;
        long wRefW = 0;
        tStackElem* wArgRef = nullptr;
        tInt wRowOff = 0;
        tInt wColOff = 0;
        long wOutH = 0;
        long wOutW = 0;

        if (wArgs.size() == 3) {
            // wArgs[0]=cols, wArgs[1]=rows, wArgs[2]=reference
            wArgRef = &wArgs[2];
            if (!StackElemToInt(wArgs[1], wRowOff) || !StackElemToInt(wArgs[0], wColOff)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
        } else if (wArgs.size() == 4) {
            // wArgs[0]=height, wArgs[1]=cols, wArgs[2]=rows, wArgs[3]=reference
            wArgRef = &wArgs[3];
            tInt wHArg = 0;
            if (!StackElemToInt(wArgs[0], wHArg) || !StackElemToInt(wArgs[1], wColOff) || !StackElemToInt(wArgs[2], wRowOff)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
            wOutH = static_cast<long>(wHArg);
            wOutW = -1; // use reference width
        } else {
            // wArgs[0]=width, wArgs[1]=height, wArgs[2]=cols, wArgs[3]=rows, wArgs[4]=reference
            wArgRef = &wArgs[4];
            tInt wWArg = 0;
            tInt wHArg = 0;
            if (!StackElemToInt(wArgs[0], wWArg) || !StackElemToInt(wArgs[1], wHArg) || !StackElemToInt(wArgs[2], wColOff)
                || !StackElemToInt(wArgs[3], wRowOff)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
            wOutW = static_cast<long>(wWArg);
            wOutH = static_cast<long>(wHArg);
        }

        if (wArgRef == nullptr || !OffsetParseRef(*wArgRef, wCr, wBaseTop, wBaseLeft, wRefH, wRefW)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
        }

        if (wArgs.size() == 3) {
            wOutH = wRefH;
            wOutW = wRefW;
        } else if (wArgs.size() == 4) {
            wOutW = wRefW;
        }

        // Excel: an OMITTED height/width defaults to the reference's height/width.
        // The parser turns omitted args into a literal 0 (PushEmptyFunctionArg), so height/width
        // arrive as 0 and are indistinguishable from an explicit 0. An explicit 0 height/width is
        // an invalid OFFSET in Excel anyway (#REF!), so treating a non-positive height/width as
        // "use the reference dimension" restores the documented default. Without this,
        // e.g. OFFSET(_SOCIETES,,1,,2) returned #REF!, which cascaded to #ARG!/#N/A through
        // VLOOKUP(...,OFFSET(...),...) in real workbooks (INSIDE situation templates).
        if (wOutH < 1) {
            wOutH = wRefH;
        }
        if (wOutW < 1) {
            wOutW = wRefW;
        }

        if (wOutH < 1 || wOutW < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
        }

        const long wNewTop = wBaseTop + static_cast<long>(wRowOff);
        const long wNewLeft = wBaseLeft + static_cast<long>(wColOff);
        const long wNewBottom = wNewTop + wOutH - 1;
        const long wNewRight = wNewLeft + wOutW - 1;

        if (wNewTop < 1 || wNewLeft < 1 || wNewBottom < wNewTop || wNewRight < wNewLeft) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
        }

        tRange* wRange = wCr->EnsureRange(
            static_cast<tIndex>(wNewTop),
            static_cast<tIndex>(wNewLeft),
            static_cast<tIndex>(wNewBottom),
            static_cast<tIndex>(wNewRight));
        if (wRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_ref, ""))));
        }
        return(tStackElem(wRange));
    }

    //=========================================================================
    //! Function LOOKUP (vector form)
    tFunctionLookup::tFunctionLookup() : tFunction() {}

    tStackElem tFunctionLookup::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // LOOKUP(lookup_value, lookup_vector, [result_vector])
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LOOKUP requires 2 or 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wSearch;
        if (!StackElemToVariant(wArgs[0], wSearch)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wSearch.IsError()) {
            return(tStackElem(wSearch));
        }

        tArrayValue wLookup;
        if (!StackElemToArray(wArgs[1], wLookup) || wLookup.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tBool wLookupIsCol = (wLookup.m_Cols == 1);
        const tBool wLookupIsRow = (wLookup.m_Rows == 1);
        if (!wLookupIsCol && !wLookupIsRow) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "LOOKUP: lookup_vector must be 1D"))));
        }

        tArrayValue wResult = wLookup;
        if (wArgs.size() == 3) {
            if (!StackElemToArray(wArgs[2], wResult) || wResult.Count() <= 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            const tBool wResultIsCol = (wResult.m_Cols == 1);
            const tBool wResultIsRow = (wResult.m_Rows == 1);
            if (!wResultIsCol && !wResultIsRow) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "LOOKUP: result_vector must be 1D"))));
            }
            const tIndex wLookupLen = wLookupIsCol ? wLookup.m_Rows : wLookup.m_Cols;
            const tIndex wResultLen = wResultIsCol ? wResult.m_Rows : wResult.m_Cols;
            if (wLookupLen != wResultLen) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "LOOKUP: vectors must be the same size"))));
            }
        }

        const tIndex wFound = FindApproxInLookupVector(wLookup, wSearch, wLookupIsCol);
        if (wFound == static_cast<tIndex>(-1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }

        const tBool wResultIsCol = (wResult.m_Cols == 1);
        const tVariant& wOut = wResultIsCol ? wResult.At(wFound, 0) : wResult.At(0, wFound);
        return(tStackElem(wOut));
    }

    //=========================================================================
    //! Function VLookup  
    tFunctionVLookup::tFunctionVLookup() : tFunction() {}
    
    tStackElem tFunctionVLookup::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3 && wArgs.size() != 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tStackElem* wArgRangeLookup = (wArgs.size() == 4) ? &wArgs[0] : nullptr;
        tStackElem* wArgColumnIndex = &wArgs[wArgs.size() == 4 ? 1 : 0];
        tStackElem* wArgRange = &wArgs[wArgs.size() == 4 ? 2 : 1];
        tStackElem* wArgSearch = &wArgs[wArgs.size() == 4 ? 3 : 2];

        tVariant wColIndexErr;
        tInt wColIndex = 0;
        if (!LookupParseIntArg(*wArgColumnIndex, wColIndex, &wColIndexErr)) {
            if (wColIndexErr.IsError()) {
                return(tStackElem(wColIndexErr));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        if (wArgRange->Type() != tStackType::t_Range || wArgRange->Range() == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tRange* wRange = wArgRange->Range();

        tVariant wSearch;
        if (!LookupParseSearchValue(*wArgSearch, wSearch)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wSearch.IsError()) {
            return(tStackElem(wSearch));
        }

        tBool wExactMatch = false;
        tVariant wRangeLookupErr;
        if (!LookupParseRangeLookup(wArgRangeLookup, wArgs.size() == 4, wExactMatch, &wRangeLookupErr)) {
            if (wRangeLookupErr.IsError()) {
                return(tStackElem(wRangeLookupErr));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        const tIndex wFoundRow = wExactMatch
            ? VLookupFindRowExact(wColRowCellRange, wRange, wSearch)
            : VLookupFindRowApprox(wColRowCellRange, wRange, wSearch);
        if (wFoundRow == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }
        return VLookupReturnFromRow(wColRowCellRange, wRange, wFoundRow, wColIndex);
    }

    //=========================================================================
    //! Function HLookup  
    tFunctionHLookup::tFunctionHLookup() : tFunction() {}

    tStackElem tFunctionHLookup::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3 && wArgs.size() != 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tStackElem* wArgRangeLookup = (wArgs.size() == 4) ? &wArgs[0] : nullptr;
        tStackElem* wArgRowIndex = &wArgs[wArgs.size() == 4 ? 1 : 0];
        tStackElem* wArgRange = &wArgs[wArgs.size() == 4 ? 2 : 1];
        tStackElem* wArgSearch = &wArgs[wArgs.size() == 4 ? 3 : 2];

        tVariant wRowIndexErr;
        tInt wRowIndex = 0;
        if (!LookupParseIntArg(*wArgRowIndex, wRowIndex, &wRowIndexErr)) {
            if (wRowIndexErr.IsError()) {
                return(tStackElem(wRowIndexErr));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        if (wArgRange->Type() != tStackType::t_Range || wArgRange->Range() == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tRange* wRange = wArgRange->Range();

        tVariant wSearch;
        if (!LookupParseSearchValue(*wArgSearch, wSearch)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wSearch.IsError()) {
            return(tStackElem(wSearch));
        }

        tBool wExactMatch = false;
        tVariant wRangeLookupErr;
        if (!LookupParseRangeLookup(wArgRangeLookup, wArgs.size() == 4, wExactMatch, &wRangeLookupErr)) {
            if (wRangeLookupErr.IsError()) {
                return(tStackElem(wRangeLookupErr));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }

        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        const tIndex wFoundCol = wExactMatch
            ? HLookupFindColExact(wColRowCellRange, wRange, wSearch)
            : HLookupFindColApprox(wColRowCellRange, wRange, wSearch);
        if (wFoundCol == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }
        return HLookupReturnFromCol(wColRowCellRange, wRange, wFoundCol, wRowIndex);
    }

    //=========================================================================
    //! Function XLookup
    tFunctionXLookup::tFunctionXLookup() : tFunction() {}

    tStackElem tFunctionXLookup::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // XLOOKUP(lookup_value, lookup_array, return_array, [if_not_found], [match_mode], [search_mode])
        if (wArgs.size() < 3 || wArgs.size() > 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wSearch;
        if (!LookupParseSearchValue(wArgs[0], wSearch)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wSearch.IsError()) {
            return(tStackElem(wSearch));
        }

        tArrayValue wLookup;
        tArrayValue wReturn;
        if (!tFunction::StackElemToArray(wArgs[1], wLookup) || wLookup.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (!tFunction::StackElemToArray(wArgs[2], wReturn) || wReturn.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tBool wHasIfNotFound = (wArgs.size() >= 4);
        tVariant wIfNotFound;
        if (wHasIfNotFound) {
            if (!LookupParseSearchValue(wArgs[3], wIfNotFound)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
            }
            // Propagate only non-#N/A errors from if_not_found argument resolution itself is fine as value.
        }

        tInt wMatchMode = 0;
        if (wArgs.size() >= 5) {
            if (!LookupParseIntArg(wArgs[4], wMatchMode)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        // MVP: exact match only (match_mode 0). Other modes → #VALUE!
        if (wMatchMode != 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XLOOKUP: only match_mode 0 is supported"))));
        }

        tInt wSearchMode = 1;
        if (wArgs.size() >= 6) {
            if (!LookupParseIntArg(wArgs[5], wSearchMode)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        if (wSearchMode != 1 && wSearchMode != -1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XLOOKUP: only search_mode 1 or -1 is supported"))));
        }

        const tBool wLookupIsCol = (wLookup.m_Cols == 1);
        const tBool wLookupIsRow = (wLookup.m_Rows == 1);
        if (!wLookupIsCol && !wLookupIsRow) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XLOOKUP: lookup_array must be a single row or column"))));
        }

        const tIndex wLen = wLookupIsCol ? wLookup.m_Rows : wLookup.m_Cols;
        if (wLookupIsCol) {
            if (wReturn.m_Rows != wLen) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XLOOKUP: return_array row count must match lookup_array"))));
            }
        } else {
            if (wReturn.m_Cols != wLen) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XLOOKUP: return_array column count must match lookup_array"))));
            }
        }

        const tIndex wFound = FindExactInLookupVector(wLookup, wSearch, wLookupIsCol, wSearchMode);

        if (wFound == static_cast<tIndex>(-1)) {
            if (wHasIfNotFound) {
                return(tStackElem(wIfNotFound));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }

        if (wLookupIsCol) {
            // Matching row -> return that row (scalar if single column).
            if (wReturn.m_Cols == 1) {
                return(tStackElem(wReturn.At(wFound, 0)));
            }
            tArrayValue* wOut = new tArrayValue(1, wReturn.m_Cols);
            for (tIndex c = 0; c < wReturn.m_Cols; ++c) {
                wOut->At(0, c) = wReturn.At(wFound, c);
            }
            return(tStackElem(wOut));
        }

        // Matching column -> return that column (scalar if single row).
        if (wReturn.m_Rows == 1) {
            return(tStackElem(wReturn.At(0, wFound)));
        }
        tArrayValue* wOut = new tArrayValue(wReturn.m_Rows, 1);
        for (tIndex r = 0; r < wReturn.m_Rows; ++r) {
            wOut->At(r, 0) = wReturn.At(r, wFound);
        }
        return(tStackElem(wOut));
    }

    //=========================================================================
    //! Function XMATCH
    tFunctionXMatch::tFunctionXMatch() : tFunction() {}

    tStackElem tFunctionXMatch::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // XMATCH(lookup_value, lookup_array, [match_mode], [search_mode])
        if (wArgs.size() < 2 || wArgs.size() > 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wSearch;
        if (!LookupParseSearchValue(wArgs[0], wSearch)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wSearch.IsError()) {
            return(tStackElem(wSearch));
        }

        tArrayValue wLookup;
        if (!tFunction::StackElemToArray(wArgs[1], wLookup) || wLookup.Count() <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tInt wMatchMode = 0;
        if (wArgs.size() >= 3) {
            if (!LookupParseIntArg(wArgs[2], wMatchMode)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        // MVP: exact match only (match_mode 0).
        if (wMatchMode != 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XMATCH: only match_mode 0 is supported"))));
        }

        tInt wSearchMode = 1;
        if (wArgs.size() >= 4) {
            if (!LookupParseIntArg(wArgs[3], wSearchMode)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
        }
        if (wSearchMode != 1 && wSearchMode != -1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XMATCH: only search_mode 1 or -1 is supported"))));
        }

        const tBool wLookupIsCol = (wLookup.m_Cols == 1);
        const tBool wLookupIsRow = (wLookup.m_Rows == 1);
        if (!wLookupIsCol && !wLookupIsRow) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XMATCH: lookup_array must be a single row or column"))));
        }

        const tIndex wFound = FindExactInLookupVector(wLookup, wSearch, wLookupIsCol, wSearchMode);
        if (wFound == static_cast<tIndex>(-1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }
        return(tStackElem(tVariant(static_cast<tInt>(wFound + 1))));
    }

    //=========================================================================
    //! Function Match
    tFunctionMatch::tFunctionMatch() : tFunction() {}

    tStackElem tFunctionMatch::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wValue;
        tRange* wRange=nullptr;
        
        tVariant wMatchType=1; // Default to 1 (greater or equal)
        tVariant wSearch;
        
        tBool wOk=true;
        if (wArgs.size() == 3) {
            // MATCH(lookup_value, lookup_array, match_type) - Excel syntax
            // In RPN: lookup_value lookup_array match_type MATCH, so after PopArgs: wArgs[0]=match_type, wArgs[1]=lookup_array, wArgs[2]=lookup_value
            tStackElem* wArgMatchType = &wArgs[0]; // match_type (third arg, first popped)
            tStackElem* wArgLookupArray = &wArgs[1]; // lookup_array (second arg)
            tStackElem* wArgLookupValue = &wArgs[2]; // lookup_value (first arg, last popped)
            
            // Identify which argument is the Range
            if (wArgLookupArray->Type() == tStackType::t_Range) {
                // Range is second: lookup_array
                wRange = wArgLookupArray->Range();
                
                // Extract lookup_value (can be Variant, Cell, or Range)
                switch(wArgLookupValue->Type()) {
                    case tStackType::t_Variant: {
                        wSearch = wArgLookupValue->Variant();
                        break;
                    }
                    case tStackType::t_Cell: {
                        tCell* wCell = wArgLookupValue->Cell();
                        if (wCell != nullptr) {
                            wSearch = wCell->Value();
                        } else {
                            wOk = false;
                        }
                        break;
                    }
                    case tStackType::t_Range: {
                        tRange* wRangeSearch = wArgLookupValue->Range();
                        if (wRangeSearch != nullptr) {
                            tCell* wCell = wRangeSearch->EnsureCell();
                            if (wCell != nullptr) {
                                wSearch = wCell->Value();
                            } else {
                                wOk = false;
                            }
                        } else {
                            wOk = false;
                        }
                        break;
                    }
                    default:
                        wOk = false;
                        break;
                }
                
                // Extract match_type (can be Variant, Cell, or Range)
                if (wOk) {
                    switch(wArgMatchType->Type()) {
                        case tStackType::t_Variant: {
                            tVariant wMatchTypeVariant = wArgMatchType->Variant();
                            if (wMatchTypeVariant.IsInt()) {
                                wMatchType = wMatchTypeVariant.Int();
                            } else {
                                wOk = false;
                            }
                            break;
                        }
                        case tStackType::t_Cell: {
                            tCell* wCell = wArgMatchType->Cell();
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->Value();
                                if (wCellValue.IsInt()) {
                                    wMatchType = wCellValue.Int();
                                } else if (wCellValue.IsDouble()) {
                                    wMatchType = static_cast<tInt>(wCellValue.Double());
                                } else {
                                    wOk = false;
                                }
                            } else {
                                wOk = false;
                            }
                            break;
                        }
                        case tStackType::t_Range: {
                            tRange* wRangeMatchType = wArgMatchType->Range();
                            if (wRangeMatchType != nullptr) {
                                tCell* wCell = wRangeMatchType->EnsureCell();
                                if (wCell != nullptr) {
                                    tVariant wCellValue = wCell->Value();
                                    if (wCellValue.IsInt()) {
                                        wMatchType = wCellValue.Int();
                                    } else if (wCellValue.IsDouble()) {
                                        wMatchType = static_cast<tInt>(wCellValue.Double());
                                    } else {
                                        wOk = false;
                                    }
                                } else {
                                    wOk = false;
                                }
                            } else {
                                wOk = false;
                            }
                            break;
                        }
                        default:
                            wOk = false;
                            break;
                    }
                }
            } else {
                wOk = false;
            }
        } else {
            if (wArgs.size() == 2) {
                // MATCH(lookup_value, lookup_array) - 2 arguments, default match_type=1
                // In RPN: lookup_value lookup_array MATCH, so after PopArgs: wArgs[0]=lookup_array, wArgs[1]=lookup_value
                tStackElem* wArgLookupArray = &wArgs[0]; // lookup_array (second arg, first popped)
                tStackElem* wArgLookupValue = &wArgs[1]; // lookup_value (first arg, last popped)
                
                if (wArgLookupArray->Type() == tStackType::t_Range) {
                    wRange = wArgLookupArray->Range();
                    
                    // Extract lookup_value (can be Variant, Cell, or Range)
                    switch(wArgLookupValue->Type()) {
                        case tStackType::t_Variant: {
                            wSearch = wArgLookupValue->Variant();
                            break;
                        }
                        case tStackType::t_Cell: {
                            tCell* wCell = wArgLookupValue->Cell();
                            if (wCell != nullptr) {
                                wSearch = wCell->Value();
                            } else {
                                wOk = false;
                            }
                            break;
                        }
                        case tStackType::t_Range: {
                            tRange* wRangeSearch = wArgLookupValue->Range();
                            if (wRangeSearch != nullptr) {
                                tCell* wCell = wRangeSearch->EnsureCell();
                                if (wCell != nullptr) {
                                    wSearch = wCell->Value();
                                } else {
                                    wOk = false;
                                }
                            } else {
                                wOk = false;
                            }
                            break;
                        }
                        default:
                            wOk = false;
                            break;
                    }
                } else {
                    wOk = false;
                }
            } else {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MATCH requires 2 or 3 arguments"))));
            }
        }
        
        if (wOk==false) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        
        if (wRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MATCH: invalid range"))));
        }
        
        // Search Value =======================================================
        tColRowCellRange* wColRowCellRange=wRange->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "MATCH: invalid range"))));
        }
        
        // Ensure match_type is an integer
        if (!wMatchType.IsInt()) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MATCH match_type must be an integer"))));
        }
        
        tInt wMatchTypeInt = wMatchType.Int();
        tIndex wPosition = 0;
        
        if (wMatchTypeInt == 0) {
            // Exact match
            tIndex wSearchCol=wRange->LeftIndex();
            for(tIndex wRow=wRange->TopIndex();wRow<=wRange->IterateBottom();wRow++) {
                tCell* wCell=wColRowCellRange->Cell(wRow,wSearchCol);
                if (wCell!=nullptr) {
                    if (wCell->Value()==wSearch) {
                        wPosition = wRow - wRange->TopIndex() + 1; // 1-based position
                        // Clean up arguments before returning
                        return(tStackElem(tVariant(wPosition)));
                    }
                }
            }
            // Check columns if range is horizontal
            tIndex wSearchRow=wRange->TopIndex();
            for(tIndex wCol=wRange->LeftIndex();wCol<=wRange->IterateRight();wCol++) {
                tCell* wCell=wColRowCellRange->Cell(wSearchRow,wCol);
                if (wCell!=nullptr) {
                    if (wCell->Value()==wSearch) {
                        wPosition = wCol - wRange->LeftIndex() + 1; // 1-based position
                        // Clean up arguments before returning
                        return(tStackElem(tVariant(wPosition)));
                    }
                }
            }
        } else if (wMatchTypeInt == 1) {
            // Greater or equal (ascending order)
            tIndex wSearchCol=wRange->LeftIndex();
            tIndex wLastMatch = 0;
            for(tIndex wRow=wRange->TopIndex();wRow<=wRange->IterateBottom();wRow++) {
                tCell* wCell=wColRowCellRange->Cell(wRow,wSearchCol);
                if (wCell!=nullptr) {
                    tVariant wCellValue = wCell->Value();
                    if (wCellValue >= wSearch) {
                        wPosition = wRow - wRange->TopIndex() + 1; // 1-based position
                        // Clean up arguments before returning
                        return(tStackElem(tVariant(wPosition)));
                    }
                    wLastMatch = wRow - wRange->TopIndex() + 1;
                }
            }
            // Check columns if range is horizontal
            tIndex wSearchRow=wRange->TopIndex();
            for(tIndex wCol=wRange->LeftIndex();wCol<=wRange->IterateRight();wCol++) {
                tCell* wCell=wColRowCellRange->Cell(wSearchRow,wCol);
                if (wCell!=nullptr) {
                    tVariant wCellValue = wCell->Value();
                    if (wCellValue >= wSearch) {
                        wPosition = wCol - wRange->LeftIndex() + 1; // 1-based position
                        // Clean up arguments before returning
                        return(tStackElem(tVariant(wPosition)));
                    }
                    wLastMatch = wCol - wRange->LeftIndex() + 1;
                }
            }
            // Return last position if no exact match found
            if (wLastMatch > 0) {
                // Clean up arguments before returning
                return(tStackElem(tVariant(wLastMatch)));
            }
        } else if (wMatchTypeInt == -1) {
            // Less or equal (descending order)
            // In descending order, find the largest value that is >= search_value
            // Since values are descending, we continue while value >= search_value
            tIndex wSearchCol=wRange->LeftIndex();
            tIndex wLastMatch = 0;
            for(tIndex wRow=wRange->TopIndex();wRow<=wRange->IterateBottom();wRow++) {
                tCell* wCell=wColRowCellRange->Cell(wRow,wSearchCol);
                if (wCell!=nullptr) {
                    tVariant wCellValue = wCell->Value();
                    if (wCellValue >= wSearch) {
                        // Keep track of the last position where value >= search_value
                        wLastMatch = wRow - wRange->TopIndex() + 1; // 1-based position
                    } else {
                        // Value < search_value, stop and return last match
                        if (wLastMatch > 0) {
                            // Clean up arguments before returning
                            return(tStackElem(tVariant(wLastMatch)));
                        }
                        break;
                    }
                }
            }
            // If we reached the end and found a match, return it
            if (wLastMatch > 0) {
                // Clean up arguments before returning
                return(tStackElem(tVariant(wLastMatch)));
            }
            // Check columns if range is horizontal
            tIndex wSearchRow=wRange->TopIndex();
            wLastMatch = 0;
            for(tIndex wCol=wRange->LeftIndex();wCol<=wRange->IterateRight();wCol++) {
                tCell* wCell=wColRowCellRange->Cell(wSearchRow,wCol);
                if (wCell!=nullptr) {
                    tVariant wCellValue = wCell->Value();
                    if (wCellValue >= wSearch) {
                        // Keep track of the last position where value >= search_value
                        wLastMatch = wCol - wRange->LeftIndex() + 1; // 1-based position
                    } else {
                        // Value < search_value, stop and return last match
                        if (wLastMatch > 0) {
                            // Clean up arguments before returning
                            return(tStackElem(tVariant(wLastMatch)));
                        }
                        break;
                    }
                }
            }
            // If we reached the end and found a match, return it
            if (wLastMatch > 0) {
                // Clean up arguments before returning
                return(tStackElem(tVariant(wLastMatch)));
            }
        } else {
            // Clean up arguments before returning
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MATCH match_type must be 0, 1, or -1"))));
        }
        
        // No match found - clean up arguments before returning
        return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));

        return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
    }

    //=========================================================================
    //! Function Row
    tFunctionRow::tFunctionRow() : tFunction() {}

    tStackElem tFunctionRow::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // ROW() can accept 0 or 1 argument
        // If 1 argument is provided, it should be a Range reference
        if (wArgs.size() > 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        
        // If 1 argument is provided, use it
        if (wArgs.size() == 1) {
            tStackElem* wArg = &wArgs[0];
            switch(wArg->Type()) {
                case tStackType::t_Range: {
                    tRange* wRange = wArg->Range();
                    if (wRange != nullptr) {
                        tVariant wResult = tVariant(wRange->TopIndex());
                        return(tStackElem(wResult));
                    }
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArg->Cell();
                    if (wCell != nullptr) {
                        tVariant wResult = tVariant(wCell->RowIndex());
                        return(tStackElem(wResult));
                    }
                    break;
                }
                default:
                    break;
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ROW: argument must be a cell or range reference"))));
        }
        
        // If 0 arguments, use the current cell/range reference (m_ItemRef)
        if (m_ItemRef == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "ROW: no cell reference available"))));
        }
        // Named formulas that use ROW() must see the calling cell (Excel), not the _$$ host.
        if (tCell* wNamedCaller = NamedFormulaCallerFromItem(m_ItemRef)) {
            return(tStackElem(tVariant(wNamedCaller->RowIndex())));
        }
        tCell* wCell=m_ItemRef->Cell();
        if (wCell!=nullptr) {
            return(tStackElem(tVariant(wCell->RowIndex())));
        }
        tRange* wRange=m_ItemRef->Range();
        if (wRange!=nullptr) {
            return(tStackElem(tVariant(wRange->TopIndex())));
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "ROW: invalid cell reference"))));
    }

    void tFunctionRow::PassByRef(tItem* sItem) {
        m_ItemRef=sItem;
    }

    tBool tFunctionRow::ByRef() {
        return(true);
    }


    //=========================================================================
    //! Function Col
    tFunctionColumn::tFunctionColumn() : tFunction()  {}

    tStackElem tFunctionColumn::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // COLUMN() can accept 0 or 1 argument
        // If 1 argument is provided, it should be a Range reference
        if (wArgs.size() > 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        
        // If 1 argument is provided, use it
        if (wArgs.size() == 1) {
            tStackElem* wArg = &wArgs[0];
            switch(wArg->Type()) {
                case tStackType::t_Range: {
                    tRange* wRange = wArg->Range();
                    if (wRange != nullptr) {
                        tVariant wResult = tVariant(wRange->LeftIndex());
                        return(tStackElem(wResult));
                    }
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArg->Cell();
                    if (wCell != nullptr) {
                        tVariant wResult = tVariant(wCell->ColIndex());
                        return(tStackElem(wResult));
                    }
                    break;
                }
                default:
                    break;
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COLUMN: argument must be a cell or range reference"))));
        }
        
        // If 0 arguments, use the current cell/range reference (m_ItemRef)
        if (m_ItemRef == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "COLUMN: no cell reference available"))));
        }
        // Named formulas that use COLUMN() must see the calling cell (Excel), not the _$$ host.
        if (tCell* wNamedCaller = NamedFormulaCallerFromItem(m_ItemRef)) {
            return(tStackElem(tVariant(wNamedCaller->ColIndex())));
        }
        tCell* wCell=m_ItemRef->Cell();
        if (wCell!=nullptr) {
            return(tStackElem(tVariant(wCell->ColIndex())));
        }
        tRange* wRange=m_ItemRef->Range();
        if (wRange!=nullptr) {
            return(tStackElem(tVariant(wRange->LeftIndex())));
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "COLUMN: invalid cell reference"))));
    }

    void tFunctionColumn::PassByRef(tItem* sItem) {
        m_ItemRef=sItem;
    }

    tBool tFunctionColumn::ByRef() {
        return(true);
    }


    //=========================================================================
    //! Function Rows  
    tFunctionRows::tFunctionRows() : tFunction() {}

    tStackElem tFunctionRows::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ROWS requires 1 arguments"))));
        }
        tStackElem* wArg = &wArgs[0];
        switch(wArg->Type()) {
            case tStackType::t_Range: {
                tRange* wRange=wArg->Range();
                tVariant wValue=wRange->BottomIndex()-wRange->TopIndex()+1;
                return(tStackElem(wValue));
            }
            case tStackType::t_Attribute:
            case tStackType::t_Cell: {
                return(tStackElem(tVariant(1)));
            }
            default: {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ROWS require range"))));
            }
        }
    }
    
    tBool tFunctionRows::ByRef() {
        return(true);
    }


    //=========================================================================
    //! Function Columns  
    tFunctionColumns::tFunctionColumns() : tFunction() {}

    tStackElem tFunctionColumns::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tStackElem* wArg = &wArgs[0];
        switch(wArg->Type()) {
            case tStackType::t_Range: {
                tRange* wRange=wArg->Range();
                tVariant wValue=wRange->RightIndex()-wRange->LeftIndex()+1;
                return(tStackElem(wValue));
            }
            case tStackType::t_Attribute:
            case tStackType::t_Cell: {
                  return(tStackElem(tVariant(1)));
            }
            default: {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Columns require range"))));
            }
        }
    }

    tBool tFunctionColumns::ByRef() {
        return(true);
    }

    //=========================================================================
    //! Function ADDRESS (uses Base10ToAlpha from SkTools for column letters)
    //=========================================================================
    tFunctionAddress::tFunctionAddress() : tFunction() {}

    tStackElem tFunctionAddress::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 2 || wArgs.size() > 5) {
            return tStackElem(tVariant(tClassError(tTypeError::t_arg, "ADDRESS requires at least 2 arguments")));
        }
        // PopArgs is LIFO; reverse so wArgs[0]=row_num, [1]=column_num, …
        std::reverse(wArgs.begin(), wArgs.end());

        tInt wRow = 1;
        tInt wCol = 1;
        tInt wAbsNum = 1;
        tBool wA1 = true;
        tString wSheet;

        if (!StackElemToInt(wArgs[0], wRow) || !StackElemToInt(wArgs[1], wCol)) {
            return tStackElem(tVariant(tClassError(tTypeError::t_arg, "ADDRESS row and column must be numbers")));
        }
        if (wArgs.size() >= 3) { StackElemToInt(wArgs[2], wAbsNum); }
        if (wArgs.size() >= 4) { StackElemToBool(wArgs[3], wA1); }
        if (wArgs.size() >= 5) {
            tVariant v5;
            if (StackElemToVariant(wArgs[4], v5) && v5.IsString()) { wSheet = v5.Str(); }
        }

        // Clamp abs_num to 1-4 (Excel behavior)
        if (wAbsNum < 1) wAbsNum = 1;
        if (wAbsNum > 4) wAbsNum = 4;

        tBool wLockRow = (wAbsNum == 1 || wAbsNum == 2);
        tBool wLockCol = (wAbsNum == 1 || wAbsNum == 3);

        tString wResult;
        if (!wSheet.empty()) {
            // Excel wraps sheet names that need quoting (spaces, punctuation, etc.).
            tBool wNeedsQuote = false;
            for (tChar wCh : wSheet) {
                if (!(std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_' || wCh == '.')) {
                    wNeedsQuote = true;
                    break;
                }
            }
            if (wNeedsQuote) {
                wResult += '\'';
                for (tChar wCh : wSheet) {
                    if (wCh == '\'') wResult += '\''; // escape ' as ''
                    wResult += wCh;
                }
                wResult += '\'';
            } else {
                wResult += wSheet;
            }
            wResult += "!";
        }
        if (wA1) {
            if (wLockCol) wResult += "$";
            wResult += Base10ToAlpha(static_cast<tIndex>(wCol));
            if (wLockRow) wResult += "$";
            wResult += std::to_string(wRow);
        } else {
            // R1C1 style
            wResult += "R";
            if (wLockRow) wResult += "$";
            wResult += std::to_string(wRow);
            wResult += "C";
            if (wLockCol) wResult += "$";
            wResult += std::to_string(wCol);
        }
        return tStackElem(tVariant(wResult));
    }

    //=========================================================================
    //! Function INDIRECT (Excel: ref_text, [a1])
    //=========================================================================
    namespace {

    static tBool IndirectUnquoteSheetName(tString& ioName) {
        if (ioName.size() < 2 || ioName.front() != '\'' || ioName.back() != '\'') {
            return false;
        }
        tString wUnquoted;
        wUnquoted.reserve(ioName.size());
        for (size_t wIndex = 1; wIndex + 1 < ioName.size(); ++wIndex) {
            if (ioName[wIndex] == '\'' && wIndex + 1 < ioName.size() && ioName[wIndex + 1] == '\'') {
                wUnquoted += '\'';
                ++wIndex;
            } else {
                wUnquoted += ioName[wIndex];
            }
        }
        ioName = wUnquoted;
        return true;
    }

    static tBool IndirectSplitSheetRef(const tString& sRef, tString& oSheetPart, tString& oAddrPart) {
        oSheetPart.clear();
        oAddrPart.clear();
        if (sRef.empty()) {
            return false;
        }
        if (sRef.front() == '\'') {
            for (size_t wIndex = 1; wIndex < sRef.size(); ++wIndex) {
                if (sRef[wIndex] != '\'') {
                    continue;
                }
                if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '\'') {
                    ++wIndex;
                    continue;
                }
                if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '!') {
                    oSheetPart = sRef.substr(0, wIndex + 1);
                    IndirectUnquoteSheetName(oSheetPart);
                    oAddrPart = sRef.substr(wIndex + 2);
                    return !oAddrPart.empty();
                }
                break;
            }
            return false;
        }
        const size_t wBangPos = sRef.rfind('!');
        if (wBangPos != tString::npos) {
            oSheetPart = sRef.substr(0, wBangPos);
            oAddrPart = sRef.substr(wBangPos + 1);
            return !oAddrPart.empty();
        }
        oAddrPart = sRef;
        return true;
    }

    static tBool IndirectResolveCellToken(tLexerToken& sToken,
                                          tIndex sCallerRow,
                                          tIndex sCallerCol,
                                          tIndex& oRow,
                                          tIndex& oCol) {
        if (sToken.R1C1()) {
            if (sToken.LockRow()) {
                oRow = static_cast<tIndex>(sToken.RowInt());
            } else {
                oRow = sCallerRow + static_cast<tIndex>(sToken.RowInt());
            }
            if (sToken.LockCol()) {
                oCol = static_cast<tIndex>(sToken.ColInt());
            } else {
                oCol = sCallerCol + static_cast<tIndex>(sToken.ColInt());
            }
        } else {
            oRow = static_cast<tIndex>(sToken.RowInt());
            oCol = static_cast<tIndex>(sToken.ColInt());
        }
        return oRow >= 1 && oCol >= 1;
    }

    static tBool IndirectParseAddressR1C1(const tString& sAddr,
                                          tIndex sCallerRow,
                                          tIndex sCallerCol,
                                          tIndex& oTop,
                                          tIndex& oLeft,
                                          tIndex& oBottom,
                                          tIndex& oRight) {
        tLexer wLex(sAddr.c_str());
        tLexerToken wFirst = wLex.next();
        if (!wFirst.is(tKind::Cell)) {
            return false;
        }
        tIndex wRow1 = 0;
        tIndex wCol1 = 0;
        if (!IndirectResolveCellToken(wFirst, sCallerRow, sCallerCol, wRow1, wCol1)) {
            return false;
        }

        tLexerToken wSecond = wLex.next();
        if (wSecond.is(tKind::End)) {
            oTop = oBottom = wRow1;
            oLeft = oRight = wCol1;
            return true;
        }
        if (!wSecond.is(tKind::Colon)) {
            return false;
        }

        tLexerToken wThird = wLex.next();
        if (!wThird.is(tKind::Cell)) {
            return false;
        }
        tIndex wRow2 = 0;
        tIndex wCol2 = 0;
        if (!IndirectResolveCellToken(wThird, sCallerRow, sCallerCol, wRow2, wCol2)) {
            return false;
        }

        oTop = std::min(wRow1, wRow2);
        oBottom = std::max(wRow1, wRow2);
        oLeft = std::min(wCol1, wCol2);
        oRight = std::max(wCol1, wCol2);
        return true;
    }

    static tBool IndirectLooksLikeA1Ref(const tString& sAddr) {
        if (sAddr.find(':') != tString::npos) {
            return true;
        }
        tBool wHasLetter = false;
        tBool wHasDigit = false;
        for (const tChar wChar : sAddr) {
            if (wChar == '$') {
                continue;
            }
            if ((wChar >= 'A' && wChar <= 'Z') || (wChar >= 'a' && wChar <= 'z')) {
                wHasLetter = true;
                continue;
            }
            if (wChar >= '0' && wChar <= '9') {
                wHasDigit = true;
                continue;
            }
            return false;
        }
        return wHasLetter && wHasDigit;
    }

    static tBool IndirectLooksLikeR1C1Ref(const tString& sAddr) {
        return !sAddr.empty() && (sAddr[0] == 'R' || sAddr[0] == 'r');
    }

    // French Excel L1C1 inside INDIRECT(...,FALSE): LC(-1), L13C3, L(+1)C(-2), etc.
    static void IndirectAppendFrenchL1C1AxisSpec(const tString& sBody,
                                                 tSize& ioPos,
                                                 tString& oOut,
                                                 tBool sAllowImplicitZero) {
        const tSize n = sBody.size();
        if (ioPos >= n) {
            if (sAllowImplicitZero) {
                oOut += "[0]";
            }
            return;
        }
        if (sBody[ioPos] == '[') {
            while (ioPos < n) {
                oOut += sBody[ioPos];
                if (sBody[ioPos] == ']') {
                    ++ioPos;
                    break;
                }
                ++ioPos;
            }
            return;
        }
        if (sBody[ioPos] == '(') {
            ++ioPos;
            tBool wNegative = false;
            if (ioPos < n && sBody[ioPos] == '+') {
                ++ioPos;
            } else if (ioPos < n && sBody[ioPos] == '-') {
                wNegative = true;
                ++ioPos;
            }
            tInt wOffset = 0;
            tBool wHasDigits = false;
            while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
                wHasDigits = true;
                wOffset = wOffset * 10 + static_cast<tInt>(sBody[ioPos] - '0');
                ++ioPos;
            }
            if (!wHasDigits || ioPos >= n || sBody[ioPos] != ')') {
                return;
            }
            ++ioPos;
            if (wNegative) {
                wOffset = -wOffset;
            }
            oOut += '[';
            oOut += std::to_string(wOffset);
            oOut += ']';
            return;
        }
        if (std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
            while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
                oOut += sBody[ioPos++];
            }
            return;
        }
        if (sAllowImplicitZero) {
            const char wNextUpper =
                static_cast<char>(std::toupper(static_cast<unsigned char>(sBody[ioPos])));
            if (wNextUpper == 'L' || wNextUpper == 'R' || wNextUpper == 'C' || sBody[ioPos] == ':') {
                oOut += "[0]";
            }
        }
    }

    static tBool IndirectLooksLikeFrenchL1C1Ref(const tString& sAddr) {
        if (sAddr.empty()) {
            return false;
        }
        const char wFirst =
            static_cast<char>(std::toupper(static_cast<unsigned char>(sAddr[0])));
        if (wFirst != 'L' || sAddr.size() < 2) {
            return false;
        }
        const char wSecond =
            static_cast<char>(std::toupper(static_cast<unsigned char>(sAddr[1])));
        return wSecond == 'C' || wSecond == '(' ||
               std::isdigit(static_cast<unsigned char>(wSecond));
    }

    static tString IndirectConvertFrenchL1C1ToR1C1(const tString& sAddr) {
        if (sAddr.empty()) {
            return sAddr;
        }
        tString wOut;
        wOut.reserve(sAddr.size() + 16);
        for (tSize wIndex = 0; wIndex < sAddr.size(); ) {
            const char wCharUpper =
                static_cast<char>(std::toupper(static_cast<unsigned char>(sAddr[wIndex])));
            if (wCharUpper == 'L' || wCharUpper == 'R') {
                wOut += 'R';
                ++wIndex;
                IndirectAppendFrenchL1C1AxisSpec(sAddr, wIndex, wOut, true);
            } else if (wCharUpper == 'C') {
                wOut += 'C';
                ++wIndex;
                IndirectAppendFrenchL1C1AxisSpec(sAddr, wIndex, wOut, true);
            } else if (wCharUpper == ':') {
                wOut += ':';
                ++wIndex;
            } else {
                return sAddr;
            }
        }
        return wOut;
    }

    static tBool IndirectLooksLikeCellRef(const tString& sAddr, tBool sA1) {
        if (sA1) {
            return IndirectLooksLikeA1Ref(sAddr);
        }
        return IndirectLooksLikeR1C1Ref(sAddr);
    }

    static tBool IndirectParseAddress(const tString& sAddr,
                                      tBool sA1,
                                      tIndex sCallerRow,
                                      tIndex sCallerCol,
                                      tIndex& oTop,
                                      tIndex& oLeft,
                                      tIndex& oBottom,
                                      tIndex& oRight) {
        if (sA1) {
            return ParseRange(sAddr, oTop, oLeft, oBottom, oRight);
        }
        return IndirectParseAddressR1C1(sAddr, sCallerRow, sCallerCol, oTop, oLeft, oBottom, oRight);
    }

    static tStackElem IndirectResolveRefText(tWorkBook* sWorkBook,
                                             tSheet* sDefaultSheet,
                                             tIndex sCallerRow,
                                             tIndex sCallerCol,
                                             const tString& sRefText,
                                             tBool sA1) {
        if (sWorkBook == nullptr || sDefaultSheet == nullptr || sRefText.empty()) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }

        tString wSheetPart;
        tString wAddrPart;
        if (!IndirectSplitSheetRef(sRefText, wSheetPart, wAddrPart)) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }

        tSheet* wTargetSheet = sDefaultSheet;
        if (!wSheetPart.empty()) {
            wTargetSheet = sWorkBook->Sheet(wSheetPart);
            if (wTargetSheet == nullptr) {
                return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
            }
        }

        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;

        // French Excel L1C1 ref_text (LC(-1), L13C3, …) even when a1 defaults to TRUE.
        // INDIRECT("C",0) column-axis is materialized at SkExcel import (ApplyFormulas); not resolved here.
        tString wAddrForParse = wAddrPart;
        tBool wParseAsA1 = sA1;
        if (IndirectLooksLikeFrenchL1C1Ref(wAddrPart)) {
            wAddrForParse = IndirectConvertFrenchL1C1ToR1C1(wAddrPart);
            wParseAsA1 = false;
        }

        if (!IndirectLooksLikeCellRef(wAddrForParse, wParseAsA1)) {
            tRange* wNamedRangeOnly = sWorkBook->FindRangeNamed(wAddrForParse);
            if (wNamedRangeOnly != nullptr) {
                return tStackElem(wNamedRangeOnly);
            }
            // Excel accepts A1-style ref_text even when a1=FALSE (common in shared formulas).
            if (!wParseAsA1 && IndirectLooksLikeA1Ref(wAddrForParse)) {
                if (IndirectParseAddress(wAddrForParse, true, sCallerRow, sCallerCol, wTop, wLeft, wBottom, wRight)) {
                    // Fall through to range/cell resolution below.
                } else {
                    return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
                }
            } else {
                return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
            }
        } else if (IndirectParseAddress(wAddrForParse, wParseAsA1, sCallerRow, sCallerCol, wTop, wLeft, wBottom, wRight)) {
            // parsed below
        } else if (!wParseAsA1 && IndirectLooksLikeA1Ref(wAddrForParse) &&
                   IndirectParseAddress(wAddrForParse, true, sCallerRow, sCallerCol, wTop, wLeft, wBottom, wRight)) {
            // A1 fallback when R1C1 parse failed.
        } else {
            tRange* wNamedRange = sWorkBook->FindRangeNamed(wAddrForParse);
            if (wNamedRange != nullptr) {
                return tStackElem(wNamedRange);
            }
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }

        if (wTop < 1 || wLeft < 1 || wBottom > Cst_MaxRow || wRight > Cst_MaxCol) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }
        if (wTop == wBottom && wLeft == wRight) {
            tCell* wCell = wTargetSheet->EnsureCell(static_cast<tInt>(wTop), static_cast<tInt>(wLeft));
            if (wCell == nullptr) {
                return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
            }
            return tStackElem(wCell);
        }
        tRange* wRange = wTargetSheet->EnsureRange(
            static_cast<tInt>(wTop),
            static_cast<tInt>(wLeft),
            static_cast<tInt>(wBottom),
            static_cast<tInt>(wRight));
        if (wRange == nullptr) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "")));
        }
        return tStackElem(wRange);
    }

    } // namespace

    tFunctionIndirect::tFunctionIndirect() : tFunction(), m_ItemRef(nullptr) {}

    tStackElem tFunctionIndirect::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.empty() || wArgs.size() > 2) {
            return tStackElem(tVariant(tClassError(tTypeError::t_arg, "INDIRECT requires 1 or 2 arguments")));
        }

        tVariant wRefText;
        tStackElem* wRefArg = &wArgs[0];
        tBool wA1 = true;
        if (wArgs.size() == 2) {
            // RPN: ref_text, a1 -> wArgs[0]=a1, wArgs[1]=ref_text
            wRefArg = &wArgs[1];
            if (!StackElemToBool(wArgs[0], wA1)) {
                return tStackElem(tVariant(tClassError(tTypeError::t_value, "INDIRECT: a1 must be logical")));
            }
        }

        if (!StackElemToVariant(*wRefArg, wRefText) || !wRefText.IsString()) {
            return tStackElem(tVariant(tClassError(tTypeError::t_value, "INDIRECT: ref_text must be text")));
        }

        tSheet* wDefaultSheet = nullptr;
        tIndex wCallerRow = 1;
        tIndex wCallerCol = 1;
        if (m_ItemRef != nullptr) {
            tCell* wCallerCell = m_ItemRef->Cell();
            if (wCallerCell != nullptr) {
                wDefaultSheet = wCallerCell->Sheet();
                wCallerRow = wCallerCell->RowIndex();
                wCallerCol = wCallerCell->ColIndex();
            } else {
                tRange* wCallerRange = m_ItemRef->Range();
                if (wCallerRange != nullptr) {
                    wDefaultSheet = wCallerRange->Sheet();
                    wCallerRow = wCallerRange->TopIndex();
                    wCallerCol = wCallerRange->LeftIndex();
                }
            }
        }
        if (wDefaultSheet == nullptr) {
            return tStackElem(tVariant(tClassError(tTypeError::t_ref, "INDIRECT: no sheet context")));
        }

        return IndirectResolveRefText(
            wDefaultSheet->WorkBook(),
            wDefaultSheet,
            wCallerRow,
            wCallerCol,
            wRefText.Str(),
            wA1);
    }

    void tFunctionIndirect::PassByRef(tItem* sItem) {
        m_ItemRef = sItem;
    }

    tBool tFunctionIndirect::ByRef() {
        return true;
    }

    //=========================================================================
    //! Call back for function SUMIF
    tCallBackRangeSumIf::tCallBackRangeSumIf(tColRowCellRange* sColRowCellRange, tVariant sCriteria, tRange* sSumRange, tRange* sCriteriaRange) 
        : tCallBackRangeFunction(sColRowCellRange), m_Criteria(sCriteria), m_SumRange(sSumRange), m_CriteriaRange(sCriteriaRange) {
        m_Value = 0;
    }

    tBool tCallBackRangeSumIf::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell != nullptr) {
            // Check if the cell value matches the criteria
            tVariant wCellValue = wCell->CalculableValue();
            
            // Debug output to understand what's happening
            #ifdef sumifdebug
            cout << "SUMIF Debug - Cell: " << wCell->StrRef() 
                 << " Value: '" << wCellValue.Str() << "'" 
                 << " Criteria: '" << m_Criteria.Str() << "'" 
                 << " Match: " << (wCellValue == m_Criteria ? "YES" : "NO") << endl;
            #endif
            
            // Simple equality check for now (can be extended for wildcards, operators)
            if (wCellValue == m_Criteria) {
                if (m_SumRange != nullptr && m_CriteriaRange != nullptr) {
                    // SUMIF with sum_range: sum corresponding cells in sum_range
                    // Calculate relative position within the criteria range
                    tIndex wCurrentRow = wCell->RowIndex();
                    tIndex wCurrentCol = wCell->ColIndex();
                    
                    // Calculate offset from criteria range start
                    tIndex wRowOffset = wCurrentRow - m_CriteriaRange->TopIndex();
                    tIndex wColOffset = wCurrentCol - m_CriteriaRange->LeftIndex();
                    
                    // Calculate corresponding position in sum range
                    tIndex wSumRow = m_SumRange->TopIndex() + wRowOffset;
                    tIndex wSumCol = m_SumRange->LeftIndex() + wColOffset;
                    
                    // Check if the calculated position is within sum range bounds
                    if (wSumRow >= m_SumRange->TopIndex() && wSumRow <= m_SumRange->BottomIndex() &&
                        wSumCol >= m_SumRange->LeftIndex() && wSumCol <= m_SumRange->RightIndex()) {
                        
                        tCell* wSumCell = m_SumRange->ColRowCellRange()->Cell(wSumRow, wSumCol);
                        if (wSumCell != nullptr) {
                            // Use current in-memory value; SCC / main Reduce ordering converges cycles.
                            // Nested InternalCalculation here overflows the stack on Ref SUMIF(V:V) SCCs.
                            tVariant wSumValue = wSumCell->CalculableValue();
                            if (!SumIfAccumulate(m_Value, wSumValue)) {
                                return false;
                            }
                            
                            #ifdef sumifdebug
                            cout << "SUMIF Debug - Adding: " << wSumValue.Str() 
                                 << " from " << wSumCell->StrRef() << endl;
                            #endif
                        }
                    }
                } else {
                    // SUMIF without sum_range: sum the criteria cells themselves
                    if (!SumIfAccumulate(m_Value, wCellValue)) {
                        return false;
                    }
                    
                    #ifdef sumifdebug
                    cout << "SUMIF Debug - Adding: " << wCellValue.Str() 
                         << " from " << wCell->StrRef() << endl;
                    #endif
                }
            }
        }
        return(true); // Continue processing
    }

    //=========================================================================
    //! Function SUMIF
    tFunctionSumIf::tFunctionSumIf() : tFunction() {}

    tStackElem tFunctionSumIf::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 2 || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUMIF requires 2 or 3 arguments"))));
        }

        tRange* wRange = nullptr;
        tCriterionArg wCrit;
        tRange* wSumRange = nullptr;

        if (wArgs.size() == 2) {
            // SUMIF(range, criteria) - 2 arguments
            // In RPN: range criteria SUMIF, so after PopArgs: wArgs[0]=criteria, wArgs[1]=range
            tStackElem* wArgCriteria = &wArgs[0]; // criteria (second arg, first popped)
            tStackElem* wArgRange = &wArgs[1]; // range (first arg, last popped)

            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "First argument must be a range"))));
            }

            wRange = wArgRange->Range();

            tString wErr;
            if (!ReadCriterionArg(wArgCriteria, wCrit, wErr)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, wErr))));
            }

            wSumRange = wRange; // Use same range for both criteria and sum
        } else {
            // SUMIF(range, criteria, sum_range) - 3 arguments
            // In RPN: range criteria sum_range SUMIF, so after PopArgs: wArgs[0]=sum_range, wArgs[1]=criteria, wArgs[2]=range
            tStackElem* wArgSumRange = &wArgs[0]; // sum_range (last popped, third arg)
            tStackElem* wArgCriteria = &wArgs[1]; // criteria (second arg)
            tStackElem* wArgRange = &wArgs[2]; // range (first popped, first arg)

            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "First argument must be a range"))));
            }
            if (wArgSumRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Third argument must be a range"))));
            }

            wRange = wArgRange->Range();
            wSumRange = wArgSumRange->Range();

            tString wErr;
            if (!ReadCriterionArg(wArgCriteria, wCrit, wErr)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, wErr))));
            }
        }

        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();

        // Sum for the single criterion selected at output cell (sOr,sOc).
        auto wComputeOne = [&](tIndex sOr, tIndex sOc) -> tVariant {
            tVariant tmp = wCrit.At(sOr, sOc);
            tCriteriaParser wCriteriaParser(tmp);
            tVariant wResult = 0;
            tBool wContinue = true;
            for (tIndex wRow = wRange->TopIndex(); wContinue && wRow <= wRange->IterateBottom(); wRow++) {
                for (tIndex wCol = wRange->LeftIndex(); wContinue && wCol <= wRange->IterateRight(); wCol++) {
                    tCell* wCriteriaCell = wColRowCellRange->Cell(wRow, wCol);
                    if (wCriteriaCell != nullptr) {
                        tVariant wCellValue = wCriteriaCell->CalculableValue();
                        if (wCriteriaParser.Match(wCellValue)) {
                            if (wSumRange != nullptr) {
                                tIndex wRowOffset = wRow - wRange->TopIndex();
                                tIndex wColOffset = wCol - wRange->LeftIndex();
                                tIndex wSumRow = wSumRange->TopIndex() + wRowOffset;
                                tIndex wSumCol = wSumRange->LeftIndex() + wColOffset;
                                if (wSumRow >= wSumRange->TopIndex() && wSumRow <= wSumRange->BottomIndex() &&
                                    wSumCol >= wSumRange->LeftIndex() && wSumCol <= wSumRange->RightIndex()) {
                                    tCell* wSumCell = wSumRange->ColRowCellRange()->Cell(wSumRow, wSumCol);
                                    if (wSumCell != nullptr) {
                                        if (!SumIfAccumulate(wResult, wSumCell->CalculableValue())) {
                                            wContinue = false;
                                        }
                                    }
                                }
                            } else {
                                if (!SumIfAccumulate(wResult, wCellValue)) {
                                    wContinue = false;
                                }
                            }
                        }
                    }
                }
            }
            return wResult;
        };

        if (!wCrit.m_IsArray) {
            return(tStackElem(wComputeOne(0, 0)));
        }

        // Unique-list array formulas (Excel loan template K/L):
        // SUMIF(Table[Year], J16:Jn, Table[Amount]) → one exact sum per year.
        const tIndex wSumCritN = wCrit.m_Rows * wCrit.m_Cols;
        const tIndex wSumRangeH = wRange->IterateBottom() - wRange->TopIndex() + 1;
        const tIndex wSumRangeW = wRange->IterateRight() - wRange->LeftIndex() + 1;
        const tIndex wSumRangeN = (wSumRangeH > 0 && wSumRangeW > 0) ? (wSumRangeH * wSumRangeW) : 0;
        if (wSumCritN > 1 && wSumRangeN > 0) {
            tBool wAllNumEq = true;
            tBool wAllExactText = true;
            for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
                for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                    tVariant wTmp = wCrit.At(r, c);
                    tCriteriaParser wParser(wTmp);
                    const tBool wNumEq = !wParser.m_Negate &&
                        wParser.m_Type == tCriteriaParser::tCriteriaType::t_Numeric &&
                        wParser.m_Operator == "=";
                    const tBool wExact = !wParser.m_Negate &&
                        wParser.m_Type == tCriteriaParser::tCriteriaType::t_Exact;
                    tDouble wIgnored = 0.0;
                    const tBool wExactLooksNumeric = wExact && wParser.m_OriginalCriteria.IsString() &&
                        tCriteriaParser::TryParseDouble(wParser.m_OriginalCriteria.String(), wIgnored, true);
                    if (!wNumEq) {
                        wAllNumEq = false;
                    }
                    if (!wExact || wExactLooksNumeric) {
                        wAllExactText = false;
                    }
                }
            }
            if (wAllNumEq || wAllExactText) {
                std::map<tDouble, tVariant> wNumSums;
                std::map<tString, tVariant> wStrSums;
                tBool wOk = true;
                for (tIndex wRow = wRange->TopIndex(); wOk && wRow <= wRange->IterateBottom(); ++wRow) {
                    for (tIndex wCol = wRange->LeftIndex(); wOk && wCol <= wRange->IterateRight(); ++wCol) {
                        tCell* wCriteriaCell = wColRowCellRange->Cell(wRow, wCol);
                        if (wCriteriaCell == nullptr) {
                            continue;
                        }
                        tVariant wCellValue = wCriteriaCell->CalculableValue();
                        tVariant wAdd;
                        if (wSumRange != nullptr) {
                            const tIndex wRowOffset = wRow - wRange->TopIndex();
                            const tIndex wColOffset = wCol - wRange->LeftIndex();
                            const tIndex wSumRow = wSumRange->TopIndex() + wRowOffset;
                            const tIndex wSumCol = wSumRange->LeftIndex() + wColOffset;
                            if (wSumRow >= wSumRange->TopIndex() && wSumRow <= wSumRange->BottomIndex() &&
                                wSumCol >= wSumRange->LeftIndex() && wSumCol <= wSumRange->RightIndex()) {
                                tCell* wSumCell = wSumRange->ColRowCellRange()->Cell(wSumRow, wSumCol);
                                if (wSumCell != nullptr) {
                                    wAdd = wSumCell->CalculableValue();
                                }
                            }
                        } else {
                            wAdd = wCellValue;
                        }
                        if (wAllNumEq) {
                            if (!(wCellValue.IsDouble() || wCellValue.IsInt())) {
                                continue;
                            }
                            const tDouble wD = wCellValue.IsDouble() ? wCellValue.Double()
                                : static_cast<tDouble>(wCellValue.Int());
                            tVariant& wAcc = wNumSums[wD];
                            if (wAcc.IsNull()) {
                                wAcc = tVariant(0);
                            }
                            if (!SumIfAccumulate(wAcc, wAdd)) {
                                wOk = false;
                            }
                        } else if (wCellValue.IsString()) {
                            tVariant& wAcc = wStrSums[wCellValue.String()];
                            if (wAcc.IsNull()) {
                                wAcc = tVariant(0);
                            }
                            if (!SumIfAccumulate(wAcc, wAdd)) {
                                wOk = false;
                            }
                        }
                    }
                }
                if (wOk) {
                    tArrayValue* wFast = new tArrayValue(wCrit.m_Rows, wCrit.m_Cols);
                    for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
                        for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                            tVariant wTmp = wCrit.At(r, c);
                            tCriteriaParser wParser(wTmp);
                            tVariant wSum = 0;
                            if (wAllNumEq) {
                                auto wIt = wNumSums.find(wParser.m_NumericValue);
                                if (wIt != wNumSums.end()) {
                                    wSum = wIt->second;
                                }
                            } else {
                                tString wS = wParser.m_OriginalCriteria.IsString()
                                    ? wParser.m_OriginalCriteria.String()
                                    : wParser.m_OriginalCriteria.Str();
                                auto wIt = wStrSums.find(wS);
                                if (wIt != wStrSums.end()) {
                                    wSum = wIt->second;
                                }
                            }
                            wFast->At(r, c) = wSum;
                        }
                    }
                    return(tStackElem(wFast));
                }
            }
        }

        tArrayValue* wOut = new tArrayValue(wCrit.m_Rows, wCrit.m_Cols);
        for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
            for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                wOut->At(r, c) = wComputeOne(r, c);
            }
        }
        return(tStackElem(wOut));
    }

    //=========================================================================
    //! Function COUNTIF
    //=========================================================================
    tFunctionCountIf::tFunctionCountIf() : tFunction() {
    }

    tStackElem tFunctionCountIf::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COUNTIF requires exactly 2 arguments"))));
        }

        // COUNTIF(range, criteria) - 2 arguments
        // In RPN: range criteria COUNTIF, so after PopArgs: wArgs[0]=criteria, wArgs[1]=range
        tStackElem* wArgCriteria = &wArgs[0]; // criteria (second arg, first popped)
        tStackElem* wArgRange = &wArgs[1]; // range (first arg, last popped)
        
        #ifdef sumifdebug
        cout << "COUNTIF Debug - 2 arguments:" << endl;
        cout << "  Arg 0: " << (wArgCriteria->Type() == tStackType::t_Variant ? "Variant" : "Not Variant") << endl;
        cout << "  Arg 1: " << (wArgRange->Type() == tStackType::t_Range ? "Range" : "Not Range") << endl;
        #endif
        
        if (wArgRange->Type() != tStackType::t_Range) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COUNTIF: first argument must be a range"))));
        }
        
        tRange* wRange = wArgRange->Range();

        // Extract criteria; an array criterion (t_Array) makes COUNTIF spill one count per element.
        tCriterionArg wCrit;
        tString wErr;
        if (!ReadCriterionArg(wArgCriteria, wCrit, wErr)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, wErr))));
        }

        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();

        // Count matches for the single criterion selected at output cell (sOr,sOc).
        auto wComputeOne = [&](tIndex sOr, tIndex sOc) -> tVariant {
            tVariant tmp = wCrit.At(sOr, sOc);
            tCriteriaParser wCriteriaParser(tmp);
            tInt wCount = 0;
            for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                    tCell* wCriteriaCell = wColRowCellRange->Cell(wRow, wCol);
                    if (wCriteriaCell != nullptr) {
                        tVariant wCellValue = wCriteriaCell->CalculableValue();
                        if (wCriteriaParser.Match(wCellValue)) wCount++;
                    }
                }
            }
            return tVariant(wCount);
        };

        if (!wCrit.m_IsArray) {
            return(tStackElem(wComputeOne(0, 0)));
        }

        // Unique-list array formulas (Excel loan template column J):
        // COUNTIF($J$16:Jn, Table[Year]) → one exact count per year.
        // Nested scans are O(criteria × range); a frequency map is O(range + criteria).
        const tIndex wCritN = wCrit.m_Rows * wCrit.m_Cols;
        const tIndex wRangeH = wRange->IterateBottom() - wRange->TopIndex() + 1;
        const tIndex wRangeW = wRange->IterateRight() - wRange->LeftIndex() + 1;
        const tIndex wRangeN = (wRangeH > 0 && wRangeW > 0) ? (wRangeH * wRangeW) : 0;
        if (wCritN > 1 && wRangeN > 0) {
            tBool wAllNumEq = true;
            tBool wAllExactText = true;
            for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
                for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                    tVariant wTmp = wCrit.At(r, c);
                    tCriteriaParser wParser(wTmp);
                    const tBool wNumEq = !wParser.m_Negate &&
                        wParser.m_Type == tCriteriaParser::tCriteriaType::t_Numeric &&
                        wParser.m_Operator == "=";
                    const tBool wExact = !wParser.m_Negate &&
                        wParser.m_Type == tCriteriaParser::tCriteriaType::t_Exact;
                    tDouble wIgnored = 0.0;
                    const tBool wExactLooksNumeric = wExact && wParser.m_OriginalCriteria.IsString() &&
                        tCriteriaParser::TryParseDouble(wParser.m_OriginalCriteria.String(), wIgnored, true);
                    if (!wNumEq) {
                        wAllNumEq = false;
                    }
                    if (!wExact || wExactLooksNumeric) {
                        wAllExactText = false;
                    }
                }
            }
            if (wAllNumEq || wAllExactText) {
                std::map<tDouble, tInt> wNumCounts;
                std::map<tString, tInt> wStrCounts;
                for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); ++wRow) {
                    for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); ++wCol) {
                        tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                        if (wCell == nullptr) {
                            continue;
                        }
                        tVariant wV = wCell->CalculableValue();
                        if (wAllNumEq) {
                            if (wV.IsDouble() || wV.IsInt()) {
                                const tDouble wD = wV.IsDouble() ? wV.Double() : static_cast<tDouble>(wV.Int());
                                wNumCounts[wD] += 1;
                            }
                        } else if (wV.IsString()) {
                            wStrCounts[wV.String()] += 1;
                        }
                    }
                }
                tArrayValue* wFast = new tArrayValue(wCrit.m_Rows, wCrit.m_Cols);
                for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
                    for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                        tVariant wTmp = wCrit.At(r, c);
                        tCriteriaParser wParser(wTmp);
                        tInt wCount = 0;
                        if (wAllNumEq) {
                            auto wIt = wNumCounts.find(wParser.m_NumericValue);
                            if (wIt != wNumCounts.end()) {
                                wCount = wIt->second;
                            }
                        } else {
                            tString wS = wParser.m_OriginalCriteria.IsString()
                                ? wParser.m_OriginalCriteria.String()
                                : wParser.m_OriginalCriteria.Str();
                            auto wIt = wStrCounts.find(wS);
                            if (wIt != wStrCounts.end()) {
                                wCount = wIt->second;
                            }
                        }
                        wFast->At(r, c) = tVariant(wCount);
                    }
                }
                return(tStackElem(wFast));
            }
        }

        tArrayValue* wOut = new tArrayValue(wCrit.m_Rows, wCrit.m_Cols);
        for (tIndex r = 0; r < wCrit.m_Rows; ++r) {
            for (tIndex c = 0; c < wCrit.m_Cols; ++c) {
                wOut->At(r, c) = wComputeOne(r, c);
            }
        }
        return(tStackElem(wOut));
    }

    //=========================================================================
    //! SUMIFS / MAXIFS / MINIFS (shared: value_range + criteria pairs)
    //=========================================================================
    tFunctionAggIfs::tFunctionAggIfs(tAggIfsKind sKind) : tFunction(), m_Kind(sKind) {}

    tStackElem tFunctionAggIfs::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // FUN(value_range, criteria_range1, criteria1, [criteria_range2, criteria2], ...)
        // In RPN: value_range criteria_range1 criteria1 ... FUN
        // After PopArgs: last element is value_range; preceding pairs are reversed.
        if (wArgs.size() < 3 || (wArgs.size() % 2) == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS aggregate requires 1 value range followed by N pairs (range,criteria)"))));
        }

        size_t wValueRangeIndex = wArgs.size() - 1;
        tRange* wValueRange = nullptr;
        tStackElem* wArgValueRange = &wArgs[wValueRangeIndex];
        if (wArgValueRange->Type() == tStackType::t_Range) {
            wValueRange = wArgValueRange->Range();
        } else {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS aggregate: first argument must be the value range"))));
        }

        struct tCriteriaPair { tRange* range; tCriterionArg criteria; };
        vector<tCriteriaPair> wPairs;
        wPairs.reserve((wArgs.size()-1)/2);

        for (int i = static_cast<int>(wValueRangeIndex) - 1; i >= 1; i -= 2) {
            size_t wIdx = static_cast<size_t>(i);
            if (wIdx >= wArgs.size() || wIdx < 1) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS aggregate: invalid argument count"))));
            }
            tStackElem* wArgCriteria = &wArgs[wIdx-1];
            tStackElem* wArgRange = &wArgs[wIdx];

            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IFS aggregate criteria range must be a range"))));
            }

            tRange* wRange = wArgRange->Range();
            tCriterionArg wCrit;
            tString wErr;
            if (!ReadCriterionArg(wArgCriteria, wCrit, wErr)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, wErr))));
            }

            wPairs.insert(wPairs.begin(), { wRange, wCrit });
        }

        tRange* wDriverRange = wPairs.front().range;

        const tIndex hLogical = wDriverRange->BottomIndex() - wDriverRange->TopIndex();
        const tIndex wLogical = wDriverRange->RightIndex() - wDriverRange->LeftIndex();

        auto sameShape = [&](tRange* r) -> tBool {
            return (r->BottomIndex() - r->TopIndex() == hLogical) && (r->RightIndex() - r->LeftIndex() == wLogical);
        };

        if (!sameShape(wValueRange)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IFS aggregate ranges must be the same size"))));
        }
        for (auto &p : wPairs) {
            if (!sameShape(p.range)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IFS aggregate criteria ranges must match sizes"))));
            }
        }

        std::vector<tCriterionArg> wCritArgs;
        wCritArgs.reserve(wPairs.size());
        for (auto &p : wPairs) wCritArgs.push_back(p.criteria);
        tIndex wOutRows = 1, wOutCols = 1;
        if (!BroadcastCriteriaShape(wCritArgs, wOutRows, wOutCols)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IFS aggregate: incompatible array criteria shapes"))));
        }

        const tIndex h = wDriverRange->IterateBottom() - wDriverRange->TopIndex();
        const tIndex w = wDriverRange->IterateRight() - wDriverRange->LeftIndex();
        const tAggIfsKind wKind = m_Kind;

        auto wComputeOne = [&](tIndex sOr, tIndex sOc) -> tVariant {
            vector<tCriteriaParser> wParsers;
            wParsers.reserve(wPairs.size());
            for (auto &p : wPairs) {
                tVariant tmp = p.criteria.At(sOr, sOc);
                wParsers.emplace_back(tmp);
            }
            tVariant wResult = 0;
            tBool wHas = false; // Max/Min: false until first numeric match (Excel returns 0 if none)
            tBool wContinue = true;
            for (tIndex dRow = 0; wContinue && dRow <= h; ++dRow) {
                for (tIndex dCol = 0; wContinue && dCol <= w; ++dCol) {
                    tBool allMatch = true;
                    for (size_t k = 0; k < wPairs.size(); ++k) {
                        tRange* r = wPairs[k].range;
                        tColRowCellRange* rCrc = r->ColRowCellRange();
                        tIndex row = r->TopIndex() + dRow;
                        tIndex col = r->LeftIndex() + dCol;
                        tCell* cell = rCrc->Cell(row, col);
                        if (cell == nullptr) { allMatch = false; break; }
                        tVariant v = cell->CalculableValue();
                        if (!wParsers[k].Match(v)) { allMatch = false; break; }
                    }
                    if (allMatch) {
                        tIndex sRow = wValueRange->TopIndex() + dRow;
                        tIndex sCol = wValueRange->LeftIndex() + dCol;
                        tCell* sCell = wValueRange->ColRowCellRange()->Cell(sRow, sCol);
                        if (sCell != nullptr) {
                            const tVariant wVal = sCell->CalculableValue();
                            if (wKind == tAggIfsKind::Sum) {
                                if (!SumIfAccumulate(wResult, wVal)) {
                                    wContinue = false;
                                }
                            } else {
                                if (!MaxMinIfAccumulate(wResult, wHas, wVal, wKind == tAggIfsKind::Max)) {
                                    wContinue = false;
                                }
                            }
                        }
                    }
                }
            }
            if (wKind != tAggIfsKind::Sum && !wHas && !wResult.IsError()) {
                return tVariant(0);
            }
            return wResult;
        };

        if (wOutRows == 1 && wOutCols == 1) {
            return tStackElem(wComputeOne(0, 0));
        }
        tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
        for (tIndex r = 0; r < wOutRows; ++r) {
            for (tIndex c = 0; c < wOutCols; ++c) {
                wOut->At(r, c) = wComputeOne(r, c);
            }
        }
        return tStackElem(wOut);
    }

    //=========================================================================
    //! Function COUNTIFS (multiple criteria)
    //=========================================================================
    tFunctionCountIfs::tFunctionCountIfs() : tFunction() {}

    tStackElem tFunctionCountIfs::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // COUNTIFS: COUNTIFS(criteria_range1, criteria1, [criteria_range2, criteria2], ...)
        // In RPN: criteria_range1 criteria1 criteria_range2 criteria2 ... COUNTIFS
        // After PopArgs: wArgs[0]=criteria2, wArgs[1]=criteria_range2, wArgs[2]=criteria1, wArgs[3]=criteria_range1, ...
        // No sum_range needed, just count matching rows
        if (wArgs.size() < 2 || (wArgs.size() % 2) != 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COUNTIFS requires pairs of (range,criteria)"))));
        }

        // Collect criteria pairs in reverse order (from end to start)
        // After PopArgs, pairs are reversed: wArgs[i]=criteria, wArgs[i+1]=criteria_range
        // We need to reverse them back to (criteria_range, criteria) order
        struct tCriteriaPair { tRange* range; tCriterionArg criteria; };
        vector<tCriteriaPair> wPairs;
        wPairs.reserve(wArgs.size()/2);

        // Process pairs from the end: (wArgs[N-2], wArgs[N-1]), (wArgs[N-4], wArgs[N-3]), ...
        // Use int to avoid underflow issues with size_t
        for (int i = static_cast<int>(wArgs.size()) - 1; i >= 1; i -= 2) {
            size_t wIdx = static_cast<size_t>(i);
            if (wIdx >= wArgs.size() || wIdx < 1) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COUNTIFS: invalid argument count"))));
            }
            tStackElem* wArgCriteria = &wArgs[wIdx-1]; // criteria (first in reversed pair)
            tStackElem* wArgRange = &wArgs[wIdx]; // criteria_range (second in reversed pair)

            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "COUNTIFS criteria range must be a range"))));
            }

            tRange* wRange = wArgRange->Range();
            tCriterionArg wCrit;
            tString wErr;
            if (!ReadCriterionArg(wArgCriteria, wCrit, wErr)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, wErr))));
            }

            // Insert at beginning to reverse the order back to original
            wPairs.insert(wPairs.begin(), { wRange, wCrit });
        }

        // Use the first criteria range as the driver for iteration and bounds
        tRange* wDriverRange = wPairs.front().range;

        // Validate all criteria ranges have same logical shape as driver
        const tIndex hLogical = wDriverRange->BottomIndex() - wDriverRange->TopIndex();
        const tIndex wLogical = wDriverRange->RightIndex() - wDriverRange->LeftIndex();

        auto sameShape = [&](tRange* r) -> tBool {
            return (r->BottomIndex() - r->TopIndex() == hLogical) && (r->RightIndex() - r->LeftIndex() == wLogical);
        };

        for (auto &p : wPairs) {
            if (!sameShape(p.range)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "COUNTIFS criteria ranges must match sizes"))));
            }
        }

        // Output shape from any array criteria (scalars stay 1x1 -> single result).
        std::vector<tCriterionArg> wCritArgs;
        wCritArgs.reserve(wPairs.size());
        for (auto &p : wPairs) wCritArgs.push_back(p.criteria);
        tIndex wOutRows = 1, wOutCols = 1;
        if (!BroadcastCriteriaShape(wCritArgs, wOutRows, wOutCols)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "COUNTIFS: incompatible array criteria shapes"))));
        }

        const tIndex h = wDriverRange->IterateBottom() - wDriverRange->TopIndex();
        const tIndex w = wDriverRange->IterateRight() - wDriverRange->LeftIndex();

        // One aggregate for the criteria selected at output cell (sOr,sOc).
        auto wComputeOne = [&](tIndex sOr, tIndex sOc) -> tVariant {
            vector<tCriteriaParser> wParsers;
            wParsers.reserve(wPairs.size());
            for (auto &p : wPairs) {
                tVariant tmp = p.criteria.At(sOr, sOc);
                wParsers.emplace_back(tmp);
            }
            tInt wCount = 0;
            for (tIndex dRow = 0; dRow <= h; ++dRow) {
                for (tIndex dCol = 0; dCol <= w; ++dCol) {
                    tBool allMatch = true;
                    for (size_t k = 0; k < wPairs.size(); ++k) {
                        tRange* r = wPairs[k].range;
                        tColRowCellRange* rCrc = r->ColRowCellRange();
                        tIndex row = r->TopIndex() + dRow;
                        tIndex col = r->LeftIndex() + dCol;
                        tCell* cell = rCrc->Cell(row, col);
                        if (cell == nullptr) { allMatch = false; break; }
                        tVariant v = cell->CalculableValue();
                        if (!wParsers[k].Match(v)) { allMatch = false; break; }
                    }
                    if (allMatch) wCount++;
                }
            }
            return tVariant(wCount);
        };

        if (wOutRows == 1 && wOutCols == 1) {
            return tStackElem(wComputeOne(0, 0));
        }
        tArrayValue* wOut = new tArrayValue(wOutRows, wOutCols);
        for (tIndex r = 0; r < wOutRows; ++r) {
            for (tIndex c = 0; c < wOutCols; ++c) {
                wOut->At(r, c) = wComputeOne(r, c);
            }
        }
        return tStackElem(wOut);
    }

    //=========================================================================
    //! Function AVERAGEIF
    //=========================================================================
    tFunctionAverageIf::tFunctionAverageIf() : tFunction() {}

    tStackElem tFunctionAverageIf::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // AVERAGEIF(range, criteria, [average_range])
        // In RPN: average_range criteria range AVERAGEIF (if 3 args) or criteria range AVERAGEIF (if 2 args)
        // After PopArgs: wArgs[0]=range, wArgs[1]=criteria, wArgs[2]=average_range (if present)
        if (wArgs.size() < 2 || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF requires 2 or 3 arguments"))));
        }
        
        tRange* wRange = nullptr; // criteria range
        tVariant wCriteria;
        tRange* wAvgRange = nullptr; // average range (optional)
        
        if (wArgs.size() == 2) {
            // AVERAGEIF(range, criteria) - 2 arguments
            // In RPN: range criteria AVERAGEIF, so after PopArgs: wArgs[0]=criteria, wArgs[1]=range
            tStackElem* wArgCriteria = &wArgs[0]; // criteria (second arg, first popped)
            tStackElem* wArgRange = &wArgs[1]; // range (first arg, last popped)
            
            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF: first argument must be a range"))));
            }
            
            wRange = wArgRange->Range();
            
            // Extract criteria (can be Variant, Cell, or Range)
            switch(wArgCriteria->Type()) {
                case tStackType::t_Variant: {
                    wCriteria = wArgCriteria->Variant();
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArgCriteria->Cell();
                    if (wCell != nullptr) {
                        wCriteria = wCell->Value();
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria cell"))));
                    }
                    break;
                }
                case tStackType::t_Range: {
                    tRange* wRangeCriteria = wArgCriteria->Range();
                    if (wRangeCriteria != nullptr) {
                        tCell* wCell = wRangeCriteria->EnsureCell();
                        if (wCell != nullptr) {
                            wCriteria = wCell->Value();
                        } else {
                            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                        }
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                    }
                    break;
                }
                default:
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF: second argument must be a value"))));
            }
            
            wAvgRange = wRange;
    #ifdef sumifdebug
            cout << "AVERAGEIF Debug - 2 arguments:" << endl;
            cout << "  Range: " << wRange->StrRef() << endl;
            cout << "  Criteria: '" << wCriteria.Str() << "'" << endl;
            cout << "  AvgRange: " << wAvgRange->StrRef() << endl;
    #endif
        } else {
            // AVERAGEIF(range, criteria, average_range) - 3 arguments
            // In RPN: range criteria average_range AVERAGEIF, so after PopArgs: wArgs[0]=average_range, wArgs[1]=criteria, wArgs[2]=range
            tStackElem* wArgAvgRange = &wArgs[0]; // average_range (third arg, first popped)
            tStackElem* wArgCriteria = &wArgs[1]; // criteria (second arg)
            tStackElem* wArgRange = &wArgs[2]; // range (first arg, last popped)
            
            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF: first argument must be a range"))));
            }
            if (wArgAvgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF: third argument must be a range"))));
            }
            
            wRange = wArgRange->Range();
            wAvgRange = wArgAvgRange->Range();
            
            // Extract criteria (can be Variant, Cell, or Range)
            switch(wArgCriteria->Type()) {
                case tStackType::t_Variant: {
                    wCriteria = wArgCriteria->Variant();
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArgCriteria->Cell();
                    if (wCell != nullptr) {
                        wCriteria = wCell->Value();
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria cell"))));
                    }
                    break;
                }
                case tStackType::t_Range: {
                    tRange* wRangeCriteria = wArgCriteria->Range();
                    if (wRangeCriteria != nullptr) {
                        tCell* wCell = wRangeCriteria->EnsureCell();
                        if (wCell != nullptr) {
                            wCriteria = wCell->Value();
                        } else {
                            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                        }
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                    }
                    break;
                }
                default:
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIF: second argument must be a value"))));
            }
    #ifdef sumifdebug
            cout << "AVERAGEIF Debug - 3 arguments:" << endl;
            cout << "  Range: " << wRange->StrRef() << endl;
            cout << "  Criteria: '" << wCriteria.Str() << "'" << endl;
            cout << "  AvgRange: " << wAvgRange->StrRef() << endl;
    #endif
        }
        
        // Clean up arguments
        
        // Validate shapes if avg range provided separately
        if (wAvgRange != nullptr) {
            const tIndex h = wRange->BottomIndex() - wRange->TopIndex();
            const tIndex w = wRange->RightIndex() - wRange->LeftIndex();
            if (!((wAvgRange->BottomIndex() - wAvgRange->TopIndex() == h) && (wAvgRange->RightIndex() - wAvgRange->LeftIndex() == w))) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "AVERAGEIF ranges must be the same size"))));
            }
        }
        
        // Parse criteria once
        tVariant tmp = wCriteria;
        tCriteriaParser parser(tmp);
    #ifdef sumifdebug
        cout << "AVERAGEIF Debug - Criteria type: "
        << (parser.m_Type == tCriteriaParser::tCriteriaType::t_Exact ? "Exact" :
            parser.m_Type == tCriteriaParser::tCriteriaType::t_Numeric ? "Numeric" :
            parser.m_Type == tCriteriaParser::tCriteriaType::t_Wildcard ? "Wildcard" : "String")
        << endl;
    #endif
        
        tColRowCellRange* crc = wRange->ColRowCellRange();
        tVariant wSum = 0;
        tInt wCount = 0;
        for (tIndex r = wRange->TopIndex(); r <= wRange->IterateBottom(); ++r) {
            for (tIndex c = wRange->LeftIndex(); c <= wRange->IterateRight(); ++c) {
                tCell* critCell = crc->Cell(r, c);
                if (!critCell) continue;
                tVariant cellVal = critCell->CalculableValue();
    #ifdef sumifdebug
                cout << "AVERAGEIF Debug - Cell: " << critCell->StrRef()
                << " Value: '" << cellVal.Str() << "'"
                << " Criteria: '" << wCriteria.Str() << "'" << endl;
    #endif
                if (parser.Match(cellVal)) {
                    // corresponding avg cell
                    tIndex dr = r - wRange->TopIndex();
                    tIndex dc = c - wRange->LeftIndex();
                    tIndex ar = wAvgRange->TopIndex() + dr;
                    tIndex ac = wAvgRange->LeftIndex() + dc;
                    tCell* avgCell = wAvgRange->ColRowCellRange()->Cell(ar, ac);
                    if (avgCell) {
                        wSum = wSum + avgCell->CalculableValue();
                        wCount += 1;
    #ifdef sumifdebug
                        cout << "AVERAGEIF Debug - Adding: " << avgCell->CalculableValue().Str()
                        << " from " << avgCell->StrRef() << endl;
    #endif
                    }
                }
            }
        }
        tDouble wRes=0;
        if (wCount == 0) {
    #ifdef sumifdebug
            cout << "AVERAGEIF Debug - No matches, returning 0" << endl;
    #endif
            return tVariant(0);
        }
        // Division as double
        if (wSum.IsDouble()) {
            wRes = wSum.Double() / wCount;
        } else {
            wRes = wSum.Int() / wCount;
        }
    #ifdef sumifdebug
        cout << "AVERAGEIF Debug - Sum: " << wSum << " Count: " << wCount << " Result: " << wRes << endl;
    #endif
        return(tStackElem(tVariant(wRes)));
    }
    //=========================================================================
    //! Constructor tFunctionAverageIfs
    //=========================================================================
    tFunctionAverageIfs::tFunctionAverageIfs(): tFunction() {
    }

    //=========================================================================
    //! Call function AVERAGEIFS
    //=========================================================================
    tStackElem tFunctionAverageIfs::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // AVERAGEIFS(average_range, criteria_range1, criteria1, criteria_range2, criteria2, ...)
        // In RPN: average_range criteria_range1 criteria1 criteria_range2 criteria2 ... AVERAGEIFS
        // After PopArgs: wArgs[0]=criteria2, wArgs[1]=criteria_range2, wArgs[2]=criteria1, wArgs[3]=criteria_range1, ..., wArgs[N-1]=average_range
        if (wArgs.size() < 3 || (wArgs.size() % 2) == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIFS requires odd number of arguments (3 or more)"))));
        }

        tRange* wAvgRange = nullptr; // average range
        std::vector<tRange*> wCriteriaRanges;
        std::vector<tVariant> wCriteria;

        // average_range is last (wArgs[wArgs.size()-1])
        size_t wAvgRangeIndex = wArgs.size() - 1;
        tStackElem* wArgAvgRange = &wArgs[wAvgRangeIndex];
        if (wArgAvgRange->Type() == tStackType::t_Range) {
            wAvgRange = wArgAvgRange->Range();
        } else {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIFS: first argument must be average_range"))));
        }

        #ifdef sumifdebug
        cout << "AVERAGEIFS Debug - Average range: " << wAvgRange->StrRef() << endl;
        cout << "AVERAGEIFS Debug - Total arguments: " << wArgs.size() << endl;
        for (size_t i = 0; i < wArgs.size(); ++i) {
            cout << "AVERAGEIFS Debug - Arg " << i << ": " 
                 << (wArgs[i].Type() == tStackType::t_Range ? "Range" : 
                     wArgs[i].Type() == tStackType::t_Variant ? "Variant" : "Other") << endl;
        }
        #endif

        // Parse criteria pairs in reverse order (from end to start, excluding average_range)
        // After PopArgs, pairs are reversed: wArgs[i]=criteria, wArgs[i+1]=criteria_range
        // We need to reverse them back to (criteria_range, criteria) order
        // Process pairs from the end: (wArgs[N-3], wArgs[N-2]), (wArgs[N-5], wArgs[N-4]), ...
        // Start from wAvgRangeIndex-1 and go down by 2, but ensure i >= 1 to access wArgs[i-1]
        // Use int to avoid underflow issues with size_t
        for (int i = static_cast<int>(wAvgRangeIndex) - 1; i >= 1; i -= 2) {
            size_t wIdx = static_cast<size_t>(i);
            if (wIdx >= wArgs.size() || wIdx < 1) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIFS: invalid argument count"))));
            }
            tStackElem* wArgCriteria = &wArgs[wIdx-1]; // criteria (first in reversed pair)
            tStackElem* wArgRange = &wArgs[wIdx]; // criteria_range (second in reversed pair)
            
            tRange* wCriteriaRange = nullptr;
            tVariant wCriteriaValue;
            
            if (wArgRange->Type() != tStackType::t_Range) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIFS criteria range must be a range"))));
            }
            
            wCriteriaRange = wArgRange->Range();
            
            // Extract criteria (can be Variant, Cell, or Range)
            switch(wArgCriteria->Type()) {
                case tStackType::t_Variant: {
                    wCriteriaValue = wArgCriteria->Variant();
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArgCriteria->Cell();
                    if (wCell != nullptr) {
                        wCriteriaValue = wCell->Value();
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria cell"))));
                    }
                    break;
                }
                case tStackType::t_Range: {
                    tRange* wRangeCriteria = wArgCriteria->Range();
                    if (wRangeCriteria != nullptr) {
                        tCell* wCell = wRangeCriteria->EnsureCell();
                        if (wCell != nullptr) {
                            wCriteriaValue = wCell->Value();
                        } else {
                            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                        }
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "Invalid criteria range"))));
                    }
                    break;
                }
                default:
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "AVERAGEIFS criteria must be a value"))));
            }
            
            // Validate range sizes match average range
            const tIndex h = wAvgRange->BottomIndex() - wAvgRange->TopIndex();
            const tIndex w = wAvgRange->RightIndex() - wAvgRange->LeftIndex();
            if (!((wCriteriaRange->BottomIndex() - wCriteriaRange->TopIndex() == h) && 
                  (wCriteriaRange->RightIndex() - wCriteriaRange->LeftIndex() == w))) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "AVERAGEIFS all ranges must be the same size"))));
            }
            
            // Insert at beginning to reverse the order back to original
            wCriteriaRanges.insert(wCriteriaRanges.begin(), wCriteriaRange);
            wCriteria.insert(wCriteria.begin(), wCriteriaValue);
            
            #ifdef sumifdebug
            cout << "AVERAGEIFS Debug - Criteria " << (wCriteriaRanges.size()) << " Range: " << wCriteriaRange->StrRef() 
                 << " Criteria: " << wCriteriaValue.String() << endl;
            #endif
        }
        
        // Clean up arguments after extracting ranges and variants

        // Parse all criteria once for optimization
        std::vector<tCriteriaParser> wParsers;
        for (const auto& crit : wCriteria) {
            tVariant tmp = crit; // Create non-const copy
            wParsers.emplace_back(tmp);
        }

        tColRowCellRange* avgCrc = wAvgRange->ColRowCellRange();
        tVariant wSum = 0;
        tInt wCount = 0;
        
        // Iterate through average range
        for (tIndex r = wAvgRange->TopIndex(); r <= wAvgRange->IterateBottom(); ++r) {
            for (tIndex c = wAvgRange->LeftIndex(); c <= wAvgRange->IterateRight(); ++c) {
                bool wAllMatch = true;
                
                // Check all criteria for this position
                for (size_t i = 0; i < wCriteriaRanges.size(); ++i) {
                    tRange* wCriteriaRange = wCriteriaRanges[i];
                    tCriteriaParser& wParser = wParsers[i];
                    
                    // Calculate offset from average range to criteria range
                    tIndex dr = r - wAvgRange->TopIndex();
                    tIndex dc = c - wAvgRange->LeftIndex();
                    tIndex cr = wCriteriaRange->TopIndex() + dr;
                    tIndex cc = wCriteriaRange->LeftIndex() + dc;
                    
                    tCell* wCriteriaCell = wCriteriaRange->ColRowCellRange()->Cell(cr, cc);
                    if (!wCriteriaCell) {
                        #ifdef sumifdebug
                        cout << "AVERAGEIFS Debug - Missing criteria cell at r=" << cr << ", c=" << cc << endl;
                        #endif
                        wAllMatch = false;
                        break;
                    }
                    
                    tVariant wCellValue = wCriteriaCell->CalculableValue();
                    bool wMatchHere = wParser.Match(wCellValue);
                    #ifdef sumifdebug
                    cout << "AVERAGEIFS Debug - Criteria " << (i+1) << ": Cell " << wCriteriaCell->StrRef()
                         << " Value='" << wCellValue.Str() << "' -> Match=" << (wMatchHere?"YES":"NO") << endl;
                    #endif
                    if (!wMatchHere) {
                        wAllMatch = false;
                        break;
                    }
                }
                
                if (wAllMatch) {
                    tCell* wAvgCell = avgCrc->Cell(r, c);
                    if (wAvgCell) {
                        tVariant wAvgValue = wAvgCell->CalculableValue();
                        wSum = wSum + wAvgValue;
                        wCount += 1;
                        
                        #ifdef sumifdebug
                        cout << "AVERAGEIFS Debug - Match at " << wAvgCell->StrRef() 
                             << " Value: " << wAvgValue.Str() << " Count: " << wCount << endl;
                        #endif
                    }
                }
            }
        }
        
        #ifdef sumifdebug
        cout << "AVERAGEIFS Debug - Final Sum: " << wSum << " Final Count: " << wCount << endl;
        #endif

        if (wCount == 0) {
            #ifdef sumifdebug
            cout << "AVERAGEIFS Debug - No matching cells, returning 0" << endl;
            #endif
            return tVariant(0); // Excel returns #DIV/0! but 0 is acceptable for now
        }
        
        tDouble wRes = 0.0;
        if (wSum.IsDouble()) {
            wRes = wSum.Double() / static_cast<tDouble>(wCount);
        } else if (wSum.IsInt()) {
            wRes = static_cast<tDouble>(wSum.Int()) / static_cast<tDouble>(wCount);
        } else {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "AVERAGEIFS sum is not numeric"))));
        }
        
        #ifdef sumifdebug
        cout << "AVERAGEIFS Debug - Result: " << wRes << endl;
        #endif
        return tVariant(wRes);
    }

    //=========================================================================
    //! Call back for function SUBTOTAL
    //=========================================================================
    tCallBackRangeSubtotal::tCallBackRangeSubtotal(tColRowCellRange* sColRowCellRange, tInt sFunctionNum) 
        : tCallBackRangeFunction(sColRowCellRange), m_FunctionNum(sFunctionNum), m_IgnoreHidden(false), m_Count(0), m_FirstCall(true) {
        // Determine if we should ignore hidden rows (function_num >= 101)
        m_IgnoreHidden = (sFunctionNum >= 101);
        // Normalize function_num to 1-11 range
        tInt wNormalizedFunc = m_IgnoreHidden ? (sFunctionNum - 100) : sFunctionNum;
        
        // Initialize based on function type
        switch(wNormalizedFunc) {
            case 1:  // AVERAGE - initialize sum to 0, count to 0
                m_Value = tVariant(0.0);
                m_Count = 0;
                break;
            case 2:  // COUNT - initialize count to 0
            case 3:  // COUNTA - initialize count to 0
                m_Value = tVariant(0);
                m_Count = 0;
                break;
            case 4:  // MAX - initialize with first value
            case 5:  // MIN - initialize with first value
                m_FirstCall = true;
                break;
            case 6:  // PRODUCT - initialize to 1
                m_Value = tVariant(1.0);
                break;
            case 7:  // STDEV
            case 8:  // STDEVP
            case 9:  // SUM - initialize to 0
                m_Value = tVariant(0.0);
                break;
            case 10: // VAR
            case 11: // VARP
                m_Value = tVariant(0.0);
                m_Count = 0;
                break;
            default:
                m_Value = tVariant(tClassError(tTypeError::t_value, "SUBTOTAL: invalid function_num"));
                break;
        }
    }

    tBool tCallBackRangeSubtotal::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell == nullptr) {
            return true;
        }

        // Excel SUBTOTAL visibility:
        // - function_num 101-111: ignore manually hidden rows/cols and outline-collapsed rows/cols
        // - function_num 1-11: still exclude rows hidden by AutoFilter (DataVisible)
        const tIndex wRow = wCell->RowIndex();
        const tIndex wCol = wCell->ColIndex();
        if (m_IgnoreHidden) {
            if (!m_ColRowCellRange->IsRowVisible(wRow) || !m_ColRowCellRange->IsColVisible(wCol)) {
                return true;
            }
        } else {
            tColRow* wRowColRow = m_ColRowCellRange->Row(wRow);
            if (wRowColRow != nullptr && !wRowColRow->DataVisible()) {
                return true;
            }
        }
        
        // Check if cell contains a SUBTOTAL formula - if so, ignore it
        const tFormula* wFormula = wCell->Formula();
        if (wFormula != nullptr) {
            tString wFormulaStr = wCell->FormulaStr();
            // Check if formula contains SUBTOTAL (case-insensitive)
            tString wFormulaUpper = wFormulaStr;
            std::transform(wFormulaUpper.begin(), wFormulaUpper.end(), wFormulaUpper.begin(), ::toupper);
            if (wFormulaUpper.find("SUBTOTAL(") != tString::npos) {
                // Skip this cell - it contains a SUBTOTAL formula
                return true;
            }
        }
        
        // Get cell value
        tVariant wCellValue = wCell->CalculableValue();
        
        // Normalize function_num to 1-11 range
        tInt wNormalizedFunc = m_IgnoreHidden ? (m_FunctionNum - 100) : m_FunctionNum;
        
        // Apply function based on function_num
        switch(wNormalizedFunc) {
            case 1: { // AVERAGE
                if (!wCellValue.IsNull()) {
                    if (wCellValue.IsDouble() || wCellValue.IsInt()) {
                        tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                        m_Value = m_Value + wNum;
                        m_Count++;
                    }
                }
                break;
            }
            case 2: { // COUNT
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    m_Count++;
                }
                break;
            }
            case 3: { // COUNTA
                if (!wCellValue.IsNull()) {
                    m_Count++;
                }
                break;
            }
            case 4: { // MAX
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    if (m_FirstCall || wNum > m_Value.Double()) {
                        m_Value = tVariant(wNum);
                        m_FirstCall = false;
                    }
                }
                break;
            }
            case 5: { // MIN
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    if (m_FirstCall || wNum < m_Value.Double()) {
                        m_Value = tVariant(wNum);
                        m_FirstCall = false;
                    }
                }
                break;
            }
            case 6: { // PRODUCT
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    if (m_Value.IsNull()) {
                        m_Value = tVariant(1.0);
                    }
                    m_Value = tVariant(m_Value.Double() * wNum);
                }
                break;
            }
            case 7: { // STDEV (sample standard deviation) - simplified implementation
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    m_Value = m_Value + wNum;
                    m_Count++;
                    // Note: Full STDEV calculation would require storing all values
                    // For now, we'll use a simplified approach
                }
                break;
            }
            case 8: { // STDEVP (population standard deviation) - simplified implementation
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    m_Value = m_Value + wNum;
                    m_Count++;
                }
                break;
            }
            case 9: { // SUM
                // SUM only adds numeric values (ignore text, errors, etc.)
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    m_Value = m_Value + wCellValue;
                }
                break;
            }
            case 10: { // VAR (sample variance) - simplified implementation
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    m_Value = m_Value + wNum;
                    m_Count++;
                }
                break;
            }
            case 11: { // VARP (population variance) - simplified implementation
                if (!wCellValue.IsNull() && (wCellValue.IsDouble() || wCellValue.IsInt())) {
                    tDouble wNum = wCellValue.IsDouble() ? wCellValue.Double() : static_cast<tDouble>(wCellValue.Int());
                    m_Value = m_Value + wNum;
                    m_Count++;
                }
                break;
            }
            default:
                return false; // Stop processing on invalid function_num
        }
        
        // For COUNT and COUNTA, set m_Value to count before returning
        if (wNormalizedFunc == 2 || wNormalizedFunc == 3) {
            m_Value = tVariant(m_Count);
        }
        // For SUM, ensure m_Value is always a Double (not Int)
        else if (wNormalizedFunc == 9 && m_Value.IsInt()) {
            m_Value = tVariant(static_cast<tDouble>(m_Value.Int()));
        }
        
        return true; // Continue processing
    }

    //=========================================================================
    //! Function SUBTOTAL
    //=========================================================================
    tFunctionSubtotal::tFunctionSubtotal() : tFunction() {}

    tStackElem tFunctionSubtotal::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        
        if (wArgs.size() < 2) {
            return tVariant(tClassError(tTypeError::t_arg, "SUBTOTAL requires at least 2 arguments"));
        }
        
        // First argument is function_num (last in reversed stack)
        tStackElem& wFuncNumArg = wArgs[wArgs.size() - 1];
        if (wFuncNumArg.Type() != tStackType::t_Variant) {
            return tVariant(tClassError(tTypeError::t_arg, "SUBTOTAL: first argument must be a number"));
        }
        
        tVariant wFuncNumVar = wFuncNumArg.Variant();
        if (!wFuncNumVar.IsInt() && !wFuncNumVar.IsDouble()) {
            return tVariant(tClassError(tTypeError::t_arg, "SUBTOTAL: first argument must be a number"));
        }
        
        tInt wFunctionNum = wFuncNumVar.IsInt() ? wFuncNumVar.Int() : static_cast<tInt>(wFuncNumVar.Double());
        
        // Validate function_num (must be 1-11 or 101-111)
        if ((wFunctionNum < 1 || wFunctionNum > 11) && (wFunctionNum < 101 || wFunctionNum > 111)) {
            return tVariant(tClassError(tTypeError::t_value, "SUBTOTAL: function_num must be 1-11 or 101-111"));
        }
        
        // Normalize function_num to 1-11 range
        tInt wNormalizedFunc = (wFunctionNum >= 101) ? (wFunctionNum - 100) : wFunctionNum;
        
        // Process remaining arguments (ranges) in reverse order
        tVariant wResult;
        // Initialize wResult based on function type
        if (wNormalizedFunc == 9) { // SUM
            wResult = tVariant(0.0);
        }
        tDouble wTotalSum = 0.0;
        tInt wTotalCount = 0;
        tBool wFirstValue = true;
        
        for (auto it = wArgs.rbegin() + 1; it != wArgs.rend(); ++it) {
            tStackElem& wArg = *it;
            
            if (wArg.Type() == tStackType::t_Variant) {
                // Single value
                tVariant wVal = wArg.Variant();
                if (!wVal.IsNull()) {
                    switch(wNormalizedFunc) {
                        case 1: { // AVERAGE
                            if (wVal.IsDouble() || wVal.IsInt()) {
                                wTotalSum += wVal.IsDouble() ? wVal.Double() : static_cast<tDouble>(wVal.Int());
                                wTotalCount++;
                            }
                            break;
                        }
                        case 2: { // COUNT
                            if (wVal.IsDouble() || wVal.IsInt()) {
                                wTotalCount++;
                            }
                            break;
                        }
                        case 3: { // COUNTA
                            wTotalCount++;
                            break;
                        }
                        case 4: { // MAX
                            if (wVal.IsDouble() || wVal.IsInt()) {
                                tDouble wNum = wVal.IsDouble() ? wVal.Double() : static_cast<tDouble>(wVal.Int());
                                if (wFirstValue || wResult.IsNull() || wNum > wResult.Double()) {
                                    wResult = tVariant(wNum);
                                    wFirstValue = false;
                                }
                            }
                            break;
                        }
                        case 5: { // MIN
                            if (wVal.IsDouble() || wVal.IsInt()) {
                                tDouble wNum = wVal.IsDouble() ? wVal.Double() : static_cast<tDouble>(wVal.Int());
                                if (wFirstValue || wResult.IsNull() || wNum < wResult.Double()) {
                                    wResult = tVariant(wNum);
                                    wFirstValue = false;
                                }
                            }
                            break;
                        }
                        case 9: { // SUM
                            // SUM only adds numeric values (ignore text, errors, etc.)
                            if (wVal.IsDouble() || wVal.IsInt()) {
                                wResult = wResult + wVal;
                            }
                            break;
                        }
                        default:
                            // For other functions, treat as SUM for now
                            wResult = wResult + wVal;
                            break;
                    }
                }
            } else if (wArg.Type() == tStackType::t_Range) {
                // Range - use callback
                tRange* wRange = wArg.Range();
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                tCallBackRangeSubtotal wCallBackSubtotal(wColRowCellRange, wFunctionNum);
                wColRowCellRange->VisitorRange(wRange, &wCallBackSubtotal);
                
                tVariant wRangeResult = wCallBackSubtotal.Value();
                
                // Combine results based on function type
                switch(wNormalizedFunc) {
                    case 1: { // AVERAGE
                        // Accumulate sum and count from callback
                        // The callback's m_Value contains the sum, m_Count contains the count
                        if (!wRangeResult.IsNull() && (wRangeResult.IsDouble() || wRangeResult.IsInt())) {
                            wTotalSum += wRangeResult.IsDouble() ? wRangeResult.Double() : static_cast<tDouble>(wRangeResult.Int());
                            wTotalCount += wCallBackSubtotal.Count();
                        }
                        break;
                    }
                    case 2: // COUNT
                    case 3: { // COUNTA
                        // Accumulate counts from callback
                        wTotalCount += wCallBackSubtotal.Count();
                        break;
                    }
                    case 4: { // MAX
                        if (!wRangeResult.IsNull() && (wRangeResult.IsDouble() || wRangeResult.IsInt())) {
                            tDouble wNum = wRangeResult.IsDouble() ? wRangeResult.Double() : static_cast<tDouble>(wRangeResult.Int());
                            if (wFirstValue || wResult.IsNull() || wNum > wResult.Double()) {
                                wResult = tVariant(wNum);
                                wFirstValue = false;
                            }
                        }
                        break;
                    }
                    case 5: { // MIN
                        if (!wRangeResult.IsNull() && (wRangeResult.IsDouble() || wRangeResult.IsInt())) {
                            tDouble wNum = wRangeResult.IsDouble() ? wRangeResult.Double() : static_cast<tDouble>(wRangeResult.Int());
                            if (wFirstValue || wResult.IsNull() || wNum < wResult.Double()) {
                                wResult = tVariant(wNum);
                                wFirstValue = false;
                            }
                        }
                        break;
                    }
                    default: { // SUM and others
                        if (!wRangeResult.IsNull()) {
                            // For SUM, only add numeric values (ignore text, errors, etc.)
                            if (wNormalizedFunc == 9) {
                                if (wRangeResult.IsDouble() || wRangeResult.IsInt()) {
                                    wResult = wResult + wRangeResult;
                                }
                            } else {
                                wResult = wResult + wRangeResult;
                            }
                        } else if (wNormalizedFunc == 9) {
                            // For SUM, if range result is null (empty range), ensure wResult is 0.0
                            if (wResult.IsNull()) {
                                wResult = tVariant(0.0);
                            }
                        }
                        break;
                    }
                }
            }
        }
        
        // Finalize result based on function type
        switch(wNormalizedFunc) {
            case 1: { // AVERAGE
                if (wTotalCount > 0) {
                    return tVariant(wTotalSum / static_cast<tDouble>(wTotalCount));
                } else {
                    return tVariant(0.0);
                }
            }
            case 2: // COUNT
            case 3: { // COUNTA
                return tVariant(wTotalCount);
            }
            case 4: // MAX
            case 5: { // MIN
                // If no values found, return error or 0
                if (wResult.IsNull() || wFirstValue) {
                    return tVariant(tClassError(tTypeError::t_value, "SUBTOTAL: no values found"));
                }
                return wResult;
            }
            default: {
                // For SUM and others, return 0 if result is null
                if (wResult.IsNull()) {
                    return tVariant(0.0);
                }
                // Ensure SUM returns a double (not int)
                if (wNormalizedFunc == 9 && wResult.IsInt()) {
                    return tVariant(static_cast<tDouble>(wResult.Int()));
                }
                return wResult;
            }
        }
    }

    namespace {

        struct tAggregateOptions {
            tBool m_IgnoreHidden;
            tBool m_IgnoreErrors;
            tBool m_IgnoreNested;
        };

        static tBool ParseAggregateOptions(tInt sOptions, tAggregateOptions& sOut) {
            if (sOptions < 0 || sOptions > 7) {
                return false;
            }
            sOut.m_IgnoreHidden = (sOptions == 1 || sOptions == 3 || sOptions == 5 || sOptions == 7);
            sOut.m_IgnoreErrors = (sOptions == 2 || sOptions == 3 || sOptions == 6 || sOptions == 7);
            sOut.m_IgnoreNested = (sOptions <= 3);
            return true;
        }

        static tBool FormulaContainsNestedAggregate(const tString& sFormula) {
            tString wUpper = sFormula;
            std::transform(wUpper.begin(), wUpper.end(), wUpper.begin(), ::toupper);
            return wUpper.find("SUBTOTAL(") != tString::npos || wUpper.find("AGGREGATE(") != tString::npos;
        }

        static tBool VariantToDouble(const tVariant& sValue, tDouble& sOut) {
            if (sValue.IsDouble()) {
                sOut = sValue.Double();
                return true;
            }
            if (sValue.IsInt()) {
                sOut = static_cast<tDouble>(sValue.Int());
                return true;
            }
            return false;
        }

        static tDouble ComputeMedian(std::vector<tDouble> sValues) {
            if (sValues.empty()) {
                return 0.0;
            }
            std::sort(sValues.begin(), sValues.end());
            const tSize wN = sValues.size();
            if ((wN % 2) == 1) {
                return sValues[wN / 2];
            }
            return (sValues[(wN / 2) - 1] + sValues[wN / 2]) / 2.0;
        }

        static tDouble ComputeModeSingle(const std::vector<tDouble>& sValues) {
            if (sValues.empty()) {
                return 0.0;
            }
            std::vector<tDouble> wSorted = sValues;
            std::sort(wSorted.begin(), wSorted.end());
            tDouble wBest = wSorted.front();
            tInt wBestCount = 1;
            tInt wCurrentCount = 1;
            for (tSize wI = 1; wI < wSorted.size(); wI++) {
                if (wSorted[wI] == wSorted[wI - 1]) {
                    wCurrentCount++;
                } else {
                    if (wCurrentCount > wBestCount) {
                        wBestCount = wCurrentCount;
                        wBest = wSorted[wI - 1];
                    }
                    wCurrentCount = 1;
                }
            }
            if (wCurrentCount > wBestCount) {
                wBest = wSorted.back();
            }
            return wBest;
        }

        static tDouble ComputeLarge(std::vector<tDouble> sValues, tInt sK) {
            if (sK < 1 || sValues.empty()) {
                return 0.0;
            }
            std::sort(sValues.begin(), sValues.end(), std::greater<tDouble>());
            if (static_cast<tSize>(sK) > sValues.size()) {
                return 0.0;
            }
            return sValues[static_cast<tSize>(sK) - 1];
        }

        static tDouble ComputeSmall(std::vector<tDouble> sValues, tInt sK) {
            if (sK < 1 || sValues.empty()) {
                return 0.0;
            }
            std::sort(sValues.begin(), sValues.end());
            if (static_cast<tSize>(sK) > sValues.size()) {
                return 0.0;
            }
            return sValues[static_cast<tSize>(sK) - 1];
        }

        static tDouble ComputePercentileInc(std::vector<tDouble> sValues, tDouble sK) {
            if (sValues.empty()) {
                return 0.0;
            }
            std::sort(sValues.begin(), sValues.end());
            const tSize wN = sValues.size();
            if (sK <= 0.0) {
                return sValues.front();
            }
            if (sK >= 1.0) {
                return sValues.back();
            }
            const tDouble wPos = (static_cast<tDouble>(wN) - 1.0) * sK;
            const tSize wLo = static_cast<tSize>(std::floor(wPos));
            const tSize wHi = static_cast<tSize>(std::ceil(wPos));
            if (wLo == wHi) {
                return sValues[wLo];
            }
            return sValues[wLo] + (wPos - static_cast<tDouble>(wLo)) * (sValues[wHi] - sValues[wLo]);
        }

        static tDouble ComputePercentileExc(std::vector<tDouble> sValues, tDouble sK) {
            if (sValues.size() < 2) {
                return 0.0;
            }
            std::sort(sValues.begin(), sValues.end());
            const tSize wN = sValues.size();
            const tDouble wPos = (static_cast<tDouble>(wN) + 1.0 / 3.0) * sK + (1.0 / 3.0);
            if (wPos <= 1.0) {
                return sValues.front();
            }
            if (wPos >= static_cast<tDouble>(wN)) {
                return sValues.back();
            }
            const tSize wLo = static_cast<tSize>(std::floor(wPos)) - 1;
            const tSize wHi = static_cast<tSize>(std::ceil(wPos)) - 1;
            const tDouble wFrac = wPos - std::floor(wPos);
            return sValues[wLo] + wFrac * (sValues[wHi] - sValues[wLo]);
        }

        static tDouble ComputeQuartileInc(std::vector<tDouble> sValues, tInt sQuart) {
            switch (sQuart) {
                case 0: return ComputeSmall(sValues, 1);
                case 1: return ComputePercentileInc(sValues, 0.25);
                case 2: return ComputeMedian(sValues);
                case 3: return ComputePercentileInc(sValues, 0.75);
                case 4: return ComputeLarge(sValues, 1);
                default: return 0.0;
            }
        }

        static tDouble ComputeQuartileExc(std::vector<tDouble> sValues, tInt sQuart) {
            switch (sQuart) {
                case 0: return ComputeSmall(sValues, 1);
                case 1: return ComputePercentileExc(sValues, 0.25);
                case 2: return ComputeMedian(sValues);
                case 3: return ComputePercentileExc(sValues, 0.75);
                case 4: return ComputeLarge(sValues, 1);
                default: return 0.0;
            }
        }

        static tDouble ComputeStdevS(const std::vector<tDouble>& sValues) {
            const tSize wN = sValues.size();
            if (wN < 2) {
                return 0.0;
            }
            tDouble wSum = 0.0;
            for (tDouble wVal : sValues) {
                wSum += wVal;
            }
            const tDouble wMean = wSum / static_cast<tDouble>(wN);
            tDouble wVar = 0.0;
            for (tDouble wVal : sValues) {
                const tDouble wDiff = wVal - wMean;
                wVar += wDiff * wDiff;
            }
            return std::sqrt(wVar / static_cast<tDouble>(wN - 1));
        }

        static tDouble ComputeStdevP(const std::vector<tDouble>& sValues) {
            const tSize wN = sValues.size();
            if (wN < 1) {
                return 0.0;
            }
            tDouble wSum = 0.0;
            for (tDouble wVal : sValues) {
                wSum += wVal;
            }
            const tDouble wMean = wSum / static_cast<tDouble>(wN);
            tDouble wVar = 0.0;
            for (tDouble wVal : sValues) {
                const tDouble wDiff = wVal - wMean;
                wVar += wDiff * wDiff;
            }
            return std::sqrt(wVar / static_cast<tDouble>(wN));
        }

        static tVariant FinalizeAggregate(tInt sFunctionNum, tInt sK,
                                          const std::vector<tDouble>& sValues, tInt sCountA) {
            switch (sFunctionNum) {
                case 1: {
                    if (sValues.empty()) {
                        return tVariant(tClassError(tTypeError::t_div0, "AGGREGATE: divide by zero"));
                    }
                    tDouble wSum = 0.0;
                    for (tDouble wVal : sValues) {
                        wSum += wVal;
                    }
                    return tVariant(wSum / static_cast<tDouble>(sValues.size()));
                }
                case 2:
                    return tVariant(static_cast<tInt>(sValues.size()));
                case 3:
                    return tVariant(sCountA);
                case 4:
                    if (sValues.empty()) {
                        return tVariant(tClassError(tTypeError::t_value, "AGGREGATE: no values"));
                    }
                    return tVariant(*std::max_element(sValues.begin(), sValues.end()));
                case 5:
                    if (sValues.empty()) {
                        return tVariant(tClassError(tTypeError::t_value, "AGGREGATE: no values"));
                    }
                    return tVariant(*std::min_element(sValues.begin(), sValues.end()));
                case 6: {
                    if (sValues.empty()) {
                        return tVariant(0.0);
                    }
                    tDouble wProduct = 1.0;
                    for (tDouble wVal : sValues) {
                        wProduct *= wVal;
                    }
                    return tVariant(wProduct);
                }
                case 7:
                    return tVariant(ComputeStdevS(sValues));
                case 8:
                    return tVariant(ComputeStdevP(sValues));
                case 9: {
                    tDouble wSum = 0.0;
                    for (tDouble wVal : sValues) {
                        wSum += wVal;
                    }
                    return tVariant(wSum);
                }
                case 10: {
                    const tDouble wStdev = ComputeStdevS(sValues);
                    return tVariant(wStdev * wStdev);
                }
                case 11: {
                    const tDouble wStdev = ComputeStdevP(sValues);
                    return tVariant(wStdev * wStdev);
                }
                case 12:
                    if (sValues.empty()) {
                        return tVariant(tClassError(tTypeError::t_num, "AGGREGATE: no values"));
                    }
                    return tVariant(ComputeMedian(sValues));
                case 13:
                    if (sValues.empty()) {
                        return tVariant(tClassError(tTypeError::t_na, "AGGREGATE: no values"));
                    }
                    return tVariant(ComputeModeSingle(sValues));
                case 14:
                    return tVariant(ComputeLarge(sValues, sK));
                case 15:
                    return tVariant(ComputeSmall(sValues, sK));
                case 16:
                    return tVariant(ComputePercentileInc(sValues, static_cast<tDouble>(sK)));
                case 17:
                    return tVariant(ComputeQuartileInc(sValues, sK));
                case 18:
                    return tVariant(ComputePercentileExc(sValues, static_cast<tDouble>(sK)));
                case 19:
                    return tVariant(ComputeQuartileExc(sValues, sK));
                default:
                    return tVariant(tClassError(tTypeError::t_value, "AGGREGATE: invalid function_num"));
            }
        }

        static tBool CollectAggregateScalar(const tVariant& sValue, tBool sIgnoreErrors,
                                            std::vector<tDouble>& sValues, tInt& sCountA) {
            if (sValue.IsError()) {
                return !sIgnoreErrors;
            }
            if (!sValue.IsNull()) {
                sCountA++;
                tDouble wNum = 0.0;
                if (VariantToDouble(sValue, wNum)) {
                    sValues.push_back(wNum);
                }
            }
            return true;
        }

    } // namespace

    //=========================================================================
    //! Call back for function AGGREGATE
    //=========================================================================
    tCallBackRangeAggregate::tCallBackRangeAggregate(tColRowCellRange* sColRowCellRange,
                                                     tBool sIgnoreHidden, tBool sIgnoreErrors, tBool sIgnoreNested)
        : tCallBackRangeFunction(sColRowCellRange),
          m_IgnoreHidden(sIgnoreHidden),
          m_IgnoreErrors(sIgnoreErrors),
          m_IgnoreNested(sIgnoreNested),
          m_CountA(0) {}

    tBool tCallBackRangeAggregate::CallBack(tAllocatorRef sAllocatorRef) {
        tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
        if (wCell == nullptr) {
            return true;
        }

        const tIndex wRow = wCell->RowIndex();
        const tIndex wCol = wCell->ColIndex();
        if (m_IgnoreHidden) {
            if (!m_ColRowCellRange->IsRowVisible(wRow) || !m_ColRowCellRange->IsColVisible(wCol)) {
                return true;
            }
        }

        const tFormula* wFormula = wCell->Formula();
        if (m_IgnoreNested && wFormula != nullptr) {
            if (FormulaContainsNestedAggregate(wCell->FormulaStr())) {
                return true;
            }
        }

        const tVariant wCellValue = wCell->CalculableValue();
        if (wCellValue.IsError() && m_IgnoreErrors) {
            return true;
        }
        if (!wCellValue.IsNull()) {
            m_CountA++;
            tDouble wNum = 0.0;
            if (VariantToDouble(wCellValue, wNum)) {
                m_NumericValues.push_back(wNum);
            }
        }
        return true;
    }

    //=========================================================================
    //! Function AGGREGATE
    //=========================================================================
    tFunctionAggregate::tFunctionAggregate() : tFunction() {}

    tStackElem tFunctionAggregate::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);

        if (wArgs.size() < 3) {
            return tVariant(tClassError(tTypeError::t_arg, "AGGREGATE requires at least 3 arguments"));
        }

        tStackElem& wFuncNumArg = wArgs[wArgs.size() - 1];
        tStackElem& wOptionsArg = wArgs[wArgs.size() - 2];
        if (wFuncNumArg.Type() != tStackType::t_Variant || wOptionsArg.Type() != tStackType::t_Variant) {
            return tVariant(tClassError(tTypeError::t_arg, "AGGREGATE: first two arguments must be numbers"));
        }

        tVariant wFuncNumVar = wFuncNumArg.Variant();
        tVariant wOptionsVar = wOptionsArg.Variant();
        if ((!wFuncNumVar.IsInt() && !wFuncNumVar.IsDouble())
            || (!wOptionsVar.IsInt() && !wOptionsVar.IsDouble())) {
            return tVariant(tClassError(tTypeError::t_arg, "AGGREGATE: first two arguments must be numbers"));
        }

        const tInt wFunctionNum = wFuncNumVar.IsInt() ? wFuncNumVar.Int() : static_cast<tInt>(wFuncNumVar.Double());
        const tInt wOptions = wOptionsVar.IsInt() ? wOptionsVar.Int() : static_cast<tInt>(wOptionsVar.Double());

        if (wFunctionNum < 1 || wFunctionNum > 19) {
            return tVariant(tClassError(tTypeError::t_value, "AGGREGATE: function_num must be 1-19"));
        }

        tAggregateOptions wAggregateOptions;
        if (!ParseAggregateOptions(wOptions, wAggregateOptions)) {
            return tVariant(tClassError(tTypeError::t_value, "AGGREGATE: options must be 0-7"));
        }

        const tBool wNeedsK = (wFunctionNum >= 14 && wFunctionNum <= 19);
        if (wNeedsK && wArgs.size() < 4) {
            return tVariant(tClassError(tTypeError::t_arg, "AGGREGATE: function requires k argument"));
        }

        tInt wK = 0;
        tSize wRangeBegin = 0;
        if (wNeedsK) {
            tVariant wKVar;
            if (!StackElemToVariant(wArgs[0], wKVar) || (!wKVar.IsInt() && !wKVar.IsDouble())) {
                return tVariant(tClassError(tTypeError::t_arg, "AGGREGATE: k must be a number"));
            }
            wK = wKVar.IsInt() ? wKVar.Int() : static_cast<tInt>(wKVar.Double());
            wRangeBegin = 1;
        }

        std::vector<tDouble> wNumericValues;
        tInt wCountA = 0;

        for (tSize wI = wRangeBegin; wI + 2 < wArgs.size(); wI++) {
            tStackElem& wArg = wArgs[wI];
            if (wArg.Type() == tStackType::t_Range) {
                tRange* wRange = wArg.Range();
                if (wRange == nullptr) {
                    continue;
                }
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                tCallBackRangeAggregate wCallBack(wColRowCellRange,
                    wAggregateOptions.m_IgnoreHidden, wAggregateOptions.m_IgnoreErrors, wAggregateOptions.m_IgnoreNested);
                wColRowCellRange->VisitorRange(wRange, &wCallBack);
                wCountA += wCallBack.CountA();
                const std::vector<tDouble>& wCollected = wCallBack.NumericValues();
                wNumericValues.insert(wNumericValues.end(), wCollected.begin(), wCollected.end());
            } else {
                tVariant wVal;
                if (!StackElemToVariant(wArg, wVal)) {
                    continue;
                }
                tInt wLocalCountA = 0;
                if (!CollectAggregateScalar(wVal, wAggregateOptions.m_IgnoreErrors, wNumericValues, wLocalCountA)) {
                    if (!wAggregateOptions.m_IgnoreErrors) {
                        return tStackElem(wVal);
                    }
                } else {
                    wCountA += wLocalCountA;
                }
            }
        }

        return tStackElem(FinalizeAggregate(wFunctionNum, wK, wNumericValues, wCountA));
    }

    // SpillKind (single scalar from ranges; see tFunctionSpillKind)
    tFunctionSpillKind tFunctionCount::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionCountA::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionCountBlank::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionIndex::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionOffset::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionDataRange::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionJson::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionLookup::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionVLookup::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionHLookup::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionXLookup::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionXMatch::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionMatch::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionRow::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionColumn::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionRows::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionColumns::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionAddress::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionIndirect::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionSumIf::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionCountIf::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionAggIfs::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionCountIfs::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionAverageIf::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionAverageIfs::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionSubtotal::SpillKind() const { return(tFunctionSpillKind::Aggregate); }
    tFunctionSpillKind tFunctionAggregate::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

} // End of namespace
