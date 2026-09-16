//=============================================================================
// SkSpreadSheet Named Range
//=============================================================================
#include "../include/SkRangeNamed.hpp"
#include "../include/SkCalculationPath.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkRangeData.hpp"
#include "../include/SkJsonKey.hpp"
#include "../include/SkSheet.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <vector>

#define _debugformula
#define _debugrangenamed
namespace SkSpreadSheet {

namespace {

    // After filter/sort, SUBTOTAL and structured refs must re-read DataVisible on data rows.
    void RecalculateAfterRangeDataApply(tRange* sRange, tRangeData& sRangeData) {
        if (sRange == nullptr) {
            return;
        }
        tSheet* wSheet = sRange->Sheet();
        if (wSheet == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return;
        }

        tIndex wTop = sRange->TopIndex();
        tIndex wLeft = sRange->LeftIndex();
        tIndex wBottom = sRange->BottomIndex();
        tIndex wRight = sRange->RightIndex();
        // Totals row (SUBTOTAL) sits below stored data range when the table has totals.
        if (sRangeData.HasTotals()) {
            wBottom += sRangeData.TotalsRowCount();
        }

        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();

        tTempoRect wRecalcRect(wTop, wLeft, wBottom, wRight);
        wContainerPath.AddRect(wColRowCellRange, &wRecalcRect);

        // Recalc formulas outside the rect that reference this table (structured refs).
        tItem::tContainerCell* wDepend = sRange->ContainerCellDepend();
        if (wDepend != nullptr) {
            for (tCell* wCell : *wDepend->Container()) {
                if (wCell != nullptr) {
                    wContainerPath.Add(wCell);
                }
            }
        }

        wContainerPath.EndCalculate();
    }

    tBool FormulaStrHasBareRowOrColumn(const tString& sFormula) {
        tString wUpper = sFormula;
        for (char& wCh : wUpper) {
            wCh = static_cast<char>(std::toupper(static_cast<unsigned char>(wCh)));
        }
        auto wFindBare = [&](const char* sName) -> tBool {
            const tSize wNameLen = std::char_traits<char>::length(sName);
            tSize wPos = 0;
            while ((wPos = wUpper.find(sName, wPos)) != tString::npos) {
                if (wPos > 0) {
                    const unsigned char wPrev = static_cast<unsigned char>(wUpper[wPos - 1]);
                    if (std::isalnum(wPrev) || wPrev == '_') {
                        wPos += wNameLen;
                        continue;
                    }
                }
                tSize wI = wPos + wNameLen;
                while (wI < wUpper.size() && std::isspace(static_cast<unsigned char>(wUpper[wI]))) {
                    ++wI;
                }
                if (wI < wUpper.size() && wUpper[wI] == '(') {
                    ++wI;
                    while (wI < wUpper.size() && std::isspace(static_cast<unsigned char>(wUpper[wI]))) {
                        ++wI;
                    }
                    if (wI < wUpper.size() && wUpper[wI] == ')') {
                        return true;
                    }
                }
                wPos += wNameLen;
            }
            return false;
        };
        return wFindBare("ROW") || wFindBare("COLUMN");
    }

} // namespace

	//! Range Formula============================================================
    tFormulaNamed::tFormulaNamed(tString sName,tString sFormulaStr,tIndex sRow) : tClass(), m_Name(sName), m_SpillRange(nullptr), m_LateBindForceTried(false) {
        m_FormulaStr=sFormulaStr;
        tWorkBook* wWorkBook=tSpreadSheetContainer::Instance()->ActiveWorkBook();
        tSheet* wSheet=wWorkBook->SheetNamedFormula();
        // Create Cell
        m_Row = sRow;
        wSheet->EnsureCell(m_Row, 1);
        m_CellRef=wSheet->ColRowCellRange()->CellAllocatorRef(m_Row,1);
        // Store name label to the right of typical 7-column spills (cols 1–7) so spill column B (abs col 2) is never the label string.
        tCell* wCellName=wSheet->EnsureCell(m_Row, kFormulaNamedNameLabelCol);
        wCellName->Value(tVariant(sName));
        m_NameRef=wSheet->ColRowCellRange()->CellAllocatorRef(m_Row, kFormulaNamedNameLabelCol);
    }
    
	tFormulaNamed::~tFormulaNamed() {
        Clear();
	}
    
    void tFormulaNamed::Clear() {
	}
    
     tColRowCellRange* tFormulaNamed::ColRowCellRange() {
         tWorkBook* wWorkBook=tSpreadSheetContainer::Instance()->ActiveWorkBook();
         tSheet* wSheetNamedFormula=wWorkBook->SheetNamedFormula();
         return(wSheetNamedFormula->ColRowCellRange());
     }
        
     tCell* tFormulaNamed::Cell()  {
         return(ColRowCellRange()->Cell(m_CellRef));
     }
     
     tString tFormulaNamed::Name() {
        if (!m_Name.empty()) {
            return m_Name;
        }
        tCell* wCellName = ColRowCellRange()->Cell(m_NameRef);
        if (wCellName == nullptr) {
            return "";
        }
        tVariant wVariant = wCellName->Value();
        if (wVariant.Type() == tVariantType::t_string) {
            return wVariant.String();
        }
        return "";
    }

    void tFormulaNamed::ClearSpillBuffer() {
        m_SpillBuffer.clear();
        m_SpillRange = nullptr;
        m_LateBindForceTried = false;
    }

    void tFormulaNamed::ClearSpillBufferValuesToNull() {
        const tVariant wNull;
        for (std::vector<tVariant>& wRow : m_SpillBuffer) {
            for (tVariant& wCell : wRow) {
                wCell = wNull;
            }
        }
    }

    void tFormulaNamed::ResizeSpillBuffer(tIndex sH, tIndex sW) {
        // One named formula reuses the same buffer across sub-expressions (e.g. two { } literals then * and +).
        // Never shrink width/height: resizing 1x7 then 6x1 would truncate columns 1–6 of row 0 and break
        // later reads of the first literal's range (JoursEtSemaines row vector + column vector).
        const tIndex wOldH = static_cast<tIndex>(m_SpillBuffer.size());
        tIndex wOldW = 0;
        if (wOldH > 0) {
            wOldW = static_cast<tIndex>(m_SpillBuffer[0].size());
        }
        const tIndex wNewH = (sH > wOldH) ? sH : wOldH;
        const tIndex wNewW = (sW > wOldW) ? sW : wOldW;
        m_SpillBuffer.resize(static_cast<std::size_t>(wNewH));
        for (tIndex r = 0; r < wNewH; ++r) {
            m_SpillBuffer[static_cast<std::size_t>(r)].resize(static_cast<std::size_t>(wNewW));
        }
    }

    void tFormulaNamed::SetSpillBufferValue(tIndex sRow, tIndex sCol, const tVariant& sValue) {
        if (sRow >= static_cast<tIndex>(m_SpillBuffer.size())) {
            return;
        }
        if (sCol >= static_cast<tIndex>(m_SpillBuffer[static_cast<std::size_t>(sRow)].size())) {
            return;
        }
        m_SpillBuffer[static_cast<std::size_t>(sRow)][static_cast<std::size_t>(sCol)] = sValue;
    }

    void tFormulaNamed::SetSpillRange(tRange* sRange) {
        m_SpillRange = sRange;
    }

    tRange* tFormulaNamed::SpillRange() const {
        return m_SpillRange;
    }

