//=============================================================================
// SkUISpreadSheetApi.hpp
// Api for React wasm
//=============================================================================
#ifndef SkUISpreadSheetApi_hpp
#define SkUISpreadSheetApi_hpp
#include <SkTypes.hpp>
#include <SkApplication.hpp>
#include <SkSpreadSheet.hpp>
#include <SkApi.hpp>
#include <SkFormatCssApi.hpp>
#include <SkInterfaceWeb.hpp>
#include "SkJavascriptFunction.hpp"

namespace SkSpreadSheet {
    //=========================================================================
    // Enreg View TopRow TopCol for undo redo
    //=========================================================================
    class tUIExtraUndo :  public tUndoExtra {
    private:
        tString m_Json;
    public:
        /// @brief      Constructor tUIExtraUndo
        tUIExtraUndo();
        
        /// @brief      Constructor tUIExtraUndo
        /// @param[in] sJson tString
        tUIExtraUndo(tString sJson);
        
        /// @brief      Copy constructor tUIExtraUndo
        /// @param[in] sUIExtraUndo tUIExtraUndo&
        tUIExtraUndo(const tUIExtraUndo& sUIExtraUndo);

        /// @brief      Copy assignment tUIExtraUndo
        /// @param[in] sUIExtraUndo tUIExtraUndo&
        /// @return     tUIExtraUndo&
        tUIExtraUndo& operator=(const tUIExtraUndo& sUIExtraUndo);

        /// @brief     Set JSON string
        /// @param[in] sJson tString
        void _Json(tString sJson);

        /// @brief     Get JSON string
        /// @return    tString
        tString _Json();
    };

    // Interface with Javascript ===============================================
    class tUISpreadSheet : public tInterfaceWeb {
    private:
        //! Application
        SkRoot::tApplication*      m_Application;
        //! Format API
        SkFormat::tFormatCssApi*   m_FormatApi;
        //! Extra undo information
        tUIExtraUndo               m_UIExtraUndo;
        //! Cooperative _Pressure generation state (non-blocking, JS-driven steps).
        tBool                      m_PressureActive = false;
        tInt                       m_PressureRow = 0;        // next row to generate (2..total)
        tInt                       m_PressureTotalRows = 0;  // target row count
        tInt                       m_PressureCol = 0;        // target column count
        clock_t                    m_PressureGenClock = 0;   // generation start (for the "done in" log)
    public:
        /// @brief      Constructor 
        tUISpreadSheet();
        /// @brief      Destructor
        ~tUISpreadSheet() override;
        
        /// @brief      Clear the spreadsheet
        void Clear();
        
        // All function begin with _ is used by Javascript

        // Interface on Change ================================================
        /// @brief      Handle cell change event
        /// @param[in]  sCell tCell* pointer to the cell
        /// @param[in]  sValue tVariant& reference to the new value
        void _OnCellChange(tCell* sCell, tVariant& sValue);
        
        // WorkBook ===========================================================
        /// @brief      Get the active workbook
        /// @return     tString URI of the active workbook
        tString _GetActiveWorkBook();
        
        /// @brief      Set the active workbook
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tBool
        tBool _SetActiveWorkBook(tString sUri);
        
        /// @brief      Create a new workbook
        /// @param[in]  sUri tString URI of the new workbook
        /// @return     tBool
        tBool _NewWorkBook(tString sUri);
        
        /// @brief      Add a workbook
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tBool
        tBool _AddWorkBook(tString sUri);
        
        /// @brief      Delete a workbook
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tBool
        tBool _DeleteWorkBook(tString sUri);
        
        /// @brief      Rename a workbook
        /// @param[in]  sUri tString current URI of the workbook
        /// @param[in]  sUriTo tString new URI of the workbook
        /// @return     tBool
        tBool _RenameWorkBook(tString sUri, tString sUriTo);
        
        /// @brief      Return Json workBook
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tString
        tString _JsonWorkBook(tString sUri);
        
        /// @brief      Retunr List of WorkBook
        /// @param[in]  sUri tString current URI of the workbook
        /// @param[in]  sUriTo tString new URI of the workbook
        /// @return     tBool
        tString _JsonWorkBooks();
        
        /// @brief      Write the workbook to JSON
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tString JSON representation of the workbook
        tString _WriteJson(tString sUri);
        
        /// @brief      Read the workbook from JSON
        /// @param[in]  sUri tString JSON string
        /// @return     tBool
        tBool _ReadJson(tString sUri);

        /// @brief      Full workbook recalculation (_$$ named-formula sheet first, then user sheets).
        /// Needed after ReadJson so dependent cells (e.g. row above referencing row below) get correct values.
        /// @return     tBool false if no active workbook
        tBool _RecalculateAll();

