//=============================================================================
// SkSpreadSheet ColRow
//=============================================================================
#include "../include/SkColRow.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkWorkBook.hpp"

namespace SkSpreadSheet {

    // CallBack or range =====================================================
	tCallBackRangeFunction::tCallBackRangeFunction(tColRowCellRange* sColRowCellRange) : tSparseArrayCallBack<tAllocatorRef>(), m_Value(), m_ColRowCellRange(sColRowCellRange) {}

	tVariant tCallBackRangeFunction::Value() { return(m_Value); }
	void tCallBackRangeFunction::Value(tVariant sValue) { m_Value = sValue; }

	tBool tCallBackRangeFunction::CallBack(tAllocatorRef sAllocatorRef) {
		return(true); // false for Stop
	};



    SkInline tBool IsInLimits(tBool sIsRow,tIndex sIndex) {
        if (sIsRow) {
            if ((sIndex<1) || (sIndex>Cst_MaxRow)) return(false);
        } else {
            if ((sIndex<1) || (sIndex>Cst_MaxCol)) return(false);
        }
        return(true);
    }


    //= ColRow ===============================================================
    tColRow::tColRow() : tClass(), 
                            m_Index(-1), 
                            m_Size(-1),
                            m_DataVisible(true),
                            m_Css(0),
                            m_ParentRef(0),
                            m_Open(true),
                            m_NbCells(0)
#ifdef checksp
                           ,m_NbCellsCheck(0)
#endif
                             {}
    
    void  tColRow::Clear() {
        DeleteFormat();
        m_ContainerRange.Container()->clear();
        m_Size=-1; // (-1) for DefaultSize
        m_DataVisible=true;
        m_ParentRef = 0;
        m_Open=true;
        m_Children.clear();
        m_NbCells = 0;
#ifdef checksp
        m_NbCellsCheck = 0;
#endif
    }

    void  tColRow::DeleteFormat() {
        if (m_Css!=0) {
            tWorkBook* wWorkBook=tSpreadSheetContainer::Instance()->ActiveWorkBook();
            wWorkBook->DeleteCellFormat(m_Css);
        }
        m_Css=0;
    }


    void tColRow::Index(tIndex sValue) { m_Index = sValue; }
	tIndex tColRow::Index() { return(m_Index); }
	tIndex tColRow::Index() const { return(m_Index); }

	void tColRow::Size(tDouble sValue) { m_Size = sValue; }
	tDouble tColRow::Size() { return(m_Size); }
            
    void tColRow::DataVisible(tBool sValue) { m_DataVisible=sValue; }
    tBool tColRow::DataVisible() {return(m_DataVisible); }
 
    // Guard parent walks against dangling ParentRef / self-cycles / A->B->A (freeze in JsonView).
    static constexpr tInt kMaxTreeParentWalk = 65536;

    tBool tColRow::IsInClosedPath(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        tColRow* wColRow=this;
        tInt wGuard=0;
        while(wColRow->m_ParentRef!=0) {
            tColRow* wParent=sColRowCellRange->ColRowByAllocatorRef(wColRow->m_ParentRef,sIsRow);
            if (wParent==nullptr || wParent==wColRow) {
                return(false);
            }
            wColRow=wParent;
            if (wColRow->m_Open==false) return(true);
            if (++wGuard > kMaxTreeParentWalk) return(false);
        }
        return(false);
    }

    tColRow* tColRow::FirstParentTreeNode(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        tColRow* wColRow=this;
        tInt wGuard=0;
        while(wColRow->m_ParentRef!=0) {
            tColRow* wParent=sColRowCellRange->ColRowByAllocatorRef(wColRow->m_ParentRef,sIsRow);
            if (wParent==nullptr || wParent==wColRow) {
                return(wColRow);
            }
            wColRow=wParent;
            if (!wColRow->IsInClosedPath(sColRowCellRange, sIsRow)) return(wColRow);
            if (++wGuard > kMaxTreeParentWalk) return(wColRow);
        }
        return(wColRow);
    }