    tBool tFormulaNamed::TryGetSpillBufferAt(tIndex sAbsRow, tIndex sAbsCol, tVariant& out) const {
        if (m_SpillBuffer.empty()) {
            return false;
        }
        // Anchor buffer at the definition cell (m_Row, col 1): same as matrix spill origin on CstSheetNamed.
        // Do not use m_SpillRange Top/Left — intermediate literals can leave a tRange whose origin differs from
        // the definition cell, which would mis-index columns (column B would read the name string cell instead).
        const tIndex t = m_Row;
        const tIndex l = 1;
        const tIndex rr = sAbsRow - t;
        const tIndex cc = sAbsCol - l;
        if (rr < 0 || cc < 0) {
            return false;
        }
        if (rr >= static_cast<tIndex>(m_SpillBuffer.size())) {
            return false;
        }
        if (cc >= static_cast<tIndex>(m_SpillBuffer[static_cast<std::size_t>(rr)].size())) {
            return false;
        }
        out = m_SpillBuffer[static_cast<std::size_t>(rr)][static_cast<std::size_t>(cc)];
        return true;
    }
    
    void tFormulaNamed::FormulaStr(tString sFormulaStr) { m_FormulaStr=sFormulaStr; };
    tString tFormulaNamed::FormulaStr() { return(m_FormulaStr); }
    
   tBool tFormulaNamed::IsLambda() const { return m_IsLambda; }

   const std::vector<tString>& tFormulaNamed::LambdaParams() const { return m_LambdaParams; }

   void tFormulaNamed::SetCapturedNames(const std::vector<tString>& sNames) { m_CapturedNames = sNames; }
   const std::vector<tString>& tFormulaNamed::CapturedNames() const { return m_CapturedNames; }
   tBool tFormulaNamed::HasCapturedNames() const { return !m_CapturedNames.empty(); }

   tVariant tFormulaNamed::Invoke(const std::vector<tStackElem>& sArgs, tBool& sArityOk,
                                  const std::map<tString, tStackElem>* sCapturedScope) {
       // Optional parameters (Excel LAMBDA + ISOMITTED): the caller may pass fewer arguments than there are
       // parameters; the missing trailing parameters are bound to an "omitted" sentinel (only ISOMITTED
       // reads it, any other use propagates like #VALUE!). Passing MORE arguments than parameters is an error.
       sArityOk = (sArgs.size() <= m_LambdaParams.size());
       if (!sArityOk) {
           return tVariant(tClassError(tTypeError::t_value, ""));
       }
       tCell* wBodyCell = Cell();
       if (wBodyCell == nullptr) {
           sArityOk = false;
           return tVariant(tClassError(tTypeError::t_name, m_Name));
       }
       // Recursion guard: IF evaluates BOTH branches eagerly, so a self-recursive LAMBDA (e.g.
       // FACT=LAMBDA(n;IF(n<=1;1;n*FACT(n-1)))) never reaches its base case and would overflow the native
       // stack. Cap the nesting depth and surface #RECURSIVE instead of crashing. (True recursion needs a
       // short-circuit IF, a separate change.)
       static tInt sInvokeDepth = 0;
       const tInt kMaxInvokeDepth = 100;
       if (sInvokeDepth >= kMaxInvokeDepth) {
           return tVariant(tClassError(tTypeError::t_recursive, ""));
       }
       // Bind parameters (already uppercased) to their arguments as the body's outermost local scope, then
       // evaluate the compiled body (its parameter references were compiled to LetVarRef). Closure: seed the
       // captured outer names first so the body can read them; parameters are bound afterwards and therefore
       // shadow any captured name of the same spelling (Excel scoping).
       std::map<tString, tStackElem> wScope;
       if (sCapturedScope != nullptr) {
           wScope = *sCapturedScope;
       }
       for (tSize i = 0; i < m_LambdaParams.size(); ++i) {
           if (i < sArgs.size()) {
               wScope[m_LambdaParams[i]] = sArgs[i];
           } else {
               // Trailing parameter not supplied: bind the omitted sentinel (ISOMITTED returns TRUE;
               // any other use propagates as #VALUE!, matching Excel).
               wScope[m_LambdaParams[i]] =
                   tStackElem(tVariant(tClassError(tTypeError::t_omitted, "")));
           }
       }
       sInvokeDepth++;
       tVariant wResult = wBodyCell->InternalCalculation(nullptr, &wScope);
       sInvokeDepth--;
       return wResult;
   }

