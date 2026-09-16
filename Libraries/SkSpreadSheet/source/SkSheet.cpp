//=============================================================================
// SkSpreadSheet Sheet
//=============================================================================
#include "../include/SkSheet.hpp"
#include "../include/SkCalculationPath.hpp"
#include "../include/SkJsonKey.hpp"
#include "../include/SkSpreadSheet.hpp"
namespace SkSpreadSheet {


	// tSheet ================================================================
	tSheet::tSheet() : SkSpAncestor(), m_WorkBookRef(0),
		m_AllocatorRef(0),
		m_Name(""),
        m_AllocatorColRowCellRange(0),
        m_Css(0),
        m_Orientation(tPrintOrientation::Portrait),
        m_FitToPage(true) {
	};

	tSheet::~tSheet() {
		Clear();
	}

    void tSheet::Clear() {
        if (m_Css!=0) {
            WorkBook()->DeleteCellFormat(m_Css);
            m_Css=0;
        }
        if (m_AllocatorColRowCellRange!=0)
                tStaticColRowCellRange::Instance()->Delete(m_AllocatorColRowCellRange);
        m_AllocatorColRowCellRange=0;
        // Set m_ColRowCellRange to nullptr to prevent use-after-free
        m_ColRowCellRange=nullptr;
        SplitClear();
        m_ViewZoomScaleNormal = 0;
        m_ShowGridLines = true;
        m_Orientation = tPrintOrientation::Portrait;
        m_FitToPage = true;
	}

	void tSheet::Set(tAllocatorRef sAllocatorRef, tAllocatorRef  sWorkBookRef, tString sSheetName) {
        if (m_AllocatorColRowCellRange!=0) {
            cout << "m_IndexAllocatorColRowCellRange!=0" << endl;
        }
        m_ColRowCellRange = tStaticColRowCellRange::Instance()->Alloc(m_AllocatorColRowCellRange);
        m_ColRowCellRange->Sheet(this);
        m_ColRowCellRange->SheetAllocator(m_AllocatorColRowCellRange);
		m_WorkBookRef = sWorkBookRef;
		AllocatorRef(sAllocatorRef);
		m_Name = sSheetName;
	}
	tAllocatorRef  tSheet::AllocatorRef() { return(m_AllocatorRef); }

	void tSheet::AllocatorRef(tAllocatorRef sAllocatorRef) { m_AllocatorRef = sAllocatorRef; };


	tWorkBook* tSheet::WorkBook() { return(tSpreadSheetContainer::Instance()->WorkBook(m_WorkBookRef)); }

	tAllocatorRef tSheet::IndexAllocatorColRowCellRange() { return(m_AllocatorColRowCellRange); }

	tColRowCellRange* tSheet::ColRowCellRange() { return(m_ColRowCellRange); }

	tString tSheet::Name() { return(m_Name); }
    void tSheet::Name(tString sName) { m_Name=sName; }

    void tSheet::Css(tFormatRef sValue) { m_Css = sValue; }
    tFormatRef tSheet::Css() { return(m_Css); }

	void tSheet::SplitV(tIndex sValue) { m_Splitter.Col(sValue); }

	tIndex tSheet::SplitV() const { return m_Splitter.Col(); }

	void tSheet::SplitH(tIndex sValue) { m_Splitter.Row(sValue); }

	tIndex tSheet::SplitH() const { return m_Splitter.Row(); }

	void tSheet::SplitClear() {
		m_Splitter.Row(-1);
		m_Splitter.Col(-1);
	}

    void tSheet::ViewZoomScaleNormal(tInt sZoom) {
        m_ViewZoomScaleNormal = sZoom;
    }

    tInt tSheet::ViewZoomScaleNormal() const {
        return m_ViewZoomScaleNormal;
    }

    void tSheet::ShowGridLines(tBool sShow) {
        m_ShowGridLines = sShow;
    }

    tBool tSheet::ShowGridLines() const {
        return m_ShowGridLines;
    }
	
    // ColRow ================================================================
	tColRow* tSheet::EnsureRow(tInt sIndex) { return(m_ColRowCellRange->EnsureRow(sIndex)); }
	tColRow* tSheet::Row(tInt sIndex) { return(m_ColRowCellRange->Row(sIndex)); }