    tIndex tColRow::SearchPrecOpen(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        tColRow* wColRow=this;
        tIndex wIndex=wColRow->Index();
        // Is in closed path
        if (IsInClosedPath(sColRowCellRange,sIsRow)) {
            wColRow=FirstParentTreeNode(sColRowCellRange, sIsRow);
            return(wColRow->Index());
        }
        wIndex--;
        if (IsInLimits(sIsRow, wIndex)) {
            wColRow=sColRowCellRange->ColRowByIndex(wIndex,sIsRow);
            if (wColRow==nullptr) return(wIndex);
            if (wColRow->IsInClosedPath(sColRowCellRange, sIsRow)) {
                wColRow=wColRow->FirstParentTreeNode(sColRowCellRange,sIsRow);
            }
        }
        return(wColRow->Index());
    }

    tIndex tColRow::LastTreeDescendantIndex(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        tIndex wLast = m_Index;
        if (!m_Children.empty()) {
            for (auto wChildRef : m_Children) {
                tColRow* wChild = sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wChild == nullptr) {
                    continue;
                }
                tIndex wChildLast = wChild->LastTreeDescendantIndex(sColRowCellRange, sIsRow);
                if (wChildLast > wLast) {
                    wLast = wChildLast;
                }
            }
            return(wLast);
        }
        // Fallback when m_Children is desynced: scan following indices whose
        // ParentRef chain includes this node (contiguous outline block).
        tAllocatorRef wSelfRef = sIsRow
            ? sColRowCellRange->RowAllocatorRef(m_Index)
            : sColRowCellRange->ColAllocatorRef(m_Index);
        if (wSelfRef == 0) {
            return(wLast);
        }
        tIndex wIndex = m_Index + 1;
        while (IsInLimits(sIsRow, wIndex)) {
            tColRow* wNext = sColRowCellRange->ColRowByIndex(wIndex, sIsRow);
            if (wNext == nullptr) {
                break;
            }
            tBool wUnder = false;
            tColRow* wWalk = wNext;
            tInt wGuard = 0;
            while (wWalk != nullptr && wWalk->m_ParentRef != 0) {
                if (wWalk->m_ParentRef == wSelfRef) {
                    wUnder = true;
                    break;
                }
                tColRow* wParent = sColRowCellRange->ColRowByAllocatorRef(wWalk->m_ParentRef, sIsRow);
                if (wParent == nullptr || wParent == wWalk) {
                    break;
                }
                wWalk = wParent;
                if (++wGuard > kMaxTreeParentWalk) {
                    break;
                }
            }
            if (!wUnder) {
                break;
            }
            wLast = wIndex;
            wIndex++;
        }
        return(wLast);
    }

    tIndex tColRow::SearchNextOpen(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        tColRow* wColRow=this;

        // Collapsed node: skip the whole subtree (direct + nested children).
        // This must run even when we ARE the closed parent (not only when InClosedPath).
        if (!wColRow->m_Open) {
            tIndex wLast = wColRow->LastTreeDescendantIndex(sColRowCellRange, sIsRow);
            tIndex wIndex = wLast + 1;
            if (!IsInLimits(sIsRow, wIndex)) {
                return(m_Index);
            }
            wColRow = sColRowCellRange->ColRowByIndex(wIndex, sIsRow);
            if (wColRow == nullptr) {
                return(wIndex);
            }
            while (wColRow->IsInClosedPath(sColRowCellRange, sIsRow)
                   || (sIsRow && !wColRow->DataVisible())) {
                wIndex++;
                if (!IsInLimits(sIsRow, wIndex)) {
                    return(wIndex);
                }
                wColRow = sColRowCellRange->ColRowByIndex(wIndex, sIsRow);
                if (wColRow == nullptr) {
                    return(wIndex);
                }
            }
            return(wColRow->Index());
        }
        
        if (wColRow->IsInClosedPath(sColRowCellRange, sIsRow)) {
            wColRow=FirstParentTreeNode(sColRowCellRange, sIsRow);
            if (wColRow!=nullptr && !wColRow->m_Open) {
                tIndex wLast = wColRow->LastTreeDescendantIndex(sColRowCellRange, sIsRow);
                tIndex wIndex = wLast + 1;
                wColRow=sColRowCellRange->ColRowByIndex(wIndex,sIsRow);
                if (wColRow==nullptr) return(wIndex);
                return(wColRow->Index());
            }
        }
        // Search Next
        tIndex wIndex=wColRow->m_Index;
        wIndex++;
        if (!IsInLimits(sIsRow, wIndex)) return(m_Index);
        wColRow=sColRowCellRange->ColRowByIndex(wIndex,sIsRow);
        if (wColRow==nullptr) return(wIndex);
        while (wColRow->IsInClosedPath(sColRowCellRange, sIsRow)
               || (sIsRow && !wColRow->DataVisible())) {
            wIndex++;
            if (!IsInLimits(sIsRow, wIndex)) return(wIndex);
            wColRow=sColRowCellRange->ColRowByIndex(wIndex,sIsRow);
            if (wColRow==nullptr) return(wIndex);
        }
        
        return(wColRow->Index());
    }


    tBool tColRow::IsVisible(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        return((m_Size!=0) && (!IsInClosedPath(sColRowCellRange, sIsRow))
               && (!sIsRow || m_DataVisible));
    }


    void tColRow::Css(tFormatRef sValue) { m_Css = sValue; }
    tFormatRef tColRow::Css() { return(m_Css); }

    void tColRow::ParentRef(tAllocatorRef sValue) { m_ParentRef = sValue; }
    tAllocatorRef tColRow::ParentRef()  { return(m_ParentRef); }

    tColRow* tColRow::ColRowParent(tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        if (m_ParentRef==0) return(nullptr);
        return(sColRowCellRange->ColRowByAllocatorRef(m_ParentRef, sIsRow));
    }

    void tColRow::Open(tBool sValue) { m_Open = sValue; }
    tBool tColRow::Open()  { return(m_Open); }

    tSize tColRow::NbChildren() { return(m_Children.size()); }

    tBool tColRow::IsChildrenExist(tAllocatorRef sValue) {
        auto wIterator = std::find(m_Children.begin(), m_Children.end(), sValue);
        return(wIterator!= m_Children.end());
    }
    
    tBool tColRow::AddChildren(tAllocatorRef sValue) {
        if (sValue==0) return(false);
        auto wIterator = std::find(m_Children.begin(), m_Children.end(), sValue);
        if (wIterator!= m_Children.end()) {
            return(false);
        }
        // Order is restored by RenumRow/RenumCol (sheet Index scan)
        m_Children.push_back(sValue);
        return(true);
    }
    
    void tColRow::InsertChildren(tAllocatorRef sValue) {
        // no Order before renum
        if (sValue==0) return;
        m_Children.push_back(sValue);
    }

    tBool tColRow::DeleteChildren(tAllocatorRef sValue) {
        auto wIterator = std::find(m_Children.begin(), m_Children.end(), sValue);
        if (wIterator!= m_Children.end()) {
            m_Children.erase(wIterator);
            return(true);
        }
        return(false);
    }
    
    tInt tColRow::Deep(tColRowCellRange* sColRowCellRange,tBool sIsRow) {
        tInt wDeep=0;
        tColRow* wColRow=this;
        tInt wGuard=0;
        while(wColRow->m_ParentRef!=0) {
            tColRow* wParent=sColRowCellRange->ColRowByAllocatorRef(wColRow->m_ParentRef, sIsRow);
            if (wParent==nullptr || wParent==wColRow) {
                break;
            }
            wColRow=wParent;
            wDeep++;
            if (++wGuard > kMaxTreeParentWalk) break;
        }
        return(wDeep);
    }
 
    // Range for calculate path
    tBool tColRow::AddRange(tRange* sRange) {
        return(m_ContainerRange.InsertClass(sRange->AllocatorRef()));
    }

    tBool tColRow::DeleteRange(tRange* sRange) {
        return(m_ContainerRange.DeleteClass(sRange->AllocatorRef()));
    }

    tBool tColRow::RangeInList(tRange* sRange) {
        tColRow::tContainerRange::tIteratorClass wIterator = m_ContainerRange.FindClass(sRange->AllocatorRef());
        return(wIterator != m_ContainerRange.Container()->end());
    }

    void  tColRow::GetIntersection(tColRow* sColRow, tContainerRange::tResult* sResult) {
        m_ContainerRange.GetIntersection(sColRow->ContainerRange(), sResult);
    }

    tColRow::tContainerRange* tColRow::ContainerRange() {
        return(&m_ContainerRange);
    }


    // Json ===============================================================
    void tColRow::Json(Writer<StringBuffer>* sWriter,tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        sWriter->Key("i"); sWriter->Int(m_Index);
        sWriter->Key("s"); sWriter->Double(m_Size);
        if (m_ParentRef!=0) {
             tColRow* wParent=sColRowCellRange->ColRowByAllocatorRef(m_ParentRef, sIsRow);
             if (wParent != nullptr) {
                sWriter->Key("p"); sWriter->Int(wParent->Index());
             }
        }
        if (!m_Open) {
            sWriter->Key("o"); sWriter->Bool(false);
        }
        if (!m_Children.empty()) {
            sWriter->Key("c"); sWriter->StartArray();
            for (auto wChildRef : m_Children) {
                tColRow* wChild=sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wChild!=nullptr) {
                    sWriter->Int(wChild->Index());
                }
            }
            sWriter->EndArray();
        }
          // Format Mutualize
        if (m_Css!=0) {
            tFormatApi* wFormatApi=sColRowCellRange->Sheet()->WorkBook()->FormatApi();
            if (wFormatApi!=nullptr) {
                tSize wIndex=wFormatApi->WriteJsonAddFormat(m_Css);
                sWriter->Key("fo"); sWriter->Int(tInt(wIndex));
            }
        }
   }

    void tColRow::Json(const Value& sValue,tColRowCellRange* sColRowCellRange, tBool sIsRow) {
        m_Index = sValue["i"].GetInt();
        m_Size = sValue["s"].GetDouble();
        m_Open = true;
        if (sValue.HasMember("o")) {
            m_Open = sValue["o"].GetBool();
        }
        if (sValue.HasMember("p")) {
            tIndex wIndex=sValue["p"].GetInt();
            tColRow* wColRow=sColRowCellRange->EnsureColRow(wIndex, sIsRow);
            (void)wColRow;
            if (sIsRow) {
                m_ParentRef=sColRowCellRange->RowAllocatorRef(wIndex);
            } else {
                m_ParentRef = sColRowCellRange->ColAllocatorRef(wIndex);
            }
        }
        if (sValue.HasMember("c")) {
            const Value& wChildren = sValue["c"];
            assert(wChildren.IsArray());
            tAllocatorRef wSelfRef = sIsRow
                ? sColRowCellRange->RowAllocatorRef(m_Index)
                : sColRowCellRange->ColAllocatorRef(m_Index);
            for (SizeType wIndex = 0; wIndex < wChildren.Size(); wIndex++) {
                tIndex wChildIndex=wChildren[wIndex].GetInt();
                tColRow* wChild = sColRowCellRange->EnsureColRow(wChildIndex, sIsRow);
                tAllocatorRef wChildRef=sIsRow
                    ? sColRowCellRange->RowAllocatorRef(wChildIndex)
                    : sColRowCellRange->ColAllocatorRef(wChildIndex);
                InsertChildren(wChildRef);
                // Keep ParentRef in sync when JSON has parent "c" but child lacks "p".
                if (wChild != nullptr && wChild->ParentRef() == 0 && wSelfRef != 0) {
                    wChild->ParentRef(wSelfRef);
                }
            }
        }
        // Get Format
        if (sValue.HasMember("fo")) {
            tFormatApi* wFormatApi=sColRowCellRange->Sheet()->WorkBook()->FormatApi();
            if (wFormatApi!=nullptr) {
                tSize wIndex = sValue["fo"].GetInt();
                tString wFormat=wFormatApi->ReadJsonGetFormat(wIndex);
                tFormatRef wCss=wFormatApi->ApplyCellFormat(wFormat);
                if (wCss!=0) m_Css=wCss; // if error 0 res
            }
        }
    
    }

    void tColRow::IncNbCells() { m_NbCells++; };
    void tColRow::DecNbCells() { m_NbCells--; }
