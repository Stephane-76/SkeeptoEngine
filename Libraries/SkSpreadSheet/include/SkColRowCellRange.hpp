//=============================================================================
// SkColRowCellRange (Container spreadsheet table)
//=============================================================================
#ifndef SkColRowCellRange_hpp
#define SkColRowCellRange_hpp

#include "SkTools.hpp"
#include "SkColRow.hpp"
#include "SkCell.hpp"

#include "SkCellClass.hpp"
#include "SkCellAttribute.hpp"
#include "SkCellClassContainer.hpp"

#include "SkCellClassUnit.hpp"

#include "SkRange.hpp"
#include "SkUndoRedoSaveSp.hpp"
#include "SkSheet.hpp"
#include "SkConditionalFormat.hpp"
// STL
#include <functional>
#include <vector>
#include <algorithm>


using namespace SkRoot;

namespace SkSpreadSheet {

    enum  tKey : tByte { t_Left=1, t_Up, t_Right, t_Down, t_PageUp=10, t_PageDown, t_Home=20, t_End };

    class tSheet;
    class tSaveSelectErase;
    class tSaveSelectColRow;

    class tContainerPath;
   

   
    //=========================================================================
    //! Call back for find cell by search string
    class tCallBackFindCell : public tSparseArrayCallBack<tAllocatorRef> {
        private:
        ///! ColRowCellRange
        tColRowCellRange*  m_ColRowCellRange;
        //! Workbook
        tWorkBook*         m_Workbook;
        ///! Search string
        tString    m_Search;
        ///! Match case (Excel: Respecter la casse)
        tBool      m_MatchCase;
        ///! Match entire cell contents (Excel: Cellule entière)
        tBool      m_MatchEntireCell;
        ///! Vector of allocator references
        tVectorAllocatorRef m_Vector;
    public:
        ///! Constructor
        tCallBackFindCell(tColRowCellRange* sColRowCellRange, tString sSearch,
                          tBool sMatchCase = false, tBool sMatchEntireCell = false);
        ///! Call method
        tBool CallBack(tAllocatorRef sAllocatorRef) override;

        //! Return vector of allocator references
        tVectorCell*  Vector() const;
    };

    //=========================================================================
    // Find Unique value
    class tCallBackFindUniqueValue : public tSparseArrayCallBack<tAllocatorRef> {
        private:
            ///! ColRowCellRange
            tColRowCellRange* m_ColRowCellRange;
            //! Workbook
            tWorkBook*         m_Workbook;
            ///! Unique formatted cell values (hash set)
            tUnorderedContainer<tString> m_UniqueValues;
        public:
            /// @brief		Constructor SkCallBackRangeFunction with owner tColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            tCallBackFindUniqueValue(tColRowCellRange* sColRowCellRange);
    
            /// @brief		Call method for find unique value if return false stop process.
            /// @param[in]	sAllocatorRef tAllocatorRef Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override;

            //! Return vector of unique values
            tVectorString*  Vector() const;
    };

    //=========================================================================
    //! Manager of all Cells, Ranges, Rows and cols for one sheet
    class alignas(SkAlign) tColRowCellRange : public tClass {
        public:
            // Allocator ======================================================
            typedef tAllocator<tColRow, tAllocatorRef, 512> tAllocatorColRow;
            typedef tAllocator<tCell, tAllocatorRef, 1024> tAllocatorCell;
            typedef tAllocator<tRange, tAllocatorRef,1024> tAllocatorRange;

            typedef tAllocator<tCellAttribute, tAllocatorRef, 1024> tAllocatorCellAttribute;
            typedef tAllocator<tCellExtend, tAllocatorRef, 1024> tAllocatorCellExtend;

            typedef tAllocator<tItemCF, tAllocatorRef, 128> tAllocatorItemCF;

            //! Sparse Array for SkColRow
            typedef tSparseArray<tAllocatorRef> tSparseArrayColRow;
            // Cell Container 2D ==============================================
            // Col tAllocatorRef for SkAllocator 
            typedef tSparseArray<tAllocatorRef> tSparseArrayCell;
            // By Row
            typedef tSparseArrayPt<tSparseArrayCell*> tSparseArray2DCell;
    private:
        //! Allocator for row (SkColRow).
        tAllocatorColRow		 m_AllocatorRow;