        /// @brief      Cooperative full-workbook recalc (non-blocking steps from JS).
        void _BeginRecalculateAllCooperative();
        tBool _StepRecalculateAllCooperative(tInt sMaxMs);
        tInt _RecalculateAllCooperativeProgress();
        tBool _IsRecalculateAllCooperativeActive();
        void _SetCooperativeCalculateEnabled(tBool sEnabled);
        tBool _CooperativeCalculateEnabled();
        /// @brief      Undo the last action
        void RebaseUIExtraUndo(tSequenceId sSequenceId);
   
        
        /// @brief      Undo the last action
        void _Undo();
        
        /// @brief      Redo the last undone action
        void _Redo();

        /// @brief      Get undo-stack recording status (default false on tApi).
        tBool _IsUndoActif();

        /// @brief      Enable or disable undo-stack recording.
        /// @param[in]  sIsUndoActif tBool
        void _SetIsUndoActif(tBool sIsUndoActif);
        
        /// @brief      Set extra undo information
        /// @param[in]  sJson tString JSON string
        void _SetExtraUndo(tString sJson);
        
        /// @brief      Get extra undo information
        /// @return     tString JSON string
        tString _GetExtraUndo();


        // User Interface ======================================================
        /// @brief      Set user interface
        /// @param[in]  sJson tString JSON string
        void _SetUserInterface(tString sJson);
        
        /// @brief      Get user interface
        /// @return     tString JSON string
        tString _GetUserInterface();
        
        /// @brief      Fix extra undo information
        void FixUndoExtra();
        
        /// @brief      Is not Active Sheet Set Sheet
        /// @param[in]  sSheet tString
        /// @return     tBool
        tBool SetSheet(tString sSheet);
            
        // Sheet ==============================================================
        /// @brief      Set the active sheet
        /// @param[in]  sSheetName tString name of the sheet
        /// @return     tBool
        tBool _SetActiveSheet(tString sSheetName);
        
        /// @brief      Get the active sheet
        /// @return     tString name of the active sheet
        tString _GetActiveSheet();
        
        /// @brief      Add a sheet
        /// @param[in]  sName tString name of the sheet
        /// @param[in]  sLeft tString name of the sheet to the left
        /// @return     tBool
        tBool _AddSheet(tString sName, tString sLeft);
        
        /// @brief      Rename a sheet
        /// @param[in]  sName tString current name of the sheet
        /// @param[in]  sNewName tString new name of the sheet
        /// @return     tBool
        tBool _RenameSheet(tString sName, tString sNewName);
        
        /// @brief      Swap two sheets
        /// @param[in]  sName1 tString name of the first sheet
        /// @param[in]  sName2 tString name of the second sheet
        /// @return     tBool
        tBool _SwapSheet(tString sName1, tString sName2, tBool sInsertAfter = false);
        
        /// @brief      Delete a sheet
        /// @param[in]  sName tString name of the sheet
        /// @return     tBool
        tBool _DeleteSheet(tString sName);
        
        /// @brief      Get the list of sheets
        /// @return     tString list of sheets
        tString _SheetsList();
        
        // Cells ==============================================================
        /// @brief      Set the value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tString value to set 
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Value(tString sRef, tString sValue, tString sSheet);

        /// @brief      Excel-like fill-handle series (single undo).
        /// @param[in]  sSourceRef tString seed range (e.g. A1:A2)
        /// @param[in]  sDestRef tString extension range to fill (e.g. A3:A12)
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _FillSeries(tString sSourceRef, tString sDestRef, tString sSheet);
        
        /// @brief      Set the value of a cell attribute
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sName tString name of the attribute
        /// @param[in]  sValue tString value to set
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ValueAttribute(tString sRef, tString sName, tString sValue, tString sSheet);

        /// @brief      Set multiple cell-class attributes on one host cell (single undo step).
        /// @param[in]  sAttributesJson JSON array of {"n":name,"v":value} objects
        tBool _CellClassAttributes(tString sRef, tString sAttributesJson, tString sSheet);

        /// @brief      Set calculable scalar on tCellClassAttribute (formula-visible value).
        tBool _ValueClassCalculable(tString sRef, tString sValue, tString sSheet);

        /// @brief      Set the string value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tString value to set
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ValueString(tString sRef, tString sValue, tString sSheet);
        
        /// @brief      Set the integer value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tInt value to set    
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ValueInt(tString sRef, tInt sValue, tString sSheet);
        
        /// @brief      Set the double value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tDouble value to set
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ValueDouble(tString sRef, tDouble sValue, tString sSheet);