#ifdef checksp
    tInt tColRow::NbCells() { return(m_NbCells); }
#endif

    tBool tColRow::IsEmpty() {
        return(
            (m_ParentRef == 0) &&
            (m_NbCells == 0) &&
            (m_Css ==0) && 
            (m_Children.size() == 0) &&
            (m_ContainerRange.Container()->size() == 0) &&
            (m_Size == -1) // -1 for default size
            );
    }

    tColRow::tVectorColRow*  tColRow::VectorChildren() { return(&m_Children); }

    tBool tColRow::IsVacantForUsedExtent() {
        return(
            (m_ParentRef == 0) &&
            (m_Children.size() == 0) &&
            (m_ContainerRange.Container()->size() == 0));
    }

#ifdef checksp
    void tColRow::Check(tSheet* sSheet, tBool sIsRow) {
        if (m_NbCells!=m_NbCellsCheck) {
            tStringStream wStream;
            wStream << "Check error on ";
            if (sIsRow) {
                wStream << "Row " << m_Index;
            }  else {
                wStream << "Col " << Base10ToAlpha(m_Index);
            }
            wStream << " throw: NbCells Error Real " << m_NbCells << " != Check " << m_NbCellsCheck;
            cout << wStream.str() << endl;
            //throw(tExceptionInternalError(wStream.str()));
        }
        for (auto wRangeRef : *m_ContainerRange.Container()) {
            tRange* wRange = sSheet->ColRowCellRange()->Range(wRangeRef);
            if (wRange!=nullptr) {
                if (sIsRow) {
                    //cout << "Row " << m_Index << " " << wRange->StrRef() << endl;
                    if (!((m_Index >= wRange->TopIndex()) && (m_Index <= wRange->BottomIndex()))) {
                        tStringStream wStream;
                        wStream << "throw: Check error on Row " << m_Index << " Range " << wRange->StrRef() << " Present !";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
                }
                else {
                    //cout << "Col " << Base10ToAlpha(m_Index) << " " << wRange->StrRef() << endl;
                    if (!((m_Index >= wRange->LeftIndex()) && (m_Index <= wRange->RightIndex()))) {
                        tStringStream wStream;
                        wStream << "throw: Check error on Col " << Base10ToAlpha(m_Index) << " Range " << wRange->StrRef() << " Present !";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
                }
            } else {
                tStringStream wStream;
                if (sIsRow) {
                    wStream << "Row "<< m_Index << " ColRowContainer  tRange(";
                    wStream << wRangeRef;
                    wStream << ") =null" ;
                } else {
                    wStream << "Col "<< Base10ToAlpha(m_Index) << " ColRowContainer  tRange(";
                    wStream << wRangeRef;
                    wStream << ") =null" ;
                    
                }
                cerr << wStream.str() << endl;
            }
        }
        // Test Tree ==========================================================
        // Test Parent
        if (m_ParentRef!=0) {
            tColRow* wParent=sSheet->ColRowCellRange()->ColRowByAllocatorRef(m_ParentRef, sIsRow);
            tAllocatorRef wSelfRef=sIsRow
                ? sSheet->ColRowCellRange()->RowAllocatorRef(m_Index)
                : sSheet->ColRowCellRange()->ColAllocatorRef(m_Index);
            if (wParent==nullptr || !wParent->IsChildrenExist(wSelfRef)) {
                tStringStream wStream;
                tString wNameColRow="Col";
                if (sIsRow) wNameColRow="Row";
                    wStream << "throw: Check error on tree child " << wNameColRow << " " << m_Index << " not in list parent " << m_ParentRef << "  !";
                cout << wStream.str() << endl;
                //throw(tExceptionInternalError(wStream.str()));
            }
            
        }
        // Test Children
        if (!m_Children.empty()) {
            for(auto wChildRef : m_Children) {
                tColRow* wColRowChild=sSheet->ColRowCellRange()->ColRowByAllocatorRef(wChildRef, sIsRow);
                if (wColRowChild==nullptr) {
                    tStringStream wStream;
                    tString wNameColRow="Col";
                    if (sIsRow) wNameColRow="Row";
                        wStream << "throw: Check error on tree parent " << wNameColRow << " " << m_Index  << "  [" << wChildRef << "]=Null   !";
                    cout << wStream.str() << endl;
                    return;
                    //throw(tExceptionInternalError(wStream.str()));
                }
                tColRow* wColRowParent=wColRowChild->ColRowParent(sSheet->ColRowCellRange(),sIsRow);
                if (wColRowParent==nullptr) {
                    tStringStream wStream;
                    tString wNameColRow="Col";
                    if (sIsRow) wNameColRow="Row";
                        wStream << "throw: Check error on tree parent " << wNameColRow << " " << wColRowChild->m_ParentRef << " IsNull on  " << m_Index << "  !";
                    cout << wStream.str() << endl;
                    continue;
                    //throw(tExceptionInternalError(wStream.str()));
                }
                if (wColRowParent->Index()!=m_Index) {
                    tStringStream wStream;
                    tString wNameColRow="Col";
                    if (sIsRow) wNameColRow="Row";
                        wStream << "throw: Check error on tree parent " << wNameColRow << " " << wColRowChild->m_ParentRef << " has no parent " << m_Index << "  !";
                    cout << wStream.str() << endl;
                    //throw(tExceptionInternalError(wStream.str()));
                }
            }
        }
    }
#endif

    tBool tColRow::operator == (tColRow sColRow) {
        return(
            (m_Index == sColRow.m_Index) &&
            (m_ParentRef == sColRow.m_ParentRef) &&
            (m_Size == sColRow.m_Size) &&
            (m_ContainerRange.Container() == sColRow.m_ContainerRange.Container())
         );
    }
        
    tBool tColRow::operator < (tColRow sColRow) {
        return(m_Index < sColRow.m_Index);
    }
        
    /// @brief		Debug
    tString tColRow::Debug() {
        tStringStream wStream;
        wStream << "Index=" << m_Index;
        if (m_ParentRef!=0) wStream << " ParentRef=" << m_ParentRef;
        if (!m_Children.empty()) {
            wStream << " {";
            tBool wIsFirst=false;
            for (auto wChildRef : m_Children) {
                if (wIsFirst) wStream << ",";
                wStream << "ref=" << wChildRef;
                wIsFirst=true;
            }
            wStream << "}";
        }
        
        /* Too big
        tContainerRange::tResult* wContainer;
        wContainer = m_ContainerRange.Container();
        for (auto wRange : *wContainer) {
            wStream << " " <<  wRange->StrRef() << " " << wRange->AllocatorRef();
        }
        */
        
        return(wStream.str());
    }

    ostream& operator << (ostream& os, const tColRow& sColRow) {
        os << sColRow.m_Index;
        return os;
    }
} // End of namespace
