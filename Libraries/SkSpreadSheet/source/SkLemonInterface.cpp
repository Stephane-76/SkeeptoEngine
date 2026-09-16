//=============================================================================
// SkLemonFormula Formula SpreadSheet interface width Lemon
//=============================================================================
#include "../include/SkLemonInterface.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkLemonSpreadSheet.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkLexerData.hpp"
#include "../include/SkRangeNamed.hpp"
#include "../include/SkWorkBook.hpp"

#include <cctype>
#include <cstring>
#include <cstdint>
#include <algorithm>

#define _debuglex
#define _debugpush

#define _debugspill

namespace  SkSpreadSheet {

namespace {

// Inline LAMBDA desugaring helpers ---------------------------------------------------------------
// Hidden-name prefix for named lambdas synthesized from an inline LAMBDA(...) (see DesugarInlineLambdas).
// Kept in sync with tFormula::Str (display expansion) and JsonFormulaNamed (persistence skip).
const char* const kInlineLambdaPrefix = "_INLLMB_";

// Identifier-char test for word-boundary detection around the LAMBDA keyword. UTF8 lead/continuation
// bytes count as identifier chars, mirroring tLexer::is_identifier_char.
inline tBool DesugarIsIdentChar(tChar c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || ((c & 0x80) != 0);
}

// Excel Table[[Col1]:[Col2]] is the full data block; Table[[#This Row],[Col1]:[Col2]] is one row.
// The Col2 branch used to always pin both ends to the formula row, so TableA1[[RANK]:[RANK]]
// became a single cell and MATCH(POS, that cell, 0) returned #N/A on Part A Table A3.
tBool TableSpecRowSpan(tTypeData sKey, tIndex sCurrentRow, tIndex sTableTop, tIndex sTableBottom,
                       tBool sHasHeaders, tIndex& sOutTop, tIndex& sOutBottom) {
    switch (sKey) {
        case tTypeData::t_ThisRow:
            if (sCurrentRow > sTableTop && sCurrentRow <= sTableBottom) {
                sOutTop = sCurrentRow;
                sOutBottom = sCurrentRow;
                return true;
            }
            return false;
        case tTypeData::t_Headers:
            sOutTop = sTableTop;
            sOutBottom = sTableTop;
            return true;
        case tTypeData::t_Totals:
            sOutTop = sTableBottom + 1;
            sOutBottom = sTableBottom + 1;
            return true;
        case tTypeData::t_All:
            sOutTop = sTableTop;
            sOutBottom = sTableBottom;
            return true;
        case tTypeData::t_Column:
        default: {
            tIndex wTop = sTableTop;
            if (sHasHeaders && sTableBottom > sTableTop) {
                wTop = sTableTop + 1;
            }
            sOutTop = wTop;
            sOutBottom = sTableBottom;
            return true;
        }
    }
}

// Index of the ')' matching the '(' at sOpen, honouring nested () and "..." strings; npos if unbalanced.
tSize DesugarMatchParen(const tString& sCode, tSize sOpen) {
    tInt wDepth = 0;
    tBool wInString = false;
    for (tSize k = sOpen; k < sCode.size(); ++k) {
        const tChar c = sCode[k];
        if (wInString) { if (c == '"') wInString = false; continue; }
        if (c == '"') { wInString = true; continue; }
        if (c == '(') { ++wDepth; }
        else if (c == ')') { --wDepth; if (wDepth == 0) return (k); }
    }
    return (tString::npos);
}

// Deterministic hidden name for an inline LAMBDA (FNV-1a 64-bit, uppercase hex) computed from sKey.
// sKey MUST include both the LAMBDA(...) text AND the captured-name set: the same span in two different
// enclosing scopes (closure vs not, or capturing different vars) compiles differently and must not alias.
// Deterministic within a run is enough: hidden lambdas are never persisted (formulas re-desugar on load).
tString DesugarHiddenName(const tString& sKey) {
    std::uint64_t wHash = 1469598103934665603ULL;
    for (unsigned char c : sKey) {
        wHash ^= static_cast<std::uint64_t>(c);
        wHash *= 1099511628211ULL;
    }
    static const char* const kHex = "0123456789ABCDEF";
    tString wName = kInlineLambdaPrefix;
    for (tInt wShift = 60; wShift >= 0; wShift -= 4) {
        wName.push_back(kHex[(wHash >> wShift) & 0xF]);
    }
    return (wName);
}

// True if sName occurs as a whole-word identifier in sSpan (case-insensitive, honouring "..." strings).
// Used to keep an inline lambda's captured set minimal: only outer names its body actually mentions.
tBool SpanReferencesName(const tString& sSpan, const tString& sName) {
    if (sName.empty()) return (false);
    const tSize wLen = sName.size();
    tBool wInString = false;
    for (tSize i = 0; i + wLen <= sSpan.size(); ++i) {
        const tChar c = sSpan[i];
        if (wInString) { if (c == '"') wInString = false; continue; }
        if (c == '"') { wInString = true; continue; }
        const tBool wBoundaryBefore = (i == 0) || !DesugarIsIdentChar(sSpan[i - 1]);
        const tBool wBoundaryAfter  = (i + wLen >= sSpan.size()) || !DesugarIsIdentChar(sSpan[i + wLen]);
        if (!wBoundaryBefore || !wBoundaryAfter) continue;
        tBool wMatch = true;
        for (tSize k = 0; k < wLen; ++k) {
            if (std::toupper(static_cast<unsigned char>(sSpan[i + k])) !=
                std::toupper(static_cast<unsigned char>(sName[k]))) { wMatch = false; break; }
        }
        if (wMatch) return (true);
    }
    return (false);
}

// Collect the LET binding names of sCode into sOut (uppercased, appended — never cleared). Shared by
// tLemonInterface::CollectLetNames and the inline-lambda desugarer (to know the enclosing closure names).
void CollectLetNamesInto(const tChar* sCode, std::set<tString>& sOut) {
    if (sCode == nullptr) return;

    tLexer wLex(sCode);
    tLocale* wLocale = tApplication::Instance()->Locale();
    wLex.SeparatorArg(wLocale->Arg());
    wLex.SeparatorDecimal(wLocale->Decimal());

    // Tokenize once so we get a cheap one-token lookahead (needed to tell a binding name from the body).
    std::vector<tLexerToken> wToks;
    for (tLexerToken wT = wLex.next();
         !wT.is_one_of(tKind::End, tKind::Unexpected);
         wT = wLex.next()) {
        wToks.push_back(wT);
    }

    auto wUpper = [](tString s) -> tString {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s;
    };

    // Track nested LET( ... ) by parenthesis depth and per-LET argument index.
    struct tLetCtx { tInt m_Depth; tInt m_ArgIndex; };
    std::vector<tLetCtx> wLetStack;
    tInt wDepth = 0;
    tBool wPendingLet = false; // previous identifier was "LET"; its '(' opens a LET context

    for (std::size_t i = 0; i < wToks.size(); ++i) {
        const tKind wK = wToks[i].Kind();
        if (wK == tKind::LeftParen) {
            wDepth++;
            if (wPendingLet) {
                wLetStack.push_back(tLetCtx{ wDepth, 0 });
                wPendingLet = false;
            }
            continue;
        }
        if (wK == tKind::RightParen) {
            if (!wLetStack.empty() && wLetStack.back().m_Depth == wDepth) {
                wLetStack.pop_back();
            }
            if (wDepth > 0) wDepth--;
            wPendingLet = false;
            continue;
        }
        if (wK == tKind::Comma || wK == tKind::Semicolon) {
            if (!wLetStack.empty() && wLetStack.back().m_Depth == wDepth) {
                wLetStack.back().m_ArgIndex++;
            }
            wPendingLet = false;
            continue;
        }
        if (wK == tKind::Identifier) {
            const tString wUp = wUpper(wToks[i].Lexeme());
            if (!wLetStack.empty()
                && wLetStack.back().m_Depth == wDepth
                && (wLetStack.back().m_ArgIndex % 2) == 0
                && i + 1 < wToks.size()) {
                const tKind wNext = wToks[i + 1].Kind();
                if (wNext == tKind::Comma || wNext == tKind::Semicolon) {
                    sOut.insert(wUp);
                }
            }
            wPendingLet = (wUp == "LET");
            continue;
        }
        wPendingLet = false;
    }
}

// Excel: MyName_R[-1]C[2] may peel as MyName_ + R[…] while the defined name is MyName.
// Try every R that starts a valid R1C1 token; pick the longest defined name (Excel longest-match).
tRange* ResolveNamedRangeForR1C1Offset(tWorkBook* sWorkBook, tLexerToken* sNameToken,
                                       tLexerToken* sRcToken) {
    if (sWorkBook == nullptr || sNameToken == nullptr || sRcToken == nullptr) {
        return nullptr;
    }
    const tString wFull = sNameToken->Lexeme() + sRcToken->Lexeme();
    tRange* wBest = nullptr;
    tSize wBestLen = 0;
    for (tSize i = 0; i < wFull.size(); ++i) {
        if (wFull[i] != 'R') {
            continue;
        }
        tLexer wLex(wFull.c_str() + i);
        tLexerToken wRcTok = wLex.next();
        if (!wRcTok.is(tKind::Cell)) {
            continue;
        }
        const tString wCandidate = wFull.substr(0, i);
        if (wCandidate.empty()) {
            continue;
        }
        auto wTryResolve = [&](const tString& sName) -> tRange* {
            return sName.empty() ? nullptr : sWorkBook->FindRangeNamed(sName);
        };
        tRange* wRange = wTryResolve(wCandidate);
        tSize wMatchLen = wCandidate.size();
        if (wRange == nullptr && !wCandidate.empty() && wCandidate.back() == '_') {
            const tString wTrim = wCandidate.substr(0, wCandidate.size() - 1);
            if ((wRange = wTryResolve(wTrim)) != nullptr) {
                wMatchLen = wTrim.size();
            }
        }
        if (wRange != nullptr && wMatchLen > wBestLen) {
            wBest = wRange;
            wBestLen = wMatchLen;
        }
    }
    return wBest;
}

} // namespace

// True if the named definition might produce a multi-cell spill (avoid Calculation() for scalar names during compile).
// Two shapes spill in practice:
//   1. Literal array constructor: {1,2;3,4}
//   2. Range-returning function call: OFFSET(...), INDEX(...), INDIRECT(...), CHOOSE(...) over ranges, FILTER(...),
//      SORT(...), UNIQUE(...), SEQUENCE(...), TRANSPOSE(...). The Excel "Budget" workbook (lstAnnées / lstMesures)
//      relies on OFFSET, so missing this case made dependents like MATCH(...,lstAnnées,0) compile against the
//      stub _$$ host cell (1 cell, single value) and crash with #ARG / #REF.
// We don't try to evaluate the formula here; we only need a cheap textual heuristic. The downside of a false
// positive is a single extra Calculation() / Compil() pass on a scalar named formula, which is cheap.
static tBool NamedDefinitionMayProduceMatrixSpill(tCell* sCell, const tString& sFormulaStr) {
    if (sCell == nullptr || sFormulaStr.empty()) {
        return false;
    }
    // Literal array formula — guaranteed multi-cell.
    if (sFormulaStr.find('{') != tString::npos) {
        return true;
    }
    // Quick uppercase scan over function-name prefixes (case-sensitive in OOXML/.sker, but accept any case for the
    // user-typed live path). We look for "<NAME>(" so we don't match inside string literals or named ranges.
    static const char* const kSpillingFunctions[] = {
        "OFFSET(", "INDEX(", "INDIRECT(", "CHOOSE(", "FILTER(", "SORT(", "SORTBY(",
        "UNIQUE(", "SEQUENCE(", "RANDARRAY(", "TRANSPOSE(", "MMULT(", "MINVERSE(", "TEXTSPLIT(", "TOROW(", "TOCOL(",
        "CHOOSECOLS(", "CHOOSEROWS(",
        "VSTACK(", "HSTACK(", "TAKE(", "DROP(", "EXPAND(", "WRAPROWS(", "WRAPCOLS(",
        "MUNIT(", "TRIMRANGE(", "FREQUENCY(", "TREND(", "GROWTH(", "LINEST(", "MODE_MULT(",
        "BYROW(", "BYCOL(", "MAKEARRAY(", "MAP(", "SCAN("
    };
    // Manual case-insensitive search to avoid mutating the string.
    auto wEqualsIgnoreCase = [](const char* sNeedle, const tString& sHay, std::size_t sFromOffset) -> tBool {
        const std::size_t wHayLen = sHay.size();
        for (std::size_t i = 0; sNeedle[i] != '\0'; ++i) {
            const std::size_t wPos = sFromOffset + i;
            if (wPos >= wHayLen) return false;
            const char a = sNeedle[i];
            const char b = sHay[wPos];
            const char wA = (a >= 'a' && a <= 'z') ? static_cast<char>(a - 32) : a;
            const char wB = (b >= 'a' && b <= 'z') ? static_cast<char>(b - 32) : b;
            if (wA != wB) return false;
        }
        return true;
    };
    for (const char* wFn : kSpillingFunctions) {
        for (std::size_t wStart = 0; wStart < sFormulaStr.size(); ++wStart) {
            // Must be at start or after a non-identifier char so "MYOFFSET(" does not match "OFFSET(".
            if (wStart > 0) {
                const char wPrev = sFormulaStr[wStart - 1];
                const tBool wIsIdent =
                    (wPrev >= 'A' && wPrev <= 'Z') || (wPrev >= 'a' && wPrev <= 'z') ||
                    (wPrev >= '0' && wPrev <= '9') || wPrev == '_' || wPrev == '.';
                if (wIsIdent) continue;
            }
            if (wEqualsIgnoreCase(wFn, sFormulaStr, wStart)) {
                return true;
            }
        }
    }
    return false;
}

/// Multi-cell spill range for a named formula definition cell (Excel-like: the name refers to the full array).
    tRange* MultiCellSpillRangeForFormulaNamed(tFormulaNamed* sFormulaNamed,
                                               tInterfaceCompil* sInterfaceCompil) {
    if (sFormulaNamed == nullptr) {
        return nullptr;
    }
    tCell* wCell = sFormulaNamed->Cell();
    if (wCell == nullptr) {
        return nullptr;
    }
    // After JSON load (or any path where only the formula string was restored), SpillRange may still be null
    // when compiling dependents like =list. Compile lazily (sCalculate=false): the late-binding path in
    // tCell::InternalCalculation pushes SpillRange instead of Cell at evaluation time, so dependents like
    // MATCH(...,lstAnnées,0) get the right tRange argument once lstAnnées itself is calculated through the
    // normal RecalculateAll pass. tFormulaNamed::Compil with sCalculate=true would call Cell()->Calculation()
    // internally — that triggered cross-sheet memory access before all sheets were fully loaded (WASM
    // "memory access out of bounds" on Budget.sker ReadJson). Pass false here, RecalculateAll runs later.
    if (sFormulaNamed->SpillRange() == nullptr) {
        const tString wFsCell = wCell->FormulaStr();
        const tString wFsNamed = sFormulaNamed->FormulaStr();
        const tString wFsEarly = wFsCell.empty() ? wFsNamed : wFsCell;
        if (!wFsEarly.empty() && NamedDefinitionMayProduceMatrixSpill(wCell, wFsEarly)) {
            if (wCell->Formula() == nullptr) {
                (void)sFormulaNamed->Compil("", false);
            }
        }
    }
    // Already computed spill rect on the named formula (no compile-time Calculation).
    tRange* wRegistered = sFormulaNamed->SpillRange();
    if (wRegistered != nullptr) {
        if (!wRegistered->IsCell()) {
            return wRegistered;
        }
        return nullptr;
    }
    tRange* wSpill = wCell->MatrixRange();
#ifdef debugspill
    tCell* wCellDebug=dynamic_cast<tCell*>(sInterfaceCompil);
    if (wCellDebug!=nullptr) cout <<  "Spill on Cell" <<  wCell->StrRef(true) << "-->"  <<  wCellDebug->StrRef(true);
    if (wSpill!=nullptr) cout << " Range " << wSpill->StrRef(true);
    cout   << endl;
#endif
    if (wSpill != nullptr && !wSpill->IsCell()) {
        return wSpill;
    }
    // 1x1 or null MatrixRange is not final: literal-array names may not have expanded yet (e.g. JSON load
    // before CompilCell; tFormulaNamed still holds the definition string).
    const tString wFsCell = wCell->FormulaStr();
    const tString wFsNamed = sFormulaNamed->FormulaStr();
    const tString wFs = wFsCell.empty() ? wFsNamed : wFsCell;
    if (!wFs.empty() && NamedDefinitionMayProduceMatrixSpill(wCell, wFs)) {
        if (wCell->Formula() == nullptr) {
            tSheet* wSheet = wCell->Sheet();
            if (wSheet != nullptr) {
                tWorkBook* wWorkBook = wSheet->WorkBook();
                if (wWorkBook != nullptr) {
                    // wSpill is MatrixRange() at this point (often null or 1x1 until CompilNamedFormulaSpill runs); stored for future use.
                    //wWorkBook->AddNamedFormulaSpill(wCell, wFs, wSpill);
                }
            }
        }
    }
    wRegistered = sFormulaNamed->SpillRange();
    if (wRegistered != nullptr && !wRegistered->IsCell()) {
        return wRegistered;
    }
    wSpill = wCell->MatrixRange();
    if (wSpill != nullptr && !wSpill->IsCell()) {
        return wSpill;
    }
    return nullptr;
}

#define  SkNulllptr nullptr