        //! Allocator for Col (SkColRow).
        tAllocatorColRow		 m_AllocatorCol;

        //! Allocator for row (tCell).
        tAllocatorCell          m_AllocatorCell;
        
        //! Allocator for row (tRange).
        tAllocatorRange		    m_AllocatorRange;

        //! Allocator for Cell (tCell).
        tAllocatorCellAttribute  m_AllocatorCellAttribute;
        
        //! Extend Cell
        tAllocatorCellExtend     m_AllocatorCellExtend;

        //! Sparse array of index on m_AllocatorRow.
        tSparseArrayColRow       m_Rows;
        //! Sparse array of index on m_AllocatorCols.
        tSparseArrayColRow		 m_Cols;

        //! Sparse array of Sparse array of tIndex, index on m_AllocatorCell
        tSparseArray2DCell		m_Cell2D;

        // Temporary ItemCF
        tAllocatorItemCF        m_AllocatorItemCF;
        
        //! Container of Cell Class
        tCellClassContainer     m_CellClassContainer;
        
        ///! Owner sheet
        tSheet*					m_Sheet;
        /// @brief              Return index on StaticColRowCellRange.
        /// @return		        tIndex
        tAllocatorRef		    m_SheetAllocator;

        ///! Clear in progress
        ///! no test
        tBool                   m_ClearInProgress;
        
        // Volatile cells keyed by allocator ref (not raw tCell*): after Delete the slot
        // resolves to nullptr instead of a dangling pointer (WASM "null function" on Formula()).
        typedef std::vector<tAllocatorRef> tSetVolatileCellRef;
        tSetVolatileCellRef    m_VectorVolatileCell;
        
        //! For Conditional Format
        tConditionalFormatContainer* m_ConditionalFormatContainer;

        //! Cached sheet extent (-1 = not computed); warmed on first LastRow/LastCol during a calc pass.
        tIndex m_CachedLastRow;
        tIndex m_CachedLastCol;

        void ComputeExtentUncached(tIndex& oLastRow, tIndex& oLastCol);
        void RefreshExtentCache();
    public:
        /// @brief      Constructor tColRowCellRange.
        tColRowCellRange();

        /// @brief      Clear tColRowCellRange (Just for Call back SkAllocator).
        void Clear();

        /// @brief      Clear tColRowCellRange (not used today).
        void clear();
        
        /// @brief		Sheet management SetSheet.
        /// @param[in]	sSheet tSheet*
        void Sheet(tSheet* sSheet);

        /// @brief		Retur sheet.
        /// @return		tSheet*
        tSheet* Sheet() const;

        /// @brief		Set Indice Allocator.
        /// @param[in]	sIndiceAllocator tInt
        void SheetAllocator(tIndex sIndiceAllocator);
        /// @brief		Retur IndiceAllocator.
        /// @return		ttAllocatorRef
        tAllocatorRef SheetAllocator();

        /// @brief      Return CellClassContainer.
        /// @return     tCellClassContainer*
        tCellClassContainer*  CellClassContainer();
        
        /// @brief    InsertClasse.
        /// @param[in] sCell  tCell*
        /// @return        tBool
        tBool InsertCellClassAttributeContainer(tCell* sCell);

        /// @brief    InsertClasse.
        /// @param[in] sCell  tCell*
        /// @return        tBool
        tBool EraseCellClassAttributeContainer(tCell* sCell);
    
        // Validation
        /// @brief      Test if index of row is valid (sIndex >= 0) && (sIndex < MaxRows).
        /// @param[in] sRow tIndex
        /// @return		tBool
        tBool ValidRow(tIndex sRow);

        /// @brief      Test if index of col is valid (sIndex >= 0) && (sIndex < MaxCols).
        /// @return		tBool 
        tBool ValidCol(tIndex sCol);

        /// @brief      Test if Range index is valid valid all corner && (sTop <= sBottom) && (sLeft <= sRight).
        /// @param[in]  sTop tIndex
        /// @param[in]  sLeft tIndex
        /// @param[in]  sBottom tIndex
        /// @param[in]  sRight tIndex
        /// @return		tBool 
        tBool ValidRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

        // ColRow management
        /// @brief      Return the row, if it does not exist it is created.
        /// @param[in]  sIndex tIndex
        /// @return		SkColRow* 
        tColRow* EnsureRow(tIndex sIndex);