        /// @brief      Get the value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tString value of the cell
        tString _GetValue(tString sRef, tString sSheet);

        /// @brief      Calculable scalar (unwraps tCellUnit); charts / numeric reads.
        tString _GetCalculableScalar(tString sRef, tString sSheet);
        
        /// @brief      Get the input value of a cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tString input value of the cell
        tString _GetInputValue(tString sRef, tString sSheet);

        /// @brief      Get the formula text for a cell (same as SkExcel tApi::Formula / FormulaStr)
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @param[in]  sUser Excel bar form for structured table refs
        /// @return     tString formula string, or empty if the cell has no formula
        tString _GetFormula(tString sRef, tString sSheet, tBool sUser = false);

        /// @brief      Formula-pick ref relative to edit anchor (structured @[Col] when in a table).
        /// @param[in]  sRefAnchor tString cell being edited (A1)
        /// @param[in]  sRef tString picked selection (A1 / A1:B2)
        /// @param[in]  sSheet tString sheet name
        /// @return     tString structured or A1 reference
        tString _CellRef(tString sRefAnchor, tString sRef, tString sSheet);

        /// @brief      Last lexer/parser error label after a failed compile (see tApi::Error).
        tString _Error();

        /// @brief      1-based line index in the formula source where the error was reported.
        tInt _ErrorLine();

        /// @brief      1-based column index in the formula source where the error was reported.
        tInt _ErrorColumn();

        /// @brief      Human-readable error with line/caret context (see tApi::ErrorWithDetail).
        tString _ErrorWithDetail();
        
        /// @brief      Get the value of a cell attribute
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sName tString name of the attribute
        /// @param[in]  sSheet tString sheet name
        /// @return     tString value of the attribute
        tString _GetValueAttribute(tString sRef, tString sName, tString sSheet);

        /// @brief      Get the live formula text for a cell attribute (rebased after insert/delete)
        /// @param[in]  sRef tString reference of the host cell
        /// @param[in]  sName tString name of the attribute
        /// @param[in]  sSheet tString sheet name
        /// @return     tString formula body without leading "=", or empty if not a formula attribute
        tString _GetFormulaAttribute(tString sRef, tString sName, tString sSheet);

        // Raz ================================================================
        /// @brief      Reset the cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Raz(tString sRef, tBool sKeepFormat, tString sSheet);
        tBool _RazFormat(tString sRef, tString sSheet);
    
        // Row & Cols =========================================================
        /// @brief      Set the size of rows
        /// @param[in]  sBegin tInt start row
        /// @param[in]  sEnd tInt end row
        /// @param[in]  sSize tDouble size to set
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _SizeRow(tInt sBegin, tInt sEnd, tDouble sSize, tString sSheet);
        
        /// @brief      Get the size of a row
        /// @param[in]  sIndex tInt row index
        /// @param[in]  sSheet tString sheet name
        /// @return     tDouble size of the row
        tDouble _GetSizeRow(tInt sIndex, tString sSheet);
    
        /// @brief      Set the size of columns
        /// @param[in]  sBegin tInt start column
        /// @param[in]  sEnd tInt end column
        /// @param[in]  sSize tDouble size to set
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _SizeCol(tInt sBegin, tInt sEnd, tDouble sSize, tString sSheet);
        
        /// @brief      Get the size of a column
        /// @param[in]  sIndex tInt column index
        /// @param[in]  sSheet tString sheet name
        /// @return     tDouble size of the column
        tDouble _GetSizeCol(tInt sIndex, tString sSheet);
    
        /// @brief      Insert rows
        /// @param[in]  sBegin tInt start row
        /// @param[in]  sEnd tInt end row   
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _InsertRow(tInt sBegin, tInt sEnd, tString sSheet);
        
        /// @brief      Delete rows
        /// @param[in]  sBegin tInt start row
        /// @param[in]  sEnd tInt end row
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteRow(tInt sBegin, tInt sEnd, tString sSheet);
        
        /// @brief      Delete rows determined by a rectangle reference (range)
        /// @param[in]  sRef tString reference like "A1:B5"
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteRowByRect(tString sRef, tString sSheet);
        
        /// @brief      Insert rows determined by a rectangle reference (range)
        /// @param[in]  sRef tString reference like "A1:B5"
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _InsertRowByRect(tString sRef, tString sSheet);

        /// @brief Insert row by rect and write a label cell in the same undo.
        tBool _InsertRowByRectWithLabel(tString sRef, tString sLabelRef, tString sLabelValue,
                                        tString sSheet);