   tBool tFormulaNamed::ParseLambda(const tString& sFormulaStr, std::vector<tString>& sParams, tString& sBody) {
       sParams.clear();
       sBody.clear();
       // Trim leading spaces and an optional leading '='.
       tSize wStart = 0;
       while (wStart < sFormulaStr.size() && (sFormulaStr[wStart] == ' ' || sFormulaStr[wStart] == '\t')) {
           ++wStart;
       }
       if (wStart < sFormulaStr.size() && sFormulaStr[wStart] == '=') {
           ++wStart;
       }
       while (wStart < sFormulaStr.size() && (sFormulaStr[wStart] == ' ' || sFormulaStr[wStart] == '\t')) {
           ++wStart;
       }
       // Must start with LAMBDA( (case-insensitive).
       const tString wKeyword = "LAMBDA";
       if (sFormulaStr.size() - wStart < wKeyword.size() + 1) {
           return false;
       }
       for (tSize i = 0; i < wKeyword.size(); ++i) {
           if (std::toupper(static_cast<unsigned char>(sFormulaStr[wStart + i])) != wKeyword[i]) {
               return false;
           }
       }
       tSize wPos = wStart + wKeyword.size();
       while (wPos < sFormulaStr.size() && (sFormulaStr[wPos] == ' ' || sFormulaStr[wPos] == '\t')) {
           ++wPos;
       }
       if (wPos >= sFormulaStr.size() || sFormulaStr[wPos] != '(') {
           return false;
       }
       // Find the matching close parenthesis for the opening one, tracking nesting and quotes.
       const tSize wOpen = wPos;
       tSize wClose = tString::npos;
       tInt wDepth = 0;
       tBool wInString = false;
       for (tSize i = wOpen; i < sFormulaStr.size(); ++i) {
           const tChar c = sFormulaStr[i];
           if (wInString) {
               if (c == '"') wInString = false;
               continue;
           }
           if (c == '"') { wInString = true; continue; }
           if (c == '(') { ++wDepth; }
           else if (c == ')') { --wDepth; if (wDepth == 0) { wClose = i; break; } }
       }
       if (wClose == tString::npos) {
           return false;
       }
       // Split the inside on TOP-LEVEL argument separators (',' or ';'), honouring nested () {} and quotes.
       const tString wInside = sFormulaStr.substr(wOpen + 1, wClose - wOpen - 1);
       std::vector<tString> wArgs;
       tString wCur;
       wDepth = 0;
       wInString = false;
       for (tSize i = 0; i < wInside.size(); ++i) {
           const tChar c = wInside[i];
           if (wInString) {
               wCur.push_back(c);
               if (c == '"') wInString = false;
               continue;
           }
           if (c == '"') { wInString = true; wCur.push_back(c); continue; }
           if (c == '(' || c == '{') { ++wDepth; wCur.push_back(c); continue; }
           if (c == ')' || c == '}') { --wDepth; wCur.push_back(c); continue; }
           if (wDepth == 0 && (c == ',' || c == ';')) {
               wArgs.push_back(wCur);
               wCur.clear();
               continue;
           }
           wCur.push_back(c);
       }
       wArgs.push_back(wCur);
       // Need at least one parameter and a body: params = all but last, body = last.
       if (wArgs.size() < 2) {
           return false;
       }
       auto wTrim = [](tString s) -> tString {
           tSize a = 0, b = s.size();
           while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
           while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) --b;
           return s.substr(a, b - a);
       };
       for (tSize i = 0; i + 1 < wArgs.size(); ++i) {
           tString wParam = wTrim(wArgs[i]);
           if (wParam.empty()) {
               return false;
           }
           // Excel optional-parameter notation in the definition UI / docs: [name] -> name.
           // Brackets are stripped for the runtime parameter name (body references use the bare name);
           // optionality itself is structural (trailing args may be omitted; ISOMITTED detects them).
           if (wParam.size() >= 2 && wParam.front() == '[' && wParam.back() == ']') {
               wParam = wTrim(wParam.substr(1, wParam.size() - 2));
               if (wParam.empty()) {
                   return false;
               }
           }
           std::transform(wParam.begin(), wParam.end(), wParam.begin(),
                          [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
           sParams.push_back(wParam);
       }
       sBody = wTrim(wArgs.back());
       return !sBody.empty();
   }

   tBool tFormulaNamed::Compil(const tString sCode, tBool sCalculate) {
       tWorkBook* wWorkBook=tSpreadSheetContainer::Instance()->ActiveWorkBook();
    
        // Keep stored definition in sync with the string being compiled (updates, not only first insert).
        if (!sCode.empty()) {
            m_FormulaStr = sCode;
            m_LateBindForceTried = false;
        }
        m_CallerRelativeKnown = false;
        m_CallerRelative = false;
        m_CallerUsesBareRow = false;
        m_CallerUsesBareColumn = false;
#ifdef debugformula
        cout << "compil " << m_FormulaStr << endl;
#endif
        // LAMBDA definition: store the parameters and compile ONLY the body, with parameters injected as scope
        // names so they resolve to LetVarRef and are bound to arguments at call time (see tCell::CallFunction).
        m_IsLambda = false;
        m_LambdaParams.clear();
        {
            std::vector<tString> wParams;
            tString wBody;
            if (ParseLambda(m_FormulaStr, wParams, wBody)) {
                m_IsLambda = true;
                m_LambdaParams = wParams;
                std::set<tString> wScope(wParams.begin(), wParams.end());
                // Closure (inline lambdas): also inject the captured outer names so body references to them
                // compile to LetVarRef; their values are supplied at call time (see Invoke / LambdaRef capture).
                for (const tString& wCap : m_CapturedNames) {
                    wScope.insert(wCap);
                }
                // Use the shared interface singleton: tWorkBook::LemonInterface() is lazily bound only inside
                // CompilCell, so it may still be null here (first compile). CompilCell binds it to this same
                // singleton, so the injected scope is picked up by the compilation below.
                tLemonInterface* wInterface = tSpreadSheetContainer::Instance()->LemonInterface();
                if (wInterface != nullptr) {
                    wInterface->SetInjectedScopeNames(wScope);
                }
                if (wWorkBook->CompilCell(Cell(), wBody.c_str())) {
                    return true;
                }
                cerr << m_Name << "->Error compil lambda body:";
                cerr << wWorkBook->LemonInterface()->ErrorWithDetail() << endl;
                m_IsLambda = false;
                m_LambdaParams.clear();
                return false;
            }
        }
        if (wWorkBook->CompilCell(Cell(), m_FormulaStr.c_str())) {
#ifdef debugformula
            cout << "Ok" << endl;
#endif
            if (sCalculate) {
                Cell()->Calculation();
            }
            return(true);
        } else {
            cerr << m_Name <<  "->Error compil:";
            cerr << wWorkBook->LemonInterface()->ErrorWithDetail() << endl;
        }
        return(false);
    }

    tBool tFormulaNamed::IsCallerRelative() {
        tCell* wHost = Cell();
        if (wHost == nullptr) {
            return false;
        }
        if (wHost->Formula() == nullptr && !m_FormulaStr.empty()) {
            (void)Compil("", false);
        }
        if (m_CallerRelativeKnown) {
            return m_CallerRelative;
        }
        // Mark known first so a cycle through other names does not recurse forever.
        m_CallerRelativeKnown = true;
        m_CallerRelative = false;
        m_CallerUsesBareRow = false;
        m_CallerUsesBareColumn = false;
        tFormula* wFormula = wHost->Formula();
        if (wFormula != nullptr) {
            const tVectorItemFormula* wItems = wFormula->VectorItemFormula();
            if (wItems != nullptr) {
                for (const tItemFormula& wItem : *wItems) {
                    if (wItem.Kind() != tKind::Function || wItem.Value().Extra() != 0) {
                        continue;
                    }
                    tString wName = wItem.Value().String();
                    for (char& wCh : wName) {
                        wCh = static_cast<char>(std::toupper(static_cast<unsigned char>(wCh)));
                    }
                    if (wName == "ROW") {
                        m_CallerUsesBareRow = true;
                    } else if (wName == "COLUMN") {
                        m_CallerUsesBareColumn = true;
                    }
                }
            }
        }
        if (m_CallerUsesBareRow || m_CallerUsesBareColumn) {
            m_CallerRelative = true;
            return true;
        }
        // Fallback: parser Extra() can disagree with the dictionary arity (ROW/COLUMN are 0-or-1).
        if (FormulaStrHasBareRowOrColumn(m_FormulaStr)) {
            tString wUpper = m_FormulaStr;
            for (char& wCh : wUpper) {
                wCh = static_cast<char>(std::toupper(static_cast<unsigned char>(wCh)));
            }
            if (wUpper.find("ROW(") != tString::npos) {
                m_CallerUsesBareRow = true;
            }
            if (wUpper.find("COLUMN(") != tString::npos) {
                m_CallerUsesBareColumn = true;
            }
            m_CallerRelative = true;
            return true;
        }
        tWorkBook* wWb = (wHost->Sheet() != nullptr) ? wHost->Sheet()->WorkBook() : nullptr;
        if (wWb == nullptr) {
            wWb = tSpreadSheetContainer::Instance()->ActiveWorkBook();
        }
        const tVectorItem* wRefs = wHost->VectorRef();
        if (wRefs != nullptr && wWb != nullptr) {
            for (tItem* wRef : *wRefs) {
                if (wRef == nullptr) {
                    continue;
                }
                tCell* wRefCell = wRef->Cell();
                if (wRefCell == nullptr) {
                    continue;
                }
                tFormulaNamed* wOther = wWb->FindFormulaNamedByCell(wRefCell);
                if (wOther != nullptr && wOther != this && wOther->IsCallerRelative()) {
                    m_CallerUsesBareRow = m_CallerUsesBareRow || wOther->m_CallerUsesBareRow;
                    m_CallerUsesBareColumn = m_CallerUsesBareColumn || wOther->m_CallerUsesBareColumn;
                    m_CallerRelative = true;
                }
            }
        }
        return m_CallerRelative;
    }

    tVariant tFormulaNamed::EvaluateAtCaller(tCell* sCaller) {
        tVariant wEmpty;
        tCell* wHost = Cell();
        if (wHost == nullptr || sCaller == nullptr) {
            return wEmpty;
        }
        if (wHost->Formula() == nullptr && !m_FormulaStr.empty()) {
            (void)Compil("", false);
        }
        tFormula* wFormula = wHost->Formula();
        if (wFormula == nullptr) {
            return wEmpty;
        }
        // Populate ROW()/COLUMN() flags before forming the memo key.
        (void)IsCallerRelative();
        tWorkBook* wWb = (sCaller->Sheet() != nullptr) ? sCaller->Sheet()->WorkBook() : nullptr;
        if (wWb == nullptr && wHost->Sheet() != nullptr) {
            wWb = wHost->Sheet()->WorkBook();
        }
        struct tCallerGuard {
            tWorkBook* m_Wb;
            tCallerGuard(tWorkBook* sWb, tCell* sCallerCell) : m_Wb(sWb) {
                if (m_Wb != nullptr) {
                    m_Wb->PushNamedFormulaCaller(sCallerCell);
                }
            }
            ~tCallerGuard() {
                if (m_Wb != nullptr) {
                    m_Wb->PopNamedFormulaCaller();
                }
            }
        };
        (void)IsCallerRelative();
        const tIndex wCacheRow = sCaller->RowIndex();
        const tIndex wCacheCol = CallerRelativeUsesColumn() ? sCaller->ColIndex() : 0;
        tVariant wCached;
        if (wWb != nullptr && wWb->TryNamedCallerEval(this, wCacheRow, wCacheCol, wCached)) {
            return wCached;
        }
        tCallerGuard wGuard(wWb, sCaller);
        // Pass the host formula so InternalCalculation returns a value without writing the _$$ cell.
        tVariant wResult = wHost->InternalCalculation(wFormula);
        if (wWb != nullptr) {
            wWb->StoreNamedCallerEval(this, wCacheRow, wCacheCol, wResult);
        }
        return wResult;
    }
    
        
    void tFormulaNamed::SetJsonFormulaValue(tString sFormulaStr) {
        Cell()->Value(sFormulaStr);
        m_FormulaStr=sFormulaStr;
    }

	//! Default JSON export filter: Named ranges and their table Data.
	/// @return     tExtension with t_Named and t_Data bits set
	tExtension DefaultJsonRangeFilter() {
		tExtension wFilter;
		wFilter.Set(t_Named);
		wFilter.Set(t_Data);
		return wFilter;
	}

	//! Container of RangeNamed================================================
	tRangeNamedContainer::tRangeNamedContainer() : tClass(), m_WorkBook(nullptr){}
	tRangeNamedContainer::~tRangeNamedContainer() {
        Clear();
    }
    
    void tRangeNamedContainer::Clear() {
        // Erase m_MapData
        for(auto wPair : m_MapData) {
            delete(wPair.second);
        }
        m_MapData.clear();
        for(auto wPair : m_MapFormula) {
            delete(wPair.second);
        }
        m_MapFormula.clear();
        // Also reset name and reverse maps: without this, ReadJson on a
        // populated workbook leaves stale entries behind. With the new
        // append semantics that stale state would produce duplicate range
        // refs (including potentially freed ones).
        m_MapName.clear();
        m_MapRef.clear();
    }

	void tRangeNamedContainer::Set(tWorkBook* sWorkBook) {
		m_WorkBook = sWorkBook;
	}

	void tRangeNamedContainer::InsertRangeNamed(tString sName, tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
#ifdef debugrangenamed
        tRange* wRange=Range(sSheetAllocatorRef,sAllocatorRangeRef);
        cout << "InsertRangeNamed("<< sName << ") ->" << wRange->StrRef() << endl;
#endif
        // Defensive: refuse 0 refs, they would poison m_MapRef/m_MapName and
        // trip ColRowCellRange::Range(0) later on.
        if (sSheetAllocatorRef == 0 || sAllocatorRangeRef == 0) {
            return;
        }
        tMapName::iterator wIteratorName = m_MapName.find(sName);
        if (wIteratorName != m_MapName.end()) {
            tNamedRangeEntry& wEntry = wIteratorName->second;
            if (wEntry.m_SheetAllocatorRef != sSheetAllocatorRef) {
                // Named ranges are single-sheet: purge all previous areas so
                // the name can be re-registered on the new sheet.
                DeleteRangeNamedByName(sName);
            } else {
                // Append the new range if not already present.
                for (tAllocatorRef wExisting : wEntry.m_RangeAllocatorRefs) {
                    if (wExisting == sAllocatorRangeRef) {
                        // Still make sure the reverse map is populated.
                        tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
                        m_MapRef[wPairRef] = sName;
                        return;
                    }
                }
                wEntry.m_RangeAllocatorRefs.push_back(sAllocatorRangeRef);
                tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
                m_MapRef[wPairRef] = sName;
                return;
            }
        }
        tNamedRangeEntry wEntry;
        wEntry.m_SheetAllocatorRef = sSheetAllocatorRef;
        wEntry.m_RangeAllocatorRefs.push_back(sAllocatorRangeRef);
        m_MapName[sName] = wEntry;
        tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
        m_MapRef[wPairRef] = sName;
    }
    

	tBool tRangeNamedContainer::DeleteRangeNamedByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
#ifdef debugrangenamed
        tRange* wRange=Range(sSheetAllocatorRef,sAllocatorRangeRef);
#endif
		tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
		tMapRef::iterator wIteratorRef;
		wIteratorRef = m_MapRef.find(wPairRef);
		if (wIteratorRef != m_MapRef.end()) {
			tString wName = (*wIteratorRef).second;
			tMapName::iterator wIteratorName;
			wIteratorName = m_MapName.find(wName);
			if (wIteratorName != m_MapName.end()) {
                // Remove the cell-named flag for single-cell ranges, same as
                // the previous single-area implementation.
                tRange* wRange=Range(sSheetAllocatorRef,sAllocatorRangeRef);
                if (wRange != nullptr && wRange->IsCell()) {
                    wRange->EnsureCell()->RemoveNamed();
                }
                // Multi-area aware: only drop this specific range ref from the
                // forward map; the name stays alive as long as it still has
                // at least one area registered.
                tNamedRangeEntry& wEntry = wIteratorName->second;
                auto& wVec = wEntry.m_RangeAllocatorRefs;
                wVec.erase(std::remove(wVec.begin(), wVec.end(), sAllocatorRangeRef), wVec.end());
                if (wVec.empty()) {
                    m_MapName.erase(wIteratorName);
                }
#ifdef debugrangenamed
                cout << "DeleteRangeNamed("<< wName << ") ->" << (wRange ? wRange->StrRef() : tString("")) << endl;
#endif
            } else {
				tStringStream wStream;
				wStream << "throw: Check error DeleteRangeNamedByRef not Name in m_MapName !";
                cout << wStream.str() << endl;
				throw(new tExceptionInternalError(wStream.str()));
			}
			m_MapRef.erase(wIteratorRef);
            // Raz Data
            DeleteRangeDataByRef(sSheetAllocatorRef, sAllocatorRangeRef);
		}
		return(false);
	}

