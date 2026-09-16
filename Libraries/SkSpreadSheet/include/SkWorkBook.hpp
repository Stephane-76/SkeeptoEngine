//=============================================================================
// SkSpreadSheet WorkBook
//=============================================================================
#ifndef SkWorkBook_hpp
#define SkWorkBook_hpp
#include <vector>
#include <map>
#include <mutex>
#include <SkApplication.hpp>
#include <SkJsonSharedString.hpp>
#include "SkSheet.hpp"
#include "SkPrintParameters.hpp"
#include "SkFunction.hpp"
#include "SkFunctionMath.hpp"
#include "SkFunctionSpreadSheet.hpp"
#include "SkFunctionLogical.hpp"
#include "SkFunctionDate.hpp"
#include "SkFunctionText.hpp"
#include "SkFunctionFinancial.hpp"
#include "SkFunctionArray.hpp"
#include "SkSelect.hpp"
#include "SkRangeNamed.hpp"
#include "SkFloatingObject.hpp"
#include "SkLemonInterface.hpp"
#include "SkUndoRedoRebase.hpp"
#include "SkTableStyle.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

	// Forward declarations
	class tContainerPath;
	class tSheet;
	typedef std::vector<tSheet*> tVectorSheet;

	class tSpreadSheetContainer;

	class tWorkBookInfo : public tClass {
	private:
		//! User Name	
		tString m_AuthorName;
		tString m_AuthorEmail;
		//! Date Creation
		tDate m_DateCreation;
		//! Date Modification
		tDate m_DateModification;
		//! Version
		tInt m_VersionMajor;
		tInt m_VersionMinor;
		tInt m_VersionPatch;
		//! Comment
		tString m_Comment;
	public:
		//! Constructor
		tWorkBookInfo();
		//! Destructor
		~tWorkBookInfo();

		//! Json
		void Json(Writer<StringBuffer>* sWriter);
		void Json(const Value& sValue);
	};
    

	//==========================================================================
	//! WorkBook
	class tWorkBook : public SkSpAncestor {
	private:
		typedef tAllocator<tSheet, tAllocatorRef, 5> tSheetAllocator;
		///! Allocator of Sheet
		tSheetAllocator	m_SheetAllocator;
		///! Id of allocator
		tAllocatorRef  m_AllocatorRef;
        ///! Identification ==============================================================
		tWorkBookInfo m_WorkBookInfo;
        
        ///! Named Range Container for Name & Data
		tRangeNamedContainer m_RangeNamedContainer;

        //! Floating objects registry (host on CstSheetClassAnchor, layout in entry).
        tFloatingObjectContainer m_FloatingObjectContainer;

		///! Vector of sheet contains all sheet
		tVectorAllocatorRef m_VectorSheet;

        //! Sheets removed from the list but kept in the allocator (undo delete-sheet).
        std::map<tString, tAllocatorRef> m_OffListSheets;
        
		//! Active sheet (see tSheet.hpp).
		tAllocatorRef		m_ActiveSheet;

		///! Uri of WorkBook
		tString	m_Uri;

		//! Default size of col and row
		tDouble m_DefaultSizeCol;
		tDouble m_DefaultSizeRow;

		//! Workbook-shared print layout (paper, margins, scale, …). Orientation and FitToPage are per sheet.
		tPrintParameters m_PrintParameters;

		//! Workbook default font (OOXML styles.xml fonts[0] on xlsx import)
		tString m_DefaultFontName;
		tDouble m_DefaultFontSize;

		//! Lemon Interface get  tSpreadSheetContainer
		tLemonInterface* m_LemonInterface;

		//! Undo Rebase Log for this workbook
		tUndoRebaseLog m_UndoRebaseLog;
  
        //! Sheet for Named Formula
        tAllocatorRef  m_SheetNamedFormulaRef;
        
        //! Sheet for class attribute anchor
        tAllocatorRef  m_SheetClassAnchor;

        //! Deduped style blobs for JsonView (cells reference them via f_i).
        struct tJsonViewStyleKey {
            tFormatRef m_Sheet;
            tFormatRef m_Col;
            tFormatRef m_Row;
            tFormatRef m_Table;
            tFormatRef m_Table2;
            tFormatRef m_Table3;
            tFormatRef m_Cell;
            tFormatRef m_ItemCf;
            bool operator<(const tJsonViewStyleKey& sOther) const;
        };
        //! Table style container
		tTableStyleContainer m_TableStyleContainer;
        //! Json view format table
        std::vector<tString> m_JsonViewFormatTable;
        //! Json view format index
        std::map<tJsonViewStyleKey, tSize> m_JsonViewFormatIndex;
        //! Json view format table active
        tBool m_JsonViewFormatTableActive;

        /// Interned text-align:none; Excel General overlay (column h-align must not leak).
        tFormatRef m_JsonViewGeneralHAlignRef;

        tFormatRef EnsureJsonViewGeneralHAlign();

        //! Cooperative full-workbook recalc (ReduceStep); null when idle.
        tContainerPath* m_CooperativeRecalcPath;
        tBool m_CooperativeRecalcActive;
        tInt m_CooperativeRecalcTotal;
        //! When true, tSaveSelect::CalculateDo defers Reduce and hands the graph to cooperative recalc.
        tBool m_CooperativeCalculateEnabled;

        //! Callers of ROW()/COLUMN() named formulas (Excel evaluates those names in the calling cell).
        std::vector<tCell*> m_NamedFormulaCallerStack;

        struct tNamedCallerEvalKey {
            const tFormulaNamed* m_Fn = nullptr;
            tIndex m_Row = 0;
            tIndex m_Col = 0;
            tBool operator<(const tNamedCallerEvalKey& sOther) const {
                if (m_Fn != sOther.m_Fn) {
                    return m_Fn < sOther.m_Fn;
                }
                if (m_Row != sOther.m_Row) {
                    return m_Row < sOther.m_Row;
                }
                return m_Col < sOther.m_Col;
            }
        };
        std::map<tNamedCallerEvalKey, tVariant> m_NamedCallerEvalCache;

        void CancelRecalculateAllCooperative();
        void PrepareRecalculateAll(tContainerPath* sContainerPath);

        tSize AcquireJsonViewStyleIndex(
            tFormatRef sSheet,
            tFormatRef sCol,
            tFormatRef sRow,
            tFormatRef sTable,
            tFormatRef sTable2,
            tFormatRef sTable3,
            tFormatRef sCell,
            tFormatRef sItemCf,
            tBool sCss);
		//! Interface default Sheet (see SkApi.hpp)
		SkInline void NormalizeSheet(tSheet** sSheet);

		/// @brief      Return iterator of m_VectorSheet by name.
		/// @param[in]  tString
		/// @return		tVectorSheet::iterator
		tVectorAllocatorRef::iterator GetIteratorSheet(tString sSheetName);
	public:
		/// @brief      Constructor SkWorkBook.
		tWorkBook();
		
		/// @brief      Destructor SkWorkBook.
		~tWorkBook();

		//! Copy constructor is deleted (mutex is not copyable)
		tWorkBook(const tWorkBook&) = delete;
		
		//! Assignment operator is deleted (mutex is not copyable)
		tWorkBook& operator=(const tWorkBook&) = delete;

		/// @brief      Clear SkWorkBook.
		void Clear();

		// Rebase ===============================================================
		/// @brief      Get UndoRebaseLog for this workbook
		/// @return     tUndoRebaseLog& reference to the rebase log
		tUndoRebaseLog& UndoRebaseLog();

        
        /// @brief      Clear All Undo Redo of AllSheet
        void ClearUndoRedo();
        

		// @brief  Set Id and Uri;
		/// @param[in]	sAllocatorRef tAllocatorRef
		/// @param[in]	sUri tString
		void Set(tAllocatorRef sAllocatorRef, tString sUri);

		//! Identification ==============================================================
		void WorkBookInfo(tString sJson);

		tString WorkBookInfo();

		/// @brief		Return Id of allocator.
		/// @return		tIndex
		tAllocatorRef  AllocatorRef();
		
		/// @brief		Set Id of allocator.
		/// @param[ind]	sId tIndex
		void  AllocatorRef(tAllocatorRef sAllocatorRef);

		/// @brief		Return Uri.
		/// @return		tString
		tString Uri();
        
        /// @brief        Set Uri.
        /// @param[in]    sUri        tString
        void Uri(tString sUri);


        /// Format ============================================================
        /// @brief      Set Format Api .
        /// @param[in]  sFormatApi*  tFormatApi*
        void FormatApi(tFormatApi* sFormatApi);

        /// @brief   Get Format Api .
        /// @return  tFormatApi*
        tFormatApi* FormatApi();

        /// @brief Excel ListObject table style registry for this workbook.
        tTableStyleContainer* TableStyleContainer();
        const tTableStyleContainer* TableStyleContainer() const;

        /// @brief Merged table-style overlay for a sheet cell (0 outside any table).
        tFormatRef TableStyleOverlayFormat(tSheet* sSheet, tIndex sRow, tIndex sCol);

        /// @brief Table overlay merged to a CSS string (SkExcel export).
        tString TableStyleOverlayCss(tSheet* sSheet, tIndex sRow, tIndex sCol);
        
        /// @brief     Get CellFormat
        /// @param[in]  sCell tCell*l
        /// @return  tString
        tString CellFormatString(tCell* sCell);
        
        /// @brief     Get input string
        /// @param[in]  sCell tCell*l
        /// @return  tString
        tString CellInputString(tCell* sCell);
        
        /// @brief      Return MaskBorder (For ClipRect)
        /// @param [in] sAllocatordRef  tFormatRef
        /// @return tShort
        tShort CellBorder(tFormatRef sAllocatorRef);
        
        /// @brief     Apply CellFormat
        /// @param[in]  sCell tCell*l
        /// @param[in]  sFormat tString
        /// @return  tBool
        tBool ApplyCellFormat(tCell* sCell,tString sFormat);
        
        /// @brief     Apply CellFormat on Row or Col
        /// @param[in]  sColRow tColRow*
        /// @param[in]  sFormat tString
        /// @return  tBool
        tBool ApplyColRowFormat(tColRow* sColRow,tString sFormat);

        /// @brief     Apply CellFormat onSheet
        /// @param[in]  sSheet tSheet*
        /// @param[in]  sFormat tString
        /// @return  tBool
        tBool ApplySheetFormat(tSheet* sSheet,tString sFormat);
        
        /// @brief     DeletelFormat
        /// @param[in]  sFormatRef  tFormatRef
        void DeleteCellFormat(tFormatRef sFormatRef);

        /// @brief     Inc Format
        /// @param[in]  sFormatRef  tFormatRef
        void IncCellFormat(tFormatRef sFormatRef);

        /// @brief     Return cell refcount on a format pool entry.
        /// @param[in]  sFormatRef  tFormatRef
        tInt CellFormatRefCount(tFormatRef sFormatRef);
        
        /// @brief     Return string Format
        /// @param[in]  sFormatRef tFormatRefRef
        /// @return tString
        tString  CellFormat(tFormatRef sFormatRef);
    
        /// @brief     Return merged  string Format
        /// @param[in] sVector tVectorFormatRef
        /// @return tString
        tString  CellFormat(tVectorFormatRef* sVector);
        
        /// @brief     delete Border
        /// @param[in] sFormatRef  tFormatRef
        /// @param[in] sBorderMask tByte
        /// @return tFormatRef
        tFormatRef DeleteBorder(tFormatRef sFormatRef,tShort sBorderMask);

        /// @brief     Return Border mask
        /// @param[in] sFormatRef  tFormatRef
        /// @return  tShort
        tShort BorderMask(tFormatRef sFormatRef);
        
        // Json ===============================================================
        /// @brief      Writer Json for JavaScript
        /// @param[in] sSheet tSheet*
        /// @param[in] sRow tIndex
        /// @param[in] sCol tIndex
        /// @param[in] sCellConsitionalFormat tCellConditionalFormat
        /// @param[in] sWriter Writer<StringBuffer>*
        void JsonFormatJavaScript(tSheet* sSheet,tIndex sRow, tIndex sCol,tBool sCss,tCellConditionalFormat* sCellConditionalFormat,Writer<StringBuffer>* sWriter);

        /// @brief Reset per-JsonView format dedup table (call at start of tJsonView::View).
        void BeginJsonViewFormatTable();

        /// @brief Emit formats[] array at end of JsonView when dedup table is non-empty.
        void WriteJsonViewFormatTable(Writer<StringBuffer>* sWriter);

        /// Sheet =================================================================
		/// @brief      Add sheet by name.
		/// @param[in]	sSheetName tString
        /// @param[in]    sSheetLeft tString (if empty insert at end)
		tSheet* AddSheet(tString sSheetName,tString sSheetLeft="");

		/// @brief      Add sheet in list.
		/// @param[in]	sSheetAllocator  tAllocatorRef
		/// @param[in]	sSheetLeft tString
		void AddSheetInList(tAllocatorRef sSheetAllocator,tString sSheetLeft);

        /// @brief      Swap  sheezt in list.
        /// @param[in]    sName1  tString
        /// @param[in]    sName2  tString
        void SwapSheets(tString sName1,tString sName2);

        /// @brief      Move a sheet to sit immediately after sInsertAfterName (empty = first).
        void MoveSheet(tString sSheetName, tString sInsertAfterName);

        /// @brief      Name of the sheet immediately to the left of sSheetName (empty if first).
        tString SheetLeftOf(tString sSheetName);

		/// @brief      Delete sheet by name (return false if don't exist) (use sErase for delete Sheet).
		/// @param[in]  sSheetName tString
		/// @param[in]  sErase tBool
		/// @return		tBool 
		tBool DeleteSheet(tString sSheetName, tBool sErase);

		/// @brief      Delete sheet by name (return false if don't exist) (use sErase for delete Sheet).
		/// @param[in]  sIndex	 tIndex 
		/// @return		tBool 
		tBool DeleteSheet(tAllocatorRef sAllocatorRef);

        /// @brief      Take a sheet kept off-list after DeleteSheet(..., false) and reinsert it.
        tSheet* TakeOffListSheet(tString sSheetName, tIndex sIndex);

		/// @brief      Return sheet by name (return nullptr if don't exist).
		/// @param[in]  sSheetName tString
		/// @return		tSheet*
		tSheet* Sheet(tString sSheetName);

		/// @brief      Return sheet by AllocatorRef.
		/// @param[in]  sAllocatorRef tAllocatorRef.
		/// @return		tSheet*
		tSheet* SheetByAllocator(tAllocatorRef sAllocatorRef);

		/// @brief      Set active sheet by name (return nullptr if don't exist).
		/// @param[in]  sSheetName tString
		/// @return		tSheet*
		tSheet* ActiveSheet(tString sSheetName);

		/// @brief      Return active sheet.
		/// @return		tSheet*
		tSheet* ActiveSheet();

        ///@breif Recalculate Sheet
        ///@param[in] tSheet* sSheet
		///@param[in] tContainerPath* sContainerPath
        void RecalculateSheet(tSheet* sSheet,tContainerPath* sContainerPath);

		/// @brief      Recalculate all sheets in the workbook.
		/// This method iterates through all sheets and recalculates all cells with formulas.
		void RecalculateAll();

		/// @brief      Begin cooperative RecalculateAll (graph built; call StepRecalculateAllCooperative).
		void BeginRecalculateAllCooperative();

		/// @brief      Advance cooperative recalc by at most sMaxMs milliseconds.
		/// @return     true when finished.
		tBool StepRecalculateAllCooperative(tInt sMaxMs);

		/// @brief      Progress 0..100 while cooperative recalc is active.
		tInt RecalculateAllCooperativeProgress();

		/// @brief      True while a cooperative recalc session is in progress.
		tBool IsRecalculateAllCooperativeActive() const;

		/// @brief      When enabled, undo CalculateDo builds the graph for cooperative StepRecalculateAllCooperative.
		void SetCooperativeCalculateEnabled(tBool sEnabled);
		tBool CooperativeCalculateEnabled() const;

		/// @brief      Hand a tSaveSelect calculation graph to cooperative StepRecalculateAllCooperative.
		void BeginCooperativeCalculateFromSaveSelect(class tSaveSelect* sSaveSelect, tColRowCellRange* sColRowCellRange, tVolatile sVolatile);

		/// @brief      Set tCell::Path() to zero on every cell (all sheets + named-formula sheet).
		/// Clears stale path indices after a tContainerPath is destroyed (e.g. incomplete Reduce) so Add() can queue cells again.
		void ResetAllCellCalculationPathsToZero();
  
		/// @brief      Recalculate sheet named formula.
		/// This method recalculates the sheet named formula.
		void RecalculateSheetNamedFormula();

		/// @brief      After ReadJson defer-recalc: mark spill slaves loaded from v/t so RecalculateAll won't #SPILL!.
		void RelinkJsonPersistedSpillSlaves();

		/// @brief      Before RecalculateAll: clear persisted spill slave values so origins can respill.
		void ClearPersistedSpillSlavesForRecalc();

		/// @brief      Return pointer of vector of Sheet.
		/// @return		tVectorSheet*
		tVectorAllocatorRef*	VectorSheet();

		/// @brief      Return pointer of Sheet by index.
		/// @param[in]	sIndex tIndex
		/// @return		tSheet*.
		tSheet* Sheet(tIndex sIndex);
        
        /// @brief      VectorSheet
        /// @return     tVectorSheet&
        tVectorSheet VectorPtSheet();
        
		// Cell ================================================================
		/// @brief		Compil cell. 
		/// @param[in]	sCell tCell* cell to compil
		/// @param[in]	sValue tChar* code
		/// @return		tBool (true if Ok false if error).
		tBool CompilCell(tCell* sCell, const tChar* sValue);

		/// @brief		Set cell value or formula (Calculate if sCalculate = true). 
		/// @param[in]	sCell tCell* cell to compil
		/// @param[in]	sVariant tVariant
		/// @param[in]  sCalculate tIndex
		/// @return		tBool (true if Ok false if error).
		tBool CellValue(tCell* sCell, tVariant& sVariant,tBool sCalculate=true);
  
        // Lemon interface
        ///@brief LemonInterface
        ///@return tLemonInterface*
        tLemonInterface* LemonInterface();

		// Named Range ========================================================
		/// @brief      Return SkRangeNamedContainer for UndoRedo Delete Col Row.
		tRangeNamedContainer* RangeNamedContainer();

        /// @brief      Return floating-object registry for this workbook.
        tFloatingObjectContainer* FloatingObjectContainer();

		/// @brief      Insert Range Named.
		/// @param[in]  sName tString
        /// @param[in]  sFormula tString
		/// @param[in]  sRect SkRect
        /// @param[in]  sSheet tSheet
        /// @return     tRange*
        tRange* InsertRangeNamed(tString sName, tTempoRect sRect,tSheet* sSheet);

		/// @brief      Insert Range Named.
		/// @param[in]  sName tString
        /// @param[in]  sFormula tString
        /// @return     tBool
        tBool InsertNamedFormula(tString sName,tString sFormula);

		/// @brief      Find FormulaNamed by Name.
		/// @param[in]  sName tString
		/// @return		tFormulaNamed*
		tFormulaNamed* FindFormulaNamed(tString sName);

        /// @brief      Find named formula by its definition cell (see FormulaNamedByCell on range container).
        /// @param[in]  sCell tCell*
        /// @return     tFormulaNamed* or nullptr
        tFormulaNamed* FindFormulaNamedByCell(tCell* sCell);

        /// @brief      Find named formula by spill range on CstSheetNamed (FormulaStr / Excel parity).
        /// @param[in]  sRange spill tRange* (e.g. A1:G6 on _$$)
        /// @return     tFormulaNamed* or nullptr
        tFormulaNamed* FindFormulaNamedBySpillRange(tRange* sRange);

        /// @brief Push the cell that is evaluating a caller-relative named formula (ROW()/COLUMN() context).
        void PushNamedFormulaCaller(tCell* sCaller);

        /// @brief Pop the current named-formula caller (must pair with PushNamedFormulaCaller).
        void PopNamedFormulaCaller();

        /// @brief Innermost cell whose formula is evaluating a caller-relative named name, or nullptr.
        tCell* NamedFormulaCaller() const;

        /// @brief Drop per-pass memo of EvaluateAtCaller (ROW()/COLUMN() named formulas).
        void ClearNamedCallerEvalCache();

        /// @brief Memoized EvaluateAtCaller result for this calculation pass.
        tBool TryNamedCallerEval(const tFormulaNamed* sFn, tIndex sRow, tIndex sCol, tVariant& oOut) const;
        void StoreNamedCallerEval(const tFormulaNamed* sFn, tIndex sRow, tIndex sCol, const tVariant& sValue);

        /// @brief Read spill from in-memory buffer for absolute (row,col) on the named-formula sheet (_$$).
        /// @param sOperandRangeTop Prefer formula whose definition lies in the operand rect; -1 for no filter.
        /// @param sOperandRangeHeight Width/height of operand range for definition containment (see SkRangeNamed).
        tBool TryNamedFormulaSpillBufferAt(tIndex sRow, tIndex sCol, tVariant& out,
                                         tIndex sOperandRangeTop = -1, tIndex sOperandRangeLeft = -1,
                                         tIndex sOperandRangeHeight = -1, tIndex sOperandRangeWidth = -1);

        /// @brief      Insert Range Data.
        /// @param[in]  sName tString
        /// @param[in]  sRangeDatatRangeData
        /// @param[in]  sRect SkRect
        /// @param[in]  sSheet tSheet
        /// @return     tRange*
        tRange* InsertRangeData(tString sName,tRangeData sRangeData, tTempoRect sRect, tSheet* sSheet);
            
        /// @brief      Insert Range Data.
        /// @param[in]  sName tString
        /// @param[in]  sRangeDatatRangeData
        /// @return     tRange*
        tRange* ApplyRangeData(tString sName,tRangeData sRangeData);

        /// @brief Replace RangeData metadata without re-running sort/filter.
        void ReplaceRangeDataMetadata(tString sName, tRangeData sRangeData);

        /// @brief Shift/sync table column metadata after a column insert.
        void SyncRangeDataAfterColumnInsert(tRange* sRange, tIndex sInsertCol, tIndex sWidth);

        /// @brief Materialize calculated-column default formulas on newly inserted table data rows.
        ///        Uses <calculatedColumnFormula> when set, otherwise copies an existing data-row formula
        ///        in the same column (Excel table autofill). Compiles cells so VectorRef deps are built.
        /// @param sLeftCol/sRightCol 0 = no column clip (full table width); otherwise intersect with table.
        void ApplyCalculatedColumnFormulasToInsertedRows(tSheet* sSheet,
                                                         tIndex sFirstRow, tIndex sLastRow,
                                                         tIndex sLeftCol = 0, tIndex sRightCol = 0);
            
		/// @brief      Delete RangeNamed.
		/// @param[in]  sName tString
		/// @return  	tBool 
		tBool DeleteRangeNamed(tString sName);

		/// @brief      Find RangeNamed by allocatorRef.
		/// @param[in]  sAllocatorRef tsAllocatorRef
		/// @return		tString
		tString FindRangeNamed(tAllocatorRef sAllocatorRef,tSheet* sSheet);

		/// @brief      Find RangeNamed by Name.
		/// @param[in]  sName tString
		/// @return		tRange* 
		tRange* FindRangeNamed(tString sName);
  
        /// @brief      Return Range Data.
		/// @param[in]  sName tString
		/// @return		tRangeData*
		tRangeData* RangeData(tString sName);
  
  
        ///@brief Return Range Data
        ///@param[in] sName tString
        ///@param[in] sColumnName tString
        ///@return tColumnData*
        tColumnData* RangeDataColumn(tString sName,tString sColumnName);
  
        /// @brief      SheetNamedFormula
        /// @description We use this spreadsheet_ CstSheetNamed 
		/// It is attached to the workbook. It allows us to contain the Formulas of the Formula Names.
        ///@brief SheetNamedFormula
		/// @return tSheet*
        tSheet* SheetNamedFormula();

		/// @brief      SheetClassAnchor
		/// @description System sheet CstSheetClassAnchor (_$$A). Hosts one cell per
		///             floating object; tCellClassAttribute and attributes are serialized
		///             with the sheet (sheets[]), not in floatingobjects[].
		/// @return     tSheet*
		tSheet* SheetClassAnchor();

        /// @brief        Get list of sheets.
        /// @param[in]    sWriter Writer<StringBuffer>* 
        void JsonSheets(Writer<StringBuffer>* sWriter);

        /// @brief        Get list of sheets.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonInfo(Writer<StringBuffer>* sWriter);

        
		/// @brief      Return json list of sheets.
		/// @return		tString 
		tString  JsonSheets();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const Value& sValue);

     
		/// @brief		WriteJson().
		/// @return  	tString 
		tString WriteJson();

		/// @brief		Reader Json. 
		void ReadJson(tString sJson);

		/// @brief		Return Json for RangeNameds.
		/// @return  	tString 
		tString JsonRangeNamed();
  
        /// @brief		Return .
		/// @param[in]	sWriter Writer<StringBuffer>*
        tString JsonFormulaNamed();
        

		/// @brief		Return Json for RangeData.
		/// @return  	tString 
		tString JsonRangeData();

		/// @brief		Return Json for ConditionalFormats in sheet.
		/// @param[in]  sSheet tSheet*
		/// @return  	tString 
		tString JsonConditionalFormats();

		/// @brief		Return Json for ConditionalFormats in sheet.
		/// @param[in]  sSheet tSheet*
		/// @return  	tString 
		tString JsonConditionalFormats(tSheet* sSheet);

		/// @brief      Return all RangeNameds IN sheet.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[out] sSheetAllocatorRef tVectorAllocatorRef 
		void GetRangeNamedsBySheet(tAllocatorRef sSheetAllocatorRef, tVectorAllocatorRef& sRangesAllocatorRef);

		/// @brief		Return default size of col millimeter.
		/// @return  	tDouble
		tDouble DefaultSizeCol();

		/// @brief      Set default size of col millimeter..
		/// @param[in]  tDouble sValue
		void DefaultSizeCol(tDouble sValue);

		/// @brief		Return Default size of row millimeter.
		/// @return  	tDouble
		tDouble DefaultSizeRow();

		/// @brief      Set default size of row millimeter..
		/// @param[in]  tDouble sValue
		void DefaultSizeRow(tDouble sValue);

		/// @brief Workbook default font family (Excel styles.xml fonts[0]).
		tString DefaultFontName() const;
		void DefaultFontName(tString sValue);

		/// @brief Workbook default font size in points.
		tDouble DefaultFontSize() const;
		void DefaultFontSize(tDouble sValue);

		/// @brief Workbook-shared print layout (not orientation / FitToPage).
		tPrintParameters& PrintParameters();
		const tPrintParameters& PrintParameters() const;
		/// @brief Copy workbook-shared fields from sSrc when current values are still defaults.
		void AdoptWorkBookPrintFieldsIfDefault(const tPrintParameters& sSrc);

		/// @brief		Operator for sort.
		/// @param[in]	sWorkBook SkWorkBook& 
		tBool operator < (tWorkBook& sWorkBook);

		/// @brief		Operator for sort.
		/// @param[in]	sWorkBook SkWorkBook& 
		/// @return  	tBool 
		tBool operator == (tWorkBook& sWorkBook);


#ifdef checksp
		/// @brief Check.
		void Check();
#endif
#ifdef checkfo        
        /// @brief Check format
        void CheckFormat();
#endif
#ifdef _DEBUGSK
        /// @brief      Debug
        tString Debug();
        
        /// @rief Debug Fomrat  cell
        /// @param[in]    sCell tCell*
        tString DebugFormatCell(tCell* sCell);
#endif

	};





} // End of namespace
#endif