        /// @brief      Insert columns
        /// @param[in]  sBegin tInt start column
        /// @param[in]  sEnd tInt end column
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _InsertCol(tInt sBegin, tInt sEnd, tString sSheet);
        
        /// @brief      Insert columns determined by a rectangle reference (range)
        /// @param[in]  sRef tString reference like "A1:B5"
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _InsertColByRect(tString sRef, tString sSheet);
        
        /// @brief      Delete columns
        /// @param[in]  sBegin tInt start column
        /// @param[in]  sEnd tInt end column
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteCol(tInt sBegin, tInt sEnd, tString sSheet);
        
        /// @brief      Delete columns determined by a rectangle reference (range)
        /// @param[in]  sRef tString reference like "A1:B5"
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteColByRect(tString sRef, tString sSheet);
        
        // Copy & Paste =======================================================
        /// @brief      Copy the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Copy(tString sRef, tString sSheet);

        /// @brief      Cut the selection at sRef (copy buffer + clear with undo).
        tBool _Cut(tString sRef, tString sSheet);

        /// @brief      Paste the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Paste(tString sRef, tString sSheet);

        /// @brief      Move a rectangular selection (copy buffer, clear source, paste dest).
        tBool _Move(tString sSourceRef, tString sDestRef, tString sSheet);

        // Format =============================================================
        /// @brief      Apply a format to the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tString format value
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Format(tString sRef, tString sValue, tString sSheet);
        
        /// @brief      Apply a format to the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sInc tBool increment or decrement precision
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Precision(tString sRef, tBool sInc, tString sSheet);
        
        // Conditional Format ==================================================
        /// @brief      Add Conditional Format Highlight Cells Rules
        /// @param[in]  sType tString
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sParam1 tString Parameter 1
        /// @param[in]  sParam2 tString Parameter 2
        /// @param[in]  sParam3 tString Parameter 3
        /// @param[in]  sParam4 tString Parameter 4
        /// @param[in]  sParam5 tString Parameter 5
        /// @param[in]  sParam6 tString Parameter 6
        /// @param[in]  sParam7 tString Parameter 7
        /// @param[in]  sParam8 tString Parameter 8
        /// @param[in]  sParam9 tString Parameter 9
        /// @param[in]  sParam10 tString Parameter 10
        /// @param[in]  sSheet tString
        /// @return     tConditionalFormat*
        tBool _ConditionalFormat(tString sType,tString sRef,tString sParam1,tString sParam2,tString sParam3,tString sParam4,tString sParam5,tString sParam6,tString sParam7,tString sParam8,tString sParam9,tString sParam10, tString sSheet);
        
        /// @brief      Delete a conditional format on the given reference
        /// @param[in]  sType tString
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteConditionalFormat(tString sType, tString sRef, tString sSheet);
              
        /// @brief      Debug the format (FormatRoot dump to cout; returns debug text)
        /// @return     tString FormatApi()->Debug() when _DEBUGSK, else empty
        tString _DEBUGSKFormat();

        /// @brief      Number of FormatRoot cell-format entries (m_MapFormat size).
        /// @return     tInt
        tInt _FormatCount();
        
        /// @brief      Apply a border to the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sBorder tInt border value
        /// @param[in]  sValue tString border format
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Border(tString sRef, tInt sBorder, tString sValue, tString sSheet);
        
        /// @brief      Get the format of the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tString format of the cell
        tString _GetFormat(tString sRef, tString sSheet);
        
        /// @brief      Get the JSON format string
        /// @return     tString JSON format string
        tString _JsonFormatString();

        /// @brief      Get JSON of all conditional formats on the active sheet
        /// @param[in]  sSheet tString sheet name
        /// @return     tString JSON array string
        tString _JsonConditionalFormat(tString sSheet);
        
        /// @brief      Get the default format string
        /// @param[in]  sFormatString tString format string
        /// @return     tString default format string
        tString _DefaultFormatString(tString sFormatString);

        /// @brief      Format a numeric value with the resolved format of a source cell.
        /// @param[in]  sRef tString cell reference
        /// @param[in]  sValue tString numeric value (parsed as double)
        /// @param[in]  sSheet tString sheet name
        /// @return     tString formatted display text
        tString _FormatValueWithCellFormat(tString sRef, tString sValue, tString sSheet);
        
        /// @brief      Apply a format string to the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sValue tString format value
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ApplyFormatString(tString sRef, tString sValue, tString sSheet);
        
        // Merge ==============================================================
        /// @brief      Merge the cells at the reference sRef
        /// @param[in]  sRef tString reference of the cells
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _Merge(tString sRef, tString sSheet);
        
