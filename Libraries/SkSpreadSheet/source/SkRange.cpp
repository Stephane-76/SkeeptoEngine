//=============================================================================
// SkSpreadSheet Range
//=============================================================================
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkTools.hpp"

#define _debugjsondb

namespace SkSpreadSheet {

	// tRange ================================================================
	tRange::tRange() :
		tItem(tTypeItem::t_Range, 0, -1, -1),

		m_BottomAllocatorRef(-1),
		m_RightAllocatorRef(-1)
		,m_AllocatorRef(-1)
	{}

	void tRange::Clear() {
		tItem::Clear();
	}

    tColRow* tRange::Top() { return(ColRowCellRange()->ColRowByAllocatorRef(m_RowAllocatorRef, true)); }
	const tColRow* tRange::Top() const { return(ColRowCellRange()->ColRowByAllocatorRef(m_RowAllocatorRef, true)); }
	tColRow* tRange::Left() { return(ColRowCellRange()->ColRowByAllocatorRef(m_ColAllocatorRef,false)); }
	const tColRow* tRange::Left() const { return(ColRowCellRange()->ColRowByAllocatorRef(m_ColAllocatorRef,false)); }
	
	tColRow* tRange::Bottom() { return(ColRowCellRange()->ColRowByAllocatorRef(m_BottomAllocatorRef,true)); }
	const tColRow* tRange::Bottom() const { return(ColRowCellRange()->ColRowByAllocatorRef(m_BottomAllocatorRef,true)); }
	
	tColRow* tRange::Right() { return(ColRowCellRange()->ColRowByAllocatorRef(m_RightAllocatorRef,false)); }
	const tColRow* tRange::Right() const { return(ColRowCellRange()->ColRowByAllocatorRef(m_RightAllocatorRef,false)); }

    tIndex tRange::TopIndex() { tColRow* wRow = Top(); return(wRow != nullptr ? wRow->Index() : -1); }
	tIndex tRange::LeftIndex() { tColRow* wCol = Left(); return(wCol != nullptr ? wCol->Index() : -1); }
	tIndex tRange::BottomIndex() { tColRow* wRow = Bottom(); return(wRow != nullptr ? wRow->Index() : -1); }
	tIndex tRange::RightIndex() { tColRow* wCol = Right(); return(wCol != nullptr ? wCol->Index() : -1); }

    tIndex tRange::IterateBottom() {
        const tIndex wBottom = BottomIndex();
        if (wBottom < Cst_MaxRow) {
            return wBottom;
        }
        tColRowCellRange* wCr = ColRowCellRange();
        if (wCr == nullptr) {
            return wBottom;
        }
        const tIndex wLast = wCr->LastRow();
        return wBottom <= wLast ? wBottom : wLast;
    }

    tIndex tRange::IterateRight() {
        const tIndex wRight = RightIndex();
        if (wRight < Cst_MaxCol) {
            return wRight;
        }
        tColRowCellRange* wCr = ColRowCellRange();
        if (wCr == nullptr) {
            return wRight;
        }
        const tIndex wLast = wCr->LastCol();
        return wRight <= wLast ? wRight : wLast;
    }

    tIndex tRange::AttachBottomBound() {
        return BottomIndex() >= Cst_MaxRow ? IterateBottom() : BottomIndex();
    }

    tIndex tRange::AttachRightBound() {
        return RightIndex() >= Cst_MaxCol ? IterateRight() : RightIndex();
    }

	tAllocatorRef tRange::AllocatorRef() { 
		return(m_AllocatorRef); 
	}

	void  tRange::AllocatorRef(tAllocatorRef sAllocatorRef) { m_AllocatorRef = sAllocatorRef; }


    void tRange::Set(const tRefItem sRef, const tAllocatorRef sTopAllocatorRef, const tAllocatorRef sLeftAllocatorRef, tAllocatorRef sBottomAllocatorRef, const tAllocatorRef sRightAllocatorRef) {
        m_ColRowCellRangeRef=sRef;
		m_RowAllocatorRef = sTopAllocatorRef;
		m_ColAllocatorRef = sLeftAllocatorRef;
		m_BottomAllocatorRef = sBottomAllocatorRef;
		m_RightAllocatorRef = sRightAllocatorRef;
	}


	const tString tRange::StrRef(tBool sSheetName) const {
		tStringStream wStream;
        if (sSheetName) wStream << Sheet()->Name() <<"!";
		wStream << Base10ToAlpha(Left()->Index()) << Top()->Index() << ":";
		wStream << Base10ToAlpha(Right()->Index()) << Bottom()->Index();
		return(wStream.str());
	};

	const tTempoRect tRange::Rect(){
		return(tTempoRect(Top()->Index(), Left()->Index(), Bottom()->Index(), Right()->Index()));
	};

