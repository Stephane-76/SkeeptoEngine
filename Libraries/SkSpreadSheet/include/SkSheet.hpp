//=============================================================================
// SkSpreadSheet Sheet
//=============================================================================
#ifndef tSheet_hpp
#define tSheet_hpp
#include <vector>
#include <SkApplication.hpp>
#include "SkCell.hpp"
#include "SkCellClassAttribute.hpp"
#include "SkColRowCellRange.hpp"
#include "SkConditionalFormat.hpp"
#include "SkPrintParameters.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

	class tWorkBook;
	class tSaveSelectErase;
    class tSaveSelectColRow;
    class tSheet;
    typedef std::vector<tSheet*> tVectorSheet;

    //==========================================================================
	//! Sheet
	class tSheet : public SkSpAncestor {
	private:
		//! Owner Workbook
		tAllocatorRef   m_WorkBookRef;
		tAllocatorRef	m_AllocatorRef;
		//! Sheet name
		tString			    m_Name;
		//! Table cell range contains Cells, Ranges, Rows and Cols.
		tColRowCellRange*  m_ColRowCellRange;
		//! Index on Allocator ColRowCellRange system
		tAllocatorRef		m_AllocatorColRowCellRange;
        
        //! For Css    in AllocatorFormat
        tFormatRef          m_Css;

        //! Split pane indices (column / row); -1 means none (same semantics as former tColRowCellRange splitter).
        tPoint m_Splitter;

        //! Page orientation (OOXML pageSetup @orientation). Workbook-shared print layout lives on tWorkBook.
        tPrintOrientation m_Orientation = tPrintOrientation::Portrait;
        //! Fit-to-page vs scale (OOXML pageSetUpPr @fitToPage). Scale / fit page counts live on tWorkBook.
        tBool m_FitToPage = true;

        //! Excel sheetView zoom (0 = default 100%)
        tInt m_ViewZoomScaleNormal = 0;
        //! Excel sheetView showGridLines (default true)
        tBool m_ShowGridLines = true;
	public:
		/// @brief		Constructor tSheet width Workbook owner and name of sheet.
		/// @param[in]	sWorkBook tWorkBook*
		/// @param[in]	sSheetName tString
		tSheet();

		/// @brief		Destructor.
		~tSheet();

		/// @brief		Clear all sheet.
		void Clear();
    
		// @brief  Set Id, Parent and SheetName;
		/// @param[in]	sAllocatorRef tAllocatorRef
		/// @param[in]	sWorkBookRef tAllocatorRef
		/// @param[in]	sWorkBook tWorkBook* 
		/// @param[in]	sWorkBook tString
		void Set(tAllocatorRef sAllocatorRef, tAllocatorRef sWorkBookRef, tString sSheetName);
    
		/// @brief		Return Id of allocator.
		/// @return		tInt
		tAllocatorRef  AllocatorRef();

		/// @brief		Set Id of allocator.
		/// @param[ind]	sId tInt
		void  AllocatorRef(tAllocatorRef sAllocatorRef);

		/// @brief		Return pointer on m_Workbook.
		/// @return		WorkBook*
		tWorkBook* WorkBook();

		/// @brief		Return index m_ColRowCellRange.
		/// @return		tInt
		tAllocatorRef IndexAllocatorColRowCellRange();

		/// @brief		Return pointer of m_ColRowCellRange.
		/// @return		tColRowCellRange*
		tColRowCellRange* ColRowCellRange();

		/// @brief		Return sheet name.
		/// @return		tString
		tString Name();

        /// @brief        Set sheet name.
        /// @param[in]    sName   tString
        void Name(tString sName);

        // Css ============================================================
        /// @brief      Return css
        /// @return     tIFormatRef
        tFormatRef  Css();

        /// @brief     Set Css Value
        /// @param[in] sFormatRef tFormatRef
        void Css(tFormatRef sFormatRef);

        // Split pane (vertical = column index, horizontal = row index)
        /// @brief      Set vertical split at column index.
        /// @param[in]  sValue tIndex
        void SplitV(tIndex sValue);

        /// @brief      Vertical split column index, or -1 if none.
        /// @return     tIndex
        tIndex SplitV() const;

        /// @brief      Set horizontal split at row index.
        /// @param[in]  sValue tIndex
        void SplitH(tIndex sValue);

        /// @brief      Horizontal split row index, or -1 if none.
        /// @return     tIndex
        tIndex SplitH() const;

        /// @brief      Clear split panes.
        void SplitClear();

        /// @brief      Excel sheetView zoomScaleNormal (0 = default 100%).
        void ViewZoomScaleNormal(tInt sZoom);
        tInt ViewZoomScaleNormal() const;

        /// @brief      Excel sheetView showGridLines.
        void ShowGridLines(tBool sShow);
        tBool ShowGridLines() const;

		// Row & Col
		/// @brief		Ensure row for this index.
		/// @param[in]	sIndex tInt	Index row
		/// @return		SkColRow*
		tColRow* EnsureRow(tInt sIndex);

		/// @brief		Return row for this index.
		/// @param[in]	sIndex tInt	Index row
		/// @return		SkColRow*
		tColRow* Row(tInt sIndex);

		/// @brief		Ensure column for this index, nullptr is column dont't exist.
		/// @param[in]	sIndex tInt	Index column
		/// @return		SkColRow*
		tColRow* EnsureCol(tInt sIndex);

		/// @brief		Return column for this index, nullptr is column dont't exist.
		/// @param[in]	sIndex tInt	Index column
		/// @return		SkColRow*
		tColRow* Col(tInt sIndex);

		// Cell
		/// @brief		Ensure cell for this index row and index column.
		/// @param[in]	sRow tInt	Index row
		/// @param[in]	sCol tInt	Index column
		/// @return		tCell*
		tCell* EnsureCell(tInt sRow, tInt sCol);
	
		/// @brief      Erase Cell.
		/// @param[in]  sRow tIndex
		/// @param[in]  sCol tIndex
		/// @return		tBool
		tBool DeleteCell(tIndex sRow, tIndex sCol);

		/// @brief		Return cell for this index row and index column, nullptr is cell dont't exist.
		/// @param[in]	sRow tInt	Index row
		/// @param[in]	sCol tInt	Index column
		/// @return		tCell*
		tCell* Cell(tInt sRow, tInt sCol);
        
        /// @brief    InsertClasse.
        /// @param[in] sCell  tCell*
        /// @return        tBool
        tBool InsertCellClassAttributeContainer(tCell* sCell);

        /// @brief    InsertClasse.
        /// @param[in] sCell  tCell*
        /// @return        tBool
        tBool DeleteCellClassAttributeContainer(tCell* sCell);

		/// @brief return cell attribute.
		/// @param[in] sRow tInt
		/// @param[in] sCol tInt
		/// @param[in] sATtribute tString
		/// @return tCellAttribute* 
		tCellAttribute* CellAttribute(tInt sRow, tInt sCol, tString sAttribute);

		/// @brief return cell attribute create if not exist on class.
		/// @param[in] sRow tInt
		/// @param[in] sCol tInt
		/// @param[in] sATtribute tString
		/// @return tCellAttribute* 
		tCellAttribute* EnsureCellAttribute(tInt sRow, tInt sCol, tString sAttribute);
		
		/// @brief      Delete Cell Attribute.
		/// @param[in] sRow tInt
		/// @param[in] sCol tInt
		/// @param[in] sATtribute tString
		/// @return		Bool
		tBool DeleteCellAttribute(tInt sRow, tInt sCol, tString sAttribute);
				
		/// @brief		Return default size of col millimeter.
		/// @return  	tDouble
		tDouble SizeCol(tInt sCol);

		/// @brief      Set default size of col millimeter..
		/// @param[in]  tDouble sValue
		void SizeCol(tInt sCol, tDouble sValue);

		/// @brief		Return Default size of row millimeter.
		/// @return  	tDouble
		tDouble SizeRow(tInt sRow);

		/// @brief      Set default size of row millimeter..
		/// @param[in]  tDouble sValue
		void SizeRow(tInt sRow, tDouble sValue);

		// Range management
		/// @brief		Return range for this for these parameters, nullptr is cell dont't exist.
		/// @param[in]	sTop tInt	Index row top
		/// @param[in]	sLeft tInt	Index column left
		/// @param[in]	sBottom tInt	Index row bottom
		/// @param[in]	sRight tInt	Index column right
		/// @return		tRange*
		tRange* Range(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight);

		/// @brief		Return range by allocatorRef.
		/// @param[in]	sAllocatorRef tAllocatorRef
		/// @return		tRange*
		tRange* Range(tAllocatorRef sAllocatorRef);

		/// @brief		Ensure range for this for these parameters.
		/// @param[in]	sTop tInt	Index row top
		/// @param[in]	sLeft tInt	Index column left
		/// @param[in]	sBottom tInt	Index row bottom
		/// @param[in]	sRight tInt	Index column right
		/// @return		tRange*
		tRange* EnsureRange(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight);

		/// @brief      EraseRange by AllocatorRef.
		/// @param[in]  sAllocatorRef tAllocatorRef 
		void EraseRange(tAllocatorRef sAllocatorRef);

		/// @brief		Delete range for this for these parameters, return false if range dont't exist.
		/// @param[in]	sTop tInt	Index row top
		/// @param[in]	sLeft tInt	Index column left
		/// @param[in]	sBottom tInt	Index row bottom
		/// @param[in]	sRight tInt	Index column right
		/// @return		tBool	true if range exist else false
		tBool EraseRange(tInt sTop, tInt sLeft, tInt sBottom, tInt sRight);

		/// @brief      Find range recover byCell.
		/// @param[in]  sCell tCell*
		/// @param[out] sResult tVectorRange*
		void FindRangesCovered(tCell* sCell, tColRow::tContainerRange::tResult* sResult);

		/// @brief      Find range recover by tInt value sRow and sCol.
		/// @param[in]  sRow tInt
		/// @param[in]  sCol tInt
		/// @param[out] sResult tVectorRange*
		void FindRangesCovered(tInt sRow, tInt sCol, tColRow::tContainerRange::tResult* sResult);
        
		/// @brief      Find DataRange Covered
		/// @param[in]  sRow tInt
		/// @param[in]  sCol tInt
		/// @return  returntuple<tString,tRange*> 
        tuple<tString,tRange*>  FindRangeDataCovered(tInt sRow, tInt sCol);
        
        /// @brief      Find range recover by tInt value sRow and sCol.
        /// @param[in]  sRect tRect
        /// @param[out] sResult tVectorRange*
        void FindRangesCovered(tRect sRect, tVectorRange* sResult);
        
        // Merged =========================================================
        /// @brief       return Merged Range.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return     tRange*
        tRange* MergedRange(tIndex sRow, tIndex sCol);
        

		// Insert delete Row & Col
		/// @brief		Insert sSize rows at sRow position.
		/// @param[in]	sRow tInt	Index row
		/// @param[in]	sSize tInt	number row to insert
		void DoInsertRow(tInt sRow, tInt sSize);
        
        /// @brief      Insert Row after Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        void DoInsertRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr);
        
        /// @brief      Insert Col after Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        void DoInsertColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr);
        
        /// @brief      Delete  Row In  Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        /// @param[in]  sUndoInsertRow tBoll
        void DoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,tBool sUndoInsertRow,const tRect* sRectMoveCells=nullptr);
        
        /// @brief      Delete  Col In  Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        /// @param[in]  sUndoInsertCol tBoll
        void DoDeleteColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,tBool sUndoInsertCol,const tRect* sRectMoveCells=nullptr);
        
        
        /// @brief      Undo Delete  Row In  Rect
        /// @param[in]  sRect  tRect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        void UndoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr);
        
        /// @brief      Undo Delete  Col In  Rect
        /// @param[in]  sRect  tRect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        void UndoDeleteColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr);
        
        // @brief      Delete Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        void DoDeleteRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect);
        
		/// @brief		Delete sSize rows at sRow position.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
		/// @param[in]	sRow tInt	Index row
		/// @param[in]	sSize tInt	number row to delete
		void DoDeleteRow(tSaveSelectErase* sSaveSelectErase, tInt sRow, tInt sSize);
        
   
		/// @brief		Undo Delete sSize rows at sRow position.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
		/// @param[in]	sRow tInt	Index row
		/// @param[in]	sSize tInt	number row to delete
		void UndoDeleteRow(tSaveSelectErase* sSaveSelectErase, tInt sRow, tInt sSize);

		/// @brief		Insert sSize columns at sCol position.
		/// @param[in]	sCol tInt	Index column
		/// @param[in]	sSize tInt	number column to insert
		void DoInsertCol(tInt sCol, tInt sSize);


		/// @brief		Delete sSize colulns at sCol position.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
		/// @param[in]	sCol tInt	Index column
		/// @param[in]	sSize tInt	number row to delete
		void DoDeleteCol(tSaveSelectErase* sSaveSelectErase, tInt sCol, tInt sSize);
        
		/// @brief		Undo Delete sSize colulns at sCol position.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
		/// @param[in]	sCol tInt	Index column
		/// @param[in]	sSize tInt	number row to delete
		void UndoDeleteCol(tSaveSelectErase* sSaveSelectErase, tInt sCol, tInt sSize);
        
        
        /// @brief      Tree Row Shift Right
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tIndex
        tBool DoTreeRight(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
        
        /// @brief      Tree Row Shift leftt
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tIndex
        tBool DoTreeLeft(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
        
        /// @brief      Undo Tree
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tIndex
        tBool UndoTree(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
    

		/// @brief		Delete Sheet.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
        void DoDeleteSheet(tSaveSelectErase* sSaveSelectErase);

		/// @brief		Undo Delete Sheet.
		/// @param[in]	sSaveSelectErase tSaveSelectErase*
		void UndoDeleteSheet(tSaveSelectErase* sSaveSelectErase);

		/// @brief		Calculate Rect.
		/// @param[in]	sRect SkRect
		/// @return		WorkBook*
		tBool  Calculate(tTempoRect* sRect);

		/// @brief		Calculate Select.
		/// @param[in]	sSelect const tSelect&
		/// @return		tBool
		tBool  Calculate(const tSelect& sSelect);

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const Value& sValue);
        
		/// @brief      Return LastRow.
		/// @return		tIndex
		tIndex LastRow();

		/// @brief      Return LastCol.
		/// @return		tIndex
		tIndex  LastCol();
        
        /// @brief      Return size beetwen ColStart  and sColEnd.
        /// @param[in]  sColStart  tIndex
        /// @param[in]  sColEnd  tIndex
        /// @param[in]  sUnit tUnitMetrics
        /// @return     tDouble
        tDouble SumWidth(tIndex sColStart,tIndex sColEnd,tUnitMetrics sUnit);
        
        
        /// @brief      Return size beetwen RowStart  and sRowEnd.
        /// @param[in]  sRowStart  tIndex
        /// @param[in]  sRowEnd  tIndex
        /// @param[in]  sUnit tUnitMetrics
        /// @return     tDouble
        tDouble SumHeight(tIndex sRowStart, tIndex sRowEnd,tUnitMetrics sUnit);
        
        /// @brief      ReturnIndex col and diff by pixel.
        /// @param[in]  sColStart  tIndex
        /// @param[in]  sPixel  tDouble
        /// @param[in]  sUnit tUnitMetrics
        /// @return     std::tuple<tIndex,tDouble>
        std::tuple<tIndex,tDouble> IndexColByPos(tIndex sColStart,tDouble sPos,tUnitMetrics sUnit);
        
        /// @brief      ReturnIndex row and diff by pixel.
        /// @param[in]  sRowStart  tIndex
        /// @param[in]  sPixel  tDouble
        /// @param[in]  sUnit tUnitMetrics
        /// @return    std::tuple<tIndex,tDouble>
        std::tuple<tIndex,tDouble> IndexRowByPos(tIndex sRowStart,tDouble sPos,tUnitMetrics sUnit);
        
 
        /// @brief      Return Next position of cell
        /// @param[in]  sCellPoint tCell
        /// @param[in]  sKey tByte
        /// @param[in]  sMeta  tByte
        /// @param[in]  sScreen  tRect
        /// @return     tRect
        tRect MoveCell(tPoint sCellPoint, tByte sKey, tByte sMeta, tRect sScreen);
 
        /// @brief      Return Next position not empty of cell
        /// @param[in]  sCellPoint tCell
        /// @param[in]  sDirection tByte
        /// @return     tRect
        tRect MoveToCell(tPoint sCellPoint, tByte sDirection);

		// Conditional Format ==================================================
		/// @brief     Add  Conditional Format on Container
        /// @param[in]  sType tyConditionalFormatType
        /// @param[in]  sRef tString
		/// @return     tConditionalFormatContainer*
		tConditionalFormat* AddConditionalFormat(tConditionalFormatType sType,tString sRef);
        
        // Conditional Format ==================================================
        /// @brief     Add  Conditional Format on Container
        /// @param[in]  sType tyConditionalFormatType
        /// @param[in]  sRef tString
        /// @return     tConditionalFormatContainer*
        tConditionalFormat* ConditionalFormat(tConditionalFormatType sType,tString sRef);
		

        /// @brief     Delete  Conditional Format on Container
        /// @param[in]  sKey ttString
        /// @param[in]  sAreaDelete tRect*
        /// @return     tBool
        tBool RemoveConditionalFormatByKey(tString sKey);

        /// @brief     Delete  Conditional Format on Container
        /// @param[in]  sType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @param[in]  sAreaDelete tRect*
        /// @return     tBool
        tBool RemoveConditionalFormatByRect(tConditionalFormatType sType,tString sRef,tRect* sAreaDelete);
        

		// Print Parameters ==================================================
		/// @brief     Merged print snapshot: workbook-shared layout + this sheet's orientation / FitToPage.
		tPrintParameters MergedPrintParameters();

		/// @brief     Return print parameters in Json String (merged snapshot, same keys as before).
		/// @return     tPrintParameters
		tString JsonPrintParameters();

		/// @brief     Same as applying JSON to @ref tPrintParameters but returns whether parsing succeeded.
		tBool JsonPrintParameters(tString sJsonPrintParameters);
  
        /// @brief     Set print parameters from Json String
		/// @param[in]  sPrintParameters tPrintParameters
		void PrintParameters(tString sJsonPrintParameters);

		/// @brief     Apply a merged snapshot: sheet keeps orientation / FitToPage; other fields go to the workbook.
		/// @param[in]  sSnapshot source layout
		void PrintParametersAssign(const tPrintParameters& sSnapshot);

		/// @brief     Load/import: set sheet orientation / FitToPage; copy workbook fields only if still default.
		void PrintParametersImport(const tPrintParameters& sSnapshot);

		tPrintOrientation PrintOrientation() const;
		void PrintOrientation(tPrintOrientation sValue);
		tBool FitToPage() const;
		void FitToPage(tBool sValue);

		   /// @brief     Find cell by search string
        /// @param[in]  sSearch tString
        /// @param[in]  sMatchCase when true, search is case-sensitive
        /// @param[in]  sMatchEntireCell when true, cell display text must equal sSearch
        /// @return     tVectorCell*
        tVectorCell* FindCell(tString sSearch, tBool sMatchCase = false, tBool sMatchEntireCell = false);

        /// @brief     Find unique value
        /// @param[in]  sSearch tString
        /// @return     tVectorString*
        tVectorString* FindUniqueValue(tRect sRect);

#ifdef checksp
		/// @brief Check.
		void Check();
#endif
#ifdef checkfo
        /// @brief Check format SpreafSheetContainer/WorkBook-/
        void CheckFormat();
#endif
#ifdef _DEBUGSK
        /// @brief        Debug.
        tString Debug();
#endif
		/// @brief		Return number of cells in sheet.
		/// @return		tLong
		tLong NbCell();

		/// @brief		Return memory taken by cols rows.
		/// @return		tLong
		tLongLong MemoryColRowSize();

		/// @brief		Return memory taken by cells.
		/// @return		tLong
		tLongLong MemoryCellSize();

		/// @brief		Return memory taken by ranges.
		/// @return		tLong
		tLongLong MemoryRangeSize();
	};

	

} // End of namespace
#endif