        // View ==============================================================
        /// @brief      Return the merged cell at the specified row and column
        /// @param[in]  sRow tInt row
        /// @param[in]  sCol tInt column
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _ReturnMerged(tInt sRow, tInt sCol, tString sSheet);
        
        /// @brief      Return the merged range at the reference sRef
        /// @param[in]  sRef tString reference of the range
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _ReturnRangeMerged(tString sRef, tString sSheet);
        
        /// @brief      Return the merged range fusion at the reference sRef
        /// @param[in]  sRef tString reference of the range 
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _ReturnRangeMergedFusion(tString sRef, tString sSheet);
        
        /// @brief      Insert a named range
        /// @param[in]  sName tString name of the range
        /// @param[in]  sRef tString reference of the range
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _InsertNamedRange(tString sName, tString sRef, tString sSheet);

        /// @brief      Delete a named range
        /// @param[in]  sName tString name of the range
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _DeleteNamedRange(tString sName, tString sSheet);

        /// @brief      Update (rename and/or re-select) an existing named
        ///             range atomically. Preserves tAllocatorRef on areas
        ///             that stay in the selection so dependent formulas keep
        ///             evaluating and automatically pick up the new name.
        /// @param[in]  sOldName current name (must exist)
        /// @param[in]  sNewName target name (may equal sOldName when only
        ///                      the selection is changing)
        /// @param[in]  sNewRef new selection (e.g. "A1:A2;B1:B2", same sheet)
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _UpdateNamedRange(tString sOldName, tString sNewName,
                                tString sNewRef, tString sSheet);

        /// @brief      Insert (or replace) a named formula.
        /// @param[in]  sName     tString name of the formula
        /// @param[in]  sFormula  tString formula definition (without leading '=')
        /// @param[in]  sSheet    tString sheet name used as the editing context
        /// @return     tBool
        tBool _InsertFormulaNamed(tString sName, tString sFormula, tString sSheet);

        /// @brief      Delete a named formula.
        /// @param[in]  sName   tString name of the formula
        /// @param[in]  sSheet  tString sheet name used as the editing context
        /// @return     tBool
        tBool _DeleteFormulaNamed(tString sName, tString sSheet);

        /// @brief      Insert (or replace) a floating object on a target sheet.
        tBool _InsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tString sSheet,
                                    tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight, tDouble sOpacity,
                                    tString sAnchorCellRef);

        /// @brief      Delete a floating object by name.
        tBool _DeleteFloatingObject(tString sName, tString sSheet);

        /// @brief      Update floating object layout (optional anchor cell ref, e.g. "Sheet1!A1").
        tBool _FloatingObjectLayout(tString sName, tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight,
                                    tDouble sOpacity, tString sAnchorCellRef, tString sSheet);

        /// @brief      Bring floating object to front on its target sheet (undoable).
        tBool _FloatingObjectBringToFront(tString sName, tString sSheet);

        /// @brief      Set a host-cell attribute on a floating object (variant parsed from string).
        tBool _FloatingObjectAttribute(tString sName, tString sAttribute, tString sValue, tString sSheet);

        /// @brief      Set multiple host-cell attributes on a floating object (single undo step).
        /// @param[in]  sAttributesJson JSON array of {"n":name,"v":value} objects
        tBool _FloatingObjectAttributes(tString sName, tString sAttributesJson, tString sSheet);

        /// @brief      Tell the UI whether identifier renames are currently safe.
        ///
        ///             The UI uses this to disable the "rename sheet" /
        ///             "rename named range" buttons when multiple users are
        ///             editing the workbook, because the undo stream cannot
        ///             rebase textual names on peers.
        /// @return     tBool true in local mode or when the user is alone.
        tBool _IsRenameAllowed();

        /// @brief      Forward multi-user state from JS to the C++ layer.
        ///
        ///             Called by the JS glue whenever another user joins or
        ///             leaves the workbook. Drives the gating performed by
        ///             IsRenameAllowed(). Ignored under TestMultiUser builds
        ///             where the dispatcher is authoritative.
        /// @param[in]  sActive tBool
        void _SetMultiUserActive(tBool sActive);

        /// @brief      Return the JSON view of the cell at the specified row and column
        /// @param[in]  sRow tInt row
        /// @param[in]  sCol tInt column
        /// @param[in]  sViewHeight tDouble view height
        /// @param[in]  sViewWidth tDouble view width
        /// @param[in]  sDiffY tDouble Y-axis difference
        /// @param[in]  sDiffX tDouble X-axis difference
        /// @param[in]  sSheet tString sheet name
        /// @param[in]  sCss   tBool true = CSS strings in format fields (HTML export); false = canvas objects (grid)
        /// @return     tString
        tString _JsonView(tInt sRow, tInt sCol, tDouble sViewHeight, tDouble sViewWidth, tDouble sDiffY, tDouble sDiffX, tString sSheet, tBool sCss = false);