    void tRangeNamedContainer::InsertRangeData(tString sName,tRangeData sRangeData,tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
        InsertRangeNamed(sName,sSheetAllocatorRef, sAllocatorRangeRef);
        tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
        if (m_MapData[wPairRef]!=nullptr) {
            delete(m_MapData[wPairRef]);
        }
        m_MapData[wPairRef] = new tRangeData(sRangeData);
    }
    
    tRange* tRangeNamedContainer::ApplyRangeData(tString sName,tRangeData sRangeData) {
        tRange* wRange = Range(sName);
        if (wRange != nullptr) {
            // Sort first on full data, then re-apply filter criteria on new row positions
            // (filter-before-sort left DataVisible attached to permuted rows).
            sRangeData.Sort(wRange);
            sRangeData.Filter(wRange);
            std::vector<tRange*> wRanges = Ranges(sName);
            for (tRange* wR : wRanges) {
                if (wR == nullptr || !wR->IsData()) {
                    continue;
                }
                tPairNameRef wPairRef =
                    std::make_pair(wR->Sheet()->AllocatorRef(), wR->AllocatorRef());
                auto wIt = m_MapData.find(wPairRef);
                if (wIt != m_MapData.end()) {
                    delete wIt->second;
                    wIt->second = new tRangeData(sRangeData);
                }
            }
            for (tRange* wR : wRanges) {
                if (wR != nullptr && wR->IsData()) {
                    RecalculateAfterRangeDataApply(wR, sRangeData);
                }
            }
        }
        return(wRange);
    }

    void tRangeNamedContainer::ReplaceRangeDataMetadata(tString sName, tRangeData sRangeData) {
        std::vector<tRange*> wRanges = Ranges(sName);
        for (tRange* wR : wRanges) {
            if (wR == nullptr || !wR->IsData()) {
                continue;
            }
            tPairNameRef wPairRef =
                std::make_pair(wR->Sheet()->AllocatorRef(), wR->AllocatorRef());
            auto wIt = m_MapData.find(wPairRef);
            if (wIt != m_MapData.end()) {
                delete wIt->second;
                wIt->second = new tRangeData(&sRangeData);
            }
        }
    }