	tColRow* tSheet::EnsureCol(tInt sIndex) { return(m_ColRowCellRange->EnsureCol(sIndex)); }
	tColRow* tSheet::Col(tInt sIndex) { return(m_ColRowCellRange->Col(sIndex)); }

	tCell* tSheet::EnsureCell(tInt sRow, tInt sCol) { return(m_ColRowCellRange->EnsureCell(sRow, sCol)); }
	tBool tSheet::DeleteCell(tIndex sRow, tIndex sCol) { return(m_ColRowCellRange->DeleteCell(sRow, sCol)); };

	tCell* tSheet::Cell(tInt sRow, tInt sCol) { return(m_ColRowCellRange->Cell(sRow, sCol)); }

    tBool tSheet::InsertCellClassAttributeContainer(tCell* sCell) { return(m_ColRowCellRange->InsertCellClassAttributeContainer(sCell)); }

    tBool tSheet::DeleteCellClassAttributeContainer(tCell* sCell) { return(m_ColRowCellRange->EraseCellClassAttributeContainer(sCell)); }

	tCellAttribute* tSheet::CellAttribute(tInt sRow, tInt sCol, tString sAttribute) { return(m_ColRowCellRange->CellAttribute(sRow, sCol,sAttribute)); }

	tCellAttribute* tSheet::EnsureCellAttribute(tInt sRow, tInt sCol, tString sAttribute) { return(m_ColRowCellRange->EnsureCellAttribute(sRow, sCol, sAttribute)); }

	tBool tSheet::DeleteCellAttribute(tInt sRow, tInt sCol, tString sAttribute) { return(m_ColRowCellRange->DeleteCellAttribute(sRow, sCol, sAttribute)); }

    tDouble tSheet::SizeCol(tInt sCol) {
        tColRow* wCol = m_ColRowCellRange->Col(sCol);
        if (wCol == nullptr) {
            return(WorkBook()->DefaultSizeCol());
        }
        tDouble wSize = wCol->Size();
        return((wSize == -1) ? WorkBook()->DefaultSizeCol() : wSize);
    }

	void tSheet::SizeCol(tInt sCol, tDouble sValue) {
		EnsureCol(sCol)->Size(sValue);
	}

	tDouble tSheet::SizeRow(tInt sRow) {
		tColRow* wRow = m_ColRowCellRange->Row(sRow);
		if (wRow == nullptr) {
			return(WorkBook()->DefaultSizeRow());
		}
		tDouble wSize = wRow->Size();
		return((wSize == -1) ? WorkBook()->DefaultSizeRow() : wSize);
      }

	void tSheet::SizeRow(tInt sRow, tDouble sValue) {
		EnsureRow(sRow)->Size(sValue);
	}

	// Range management
	tRange* tSheet::Range(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight) { return(m_ColRowCellRange->Range(sTop,sLeft,sBottom,sRight)); };
	
	tRange* tSheet::Range(tAllocatorRef sAllocatorRef) { return(m_ColRowCellRange->Range(sAllocatorRef)); }

	tRange* tSheet::EnsureRange(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight) { return(m_ColRowCellRange->EnsureRange(sTop, sLeft, sBottom, sRight)); };
	void tSheet::EraseRange(tAllocatorRef sAllocatorRef) { m_ColRowCellRange->DeleteRangeByAllocatorRef(sAllocatorRef); }

	tBool tSheet::EraseRange(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight) { return(m_ColRowCellRange->DeleteRange(sTop, sLeft, sBottom, sRight)); }

	void tSheet::FindRangesCovered(tCell* sCell, tColRow::tContainerRange::tResult* sResult) { m_ColRowCellRange->FindRanges(sCell,sResult); }

    void tSheet::FindRangesCovered(tRect sRect, tVectorRange* sResult) {
        m_ColRowCellRange->FindRangesCovered(sRect, sResult);
    }

	void tSheet::FindRangesCovered(tInt sRow, tInt sCol, tColRow::tContainerRange::tResult* sResult) { m_ColRowCellRange->FindRangesCovered(sRow,sCol,sResult);
    }