        /// @brief      Floating objects render payload for a target sheet.
        tString _JsonFloatingObjectsForSheet(tString sTargetSheetName, tString sSheet);
        tString _JsonFloatingObjects(tString sSheet);

        /// @brief      Return the JSON right justify for the specified column
        /// @param[in]  sCol tIndex column
        /// @param[in]  sViewWidth tDouble view width
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonRightJustify(tIndex sCol, tDouble sViewWidth, tString sSheet);

        /// @brief      Return the JSON bottom justify for the specified row
        /// @param[in]  sRow tIndex row
        /// @param[in]  sViewHeight tDouble view height
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonBottomJustify(tIndex sRow, tDouble sViewHeight, tString sSheet);
        
        /// @brief      Return the JSON column by pixel for the specified column start
        /// @param[in]  sColStart tIndex column start
        /// @param[in]  sPixel tDouble pixel value
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonColByPixel(tIndex sColStart, tDouble sPixel, tString sSheet);
        
        /// @brief      Return the JSON row by pixel for the specified row start
        /// @param[in]  sRowStart tIndex row start
        /// @param[in]  sPixel tDouble pixel value
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonRowByPixel(tIndex sRowStart, tDouble sPixel, tString sSheet);
        
        /// @brief      Return the JSON bottom right
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonBottomRight(tString sSheet);
        
        /// @brief      Return the JSON pixel bottom right
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonPixelBottomRight(tString sSheet);
        
        /// @brief      Return the JSON Named Range
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _JsonRangeNamed();
        
        /// @brief      Return JSON list of formula named
        /// @return     tString
        tString _JsonFormulaNamed();

        /// @brief      Sparse JSON of merged @ref tPrintParameters (workbook layout + this sheet's orientation / FitToPage).
        /// @param[in]  sSheet  sheet name (empty = keep current active sheet)
        /// @return     JSON object string, or empty on failure
        tString _JsonPrintParameters(tString sSheet);

        /// @brief      Apply print parameters JSON to the sheet selected via @ref SetSheet.
        /// @param[in]  sJson   UTF-8 JSON object (same keys as @ref tPrintParameters::JsonWrite)
        /// @param[in]  sSheet  sheet name (empty = keep current active sheet)
        /// @return     false when the sheet is missing or the JSON is invalid
        tBool _SetJsonPrintParameters(tString sJson, tString sSheet);
        
         /// @brief      Return JSON list of tables.
        /// @return     tString
        tString _JsonRangeData();

        /// @brief      Distinct formatted values in a sheet range (autofilter value list).
        /// @param[in]  sRef   A1-style range ref
        /// @param[in]  sSheet sheet name (empty = active sheet)
        /// @return     JSON {"values":[...]} or empty on failure
        tString _JsonFindUniqueValue(tString sRef, tString sSheet);

        /// @brief      Find cells whose display text matches @p sSearch on the active sheet.
        /// @param[in]  sSearch           needle
        /// @param[in]  sMatchCase        Excel "Match case"
        /// @param[in]  sMatchEntireCell  Excel "Match entire cell contents"
        /// @param[in]  sSheet            sheet name (empty = active sheet)
        /// @return     JSON {"cells":["A1",...]} or empty on failure
        tString _JsonFindCell(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell, tString sSheet);

        /// @brief      Apply RangeData JSON (sort + filter) on a named table range.
        /// @param[in]  sName     named range / table name
        /// @param[in]  sJsonData RangeData JSON payload
        /// @param[in]  sSheet    sheet name (empty = active sheet)
        /// @return     tBool
        tBool _UndoApplyRangeData(tString sName, tString sJsonData, tString sSheet);

        /// @brief      Attach RangeData to a named range (e.g. promote Excel autofilter).
        /// @param[in]  sName     named range name
        /// @param[in]  sRef      A1-style range ref
        /// @param[in]  sJsonData RangeData JSON (empty: derive columns from header row)
        /// @param[in]  sSheet    sheet name (empty = active sheet)
        /// @return     tBool
        tBool _UndoAddRangeData(tString sName, tString sRef, tString sJsonData, tString sSheet);
        
        // TreeView ===========================================
        // TreeView ===========================================================
        /// @brief     Switch Open Clode Node Row
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _OpenCloseTreeRow(tIndex sRow, tString sSheet);
 