	const tString tRange::Name() {
		// Search if t_Named
		if (IsNamed()) 	return(WorkBook()->FindRangeNamed(this->AllocatorRef(), Sheet()));
		return("");
	}
    tBool tRange::IsCell() {
        return((m_RowAllocatorRef==m_BottomAllocatorRef) && (m_ColAllocatorRef==m_RightAllocatorRef));
    }
        
    tCell* tRange::Cell() {
        return(ColRowCellRange()->Cell(TopIndex(),LeftIndex()));
    };

    tCell* tRange::EnsureCell() {
        return(ColRowCellRange()->EnsureCell(TopIndex(),LeftIndex()));
    };

	tBool tRange::CoveredCell(tInt sRow, tInt sCol) {
			return(((sRow >= Top()->Index()) &&
				(sRow <= Bottom()->Index()) &&
				(sCol >= Left()->Index()) &&
				(sCol <= Right()->Index())));
	}

	tBool tRange::IntersectRect(tRect* sRect) {
		if (sRect->Top() > Bottom()->Index()) return(false);
		if (sRect->Left() > Right()->Index()) return(false);
		if (sRect->Bottom() < Top()->Index()) return(false);
		if (sRect->Right() < Left()->Index()) return(false);
		return(true);
	}

    tBool tRange::EncloseRect(tRect* sRect) {
        // Return true when sRect is fully inside this range
        if (sRect->Top() < Top()->Index()) return(false);
        if (sRect->Left() < Left()->Index()) return(false);
        if (sRect->Bottom() > Bottom()->Index()) return(false);
        if (sRect->Right() > Right()->Index()) return(false);
        return(true);
    }

    tBool tRange::InsideRect(tRect* sRect) {
        // Return true when this range is fully inside sRect
        if (Top()->Index()    < sRect->Top()) return(false);
        if (Left()->Index()   < sRect->Left()) return(false);
        if (Bottom()->Index() > sRect->Bottom()) return(false);
        if (Right()->Index()  > sRect->Right()) return(false);
        return(true);
    }

    tBool tRange::IsEmpty() {
        return((!IsMerged()) && 
               (!IsNamed()) &&
               (!IsData()) &&
               (!IsConditionalFormat()) &&
               (!IsMatOrigin()) &&
               (!IsSpillRange()) &&
               (NotDependent())
              );
    }

	void tRange::ClearDependent() {
		// Delete cell dependent of cell in vector
		for (tItem* wItem : *ContainerCellDepend()->Container()) {
			// If Error #NAME? wItem can be null
			if (wItem != nullptr) {
				tCell* wCell = wItem->Cell();
				if (wCell != nullptr) {
					for (tInt wIndex = 0; wIndex < wCell->VectorRef()->size(); wIndex++) {
						if ((*wCell->VectorRef())[wIndex] == this) {
							(*wCell->VectorRef())[wIndex] = nullptr;
					}

					}
				}
			}
		}
		ContainerCellDepend()->Container()->clear();
	}
    
    void tRange::Json(Writer<StringBuffer>* sWriter) {
		sWriter->Key("r");
		sWriter->String(StrRef().c_str());
		
		if (IsNamed()) {
			sWriter->Key("n");
            sWriter->String(Name().c_str());
        } 
		if (IsMerged()) {
			sWriter->Key("m");
			sWriter->String("y");
		}
		if (IsData()) {
			sWriter->Key("d");
			sWriter->String("y");
		}
		if (IsConditionalFormat()) {
			sWriter->Key("cf");
			sWriter->String("y");
		}
		if (!m_Extension.Empty()) {
			sWriter->Key("ex");
			sWriter->Int(static_cast<int>(m_Extension.BitSet()));
		}
    }

	tBool tRange::operator == (tRange& sRange) {
        //cout << "(" << StrRef() << "==" << sRange.StrRef()<< ")" << endl;
        return((Top()->Index() == sRange.Top()->Index()) &&
            (Left()->Index() == sRange.Left()->Index()) &&
            (Bottom()->Index() == sRange.Bottom()->Index()) &&
            (Right()->Index() == sRange.Right()->Index()));
	}