    tBool tRangeNamedContainer::AddNewRangeData(tString sName,tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
        InsertRangeNamed(sName,sSheetAllocatorRef, sAllocatorRangeRef);
        tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
        if (m_MapData[wPairRef]!=nullptr) {
            delete(m_MapData[wPairRef]);
        }
        //m_MapData[wPairRef] = new tRangeData(sAllocatorRangeRef);
        return(true);
    }
      
 
	tBool tRangeNamedContainer::DeleteRangeNamedByName(tString sName) {
		tMapName::iterator wIteratorName;
		wIteratorName = m_MapName.find(sName);
		if (wIteratorName != m_MapName.end()) {
            // Copy out sheet + ranges before we start mutating the container.
            tAllocatorRef wSheetRef = wIteratorName->second.m_SheetAllocatorRef;
            std::vector<tAllocatorRef> wRangeRefs = wIteratorName->second.m_RangeAllocatorRefs;
            // Erase the forward entry first so the overlap scan below can
            // ignore sName while looking for surviving owners of each area.
            m_MapName.erase(wIteratorName);
            for (tAllocatorRef wRangeRef : wRangeRefs) {
                tPairNameRef wPairRef = std::make_pair(wSheetRef, wRangeRef);
                tMapRef::iterator wIteratorRef = m_MapRef.find(wPairRef);
                if (wIteratorRef == m_MapRef.end()) {
                    tStringStream wStream;
                    wStream << "throw:  DeleteRangeNamedByName not Name in m_MapRef !";
                    cout << wStream.str() << endl;
                    throw(new tExceptionInternalError(wStream.str()));
                }
                // Overlap-aware: another name may still own this (sheet,range)
                // pair. If so, redirect the reverse map to the survivor and
                // keep the range flagged. Otherwise do the historical cleanup.
                tString wOtherName;
                for (auto& wIt : m_MapName) {
                    if (wIt.second.m_SheetAllocatorRef != wSheetRef) continue;
                    const auto& wOther = wIt.second.m_RangeAllocatorRefs;
                    if (std::find(wOther.begin(), wOther.end(), wRangeRef) != wOther.end()) {
                        wOtherName = wIt.first;
                        break;
                    }
                }
                if (!wOtherName.empty()) {
                    m_MapRef[wPairRef] = wOtherName;
                    continue;
                }
                // If the backing range is still a single cell, clear the
                // cell's "named" flag (preserves historical behavior).
                tRange* wRange = Range(wSheetRef, wRangeRef);
                if (wRange != nullptr && wRange->IsCell()) {
                    wRange->EnsureCell()->RemoveNamed();
                }
                m_MapRef.erase(wIteratorRef);
                // Drop any associated RangeData for this area.
                DeleteRangeDataByRef(wSheetRef, wRangeRef);
            }
			return(true);
		}
		return(false);
	}