        /// @brief     Switch Open Clode Node Row
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _OpenCloseTreeCol(tIndex sRow, tString sSheet);
 
        /// @brief     Change Tree Row
        /// @param[in]  sRight tBool
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to change
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ChangeTreeRow(tBool sRight,tIndex sRow,tIndex sSize, tString sSheet);
        
        /// @brief     Change Tree Col
        /// @param[in]  sRight tBool
        /// @param[in]  sCol  tIndex Index row
        /// @param[in]  sSize tIndex number row to change
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ChangeTreeCol(tBool sRight,tIndex sCol,tIndex sSize, tString sSheet);
        
        /// @brief     Freeze panes / split view via @ref tUndoSplitView.
        /// @param[in]  sCde      1 = vertical (SplitV), 2 = horizontal (SplitH), 3 = clear both
        /// @param[in]  sPosition column index (cde 1) or row index (cde 2); ignored when clearing
        /// @param[in]  sSheet    sheet name (empty = active sheet)
        /// @return     tBool
        tBool _SplitView(tByte sCde, tIndex sPosition, tString sSheet);

        /// @brief     Freeze column boundary from @ref tSheet::SplitV (>=1 frozen, -1 none).
        /// @param[in]  sSheet    sheet name (empty = active sheet)
        /// @return     SplitV index or -1 if unset / invalid sheet
        tIndex _SplitFreezeCol(tString sSheet);

        /// @brief     Freeze row boundary from @ref tSheet::SplitH (>=1 frozen, -1 none).
        /// @param[in]  sSheet    sheet name (empty = active sheet)
        /// @return     SplitH index or -1 if unset / invalid sheet
        tIndex _SplitFreezeRow(tString sSheet);
        
        // Class ==============================================================
        /// @brief      Register a class attribute
        /// @param[in]  sClassName tString class name
        /// @param[in]  sLabel tString label
        /// @param[in]  sFamily tString family
        /// @return     tBool
        tBool _RegisterClassAttribute(tString sClassName, tString sLabel,tString sFamily);
        
        /// @brief      Add a property to a class
        /// @param[in]  sName tString property name
        /// @param[in]  sType tString property type
        /// @param[in]  sLabel tString property label
        /// @param[in]  sOrder tSize property order
        /// @param[in]  sDefaultValue tString default value of the property
        /// @return     tBool
        tBool _AddProperty(tString sName, tString sType, tString sLabel, tSize sOrder, tString sDefaultValue, tString sKind = "");
        
        /// @brief      Set the class of the cell at the reference sRef
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sClassName tString class name
        /// @return     tBool
        tBool _CellClass(tString sRef, tString sClassName);
        
        /// @brief      Return the JSON cell class
        /// @return     tString
        tString _JsonCellClass();
   
        /// @brief      Return the JSON cell class by name
        /// @param[in]  sClassName tString name of the class
        /// @return     tString JSON representation of the cell class
        tString _JsonCellClassByName(tString sClassName);

        /// @brief      Apply a unit to the cell
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sFamily tString family of the unit
        /// @param[in]  sUnit tString unit to apply
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _ApplyUnit(tString sRef, tString sFamily, tString sUnit, tString sSheet);

        /// @brief      Return next position of cell.
        /// @param[in]  sRow tInt
        /// @param[in]  sCol tInt
        /// @param[in]  sKey  tByte
        /// @param[in]  sMeta  tByte
        /// @param[in]  sTop  tInt
        /// @param[in]  sLeft  tInt
        /// @param[in]  sBottom  tInt
        /// @param[in]  sRight  tint
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _MoveCell(tInt sRow, tInt sCol, tByte sKey,tByte sMeta, tInt sTop,tInt sLeft, tInt sBottom, tInt sRight, tString sSheet);

        /// @brief      Move to a next no empy cell in a specified direction
        /// @param[in]  sRow tInt row of the cell
        /// @param[in]  sCol tInt column of the cell
        /// @param[in]  sDirection tInt direction to move the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tString
        tString _MoveToCell(tInt sRow, tInt sCol, tInt sDirection, tString sSheet);

        // Pixel =============================================================
        /// @brief      Sum the pixel height of rows in a range
        /// @param[in]  sRowStart tIndex start row
        /// @param[in]  sRowEnd tIndex end row
        /// @param[in]  sSheet tString sheet name
        /// @return     tDouble total pixel height
        tDouble _SumPixelHeight(tIndex sRowStart, tIndex sRowEnd, tString sSheet);