    // For treatment of stack =====================================================
    tLemonFunctionMethod::tLemonFunctionMethod() : tClass(), m_Name(""), m_NbArg(0), m_IsMethod(false),m_Function(nullptr), m_DynamicRight(nullptr), m_IsIf(false) {}
    
    tLemonFunctionMethod::tLemonFunctionMethod(const tLemonFunctionMethod& sLemonFunction) : tClass() ,
        m_Name(sLemonFunction.m_Name),
        m_NbArg(sLemonFunction.m_NbArg),
		m_IsMethod(sLemonFunction.m_IsMethod),
        m_Function(nullptr),
        m_DynamicRight(sLemonFunction.m_DynamicRight),
        m_IsIf(sLemonFunction.m_IsIf),
        m_ArgEndPos(sLemonFunction.m_ArgEndPos) {}

	tLemonFunctionMethod::tLemonFunctionMethod(tString sName,tLexerToken* sDynamicRight, tBool sIsMethod) : tClass(), m_Name(sName), m_NbArg(0),m_IsMethod(sIsMethod), m_Function(nullptr), m_DynamicRight(sDynamicRight), m_IsIf(false) {
        // Detect the built-in IF (case-insensitive) so it is compiled to short-circuit opcodes. A method call
        // (obj.IF(...)) is never the built-in IF.
        tString wUpper = sName;
        std::transform(wUpper.begin(), wUpper.end(), wUpper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        m_IsIf = (wUpper == "IF") && !sIsMethod;
    }

    tFunction*  tLemonFunctionMethod::Function() {
        if (m_Function==nullptr) {
            tFunctionDictionary* wFunctionDictionary=tSpreadSheetContainer::Instance()->FunctionDictionary();
            tFunctionRef wFunctionRef=wFunctionDictionary->FunctionRef(m_Name);
            m_Function=wFunctionRef.Function(); // Empty return nullptr
        }
        return(m_Function);
    }

    tString tLemonFunctionMethod::Name() { return(m_Name); }
    void tLemonFunctionMethod::Name(tString sName) { m_Name = sName; }

    tInt tLemonFunctionMethod::NbArg() { return(m_NbArg); }
    void tLemonFunctionMethod::IncArg() { m_NbArg++; }

    tLexerToken* tLemonFunctionMethod::DynamicRight() { return(m_DynamicRight); }
    void tLemonFunctionMethod::DynamicRight(tLexerToken* sDynamicRight) { m_DynamicRight = sDynamicRight; }

    tBool tLemonFunctionMethod::IsIf() const { return(m_IsIf); }
    void tLemonFunctionMethod::AddArgEndPos(tInt sPos) { m_ArgEndPos.push_back(sPos); }
    const tVectorInt& tLemonFunctionMethod::ArgEndPos() const { return(m_ArgEndPos); }

    // Class interface with Lemon ================================================
	tLemonInterface::tLemonInterface() : tClass(), m_LemonReserved(), m_WorkBook(nullptr), m_InterfaceCompil(), m_LemonParser(nullptr), m_CompilError(tErrorFormula::t_None),m_Code(""), m_Error(""),m_ErrorLine(0),m_ErrorColumn(0) {
        tLocale* wLocale=tApplication::Instance()->Locale();
        m_Decimal=wLocale->Decimal();
        m_Arg=wLocale->Arg();
    }

	void tLemonInterface::Clear() {
        m_CompilError=tErrorFormula::t_None;
		m_Code="";
        m_Error="";
        ClearCompil();
        // RangeData / column pointers are per workbook Json load; stale cache caused wrong
        // "column … don't exist" across cells (e.g. Dépenses after a failed lookup cached nullptr).
        ClearTableDataCache();
	}

	void tLemonInterface::ClearCompil() {
		m_CurrentFormula.Clear();
		while (!m_StackFunction.empty()) m_StackFunction.pop();
	}

	void tLemonInterface::ClearTableDataCache() {
		m_RangeDataCache.clear();
		m_ColumnDataByTableAndName.clear();
	}

	tRangeData* tLemonInterface::CachedRangeData(const tString& sTableName) {
		if (m_WorkBook == nullptr) {
			return nullptr;
		}
		auto wIt = m_RangeDataCache.find(sTableName);
		if (wIt != m_RangeDataCache.end()) {
			return wIt->second;
		}
		tRangeData* wRd = m_WorkBook->RangeData(sTableName);
		m_RangeDataCache.emplace(sTableName, wRd);
		return wRd;
	}

	tColumnData* tLemonInterface::CachedFindColumnByName(tRangeData* sRangeData, tSheet* sSheet, tRange* sTableRange,
	                                                     const tString& sTableName, const tString& sColumnName) {
		if (sRangeData == nullptr || sColumnName.empty()) {
			return nullptr;
		}
		return sRangeData->FindColumnByName(sSheet, sTableRange, sColumnName);
	}

    /// @brief        Return Sheet by name ( if sName is empty retturn current sheet .
    /// @param[in]    sSheetName tString
    /// @return       tSheet*
    tSheet* tLemonInterface::Sheet(tString sSheetName) {
        if (sSheetName=="") {
            return(m_InterfaceCompil->Sheet());
        }
        tSheet* wSheet=m_InterfaceCompil->Sheet()->WorkBook()->Sheet(sSheetName);
        return(wSheet);
    }

    tBool tLemonInterface::IsInlineLambdaName(const tString& sName) {
        return (sName.rfind(kInlineLambdaPrefix, 0) == 0);
    }

    tString tLemonInterface::DesugarInlineLambdas(tWorkBook* sWorkBook, const tString& sCode) {
        if (sWorkBook == nullptr) {
            return (sCode);
        }
        // Fast path: skip formulas that cannot contain the LAMBDA keyword (case-insensitive).
        tBool wMaybe = false;
        for (tSize i = 0; i + 6 <= sCode.size(); ++i) {
            if ((sCode[i] == 'l' || sCode[i] == 'L') &&
                (sCode[i+1] == 'a' || sCode[i+1] == 'A') && (sCode[i+2] == 'm' || sCode[i+2] == 'M') &&
                (sCode[i+3] == 'b' || sCode[i+3] == 'B') && (sCode[i+4] == 'd' || sCode[i+4] == 'D') &&
                (sCode[i+5] == 'a' || sCode[i+5] == 'A')) { wMaybe = true; break; }
        }
        if (!wMaybe) {
            return (sCode);
        }
        tRangeNamedContainer* wContainer = sWorkBook->RangeNamedContainer();
        if (wContainer == nullptr) {
            return (sCode);
        }
        // Enclosing closure names available to inline lambdas found here: the caller's injected scope
        // (enclosing lambda parameters, still set on this interface at desugar entry) plus the LET binding
        // names of this very formula. An inline lambda captures those of these names that its body mentions.
        std::set<tString> wEnclosingNames = m_InjectedScopeNames;
        CollectLetNamesInto(sCode.c_str(), wEnclosingNames);
        tString wResult;
        wResult.reserve(sCode.size());
        const tSize n = sCode.size();
        tSize i = 0;
        tBool wInString = false;
        while (i < n) {
            const tChar c = sCode[i];
            if (wInString) {
                wResult.push_back(c);
                if (c == '"') wInString = false;
                ++i;
                continue;
            }
            if (c == '"') { wInString = true; wResult.push_back(c); ++i; continue; }
            // Match the LAMBDA keyword at a word boundary, optionally followed by spaces then '('.
            const tBool wKeyword =
                (c == 'L' || c == 'l') && (i + 6 <= n) &&
                (sCode[i+1] == 'a' || sCode[i+1] == 'A') && (sCode[i+2] == 'm' || sCode[i+2] == 'M') &&
                (sCode[i+3] == 'b' || sCode[i+3] == 'B') && (sCode[i+4] == 'd' || sCode[i+4] == 'D') &&
                (sCode[i+5] == 'a' || sCode[i+5] == 'A');
            if (wKeyword) {
                const tBool wBoundaryBefore = (i == 0) || !DesugarIsIdentChar(sCode[i-1]);
                const tBool wBoundaryAfter  = (i + 6 >= n) || !DesugarIsIdentChar(sCode[i+6]);
                if (wBoundaryBefore && wBoundaryAfter) {
                    tSize j = i + 6;
                    while (j < n && (sCode[j] == ' ' || sCode[j] == '\t')) ++j;
                    if (j < n && sCode[j] == '(') {
                        const tSize wClose = DesugarMatchParen(sCode, j);
                        if (wClose != tString::npos) {
                            // Outermost span "LAMBDA(...)"; nested inline lambdas inside are desugared when this
                            // hidden lambda's own body is compiled (recursive Compil), so skip past it here.
                            const tString wSpan = sCode.substr(i, wClose - i + 1);
                            // Closure capture: the enclosing names this span actually references (whole-word).
                            // std::set is already sorted, so the key below is stable regardless of discovery order.
                            std::vector<tString> wCaptured;
                            for (const tString& wNm : wEnclosingNames) {
                                if (SpanReferencesName(wSpan, wNm)) {
                                    wCaptured.push_back(wNm);
                                }
                            }
                            // Hidden name keyed on span + captured set (see DesugarHiddenName): identical spans in
                            // different closure environments must map to different hidden lambdas.
                            tString wKey = wSpan;
                            wKey.push_back('\x1f');
                            for (const tString& wCap : wCaptured) { wKey += wCap; wKey.push_back('\x1e'); }
                            const tString wName = DesugarHiddenName(wKey);
                            tFormulaNamed* wLambda = wContainer->FormulaNamed(wName);
                            if (wLambda == nullptr) {
                                // Create WITHOUT compiling, seed the captured names, THEN compile so the body sees
                                // them as scope (LetVarRef). This runs a nested Compil that fully completes before
                                // returning (safe: we are called before this compile sets up its own parse state).
                                wContainer->ApplyFormulaNamed(wName, wSpan, false, 0);
                                wLambda = wContainer->FormulaNamed(wName);
                                if (wLambda != nullptr) {
                                    wLambda->SetCapturedNames(wCaptured);
                                    wLambda->Compil(wSpan, true);
                                }
                            }
                            if (wLambda != nullptr && wLambda->IsLambda()) {
                                wResult += wName;
                                i = wClose + 1;
                                continue;
                            }
                            // Malformed LAMBDA (e.g. no body): leave the text verbatim so the normal compiler
                            // surfaces the error. Any stale non-lambda entry stays hidden and unreferenced.
                        }
                    }
                }
            }
            wResult.push_back(c);
            ++i;
        }
        return (wResult);
    }

    tBool tLemonInterface::Compil(tWorkBook* sWorkBook, tInterfaceCompil* sInterfaceCompil,
                                  const tChar* sCode) {
		// Desugar inline LAMBDA(...) into hidden named lambdas BEFORE any parse state is set up. Registration
		// below runs nested CompilCell calls that reuse this singleton and consume m_InjectedScopeNames (which
		// belongs to THIS compile, e.g. an enclosing lambda body's parameters): snapshot and restore it.
		const std::set<tString> wSavedInjected = m_InjectedScopeNames;
		const tString wCode = DesugarInlineLambdas(sWorkBook, tString(sCode));
		m_InjectedScopeNames = wSavedInjected;

		Clear();
		m_InterfaceCompil = sInterfaceCompil;
		m_Code=wCode;// Error with detail
#ifdef debuglex
		cout << "Compil "  << "=" << sCode << endl;
#endif
		m_WorkBook = sWorkBook;
		// Clear Vector of formula on cell
		// ====================================================================
		// LET binding names are collected lazily by SetCellDependAndFormulaKey the first time it meets the
		// "LET" token (see m_LetBoundNames doc). No pre-pass here: formulas without LET pay nothing. The key
		// pass runs before the parse pass and the set is a member, so both passes stay in sync.
		// ====================================================================
		// Search if formula exist. If necessary add references
		// ====================================================================
		tString wFormulaKey = SetCellDependAndFormulaKey(wCode.c_str());
		
        while (!m_StackFunction.empty()) { m_StackFunction.pop(); }
        if (CompilError()==tErrorFormula::t_None) {
            // Make Formula ===================================================
            // SharedFormulaPool dedups opcode storage via tSharedFormula after SetCellFormula,
            // but ParserRun must run for every host cell: SetCellDependAndFormulaKey clears
            // VectorRef and PushRef happens only during parse (R1C1 refs like R[0]C[-4]).
            // Skipping parse on a pool hit left slaves with empty VectorRef -> #REF! in FormulaStr.
            ParserInit(this);
            ParserRun(this, wCode.c_str());
            ParserDone(this);
            // Verfy important with Named Formula
            //m_InterfaceCompil=sInterfaceCompil;

#ifdef debuglex
			cout << "VectorItemFormula()-------------------------------------------" << endl;
			for (auto wItem : *m_CurrentFormula.VectorItemFormula()) {
				cout << wItem.Kind() << ":" << wItem.Value() << endl;
			}
			cout << "--------------------------------------------------------------" << endl;
#endif
            if (CompilError()!=tErrorFormula::t_None) {
                m_InterfaceCompil->ClearVectorRefAndDeleteDependant();
            } else {
                SetCellFormula();
            }
        }
#ifdef debuglex
        if (CompilError()!=tErrorFormula::t_None) {
            cerr << "Error " << sCode << "------>" << Error() << endl;
        }
#endif
        return(CompilError()==tErrorFormula::t_None);
	}

	tString tLemonInterface::SetCellDependAndFormulaKey(const tChar* sCode) {
		m_CurrentFormula.Clear();
		// Clear VectorRef and delete range if not dependent cell
		m_InterfaceCompil->ClearVectorRefAndDeleteDependant();
		// Reset LET locals: filled lazily below when the first "LET" token is seen (see m_LetBoundNames doc).
		// Cleared for every formula so names never leak from a previous LET formula.
		m_LetBoundNames.clear();
		m_LetNamesCollected = false;
		// Merge one-shot injected scope names (LAMBDA parameters) so the body compiles with them as scope
		// references (LetVarRef), then consume them so they never leak to the next formula.
		if (!m_InjectedScopeNames.empty()) {
			m_LetBoundNames.insert(m_InjectedScopeNames.begin(), m_InjectedScopeNames.end());
			m_InjectedScopeNames.clear();
		}

		// Process attribute ===================================================
		tString  wAttribute;

		tLexerToken wLexerToken;
		tStackLexerToken wStackCell;

        // Lexer ===============================================================
		tLexer wLex(sCode);
        
        // Add Separator Decimal & Arg =========================================
        tLocale* wLocale=tApplication::Instance()->Locale();
        wLex.SeparatorArg(wLocale->Arg());
        m_Decimal=wLocale->Decimal();
        m_Arg=wLocale->Arg();
        // Lang to US lang
        wLex.SeparatorDecimal(m_Decimal);
     
        tVectorString wVectorFommulaItem;
        
		wLexerToken = wLex.next();
        tString wTokenLex="";
        
        // function metho dec to Keep m_StackFunction with Cell or Range 
        tInt wDecFunction=0;
        
        // For Table Label Simple Double
        tLexerToken wLastToken;
        
        // Track array-literal context so the ';' row separator inside { ... } / | ... | is NOT rewritten to the
        // internal ',' like a locale argument separator (that flip turns a vertical literal {1;2;3} into a
        // horizontal {1,2,3}, so a reload/recompile spills sideways -> #SPILL!). Only argument ';' is converted.
        tInt wCurlyDepthKey = 0;
        tBool wInPipeLiteralKey = false;
     
     
		// Loop until End or Unexpected caracter
		while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
#ifdef debuglex
			std::cout << wLex.Column() << " Lex -->" << wLexerToken << endl;
#endif // debuglex

			wTokenLex = wLexerToken.Lexeme();

			tKind wKind = wLexerToken.Kind();
			// Update literal-array depth before the arg-separator rewrite below (see default case). '{' precedes any
			// inner ';', so by the time a row separator is seen wCurlyDepthKey is already > 0.
			if (wKind == tKind::LeftCurly) {
				wCurlyDepthKey++;
			} else if (wKind == tKind::RightCurly) {
				if (wCurlyDepthKey > 0) wCurlyDepthKey--;
			} else if (wKind == tKind::Pipe) {
				wInPipeLiteralKey = !wInPipeLiteralKey;
			}
			switch (wKind) {
            case tKind::Float: {
                if (m_Decimal!='.') {
                    // Internal US (Important)
                    std::replace(wTokenLex.begin(), wTokenLex.end(),m_Decimal,'.');
                }
                break;
            }
			case tKind::NotEqual: wTokenLex = "<>"; break;
			case tKind::GreaterThanOrEqual: wTokenLex = ">="; break;
			case tKind::LessThanOrEqual: wTokenLex = "<="; break;
			case tKind::Sheet: wTokenLex = ""; wStackCell.push(new tLexerToken(wLexerToken));
                break;
			case tKind::Cell: {
				tLexerToken* wLexerTokenTempo = new tLexerToken(wLexerToken);
				const tBool wNamedRcSuffix =
				    !wStackCell.empty() && wStackCell.top()->Kind() == tKind::Name;
				wStackCell.push(wLexerTokenTempo);
				if (wNamedRcSuffix) {
					if (!wVectorFommulaItem.empty()) {
						tString& wBack = wVectorFommulaItem.back();
						if (wBack == "name" || wBack == "nameR"
						    || (wBack.size() >= 4 && wBack.compare(0, 4, "name") == 0)) {
							wBack = "nameRC";
						}
					} else {
						wVectorFommulaItem.push_back("nameRC");
					}
					wTokenLex = "";
				} else {
					wTokenLex = wLexerToken.FormulaKey();
				}
				break;
			}
			case tKind::Attribute: {
				wTokenLex = wLexerToken.Lexeme();
				tString wTokenLexAttribute = wTokenLex;
				wTokenLexAttribute.erase(0, 1);
				wAttribute=wTokenLexAttribute;
				break;
			}
			case tKind::Identifier: {
				// Is reserved word and or not etc...
				tString wResult = wLexerToken.Lexeme();
                if (wResult == "@") {
                    tLexerToken wNextToken = wLex.next();
                    if (wNextToken.is_one_of(tKind::End, tKind::Unexpected)) {
                        wLexerToken = wNextToken;
                        break;
                    }
                    if (wNextToken.Kind() == tKind::LabelSquare) {
                        m_ThisRowColScratch = "@[" + wNextToken.Lexeme() + "]";
                        tLexerToken wSynthetic(tKind::LabelSquare,
                                               m_ThisRowColScratch.c_str(),
                                               m_ThisRowColScratch.size());
                        wTokenLex = "";
                        if (!SetTable(wLex, nullptr, &wSynthetic, wTokenLex)) {
                            tStringStream wStream;
                            wStream << "Syntax error  !";
                            Error(wStream.str(), wLex.Line(), wLex.Column());
                            CompilError(tErrorFormula::t_Function);
                            return("");
                        }
                        wVectorFommulaItem.push_back(wTokenLex);
                        wLastToken = wNextToken;
                        wLexerToken = wLex.next();
                        continue;
                    }
                    tString wNextUpper = wNextToken.Lexeme();
                    std::transform(wNextUpper.begin(), wNextUpper.end(), wNextUpper.begin(), ::toupper);
                    const tInt wNextRes = Id(wNextUpper);
                    if (wNextToken.Kind() == tKind::Identifier && wNextRes == -1
                        && !tSpreadSheetContainer::Instance()->FunctionDictionary()->Exist(wNextUpper)) {
                        m_ThisRowColScratch = "@[" + wNextToken.Lexeme() + "]";
                        tLexerToken wSynthetic(tKind::LabelSquare,
                                               m_ThisRowColScratch.c_str(),
                                               m_ThisRowColScratch.size());
                        wTokenLex = "";
                        if (!SetTable(wLex, nullptr, &wSynthetic, wTokenLex)) {
                            tStringStream wStream;
                            wStream << "Syntax error  !";
                            Error(wStream.str(), wLex.Line(), wLex.Column());
                            CompilError(tErrorFormula::t_Function);
                            return("");
                        }
                        wVectorFommulaItem.push_back(wTokenLex);
                        wLastToken = wNextToken;
                        wLexerToken = wLex.next();
                        continue;
                    }
                    wLexerToken = wNextToken;
                    wTokenLex = wLexerToken.Lexeme();
                    wResult = wTokenLex;
                }
				std::transform(wResult.begin(), wResult.end(), wResult.begin(), ::toupper);
                // Lazy LET detection: this sequential scan is guaranteed to meet "LET" before its binding
                // names and body, so collect the binding-name set right here on the first hit (once per
                // formula). One scan catches nested/sequential LETs too. Formulas without LET never reach
                // this and pay nothing. The set persists (member) into the later Lemon parse pass.
                if (!m_LetNamesCollected && wResult == "LET") {
                    CollectLetNames(sCode);
                    m_LetNamesCollected = true;
                }
                // LET local variable: keep the identifier verbatim in the formula key and skip the
                // named-range machinery (no wStackCell push, no "name" placeholder). This guarantees a
                // LET local consumes no VectorRef slot, so tFormula::Str and the RPN evaluator stay
                // aligned on the same VectorRef ordering. wTokenLex already holds the original lexeme.
                if (IsLetName(wResult)) {
                    break;
                }
                // Bare reference to a named LAMBDA used as a value (not a call): emit NO VectorRef slot here;
                // the Lemon parse pass pushes a first-class lambda value (tKind::LambdaRef). A name immediately
                // followed by '(' is a call instead (handled by wNamedLambdaCall below). Keep the name verbatim
                // in the formula key (like a LET local) so distinct lambda names yield distinct keys and the
                // VectorRef ordering stays aligned across both passes.
                if (wLex.peek_skip_space() != '(' && m_WorkBook != nullptr) {
                    tFormulaNamed* wLambdaFn = m_WorkBook->FindFormulaNamed(wResult);
                    if (wLambdaFn != nullptr && wLambdaFn->IsLambda()) {
                        break;
                    }
                }
				// Search is reserved word
				tInt wRes = Id(wResult);
				// A user-defined named LAMBDA is called like a function (NAME(...)) but is not in the function
				// dictionary. When such a name is immediately followed by '(', route it to the function branch
				// (resolved at evaluation time by tCell::CallNamedLambda) so no ref is pushed for the callee and
				// the VectorRef stays aligned with the Lemon parse pass. Built-ins already fall through via
				// Exist(); genuinely unknown names keep the legacy "X is not a function !" compile error.
				const tBool wFollowedByParen = (wLex.peek_skip_space() == '(');
				tBool wNamedLambdaCall = false;
				if (wFollowedByParen && m_WorkBook != nullptr) {
					tFormulaNamed* wFn = m_WorkBook->FindFormulaNamed(wResult);
					wNamedLambdaCall = (wFn != nullptr && wFn->IsLambda());
				}
				// Bare identifier that matches a named range / non-lambda defined name must win over a
				// homonymous built-in (e.g. SUM(Gamma) with named range "Gamma" vs worksheet function GAMMA).
				// A following '(' keeps the function/LAMBDA call path (GAMMA(5), MyLambda(...)).
				tBool wPreferNamedRef = false;
				if (!wFollowedByParen && m_WorkBook != nullptr) {
					const tString wOriginalForLookup = wLexerToken.Lexeme();
					if (!m_WorkBook->RangeNamedContainer()->Ranges(wOriginalForLookup).empty()) {
						wPreferNamedRef = true;
					} else {
						tFormulaNamed* wNamedFn = m_WorkBook->FindFormulaNamed(wOriginalForLookup);
						if (wNamedFn == nullptr) {
							wNamedFn = m_WorkBook->FindFormulaNamed(wResult);
						}
						if (wNamedFn != nullptr && !wNamedFn->IsLambda()) {
							wPreferNamedRef = true;
						}
					}
				}
				if (wRes != -1) {
					wTokenLex = " " + wTokenLex + " ";
				} // Else if not function --W  Name of Range or CellClass or Table
				else if (!wNamedLambdaCall
				         && (!tSpreadSheetContainer::Instance()->FunctionDictionary()->Exist(wResult)
				             || wPreferNamedRef)) {
                    tLexerToken* wLexerTokenTempo = new tLexerToken(wLexerToken);
                    wKind=tKind::Name;
                    wLexerTokenTempo->Kind(tKind::Name);
                    wStackCell.push(wLexerTokenTempo);
                    // Placeholder "name" in FormulaKey: real table/column for display comes from
                    // VectorRef + tFormula::TableStr (FindRangeDataCovered on the referenced cell), so table rename
                    // does not depend on embedding the old table id in the key string.
                    //
                    // Multi-area named range correction: the Lemon pre-pass expands a
                    // multi-area name into N opcodes and bumps the enclosing function
                    // arg count by (N-1). If two formulas reuse the same cached
                    // opcode stream through the SharedFormulaPool, they must share
                    // the *same* expansion shape. Encoding the current area count
                    // into the formula key ("name", "name2", "name3", ...) makes
                    // the key distinct for ranges that no longer have the same
                    // number of areas, which forces a re-compile when the named
                    // range selection changed since the last shared entry.
                    if (m_WorkBook != nullptr) {
                        // Named ranges are stored with their original case, so use
                        // the raw lexeme rather than the upper-cased reserved-word
                        // key for the container lookup.
                        tString wOriginalName = wLexerToken.Lexeme();
                        std::vector<tRange*> wRanges =
                            m_WorkBook->RangeNamedContainer()->Ranges(wOriginalName);
                        std::size_t wAreaCount = wRanges.size();
                        if (wAreaCount > 1) {
                            // Multi-area: legacy "name<N>" key encodes the area count.
                            wTokenLex = "name" + std::to_string(wAreaCount);
                        } else {
                            // Critical: single-token "name" formulas (just =someName) collide in the
                            // SharedFormulaPool when one resolves to a single cell (e.g. =AnnéeSélectionnée
                            // -> tKind::Cell) and another resolves to a multi-cell range (e.g. =lstAnnées
                            // -> tKind::Range). Whichever was compiled first wins the cache, the second
                            // one then reuses the wrong opcode shape and crashes with #REF!. Encode the
                            // resolution shape directly into the key so cell-vs-range entries stay
                            // separate (see Budget.xlsx regression — Apr 2026).
                            tBool wResolvesToRange = false;
                            if (wAreaCount == 1) {
                                tRange* wR = wRanges[0];
                                wResolvesToRange = (wR != nullptr) && !wR->IsCell();
                            } else {
                                tFormulaNamed* wFn = m_WorkBook->FindFormulaNamed(wOriginalName);
                                if (wFn != nullptr) {
                                    tRange* wSpill = wFn->SpillRange();
                                    wResolvesToRange = (wSpill != nullptr) && !wSpill->IsCell();
                                }
                            }
                            wTokenLex = wResolvesToRange ? "nameR" : "name";
                        }
                    } else {
                        wTokenLex = "name";
                    }
                }  else {
                    // If Dynamic Range RIght ==================================
                    tLexerToken* wDynamiSumRight=nullptr;
                    if (wLastToken.Kind()==tKind::Colon) {
                        if (!wStackCell.empty()) {
                            tLexerToken* wCellToken=wStackCell.top();
                            if (wCellToken->Kind()==tKind::Cell) {
                                // Set Cell
                                wStackCell.pop();
                                wDynamiSumRight=wCellToken;
                            }
                        }
                    }
                    LexPushFunctionMethod(wResult,wDynamiSumRight, false);
                }
				break;
			}
            case tKind::LabelDouble:
			case tKind::LabelSimple:  {
                tChar wSep='\'';
                if (wKind==tKind::LabelDouble) wSep='"';
                wTokenLex = tString(1, wSep) + wTokenLex + tString(1, wSep);
                // See tKind::LabelSquare (it's possible table
                break;
            }
           
            case tKind::LabelSquare: {
                // Search Sheet Table or nothing
                tLexerToken* wTable=nullptr;
                tBool wOk=true;
                tString wTokenAfter="";
                if ((wStackCell.size()==1) || (IsTableName(&wLastToken)))  {
                    tBool wName=true;
                    if (wStackCell.size()==1) wName=(wStackCell.top()->Kind()==tKind::Name);
                    if (wName) {
                        if (wStackCell.size()==1) {
                           wTable=wStackCell.top();
                           // skip Name
                           wStackCell.pop();
                        } else {
                            wTable=&wLastToken;
                            // Skip Token Label
                            wVectorFommulaItem.pop_back();
                            if (wLastToken.Kind()==tKind::LabelSimple) {
                                wTokenAfter="'name'";
                            } else {
                                wTokenAfter="\"name\"";
                            }
                        }
                    } else wOk=false;
                }
                if (wOk) {
                    wTokenLex="";
                    if (!SetTable(wLex,wTable,&wLexerToken,wTokenLex)) {
                        wOk=false;
                    }
                    if (wTokenAfter!="") {
                        wTokenLex=wTokenAfter+wTokenLex;
                    }
                }
                if (!wOk) {
                     tStringStream wStream;
                      wStream << "Syntax error  !";
                      Error(wStream.str(),wLex.Line(),wLex.Column());
                      CompilError(tErrorFormula::t_Function);
                      return(""); // Error
                }
                break;
            }
            // Error ========================================================
            case tKind::ErrorRef:
            case tKind::ErrorName: {
                wStackCell.push(new tLexerToken(wLexerToken));
                break;
            }
            case tKind::Colon: {
                    break;
            }
            // Function ()) ================================================
            case tKind::LeftParen: {
                // Is Name not function error
                if (wStackCell.size()>0) {
                    tLexerToken* wLexerTokenTempo=wStackCell.top();
                    if (wLexerTokenTempo->Kind()==tKind::Name) {
                        tStringStream wStream;
                        wStream <<  wLexerTokenTempo->Lexeme() << " is not a function !";
                        Error(wStream.str(),wLex.Line(),wLex.Column());
                        CompilError(tErrorFormula::t_Function);
                        return("");
                    }
                }
                break;
            }
            case tKind::RightParen : {
                wDecFunction++;
                break;
            }
            
 
            //case tKind::Exclamation: wTokenLex =""; break;
			case tKind::Unexpected : {
				tStringStream wStream;
				wStream << "Bad character " << wTokenLex;
				Error(wStream.str(),wLex.Line(),wLex.Column());
                CompilError(tErrorFormula::t_SyntaxError);
				return("");
			}
			default:
                // Arg
                if (wTokenLex.length()==1) {
                    tChar wChar=wTokenLex[0];
                    // Convert the locale argument separator to the internal US comma — but never the ';' row
                    // separator inside an array literal { ... } or | ... |. There ';' must stay a row separator
                    // (vertical array); rewriting it to ',' would make {1;2;3} store as {1,2,3} (horizontal) and
                    // spill sideways on the next recompile (#SPILL!). Excel: ',' = column, ';' = row.
                    if (wChar==m_Arg && wCurlyDepthKey==0 && !wInPipeLiteralKey) {
                        // Internal US  (Important)
                        wTokenLex=',';
                    }
                }
                // other character Ok
				break;

			}
            // Table
            wLastToken=wLexerToken;
			wLexerToken = wLex.next();
			// Reference to other cell(s) or range(s)
			tBool wDeferNamedR1C1Suffix = false;
			// Excel: MyNameR[-1]C[2] — defer flush so Name+Cell merge as R1C1 offset.
			// Do not defer after arg separators: IF(ISNA(B19),MyName,B19) must flush MyName
			// on the comma, not treat the next cell as an R1C1 suffix.
			if (wLexerToken.Kind() == tKind::Cell && !wStackCell.empty()
			    && wKind != tKind::Comma && wKind != tKind::Semicolon) {
				if (wStackCell.top()->Kind() == tKind::Name
				    && (wStackCell.size() == 1 || wStackCell.size() == 2)) {
					wDeferNamedR1C1Suffix = true;
				}
			}
			if (!wDeferNamedR1C1Suffix
			    && ((((wKind != tKind::Sheet) && (wKind != tKind::Colon) && (wKind != tKind::Cell)
			          && (wKind != tKind::Name) && (wKind != tKind::ErrorRef))
			         || (wLexerToken.Kind() == tKind::End)))) {
				if (!wStackCell.empty()) {
					if (wStackCell.size() > 3) {
                        tStringStream wStream;
                        wStream << "Syntax error ref";
                        Error(wStream.str(),wLex.Line(),wLex.Column());
						CompilError(tErrorFormula::t_Ref);
						return(""); // Error
					}
					tBool wOk = false;
					// Une seule cellule
					if (wStackCell.size() == 1) {
						tLexerToken* wToken1 = wStackCell.top();  wStackCell.pop();
						if (wToken1->Kind() == tKind::Cell) {
							wOk = true;
							SetCell(wToken1,wAttribute);
						} else
                        if (wToken1->Kind() == tKind::Name) {
                            wOk=true;
                            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(),wToken1,wAttribute);
                        } else
						if (wToken1->Kind() == tKind::ErrorRef) {
							wOk = true;
							SetErrorRef(wToken1);
						} else
						if (wToken1->Kind() == tKind::ErrorName) {
							wOk = true;
							SetErrorRef(wToken1);
						}
					}
					if (wStackCell.size() == 2) {
						tLexerToken* wToken2 = wStackCell.top();  wStackCell.pop();
						tLexerToken* wToken1 = wStackCell.top();  wStackCell.pop();
                        if ((wToken1->Kind() == tKind::Cell) &&  (wToken2->Kind() == tKind::Cell)) {
                            wOk = true;
                            SetRange(wToken1, wToken2);
						} else
                        if ((wToken1->Kind() == tKind::ErrorRef) || (wToken2->Kind() == tKind::ErrorRef)
                            || (wToken1->Kind() == tKind::ErrorName) || (wToken2->Kind() == tKind::ErrorName)) {
                            // Handle #REF!:#REF! or partial #REF! ranges
                            wOk = true;
                            SetRange(wToken1, wToken2);
						} else
                        if ((wToken1->Kind() == tKind::Cell) && (wToken2->Kind() == tKind::Name)) {
                            wOk = true;
                            SetRange(wToken1, wToken2);
                        } else
                        if ((wToken1->Kind() == tKind::Name) && (wToken2->Kind() == tKind::Cell)) {
                            if (wToken2->R1C1()) {
                                wOk = SetNamedRangeR1C1Offset(m_InterfaceCompil->Sheet(), wToken1, wToken2,
                                                                wAttribute);
                            } else {
                                wOk = true;
                                SetRange(wToken1, wToken2);
                            }
                        } else
                        if ((wToken1->Kind() == tKind::Name) && (wToken2->Kind() == tKind::Name)) {
                            wOk = true;
                            SetRange(wToken1, wToken2);
                        } else
                        if ((wToken1->Kind() == tKind::Sheet) && (wToken2->Kind() == tKind::Cell)) {
                            wOk = true;
                            SetSheetCell(wToken1, wToken2, wAttribute);
                        } else
                        if ((wToken1->Kind() == tKind::Sheet) && (wToken2->Kind() == tKind::Name))  {
                            wOk=true;
                            tSheet* wSheet = SheetByToken(wToken1);
                            SetCellClassOrRangeNamed(wSheet,wToken2,wAttribute);
                        }
					}
					if (wStackCell.size() == 3) {
						tLexerToken* wToken3 = wStackCell.top();  wStackCell.pop();
						tLexerToken* wToken2 = wStackCell.top();  wStackCell.pop();
						tLexerToken* wToken1 = wStackCell.top();  wStackCell.pop();
						if (wToken1->Kind() == tKind::Sheet) {
							if ((wToken2->Kind() == tKind::Cell || wToken2->Kind() == tKind::ErrorRef) &&
							    (wToken3->Kind() == tKind::Cell || wToken3->Kind() == tKind::ErrorName)) {
								wOk=true;
                                SetSheetRange(wToken1, wToken2, wToken3);
							} else if (wToken2->Kind() == tKind::Name && wToken3->Kind() == tKind::Cell) {
								wOk = SetNamedRangeR1C1Offset(SheetByToken(wToken1), wToken2, wToken3,
								                              wAttribute);
							}
						}
           		}
					if (!wOk) {
                        // if not error set Synatx Error
                        if (m_CompilError==tErrorFormula::t_None) {
                            CompilError(tErrorFormula::t_SyntaxError);
                            tStringStream wStream;
                            wStream << "Syntax error !";
                            Error(wStream.str(),wLex.Line(),wLex.Column());
                        }
						return(""); // Error
					}
				}
                wAttribute="";
			}
            // After Cell or range
            while (wDecFunction>0) {
                wDecFunction--;
                PopFunctionMethod();
            }
            wVectorFommulaItem.push_back(wTokenLex);
		}
     
        
		// last character is bad
		if (wLexerToken.Kind()==tKind::Unexpected) {
			tStringStream wStream;
			wStream << "Bad character " << wLexerToken.Lexeme();
			Error(wStream.str(),wLex.Line(),wLex.Column());
			CompilError(tErrorFormula::t_SyntaxError);
			return("");
		}
        // Make Formula Ket
        tString wFormulaKey = "";
        for(auto wToken : wVectorFommulaItem) wFormulaKey+=wToken;
        