        /// @brief      Delete Row.
        /// @param[in]  sIndex tIndex
        void  DeleteRow(tIndex sIndex);
        
        
        /// @brief      Return the AllocatorRef row by indice
        /// @param[in]  sIndex tIndex
        /// @return     tAllocatorRef
        tAllocatorRef RowAllocatorRef(tIndex sIndex);

        /// @brief      Return the AllocatorRef row by indice
        /// @param[in]  sIndex tIndex
        /// @return     tAllocatorRef
        tAllocatorRef ColAllocatorRef(tIndex sIndex);


        /// @brief		Return the row, if it does exist return nullptr
        /// @param[in]  sIndex tIndex
        /// @return		SkColRow*
        tColRow* Row(tIndex sIndex);

        /// @brief      Return the col, if it does not exist it is created.
        /// @param[in]  sIndex tIndex
        /// @return		SkColRow*
        tColRow* EnsureCol(tIndex sIndex);

        /// @brief      Delete Col.
        /// @param[in]  sIndex tIndex
        void  DeleteCol(tIndex sIndex);
        
        /// @brief		Return the col, if it does exist return nullptr
        /// @param[in]  sIndex tIndex
        /// @return		SkColRow*
        tColRow* Col(tIndex sIndex);
        
        // Cell attribute management ==========================================
        /// @brief      Alloc Cell Attribute.
        /// @return		tAllocatorRef
        tAllocatorRef AllocCellAttribute();

        /// @brief      Return Cell Attribute.
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return		tCellAttribute*
        tCellAttribute* CellAttribute(tAllocatorRef sAllocatorRef);
        
        /// @brief      Alloc Cell Extend.      
        /// @return     tAllocatorRef
        tAllocatorRef AllocCellExtend();

        /// @brief      Return Cell Extend.
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return     tCellExtend*
        tCellExtend* CellExtend(tAllocatorRef sAllocatorRef);

        /// @brief      Delete Cell Extend.
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return     tBool
        tBool DeleteCellExtend(tAllocatorRef sAllocatorRef);

        /// @brief return cell attribute.
        /// @param[in] sRow tInt
        /// @param[in] sCol tInt
        /// @param[in] sATtribute tString
        /// @return tCellAttribute* 
        tCellAttribute* CellAttribute(tInt sRow, tInt sCol,tString sAttribute);

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

        /// @brief      Delete Cell Attribute.
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return		Bool
        tBool DeleteCellAttribute(tAllocatorRef sAllocatorRef);

        // Cell management ====================================================
        /// @brief      Returns the cell, if it does not exist it is created.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return		tCell*
        tCell* EnsureCell(tIndex sRow, tIndex sCol);

        /// @brief      Returns the cell, if it does not exist it is created.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[in]  sClassName tString
        /// @return        tCell*
        tCell* EnsureCellClass(tIndex sRow, tIndex sCol,tString sClassName);

        /// @brief      Erase Cell.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return		tBool
        tBool DeleteCell(tIndex sRow, tIndex sCol);

        /// @brief      Returns the cell, if it does not exist return nullptr.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return		tCell*
        tCell* Cell(tIndex sRow, tIndex sCol);
        
        
        /// @brief return Is Cell is delete
        /// @param[in] sAllocatorRef  tAllocatorRef
        /// @return     tBool
        tBool IsDeletedCell(tAllocatorRef sAllocatorRef);
        

        /// @brief      Returns the cell, if it does not exist return nullptr.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return		tAllocatorRef
        tAllocatorRef CellAllocatorRef(tIndex sRow, tIndex sCol);

        /// @brief      Set AllocatorRef in cell for insert range sort...
        /// @param[in] sAllocatorRef  tAllocatorRef
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        void SetCellAllocatorRef(tAllocatorRef sAllocatorRef,tIndex sRow, tIndex sCol);
        
        /// @brief     Return the cell by allocator (m_Cell2D).
        /// @param[in] sAllocatorRef  tAllocatorRef
        /// @return		tCell*
        tCell* Cell(tAllocatorRef sAllocatorRef);

        /// @brief		Erase cell by allocator (m_Cell2D).
        /// @param[in]  sAllocatorRef tAllocatorRef 
        void DeleteCell(tAllocatorRef sAllocatorRef);
 
        
        // Volatile ===========================================================
        /// @brief      AddVolatile Cell/// @param[in]  sAllocator tAllocatorR
        /// @param[in]  sCell tCell*
        tBool AddVolatile(tCell* sCell);