        /// @brief      Sum the pixel width of columns in a range
        /// @param[in]  sColStart tIndex start column
        /// @param[in]  sColEnd tIndex end column
        /// @param[in]  sSheet tString sheet name
        /// @return     tDouble total pixel width
        tDouble _SumPixelWidth(tIndex sColStart, tIndex sColEnd, tString sSheet);
            
        // Tools ==============================================================
        
        /// @brief      Convert a base-10 integer to an alphanumeric string
        /// @param[in]  sValue tInt value to convert
        /// @return     tString alphanumeric representation of the value
        tString _Base10toAlpha(tInt sValue);

        /// @brief      Convert an alphanumeric string to a base-10 integer
        /// @param[in]  sValue tString value to convert
        /// @return     tInt base-10 representation of the value
        tInt _AlphaToBase10(tString sValue);

        /// @brief      Parse one cell ref (A1, $B$2) via lexer → JSON {"ok":true,"row":..,"col":..} or {"ok":false}.
        tString _ParseCell(tString sRef);

        /// @brief      Parse one range ref (A1:B2) via lexer → JSON bounds or {"ok":false}.
        tString _ParseRange(tString sRef);

        /// @brief      Prefix unqualified A1 refs with sSheet (Lexer-based, formulas supported).
        tString _QualifyRefsForSheet(tString sSheet, tString sText);

        /// @brief      Remove sSheet! from refs on sSheet; other sheets unchanged.
        tString _StripTargetSheetFromRefs(tString sSheet, tString sText);

        /// @brief      Collect formula refs (cells/ranges/tables/names) for in-formula highlighting.
        tString _CollectFormulaRefs(tString sText, tString sSheet, tIndex sRow, tIndex sCol);

        /// @brief      Get the maximum column index
        /// @return     tIndex maximum column index
        tIndex _MaxCol();

        /// @brief      Get the maximum row index
        /// @return     tIndex maximum row index
        tIndex _MaxRow();

        // Internal ===========================================================

        /// @brief      Ensure the cell exists
        /// @param[in]  sRef tString reference of the cell
        /// @param[in]  sSheet tString sheet name
        /// @return     tBool
        tBool _EnsureCell(tString sRef, tString sSheet);

        /// @brief      Set the language
        /// @param[in]  sLang tString language to set
        void _SetLang(tString sLang);
        
        
        tBool _AddFunction(tString sName,tString sLabel,tString sFamily, tInt sNbArg);


        /// @brief      Call a function with JSON parameters
        /// @param[in]  sFunctionName tString name of the function to call
        /// @param[in]  sJsonParams tString JSON string containing the parameters
        /// @return     tString JSON string containing the result or error
        tString _Call(tString sFunctionName, tString sJsonParams);
        
        /// Message Chat WEB ==================================================
        /// @brief PostMessage to Serveur
        /// @param[in] sMessage tString
        /// @return tBool
        tBool PostMessage(tString sMessage) override;
         
        /// @brief GetMessage from  Serveur
        /// @param[in] sMessage tString
        /// @return tBool
        tBool GetMessage(tString sMessage) override;
        
        // Gen Demo ===========================================================

        /// @brief      Apply pressure to a dynamic row and column
        /// @param[in]  sDynamicRow tInt dynamic row
        /// @param[in]  sDynamicCol tInt dynamic column
        /// @param[in]  sSheet tString sheet name
        void _Pressure(tInt sDynamicRow, tInt sDynamicCol, tString sSheet);

        /// @brief      Cooperative (non-blocking) _Pressure generation, driven step-by-step
        ///             from JS so the UI can show a live progress %. Only the generation phase
        ///             yields; _EndPressureCooperative runs the blocking JsonEnd finalize.
        /// @param[in]  sDynamicRow tInt target row count
        /// @param[in]  sDynamicCol tInt target column count
        /// @param[in]  sSheet tString sheet name
        void _BeginPressureCooperative(tInt sDynamicRow, tInt sDynamicCol, tString sSheet);
        /// @brief      Generate up to sMaxRows rows; returns true once the whole grid is queued.
        tBool _StepPressureCooperative(tInt sMaxRows);
        /// @brief      Generation progress 0..100 (JsonEnd finalize is separate).
        tInt _PressureCooperativeProgress();
        /// @brief      True while a cooperative generation is in progress.
        tBool _IsPressureCooperativeActive();
        /// @brief      Run the blocking batch compile + cold calculate, then log stats.
        void _EndPressureCooperative();
	};

    void ShowParseErrorJson(Document* sDocJson,tString sJsonStr);
    
    tString StringForJson(tString result);
    
    tString JsonBoolResult(bool result);

    tString JsonStringResult(tString result);

    tString JsonObjectResult(tString result);

    tString JsonDoubleResult(tString result);
}



#endif