		m_CurrentFormula.FormulaKey(wFormulaKey);
		CompilError(tErrorFormula::t_None);
		return(wFormulaKey);
	}

	// Reference ===========================================================
	tBool tLemonInterface::AddIdRef(tString sName, tInt sId, tVirtualClass* sClass,tKind sKind) {
		return(m_LemonReserved.AddIdRef(sName, sId, sClass,sKind));
	}

	tInt  tLemonInterface::Id(tString sName) {
		return(m_LemonReserved.Id(sName));
	}

	tRange* tLemonInterface::RangeNamed(tString sName) {
		return(m_WorkBook->FindRangeNamed(sName));
	}

    tCell* tLemonInterface::CellClass(tString sName) {
        tSheet* wSheet=m_InterfaceCompil->Sheet();
        tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
        return(wColRowCellRange->CellClassContainer()->CellByName(sName));
    }

    tCell* tLemonInterface::CellClassWhithSheet(tSheet* sSheet,tString sName) {
        tColRowCellRange* wColRowCellRange=sSheet->ColRowCellRange();
        return(wColRowCellRange->CellClassContainer()->CellByName(sName));
    }


	tInt tLemonInterface::NbArg(tString sName) {
		return(tSpreadSheetContainer::Instance()->FunctionDictionary()->NbArg(sName));
	}

	tTempoPoint* tLemonInterface::GetColRow(tLexerToken* sCellToken) {
		tTempoPoint* wTempoPoint = new tTempoPoint();
		if (sCellToken->R1C1()) {
			if (sCellToken->LockRow()) {
				wTempoPoint->Row(sCellToken->RowInt());
			}
			else {
				wTempoPoint->Row(m_InterfaceCompil->RowIndex() + sCellToken->RowInt());
			}
			if (sCellToken->LockCol()) {
				wTempoPoint->Col(sCellToken->ColInt());
			}
			else {
				wTempoPoint->Col(m_InterfaceCompil->ColIndex() + sCellToken->ColInt());
			}
		}
		else {
			wTempoPoint->Row(sCellToken->RowInt());
			wTempoPoint->Col(sCellToken->ColInt());
		}
		return(wTempoPoint);
	}


    // Interface SetCellAndFormulaKey =======================================
    void  tLemonInterface::_SetCell(tCell* sCell,tString sAttribute) {
        if (sCell!=nullptr) {
            // If attributee return attribute
            if (sAttribute!="") {
                tCellAttribute* wCellAttribute = CellAttribute(sCell,sAttribute);
                if (wCellAttribute==nullptr) return;
                sCell = wCellAttribute;
            }
        }
#ifdef debuglex
        if (sCell!=nullptr) {
            cout << "   SetCell ->" << sCell << ":" << sCell->StrRef();
        } else {
            cout << "   SetCell ->nullptr";
        }
        cout << endl;
#endif
        if (m_StackFunction.empty()) {
            m_InterfaceCompil->PushRef(sCell);
        } else {
            tBool wPushDependent=true;
            tLemonFunctionMethod wLemonFunction = m_StackFunction.top();
            tFunction* wFunction = wLemonFunction.Function();
            if (wFunction!=nullptr) {
                wPushDependent=!wFunction->ByRef();
            }
            m_InterfaceCompil->PushRef(sCell,wPushDependent);
        }
    }
    
     tCellAttribute* tLemonInterface::CellAttribute(tCell* sCell,tString sAttribute) {
        // Remove leading dot t
        if (sAttribute.length()>0) {
            if (sAttribute[0]=='.') {
                sAttribute.erase(0, 1);
            }
        }
        
        if (sAttribute.length()==0) {
            return(nullptr);
        }
        
        if (sCell->PtValue()->Type() != tVariantType::t_class) {
            return(nullptr);
        }
        tVariant* wVariantClass = sCell->PtValue();;
        tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wVariantClass->Class());
        if (wCellClass == nullptr) {
            return(nullptr);
        }
        tCellAttribute* wCellAttribute = wCellClass->CellAttribute(sAttribute);
        return(wCellAttribute);
    }
     
    tCell* tLemonInterface::Cell(tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
        tSheet* wSheet = m_InterfaceCompil->Sheet();
        tTempoPoint* wTempoPoint = GetColRow(sCellToken);
        tCell* wCell=wSheet->EnsureCell(wTempoPoint->Row(), wTempoPoint->Col());
        if (sAttributeToken!=nullptr) {
            return(CellAttribute(wCell,sAttributeToken->Lexeme()));
        } else {
            return(wCell);
        }
        
    }
    
    tCell* tLemonInterface::SheetCell(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
        tSheet* wSheet = SheetByToken(sCellTokenSheet);
        if (wSheet != nullptr) {
           tTempoPoint* wTempoPoint = GetColRow(sCellToken);
           tCell* wCell=wSheet->EnsureCell(wTempoPoint->Row(), wTempoPoint->Col());
           if (sAttributeToken!=nullptr) {
             return(CellAttribute(wCell,sAttributeToken->Lexeme()));
           } else {
            return(wCell);
           }
        }
        return(nullptr);
    }   
    
    tRange* tLemonInterface::Range(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
        tSheet* wSheet = m_InterfaceCompil->Sheet();
        tTempoPoint* wTempoPointTop = GetColRow(sCellTokenTop);
        tTempoPoint* wTempoPointBottom = GetColRow(sCellTokenBottom);
        // Expand shorthand references:
        // - Full column: $B:$B  -> $B1:$B<Cst_MaxRow>
        // - Full row:    $13:$13 -> $A13:<Cst_MaxCol>13
        const tBool wTopHasRow = !sCellTokenTop->Row().empty();
        const tBool wBottomHasRow = !sCellTokenBottom->Row().empty();
        const tBool wTopHasCol = !sCellTokenTop->Col().empty();
        const tBool wBottomHasCol = !sCellTokenBottom->Col().empty();
        if (!wTopHasRow && !wBottomHasRow && wTopHasCol && wBottomHasCol) {
            wTempoPointTop->Row(1);
            wTempoPointBottom->Row(Cst_MaxRow);
        }
        if (!wTopHasCol && !wBottomHasCol && wTopHasRow && wBottomHasRow) {
            wTempoPointTop->Col(1);
            wTempoPointBottom->Col(Cst_MaxCol);
        }
        return(wSheet->EnsureRange(wTempoPointTop->Row(), wTempoPointTop->Col(), wTempoPointBottom->Row(), wTempoPointBottom->Col()));
    }

    tRange* tLemonInterface::Range(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
        tSheet* wSheet = SheetByToken(sCellTokenSheet);
        if (wSheet != nullptr) {
            tTempoPoint* wTempoPointTop = GetColRow(sCellTokenTop);
            tTempoPoint* wTempoPointBottom = GetColRow(sCellTokenBottom);
            // Expand shorthand references for sheet-qualified ranges too.
            const tBool wTopHasRow = !sCellTokenTop->Row().empty();
            const tBool wBottomHasRow = !sCellTokenBottom->Row().empty();
            const tBool wTopHasCol = !sCellTokenTop->Col().empty();
            const tBool wBottomHasCol = !sCellTokenBottom->Col().empty();
            if (!wTopHasRow && !wBottomHasRow && wTopHasCol && wBottomHasCol) {
                wTempoPointTop->Row(1);
                wTempoPointBottom->Row(Cst_MaxRow);
            }
            if (!wTopHasCol && !wBottomHasCol && wTopHasRow && wBottomHasRow) {
                wTempoPointTop->Col(1);
                wTempoPointBottom->Col(Cst_MaxCol);
            }
            return(wSheet->EnsureRange(wTempoPointTop->Row(), wTempoPointTop->Col(), wTempoPointBottom->Row(), wTempoPointBottom->Col()));
        }
        return(nullptr);
    }
    
	void tLemonInterface::SetCell(tLexerToken* sCellToken, tString sAttribute) {
		tSheet* wSheet = m_InterfaceCompil->Sheet();

        // Get Position A1 or R1C1 ============================================
		tTempoPoint* wTempoPoint=GetColRow(sCellToken);

		tCell* wCell = wSheet->EnsureCell(wTempoPoint->Row(), wTempoPoint->Col());
        _SetCell(wCell,sAttribute);
	}
    
    tSheet* tLemonInterface::SheetByToken(tLexerToken* sCellTokenSheet) {
        tString wSheetName = sCellTokenSheet->Lexeme().substr(0, sCellTokenSheet->Lexeme().size() - 1);
        // Is simple quote
        if (wSheetName.length()>2) {
            if (wSheetName[0]=='\'') {
                wSheetName.erase(0, 1);
                wSheetName.erase(wSheetName.size() - 1);
            }
        }
        return(m_WorkBook->Sheet(wSheetName));
    }


	void tLemonInterface::SetSheetCell(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken, tString sAttribute) {
		tString wSheetName = sCellTokenSheet->Lexeme().substr(0, sCellTokenSheet->Lexeme().size() - 1);;
		tCell* wCell = nullptr;
        tSheet* wSheet = SheetByToken(sCellTokenSheet);
        if (wSheet!=nullptr) {
            // Get Position A1 or R1C1 ============================================
            tTempoPoint* wTempoPoint = GetColRow(sCellToken);
            wCell = wSheet->EnsureCell(wTempoPoint->Row(), wTempoPoint->Col());
        } else {
#ifdef debuglex
            cout << "   SetShetCell wSheet nullptr ->" << endl;
#endif
        }
        _SetCell(wCell, sAttribute);
	}

    void tLemonInterface::_SetRange(tRange* sRange) {
        // Is By Ref
        if (m_StackFunction.empty()) {
            if (sRange != nullptr && sRange->IsNamed()) {
                // EXCEL RANGE NAMED 
#ifdef debug_calculation_recursive
                tString wRangeName = sRange->Name();
                tCell* wRangeCell = sRange->Cell();
                if (wRangeCell != nullptr) {
                    tSheet* wSheet = m_InterfaceCompil->Sheet();
                    if (wSheet != nullptr) {
                        tCell* wCurrentCell = wSheet->EnsureCell(m_InterfaceCompil->RowIndex(), m_InterfaceCompil->ColIndex());
                        if (wCurrentCell != nullptr && wCurrentCell->StrRef() == "C6") {
                            cout << "DEBUG _SetRange C6: Named range " << wRangeName << " points to cell " << wRangeCell->StrRef() << " in sheet " << (wRangeCell->Sheet() != nullptr ? wRangeCell->Sheet()->Name() : "null") << endl;
                        }
                    }
                }
#endif
                if (sRange->IsCell()) {
                    m_InterfaceCompil->PushRef(sRange->EnsureCell());
                } else {
                    m_InterfaceCompil->PushRef(sRange);
                }
            } else {
                 m_InterfaceCompil->PushRef(sRange); 
            }
            
        } else {
            tBool wPushDependent=true;
            // Is By Ref
            tLemonFunctionMethod wLemonFunction=m_StackFunction.top();
            tFunction* wFunction=wLemonFunction.Function();
            if (wFunction!=nullptr) {
                wPushDependent=!wFunction->ByRef();
            }
            m_InterfaceCompil->PushRef(sRange,wPushDependent);
        }
       
    #ifdef debuglex
        if (sRange!=nullptr) cout << "   SetRange ->" << sRange->StrRef(true) << "->" << sRange->AllocatorRef() << endl;
    #endif
    }
    
    SkInline tBool tLemonInterface::IsTableName(tLexerToken* sLexerToken) {
        return((sLexerToken->Kind()==tKind::LabelDouble) ||
               (sLexerToken->Kind()==tKind::LabelSimple));
    }
    

    void tLemonInterface::SetPartialRangeBound(tLexerToken* sCellToken) {
        if (sCellToken == nullptr) {
            _SetRange(nullptr);
            return;
        }
        switch (sCellToken->Kind()) {
        case tKind::Name:
            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(), sCellToken, "");
            break;
        case tKind::Cell:
            SetCell(sCellToken, "");
            break;
        case tKind::ErrorRef:
        case tKind::ErrorName:
            _SetRange(nullptr);
            break;
        default:
            _SetRange(nullptr);
            break;
        }
    }

    void tLemonInterface::SetRange(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
        // Partial #REF! bounds (Excel: SUM(#REF!:MyRange), A1:#REF!, #REF!:#REF!)
        const tBool wTopError =
            sCellTokenTop->Kind() == tKind::ErrorRef || sCellTokenTop->Kind() == tKind::ErrorName;
        const tBool wBottomError =
            sCellTokenBottom->Kind() == tKind::ErrorRef || sCellTokenBottom->Kind() == tKind::ErrorName;
        if (wTopError && wBottomError) {
            _SetRange(nullptr);
            return;
        }
        if (wTopError || wBottomError) {
            SetPartialRangeBound(wTopError ? sCellTokenBottom : sCellTokenTop);
            return;
        }
        if (sCellTokenTop->Kind() == tKind::Name && sCellTokenBottom->Kind() == tKind::Cell) {
            if (sCellTokenBottom->R1C1()) {
                SetNamedRangeR1C1Offset(m_InterfaceCompil->Sheet(), sCellTokenTop, sCellTokenBottom, "");
                return;
            }
            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(), sCellTokenTop, "");
            SetCell(sCellTokenBottom, "");
            return;
        }
        if (sCellTokenTop->Kind() == tKind::Cell && sCellTokenBottom->Kind() == tKind::Name) {
            SetCell(sCellTokenTop, "");
            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(), sCellTokenBottom, "");
            return;
        }
        if (sCellTokenTop->Kind() == tKind::Name && sCellTokenBottom->Kind() == tKind::Name) {
            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(), sCellTokenTop, "");
            SetCellClassOrRangeNamed(m_InterfaceCompil->Sheet(), sCellTokenBottom, "");
            return;
        }

        tRange* wRange=Range(sCellTokenTop, sCellTokenBottom);
        _SetRange(wRange);
    }

	void tLemonInterface::SetSheetRange(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
		const tBool wTopError =
		    sCellTokenTop->Kind() == tKind::ErrorRef || sCellTokenTop->Kind() == tKind::ErrorName;
		const tBool wBottomError =
		    sCellTokenBottom->Kind() == tKind::ErrorRef || sCellTokenBottom->Kind() == tKind::ErrorName;
		if (wTopError && wBottomError) {
			_SetRange(nullptr);
			return;
		}
		if (wTopError || wBottomError) {
		    SetPartialRangeBound(wTopError ? sCellTokenBottom : sCellTokenTop);
		    return;
		}
		
		tString wSheetName = sCellTokenSheet->Lexeme().substr(0, sCellTokenSheet->Lexeme().size() - 1);;
		tRange* wRange = nullptr;
        tSheet* wSheet = SheetByToken(sCellTokenSheet);
        if (wSheet!=nullptr) {
            // Get Position A1 or R1C1 ============================================
            tTempoPoint* wTempoPointTop = GetColRow(sCellTokenTop);
            tTempoPoint* wTempoPointBottom = GetColRow(sCellTokenBottom);
            
            if (wSheet != nullptr) wRange = wSheet->EnsureRange(wTempoPointTop->Row(), wTempoPointTop->Col(), wTempoPointBottom->Row(), wTempoPointBottom->Col());
        }
        _SetRange(wRange);
	}

	void tLemonInterface::SetErrorRef(tLexerToken* sErrorToken) {
        m_InterfaceCompil->PushRef(SkNulllptr);
	}

	void tLemonInterface::SetErrorName(tLexerToken* sErrorToken) {
        m_InterfaceCompil->PushRef(SkNulllptr);
    }

    tString tLemonInterface::TableColumnKey(tLexerToken* sTableName, tLexerToken* sColumnName) {
        tRange* wTableRange = m_WorkBook->FindRangeNamed(sTableName->PureLexeme());
        if (wTableRange != nullptr) {
            tRangeData* wRangeData = CachedRangeData(sTableName->PureLexeme());
            if (wRangeData != nullptr) {
                //tTempoRect wRect =static_cast<tTempoRect>(wTableRange->Rect());
                tStringStream wStream;
                if (sColumnName != nullptr) {
                    tString wColumnName = sColumnName->PureLexeme();
                    
                    tColumnData* wColumnData = CachedFindColumnByName(wRangeData, wTableRange->Sheet(), wTableRange,
                                                                      sTableName->PureLexeme(), wColumnName);
                    if (wColumnData != nullptr) {
                        wStream << wColumnData->Ordinal(wTableRange->LeftIndex());
                    }
                }
                
                return(wStream.str());
            }
        }
        return("");
    }
            
    tBool tLemonInterface::SetCellClassOrRangeNamed(tSheet* sSheet, tLexerToken* sCellTokenName, tString sAttribute) {
        tString wName = sCellTokenName->Lexeme();
        
        // Is CellClass =======================================================
        tCell* wCell = nullptr;
        if (sSheet != nullptr) {
            wCell = sSheet->ColRowCellRange()->CellClassContainer()->CellByName(wName);
        }
        
        if (wCell != nullptr) {
            // Get Attribute
            if (sAttribute != "") {
                if (wCell->PtValue()->Type() != tVariantType::t_class) {
                    m_InterfaceCompil->PushRef(nullptr);
                    return(true);
                }
                tVariant* wVariantClass = wCell->PtValue();
                tCellClassAttribute* wCellClass = dynamic_cast<tCellClassAttribute*>(wVariantClass->Class());
                if (wCellClass == nullptr) {
                    m_InterfaceCompil->PushRef(nullptr);
                    return(true);
                }
                tCellAttribute* wCellAttribute = wCellClass->CellAttribute(sAttribute);
                wCell = wCellAttribute;
            }
            m_InterfaceCompil->PushRef(wCell);
            return(true);
        }
        
        // Is RangeNamed ======================================================
        tRange* wRange = m_WorkBook->FindRangeNamed(wName);
        if (wRange != nullptr) {
            if (sAttribute == "") {
                // Multi-area named range: when the enclosing context is an
                // aggregation function, push one ref per area so VectorRef
                // matches the N opcodes PushID will emit. Otherwise keep the
                // first area only — that keeps VectorRef and the opcode
                // stream aligned 1:1 whether the formula comes from a fresh
                // Lemon compile or from the shared-formula pool on reload.
                std::vector<tRange*> wRanges = m_WorkBook->RangeNamedContainer()->Ranges(wName);
                if (wRanges.size() > 1 && ShouldExpandMultiAreaNamedRange()) {
                    for (tRange* wR : wRanges) {
                        if (wR != nullptr) m_InterfaceCompil->PushRef(wR);
                    }
                    return(true);
                }
                if (wRange->IsCell()) {
                    tCell* wCell=wRange->EnsureCell();
                    wCell->SetNamed();
                    m_InterfaceCompil->PushRef(wCell);
                } else {
                    m_InterfaceCompil->PushRef(wRange);
                }
                return(true);
            }
        }
        // Is FormulaNamed ======================================================
        tFormulaNamed* wFormulaNamed = m_WorkBook->FindFormulaNamed(wName);
        if (wFormulaNamed != nullptr) {
            if (sAttribute == "") {
                // Push Spill depend or cell
                tRange* wSpill = MultiCellSpillRangeForFormulaNamed(wFormulaNamed,
                                                                    m_InterfaceCompil);
                if (wSpill != nullptr) {
                    m_InterfaceCompil->PushRef(wSpill);
                } else {
                    m_InterfaceCompil->PushRef(wFormulaNamed->Cell());
                }
                return(true);
            }
        }
        
        SetErrorName(sCellTokenName);
        
        return(false);
    }

    tBool tLemonInterface::SetNamedRangeR1C1Offset(tSheet* sSheet, tLexerToken* sNameToken,
                                                   tLexerToken* sRcToken, tString sAttribute) {
        (void)sSheet;
        if (sAttribute != "" || sNameToken == nullptr || sRcToken == nullptr) {
            return(false);
        }
        tRange* wNamed = ResolveNamedRangeForR1C1Offset(m_WorkBook, sNameToken, sRcToken);
        if (wNamed == nullptr) {
            SetErrorName(sNameToken);
            return(false);
        }
        tSheet* wRefSheet = wNamed->Sheet();
        if (wRefSheet == nullptr) {
            SetErrorName(sNameToken);
            return(false);
        }
        tTempoPoint* wPt = GetColRow(sRcToken);
        const tIndex wTop = wPt->Row();
        const tIndex wLeft = wPt->Col();
        delete wPt;
        // Same shape as the named range; top-left at R1C1 offset from the formula cell (Excel R1C1).
        const tIndex wHeight = wNamed->BottomIndex() - wNamed->TopIndex();
        const tIndex wWidth = wNamed->RightIndex() - wNamed->LeftIndex();
        tRange* wRange = wRefSheet->EnsureRange(wTop, wLeft, wTop + wHeight, wLeft + wWidth);
        _SetRange(wRange);
        return(true);
    }

    tBool tLemonInterface::SetTable(tLexer& sLex, tLexerToken* sTable ,tLexerToken* sToken,tString& sTokenLex) {
        // Parse =============================================================
        tLexerData wLexerData(sTable,m_Arg);
        if (!wLexerData.Parse(sToken)) {
            Error("Syntax error !",sLex.Line(),sLex.Column());
            return(false);
        }
        
        tString wTableName;
        tRange* wTableRange=nullptr;
        // if s table is Null ptr search Table Recover (use cell's sheet, not ActiveSheet, for correct table in iterative calculation)
        if (sTable==nullptr) {
            tie(wTableName,wTableRange)=m_InterfaceCompil->Sheet()->FindRangeDataCovered(m_InterfaceCompil->RowIndex(),
                                                                                         m_InterfaceCompil->ColIndex());
        
            if (wTableName=="") {
              Error("Not in table !",sLex.Line(),sLex.Column());
              return(false);
            }
        } else {
            wTableName=sTable->Lexeme();
            wTableRange = m_WorkBook->FindRangeNamed(sTable->Lexeme());
        }
        
        if (wTableRange != nullptr) {
            if (wTableRange->IsData()) {
                // Stored range is always header + data only (totals row never in range). When HasTotals(), totals row = wTableBottom+1.
                tIndex wTableTop = wTableRange->TopIndex();
                tIndex wTableBottom = wTableRange->BottomIndex();
                tIndex wTableLeft = wTableRange->LeftIndex();
                tIndex wTableRight = wTableRange->RightIndex();
                
                tRangeData* wRangeData = CachedRangeData(wTableName);
                tSheet* wSheet=wTableRange->Sheet();
                if (wRangeData != nullptr) {
                    if (wLexerData.Col1()!="") {
                        tColumnData* wColumnData1 = CachedFindColumnByName(wRangeData, wSheet, wTableRange, wTableName, wLexerData.Col1());
                        if (wColumnData1 != nullptr) {
                            tColumnData* wColumnData2=nullptr;
                            if (wLexerData.Col2()!="") {
                                wColumnData2 = CachedFindColumnByName(wRangeData, wSheet, wTableRange, wTableName, wLexerData.Col2());
                                if (wColumnData2==nullptr) {
                                     tStringStream wStream;
                                      wStream << "Error column " << wLexerData.Col1() << " don!' exist ! on table" << wTableName << " !";
                                        Error(wStream.str(),-1,-1);
                                      return(false);
                                }
                            }
                        
                        
                            tIndex wColumnIndex1 = wColumnData1->Ordinal(wTableLeft);
                            wLexerData.NumCol1(wColumnIndex1 + 1);

                            tIndex wColumnCol = wColumnData1->SheetCol();
                            tIndex wCurrentRow = m_InterfaceCompil->RowIndex();
                            
                            
                            if (wLexerData.Col2()!="") {
                                tIndex wColumnIndex2 = wColumnData2->Ordinal(wTableLeft);
                                wLexerData.NumCol2(wColumnIndex2 + 1);
                                 tIndex wColumnCol2 = wColumnData2->SheetCol();
                                
                                 // Normalize column indices
                                if (wColumnCol > wColumnCol2) {
                                    tIndex wTemp=wColumnCol;
                                    wColumnCol=wColumnCol2;
                                    wColumnCol2=wTemp;
                                }
                                tIndex wSpanTop = wCurrentRow;
                                tIndex wSpanBottom = wCurrentRow;
                                if (!TableSpecRowSpan(wLexerData.SpecialKey(), wCurrentRow, wTableTop,
                                                      wTableBottom, wRangeData->HasHeaders(),
                                                      wSpanTop, wSpanBottom)) {
                                    tStringStream wStream;
                                    wStream << "Error column " << wLexerData.Col1()
                                            << " don!' exist ! on table" << wTableName << " !";
                                    Error(wStream.str(),-1,-1);
                                    return(false);
                                }
                                tRange* wRange = wSheet->EnsureRange(wSpanTop,
                                                                     wColumnCol,
                                                                     wSpanBottom,
                                                                     wColumnCol2);
                                sTokenLex=wLexerData.TokenFormula();
                                _SetRange(wRange);
                                return(true);
                            }
                            
    
                            
                       
                            switch (wLexerData.SpecialKey()) {
                                case tTypeData::t_ThisRow : {
                                    // #This Row: current row (stored range = data only)
                                    if (wCurrentRow > wTableTop && wCurrentRow <= wTableBottom) {
                                        tCell* wCell = wSheet->EnsureCell(wCurrentRow, wColumnCol);
                                        _SetCell(wCell, "");
                                        sTokenLex=wLexerData.TokenFormula();
                                        return(true);
                                    } else {
                                        tStringStream wStream;
                                        wStream << "Error column " << wLexerData.Col1() << " don!' exist ! on table" << wTableName << " !";
                                        Error(wStream.str(),-1,-1);
                                        return(false);
                                    }
                                    break;
                                }
                                case  tTypeData::t_Headers: {
                                    // #Headers: reference the header range (first row of table)
                                    tCell* wHeadersCell = wSheet->EnsureCell(wTableTop,
                                                                             wColumnCol);
                                     sTokenLex=wLexerData.TokenFormula();
                                    _SetCell(wHeadersCell,"");
                                    return(true);
                                }
                                case tTypeData::t_Totals: {
                                    // #Totals: totals row is always the row after stored range (stored = header+data only)
                                    tCell* wTotalsCell = wSheet->EnsureCell(wTableBottom + 1,
                                                                            wColumnCol);
                                     sTokenLex=wLexerData.TokenFormula();
                                    _SetCell(wTotalsCell,"");
                                    return(true);
                                
                                }
                                case tTypeData::t_Column: {
                                    // Table[Column]: stored range is data only
                                    if (wRangeData->HasHeaders() && wTableBottom > wTableTop) wTableTop++;
                                    tRange* wColumnRange = wSheet->EnsureRange(wTableTop,
                                                                               wColumnCol,
                                                                               wTableBottom,
                                                                               wColumnCol);
                                    sTokenLex=wLexerData.TokenFormula();
                                    _SetRange(wColumnRange);
                                    return(true);
                                }
                                default:
                                    Error("Error  table  bad syntax !",sLex.Line(),sLex.Column());
                                    return(false);
                                    break;
                            } // End of switch (sSpecialToken->Kind())
                        } else {
                            // wColumnData == nullptr
                            tStringStream wStream;
                            wStream << "Error on table " << wTableName << " column " << wLexerData
                                .Col1() << " don't exist !";
                            Error(wStream.str(),-1,-1);
                            return(false);
                        }
                     
                    } else { // Col1==-1
                        switch (wLexerData.SpecialKey()) {
                           case  tTypeData::t_Headers: {
                                    // #Headers: reference the header range (first row of table)
                                    tRange* wHeadersRange = wSheet->EnsureRange(wTableTop,
                                                                                wTableLeft,
                                                                                wTableTop,
                                                                                wTableRight);
                                    sTokenLex=wLexerData.TokenFormula();
                                    _SetRange(wHeadersRange);
                                    return(true);
                            }
                            case  tTypeData::t_Totals: {
                                    wTableBottom = wTableBottom + 1;
                                    // #Totals: reference the total range (last row of table)
                                    tRange* wTotalsRange = wSheet->EnsureRange(wTableBottom,
                                                                               wTableLeft,
                                                                               wTableBottom,
                                                                               wTableRight);
                                    sTokenLex=wLexerData.TokenFormula();
                                    _SetRange(wTotalsRange);
                                    return(true);
                            }
                            case  tTypeData::t_All: {
                                    // #All: reference full table range (header + data + total)
                                    tRange* wAllRange = wSheet->EnsureRange(wTableTop,
                                                                            wTableLeft,
                                                                            wTableBottom,
                                                                            wTableRight);
                                    sTokenLex=wLexerData.TokenFormula();
                                    _SetRange(wAllRange);
                                    return(true);
                            }
                            default:
                                Error("Error  table  bas syntax !",
                                      sLex.Line(),sLex.Column());
                                return(false);
                                break;
                        }
                    }
                } else {
                    // wRangeData == nullptr
                    tStringStream wStream;
                    wStream << "Error Table  " << wTableName << " don!' exist !";
                    Error(wStream.str(),sLex.Line(),sLex.Column());
                    return(false);
                }
            }
        }
        return(false);
    }

	// Interface formula stack ================================================
	void tLemonInterface::PushInteger(tLexerToken* sCellToken, tBool sNegate) {
        tInt wInt = atoi(sCellToken->Lexeme().c_str());
        if (sNegate) wInt = -wInt;
        tVariant wValue = wInt;
        m_CurrentFormula.Push(sCellToken->Kind(), wValue);
    #ifdef debugpush
        cout << "Lemon P Integer ->" << (sNegate ? "-" : "") << sCellToken->Lexeme() << endl;
    #endif
	}
	void tLemonInterface::PushFloat(tLexerToken* sCellToken, tBool sNegate) {
        tDouble wDouble;
        if (m_Decimal!='.') {
            tString wValueStr=sCellToken->Lexeme().c_str();
            std::replace(wValueStr.begin(), wValueStr.end(),m_Decimal,'.');
            wDouble = atof(wValueStr.c_str());
        } else {
            wDouble = atof(sCellToken->Lexeme().c_str());
        }
        if (sNegate) wDouble = -wDouble;
        tVariant wValue = wDouble;
        m_CurrentFormula.Push(sCellToken->Kind(), wValue);
#ifdef debugpush
        cout << "Lemon P Float ->" << (sNegate ? "-" : "") << sCellToken->Lexeme() << endl;
#endif
	};

    void tLemonInterface::PushBool(tLexerToken* sCellToken) {
        tString wValueStr=sCellToken->Lexeme().c_str();
        std::transform(wValueStr.begin(), wValueStr.end(), wValueStr.begin(), ::toupper);
        tVariant wVariant;
        if (wValueStr=="TRUE") wVariant.SetBool(true);
        if (wValueStr=="FALSE") wVariant.SetBool(false);
        m_CurrentFormula.Push(sCellToken->Kind(),wVariant);
#ifdef debugpush
        cout << "Lemon P Bool  ->" << sCellToken->Lexeme() << endl;
#endif
    }

	void tLemonInterface::PushLabel(tLexerToken* sCellToken) {
        tVariant wValue=sCellToken->Lexeme().c_str();
        m_CurrentFormula.Push(sCellToken->Kind(), wValue);
	}

	void tLemonInterface::PushCell(tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
        if (sAttributeToken!=nullptr) {
            tVariant wValue = sCellToken->Lexeme();
            m_CurrentFormula.Push(tKind::Attribute, wValue);
        } else {
            tVariant wValue=sCellToken->FormulaKey();
            m_CurrentFormula.Push(sCellToken->Kind(), wValue);
        }
#ifdef debugpush
        tCell* wCell=Cell(sCellToken,sAttributeToken);
        cout << "Lemon P C ->";
        if (wCell!=nullptr) {
          cout << wCell->StrRef(true);
        } else {
           cout << "nullptr";
        }
         cout  << endl;
#endif
    }

	void tLemonInterface::PushSpillRef(tLexerToken* sCellToken) {
		tVariant wValue = sCellToken->FormulaKey();
		m_CurrentFormula.Push(tKind::SpillRef, wValue);
#ifdef debugpush
		tCell* wCell = Cell(sCellToken, nullptr);
		cout << "Lemon P SpillRef ->";
		if (wCell != nullptr) {
			cout << wCell->StrRef(true);
		} else {
			cout << "nullptr";
		}
		cout << "#" << endl;
#endif
	}

	void tLemonInterface::PushRange(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
		tString wKeyTop = sCellTokenTop->FormulaKey();
		tString wKeyBottom = sCellTokenBottom->FormulaKey();
        tVariant wValue=wKeyTop+":"+wKeyBottom;
        m_CurrentFormula.Push(tKind::Range, wValue);
        
#ifdef debugpush
        tRange* wRange=Range(sCellTokenTop, sCellTokenBottom);
		//cout << "PushRange " << sCellTokenTop->Lexeme() << ":" << sCellTokenBottom->Lexeme() << ":" << sCellTokenTop->IndLex() << endl;
        cout << "Lemon P R ->";
        if (wRange!=nullptr) {
          cout << wRange->StrRef(true);
        } else {
           cout << "nullptr";
        }
         cout  << endl;      
#endif
        
	}

	void tLemonInterface::PushDynamicRangeRight(tLexerToken* sCellTokenLeft) {
		// Left bound: store ref index when available so eval does not consume wIndice (keeps ref order for DynamicRangeLeft). Right bound = from stack at eval.
        tVariant wValue(sCellTokenLeft->FormulaKey());
        m_CurrentFormula.Push(tKind::DynamicRangeRight, wValue);
#ifdef debugpush
        cout << "Lemon P DyR  ->" << sCellTokenLeft->Lexeme() << endl;
#endif
		
	}

	void tLemonInterface::PushDynamicRangeLeft(tLexerToken* sCellTokenRight) {
		// Right bound: store ref index when available so eval does not consume wIndice. Left bound = from stack at eval.
        tVariant wValue(sCellTokenRight->FormulaKey());
        m_CurrentFormula.Push(tKind::DynamicRangeLeft, wValue);
#ifdef debugpush
        cout << "Lemon P DyL  ->" << sCellTokenRight->Lexeme() << endl;
#endif
	}

	void tLemonInterface::PushDynamicRangeBoth() {
        tVariant wValue;
		// Both bounds from stack at eval (e.g. Function1():Function2()). No PushRange.
		m_CurrentFormula.Push(tKind::DynamicRangeBoth, wValue);
#ifdef debugpush
        cout << "Lemon P DyA  ->"  << endl;
#endif
	}

	void tLemonInterface::PushSheetErrorRef(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken, tLexerToken* sAttributeToken) {
		m_InterfaceCompil->PushRef(SkNulllptr);
		// Keep full sheet-qualified error text for bytecode (matches Excel / OFFSET).
		tString wCombined = sCellTokenSheet->Lexeme() + sCellToken->Lexeme();
		if (sAttributeToken != nullptr) {
			tVariant wValue = wCombined + sAttributeToken->Lexeme();
			m_CurrentFormula.Push(tKind::Attribute, wValue);
		} else {
			tVariant wValue = wCombined.c_str();
			m_CurrentFormula.Push(sCellToken->Kind(), wValue);
		}
#ifdef debugpush
		cout << "Lemon P SheetErrorRef ->" << wCombined << endl;
#endif
	}

	void tLemonInterface::PushSheetCell(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
		// Key
		tVariant wKey = "!"+sCellToken->FormulaKey();
        if (sAttributeToken!=nullptr) {
            tVariant wValue = wKey+sAttributeToken->Lexeme();
            m_CurrentFormula.Push(tKind::Attribute, wValue);
        } else {
            m_CurrentFormula.Push(sCellToken->Kind(), wKey);
#ifdef debugpush
        tCell* wCell=SheetCell(sCellTokenSheet, sCellToken, sAttributeToken);
        cout << "Lemon P SC ->";
        if (wCell!=nullptr) {
          cout << wCell->StrRef(true);
        } else {
           cout << "nullptr";
        }
         cout  << endl;
#endif
        }
	}

	void tLemonInterface::PushSheetSpillRef(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken) {
		tVariant wKey = "!" + sCellToken->FormulaKey();
		m_CurrentFormula.Push(tKind::SpillRef, wKey);
#ifdef debugpush
		tCell* wCell = SheetCell(sCellTokenSheet, sCellToken, nullptr);
		cout << "Lemon P SheetSpillRef ->";
		if (wCell != nullptr) {
			cout << wCell->StrRef(true);
		} else {
			cout << "nullptr";
		}
		cout << "#" << endl;
#endif
	}

	
	void tLemonInterface::PushSheetRange(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom) {
		// Key
		tString wKeyTop = sCellTokenTop->FormulaKey();
		tString wKeyBottom = sCellTokenBottom->FormulaKey();
        tVariant wValue="!"+wKeyTop+":"+wKeyBottom;

		m_CurrentFormula.Push(tKind::Range,wValue);
#ifdef debugpush
        tRange* wRange=Range(sCellTokenSheet,sCellTokenTop, sCellTokenBottom);
        cout << "Lemon P SR ->";
        if (wRange!=nullptr) {
          cout << wRange->StrRef(true);
        } else {
           cout << "nullptr";
        }
         cout  << endl;      
#endif

	}

	void tLemonInterface::PushID(tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
		tString wName = sCellToken->Lexeme();
#ifdef debugpush
        cout << "Lemon P ID  ->" << sCellToken->Lexeme();
#endif
        // Is a LET local variable ============================================
        // Must be checked FIRST: a LET local shadows any workbook named range / cell class and, unlike
        // them, emits NO VectorRef entry (it resolves against the runtime scope stack, not the grid).
        if (IsLetName(wName)) {
            tVariant wValue(wName);
            m_CurrentFormula.Push(tKind::LetVarRef, wValue);
            return;
        }
        // Is a named LAMBDA referenced as a value (not called): push a first-class lambda opcode. Like a LET
        // local it consumes NO VectorRef slot; it resolves against the named-formula container at eval time.
        if (m_WorkBook != nullptr) {
            tString wUpper = wName;
            std::transform(wUpper.begin(), wUpper.end(), wUpper.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            tFormulaNamed* wLambda = m_WorkBook->FindFormulaNamed(wUpper);
            if (wLambda != nullptr && wLambda->IsLambda()) {
                tVariant wValue(wUpper);
                m_CurrentFormula.Push(tKind::LambdaRef, wValue);
                return;
            }
        }
        // Is CellClass =======================================================
        tCell* wCell=CellClass(wName);
        if (wCell!=nullptr) {
             if (sAttributeToken!=nullptr) {
                tVariant wValue=sCellToken->FormulaKey()+sAttributeToken->FormulaKey();
                m_CurrentFormula.Push(tKind::Attribute, wValue);
             } else {
                tVariant wValue=sCellToken->FormulaKey();
                m_CurrentFormula.Push(tKind::Cell, wValue);
            }
#ifdef debugpush
        tCell* wCell=Cell(sCellToken, sAttributeToken);
        cout << "  Cell ";
        if (wCell!=nullptr) {
          cout << wCell->StrRef(true);
        } else {
           cout << "nullptr";
        }
#endif
        } else {
            // Is RangeNamed ======================================================
            tRange* wRange = m_WorkBook->FindRangeNamed(wName);
            if (wRange != nullptr) {
                if (sAttributeToken!=nullptr) {
                     tStringStream wStream;
                      wStream << "Error Attribue on Range  !";
                      Error(wStream.str(),-1,-1);
                      CompilError(tErrorFormula::t_Attribute);
                      return; // Error
                }
#ifdef debugpush
                cout << " Name " << sCellToken->Lexeme() << " " <<  wRange->StrRef(true);
#endif
                
                // Multi-area named range: expand into one opcode per area so
                // aggregation functions like SUM(Test) see each area as a
                // distinct argument. The grammar rule `exprList ::= expr.`
                // already calls IncArg() once, so we bump it (N-1) more times.
                // The pre-pass has already pushed the matching refs into
                // VectorRef via the same predicate, so both streams stay
                // aligned even when the formula is restored from the shared
                // pool on reload (where Lemon may be skipped).
                std::vector<tRange*> wRanges = m_WorkBook->RangeNamedContainer()->Ranges(wName);
                if (wRanges.size() > 1 && ShouldExpandMultiAreaNamedRange()) {
                    for (tRange* wR : wRanges) {
                        if (wR == nullptr) continue;
                        tVariant wValue = wR->StrRef();
                        m_CurrentFormula.Push(tKind::Range, wValue);
                    }
                    for (size_t i = 1; i < wRanges.size(); ++i) {
                        IncArg();
                    }
                    return;
                }
                if (wRange->IsCell()) {
                    tCell* wCell=wRange->EnsureCell();
                    wCell->SetNamed();
                    tVariant wValue = wCell->StrRef();
                    m_CurrentFormula.Push(tKind::Cell, wValue);
                } else {
                    tVariant wValue = wRange->StrRef();
                    m_CurrentFormula.Push(tKind::Range, wValue);
                }
            } else {
                // Is Formula named (multi-cell spill uses Range — same as Excel array names)
                tFormulaNamed* wFormulaNamed=m_WorkBook->FindFormulaNamed(wName);
                if (wFormulaNamed!=nullptr) {
                    tRange* wSpill = MultiCellSpillRangeForFormulaNamed(wFormulaNamed,
                                                                        m_InterfaceCompil);
                    if (wSpill != nullptr) {
                        tVariant wValue = wSpill->StrRef();
                        m_CurrentFormula.Push(tKind::Range, wValue);
                    } else {
                        tVariant wValue = wFormulaNamed->Cell()->StrRef();
                        m_CurrentFormula.Push(tKind::Cell,wValue);
                    }
                    return; // Ok
                } else {
#ifdef debugpush
                   cout << "--- NAME NOT FOUND " << wName << endl;
#endif
                    // Error #Name
                    tVariant wValue = sCellToken->Lexeme();
                    m_CurrentFormula.Push(tKind::ErrorName, wValue);
                }
#ifdef debugpush
                cout << "#REF";
#endif
            }
        }
#ifdef debugpush
        cout << endl;
#endif
	}

    void tLemonInterface::PushNamedRangeR1C1OffsetParse(tLexerToken* sNameToken, tLexerToken* sRcToken,
                                                        tLexerToken* sAttributeToken) {
        if (sAttributeToken != nullptr) {
            tStringStream wStream;
            wStream << "Error Attribute on Range !";
            Error(wStream.str(), -1, -1);
            CompilError(tErrorFormula::t_Attribute);
            return;
        }
        tRange* wNamed = ResolveNamedRangeForR1C1Offset(m_WorkBook, sNameToken, sRcToken);
        if (wNamed == nullptr || wNamed->Sheet() == nullptr) {
            tVariant wValue = sNameToken->Lexeme();
            m_CurrentFormula.Push(tKind::ErrorName, wValue);
            return;
        }
        tTempoPoint* wPt = GetColRow(sRcToken);
        const tIndex wTop = wPt->Row();
        const tIndex wLeft = wPt->Col();
        delete wPt;
        const tIndex wHeight = wNamed->BottomIndex() - wNamed->TopIndex();
        const tIndex wWidth = wNamed->RightIndex() - wNamed->LeftIndex();
        tRange* wRange = wNamed->Sheet()->EnsureRange(wTop, wLeft, wTop + wHeight, wLeft + wWidth);
        if (wRange->IsCell()) {
            tVariant wValue = wRange->EnsureCell()->StrRef();
            m_CurrentFormula.Push(tKind::Cell, wValue);
        } else {
            tVariant wValue = wRange->StrRef();
            m_CurrentFormula.Push(tKind::Range, wValue);
        }
    }

    void tLemonInterface::PushSheetNamedRangeR1C1OffsetParse(tLexerToken* sSheetToken, tLexerToken* sNameToken,
                                                             tLexerToken* sRcToken,
                                                             tLexerToken* sAttributeToken) {
        (void)sSheetToken;
        PushNamedRangeR1C1OffsetParse(sNameToken, sRcToken, sAttributeToken);
    }

    void tLemonInterface::PushSheetID(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken,tLexerToken* sAttributeToken) {
#ifdef debugpush
        cout << "Lemon P SID  ->" << sCellTokenSheet->Lexeme() <<  sCellToken->Lexeme();
#endif
        tString wName = sCellToken->Lexeme();
        tSheet* wSheet=SheetByToken(sCellTokenSheet);
        tCell* wCell=nullptr;
        // Is CellClass =======================================================
        if (wSheet!=nullptr) wCell=CellClassWhithSheet(wSheet, wName);
        if (wCell!=nullptr) {
            if (sAttributeToken!=nullptr) {
                tVariant wValue=sCellToken->FormulaKey()+sAttributeToken->FormulaKey();
                m_CurrentFormula.Push(tKind::Attribute, wValue);
            } else {
                tVariant wValue=sCellToken->FormulaKey();
                m_CurrentFormula.Push(tKind::Cell, wValue);
            }
#ifdef debugpush
            tCell* wCell=Cell(sCellToken, sAttributeToken);
            cout << "  Cell ";
            if (wCell!=nullptr) {
              cout << wCell->StrRef(true);
            } else {
               cout << "nullptr";
            }
#endif
        } else {
            m_InterfaceCompil->PushRef(nullptr);
#ifdef debugpush
            cout << "#REF";
#endif
            tVariant wValue = sCellToken->Lexeme();
            m_CurrentFormula.Push(sCellToken->Kind(), wValue);
        }
#ifdef debugpush
        cout << endl;
#endif
    }

	void tLemonInterface::PushErrorRef(tLexerToken* sCellToken) {
        m_InterfaceCompil->PushRef(SkNulllptr);

		tVariant wValue = sCellToken->Lexeme();
		m_CurrentFormula.Push(sCellToken->Kind(), wValue);
#ifdef debugpush
         cout << "Lemon P ErrorRef " << endl;
#endif
	}

	void tLemonInterface::PushErrorName(tLexerToken* sCellToken) {
        m_InterfaceCompil->PushRef(SkNulllptr);

		tVariant wValue = sCellToken->Lexeme();
		m_CurrentFormula.Push(sCellToken->Kind(), wValue);
#ifdef debugpush
         cout << "Lemon P ErrorName " << endl;
#endif
	}

	void tLemonInterface::PushOp(tKind sOp) {
        tVariant wValue;
        m_CurrentFormula.Push(sOp, wValue);
#ifdef debugpush
         cout << "Lemon P Op   ->" << sOp;
#endif
	}

	void tLemonInterface::PushPercentLiteral() {
		tVariant wHundred(100);
		tVariant wOp;
		m_CurrentFormula.Push(tKind::Integer, wHundred);
		m_CurrentFormula.Push(tKind::Divide, wOp);
#ifdef debugpush
		cout << "Lemon P PercentLiteral -> /100" << endl;
#endif
	}

	void tLemonInterface::PushFunctionMethod(tLemonFunctionMethod* sLemonFunction) {
		tString wName = sLemonFunction->Name();
        std::transform(wName.begin(), wName.end(), wName.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		// Short-circuit IF: rewrite IF(cond;then[;else]) into IfCond/IfElse/IfEnd opcodes so only the taken
		// branch is evaluated (Excel semantics; also what makes recursive LAMBDAs terminate). The branch
		// bytecode is already emitted in order [cond][then][else]; we splice the markers at the recorded
		// argument boundaries. Invalid arity falls through to the eager function call (tFunctionIf reports it).
		if (wName == "IF") {
			const tInt wNbArg = sLemonFunction->NbArg();
			const tVectorInt& wPos = sLemonFunction->ArgEndPos();
			if ((wNbArg == 2 || wNbArg == 3) && static_cast<tInt>(wPos.size()) >= wNbArg) {
				tVectorItemFormula* wVec = m_CurrentFormula.VectorItemFormula();
				const tInt wCondEnd = wPos[0]; // position after the condition bytecode
				const tInt wThenEnd = wPos[1]; // position after the then bytecode
				// Insert the later boundary first so the earlier index stays valid.
				wVec->insert(wVec->begin() + wThenEnd, tItemFormula(tKind::IfElse, tVariant()));
				wVec->insert(wVec->begin() + wCondEnd, tItemFormula(tKind::IfCond, tVariant()));
				if (wNbArg == 2) {
					// No else branch: Excel returns FALSE when the condition is false.
					tVariant wFalse;
					wFalse.SetBool(false);
					m_CurrentFormula.Push(tKind::Bool, wFalse);
				}
				tVariant wEmpty;
				m_CurrentFormula.Push(tKind::IfEnd, wEmpty);
				return;
			}
			// else: invalid arity -> fall through to the normal (eager) IF function call.
		}
		tVariant wVariant = tVariant(wName);
		tShort wNbArg = static_cast<tShort>(sLemonFunction->NbArg());
		wVariant.Extra(wNbArg);
#ifdef debugpush
		cout << "Lemon P Fu  " << sLemonFunction->Name() <<  "() NbArg=" << wNbArg << endl;
#endif
    	m_CurrentFormula.Push(tKind::Function, wVariant);
	}

	// Interface Lemon for function
	void tLemonInterface::PushFunctionMethod(tString sFunction, tBool sIsMethod) {
		LexPushFunctionMethod(sFunction, nullptr, sIsMethod);
	}

    void tLemonInterface::LexPushFunctionMethod(tString sValue,
                                             tLexerToken* sDynamicRight,
                                             tBool sIsMethod) {
        std::transform(sValue.begin(), sValue.end(), sValue.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		tLemonFunctionMethod wLemonFunction(sValue, sDynamicRight, sIsMethod);
		m_StackFunction.push(wLemonFunction);
    }
    
    tLemonFunctionMethod tLemonInterface::PopFunctionMethod() {
        if (!m_StackFunction.empty()) {
            tLemonFunctionMethod wResult = m_StackFunction.top();
            m_StackFunction.pop();
            if (wResult.DynamicRight()!=nullptr) {
                SetCell(wResult.DynamicRight(), "");
            }
            
            return(wResult);
        }
        return(tLemonFunctionMethod()); // Empty Method
    }
    
    void tLemonInterface::IncArg() {
            m_StackFunction.top().IncArg();
            // For IF, record the bytecode position right after each argument so funcEnd can splice in the
            // IfCond / IfElse markers at the branch boundaries (short-circuit compilation).
            if (m_StackFunction.top().IsIf()) {
                m_StackFunction.top().AddArgEndPos(
                    static_cast<tInt>(m_CurrentFormula.VectorItemFormula()->size()));
            }
	};

    // LET support =============================================================
    // Called lazily by SetCellDependAndFormulaKey on the first "LET" token of the formula (never for
    // LET-free formulas), so no fast-reject guard is needed here: when we get called there IS a LET.
    void tLemonInterface::CollectLetNames(const tChar* sCode) {
        m_LetBoundNames.clear();
        CollectLetNamesInto(sCode, m_LetBoundNames);
    }

    tBool tLemonInterface::IsLetName(const tString& sName) const {
        if (m_LetBoundNames.empty()) return(false);
        tString wUp = sName;
        std::transform(wUp.begin(), wUp.end(), wUp.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return(m_LetBoundNames.find(wUp) != m_LetBoundNames.end());
    }

    void tLemonInterface::SetInjectedScopeNames(const std::set<tString>& sNames) {
        m_InjectedScopeNames.clear();
        for (const tString& wName : sNames) {
            tString wUp = wName;
            std::transform(wUp.begin(), wUp.end(), wUp.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            if (!wUp.empty()) {
                m_InjectedScopeNames.insert(wUp);
            }
        }
    }

    void tLemonInterface::LetBeginScope() {
        tVariant wEmpty;
        m_CurrentFormula.Push(tKind::LetBeginScope, wEmpty);
    }

    void tLemonInterface::LetBind(tString sName) {
        tVariant wName(sName);
        m_CurrentFormula.Push(tKind::LetBind, wName);
    }

    void tLemonInterface::LetEndScope() {
        tVariant wEmpty;
        m_CurrentFormula.Push(tKind::LetEndScope, wEmpty);
    }

    tBool tLemonInterface::ShouldExpandMultiAreaNamedRange() {
        // Same logic as the existing ByRef guard in _SetCell: expansion is
        // only safe if the immediate enclosing function consumes arguments
        // by value (aggregation style). Outside any function, or inside a
        // byref function, we fall back to the first-area-only behaviour.
        if (m_StackFunction.empty()) return(false);
        tLemonFunctionMethod wLemonFunction = m_StackFunction.top();
        tFunction* wFunction = wLemonFunction.Function();
        if (wFunction == nullptr) return(true);
        return(!wFunction->ByRef());
    }

    void tLemonInterface::PushEmptyFunctionArg() {
        // IF(cond;;else): Excel omitted branch is blank. Must push a stack value so short-circuit
        // IF leaves exactly one result (IncArg records ArgEndPos after this push — see lemon rule).
        if (!m_StackFunction.empty() && m_StackFunction.top().IsIf()) {
            tVariant wBlank;
            m_CurrentFormula.Push(tKind::LabelSimple, wBlank);
            return;
        }
        // OFFSET / TAKE / …: omitted numeric args default to 0.
        tVariant wZero(static_cast<tInt>(0));
        m_CurrentFormula.Push(tKind::Integer, wZero);
    }

    tBool tLemonInterface::PushThisRowColumn(tLexerToken* sCol) {
        if (sCol == nullptr) {
            return false;
        }
        m_ThisRowColScratch = "@[" + sCol->Lexeme() + "]";
        tLexerToken wSynthetic(tKind::LabelSquare,
                               m_ThisRowColScratch.c_str(),
                               m_ThisRowColScratch.size());
        return PushTable(nullptr, &wSynthetic);
    }

    tBool tLemonInterface::PushTable(tLexerToken* sTable, tLexerToken* sCol) {
#ifdef debugpush
        cout << "Lemon P TA ";
        if (sTable!=nullptr) cout << sTable->Lexeme() <<  ":";
        cout << sCol->Lexeme();
#endif
        tString wTableName;
        tRange* wTableRange=nullptr;
        // if s table is Null ptr search Table Recover (use cell's sheet, not ActiveSheet, for correct table in iterative calculation)
        if (sTable==nullptr) {
            tie(wTableName,wTableRange)=m_InterfaceCompil->Sheet()->FindRangeDataCovered(m_InterfaceCompil->RowIndex(),
                                                                                         m_InterfaceCompil->ColIndex());
        
            if (wTableName=="") {
              Error("Not in table !",-1,-1);
              return(false);
            }
        } else {
            wTableName=sTable->Lexeme();
            wTableRange = m_WorkBook->FindRangeNamed(sTable->Lexeme());
            if (wTableRange==nullptr) {
                Error("Table "+wTableName+" don't exist !",-1,-1);
                return(false);
            }
        }
          
    
        tLexerData wLexerData(sTable,m_Arg);
        if (!wLexerData.Parse(sCol)) return(false);
   
        
        if (wTableRange != nullptr) {
            tSheet* wSheet=wTableRange->Sheet();
            if (wTableRange->IsData()) {
                // Stored range is always header + data only; when HasTotals(), totals row = wTableBottom+1
                tIndex wTableTop = wTableRange->TopIndex();
                tIndex wTableBottom = wTableRange->BottomIndex();
                tIndex wTableLeft = wTableRange->LeftIndex();
                tIndex wTableRight = wTableRange->RightIndex();
                
                tRangeData* wRangeData = CachedRangeData(wTableName);
                
                if (wRangeData != nullptr) {
                    if (wLexerData.Col1()!="") {
                        tColumnData* wColumnData1 = CachedFindColumnByName(wRangeData, wSheet, wTableRange, wTableName, wLexerData.Col1());
                        tColumnData* wColumnData2;
                        if (wColumnData1 != nullptr) {
                          if (wLexerData.Col2()!="") {
                                wColumnData2 = CachedFindColumnByName(wRangeData, wSheet, wTableRange, wTableName, wLexerData.Col2());
                                if (wColumnData2==nullptr) {
                                     tStringStream wStream;
                                      wStream << "Error column " << wLexerData.Col1() << " don!' exist ! on table" << wTableName << " !";
                                      Error(wStream.str(),-1,-1);
                                      return(false);
                                }
                            }
                        
                            tIndex wColumnIndex1 = wColumnData1->Ordinal(wTableLeft);
                            
                            wLexerData.NumCol1(wColumnIndex1 + 1);
                            
                            tIndex wColumnCol = wColumnData1->SheetCol();
                            tIndex wCurrentRow = m_InterfaceCompil->RowIndex();
                            
                            if (wLexerData.Col2()!="") {
                                tIndex wColumnIndex2 = wColumnData2->Ordinal(wTableLeft);
                                wLexerData.NumCol2(wColumnIndex2 + 1);
                                
                                tIndex wFirstCol = wColumnData1->SheetCol();
                                tIndex wSecondCol = wColumnData2->SheetCol();
                                // Normalize column indices
                                if (wFirstCol > wSecondCol) {
                                    tIndex wTemp=wFirstCol;
                                    wFirstCol=wSecondCol;
                                    wSecondCol=wTemp;
                                }
                                tIndex wSpanTop = wCurrentRow;
                                tIndex wSpanBottom = wCurrentRow;
                                if (!TableSpecRowSpan(wLexerData.SpecialKey(), wCurrentRow, wTableTop,
                                                      wTableBottom, wRangeData->HasHeaders(),
                                                      wSpanTop, wSpanBottom)) {
                                    return(false);
                                }
                                (void)wSheet->EnsureRange(wSpanTop,
                                                          wFirstCol,
                                                          wSpanBottom,
                                                          wSecondCol);
                                tVariant wValue;
                                m_CurrentFormula.Push(tKind::Range, wValue);
                                return(true);
                            }

                            switch (wLexerData.SpecialKey()) {
                                case tTypeData::t_ThisRow : {
                                    // #This Row: current row (stored range = data only)
                                    if (wCurrentRow > wTableTop && wCurrentRow <= wTableBottom) {
                                        (void)wSheet->EnsureCell(wCurrentRow, wColumnCol);
                                        tStringStream wStream;
                                        wStream << "r" << wColumnCol;
                                        // Set Cell Key for formulaStr()
                                        tVariant wValue = wStream.str();
                                        m_CurrentFormula.Push(tKind::Cell, wValue);
                                        return(true);
                                    }
                                    break;
                                }
                                case  tTypeData::t_Headers: {
                                  // #Headers: first cell of column (first row of table)
                                    (void)wSheet->EnsureCell(wTableTop,
                                                             wColumnCol);
                                    tStringStream wStream;
                                    wStream << "h" << wColumnCol;
                                    // Set Range Key for formulaStr()
                                    tVariant wValue = wStream.str();
                                    m_CurrentFormula.Push(tKind::Cell, wValue);
                                    return(true);
                                }
                                 case  tTypeData::t_Totals: {
                                    // #Totals: totals row is always the row after stored range
                                    (void)wSheet->EnsureCell(wTableBottom + 1,
                                                             wColumnCol);
                                    tStringStream wStream;
                                    wStream << "t" << wColumnCol;
                                    // Set Range Key for formulaStr()
                                    tVariant wValue = wStream.str();
                                    m_CurrentFormula.Push(tKind::Cell, wValue);
                                    return(true);
                                }
                                case tTypeData::t_Column: {
                                    // Table[Column]: stored range is data only
                                    tIndex wColTop = wTableTop;
                                    if (wRangeData->HasHeaders() && wTableBottom > wTableTop)
                                        wColTop = wTableTop + 1;
                                    (void)wSheet->EnsureRange(wColTop,
                                                              wColumnCol,
                                                              wTableBottom,
                                                              wColumnCol);
                                    tStringStream wStream;
                                    wStream << "c" << wColumnCol;
                                    // Set Range Key for formulaStr()
                                    tVariant wValue = wStream.str();
                                    m_CurrentFormula.Push(tKind::Range, wValue);
                                    return(true);
                                }
                             
                                default:
                                    return(false);
                            } // End of switch (sSpecialToken->Kind())
                        }
                    } else {
                        switch (wLexerData.SpecialKey()) {
                          case  tTypeData::t_Headers: {
                                 // #Headers: first row of table
                                (void)wSheet->EnsureRange(wTableTop,
                                                          wTableLeft,
                                                          wTableTop,
                                                          wTableRight);
                                tStringStream wStream;
                                wStream << "h";
                                // Set Range Key for formulaStr()
                                tVariant wValue = wStream.str();
                                m_CurrentFormula.Push(tKind::Range, wValue);
                                return(true);
                        }
                        case  tTypeData::t_Totals: {
                                 // #Totals: last row+1 of table
                                wTableBottom = wTableBottom + 1;
                                (void)wSheet->EnsureRange(wTableBottom,
                                                          wTableLeft,
                                                          wTableBottom,
                                                          wTableRight);
                                tStringStream wStream;
                                // Distinct from "h" (headers) and "t" (#All full-table range below).
                                wStream << "o";
                                // Set Range Key for formulaStr()
                                tVariant wValue = wStream.str();
                                m_CurrentFormula.Push(tKind::Range, wValue);
                                return(true);
                        }
                        case tTypeData::t_All: {
                               // #All: full table range (header + data + total)
                                (void)wSheet->EnsureRange(wTableTop,
                                                          wTableLeft,
                                                          wTableBottom,
                                                          wTableRight);
                                
                                tStringStream wStream;
                                wStream << "t";
                                // Set Range Key for formulaStr()
                                tVariant wValue = wStream.str();
                                m_CurrentFormula.Push(tKind::Range, wValue);
                                return(true);
                        }
                         default: return(false);
                    }
                       
                  }
                }
        }
        return(false);
    }
	tVariant wValue = sTable->Lexeme();
	m_CurrentFormula.Push(tKind::ErrorName, wValue);
#ifdef debugpush
    cout << endl;
#endif
	return(false);
}


	tFormula* tLemonInterface::Formula() { return(&m_CurrentFormula); }

    void tLemonInterface::CompilError(tErrorFormula sValue) {
		m_CompilError = sValue;
	}
    
	tErrorFormula tLemonInterface::CompilError() {
		return(m_CompilError);
	}

	void tLemonInterface::Error(tString sLexerError,tInt sLexLine,tInt sLexColumn) {
        if (m_Error=="") {
            m_Error = sLexerError;
            m_ErrorLine = sLexLine;
            m_ErrorColumn = sLexColumn;
        }
    }

    tString tLemonInterface::Error() {
        tStringStream wStream;
        wStream << m_Error;
        return(wStream.str());
    }

    tInt tLemonInterface::ErrorLine()  { return(m_ErrorLine); }

    tInt tLemonInterface::ErrorColumn()  { return(m_ErrorColumn); }

    tString tLemonInterface::ErrorWithDetail() {
        if ((m_ErrorLine==-1) || (m_ErrorColumn==-1)) {
            return(m_Error);
        }

        if (m_Error.empty()) {
            if (m_Code.empty())
                return(tString("Formula error"));
            // Never surface the bare formula alone — callers treat this as the user message.
            return(tString("Formula error: ") + m_Code);
        }
    
        tStringStream wStreamError;
        wStreamError << m_Error << " [";
        tString wError=m_Code;
        tInt wErrorLine = m_ErrorLine;
        if (wErrorLine <= 0) {
            wErrorLine = 1;
        }
        if (wError.find('\n') != std::string::npos) {
            wStreamError << wErrorLine << ",";
        }
        wStreamError << m_ErrorColumn << "]";
        
        // Split the code into lines
        std::vector<tString> wLines;
        std::istringstream iss(wError);
        tString wLine;
        
        // Handle single line case
        if (wError.find('\n') == std::string::npos) {
            wLines.push_back(wError);
        } else {
            while (std::getline(iss, wLine)) {
                wLines.push_back(wLine);
            }
        }

        // Check if the line number is valid
        if (wErrorLine > static_cast<tInt>(wLines.size())) {
            wStreamError << "->" << wError;
            return(wStreamError.str());
        }
        // Calculate the position in the original string
        tSize position = 0;
        for (tInt i = 0; i < wErrorLine - 1; i++) {
            position += wLines[i].length() + 1; // +1 for the newline character
        }
        if (m_ErrorColumn > 0) {
            position += static_cast<tSize>(m_ErrorColumn);
        }

        // Insert the "^" character at the error position
        if (position < wError.size()) {
            wError.insert(position, "^....");
        } else {
            wError+= "^....";
        }
        wStreamError << "->" << wError;
        return(wStreamError.str());
    }

	void tLemonInterface::SetCellFormula() {
		m_InterfaceCompil->Formula(&m_CurrentFormula);
	}

	void* tLemonInterface::LemonParser() { return(m_LemonParser); }
	void tLemonInterface::LemonParser(void* sLemonParser) { m_LemonParser = sLemonParser; }

	tString tLemonInterface::FormulaKey() {
		return(m_CurrentFormula.FormulaKey());
	}

	void tLemonInterface::FormulaKey(tString sFormulaKey) {
		m_CurrentFormula.FormulaKey(sFormulaKey);
	}

	void tLemonInterface::Debug() {

		m_InterfaceCompil->Formula()->Debug();
	}
} // end of namespace