        /// @brief      AddVolatile Cell
        /// @param[in]  sCell tCell*
        tBool DeleteVolatile(tCell* sCell);
        
        /// @brief      Add cell Volatile in path
        /// @param[in]  sContainerPath tContainerPath*
        /// @param[in]  sVolatile tVolatile
        void AddPathVolatile(tContainerPath*  sContainerPath,tVolatile sVolatile);
            
#ifdef checksp
        /// @brief      IsInVolatileCells return true if cell is in volatile cells.
        /// @param[in]  sCell tCell*
        /// @return     tBool
        tBool IsInVolatileCells(tCell* sCell);
#endif
        
        // Range management
        /// @brief      FindRange return nullptr if don't exist.
        /// @param[in]  sTop tIndex
        /// @param[in]  sLeft tIndex
        /// @param[in]  sBottom tIndex
        /// @param[in]  sRight tIndex
        /// @return		tRange* 
        tRange* Range(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

        /// @brief      EnsureRange create range if don't exist.
        /// @param[in]  sTop tIndex
        /// @param[in]  sLeft tIndex
        /// @param[in]  sBottom tIndex
        /// @param[in]  sRight tIndex
        /// @return		tRange* 
        tRange* EnsureRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);
        
        /// @brief      Dynamic range right
        /// @param[in]  sCellLeft tCell*
        /// @param[in]  sCellRight tCell*
        /// @return     tBool
        tRange* EnsureRange(tCell* sCellLeft, tCell* sCellRight);

        /// @brief      EraseRange by AllocatorRef.
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @param[in]  sClean tBool Don't test il attach to col row (After bad compil);
        void DeleteRangeByAllocatorRef(tAllocatorRef sAllocatorRef, tBool sClean=false);
        
        /// @brief      EraseRange return false if don't exist.
        /// @param[in]  sTop tIndex
        /// @param[in]  sLeft tIndex
        /// @param[in]  sBottom tIndex
        /// @param[in]  sRight tIndex
        /// @return		tBool 
        tBool DeleteRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