    tuple<tString,tRange*> tSheet::FindRangeDataCovered(tInt sRow, tInt sCol) {
        return(m_ColRowCellRange->FindRangeDataCovered(sRow, sCol));
    }
    
   
    tRange* tSheet::MergedRange(tIndex sRow, tIndex sCol) {
        return(m_ColRowCellRange->MergedRange(sRow, sCol));
    }

	// Insert delete Row & Col
    void tSheet::DoInsertRow(tInt sRow, tInt sSize) { m_ColRowCellRange->DoInsertRow(sRow, sSize,nullptr); };

    void tSheet::DoInsertRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) { m_ColRowCellRange->DoInsertRowByRect(sSaveSelectErase,sRect,sRectMoveCells); }

    void tSheet::DoInsertColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) { m_ColRowCellRange->DoInsertColByRect(sSaveSelectErase,sRect,sRectMoveCells); }

    void tSheet::DoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,tBool IsUndoInsert,const tRect* sRectMoveCells) { m_ColRowCellRange->DoDeleteRowByRect(sSaveSelectErase,sRect,IsUndoInsert,sRectMoveCells); }

    void tSheet::DoDeleteColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,tBool IsUndoInsert,const tRect* sRectMoveCells) { m_ColRowCellRange->DoDeleteColByRect(sSaveSelectErase,sRect,IsUndoInsert,sRectMoveCells); }

    void tSheet::UndoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) { m_ColRowCellRange->UndoDeleteRowByRect(sSaveSelectErase,sRect,sRectMoveCells); }

    void tSheet::UndoDeleteColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells) { m_ColRowCellRange->UndoDeleteColByRect(sSaveSelectErase,sRect,sRectMoveCells); }

 	void tSheet::DoDeleteRow(tSaveSelectErase* sSaveSelectErase, tInt sRow, tInt sSize) { m_ColRowCellRange->DoDeleteRow(sSaveSelectErase, sRow, sSize); };


	void tSheet::UndoDeleteRow(tSaveSelectErase* sSaveSelectErase, tInt sRow, tInt sSize) { m_ColRowCellRange->UndoDeleteRow(sSaveSelectErase, sRow, sSize); };

    void tSheet::DoInsertCol(tInt sCol, tInt sSize) { m_ColRowCellRange->DoInsertCol(sCol, sSize,nullptr); };


 	void tSheet::DoDeleteCol(tSaveSelectErase* sSaveSelectErase, tInt sCol, tInt sSize) { m_ColRowCellRange->DoDeleteCol(sSaveSelectErase, sCol, sSize); };

    void tSheet::UndoDeleteCol(tSaveSelectErase* sSaveSelectErase, tInt sRow, tInt sSize) { m_ColRowCellRange->UndoDeleteCol(sSaveSelectErase, sRow, sSize); };

    tBool  tSheet::DoTreeRight(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        return(m_ColRowCellRange->DoTreeRight(sSaveSelectColRow, sPosition, sSize, sIsRow));
    }
    tBool  tSheet::DoTreeLeft(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        return(m_ColRowCellRange->DoTreeLeft(sSaveSelectColRow, sPosition, sSize, sIsRow));
    }

    tBool  tSheet::UndoTree(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow) {
        return(m_ColRowCellRange->UndoTree(sSaveSelectColRow, sPosition, sSize, sIsRow));
    }

	void tSheet::DoDeleteSheet(tSaveSelectErase* sSaveSelectErase) { m_ColRowCellRange->DeleteSheet(sSaveSelectErase);  }

	void tSheet::UndoDeleteSheet(tSaveSelectErase* sSaveSelectErase) { m_ColRowCellRange->UndoDeleteSheet(sSaveSelectErase);  }

	tBool  tSheet::Calculate(tTempoRect* sRect) {
		tContainerPath wContainerPath;
		wContainerPath.Calculate(m_ColRowCellRange,sRect);
		return(true);
	}

	tBool  tSheet::Calculate(const tSelect& sSelect) {
		tContainerPath wContainerPath;
		wContainerPath.Calculate(m_ColRowCellRange, sSelect);
		return(true);
	}


	// Json ===============================================================
	void tSheet::Json(Writer<StringBuffer>* sWriter) {
		sWriter->Key("name"); sWriter->String(m_Name.c_str());
		m_ColRowCellRange->Json(sWriter);
		{
			tPrintParameters wSheetPrint;
			wSheetPrint.m_Orientation = m_Orientation;
			wSheetPrint.m_FitToPage = m_FitToPage;
			sWriter->Key(kJsonKeySheetPrintParameters);
			wSheetPrint.JsonWrite(sWriter, tPrintJsonScope::Sheet);
		}
		const tIndex wSv = SplitV();
		if (wSv != -1) {
			sWriter->Key(kJsonKeySplitV);
			sWriter->Int(wSv);
		}
		const tIndex wSh = SplitH();
		if (wSh != -1) {
			sWriter->Key(kJsonKeySplitH);
			sWriter->Int(wSh);
		}
		if (m_ViewZoomScaleNormal > 0) {
			sWriter->Key(kJsonKeyViewZoom);
			sWriter->Int(m_ViewZoomScaleNormal);
		}
		if (!m_ShowGridLines) {
			sWriter->Key(kJsonKeyShowGridLines);
			sWriter->Bool(false);
		}
	}

	void tSheet::Json(const Value& sValue) {
		m_Name = sValue["name"].GetString();
		m_ColRowCellRange->Json(sValue);
		if (sValue.HasMember(kJsonKeySheetPrintParameters)) {
			const Value& wPrintRoot = sValue[kJsonKeySheetPrintParameters];
			tPrintParameters wParsed;
			if (wParsed.JsonRead(wPrintRoot)) {
				PrintParametersImport(wParsed);
			}
		}
		if (sValue.HasMember(kJsonKeySplitV)) {
			SplitV(static_cast<tIndex>(sValue[kJsonKeySplitV].GetInt()));
		}
		if (sValue.HasMember(kJsonKeySplitH)) {
			SplitH(static_cast<tIndex>(sValue[kJsonKeySplitH].GetInt()));
		}
		if (sValue.HasMember(kJsonKeyViewZoom)) {
			ViewZoomScaleNormal(sValue[kJsonKeyViewZoom].GetInt());
		}
		if (sValue.HasMember(kJsonKeyShowGridLines)) {
			ShowGridLines(sValue[kJsonKeyShowGridLines].GetBool());
		}
	}


	tIndex tSheet::LastRow() {
		return(m_ColRowCellRange->LastRow());
	}

	tIndex tSheet::LastCol() {
		return(m_ColRowCellRange->LastCol());
	}

    tDouble tSheet::SumWidth(tIndex sColStart,tIndex sColEnd,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->SumWidth(sColStart,sColEnd,sUnit));
    }

    tDouble tSheet::SumHeight(tIndex sRowStart,tIndex sRowEnd,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->SumHeight(sRowStart,sRowEnd,sUnit));
    }

    std::tuple<tIndex,tDouble>  tSheet::IndexColByPos(tIndex sColStart,tDouble sPos,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->IndexColByPos(sColStart, sPos,sUnit));
    }

    std::tuple<tIndex,tDouble>  tSheet::IndexRowByPos(tIndex sRowStart,tDouble sPos,tUnitMetrics sUnit) {
        return(m_ColRowCellRange->IndexRowByPos(sRowStart, sPos,sUnit));
    }

    tRect tSheet::MoveCell(tPoint sCellPoint, tByte sKey, tByte sMeta,tRect sScreen) {
        return(m_ColRowCellRange->MoveCell(sCellPoint, sKey,sMeta,sScreen));
    }

    tRect tSheet::MoveToCell(tPoint sCellPoint, tByte sDirection) {
        return(m_ColRowCellRange->MoveToCell(sCellPoint, sDirection));
    }

	// Conditional Format ==================================================
	tConditionalFormat* tSheet::AddConditionalFormat(tConditionalFormatType sType,tString sRef) {
		return(m_ColRowCellRange->AddConditionalFormat(sType,sRef));
	}

    tConditionalFormat* tSheet::ConditionalFormat(tConditionalFormatType sType,tString sRef) {
        return(m_ColRowCellRange->ConditionalFormat(sType,sRef));
    }

    tBool tSheet::RemoveConditionalFormatByKey(tString sKey) {
        return(m_ColRowCellRange->RemoveConditionalFormatByKey(sKey));
    }

    tBool tSheet::RemoveConditionalFormatByRect(tConditionalFormatType sType,tString sRef,tRect* sAreaDelete) {
        return(m_ColRowCellRange->RemoveConditionalFormatByRect(sType,sRef,sAreaDelete));
    }

	// Print Parameters ==================================================
	namespace {
		tBool SheetNameIsInternalPrintHost(const tString& sName)
		{
			return sName.size() >= 3 && sName[0] == '_' && sName[1] == '$' && sName[2] == '$';
		}
	} // namespace

	tPrintParameters tSheet::MergedPrintParameters()
	{
		tPrintParameters wMerged;
		if (tWorkBook* wWorkBook = WorkBook()) {
			wMerged = wWorkBook->PrintParameters();
		}
		wMerged.m_Orientation = m_Orientation;
		wMerged.m_FitToPage = m_FitToPage;
		return wMerged;
	}

	tString tSheet::JsonPrintParameters() { return(MergedPrintParameters().JsonString()); }

	tBool tSheet::JsonPrintParameters(tString sJsonPrintParameters) {
		tPrintParameters wParsed;
		if (!wParsed.JsonParse(sJsonPrintParameters)) {
			return false;
		}
		PrintParametersAssign(wParsed);
		return true;
	}

    void tSheet::PrintParameters(tString sJsonPrintParameters) { (void)JsonPrintParameters(sJsonPrintParameters); }

	void tSheet::PrintParametersAssign(const tPrintParameters& sSnapshot) {
		m_Orientation = sSnapshot.m_Orientation;
		m_FitToPage = sSnapshot.m_FitToPage;
		if (tWorkBook* wWorkBook = WorkBook()) {
			wWorkBook->PrintParameters().CopyWorkBookFieldsFrom(sSnapshot);
		}
	}

	void tSheet::PrintParametersImport(const tPrintParameters& sSnapshot) {
		m_Orientation = sSnapshot.m_Orientation;
		m_FitToPage = sSnapshot.m_FitToPage;
		if (SheetNameIsInternalPrintHost(m_Name)) {
			return;
		}
		if (tWorkBook* wWorkBook = WorkBook()) {
			wWorkBook->AdoptWorkBookPrintFieldsIfDefault(sSnapshot);
		}
	}

	tPrintOrientation tSheet::PrintOrientation() const { return m_Orientation; }
	void tSheet::PrintOrientation(tPrintOrientation sValue) { m_Orientation = sValue; }
	tBool tSheet::FitToPage() const { return m_FitToPage; }
	void tSheet::FitToPage(tBool sValue) { m_FitToPage = sValue; }

	tVectorCell* tSheet::FindCell(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell) {
		return(m_ColRowCellRange->FindCell(sSearch, sMatchCase, sMatchEntireCell));
	}

	tVectorString* tSheet::FindUniqueValue(tRect sRect) {
		return(m_ColRowCellRange->FindUniqueValue(sRect));
	}
	
#ifdef checksp
	/// @brief Check.
	void tSheet::Check() {
        m_ColRowCellRange->Check();
	}
#endif

#ifdef checkfo
    void tSheet::CheckFormat() {
        if (m_Css!=0) {
            WorkBook()->FormatApi()->IncCheck(m_Css);
        }
        m_ColRowCellRange->CheckFormat();
    }
#endif
#ifdef _DEBUGSK
tString tSheet::Debug() {
    tStringStream wStream;
    wStream << "Name " << m_Name << endl;
    wStream << m_ColRowCellRange->Debug();
    return(wStream.str());
}
#endif
	tLong tSheet::NbCell() { return(m_ColRowCellRange->NbCell()); }
	tLongLong tSheet::MemoryColRowSize() { return(m_ColRowCellRange->MemoryColRowSize()); }
	tLongLong tSheet::MemoryCellSize() { return(m_ColRowCellRange->MemoryCellSize()); }
	tLongLong tSheet::MemoryRangeSize() { return(m_ColRowCellRange->MemoryRangeSize()); }
} // End of namespace
