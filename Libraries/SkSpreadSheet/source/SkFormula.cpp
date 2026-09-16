//=============================================================================
// SkSpreadSheet Formula
//=============================================================================
#include "../include/SkFormula.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkRangeNamed.hpp"

#define _debugformula
#define _debugformuladata

namespace SkSpreadSheet {
	// Item Formula ===========================================================
	tItemFormula::tItemFormula() : tClass(), m_Kind{}, m_Value{} {};
	tItemFormula::tItemFormula(tKind sKind, const tVariant& sVariant) : tClass(), m_Kind(sKind), m_Value(sVariant){}

	tItemFormula::tItemFormula(const tItemFormula& sItemFormula) : tClass(), m_Kind(sItemFormula.m_Kind), m_Value(sItemFormula.m_Value) {};

	tItemFormula::~tItemFormula() {
		m_Value.Clear();
	}

	tKind tItemFormula::Kind() { return(m_Kind); }
	tKind tItemFormula::Kind() const { return(m_Kind); }
	
	tVariant& tItemFormula::Value() { return(m_Value); }
	const tVariant& tItemFormula::Value() const { return(m_Value); }

	void tItemFormula::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("k");
		sWriter->Int(static_cast<int>(m_Kind));
		m_Value.Json(sWriter);	
		sWriter->EndObject();
	}

	void tItemFormula::Json(const rapidjson::Value& sValue) {
		m_Kind = static_cast<tKind>(sValue["k"].GetInt());
		m_Value.Json(sValue);
	}	
	// SkFormula ==============================================================
	tFormula::tFormula() : tClass(), m_FormulaKey(""),m_BitSetVolatile(0){};

	tFormula::tFormula(const tFormula& sFormula) : tClass(sFormula), 
		m_FormulaKey(sFormula.m_FormulaKey), 
		m_VectorItemFormula(sFormula.m_VectorItemFormula), 
        m_BitSetVolatile(sFormula.m_BitSetVolatile){};

	tFormula::~tFormula()  {
		Clear();
	}

	void tFormula::Clear() {
		m_VectorItemFormula.clear();
        m_BitSetVolatile.ClearAll();
	}

	tVectorItemFormula* tFormula::VectorItemFormula() { return(&m_VectorItemFormula); }
	const tVectorItemFormula* tFormula::VectorItemFormula() const { return(&m_VectorItemFormula); }

	void tFormula::Push(tKind sKind, tVariant& sVariant) {
        if (sKind==tKind::Function) {
            tString wName=sVariant.String();
            tFunctionRef wFunctionRef=tSpreadSheetContainer::Instance()->FunctionDictionary()->FunctionRef(wName);
            if (wFunctionRef.Volatile()!=tVolatile::t_None) {
                SetVolatile(wFunctionRef.Volatile());
            }
        }
		m_VectorItemFormula.push_back(tItemFormula(sKind, sVariant));
	}


	void tFormula::SetVolatile(tVolatile sVolatile) {
        m_BitSetVolatile.Set(tByte(sVolatile));
	}

    tBool tFormula::Volatile(tVolatile sVolatile) const {
        return(m_BitSetVolatile.Value(tChar(sVolatile)));
    }


    tBitSetVolatile tFormula::BitSetVolatile() {
        return(m_BitSetVolatile);
    }
    
    tBitSetVolatile tFormula::BitSetVolatile() const {
        return(m_BitSetVolatile);
    }

    tBool tFormula::IsVolatile() {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_All)));
    }
    
    tBool tFormula::IsVolatile() const {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_All)));
    }

	tBool tFormula::IsRowVolatile() {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_Row)));
    }
    
	tBool tFormula::IsRowVolatile() const {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_Row)));
    }

	tBool tFormula::IsColVolatile() {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_Col)));
    }
    
	tBool tFormula::IsColVolatile() const {
        return(m_BitSetVolatile.Value(tByte(tVolatile::t_Col)));
    }


	tString tFormula::FormulaKey() const { return(m_FormulaKey); }

	void tFormula::FormulaKey(tString sFormulaKey) { m_FormulaKey = sFormulaKey; }

	tString tFormula::Key() const {
		return(m_FormulaKey);
	}
    
 
    tString tFormula::ClassOrRangeStr(const tCell* sCellRoot ,tLexer* sLex, tIndex sIndex, tLexerToken* sToken,tBool& sIsCellClassOrRangeNamed, tIndex& sExtraRefs, tBool sR1C1) const {
        tStringStream wStream;
        sExtraRefs = 0;
        const tVectorItem* wVectorRef=sCellRoot->VectorRef();
        // Get Time Cell Or range
        tItem* wItem = wVectorRef->at(sIndex);
        tSheet* wSheetRoot=sCellRoot->Sheet();
        if (wItem != nullptr) {
            tCell* wCell = wItem->Cell();
            tRange* wRange = wItem->Range();
            // Grid lookup validates tCell refs only; tCellAttribute lives on the class allocator.
            if (wCell != nullptr && wItem->Type() == tTypeItem::t_Cell) {
                tColRowCellRange* wHostCr = wCell->ColRowCellRange();
                if (wHostCr == nullptr ||
                    wHostCr->Cell(wCell->RowIndex(), wCell->ColIndex()) != wCell) {
                    return "#REF!";
                }
            }
            if (wRange != nullptr) {
                tSheet* wRangeSheet = wRange->Sheet();
                if (wRangeSheet == nullptr) {
                    return "#REF!";
                }
                tColRowCellRange* wRangeCr = wRangeSheet->ColRowCellRange();
                if (wRangeCr == nullptr ||
                    wRangeCr->Range(wRange->TopIndex(), wRange->LeftIndex(),
                                    wRange->BottomIndex(), wRange->RightIndex()) != wRange) {
                    return "#REF!";
                }
            }
            // FormulaKey "name" placeholder with a single pushed cell: resolve via workbook
            // reverse map (compile sets IsNamed on the cell; covering Range may not be marked).
            if (sIsCellClassOrRangeNamed && wCell != nullptr) {
                tWorkBook* wBook = wSheetRoot->WorkBook();
                if (wBook != nullptr) {
                    tColRowCellRange* wCr = wCell->ColRowCellRange();
                    tRange* wRangeCell = (wCr != nullptr)
                        ? wCr->Range(wCell->RowIndex(), wCell->ColIndex(),
                                     wCell->RowIndex(), wCell->ColIndex())
                        : nullptr;
                    tString wName;
                    if (wRangeCell != nullptr) {
                        wName = wBook->FindRangeNamed(wRangeCell->AllocatorRef(), wRangeCell->Sheet());
                    }
                    if (!wName.empty()) {
                        return wName;
                    }
                }
            }
            // Is Range Named take range named
            // SearchRange
            if (wCell!=nullptr) {
                if (wCell->IsNamed()) {
                tRange* wRangeCell=wCell->Sheet()->ColRowCellRange()->Range(wCell->RowIndex(),wCell->ColIndex(),wCell->RowIndex(),wCell->ColIndex());
                    if (wRangeCell!=nullptr) {
                            if (wRangeCell->IsNamed()) {
                                wRange=wRangeCell;
                                wCell=nullptr;
                            }
                    }
                }
            }
            if (wCell != nullptr) {
                if (wCell->Sheet()!=wSheetRoot) {
                    tString wSheetName=wCell->Sheet()->Name();
                    // Sheet for FormulaNamed ==================================
                    if (wSheetName==CstSheetNamed) {
                        // Prefer registered name (stable); column-2 label may hold error values that stringify as #VALUE!.
                        tFormulaNamed* wFormulaNamed = wSheetRoot->WorkBook()->FindFormulaNamedByCell(wCell);
                        if (wFormulaNamed != nullptr) {
                            wStream << wFormulaNamed->Name();
                            return(wStream.str());
                        }
                        // Legacy: display visible name string when FindFormulaNamedByCell misses (e.g. old workbooks).
                        // Name label lives at kFormulaNamedNameLabelCol (column I), not column 2 (was moved for spill parity).
                        tCell* wCellLabel =
                            wCell->Sheet()->EnsureCell(wCell->RowIndex(),tFormulaNamed::kFormulaNamedNameLabelCol);
                        wStream << wCellLabel->Value();
                        return(wStream.str());
                    }
                    wSheetName = SheetNameForFormula(wSheetName);
                    wStream << wSheetName << "!";
                }
                tCellAttribute* wCellAttribute=dynamic_cast<tCellAttribute*>(wCell);
                if (wCellAttribute!=nullptr) {
                    tCell* wCellParent=wCell->Sheet()->Cell(wCellAttribute->RowIndex(), wCellAttribute->ColIndex());
                    tCellClassAttribute* wCellClass=wCellParent->ClassAttribute();
                    if (wCellClass!=nullptr) {
                        sIsCellClassOrRangeNamed=true;
                        wStream << wCellClass->RefName();
                        return(wStream.str());
                    }
        
                }
                if (!sR1C1) {
                    if (sToken->LockCol()) wStream << "$";
                    wStream << Base10ToAlpha(wCell->ColIndex());
                    if (sToken->LockRow()) wStream << "$";
                    wStream << wCell->RowIndex();
                } else {
                    //cout << " Row:" << wCell->RowIndex() << "- this:" << RowIndex() << endl;
                    //cout << " Col:" << wCell->ColIndex() << "- this:" << ColIndex() << endl;
                    wStream << "R";
                    if (!sToken->LockRow()) {
                        wStream << "[";
                        wStream << wCell->RowIndex() - sCellRoot->RowIndex();
                        wStream << "]";
                    } else {
                        wStream << wCell->RowIndex();
                    }
                    wStream << "C";
                    if (!sToken->LockCol()) {
                        wStream << "[";
                        wStream << wCell->ColIndex() - sCellRoot->ColIndex();
                        wStream << "]";
                    }
                    else {
                        wStream << wCell->ColIndex();
                    }
                }
                return(wStream.str());
            }
            if (wRange != nullptr) {
                // Multi-cell named formula spill: show Excel-style name (JoursEtSemaines), not '_$$'!$A$1:$G$6
                if (wRange->Sheet()->Name() == CstSheetNamed) {
                    tFormulaNamed* wFormulaNamed = wSheetRoot->WorkBook()->FindFormulaNamedBySpillRange(wRange);
                    if (wFormulaNamed != nullptr) {
                        sIsCellClassOrRangeNamed = true;
                        return wFormulaNamed->Name();
                    }
                }
                if ((wRange->Sheet()!=wSheetRoot) && (!wRange->IsNamed()) && (!wRange->IsData())) {
                    tString wSheetName = SheetNameForFormula(wRange->Sheet()->Name());
                    wStream << wSheetName << "!";
                }
                //cout << " Top   :" << wRange->TopIndex() << "- this:" << RowIndex() << endl;
                //cout << " Left  :" << wRange->LeftIndex() << "- this:" << ColIndex() << endl;
                //cout << " Bottom:" << wRange->TopIndex() << "- this:" << RowIndex() << endl;
                //cout << " Right :" << wRange->LeftIndex() << "- this:" << ColIndex() << endl;
                // Is Range Name Deleted
                if ((!wRange->IsNamed()) && (sIsCellClassOrRangeNamed)) {
                    wStream << "$";
                    wStream << Base10ToAlpha(wRange->LeftIndex());
                    wStream << "$";
                    wStream << wRange->TopIndex();
                    wStream << ":";
                    wStream << "$";
                    wStream << Base10ToAlpha(wRange->RightIndex());
                    wStream << "$";
                    wStream << wRange->BottomIndex();
                    return(wStream.str());
                }
                
                
                // Is RangeNamed
                if (wRange->IsNamed()) {
                    tString wName = wRange->Name();
                    // Multi-area named range: render the name when we are on
                    // the first area. If the sibling areas also follow in
                    // VectorRef (aggregation context, e.g. SUM(MULTI)), skip
                    // them by reporting the extra count to the caller so the
                    // rendering loop advances over the expansion.
                    //
                    // Overlap caveat: when several names share an area (e.g.
                    // Test1=A1:A2;B1:B2 and Test2=A1:A2;C1:C2), wRange->Name()
                    // returns the most-recently-registered owner. To avoid
                    // mis-labelling the formula, we only accept the candidate
                    // name if its full area sequence matches the next N refs
                    // in VectorRef, otherwise we look for another name whose
                    // sequence does match (preserves the user-typed name even
                    // after an overlap insertion has flipped the reverse map).
                    tWorkBook* wBook = wSheetRoot->WorkBook();
                    tBool wRenderAsName = true;
                    auto wMatchesAt = [&](const std::vector<tRange*>& sCandidate) -> tBool {
                        if (sCandidate.empty()) return(false);
                        if (sCandidate.front() != wRange) return(false);
                        if (sIndex + sCandidate.size() > wVectorRef->size()) return(false);
                        for (size_t i = 0; i < sCandidate.size(); ++i) {
                            tItem* wItemI = wVectorRef->at(sIndex + i);
                            if (wItemI == nullptr || wItemI->Range() != sCandidate[i]) {
                                return(false);
                            }
                        }
                        return(true);
                    };
                    if (wBook != nullptr && !wName.empty()) {
                        std::vector<tRange*> wRanges = wBook->RangeNamedContainer()->Ranges(wName);
                        if (wRanges.size() > 1) {
                            wRenderAsName = false;
                            if (wMatchesAt(wRanges)) {
                                wRenderAsName = true;
                                sExtraRefs = static_cast<tIndex>(wRanges.size() - 1);
                            } else {
                                // Look for a different name whose sequence
                                // does match starting at this ref. Useful
                                // when m_MapRef's last-wins reverse lookup
                                // returns the wrong owner under overlap.
                                tRangeNamedContainer* wContainer = wBook->RangeNamedContainer();
                                std::vector<tString> wAllNames = wContainer->AllNames();
                                for (tString& wCandidateName : wAllNames) {
                                    if (wCandidateName == wName) continue;
                                    std::vector<tRange*> wCandidate = wContainer->Ranges(wCandidateName);
                                    if (wMatchesAt(wCandidate)) {
                                        wName = wCandidateName;
                                        wRenderAsName = true;
                                        sExtraRefs = static_cast<tIndex>(wCandidate.size() - 1);
                                        break;
                                    }
                                }
                                if (!wRenderAsName) {
                                    // Single-area name covering wRange exactly.
                                    for (tString& wCandidateName : wAllNames) {
                                        std::vector<tRange*> wCandidate = wContainer->Ranges(wCandidateName);
                                        if (wCandidate.size() == 1 && wCandidate.front() == wRange) {
                                            wName = wCandidateName;
                                            wRenderAsName = true;
                                            break;
                                        }
                                    }
                                }
                                if (!wRenderAsName && wRanges.front() == wRange) {
                                    // Non-aggregation context: only the first
                                    // area was pushed (e.g. ROWS(MULTI) or
                                    // MULTI+1). Render the user-typed name
                                    // even though siblings are not present.
                                    wRenderAsName = true;
                                }
                            }
                        }
                    }
                    if (wRenderAsName && !wName.empty()) {
                        // Only set the colon/second-anchor skip flag for multi-cell named
                        // ranges stored as CR:CR in the formula key. Single-cell named refs
                        // (e.g. R[0]C4 -> D17) must not suppress the next cell token in
                        // concatenations like ...&"#E:"&R3C[0].
                        if (wRange != nullptr && !wRange->IsCell()) {
                            sIsCellClassOrRangeNamed = true;
                        }
                        return(wName);
                    }
                }

                if (sToken->RowInt() == -1) {
                    wStream << wRange->Sheet()->Name() + "!";
                }
                if (!sR1C1) {
                    if (sToken->LockCol()) wStream << "$";
                    wStream << Base10ToAlpha(wRange->LeftIndex());
                    if (sToken->LockRow()) wStream << "$";
                    wStream << wRange->TopIndex();
                    // ":"
                    sLex->nextKey();
                    // tCell
                    tLexerToken wTokenBottom(sLex->nextKey());

                    wStream << ":";
                    if (wTokenBottom.LockCol()) wStream << "$";
                    wStream << Base10ToAlpha(wRange->RightIndex());
                    if (wTokenBottom.LockRow()) wStream << "$";
                    wStream << wRange->BottomIndex();
                } else {
                    wStream << "R";
                    if (!sToken->LockRow()) {
                        wStream << "[";
                        wStream << wRange->TopIndex() - sCellRoot->RowIndex();
                        wStream << "]";
                    }
                    else {
                        wStream << wRange->TopIndex();
                    }
                    // ":"
                    sLex->nextKey();
                    // tCell
                    wStream << "C";
                    if (!sToken->LockCol()) {
                        wStream << "[";
                        wStream << wRange->LeftIndex() - sCellRoot->ColIndex();
                        wStream << "]";
                    }
                    else {
                        wStream << wRange->LeftIndex();
                    }
                    wStream << ":";
                    tLexerToken wTokenBottom(sLex->nextKey());
                    wStream << "R";
                    if (!wTokenBottom.LockRow()) {
                        wStream << "[";
                        wStream << wRange->BottomIndex() - sCellRoot->RowIndex();;
                        wStream << "]";
                    }
                    else {
                        wStream << wRange->BottomIndex();
                    }
                    wStream << "C";
                    if (!wTokenBottom.LockCol()) {
                        wStream << "[";
                        wStream << wRange->RightIndex() - sCellRoot->ColIndex();;
                        wStream << "]";
                    }
                    else {
                        wStream << wRange->RightIndex();
                    }
                }
            } else {
                if (sIsCellClassOrRangeNamed) {
                    wStream << "#NAME?";
                } else {
                    wStream << "#REF!";
                }
            }
        } else {
            if (sIsCellClassOrRangeNamed) {
                wStream << "#NAME?";
            }
            else {
                wStream << "#REF!";
            }
        }
        return(wStream.str());
    }
    
    tString Bracket(tString sValue, tBool sIs) {
        if (sIs) {
            sValue="["+sValue+"]";
        }
        return(sValue);
    }
    
    static tString TableStrStripQuotes(const tString& sRaw) {
        if (sRaw.size() < 2) {
            return sRaw;
        }
        const tChar wFirst = sRaw.front();
        const tChar wLast = sRaw.back();
        if ((wFirst == '\'' && wLast == '\'') || (wFirst == '"' && wLast == '"')) {
            return sRaw.substr(1, sRaw.size() - 2);
        }
        return sRaw;
    }

    // Numeric column tokens inside table $...$ segments (TokenFormula) are 1-based ordinals;
    // tColumnData::Index() / RangeData JSON use 0-based column indices.
    static tSize TableStrNumericTokenToZeroBasedIndex(tInt sLexCol) {
        if (sLexCol <= 0) {
            return 0;
        }
        return static_cast<tSize>(sLexCol - 1);
    }

    tString tFormula::TableStr(const tCell* sCellRoot, tLexer* sLex, tIndex sIndex,tLexerToken* sLexerToken,tString sSep,tChar sArg,tBool sUser,const tString* sExplicitTableName) const {
        // Get Cell or Range
        const tVectorItem* wVectorRef=sCellRoot->VectorRef();
        tItem* wItem = wVectorRef->at(sIndex);
        // Skip $
        *sLexerToken = sLex->nextKey();
       
        tVectorString wResults;

        tIndex wNumCol1=-1;
        tIndex wNumCol2=-1;
        
        tIndex wIndexCol1=-1;
        tIndex wIndexCol2=-1;

        // Get Element
        tColRowCellRange* wColRowCellRangeRoot=sCellRoot->ColRowCellRange();
        tSheet* wSheetRoot=wColRowCellRangeRoot->Sheet();
        tWorkBook* wWorkBook=wSheetRoot->WorkBook();
   
        // By Defautt search on sCellRoot
        tString wTableNameRoot="";
        tRange* wTableRange;
         tie(wTableNameRoot,wTableRange)= wColRowCellRangeRoot->FindRangeDataCovered(sCellRoot->RowIndex(),
                                                                                     sCellRoot->ColIndex());
        tBool wNotTable=(sUser) && (wTableRange!=nullptr);
        tString wThisRow="[#This Row]";
        // Push Table Name ==========
        if (wNotTable) {
            wThisRow="@";
        }
        wResults.push_back("[");
    
    
        tBool wPendingMinusForIndex=false;
        while (!sLexerToken->is_one_of(tKind::End, tKind::Unexpected,tKind::Dollar)) {
#ifdef debugformula
            cout << "Lexer:" << sLexerToken->Kind() << ":" << sLexerToken->Lexeme() << endl;
#endif
            switch (sLexerToken->Kind()) {
                case  tKind::Identifier : {
                    
                    break;
                }
                case tKind::LabelSquare : {
                    tBool wIsSpecialKey=false;
                    switch (sLexerToken->Lexeme()[0]) {
                        case 'r': wResults.push_back(wThisRow); wIsSpecialKey=true; break;
                        case 'h': wResults.push_back("[#Headers]");  wIsSpecialKey=true; break;
                        case 'a': wResults.push_back("[#All]"); wIsSpecialKey=true; break;
                        case 't': wResults.push_back("[#Totals]"); wIsSpecialKey=true; break;
                        default: break;
                    }
                    if (!wIsSpecialKey) {
                        tClassString wColStr(sLexerToken->Lexeme());
                        if (wColStr.IsInteger()) {
                            tInt wNumCol=std::atoi(wColStr().c_str());
                            if (wNumCol < 0) {
                                // Defensive clamp: table selectors are serialized as non-negative 0-based indices.
                                wNumCol = 0;
                            }
                            if (wIndexCol1==-1) {
                                wIndexCol1=static_cast<tIndex>(wResults.size());
                                wNumCol1=wNumCol;
                            } else {
                                wIndexCol2=static_cast<tIndex>(wResults.size());
                                wNumCol2=wNumCol;
                            }
                            wResults.push_back("[#REF!]");
#ifdef debugformuladata
                            cout << "TableStr lexer: LabelSquare numeric column token ->[#REF!] numCol="
                                 << wNumCol << " labelSquareLex=" << sLexerToken->Lexeme() << endl;
#endif
                        }
                    }
                    break;
                }
                /*
                case tKind::LeftCurly: {
                    tVectorString wList;
                    while (!sLexerToken->is_one_of(tKind::End, tKind::Unexpected,tKind::RightCurly)) {
                        switch (sLexerToken->Kind()) {
                            case tKind::Integer:
                            case tKind::Float:
                            case tKind::Bool:
                            case tKind::LabelSimple:
                            case tKind::LabelDouble: {
                                tString wNumCol=sLexerToken->Lexeme();
                                wList.push_back(sLexerToken->Lexeme());
                                break;
                            }
                            default:
                                break;
                        }
                        *sLexerToken = sLex->nextKey();
                    }
                    
                    //wResults.push_back(wList);
                    break;
                }
                */
                case tKind::Integer: {
                    tInt wNumCol=std::atoi(sLexerToken->Lexeme().c_str());
                    if (wPendingMinusForIndex) {
                        wNumCol = -wNumCol;
                        wPendingMinusForIndex = false;
                    }
                    if (wNumCol < 0) {
                        // Defensive clamp: avoid emitting negative table indices in FormulaStr.
                        wNumCol = 0;
                    }
                    if (wIndexCol1==-1) {
                        wIndexCol1=static_cast<tIndex>(wResults.size());
                        wResults.push_back("#REF!");
                        wNumCol1=wNumCol;
#ifdef debugformuladata
                        cout << "TableStr lexer: Integer column index (col1 slot) lex=" << sLexerToken->Lexeme()
                             << " -> placeholder #REF! until table resolve" << endl;
#endif
                    } else {
                        wIndexCol2=static_cast<tIndex>(wResults.size());
                        wResults.push_back("#REF!");
                        wNumCol2=wNumCol;
#ifdef debugformuladata
                        cout << "TableStr lexer: Integer column index (col2 slot) lex=" << sLexerToken->Lexeme()
                             << " -> placeholder #REF! until table resolve" << endl;
#endif
                    }
                    break;
                }
               
                case tKind::Comma:  {
                    // User form is [@[Col]], not [@;[Col]] — internal FormulaKey always uses US comma after [r].
                    if (wNotTable && !wResults.empty() && wResults.back() == "@") {
                        break;
                    }
                    wResults.push_back(tString(1, sArg));
                    break;
                }
                case tKind::Minus: {
                    // In table index context, keep sign for next integer token instead of serializing raw '-'.
                    wPendingMinusForIndex = true;
                    break;
                }
                case tKind::Colon:  wResults.push_back(":"); break;
                default:
                    
                    wResults.push_back(sLexerToken->Lexeme());
                  break;
            }
            *sLexerToken = sLex->nextKey();
        }
#ifdef debugformuladata
        {
            tString wJoined;
            for (const auto& wPart : wResults) {
                wJoined += wPart;
            }
            cout << "TableStr after $-segment: numCol1=" << wNumCol1 << " numCol2=" << wNumCol2
                 << " idxCol1=" << wIndexCol1 << " idxCol2=" << wIndexCol2
                 << " parts=\"" << wJoined << "\""
                 << " explicitTable=" << (sExplicitTableName ? *sExplicitTableName : tString("(none)"))
                 << endl;
        }
#endif
      
        // Resolve table + columns from VectorRef[sIndex] (wItem), not from the FormulaKey literal.
        // FormulaKey uses "name$...$" for structured refs; after a table rename, display still follows
        // FindRangeDataCovered on this referenced cell/range.
        tIndex wRow=-1;
        tIndex wCol=-1;

        tSheet* wSheetTable=wItem->Sheet();
   
        tCell* wCell=wItem->Cell();
        if (wCell!=nullptr) {
           wRow=wCell->RowIndex();
           wCol=wCell->ColIndex();
        }
        tRange* wRange=wItem->Range();
        if (wRange!=nullptr) {
           wRow=wRange->TopIndex();
           wCol=wRange->LeftIndex();
        }
#ifdef debugformuladata
        cout << "TableStr geometry: refIndex=" << sIndex << " row=" << wRow << " col=" << wCol
             << " itemCell=" << (wCell != nullptr) << " itemRange=" << (wRange != nullptr) << endl;
#endif
        // Find Coverde
        tString wTableName="";
        tRange* wSearchRange=nullptr;
        tie(wTableName,wSearchRange)= wSheetTable->ColRowCellRange()->FindRangeDataCovered(wRow,wCol);
        tRangeData* wRangeData = nullptr;
        if (wSearchRange != nullptr) {
#ifdef debugformuladata
            cout << "TableStr FindRangeDataCovered ok tableName=\"" << wTableName << "\" range="
                 << wSearchRange->StrRef(true) << endl;
#endif
            wRangeData = wWorkBook->RangeData(wTableName);
#ifdef debugformuladata
            if (wRangeData == nullptr) {
                cout << "TableStr: RangeData missing for covered name=\"" << wTableName << "\"" << endl;
            }
#endif
        } else {
#ifdef debugformuladata
            cout << "TableStr FindRangeDataCovered miss row=" << wRow << " col=" << wCol << endl;
#endif
        }
        // Find for all
        if ((wSearchRange == nullptr || wRangeData == nullptr) && sExplicitTableName != nullptr && !sExplicitTableName->empty()) {
            const tString wHint = TableStrStripQuotes(*sExplicitTableName);
            tRange* wNamed = wWorkBook->FindRangeNamed(wHint);
#ifdef debugformuladata
            if (wNamed == nullptr) {
                cout << "TableStr explicit hint: FindRangeNamed(\"" << wHint << "\") -> null" << endl;
            } else if (!wNamed->IsData()) {
                cout << "TableStr explicit hint: range \"" << wHint << "\" ref=" << wNamed->StrRef(true)
                     << " IsData=0" << endl;
            }
#endif
            if (wNamed != nullptr && wNamed->IsData()) {
#ifdef debugformuladata
                cout << "TableStr explicit hint: named data range ok " << wNamed->StrRef(true) << endl;
#endif
                tRangeData* wTryRd = wWorkBook->RangeData(wHint);
                if (wTryRd != nullptr) {
#ifdef debugformuladata
                    cout << "TableStr explicit hint: RangeData ok for \"" << wHint << "\"" << endl;
#endif
                    wTableName = wHint;
                    wSearchRange = wNamed;
                    wRangeData = wTryRd;
                } else {
#ifdef debugformuladata
                    cout << "TableStr explicit hint: RangeData null for \"" << wHint << "\" range="
                         << wNamed->StrRef(true) << endl;
#endif
                }
            }
        }
        if (wSearchRange!=nullptr) {
            // Table
            if (wRangeData==nullptr) {
                wRangeData=wWorkBook->RangeData(wTableName);
            }
            if (wRangeData==nullptr) {
#ifdef debugformuladata
                cout << "TableStr: abort no RangeData for table=\"" << wTableName << "\" searchRange="
                     << wSearchRange->StrRef(true) << endl;
#endif
                wResults.push_back("]");
                tString wResult="";
                for(tInt wInd=0; wInd<wResults.size(); wInd++) { wResult+=wResults[wInd]; }
                return(wResult);
            }
            if (!wNotTable) {
                wResults[0]=sSep+wTableName+sSep+'[';
            }
            // Search column
            if ((wNumCol1!=-1) && (wIndexCol1!=-1)) {
                tColumnData* wColumnData1=nullptr;
                const tSize wZeroBased1 = TableStrNumericTokenToZeroBasedIndex(wNumCol1);
                const tIndex wTableLeft = wSearchRange->LeftIndex();
                wColumnData1=wRangeData->FindColumnByOrdinal(wTableLeft, wZeroBased1);
                if (wColumnData1==nullptr) {
                    wColumnData1=wRangeData->FindColumnByIndex(wZeroBased1);
                }
                if (wColumnData1!=nullptr) {
                    const tString wColLabel = wColumnData1->HeaderLabel(
                        wSearchRange->Sheet(), wSearchRange->TopIndex(), wTableLeft);
#ifdef debugformuladata
                    cout << "TableStr col1 resolved name=\"" << wColLabel << "\" col="
                         << wColumnData1->SheetCol() << " (sought numCol1=" << wNumCol1 << ")" << endl;
#endif
                    tClassString wColStr(wResults[wIndexCol1]);
                    if (wColStr.StartsWith("[") || (wNotTable && wResults.size() > 1 && wResults[1] == "@")) {
                        wResults[wIndexCol1]='['+wColLabel+']';
                    } else {
                        wResults[wIndexCol1]=wColLabel;
                    }
                    if ((wNumCol2!=-1) && (wIndexCol2!=-1)) {
                        const tSize wZeroBased2 = TableStrNumericTokenToZeroBasedIndex(wNumCol2);
                        tColumnData* wColumnData2=wRangeData->FindColumnByOrdinal(wTableLeft, wZeroBased2);
                        if (wColumnData2==nullptr) {
                            wColumnData2=wRangeData->FindColumnByIndex(wZeroBased2);
                        }
                        if (wColumnData2!=nullptr) {
                            const tString wColLabel2 = wColumnData2->HeaderLabel(
                                wSearchRange->Sheet(), wSearchRange->TopIndex(), wTableLeft);
#ifdef debugformuladata
                            cout << "TableStr col2 resolved name=\"" << wColLabel2 << "\" col="
                                 << wColumnData2->SheetCol() << " (sought numCol2=" << wNumCol2 << ")" << endl;
#endif
                           tClassString wColStr2(wResults[wIndexCol2]);
                            if (wColStr2.StartsWith("[") || (wNotTable && wResults.size() > 1 && wResults[1] == "@")) {
                                wResults[wIndexCol2]='['+wColLabel2+']';
                            } else {
                                wResults[wIndexCol2]=wColLabel2;
                            }
                        } else {
#ifdef debugformuladata
                            cout << "TableStr col2 UNRESOLVED numCol2=" << wNumCol2
                                 << " tried FindColumnByIndex(" << wZeroBased2 << ") from lex=" << wNumCol2
                                 << " table=\"" << wTableName << "\" RangeData.Json=" << wRangeData->Json()
                                 << endl;
#endif
                        }
                    }
                } else {
#ifdef debugformuladata
                    cout << "TableStr col1 UNRESOLVED numCol1=" << wNumCol1
                         << " tried FindColumnByIndex(" << wZeroBased1 << ") from lex=" << wNumCol1
                         << " table=\"" << wTableName << "\" RangeData.Json=" << wRangeData->Json() << endl;
#endif
                }
            } // ((wNumCol1!=-1) && (wIndexCol1!=-1))
        } else {
#ifdef debugformuladata
            cout << "TableStr: no wSearchRange (cannot map table). row=" << wRow << " col=" << wCol
                 << " numCol1=" << wNumCol1 << " numCol2=" << wNumCol2
                 << " explicit=" << (sExplicitTableName ? *sExplicitTableName : tString("(none)")) << endl;
#endif
        }
        
        wResults.push_back("]");
        tString wResult="";
        for(tInt wInd=0; wInd<wResults.size(); wInd++) { wResult+=wResults[wInd]; }
#ifdef debugformula
        if (wResult.find("#REF") != tString::npos) {
            cout << "TableStr final (contains #REF): \"" << wResult << "\"" << endl;
        }
#endif
        return(wResult);
    }

    tString tFormula::Str(const tCell* sCellRoot, tBool sR1C1, tBool sUser) const {
        tIndex wIndex = 0; // Index Identifier
        tString wFormulaStr = "";
        // Use lexer for compose formula
        tString  wCode = m_FormulaKey;
        
        const tVectorItem* wVectorRef=sCellRoot->VectorRef();
        // Get lang decimal et arg separator
        tLocale* wLocale=tApplication::Instance()->Locale();
        const tChar wDecimal=wLocale->Decimal();

        tLexer wLex(wCode.c_str());
        wLex.SeparatorDecimal('.'); // US Lang
        tLexerToken wLexerToken = wLex.nextKey();

        // Inside { ... } or between first/second | of |...|, comma is array syntax — do not map to locale list separator
        tInt wCurlyLiteralDepth = 0;
        tBool wInsidePipeLiteral = false;

        tBool wIsRangeNamed=false;
#ifdef  debugformula
        cout << "------------------------------------------------" << endl;
        std::cout << "FormulaStr  -->" << m_FormulaKey<< " " << endl;
#endif
        // Loop until End or Unexpected caracter
        while(!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
#ifdef  debugformula
            std::cout << "Lex -->" << wLexerToken << " ";
#endif
            tString  wTokenLex = wLexerToken.Lexeme();
            
            switch (wLexerToken.Kind()) {
            case tKind::Integer: break;
            case tKind::Float: {
                if (wDecimal!='.') {
                    std::replace(wTokenLex.begin(), wTokenLex.end(), '.', wDecimal);
                }
                break;
            }
            case tKind::Bool:
            case tKind::Plus: break;
            case tKind::Minus: break;
            case tKind::Times: break;
            case tKind::Divide: break;
            case tKind::UnaryMinus: break;
            case tKind::Equal:  break;
            case tKind::NotEqual: wTokenLex = "<>"; break;
            case tKind::GreaterThan: break;
            case tKind::GreaterThanOrEqual: wTokenLex = ">="; break;
            case tKind::LessThan: break;
            case tKind::LessThanOrEqual: wTokenLex = "<="; break;
            case tKind::Sheet: break;
            case tKind::ErrorRef: break;
            case tKind::ErrorName: break;
            case tKind::Attribute: break;
            //case tKind::Exclamation : wTokenLex=""; break; // Sheet1!SkButtom.name
            case tKind::Cell: {
                // FlipFlap
                if (wIsRangeNamed) {
                    wIsRangeNamed=false;
                    wTokenLex = "";
                } else {
                    if (wIndex < wVectorRef->size()) {
                        tIndex wExtra = 0;
                        wTokenLex=ClassOrRangeStr(sCellRoot,&wLex, wIndex, &wLexerToken,wIsRangeNamed,wExtra,sR1C1);
#ifdef  debugformula
                        cout << "wTokenLex[" << wIndex << "] -->" << wTokenLex;
#endif
                        
                        // Next Elem (+ sibling areas consumed for multi-area named range)
                        wIndex += 1 + wExtra;
                    } else {
                        wTokenLex = "#REF!";
                    }
                }
                break;
            }
            case tKind::LeftParen: break;
            case tKind::RightParen: break;
            case tKind::LeftSquare:break;
            case tKind::RightSquare:break;

            case tKind::Identifier: {
                // Inline LAMBDA desugaring: a hidden name (_INLLMB_...) stands for an inline LAMBDA(...) that
                // was rewritten into a hidden named lambda at compile time. Render it back as its verbatim
                // LAMBDA(...) definition so the formula round-trips exactly as the user typed it. Hidden names
                // carry no VectorRef slot (see SetCellDependAndFormulaKey), so wIndex must not advance here.
                if (wTokenLex.rfind("_INLLMB_", 0) == 0) {
                    tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
                    if (wWorkBook != nullptr) {
                        tFormulaNamed* wInline = wWorkBook->FindFormulaNamed(wTokenLex);
                        if (wInline != nullptr) {
                            wTokenLex = wInline->FormulaStr();
                        }
                    }
                    break;
                }
                // The lexer pre-pass encodes a named-range placeholder as
                // either "name" (single-area) or "nameN" (multi-area with N
                // areas). Both must enter the rendering branch that calls
                // ClassOrRangeStr/TableStr, otherwise the formula would be
                // rendered with the literal "name2" text instead of the
                // actual named range.
                auto wIsNamePlaceholder = [](const tString& sLex) -> tBool {
                    if (sLex == "name") return(true);
                    // "nameR": single-area named identifier whose resolution is a multi-cell range
                    // (FormulaNamed spill or NamedRange-on-range). Encoded by SetCellDependAndFormulaKey
                    // to keep the SharedFormulaPool entry distinct from a single-cell "name".
                    if (sLex == "nameR") return(true);
                    // "nameRC": named range + R1C1 offset suffix (Excel MyNameR[-1]C[2]).
                    if (sLex == "nameRC") return(true);
                    if (sLex.size() < 5) return(false);
                    if (sLex.compare(0, 4, "name") != 0) return(false);
                    for (std::size_t i = 4; i < sLex.size(); ++i) {
                        if (sLex[i] < '0' || sLex[i] > '9') return(false);
                    }
                    return(true);
                };
                if (wIsNamePlaceholder(wTokenLex)) {
                    if (wIndex < wVectorRef->size()) {
                        // TABLE ==============================================
                        if (wLex.peek_skip_space()=='$') {
                            wLexerToken=wLex.next();
                            wTokenLex=TableStr(sCellRoot,
                                               &wLex,
                                               wIndex,
                                               &wLexerToken,
                                               "",
                                               wLocale->Arg(),
                                               sUser,
                                               nullptr);
                        } else {
                            tBool wRangeNamed=true;
                            tIndex wExtra = 0;
                            wTokenLex = ClassOrRangeStr(sCellRoot,&wLex, wIndex, &wLexerToken,wRangeNamed,wExtra,sR1C1);
                            wIndex += wExtra;
                        }
                        wIndex++;
                    }
                    else {
                        wTokenLex = "#NAME?";
                    }
#ifdef  debugformula
                    cout << "wTokenLex[" << wIndex << "] -->" << wTokenLex;
#endif
                } else if (wLex.peek_skip_space()=='$') {
                    // Internal table token: TableName$[...] (compiled structured ref)
                    if (wIndex < wVectorRef->size()) {
                        const tString wTableIdHint = wTokenLex;
                        wLexerToken=wLex.next();
                        wTokenLex=TableStr(sCellRoot,
                                           &wLex,
                                           wIndex,
                                           &wLexerToken,
                                           "",
                                           wLocale->Arg(),
                                           sUser,
                                           &wTableIdHint);
                        wIndex++;
                    } else {
                        wTokenLex = "#REF!";
                    }
                }
                break;
            }
            case tKind::LabelDouble:
            case tKind::LabelSimple: {
                const tString wTableNameHint = wLexerToken.Lexeme();
                tString wSep="'";
                if (wLexerToken.Kind()==tKind::LabelDouble) wSep="\"";
                wTokenLex = wSep + wTableNameHint + wSep;
                if (wLex.peek_skip_space()=='$') {
                    wLexerToken=wLex.next();
                    wTokenLex=TableStr(sCellRoot,
                                       &wLex,
                                       wIndex,
                                       &wLexerToken,
                                       wSep,
                                       wLocale->Arg(),
                                       sUser,
                                       &wTableNameHint);
                    wIndex++;
                }
                break;
            }
            // Constant array delimiters (Excel-style)
            case tKind::LeftCurly:
                wCurlyLiteralDepth++;
                break;
            case tKind::RightCurly:
                if (wCurlyLiteralDepth > 0) {
                    wCurlyLiteralDepth--;
                }
                break;
            case tKind::Pipe:
                wInsidePipeLiteral = !wInsidePipeLiteral;
                break;

            case tKind::Ampersand:
            case tKind::Percent:
            case tKind::Hash: break;
             //case tKind::Exclamation: wCstLemon = EXCLAMATION; break;
            case tKind::Dot : break;
            case tKind::Comma: {
                if (wCurlyLiteralDepth > 0 || wInsidePipeLiteral) {
                    break;
                }
                tChar wArg=wLocale->Arg();
                if (wArg!=',') {
                    wTokenLex=tString(1, wArg);
                }
                break;
                }
            case tKind::Semicolon:
                break;
            case tKind::NewLine: break;
            case tKind::Colon:  {
                if (wIsRangeNamed) {
                    wTokenLex="";
                }
                break;
            }
            default: {
                    cout << "Cell::FormulaStr() Error " << std::setw(12) << wLexerToken.Kind() << " |" << wLexerToken.Lexeme() << endl;
                    cout << "formula : " << m_FormulaKey << endl;
                    break;
            }
            }
            wFormulaStr += wTokenLex;
#ifdef  debugformula
            cout << endl;
#endif
            wLexerToken = wLex.nextKey();
        }
        return(wFormulaStr);
    }

	tString tFormula::operator()() const {
		return(m_FormulaKey);
	}

	tString tFormula::Debug() const {
        tStringStream wStream;
		for (const auto& wItemFormula : m_VectorItemFormula) {
			if (wItemFormula.Kind() == tKind::Function) {
				wStream << wItemFormula.Kind() << " " << wItemFormula.Value() << ":" << "NbArg " << wItemFormula.Value().Extra() << endl;
            } else {
				wStream << wItemFormula.Kind() << " " << wItemFormula.Value() << " ";
            }
		}
        return(wStream.str());
	}

	void tFormula::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
		sWriter->Key("formula");
		sWriter->String(m_FormulaKey.c_str());
		sWriter->Key("v");
		sWriter->StartArray();
		for (auto wItemFormula : m_VectorItemFormula) {
			wItemFormula.Json(sWriter);
		}
		sWriter->EndArray();
        sWriter->EndObject();
	}

	void tFormula::Json(const Value& sValue) {
		m_FormulaKey = sValue["formula"].GetString();
		const Value& wVector = sValue["v"];
		assert(wVector.IsArray());
		for (SizeType wIndex = 0; wIndex < wVector.Size(); wIndex++) {
			tItemFormula wItemFormula;	
			wItemFormula.Json(wVector[wIndex]);
			m_VectorItemFormula.push_back(wItemFormula);
		}
	}
	
	
#ifdef checksp
	/// @brief      Check.
	void tFormula::Check() const {
		if (m_VectorItemFormula.empty()) {
			tStringStream wError;
			wError << "throw: Formula :" << m_FormulaKey << " m_VectorItemFormula  empty !";
			throw(tExceptionInternalError(wError.str()));
		}
	}
#endif


}