        /// @brief      Set and Add Range in SkColRow Container.
        /// @param[in]  sRange tRange* 
        /// @param[in]  sTop tIndex
        /// @param[in]  sLeft tIndex
        /// @param[in]  sBottom tIndex
        /// @param[in]  sRight tIndex
        void SetRange(tRange* sRange, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

    
        /// @brief      Delete Range in SkColRow Container.
        /// @param[in]  sRange tRange*
        /// @param[in]  sEraseRange tBool
        /// @param[in]  sClean tBool Don't test il attach to col row (After bad compil);
        void DetachRangeFromColRow(tRange* sRange,tBool sEraseColRow,tBool sClean=false);
        
        /// @brief      Add  Range in SkColRow Container.
        /// @param[in]  sRange tRange*
        void AttachRangeToColRow(tRange* sRange);

        /// @brief      Attach full-column / full-sheet ranges ($C:$C) onto a newly created row.
        void AttachFullColumnRangesToNewRow(tColRow* sRow, tIndex sRowIndex);

        /// @brief      Attach full-row / full-sheet ranges ($1:$1) onto a newly created column.
        void AttachFullRowRangesToNewCol(tColRow* sCol, tIndex sColIndex);
        
        /// @brief      Find ranges in SkColRow Container by Rect.
        /// @param[in]  sRect tRect
        /// @param[out] sResult tVectorRange*
        void FindRanges(tRect sRect, tVectorRange* sResult);
        
        /// @brief      Find range Merged byCell.
        /// @param[in]  sCell tCell*
        /// @param[out] sResult tClassUnorderedContainer<tRange>::tVectorResult*
        void FindRanges(tCell* sCell, tColRow::tContainerRange::tResult* sResult);
        
        /// @brief      Find range recover by tIndex value sRow and sCol.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[out] sResult tClassUnorderedContainer<tRange>::tVectorResult*
        void FindRangesCovered(tIndex sRow, tIndex sCol, tColRow::tContainerRange::tResult* sResult);
        
        	/// @brief      Find DataRange covered by (sRow,sCol), including the totals row when it sits
		///             immediately below the stored header+data IsData rect and RangeData::HasTotals().
		/// @param[in]  sRow tInt
		/// @param[in]  sCol tInt
		/// @return  str::tuple<tString,tRange*>
        std::tuple<tString,tRange*> FindRangeDataCovered(tInt sRow, tInt sCol);
  
        
        /// @brief      Find range recover by tIndex value sRow and sCol.
        /// @param[in]  sRect tRect
        void FindRangesCovered(tRect sRect, tVectorRange* sResult);
        
        // Merged =========================================================
        /// @brief       return Merged Range.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return     tRange*
        tRange* MergedRange(tIndex sRow, tIndex sCol);
        
        /// @brief		Return the Range by allocator for check.
        /// @param[in]  sIndexAllocator tIndex
        /// @return		tRange*
        tRange* Range(tAllocatorRef sIndexAllocator);

        /// @brief      IsDeleted
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return        tBool
        tBool IsDeletedRange(tAllocatorRef sAllocatorRef);
    

        /// @brief  Renum row (SkColRow)
        /// @param[in]  sRow tIndex
        void RenumRow(tIndex sRow);

        /// @brief  Renum row (SkColRow)
        /// @param[in]  sCol tIndex
        void RenumCol(tIndex sCol);

        /// @brief Fill missing ParentRef from m_Children so Renum does not wipe the tree.
        /// @param[in]  sIsRow tBool
        void SyncTreeParentRefsFromChildren(tBool sIsRow);
        

        /// @brief Return Rect For DeleteRow
        /// @param[in] sRow tIndex
        /// @param[in] sSize tIndex
        /// @return tRect
        tRect GetRectDeleteAreaRow(tIndex sRow,tIndex sSize);

        /// @brief Return Rect For DeleteCol
        /// @param[in] sRow tIndex
        /// @param[in] sSize tIndex
        /// @return tRect
        tRect GetRectDeleteAreaCol(tIndex sCol,tIndex sSize);
     
        /// @brief      Allocate ItemCF
        /// @return     tAllocatorRef
        tAllocatorRef AllocItemCF();

        /// @brief      Get ItemCF
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return     tItemCF*
        tItemCF* ItemCF(tAllocatorRef sAllocatorRef);

        /// @brief      Delete ItemCF
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return     void
        void DeleteItemCF(tAllocatorRef sAllocatorRef);

        // Raz Rect ===========================================================
        /// @brief      DecNbCells Erase Row or Col if empty 
        /// @param[in]  sIndex tIndex
        /// @param[in]  sIsRow tBool
        /// @return     tBool
        void DecNbCells(tColRow* sColRow,tBool sIsRow);
      
        // Insert delete Row & Col
        /// @brief      Insert sSize row(s) at position sRow.
        /// @param[in]  sRow tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        void DoInsertRow(tIndex sRow, tIndex sSize,tSaveSelectErase* sSaveSelectErase);
        
        /// @brief      Insert Row after Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        void DoInsertRowByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr,tBool sPreserveSpanningRanges=false);
        
        /// @brief      Insert Col after Rect
        /// @param[in]  sSaveSelectErase  tSaveSelectErase*
        /// @param[in]  sRect  tRect
        void DoInsertColByRect(tSaveSelectErase* sSaveSelectErase,tRect sRect,const tRect* sRectMoveCells=nullptr,tBool sPreserveSpanningRanges=false);
        
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

        /// @brief Move live cells from sSource to sDest on the 2D grid (same size; overlap supported).
        ///        Detaches from m_Cell2D without destroying moved tCell objects.
        /// @return tBool false if geometry differs (not a pure translate)
        tBool RelocateRect(tRect& sSource, tRect& sDest);
        
        
        /// @brief      Delete sSize row(s) at position sRow.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        /// @param[in]  sRow tIndex
        /// @param[in]  sSize tIndex
        void DoDeleteRow(tSaveSelectErase* sSaveSelectErase, tIndex sRow, tIndex sSize);

        
        /// @brief      Delete sSize row(s) at position sRow.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        /// @param[in]  sRow tIndex
        /// @param[in]  sSize tIndex
        void UndoDeleteRow(tSaveSelectErase* sSaveSelectErase, tIndex sRow, tIndex sSize);

        /// @brief      Insert sSize col(s) at position sCol.
        /// @param[in]  sCol tIndex
        /// @param[in]  sSize tIndex
        void DoInsertCol(tIndex sCol, tIndex sSize,tSaveSelectErase* sSaveSelectErase);

        
        /// @brief      Earse sSize col(s) at position sCol.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        /// @param[in]  sCol tIndex
        /// @param[in]  sSize tIndex
        void DoDeleteCol(tSaveSelectErase* sSaveSelectErase, tIndex sCol, tIndex sSize);

        /// @brief      Earse sSize col(s) at position sCol.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        /// @param[in]  sCol tIndex
        /// @param[in]  sSize tIndex
        void UndoDeleteCol(tSaveSelectErase* sSaveSelectErase, tIndex sCol, tIndex sSize);
        
        
        ///=====================================================================
        /// @brief      Get Col or Row
        /// @param[in]  sIndex tIndex
        /// @param[in]  sIsRow tBool
        /// @return     tBool
        tColRow* ColRowByIndex(tIndex sIndex,tBool sIsRow);
        
        
        /// @brief      Get Col or Row
        /// @param[in]  sAllocator  ttAllocatorRef
        /// @param[in]  sIsRow tBool
        /// @return     tColRow* pointer to ColRow
        tColRow* ColRowByAllocatorRef(tAllocatorRef sAllocator,tBool sIsRow);
        
        /// @brief      Get Col or Row (const version)
        /// @param[in]  sAllocator  ttAllocatorRef
        /// @param[in]  sIsRow tBool
        /// @return     const tColRow* const pointer to ColRow
        const tColRow* ColRowByAllocatorRef(tAllocatorRef sAllocator,tBool sIsRow) const;
        
        /// @brief      Ensure  Col or Row
        /// @param[in]  sIndex tIndex
        /// @param[in]  sIsRow tBool
        /// @return     tBool
        tColRow* EnsureColRow(tIndex sIndex,tBool sIsRow);

        
        /// @brief      Return grand parent
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sIsRow tBool
        /// @return tIndex
        tIndex GrandParent(tIndex sPosition,tBool sIsRow);
        
        /// @brief      Tree Row Shift Right
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tBool
        tBool DoTreeRight(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
        
        /// @brief      Tree Row Shift leftt
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tBool
        tBool DoTreeLeft(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
        
        
       
        /// @brief      Undo Tree
        /// @param[in]  sSaveSelectColRow  tSaveSelectColRow*
        /// @param[in]  sPosition  tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRow tBool
        /// @return tIndex
        tBool UndoTree(tSaveSelectColRow* sSaveSelectColRow, tIndex sPosition, tIndex sSize,tBool sIsRow);
    

        // Sheet ==============================================================
        /// @brief		Delete Sheet.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        void DeleteSheet(tSaveSelectErase* sSaveSelectErase);

        /// @brief		Undo Delete Sheet.
        /// @param[in]	sSaveSelectErase tSaveSelectErase*
        void UndoDeleteSheet(tSaveSelectErase* sSaveSelectErase);

        // Visitor Range ======================================================
        /// @brief      Visitor Cell by range (call back for function sum, min, max ..).
        /// @param[in]  sRange tRange* 
        /// @param[in]  SkCallBackRange SkSparseArrayCallBack<tAllocatorRef>* 
        void VisitorRange(tRange* sRange, tSparseArrayCallBack<tAllocatorRef>* SkCallBackRange);

        /// @brief      Call back all cell.
        /// @param[in]  sCallBackRange SkSparseArrayCallBack<tAllocatorRef>*
        void CallBackAllCell(tSparseArrayCallBack<tAllocatorRef>* sCallBackRange);

        /// @brief      Call back cells in a row/col index range (sparse grid indices).
        void CallBackAllCellInRange(
            tSparseArrayCallBack<tAllocatorRef>* sCallBackRange,
            tAllocatorRef sTop,
            tAllocatorRef sBottom,
            tAllocatorRef sLeft,
            tAllocatorRef sRight);

        /// @brief      Call back all cells in the physical sparse grid (integrity / debug).
        void CallBackAllCellPhysical(tSparseArrayCallBack<tAllocatorRef>* sCallBackRange);

        /// @brief      Invoke visitor for each live range on this sheet.
        void CallBackAllRanges(const std::function<void(tRange*)>& sVisitor);

        /// @brief Delete Call back allocator.
        void Delete();

        // Json ===============================================================
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonMerged(Writer<StringBuffer>* sWriter);
        

        /// @brief		Writer Json.
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        void Json(const Value& sValue);
        
        /// brief SizeRow
        /// @param[in]    sRow tIndex
        /// @param[ind]     sUnit Unit  tUnit
        tDouble SizeRow(tIndex sRow,tUnitMetrics sUnit);

        /// brief SizeCol
        /// @param[in]    sCol  tIndex
        /// @param[ind]     sUnit Unit  tUnit
        tDouble SizeCol(tIndex sCol,tUnitMetrics sUnit);

        /// @brief      Return LastRow.
        /// @return		tIndex
        tIndex LastRow();

        /// @brief      Return LastCol.
        /// @return		tIndex
        tIndex LastCol();

        /// @brief      Drop cached LastRow/LastCol (next access rescans).
        void InvalidateExtentCache();

        /// @brief      After matrix/array spill: rescan extent if spill exceeds cached bounds.
        void NotifySpillExtent(tIndex sBottom, tIndex sRight);

        /// @brief Last row index to emit in Json (content + in-extent explicit row sizes).
        tIndex LastRowForJson();

        /// @brief Last col index to emit in Json (content + in-extent explicit col sizes).
        tIndex LastColForJson();
        
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
        /// @param[in]  sPos  tDouble
        /// @param[in]  sUnit tUnitMetrics
        /// @return     std::tuple<tIndex,tDouble>
        std::tuple<tIndex,tDouble> IndexColByPos(tIndex sColStart,tDouble sPos,tUnitMetrics sUnit);
        
        /// @brief      ReturnIndex row and diff by pixel.
        /// @param[in]  sRowStart  tIndex
        /// @param[in]  sPos  tDouble
        /// @param[in]  sUnit tUnitMetrics
        /// @return    std::tuple<tIndex,tDouble>
        std::tuple<tIndex,tDouble> IndexRowByPos(tIndex sRowStart,tDouble sPos,tUnitMetrics sUnit);
        
        /// @brief      Return Cell empty (nullptr) or empty
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol  tIndex
        /// @return     tBool
        tBool CellValueEmpty(tIndex sRow,tIndex sCol);
        
        /// @brief      Return Row Visible
        /// @param[in]  sRow  tIndex
        /// @return     tBool
        tBool IsRowVisible(tIndex sRow);

        /// @brief      Return Col Visible
        /// @param[in]  sCol  tIndex
        /// @return     tBool
        tBool IsColVisible(tIndex sCol);
        
        /// @brief      Return Next position of cel
        /// @param[in]  sCellPoint tCell
        /// @param[in]  sKey  tByte
        /// @param[in]  sMeta  tByte
        /// @param[in]  sScreen  tRect
        /// @return     tRect
        tRect MoveCell(tPoint sCellPoint, tByte sKey, tByte sMeta, tRect sScreen);
        
        /// @brief      Return Next position of cel
        /// @param[in]  sCellPoint tCell
        /// @param[in]  sDirection tByte
        /// @return     tRect
        tRect MoveToCell(tPoint sCellPoint, tByte sDirection);

            // JSON Payload Interface ============================================
        /// @brief      Set JSON payload for a cell by row/col
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[in]  sJson tString (minified JSON recommended)
        /// @return     tBool true if successful
        tBool SetCellJsonPayload(tIndex sRow, tIndex sCol, const tString& sJson);

        /// @brief      Get JSON payload for a cell by row/col
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[out] sOutJson tString&
        /// @return     tBool true if found
        tBool GetCellJsonPayload(tIndex sRow, tIndex sCol, tString& sOutJson);

        /// @brief      Delete JSON payload for a cell by row/col
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return     tBool true if existed and removed
        tBool DeleteCellJsonPayload(tIndex sRow, tIndex sCol);

        // Conditional Format ==================================================
        /// @brief     Add  Conditional Format on Container
        /// @brief     sType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @return     tConditionalFormatContainer*
        tConditionalFormat* AddConditionalFormat(tConditionalFormatType sType,tString sRef);
        
        // Conditional Format ==================================================
        /// @brief     Add  Conditional Format on Container
        /// @brief     sType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @return     tConditionalFormatContainer*
        tConditionalFormat* ConditionalFormat(tConditionalFormatType sType,tString sRef);
        
        /// @brief Get conditional format by Range
        /// @brief     sType tConditionalFormatType
        /// @param[in]  sRange tRange*
        /// @return    tRangeConditionnalFormat*
        tRangeConditionnalFormat* ConditionalFormatByRange(tRange* sRange);
        
    
        /// @brief     Delete  Conditional Format on Container
        /// @param[in]  sKey ttString
        /// @param[in]  sAreaDelete tRect*
        /// @return     tBool
        tBool RemoveConditionalFormatByKey(tString sKey);

        /// @brief     Delete  Conditional Format on Container
        /// @brief     sType tConditionalFormatType
        /// @param[in]  sRange tRange*
        /// @param[in]  sKey ttString
        /// @param[in]  sAreaDelete tRect*
        /// @return     tBool
        tBool RemoveConditionalFormatByRect(tConditionalFormatType sType,tString sRect,tRect* sAreaDelete);

        /// @brief Remove CF extension flags on ranges not registered in the container (and drop empty geometry).
        void StripOrphanConditionalFormatExtension();

        /// @brief CF geometry must not carry merge flags (covered-range undo can set both).
        void StripMergedOnConditionalFormatRanges();
        
         /// @brief     Return Conditional Format Container
        /// @return     tConditionalFormatContainer*
        tConditionalFormatContainer* ConditionalFormatContainer();
        
        /// @Brief ReturnClear in progress
        tBool  ClearInProgress();

        /// @brief Release row/col/cell/CF css refs before bulk allocator clear (workbook-scoped pool).
        void ReleaseAllCellFormats();

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
        /// @brief Check format
        void CheckFormat();
#endif
#ifdef _DEBUGSK
        /// @brief        Debug.
        tString Debug();
#endif

#ifdef  _DEBUGSK
        void DebugCell(tString sTitle,tString sRef);
#endif

        // Statistics...
        /// @brief		Returns the number of cells in tColRowCellRange.
        /// @return		tLongLong
        tLong NbCell();

        /// @brief		Return the memory taken by the ColRow.
        /// @return		tLongLong
        tLongLong MemoryColRowSize();

        /// @brief		Return the memory taken by the cells.
        /// @return		tLongLong
        tLongLong MemoryCellSize();

        /// @brief		Return the memory taken by the ranges.
        /// @return		tLongLong
        tLongLong MemoryRangeSize();

private:
        void SaveColRowsAndRangesForDelete(tSaveSelectErase* sSaveSelectErase,
            tIndex sIndexParent, tIndex sPosition, tIndex sSize, tBool sDoesRow);
        void ErasePhysicalRowsAfterDelete(tIndex sRow, tIndex sSize);
        void ErasePhysicalColsAfterDelete(tIndex sCol, tIndex sSize);
        void DoDeleteColRowAxis(tSaveSelectErase* sSaveSelectErase,
            tIndex sPosition, tIndex sSize, tBool sDoesRow);
    };

    //============================================================================
    //! Contains all tColRowCellRange
    class tStaticColRowCellRange  {
        private:
            typedef tAllocator<tColRowCellRange, tAllocatorRef, 16> tAllocatorColRowCellRange;
            //! Array for all ColRowCellRange
            tAllocatorColRowCellRange m_ColRowCellRange;
        public:
            /// @brief      Constructor tStaticAllocatorTable.
            tStaticColRowCellRange();

            /// @brief      Destructor tStaticAllocatorTable.
            ~tStaticColRowCellRange();

            /// @brief      Clear.
            void Clear();

            /// @brief      Alloc tColRowCellRange.
            /// @param[out]	sAllocatorRef tAllocatorRef
            /// @return		tColRowCellRange*
            tColRowCellRange* Alloc(tAllocatorRef& sAllocatorRef);
            /// @brief      Return tColRowCellRange by Index.
            /// @param[in]	sIndex tIndex
            /// @return		tColRowCellRange*
            tColRowCellRange* ColRowCellRange(tAllocatorRef sAllocatorRef);
            /// @brief      Delete  tColRowCellRange by Index.
            /// @param[in]	sIndex tIndex
            void Delete(tAllocatorRef sAllocatorRef);

            /// @brief      Return static instance of SkTableAllocatorTable.
            /// @return		SkTableAllocatorTable*
            static tStaticColRowCellRange* Instance();
            static void Done();
    };

} // End of namespace
#endif