    tBool tRangeNamedContainer::DeleteRangeDataByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
        tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
        tMapData::iterator wIteratorData;
        wIteratorData = m_MapData.find(wPairRef);
        if (wIteratorData != m_MapData.end()) {
            tRangeData* wRangeData=m_MapData[wPairRef];
            m_MapData.erase(wIteratorData);
            if (wRangeData!=nullptr) {
                delete(wRangeData);
            } else {
                tStringStream wStream;
                wStream << "throw: DeleteRangeDataByRef not RangeData in m_MapData !";
                cout << wStream.str() << endl;
                throw(new tExceptionInternalError(wStream.str()));
            }
            return(true);
        }
        return(false);
    }


	tRange* tRangeNamedContainer::Range(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sRangeAllocatorRef) {
		if (m_WorkBook == nullptr) {
			return(nullptr);
		}
		tSheet* wSheet = m_WorkBook->SheetByAllocator(sSheetAllocatorRef);
		if (wSheet == nullptr) {
			return(nullptr);
		}
		return(wSheet->ColRowCellRange()->Range(sRangeAllocatorRef));
	}

	tRange* tRangeNamedContainer::Range(tString sName) {
		tMapName::iterator wIteratorName;
		wIteratorName = m_MapName.find(sName);
		if (wIteratorName != m_MapName.end()) {
			const tNamedRangeEntry& wEntry = wIteratorName->second;
			if (!wEntry.m_RangeAllocatorRefs.empty()) {
				// Multi-area aware: return the first area (back-compat
				// behavior for all formula/evaluation call-sites that still
				// expect a single tRange*).
				tAllocatorRef wFirst = wEntry.m_RangeAllocatorRefs.front();
				tPairNameRef wPairRef = std::make_pair(wEntry.m_SheetAllocatorRef, wFirst);
				tMapRef::iterator wIteratorRef = m_MapRef.find(wPairRef);
				if (wIteratorRef != m_MapRef.end()) {
					return(Range(wEntry.m_SheetAllocatorRef, wFirst));
				}
			}
		}
		return(nullptr);
	}

	std::vector<tRange*> tRangeNamedContainer::Ranges(tString sName) {
		std::vector<tRange*> wResult;
		tMapName::iterator wIteratorName = m_MapName.find(sName);
		if (wIteratorName == m_MapName.end()) {
			return(wResult);
		}
		const tNamedRangeEntry& wEntry = wIteratorName->second;
		wResult.reserve(wEntry.m_RangeAllocatorRefs.size());
		for (tAllocatorRef wRangeRef : wEntry.m_RangeAllocatorRefs) {
			tRange* wRange = Range(wEntry.m_SheetAllocatorRef, wRangeRef);
			if (wRange != nullptr) {
				wResult.push_back(wRange);
			}
		}
		return(wResult);
	}

	std::vector<tString> tRangeNamedContainer::AllNames() {
		std::vector<tString> wResult;
		wResult.reserve(m_MapName.size());
		for (auto& wIt : m_MapName) {
			wResult.push_back(wIt.first);
		}
		return(wResult);
	}

	std::vector<tString> tRangeNamedContainer::AllFormulaNamedNames() {
		std::vector<tString> wResult;
		wResult.reserve(m_MapFormula.size());
		for (const auto& wIt : m_MapFormula) {
			wResult.push_back(wIt.first);
		}
		std::sort(wResult.begin(), wResult.end());
		return wResult;
	}

	tBool tRangeNamedContainer::IsRangeSharedByOtherName(tString sExcludeName,
	                                                   tAllocatorRef sSheetAllocatorRef,
	                                                   tAllocatorRef sRangeAllocatorRef) {
		for (auto& wIt : m_MapName) {
			if (wIt.first == sExcludeName) continue;
			if (wIt.second.m_SheetAllocatorRef != sSheetAllocatorRef) continue;
			const auto& wOther = wIt.second.m_RangeAllocatorRefs;
			if (std::find(wOther.begin(), wOther.end(), sRangeAllocatorRef) != wOther.end()) {
				return(true);
			}
		}
		return(false);
	}

    tRangeData* tRangeNamedContainer::RangeData(tString sName) {
        tRange* wRange=Range(sName);
        
        if (wRange!=nullptr) {
            tPairNameRef wPairRef = std::make_pair(wRange->Sheet()->AllocatorRef(),
                                                   wRange->AllocatorRef());
            tMapData::iterator wIteratorData;
            wIteratorData = m_MapData.find(wPairRef);
            if (wIteratorData != m_MapData.end()) {
                    return((*wIteratorData).second);
            }
        }
		return(nullptr);
    }

	tString tRangeNamedContainer::FormulaNamedStr(tString sName) {
		tMapFormula::iterator wIteratorFormula;
		wIteratorFormula = m_MapFormula.find(sName);
		if (wIteratorFormula != m_MapFormula.end()) {
            tFormulaNamed* wFormulaNamed = (*wIteratorFormula).second;
            // A LAMBDA's hidden cell only holds the compiled body, so Cell()->FormulaStr() would drop the
            // LAMBDA(...) wrapper. Return the stored definition string (same source used for JSON round-trip).
            if (wFormulaNamed->IsLambda()) {
                return(wFormulaNamed->FormulaStr());
            }
            return(wFormulaNamed->Cell()->FormulaStr());
		}
		return("");
	}
    
    tFormulaNamed* tRangeNamedContainer::FormulaNamedByCell(tCell* sCell) {
        if (sCell == nullptr) {
            return(nullptr);
        }
        for (auto& wPair : m_MapFormula) {
            tFormulaNamed* wFn = wPair.second;
            if (wFn == nullptr) {
                continue;
            }
            tCell* wDef = wFn->Cell();
            if (wDef != nullptr && wDef == sCell) {
                return(wFn);
            }
            // Definition and evaluating cell can be distinct tCell* for the same logical cell.
            if (wDef != nullptr) {
                tSheet* wSh = sCell->Sheet();
                if (wSh != nullptr && wDef->Sheet() == wSh &&
                    wDef->RowIndex() == sCell->RowIndex() &&
                    wDef->ColIndex() == sCell->ColIndex()) {
                    return(wFn);
                }
            }
        }
        return(nullptr);
    }

    tFormulaNamed* tRangeNamedContainer::FormulaNamedBySpillRange(tRange* sRange) {
        if (sRange == nullptr) {
            return(nullptr);
        }
        for (auto& wPair : m_MapFormula) {
            tFormulaNamed* wFn = wPair.second;
            if (wFn == nullptr) {
                continue;
            }
            tCell* wCell = wFn->Cell();
            if (wCell == nullptr) {
                continue;
            }
            tRange* wSpill = wCell->MatrixRange();
            if (wSpill == nullptr) {
                continue;
            }
            if (wSpill == sRange) {
                return wFn;
            }
            if (wSpill->Sheet() == sRange->Sheet() &&
                wSpill->TopIndex() == sRange->TopIndex() &&
                wSpill->LeftIndex() == sRange->LeftIndex() &&
                wSpill->BottomIndex() == sRange->BottomIndex() &&
                wSpill->RightIndex() == sRange->RightIndex()) {
                return wFn;
            }
        }
        return(nullptr);
    }

    tBool tRangeNamedContainer::TryFormulaNamedSpillBufferAt(tIndex sRow, tIndex sCol, tVariant& out,
                                                             tIndex sOperandRangeTop, tIndex sOperandRangeLeft,
                                                             tIndex sOperandRangeHeight, tIndex sOperandRangeWidth) const {
        // When multiple named formulas share _$$, map iteration order could return the wrong spill buffer.
        // Prefer the formula whose definition cell lies inside the operand rectangle (e.g. '_$$'!A1:G6 includes
        // the definition at column 1 even when Left is 0). Exact top-left-only match fails for A:G vs B-first spill.
        const tBool wWantFilter = (sOperandRangeTop >= 0 && sOperandRangeLeft >= 0);
        const tBool wUseRectFilter = (sOperandRangeHeight > 0 && sOperandRangeWidth > 0);
        if (wWantFilter) {
            for (const auto& wPair : m_MapFormula) {
                const tFormulaNamed* wFn = wPair.second;
                if (wFn == nullptr) {
                    continue;
                }
                tBool wDefMatches = false;
                if (wUseRectFilter) {
                    const tIndex wDr = wFn->DefinitionRow();
                    const tIndex wDc = wFn->DefinitionCol();
                    wDefMatches = (wDr >= sOperandRangeTop && wDr <= sOperandRangeTop + sOperandRangeHeight - 1 &&
                                   wDc >= sOperandRangeLeft && wDc <= sOperandRangeLeft + sOperandRangeWidth - 1);
                } else {
                    wDefMatches = (wFn->DefinitionRow() == sOperandRangeTop &&
                                   wFn->DefinitionCol() == sOperandRangeLeft);
                }
                if (!wDefMatches) {
                    continue;
                }
                if (wFn->TryGetSpillBufferAt(sRow, sCol, out)) {
                    return true;
                }
            }
        }
        for (const auto& wPair : m_MapFormula) {
            const tFormulaNamed* wFn = wPair.second;
            if (wFn != nullptr && wFn->TryGetSpillBufferAt(sRow, sCol, out)) {
                return true;
            }
        }
        return false;
    }

    tFormulaNamed* tRangeNamedContainer::FormulaNamed(tString sName) {
		tMapFormula::iterator wIteratorFormula;
		wIteratorFormula = m_MapFormula.find(sName);
		if (wIteratorFormula != m_MapFormula.end()) {
			return((*wIteratorFormula).second);
		}
		return(nullptr);
    }
   
	tString tRangeNamedContainer::RangeByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef) {
		tString wName = "";
		tPairNameRef wPairRef = std::make_pair(sSheetAllocatorRef, sAllocatorRangeRef);
		tMapRef::iterator wIteratorRef;
		wIteratorRef = m_MapRef.find(wPairRef);
		if (wIteratorRef != m_MapRef.end()) {
			wName = (*wIteratorRef).second;
		}
		return(wName);
	}
     

    void tRangeNamedContainer::GetRangeNamedsBySheet(tAllocatorRef sSheetAllocatorRef, tVectorAllocatorRef& sRangesAllocatorRef) {
        sRangesAllocatorRef.clear();
        for (auto wPair : m_MapRef) {
            if (wPair.first.first == sSheetAllocatorRef) {
                sRangesAllocatorRef.push_back(wPair.first.second);
            }
        }
    }

    tIndex tRangeNamedContainer::FindFirstFreeFormulaNamedRow() const {
        if (m_WorkBook == nullptr) {
            return (0);
        }
        tSheet* const wSheet = m_WorkBook->SheetNamedFormula();
        if (wSheet == nullptr) {
            return (0);
        }
        const tIndex wLast = wSheet->LastRow();
        for (tIndex wRow = 1; wRow <= wLast; ++wRow) {
            if (wSheet->Cell(static_cast<tInt>(wRow), 1) == nullptr &&
                wSheet->Cell(static_cast<tInt>(wRow), tFormulaNamed::kFormulaNamedNameLabelCol) == nullptr) {
                return (wRow);
            }
        }
        return (wLast + 1);
    }

    void tRangeNamedContainer::ClearFormulaNamedRow(tIndex sRow) const {
        if (m_WorkBook == nullptr || sRow == 0) {
            return;
        }
        tSheet* const wSheet = m_WorkBook->SheetNamedFormula();
        if (wSheet == nullptr) {
            return;
        }
        wSheet->DeleteCell(sRow, 1);
        wSheet->DeleteCell(sRow, tFormulaNamed::kFormulaNamedNameLabelCol);
    }

    // Recompile one dependent; sFormulaSource must include leading '=' when it is a formula.
    static tBool RecompilFormulaDependentCell(tWorkBook* sWorkBook, tCell* sCell, const tString& sFormulaSource) {
        if (sWorkBook == nullptr || sCell == nullptr || sFormulaSource.empty()) {
            return false;
        }
        if (!sWorkBook->CompilCell(sCell, sFormulaSource.c_str())) {
            return false;
        }
        sCell->Calculation();
        return true;
    }

    // Collect dependents and capture FormulaStr while the host cell is still alive.
    static void CollectFormulaNamedDependentFormulas(
        tWorkBook* sWorkBook,
        tCell* sHostCell,
        std::vector<std::pair<tCell*, tString>>& oOut) {
        std::set<tCell*> wSeen;
        auto wAdd = [&](tCell* sCell) {
            if (sCell == nullptr || !wSeen.insert(sCell).second) {
                return;
            }
            const tString wFormula = sCell->FormulaStr();
            if (!wFormula.empty()) {
                oOut.emplace_back(sCell, wFormula);
            }
        };
        if (sHostCell != nullptr) {
            for (auto wItem : *sHostCell->ContainerCellDepend()->Container()) {
                wAdd(wItem->Cell());
            }
        }
        if (sWorkBook == nullptr || sHostCell == nullptr) {
            return;
        }
        // Fallback: ContainerCellDepend can miss dependents when the graph was built only via VectorRef.
        for (tAllocatorRef wSheetRef : *sWorkBook->VectorSheet()) {
            tSheet* wSheet = sWorkBook->SheetByAllocator(wSheetRef);
            if (wSheet == nullptr) {
                continue;
            }
            const tIndex wLastRow = wSheet->LastRow();
            const tIndex wLastCol = wSheet->LastCol();
            for (tIndex wRow = 0; wRow <= wLastRow; ++wRow) {
                for (tIndex wCol = 0; wCol <= wLastCol; ++wCol) {
                    tCell* wCell = wSheet->Cell(static_cast<tInt>(wRow), static_cast<tInt>(wCol));
                    if (wCell == nullptr || wCell->Formula() == nullptr) {
                        continue;
                    }
                    for (tItem* wRefItem : *wCell->VectorRef()) {
                        if (wRefItem == sHostCell) {
                            wAdd(wCell);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Recompile cells that referenced a named formula so VectorRef/ContainerCellDepend stay aligned.
    static void RecompilFormulaDependentCells(
        tWorkBook* sWorkBook,
        const std::vector<std::pair<tCell*, tString>>& sDependents) {
        if (sWorkBook == nullptr) {
            return;
        }
        for (const auto& wDep : sDependents) {
            tCell* wCell = wDep.first;
            if (wCell == nullptr) {
                continue;
            }
            tString wSource = wDep.second;
            if (wSource.empty()) {
                continue;
            }
            if (wSource[0] != '=') {
                wSource = "=" + wSource;
            }
            if (!RecompilFormulaDependentCell(sWorkBook, wCell, wSource)) {
                (void)RecompilFormulaDependentCell(sWorkBook, wCell, "=#NAME?");
            }
        }
    }

    tBool tRangeNamedContainer::ApplyFormulaNamed(tString sName,tString sFormula,tBool sCompil,tIndex sRow) {
        tFormulaNamed* wForumlaNamed=nullptr;
        tMapFormula::iterator wIterator;
        wIterator=m_MapFormula.find(sName);
        // Don't erase for undo 
        if (wIterator!=m_MapFormula.end()) {
            wForumlaNamed=(*wIterator).second;
        } else {
            tIndex wResolvedRow = sRow;
            if (wResolvedRow == 0) {
                wResolvedRow = FindFirstFreeFormulaNamedRow();
            }
            wForumlaNamed=new tFormulaNamed(sName,sFormula,wResolvedRow);
            m_MapFormula[sName] = wForumlaNamed;
        }
        if (sCompil) {
            return(wForumlaNamed->Compil(sFormula,true));
        }
        return(true);
    }

    tBool tRangeNamedContainer::DeleteFormulaNamed(tString sName,tBool sReleaseHostRow) {
        tMapFormula::iterator wIterator;
        wIterator=m_MapFormula.find(sName);
        if (wIterator!=m_MapFormula.end()) {
            const tIndex wRow = (*wIterator).second->DefinitionRow();
            tCell* const wHostCell = (*wIterator).second->Cell();
            std::vector<std::pair<tCell*, tString>> wDependents;
            CollectFormulaNamedDependentFormulas(m_WorkBook, wHostCell, wDependents);
            delete((*wIterator).second);
            m_MapFormula.erase(wIterator);
            RecompilFormulaDependentCells(m_WorkBook, wDependents);
            if (sReleaseHostRow) {
                ClearFormulaNamedRow(wRow);
            }
            return(true);
        }
    
        return(false);
    }
    
	void tRangeNamedContainer::Json(Writer<StringBuffer>* sWriter, tExtension sFilter) {
		tBool wIncludeNamed = sFilter.Value(t_Named);
		tBool wIncludeData = sFilter.Value(t_Data);
		if (!wIncludeNamed && !wIncludeData) {
			return;
		}
		sWriter->Key("namedranges");
		sWriter->StartArray();
		// m_MapName is unordered_map: iteration order depends on insertion/hashing. Sort by name for stable WriteJson round-trip.
		std::vector<std::pair<tString, tNamedRangeEntry>> wSortedNamed;
		wSortedNamed.reserve(m_MapName.size());
        // Sort By Name
    	for (const auto& wEntry : m_MapName)
			wSortedNamed.push_back(wEntry);
            
		std::sort(wSortedNamed.begin(), wSortedNamed.end(),
		          [](const std::pair<tString, tNamedRangeEntry>& a, const std::pair<tString, tNamedRangeEntry>& b) {
			          return a.first < b.first;
		          });
        // First Loop Named Range: merge multi-area entries so one name yields
        // exactly one JSON object with "r" as "A1:B2;D5:E6".
        if (wIncludeNamed) {
            for (const auto& wRangeNamed : wSortedNamed) {
                const tNamedRangeEntry& wEntry = wRangeNamed.second;
                tSheet* wSheet = m_WorkBook->SheetByAllocator(wEntry.m_SheetAllocatorRef);
                if (wSheet == nullptr || wEntry.m_RangeAllocatorRefs.empty()) {
                    continue;
                }
                // Data ranges are handled in the next loop (single-area only).
                tRange* wFirst = wSheet->ColRowCellRange()->Range(wEntry.m_RangeAllocatorRefs.front());
                if (wFirst != nullptr && wFirst->IsData()) {
                    continue;
                }
                // Join all areas into a single StrRef.
                tString wJoined;
                tBool wFirstArea = true;
                for (tAllocatorRef wRangeRef : wEntry.m_RangeAllocatorRefs) {
                    tRange* wRange = wSheet->ColRowCellRange()->Range(wRangeRef);
                    if (wRange == nullptr) continue;
                    if (!wFirstArea) wJoined += ";";
                    wJoined += wRange->StrRef();
                    wFirstArea = false;
                }
                sWriter->StartObject();
                sWriter->Key("n"); sWriter->String(wRangeNamed.first.c_str());
                sWriter->Key("s"); sWriter->String(wSheet->Name().c_str());
                sWriter->Key("r"); sWriter->String(wJoined.c_str());
                sWriter->EndObject();
            }
        }
            
        // Second  Loop Data: data ranges stay single-area in the JSON.
        if (wIncludeData) {
            for (const auto& wRangeNamed : wSortedNamed) {
                const tNamedRangeEntry& wEntry = wRangeNamed.second;
                tSheet* wSheet = m_WorkBook->SheetByAllocator(wEntry.m_SheetAllocatorRef);
                if (wSheet == nullptr) continue;
                for (tAllocatorRef wRangeRef : wEntry.m_RangeAllocatorRefs) {
                    tRange* wRange = wSheet->ColRowCellRange()->Range(wRangeRef);
                    if (wRange == nullptr || !wRange->IsData()) continue;
                    tPairNameRef wPairRef = std::make_pair(wEntry.m_SheetAllocatorRef, wRangeRef);
                    sWriter->StartObject();
                    sWriter->Key("n"); sWriter->String(wRangeNamed.first.c_str());
                    sWriter->Key("s"); sWriter->String(wSheet->Name().c_str());
                    sWriter->Key("r"); sWriter->String(wRange->StrRef().c_str());
                    tMapData::iterator wIteratorData = m_MapData.find(wPairRef);
                    if (wIteratorData != m_MapData.end()) {
                        tRangeData* wRangeData = wIteratorData->second;
                        if (wRangeData != nullptr && !wRangeData->IsEmpty()) {
                            sWriter->Key(kJsonKeyData);
                            wRangeData->Json(sWriter, wSheet, wRange->TopIndex(), wRange->LeftIndex());
                        }
                    }
                    sWriter->EndObject();
                }
            }
        }
		sWriter->EndArray();
	}

	void tRangeNamedContainer::Json(const Value& sValue) {
        if (sValue.HasMember("namedranges")) {
            const Value& wCellClassArray = sValue["namedranges"];
            assert(wCellClassArray.IsArray());
            for (SizeType wIndex = 0; wIndex < wCellClassArray.Size(); wIndex++) {
                const Value& wRangeNamed = wCellClassArray[wIndex];
                tString wName = wRangeNamed["n"].GetString();
                tString wFormula = "";
                if (wRangeNamed.HasMember(kJsonKeyFormula) && wRangeNamed[kJsonKeyFormula].IsString()) {
                    wFormula = wRangeNamed[kJsonKeyFormula].GetString();
                }
                if (wRangeNamed.HasMember("s")) {
                    tString wSheetStr = wRangeNamed["s"].GetString();
                    // Create Sheet before Sheet->Json()
                    tSheet* wSheet=m_WorkBook->AddSheet(wSheetStr);
                    tString wRef=wRangeNamed["r"].GetString();
                    
                    tSelect wSelect;
                    tBool wResult = wSelect.Parse(wRef);
                    if (wResult) {
                        tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
                        for (auto wItem : *wVectorSelect) {
                            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                            if (wRect != nullptr) {
                                // Check if this range has table data
                                if (wRangeNamed.HasMember(kJsonKeyData)) {
                                    tRangeData wRangeData;
                                    wRangeData.Json(wRangeNamed[kJsonKeyData],
                                                    wRect->Left(), wRect->Right());
                                    // Always use InsertRangeData if data key exists, even if empty
                                    // This ensures the range is marked as a table (IsData flag)
                                    m_WorkBook->InsertRangeData(wName, wRangeData, *wRect, wSheet);
                                } else {
                                    m_WorkBook->InsertRangeNamed(wName,*wRect,wSheet);
                                }
                            }
                        }
                    }
                }
            }
        }
	}
    
    void tRangeNamedContainer::JsonCompil() {
        for (auto& wIterator : m_MapFormula) {
            tFormulaNamed* wFormulaNamed = wIterator.second;
            if (wFormulaNamed == nullptr || wFormulaNamed->FormulaStr().empty()) {
                continue;
            }
            // Do not Add() to sContainerPath here: tContainerPath::Add allocates Path on the definition cell.
            // Sheet cells compiled next call MultiCellSpillRangeForFormulaNamed -> Calculation(), which skips Add
            // when Path()!=0, so spill/Range refs never materialize for =name until EndCalculate.
            (void)wFormulaNamed->Compil("", false);
        }
    }

    void tRangeNamedContainer::AddFormulaNamedCellsToPath(tContainerPath* sContainerPath,
                                                          tBool sDeferSheetRecalc) {
        if (sContainerPath == nullptr) {
            return;
        }
        for (auto& wPair : m_MapFormula) {
            tFormulaNamed* wFormulaNamed = wPair.second;
            if (wFormulaNamed == nullptr) {
                continue;
            }
            if (wFormulaNamed != nullptr) {
                // Compile if formula contains {
                if (wFormulaNamed->FormulaStr().find("{") != std::string::npos) {
                    // ReadJson defer-recalc: matrix named defs stay compile-only (JsonCompil);
                    // Calculation() would re-spill sheet refs and #SPILL! on pre-loaded extend cells.
                    if (!sDeferSheetRecalc) {
                        wFormulaNamed->Compil("", true);
                    }
                } else {
                    tCell* wCell = wFormulaNamed->Cell();
                    wFormulaNamed->Compil("", false);
                    sContainerPath->Add(wCell);
                }
            }
        }
    }


    void tRangeNamedContainer::JsonFormulaNamed(Writer<StringBuffer>* sWriter) {
        if (!m_MapFormula.empty()) {
            sWriter->Key("formulanamed");
            sWriter->StartArray();
            for (auto wFormulaNamed : m_MapFormula) {
                // Hidden inline-lambda helpers (_INLLMB_...) are never persisted: cells save the verbatim
                // LAMBDA(...) text (tFormula::Str expands the hidden name), and load re-desugars, recreating them.
                if (tLemonInterface::IsInlineLambdaName(wFormulaNamed.first)) {
                    continue;
                }
                sWriter->StartObject();
                sWriter->Key("n"); sWriter->String(wFormulaNamed.first.c_str());
                // Persist the definition string (m_FormulaStr), not Cell()->FormulaStr(): CompilCell may
                // rewrite the cell to a canonical form with different list/array separators, breaking JSON round-trip.
                sWriter->Key("f"); sWriter->String(wFormulaNamed.second->FormulaStr().c_str());
                sWriter->EndObject();
            }
            sWriter->EndArray();
        }
    }

    void tRangeNamedContainer::JsonFormulaNamed(const Value& sValue) {
        if (sValue.HasMember("formulanamed")) {
            const Value& wFormulaNamedArray = sValue["formulanamed"];
            assert(wFormulaNamedArray.IsArray());
        
            for (SizeType wIndex = 0; wIndex < wFormulaNamedArray.Size(); wIndex++) {
                const Value& wValueFormulaNamed = wFormulaNamedArray[wIndex];
                tString wName = wValueFormulaNamed["n"].GetString();
                tString wFormula = wValueFormulaNamed["f"].GetString();
                // Each named formula needs its own host row (same as ApplyFormulaNamed with sRow==0).
                const tIndex wRow = FindFirstFreeFormulaNamedRow();
                tFormulaNamed* wFormulaNamed=new tFormulaNamed(wName,wFormula,wRow);
                // Add in map
                m_MapFormula[wName]=wFormulaNamed;
                // Compil  after
            }
        }
    }

#ifdef _DEBUGSK
    tString tRangeNamedContainer::Debug() {
        tStringStream wStream;
        wStream << "Named Range ---------------" << endl;
        for (auto wRangeNamed : m_MapName) {
            const tString& wName = wRangeNamed.first;
            const tNamedRangeEntry& wEntry = wRangeNamed.second;
            tSheet* wSheet = m_WorkBook->SheetByAllocator(wEntry.m_SheetAllocatorRef);
            if (wSheet == nullptr) {
                wStream << wName << "---> nullptr sheet" << endl;
                continue;
            }
            // Join areas for readable debug output (e.g. "A1:B2;D5:E6").
            tString wJoined;
            tBool wFirstArea = true;
            for (tAllocatorRef wRangeRef : wEntry.m_RangeAllocatorRefs) {
                tRange* wRange = wSheet->ColRowCellRange()->Range(wRangeRef);
                if (wRange == nullptr) continue;
                if (!wFirstArea) wJoined += ";";
                wJoined += wRange->StrRef();
                wFirstArea = false;
            }
            wStream << wName << "---> " << wSheet->Name() << "!" << wJoined << endl;
            // Dump RangeData per area if present.
            for (tAllocatorRef wRangeRef : wEntry.m_RangeAllocatorRefs) {
                tRange* wRange = wSheet->ColRowCellRange()->Range(wRangeRef);
                if (wRange != nullptr && wRange->IsData()) {
                    tPairNameRef wPairRef = std::make_pair(wEntry.m_SheetAllocatorRef, wRangeRef);
                    tMapData::iterator wIteratorData = m_MapData.find(wPairRef);
                    if (wIteratorData != m_MapData.end()) {
                        tRangeData* wRangeData = wIteratorData->second;
                        if (wRangeData != nullptr) {
                            wStream << wRangeData->Debug(wRange->Sheet(), wRange);
                        }
                    }
                }
            }
        }
         wStream << "Formula Named ---------------" << endl;
        for (auto wFormulaNamed : m_MapFormula) {
           tString wName = wFormulaNamed.first;
           tFormulaNamed*  wFormulaNamedClass = wFormulaNamed.second;
            tCell* wCell= wFormulaNamedClass->Cell();
           
            wStream << wName << "/" << wCell->StrRef();
            wStream << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
           
        }
        return(wStream.str());
    }
#endif
} // End of Name Space