	tBool tRange::operator < (tRange& sRange) {
        //cout << "(" << StrRef() << "<" << sRange.StrRef()<< ")" << endl;
        if (Top()->Index() == sRange.Top()->Index()) {
            if (Left()->Index() == sRange.Left()->Index()) {
                if (Bottom()->Index() == sRange.Bottom()->Index()) {
                    return(Right()->Index() < sRange.Right()->Index());
                }
                else { return(Bottom()->Index() < sRange.Bottom()->Index()); }
            }
            else { return(Left()->Index() < sRange.Left()->Index()); }
        }
        else { return(Top()->Index() < sRange.Top()->Index()); }
	}

#ifdef checksp
	/// @brief      Check.
	void tRange::Check() {
		// 1 Verify Formula m_VectorRef
        if ((Top()==nullptr) || (Bottom()==nullptr) || (Left()==nullptr) || (Right()==nullptr)) {
            return;
        }
		for (auto wItemDepend : *m_ContainerCellDepend.Container()) {
			if (wItemDepend == nullptr) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << StrRef() << " item nullptr on m_ContainerCellDepend !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
			tRange* wRange = wItemDepend->Range();
			if (wRange != nullptr) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << StrRef() << " Range in " << wRange->StrRef() << "m_ContainerCellDepend !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));

			}
			tCell* wCell = wItemDepend->Cell();
			tBool wOk = false;
			for (auto wCellRange : *wCell->VectorRef()) {
				if (wCellRange == this) {

					wOk = true;
				}
			}
			if (!wOk) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << StrRef();
				wStream << " Cell " << wCell->StrRef() << " not in m_VectorRef !";
                cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
		}
      
		// ==========================================================
		tColRowCellRange* wColRowcellRange = ColRowCellRange();
		const tIndex wCheckRowEnd = AttachBottomBound();
		for (tInt wRow = Top()->Index(); wRow <= wCheckRowEnd; wRow++) {
			tColRow* wColRow = wColRowcellRange->Row(wRow);
			if (wColRow == nullptr) {
				continue;
			}
			if (!wColRow->RangeInList(this)) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << Sheet()->Name() << "!" << StrRef() << " AllocatorRef(" << AllocatorRef() << ")";
				wStream << " Row " << wRow << " not in ContainerRange !";
				cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
		}
		const tIndex wCheckColEnd = AttachRightBound();
		for (tInt wCol = Left()->Index(); wCol <= wCheckColEnd; wCol++) {
			tColRow* wColRow = wColRowcellRange->Col(wCol);
			if (wColRow == nullptr) {
				continue;
			}
			if (!wColRow->RangeInList(this)) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << wColRowcellRange ->Sheet()->Name()<< "!" << StrRef() << " AllocatorRef'" << AllocatorRef() << ")";
				wStream << " Col " << wCol << " not in ContainerRange !";
				cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
		}
		// Good Allocator
		if (wColRowcellRange->Range(m_AllocatorRef) == 0) {
			tStringStream wStream;
			wStream << "throw: Memory allocator false  range " << StrRef();
			cout << wStream.str() << endl;
			throw(tExceptionInternalError(wStream.str()));
		}
		/* 
		if ((NotDependent()) && (Extension() != tExtension::t_Named)) {
			tStringStream wStream;
			wStream << "throw:Check error on range " << StrRef() << " Range Empty ";
			//cout << wStream.str() << endl;
			//throw(tExceptionInternalError(wStream.str()));
		}
		*/

		// ==========================================================
		// Test Conditional Format
		if (IsConditionalFormat()) {
			tConditionalFormatContainer* wConditionalFormatContainer=ColRowCellRange()->ConditionalFormatContainer();
			if (wConditionalFormatContainer==nullptr) {
				tStringStream wStream;
				wStream << "throw: Check error on range " << StrRef() << " Conditional Format Container not found";
				cerr << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
			}
            wConditionalFormatContainer->CheckRange(this);
		}
	}

#endif

	tString tRange::Debug() {
        tStringStream wStream;
        wStream << StrRef() << " Alloc(" << m_AllocatorRef << ") -->:" << this << " ";
        if (IsMerged()) wStream << " Merged ";
        if (IsNamed())  wStream << "Named ";
		if (IsData()) wStream << "Data ";
        if (IsConditionalFormat()) wStream << "Conditional Format->";
		if (IsCFHR()) wStream << "CFHR ";
		if (IsCFDB()) wStream << "CFDB ";
		if (IsCFCS()) wStream << "CFCS ";
		if (IsCFIS()) wStream << "CFIS ";
		if (IsCFCF()) wStream << "CFCF ";
		if (IsMatOrigin()) wStream << "MatOrigin ";
		if (IsMatExtend()) wStream << "MatExtend ";
		if (IsSpillRange()) wStream << "SpillRange ";
        if (m_ContainerCellDepend.Container()->size()>0) wStream << "Depend ";
		for (auto wItem : *m_ContainerCellDepend.Container()) {
			tCell* wCell = wItem->Cell();
			if (wCell != nullptr) {
                wStream << "," << wCell->StrRef();
			}
			else {
                wStream << ",nullptr (Error)";
			}
		}
        wStream << endl;
        return(wStream.str());
	}
	
} // end of namespace
