//=============================================================================
// SkSpreadSheet Api
//=============================================================================
/*! \mainpage SkSpreadSheet
 *
 * \section intro_sec Introduction
 *
 * This Spreadsheet has been designed to offer a good compromise between memory occupancy and speed..
 *
 * \section composition_sec Composition
 *
 * \subsection tItem tItem (see tItem.hpp).
 * The tItem is the ancestor of tCell and tRange.
 * It is hung on the sheet by tColRow elements. 
 * These elements know their position only by the index of the tColRow.
 * So when inserting rows or columns the response times are excellent.
 * The system reconstitutes the formula of the cell thanks to the dependent elements and their tColRow (index)
 * \subsection tColRow tColRow (see tColRow.hpp).
 * tColRow is either a row or a column.
 * It links a cell or a row to a sheet (via tColRowCellRange).
 * These it which contains the index number of row or column.
 * \subsection tCell tCell (see tCell.hpp).
 * The cell is the basic building block of the spreadsheet. It is derived from tItem.
 * The cell contains a value and a calculation formula.
 * It has all the dependent cells in a vector.
 * \subsection tRange tRange (see tRange.hpp).
 * The range is used to functions. Sum(), to example. 
 * She targets a cell range. 
 * It has a vector on all cells that have a formula using it.
 * \subsection tSheet  tSheet (see tSheet.hpp).
 * The sheet contains, the tCells, the tRange and the tColRow, via tColRowCellRange.
 * \subsection tColRowCellRange tColRowCellRange (see tColRowCellRange.hpp)
 * The tColRowCellRange contains, the tCells, the tRanges and the tColRows.
 * \subsection SkLexer SkLexer (see SkLexer.hpp).
 * Cutting text instructions into token.
 * \subsection SkLemonToken SkLemonToken (see SkLemonToken.hpp).
 * Token to lemon.
 * \subsection SkLemonReserved SkLemonReserved (see SkLemonReserved.hpp).
 * Used to save reserved words and functions in dictionaries.
 * \subsection SkLemonInterface  SkLemonInterface (see SkLemonInterface.hpp).
 * Interface between SkLemonSpreadSheet.y and Skeepto.
 * \subsection SkLemonSpreadSheet  SkLemonSpreadSheet (see SkLemonSpreadSheet.y).
 * The parser to the spreadsheet in lemon syntax.
 * \subsection tSharedFormula tSharedFormula (tSharedFormula.hpp).
 * Allows you to group all identical formulas into one (pooling).
 * \subsection SkFunction SkFunction (SkFunction.hpp).
 * Functions to the spreadsheet. This module contains the function dictionaries. As well as the ancestor classes.
 * \subsection SkWorkBook  SkWorkBook (see SkWorkBook.hpp).
 * Onwer Workbook
 * \subsection SkUndoRedo  SkUndoRedo (see SkUndoRedo.hpp).
 * Undo Redo.
 * \subsection SkUndoRedoSave  SkUndoRedoSave (see SkUndoRedoSave.hpp).
 * Used to keep old values i, undo redo..
 * \subsection SkApi  SkApi (see SkApi.hpp).
 * Application program interface.
 * \subsection SkTools  SkTools (see SkTools.hpp).
 * Tools contains functions Base10ToAlpha and AlphaToBase10.
  */

//=============================================================================
#ifndef SkApi_hpp
#define SkApi_hpp

#include "SkSpreadSheet.hpp"
#include "SkLemonInterface.hpp"
#include "SkLemonSpreadSheet.hpp"
#include "SkSharedFormula.hpp"
#include "SkUndoRedoSp.hpp"
#include "SkCopyPaste.hpp"
#include "SkJsonView.hpp"
#include "SkConditionalFormat.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {
    //=========================================================================
    //! Low level API with Spreadsheet 
    class tApi : tClass {
    protected:
        //! @brief Mode
        tMode m_Mode;

        //! @brief Multi-user collaborative flag.
        //!
        //! When true, the local user is not the sole connected author of the
        //! workbook. Any operation that would textually rewrite an identifier
        //! already embedded in formulas (sheet rename, named-range rename,
        //! etc.) is refused in that state, because a safe rebase of those
        //! names on remote peers is not implemented. The flag defaults to
        //! false for standalone tApi instances; tInterfaceWeb overrides the
        //! virtual getter to compute it from the dispatcher.
        tBool m_MultiUserActive;

        //! Container
        tSpreadSheetContainer*		m_SpreadSheetContainer;
        //! Application
        tApplication*				m_Application;
        
        //! Interafce JsonView
        tJsonView                    m_JsonView;
        //! Interface default Sheet
        /// @brief      Get current sheet.
        /// @param[out] sSheet tSheet**
        void NormalizeSheet(tSheet** sSheet);
        tColRow::tContainerRange::tResult	m_VectorResult;
    protected:
        tCellModelClassAttribute*   m_LastModelClassAttribute;
    public:
        /// @brief      Constructor for SkApi.
        tApi();

        /// @brief      Destructor for SkApi.
        virtual ~tApi();
        
        // Mode 
        /// @brief        Set alone mode.
        /// @param[in]    sAlone tBool
        void Alone(tBool sAlone);

        /// @brief        Get alone mode status.
        /// @return        tBool
        tBool Alone();
 
        /// @brief Set undo active status.
        /// @param[in] sIsUndoActif tBool
        void IsUndoActif(tBool sIsUndoActif);
        
        /// @brief        Get undo active status.
        /// Default is false: commands apply but are not recorded on the undo stack.
        /// @return       tBool
        tBool IsUndoActif();
        
        /// @brief Set JSON mode status.
        /// @param[in] sJson tBool
        void IsJson(tBool sJson);
        
        /// @brief        Get JSON mode status.
        /// @return       tBool
        tBool IsJson();

        /// @brief        Set client mode.
        /// @param[in]    sClient tBool
        void Client(tBool sClient);

        /// @brief        Get client mode status.
        /// @return        tBool
        tBool Client();
        
        /// @brief        Set server mode.
        /// @param[in]    sServer tBool
        void Server(tBool sServer);

        /// @brief        Get server mode status.
        /// @return        tBool
        tBool Server();

        //! @brief Declare whether multiple users are currently collaborating.
        //!
        //! Called from JS (embind) or from tests to toggle the "multi-user"
        //! state when no native dispatcher is available to compute it
        //! automatically. tInterfaceWeb::MultiUserActive() may ignore this
        //! value and derive the real state from its dispatcher.
        //! @param[in] sActive tBool
        void MultiUserActive(tBool sActive);

        //! @brief Multi-user collaborative mode getter.
        //!
        //! Default implementation returns the stored m_MultiUserActive flag.
        //! Override in subclasses (e.g. tInterfaceWeb) to derive the state
        //! from the real connection topology.
        //! @return tBool true when multiple authors are connected.
        virtual tBool MultiUserActive();

        //! @brief Tell the caller whether identifier renames are safe.
        //!
        //! A rename of a sheet name or of a named range cannot be propagated
        //! by the undo system when other users are editing the workbook,
        //! because the undo stream stores sheet / range names by reference
        //! and cannot rebase those strings remotely. This helper gives the
        //! UI a single place to check before enabling a rename control.
        //! @return tBool true in local mode (Client()==false) or when the
        //!         user is the only connected author.
        tBool IsRenameAllowed();


        // Interface Undo =====================================================
        /// @brief      Execute undo operation.
        /// @param[in]	sUndo tUndo*
        /// @return		tBool
        virtual tBool Do(tUndo* sUndo);

        /// @brief      Undo last operation.
        /// @return     tBool
        virtual tBool Undo();

        /// @brief      Redo last undone operation.
        /// @return     tBool
        virtual tBool Redo();

        /// @brief      Get last undo operation.
        /// @return     tUndo*
        virtual tUndo* LastUndo();

        /// @brief      Get last redo operation.
        /// @return     tUndo*
        virtual tUndo* LastRedo();
        
        /// @brief      Clear All Undo Redo of AllSheet
        virtual void ClearUndoRedo();
        
        // Interface Undo Redo ================================================
        /// @brief      Undo reset cell operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sKeepFormat tBool true to clear content only (keep CSS format)
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoRaz(tString sRef, tBool sKeepFormat = false, tSheet* sSheet = nullptr);

        /// @brief      Undo clear-format operation: remove CSS format only, keep cell
        ///             content, formula and class attributes.
        /// @param[in]  sRef tString like A1 or A1:B2
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoRazFormat(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Undo cell format operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sFormat tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellFormat(tString sRef, tString sFormat, tSheet* sSheet = nullptr);

        /// @brief      Undo cell format operation using tRect (Row, Col, StrRef) and sheet.
        /// @param[in]  sRect tRect range (use RectRow/RectCol/StrRef; sheet passed separately)
        /// @param[in]  sFormat tString CSS-style format (e.g. background-color, color, font)
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellFormat(tRect sRect, tString sFormat, tSheet* sSheet = nullptr);
        
        
        /// @brief      Undo cell precision operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sInc tBool true inc, false dec
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellPrecision(tString sRef,tBool sInc,tSheet* sSheet = nullptr);
        
        /// @brief      Undo cell border operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sBorder tShort
        /// @param[in]  sFormat tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellBorder(tString sRef, tShort sBorder, tString sFormat, tSheet* sSheet = nullptr);

        /// @brief      Undo cell value operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sValue tVariant Value
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool UndoCellValue(tString sRef, tVariant sValue, tSheet* sSheet = nullptr);

        /// @brief      Undo cell value operation by row and column.
        /// @param[in]  sRow tIndex row
        /// @param[in]  sCol tIndex col
        /// @param[in]  sValue tVariant Value
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellValue(tIndex sRow, tIndex sCol, tVariant sValue, tSheet* sSheet = nullptr);

        /// @brief      Excel-like fill-handle series as a single undo.
        ///             Reads the seed cells in @p sSourceRef, extrapolates the
        ///             series (numbers / months / weekdays / "prefix+number" /
        ///             shifted formulas) into @p sDestRef and applies it through
        ///             one tUndoFillSeries. Orientation/direction are inferred
        ///             from the geometry (single-axis extension).
        /// @param[in]  sSourceRef seed range (original selection, e.g. A1:A2)
        /// @param[in]  sDestRef   extension range (cells to fill, e.g. A3:A12)
        /// @param[in]  sSheet     sheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool UndoFillSeries(tString sSourceRef, tString sDestRef, tSheet* sSheet = nullptr);

        /// @brief      Undo cell class operation.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sValue tVariant
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellClass(tString sRef, tVariant* sValue, tSheet* sSheet = nullptr);

        /// @brief      Undo cell class operation.
        /// @param[in]  sRow tIndex row
        /// @param[in]  sCol tIndex col
        /// @param[in]  sValue tVariant
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellClass(tIndex sRow, tIndex sCol, tVariant* sValue, tSheet* sSheet = nullptr);

        /// @brief      Undo Ensure cell in active sheet or in sSheet to sRef set Class with ClassName.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sClassName tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellClass(tString sRef, tString sClassName, tSheet* sSheet = nullptr);
        
        /// @brief      Undo Ensure cell in active sheet or in sSheet to Row sRow and Column sCol set Class with ClassName.
        /// @param[in]  sRow tIndex row
        /// @param[in]  sCol tIndex col
        /// @param[in]  sClassName tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellClass(tIndex sRow, tIndex sCol, tString sClassName, tSheet* sSheet = nullptr);

        /// @brief      Undo Cell attribute if class exist with ref.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sAttribute tString
        /// @param[in]  sVariant tVariant
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellAttribute(tString sRef, tString sAttribute, tVariant sVariant, tSheet* sSheet = nullptr);

        /// @brief      Undo Cell attribute if class exist.
        /// @param[in]  sRow tIndex row
        /// @param[in]  sCol tIndex col
        /// @param[in]  sAttribute tString
        /// @param[in]  sVariant tVariant
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellAttribute(tIndex sRow, tIndex sCol, tString sAttribute, tVariant sVariant, tSheet* sSheet = nullptr);

        /// @brief      Set calculable scalar on a cell class (tCellClassAttribute::m_Value).
        /// @param[in]  sRef tString like A1
        /// @param[in]  sVariant tVariant
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoCellClassCalculable(tString sRef, tVariant sVariant, tSheet* sSheet = nullptr);
        
        /// @brief      Undo Cell attribute if class exist with ref.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sFamily tString (Length, Mass, Monet ...)
        /// @param[in]  sUnit tString
        /// @return     tBool
        tBool UndoApplyUnit(tString sRef, tString sFamily, tString sUnit);

        /// @brief      Create or delete Merged Range. Call twice to create and delete the range.
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoApplyMerge(tString sRef, tSheet* sSheet = nullptr);

        // Insert delete Row & Col
        /// @brief      Insert sSize rows at sRow position.
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to insert
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertRow(tIndex sRow, tIndex sSize, tSheet* sSheet = nullptr);
        
        // Insert delete Row & Col
        /// @brief     Insert after sRect
        /// @param[in]  sRect tRect
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertRowByRect(tRect sRect, tSheet* sSheet = nullptr);

        /// @brief Insert row by rect and write a label in the same undo entry.
        /// @param[in] sRect Insert rectangle (typically one row below a table)
        /// @param[in] sLabelRef A1 ref of the label cell (usually leftmost of that row)
        /// @param[in] sLabelValue Label text (e.g. "Total")
        /// @param[in] sSheet Sheet (nullptr = active)
        tBool UndoInsertRowByRectWithLabel(tRect sRect, tString sLabelRef, tString sLabelValue,
                                           tSheet* sSheet = nullptr);

        /// @brief      Insert sSize Column at sCol position.
        /// @param[in]  sCol tIndex Index column
        /// @param[in]  sSize tIndex number column to insert
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertCol(tIndex sCol, tIndex sSize, tSheet* sSheet = nullptr);

        /// @brief     Insert after sRect
        /// @param[in]  sRect tRect
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertColByRect(tRect sRect, tSheet* sSheet = nullptr);

        /// @brief      Delete sSize rows at sRow position.
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteRow(tIndex sRow, tIndex sSize, tSheet* sSheet = nullptr);
        
        /// @brief      Delete at sRect 
        /// @param[in]  sRect tRect
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteRowByRect(tRect sRect, tSheet* sSheet = nullptr);


        /// @brief      Delete sSize Column at sCol position.
        /// @param[in]  sCol tIndex Index column
        /// @param[in]  sSize tIndex number column to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteCol(tIndex sCol, tIndex sSize, tSheet* sSheet = nullptr);

        /// @brief     Delete sRect
        /// @param[in]  sRect tRect
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteColByRect(tRect sRect, tSheet* sSheet = nullptr);

        
        /// @brief      Switch open/close tree node for row
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoOpenCloseTreeRow(tIndex sRow,tSheet* sSheet = nullptr);
 
        /// @brief      Switch open/close tree node for column
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoOpenCloseTreeCol(tIndex sRow,tSheet* sSheet = nullptr);
 
        /// @brief      Change tree row structure
        /// @param[in]  sRight tBool
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoChangeTreeRow(tBool sRight,tIndex sRow,tIndex sSize,tSheet* sSheet = nullptr);
        
        /// @brief      Change tree column structure
        /// @param[in]  sRight tBool
        /// @param[in]  sCol  tIndex Index row
        /// @param[in]  sSize tIndex number row to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool UndoChangeTreeCol(tBool sRight,tIndex sCol,tIndex sSize,tSheet* sSheet = nullptr);

        /// @brief      Freeze-pane / split view (see @ref tUndoSplitView).
        /// @param[in]  sCde      1 = vertical (SplitV), 2 = horizontal (SplitH), 3 = clear
        /// @param[in]  sPosition freeze column index (cde 1) or row index (cde 2); ignored when clearing
        /// @param[in]  sSheet    target sheet (nullptr = active sheet)
        /// @return     tBool
        tBool UndoSplitView(tByte sCde, tIndex sPosition, tSheet* sSheet = nullptr);
        
        /// @brief      Add sheet operation.
        /// @param[in]  sName tString
        /// @param[in]  sSheetLeft tString (if empty insert at end)
        /// @return     tBool
        tBool UndoAddSheet(tString sName, tString sSheetLeft = "");
        
        /// @brief      Rename sheet operation.
        /// @param[in]  sName tString
        /// @param[in]  sNewName tString
        /// @return     tBool
        tBool UndoRenameSheet(tString sName, tString sNewName);
        
        /// @brief      Swap or move sheets (insert-after when sInsertAfter is true).
        /// @param[in]  sName1 tString
        /// @param[in]  sName2 tString swap partner, or anchor sheet to insert after
        /// @param[in]  sInsertAfter tBool move sName1 after sName2 instead of swapping
        /// @return     tBool
        tBool UndoSwapSheet(tString sName1, tString sName2, tBool sInsertAfter = false);
        
        /// @brief      Delete sheet operation.
        /// @param[in]  sName tString
        /// @return     tBool
        tBool UndoDeleteSheet(tString sName);

        /// @brief      Add named range via @ref tUndoAddRangeNamed (undo stack entry).
        /// @param[in]  sName tString
        /// @param[in]  sRef  tString Like A1:A3
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoAddRangeNamed(tString sName, tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Same as UndoAddRangeNamed (historical name).
        /// @param[in]  sName tString
        /// @param[in]  sRef  tString Like A1:A3
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertRangeNamed(tString sName, tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Add named range with RangeData via @ref tUndoAddRangeData.
        /// @param[in]  sName tString
        /// @param[in]  sRef tString like A1:A3
        /// @param[in]  sJsonData tString RangeData JSON (empty: derive from first row of @p sRef)
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoAddRangeData(tString sName, tString sRef, tString sJsonData,
                               tSheet* sSheet = nullptr);

        /// @brief      Same as UndoAddRangeData (historical name).
        /// @param[in]  sName tString
        /// @param[in]  sRef tString like A1:A3
        /// @param[in]  sJsonData tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertRangeData(tString sName, tString sRef, tString sJsonData,
                                  tSheet* sSheet = nullptr);

        /// @brief      Apply RangeData JSON to an existing named range via @ref tUndoApplyRangeData.
        /// @param[in]  sName tString Named range that must already exist
        /// @param[in]  sJsonData tString RangeData JSON
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoApplyRangeData(tString sName, tString sJsonData, tSheet* sSheet = nullptr);

        /// @brief      Delete named range via @ref tUndoDeleteRangeNamed.
        /// @param[in]  sName tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteRangeNamed(tString sName, tSheet* sSheet = nullptr);

        /// @brief      Atomically rename and/or change the selection of an
        ///             existing named range. Areas that are preserved across
        ///             the edit keep their tRange* (and therefore their
        ///             tAllocatorRef), so formulas pointing at the old name
        ///             keep evaluating and will automatically render under
        ///             the new name.
        /// @param[in]  sOldName current name (must exist)
        /// @param[in]  sNewName target name (may equal sOldName when only
        ///                      changing the selection). Renaming fails if
        ///                      another named range already uses sNewName.
        /// @param[in]  sNewRef  new selection string (e.g. "A1:A2;B1:B2").
        ///                      Multi-area is supported but must stay on a
        ///                      single sheet.
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool false if the operation could not be applied
        ///             (missing source, conflicting target, empty ref...)
        tBool UndoUpdateRangeNamed(tString sOldName, tString sNewName,
                                   tString sNewRef, tSheet* sSheet = nullptr);
        
        /// @brief      Undo insert formula named operation.
        /// @param[in]  sName tString
        /// @param[in]  sFormula tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoInsertFormulaNamed(tString sName,tString sFormula,tSheet* sSheet = nullptr);

        /// @brief      Undo insert floating object (registry + host cell class on _$$A).
        /// @param[in]  sName tString floating object name
        /// @param[in]  sClassName tString cell class on host cell
        /// @param[in]  sTargetSheetName tString sheet where the object is displayed
        /// @param[in]  sSheet tSheet* rebase context (if nullptr use _$$A)
        /// @param[in]  sDiffX tDouble optional layout offset X (with sWidth/sHeight/sAnchorCellRef)
        /// @param[in]  sDiffY tDouble optional layout offset Y
        /// @param[in]  sWidth tDouble optional layout width (>0)
        /// @param[in]  sHeight tDouble optional layout height (>0)
        /// @param[in]  sOpacity tDouble optional layout opacity (>0)
        /// @param[in]  sAnchorCellRef tString optional anchor ref (e.g. "Sheet1!B5")
        /// @return     tBool false if the operation could not be applied
        tBool UndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName,
                                       tSheet* sSheet = nullptr,
                                       tDouble sDiffX = 0, tDouble sDiffY = 0,
                                       tDouble sWidth = 0, tDouble sHeight = 0, tDouble sOpacity = 0,
                                       tString sAnchorCellRef = "");

        /// @brief      Undo delete floating object.
        /// @param[in]  sName tString floating object name
        /// @param[in]  sSheet tSheet* rebase context (if nullptr use _$$A)
        /// @return     tBool false if the operation could not be applied
        tBool UndoDeleteFloatingObject(tString sName, tSheet* sSheet = nullptr);

        /// @brief      Undo floating object layout change (anchor, offset, size, opacity).
        /// @param[in]  sName tString floating object name
        /// @param[in]  sDiffX tDouble offset from anchor (X)
        /// @param[in]  sDiffY tDouble offset from anchor (Y)
        /// @param[in]  sWidth tDouble object width
        /// @param[in]  sHeight tDouble object height
        /// @param[in]  sOpacity tDouble opacity (0..1)
        /// @param[in]  sAnchorCellRef tString optional anchor ref (e.g. "Sheet1!B5")
        /// @param[in]  sSheet tSheet* rebase context (if nullptr use target sheet)
        /// @return     tBool false if the operation could not be applied
        tBool UndoFloatingObjectLayout(tString sName, tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight, tDouble sOpacity, tString sAnchorCellRef = "", tSheet* sSheet = nullptr);

        /// @brief      Bring floating object to front on its target sheet (undoable z-index bump).
        tBool UndoFloatingObjectBringToFront(tString sName, tSheet* sSheet = nullptr);

        /// @brief      Undo cell-class attribute on floating object host cell (_$$A).
        /// @param[in]  sName tString floating object name
        /// @param[in]  sAttribute tString property name on tCellClassAttribute
        /// @param[in]  sVariant tVariant new value or formula
        /// @return     tBool false if object/host sheet is missing
        tBool UndoFloatingObjectAttribute(tString sName, tString sAttribute, tVariant sVariant);

        /// @brief      Undo multiple cell-class attributes on one host cell (single undo step).
        /// @param[in]  sRef tString like A1
        /// @param[in]  sAttributes batch of attribute name/value pairs
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool false if validation fails or the operation could not be applied
        tBool UndoCellClassAttributes(tString sRef, const tVectorCellAttributeWire& sAttributes, tSheet* sSheet = nullptr);

        /// @brief      Undo multiple cell-class attributes on floating object host cell (_$$A).
        /// @param[in]  sName tString floating object name
        /// @param[in]  sAttributes batch of attribute name/value pairs
        /// @return     tBool false if validation fails or object/host sheet is missing
        tBool UndoFloatingObjectAttributes(tString sName, const tVectorCellAttributeWire& sAttributes);

        /// @brief      Undo delete formula named operation.
        /// @param[in]  sName tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteFormulaNamed(tString sName, tSheet* sSheet = nullptr);

        /// @brief      Undo apply JSON payload operation.
        /// @param[in]  sRef tString
        /// @param[in]  sJsonPayload tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoJsonPayload(tString sRef, tString sJsonPayload, tSheet* sSheet = nullptr);


        /// @brief      Copy operation.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool Copy(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Paste operation.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool UndoPaste(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Cut selection (copy to clipboard + clear source with undo).
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool UndoCut(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Move selection (copy buffer, clear source, paste at dest).
        /// @param[in]  sSourceRef tString
        /// @param[in]  sDestRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        virtual tBool UndoMove(tString sSourceRef, tString sDestRef, tSheet* sSheet = nullptr);
        
        // Conditional Format ==================================================
        /// @brief      Add conditional formatting to a cell range (like Excel).
        /// @param[in]  sType tString Type of conditional formatting:
        ///                         - "ColorScales": Color gradient based on cell values
        ///                         - "IconSets": Icons based on cell values (3 or 5 icons)
        ///                         - "DataBars": Horizontal bars in cells
        ///                         - "HighlightCellsRules": Rules-based highlighting
        ///                         - "CustomFormulas": Custom formula-based formatting
        /// @param[in]  sRef tString Cell range reference (e.g., "A1:A10")
        /// @param[in]  sParam1 tString Parameter 1 (usage depends on type):
        ///                         - ColorScales: min_color (#RRGGBB)
        ///                         - IconSets: first icon (emoji)
        ///                         - DataBars: min_value
        ///                         - HighlightCellsRules: condition
        ///                         - CustomFormulas: formula (% = cell value)
        /// @param[in]  sParam2 tString Parameter 2 (usage depends on type):
        ///                         - ColorScales: mid_color (#RRGGBB) or empty for 2-color
        ///                         - IconSets: second icon (emoji)
        ///                         - DataBars: max_value
        ///                         - HighlightCellsRules: additional parameter
        ///                         - CustomFormulas: format_if_true (CSS)
        /// @param[in]  sParam3 tString Parameter 3 (usage depends on type):
        ///                         - ColorScales: max_color (#RRGGBB)
        ///                         - IconSets: third icon (emoji)
        ///                         - DataBars: bar_color (#RRGGBB)
        ///                         - HighlightCellsRules: additional parameter
        ///                         - CustomFormulas: format_if_false (CSS)
        /// @param[in]  sParam4 tString Parameter 4 (usage depends on type):
        ///                         - ColorScales: empty (auto-calculated)
        ///                         - IconSets: threshold1 (3-icon mode) or fourth icon (5-icon mode)
        ///                         - DataBars: background_color (#RRGGBB)
        ///                         - HighlightCellsRules: additional parameter
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam5 tString Parameter 5 (usage depends on type):
        ///                         - ColorScales: empty (auto-calculated)
        ///                         - IconSets: threshold2 (3-icon mode) or fifth icon (5-icon mode)
        ///                         - DataBars: additional parameter
        ///                         - HighlightCellsRules: additional parameter
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam6 tString Parameter 6 (usage depends on type):
        ///                         - ColorScales: empty (auto-calculated)
        ///                         - IconSets: threshold3 (3-icon mode) or threshold1 (5-icon mode)
        ///                         - DataBars: additional parameter
        ///                         - HighlightCellsRules: additional parameter
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam7 tString Parameter 7 (usage depends on type):
        ///                         - ColorScales: empty
        ///                         - IconSets: empty (3-icon mode) or threshold2 (5-icon mode)
        ///                         - DataBars: empty
        ///                         - HighlightCellsRules: empty
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam8 tString Parameter 8 (usage depends on type):
        ///                         - ColorScales: empty
        ///                         - IconSets: empty (3-icon mode) or threshold3 (5-icon mode)
        ///                         - DataBars: empty
        ///                         - HighlightCellsRules: empty
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam9 tString Parameter 9 (usage depends on type):
        ///                         - ColorScales: empty
        ///                         - IconSets: empty (3-icon mode) or threshold4 (5-icon mode)
        ///                         - DataBars: empty
        ///                         - HighlightCellsRules: empty
        ///                         - CustomFormulas: empty
        /// @param[in]  sParam10 tString Parameter 10 (usage depends on type):
        ///                         - ColorScales: empty
        ///                         - IconSets: icon_type ("Flags", "Arrows", "Ratings", etc.)
        ///                         - DataBars: empty
        ///                         - HighlightCellsRules: empty
        ///                         - CustomFormulas: empty
        /// @param[in]  sSheet tSheet* Target sheet (nullptr = active sheet)
        /// @return     tBool true if successful, false otherwise
        /// @note       For detailed examples, see Documentation_UndoConditionalFormat.md
        tBool UndoConditionalFormat(tString sType,tString sRef,tString sParam1,tString sParam2,tString sParam3,tString sParam4,tString sParam5,tString sParam6,tString sParam7,tString sParam8,tString sParam9,tString sParam10, tSheet* sSheet=nullptr);
       
        // Delete Conditional Format ==================================================
        /// @brief      Delete conditional formatting operation.
        /// @param[in]  sType tString
        /// @param[in]  sRef tString Cell range reference (e.g., "A1:A10")
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoDeleteConditionalFormat(tString sType, tString sRef, tSheet* sSheet = nullptr);

        // Interface Direct ===================================================
        
        /// @brief      Set format API.
        /// @param[in]  sFormatApi tFormatApi*
        void FormatApi(tFormatApi* sFormatApi);

        /// @brief      Get format API.
        /// @return     tFormatApi*
        tFormatApi* FormatApi();

        /// @brief      Get format string parameter.
        /// @return     tString
        tString JsonFormatString();
        
        /// @brief      Return format string.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString CellFormatString(tString sRef, tSheet* sSheet = nullptr);
        
        /// @brief      Return input string.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString CellInputString(tString sRef, tSheet* sSheet = nullptr);
        
        
        /// @brief      Return cell JSON payload.
        /// @param[in]  sRef tString like A1
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString CellJsonPayload(tString sRef, tSheet* sSheet = nullptr);
        
        
        // Check
#ifdef checksp
        /// @brief      Check integrity.
        void Check();
#endif
#ifdef checkfo
        /// @brief      Check format.
        virtual void CheckFormat();
#endif

        // Interface Col Row ==================================================
        /// @brief      Get size of row.
        /// @param[in]  sIndex tIndex Index of row
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tDouble
        tDouble SizeRow(tIndex sIndex, tSheet* sSheet = nullptr);

        /// @brief      Get size of column.
        /// @param[in]  sIndex tIndex Index of col
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tDouble
        tDouble SizeCol(tIndex sIndex, tSheet* sSheet = nullptr);
        
       
        /// @brief      Set size of row range.
        /// @param[in]  sBegin tIndex Index of row begin
        /// @param[in]  sEnd tIndex Index of row end
        /// @param[in]  sSize tDouble in millimeter
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoSizeRow(tIndex sBegin, tIndex sEnd, tDouble sSize, tSheet* sSheet = nullptr);

        /// @brief      Set size of column range.
        /// @param[in]  sBegin tIndex Index of col begin
        /// @param[in]  sEnd tIndex Index of col end
        /// @param[in]  sSize tDouble in millimeter
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool UndoSizeCol(tIndex sBegin, tIndex sEnd, tDouble sSize, tSheet* sSheet = nullptr);
        
        /// @brief      Register class attribute.
        /// @param[in]  sClassName tString
        /// @param[in]  sLabel tString
        /// @param[in]  sFamily  tString
        /// @return     tBool
        tBool RegisterClassAttribute(tString sClassName, tString sLabel,tString sFamily);

        /// @brief      Add property to class.
        /// @param[in]  sName tString
        /// @param[in]  sType tString
        /// @param[in]  sLabel tString
        /// @param[in]  sOrder tSize
        /// @param[in]  sDefaultValue tString
        /// @return     tBool
        tBool AddProperty(tString sName, tString sType, tString sLabel, tSize sOrder, tString sDefaultValue, tString sKind = "");
        
        // Interface WorkBook =================================================
        tWorkBook* ActiveWorkBook();

        /// @brief      Set active workbook.
        /// @param[in]  sUri tString
        /// @return     tBool
        tBool ActiveWorkBook(tString sUri);
        
        /// @brief      Add workbook by URI.
        /// @param[in]  sWorkBookUri tString
        /// @return     tWorkBook*
        tWorkBook* AddWorkBook(tString sWorkBookUri);

        /// @brief      Create new workbook by URI.
        /// @param[in]  sWorkBookUri tString
        /// @return     tWorkBook*
        tWorkBook* NewWorkBook(tString sWorkBookUri);

        /// @brief      Set workbook information.
        /// @param[in]  sWorkBookInfo tString
        /// @return     tBool
        tBool WorkBookInfo(tString sWorkBookInfo);

        /// @brief      Get workbook information.
        /// @return     tString
        tString WorkBookInfo();
        
        /// @brief      Return JSON workbook.
        /// @param[in]  sUri tString URI of the workbook
        /// @return     tString
        tString JsonWorkBook(tString sUri);
        
        /// @brief      Return list of workbooks.
        /// @param[in]  sUri tString current URI of the workbook
        /// @param[in]  sUriTo tString new URI of the workbook
        /// @return     tBool
        tString JsonWorkBooks();

        /// @brief      Delete workbook by URI.
        /// @param[in]  sWorkBookUri tString
        /// @return     tBool
        tBool DeleteWorkBook(tString sWorkBookUri);
        
        /// @brief      Rename workbook by URI.
        /// @param[in]  sUri tString
        /// @param[in]  sUriTo tString
        /// @return     tBool
        tBool RenameWorkBook(tString sUri, tString sToUri);
        
        /// @brief      Get JSON string list URI of workbooks.
        /// @return     tString
        tString JsonWorkBooksList();
        
        /// @brief      Get workbook by URI.
        /// @param[in]  sWorkBookUri tString
        /// @return     tWorkBook*
        tWorkBook* WorkBook(tString sWorkBookUri);

        /// @brief      Write JSON workbook by URI.
        /// @param[in]  sWorkBookUri tString
        /// @return     tString
        tString WriteJson(tString sWorkBookUri);

        /// @brief      Read JSON.
        /// @param[in]  sJson tString
        /// @return     tBool
        tBool ReadJson(tString sJson);
        
        // Interface Sheet ====================================================
        /// @brief      Add sheet by name.
        /// @param[in]  sSheetName tString
        /// @return     tSheet*
        tSheet* AddSheet(tString sSheetName);
        
        /// @brief      Return sheet by name (returns nullptr if doesn't exist).
        /// @param[in]  sSheetName tString
        /// @return     tSheet*
        tSheet* Sheet(tString sSheetName);

        /// @brief      Delete sheet by name (returns false if doesn't exist).
        /// @param[in]  sSheetName tString
        /// @return     tBool
        tBool DeleteSheet(tString sSheetName);

        /// @brief      Set active sheet by name (returns nullptr if doesn't exist).
        /// @param[in]  sSheetName tString
        /// @return     tSheet*
        tSheet* ActiveSheet(tString sSheetName);

        /// @brief      Get active sheet.
        /// @return     tSheet*
        tSheet* ActiveSheet();

        /// @brief      Return string vector with all sheet names.
        /// @return     tVectorString
        tVectorString GetVectorOfSheet();

        /// @brief      Get size of row.
        /// @param[in]  sIndex tIndex Index of row
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tDouble
        tDouble UndoSizeRow(tIndex sIndex, tSheet* sSheet = nullptr);

        /// @brief      Get size of column.
        /// @param[in]  sIndex tIndex Index of col
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tDouble
        tDouble UndoSizeCol(tIndex sIndex, tSheet* sSheet = nullptr);

        /// @brief      Set cell value by row and column.
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sValue tVariant Value
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool CellValue(tIndex sRow, tIndex sCol, tVariant sValue, tSheet* sSheet = nullptr);

        /// @brief      Set cell value with reference.
        /// @param[in]  sRef tString
        /// @param[in]  sValue tVariant Value
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tBool
        tBool CellValue(tString sRef, tVariant sValue, tSheet* sSheet = nullptr);

        /// @brief      Get cell value by row and column.
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tVariant
        tVariant CellValue(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr);

        /// @brief      Get cell value by reference.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tVariant
        tVariant CellValue(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Calculable scalar (unwraps tCellUnit); same as CellValue then CalculableScalarFromVariant.
        tVariant CellCalculableScalarValue(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Get cell format by row and column.
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tString
        tString CellFormat(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr);

        /// @brief      Get cell format by reference.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tString
        tString CellFormat(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Format a numeric value using the resolved format of a cell (sheet/col/row/cell cascade).
        /// @param[in]  sRef tString cell reference (A1)
        /// @param[in]  sValue tDouble value to format
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tString formatted display text
        tString FormatValueWithCellFormat(tString sRef, tDouble sValue, tSheet* sSheet = nullptr);

        /// @brief      Get cell attribute value.
        /// @param[in]  sRef tString
        /// @param[in]  sAttribute tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tVariant
        tVariant CellAttributeValue(tString sRef, tString sAttribute, tSheet* sSheet = nullptr);

        /// @brief      Get cell attribute.
        /// @param[in]  sRef tString
        /// @param[in]  sAttribute tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tCellAttribute*
        tCellAttribute* CellAttribute(tString sRef, tString sAttribute, tSheet* sSheet = nullptr);

        /// @brief      Ensure cell by row and column.
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tCell*
        tCell* EnsureCell(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr);

        /// @brief      Ensure cell by reference.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tCell*
        tCell* EnsureCell(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Get cell by row and column (returns nullptr if doesn't exist).
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tCell*
        virtual tCell* Cell(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr);

        /// @brief      Get cell by reference (returns nullptr if doesn't exist).
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tCell*
        virtual tCell* Cell(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Compile cell in active sheet.
        /// @param[in]  sCell tCell* cell to compile
        /// @param[in]  sValue const tChar* code
        /// @return     tBool (true if Ok false if error)
        tBool CompilCell(tCell* sCell, const tChar* sValue);

        /// @brief      Get Lemon interface (for testing).
        /// @return     tLemonInterface*
        tLemonInterface* LemonInterface();

        /// @brief      Get formula by row and column.
        /// @param[in]  sRow tIndex Row
        /// @param[in]  sCol tIndex Column
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @param[in]  sUser when true, Excel bar form (e.g. [@[Col]] instead of Table[[#This Row][Col]])
        /// @return     tString
        tString Formula(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr, tBool sUser = false);

    
        /// @brief      Formula-pick reference relative to an edit anchor.
        ///             Same table data row → @[Col] / [@[Col1]:[Col2]].
        ///             Full data column(s) → [[Col]] (or Table[[Col]] if anchor is outside).
        ///             Otherwise returns the A1 selection string.
        /// @param[in]  sRefAnchor tString Reference of the cell being edited
        /// @param[in]  sRef tString Picked selection (A1 / A1:B2)
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @return     tString
        tString CellRef(tString sRefAnchor,tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Find range by coordinates (returns nullptr if doesn't exist).
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* FindRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Ensure range by coordinates (returns nullptr if doesn't exist).
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Get range by reference (returns nullptr if doesn't exist).
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* Range(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Delete range by coordinates (returns false if doesn't exist).
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool DeleteRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Ensure named range by coordinates (returns nullptr if doesn't exist).
        /// @param[in]  sName tString
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureRangeNamed(tString sName,tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Ensure named range by coordinates (returns nullptr if doesn't exist).
        /// @param[in]  sName tString
        /// @param[in]  sJsonData tString
        /// @param[in]  sFormula tString
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureRangeData(tString sName,tString sJsonData, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Delete named range (returns false if doesn't exist).
        /// @param[in]  sName tString
        /// @return     tBool
        tBool DeleteRangeNamed(tString sName);

        /// @brief      Ensure merged range by coordinates (returns nullptr if doesn't exist).
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureMergedRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        /// @brief      Delete merged range by coordinates (returns false if doesn't exist).
        /// @param[in]  sTop tIndex Index row top
        /// @param[in]  sLeft tIndex Index column left
        /// @param[in]  sBottom tIndex Index row bottom
        /// @param[in]  sRight tIndex Index column right
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool DeleteMergedRange(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tSheet* sSheet = nullptr);

        // Interface Str =======================================================
        /// @brief      Get formula by row and column.
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet (if nullptr take active sheet)
        /// @param[in]  sUser when true, Excel bar form (e.g. [@[Col]] instead of Table[[#This Row][Col]])
        /// @return     tString
        tString Formula(tString sRef, tSheet* sSheet = nullptr, tBool sUser = false);
        
        /// @brief      Find range by reference (returns nullptr if doesn't exist).
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* FindRange(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Ensure range by reference (returns nullptr if doesn't exist).
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureRange(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Delete range by reference (returns false if doesn't exist).
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tBool
        tBool DeleteRange(tString sRef, tSheet* sSheet = nullptr);

        /// @brief      Ensure named range by reference (returns nullptr if doesn't exist).
        /// @param[in]  sName tString Name of Range
        /// @param[in]  sRef tString Ref of Range
        /// @param[in]  sFormula  tString
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRange*
        tRange* EnsureRangeNamed(tString sName, tString sRef,tString sFormula="", tSheet* sSheet = nullptr);

        /// @brief      Find named range (returns nullptr if doesn't exist).
        /// @param[in]  sName tString Name of Range
        /// @return     tRange*
        virtual tRange* FindRangeNamed(tString sName);
        
        /// @brief      Return Range Data.
		/// @param[in]  sName tString
		/// @return		tRangeData*
		tRangeData* RangeData(tString sName);

        /// @brief      Insert rows at position.
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to insert
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void InsertRow(tIndex sRow, tIndex sSize, tSheet* sSheet = nullptr);

        /// @brief      Delete rows at position.
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sSize tIndex number row to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void DeleteRow(tIndex sRow, tIndex sSize, tSheet* sSheet = nullptr);

        /// @brief      Insert columns at position.
        /// @param[in]  sCol tIndex Index column
        /// @param[in]  sSize tIndex number column to insert
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void InsertCol(tIndex sCol, tIndex sSize, tSheet* sSheet = nullptr);

        /// @brief      Delete columns at position.
        /// @param[in]  sCol tIndex Index column
        /// @param[in]  sSize tIndex number column to delete
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void DeleteCol(tIndex sCol, tIndex sSize, tSheet* sSheet = nullptr);
        
        /// @brief      Delete sheet.
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void DeleteSheet(tSheet* sSheet = nullptr);

        /// @brief      Get bottom-right position.
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tPoint
        tPoint BottomRight(tSheet* sSheet = nullptr);
        
        /// @brief      Get sum of pixel width between columns.
        /// @param[in]  sColStart tIndex
        /// @param[in]  sColEnd tIndex
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tDouble
        tDouble SumPixelWidth(tIndex sColStart, tIndex sColEnd, tSheet* sSheet = nullptr);
        
        /// @brief      Get sum of pixel height between rows.
        /// @param[in]  sRowStart tIndex
        /// @param[in]  sRowEnd tIndex
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tDouble
        tDouble SumPixelHeight(tIndex sRowStart, tIndex sRowEnd, tSheet* sSheet = nullptr);
        
        /// @brief      Get column index by pixel.
        /// @param[in]  sColStart tIndex
        /// @param[in]  sPixel tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     std::tuple<tIndex, tDouble>
        std::tuple<tIndex, tDouble> IndexColByPixel(tIndex sColStart, tDouble sPixel, tSheet* sSheet = nullptr);
        
        /// @brief      Get row index by pixel.
        /// @param[in]  sRowStart tIndex
        /// @param[in]  sPixel tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     std::tuple<tIndex, tDouble>
        std::tuple<tIndex, tDouble> IndexRowByPixel(tIndex sRowStart, tDouble sPixel, tSheet* sSheet = nullptr);
    
        /// @brief      Move cell to next position.
        /// @param[in]  sCellPoint tPoint
        /// @param[in]  sKey  tByte
        /// @param[in]  sMeta  tByte
        /// @param[in]  sScreen  tRect
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRect
        tRect MoveCell(tPoint sCellPoint, tByte sKey,tByte sMeta, tRect sScreen, tSheet* sSheet = nullptr);
      

        /// @brief      Move to next non-empty cell position.
        /// @param[in]  sCellPoint tPoint
        /// @param[in]  sDirection tByte
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tRect
        tRect MoveToCell(tPoint sCellPoint, tByte sDirection, tSheet* sSheet = nullptr);

        ///= Interface Json =========================================================== 
        /// @brief      Get JSON view.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[in]  sUnit tUnit
        /// @param[in]  sViewHeight tDouble
        /// @param[in]  sViewWidth tDouble
        /// @param[in]  sDiffY tDouble
        /// @param[in]  sDiffX tDouble
        /// @param[in]  sDepl tInt // Mask
        /// @param[in]  sCss tBool
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tString Json
        tString JsonView(tIndex sRow, tIndex sCol, tUnitMetrics sUnit, tDouble sViewHeight, tDouble sViewWidth, tDouble sDiffY, tDouble sDiffX, tBool sCss, tSheet* sSheet = nullptr);
        
        /// @brief      Get JSON right justify on view.
        /// @param[in]  sCol tIndex
        /// @param[in]  sUnit tUnit
        /// @param[in]  sViewWidth tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tString Json
        tString JsonRightJustify(tIndex sCol, tUnitMetrics sUnit, tDouble sViewWidth, tSheet* sSheet = nullptr);
        
        /// @brief      Get JSON bottom justify on view.
        /// @param[in]  sRow tIndex
        /// @param[in]  sUnit tUnit
        /// @param[in]  sViewHeight tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tString Json
        tString JsonBottomJustify(tIndex sRow, tUnitMetrics sUnit, tDouble sViewHeight, tSheet* sSheet = nullptr);
        
        /// @brief      Get JSON column by pixel.
        /// @param[in]  sColStart tIndex
        /// @param[in]  sPixel tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tString
        tString JsonColByPixel(tIndex sColStart, tDouble sPixel, tSheet* sSheet = nullptr);
        
        /// @brief      Get JSON row by pixel.
        /// @param[in]  sRowStart tIndex
        /// @param[in]  sPixel tDouble
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tString
        tString JsonRowByPixel(tIndex sRowStart, tDouble sPixel, tSheet* sSheet = nullptr);

        /// @brief      Floating objects on a target sheet for React overlay layer.
        /// @param[in]  sTargetSheetName tString display sheet to filter
        /// @param[in]  sSheet tSheet active context (if nullptr take active sheet)
        /// @return     tString JSON { objects: [...] }
        tString JsonFloatingObjectsForSheet(tString sTargetSheetName, tSheet* sSheet = nullptr);

        /// @brief      All floating objects in the active workbook.
        /// @description Each entry: n (name), c (class), t (target sheet), dx, dy, w, h, op, ar, ac.
        /// @param[in]  sSheet tSheet* workbook context (if nullptr take active sheet)
        /// @return     tString JSON { "objects": [ ... ] }
        tString JsonFloatingObjects(tSheet* sSheet = nullptr);
        
        /// @brief      Get JSON list of sheets.
        /// @return     tString Json
        tString JsonSheets();
        
        /// @brief      Return JSON list of named ranges.
        /// @return     tString
        tString JsonRangeNamed();
        
        /// @brief      Return JSON list of formula named
        /// @return     tString
        tString JsonformulaNamed();
        
        /// @brief      Return JSON list of tables.
        /// @return     tString
        tString JsonRangeData();


        ///@brief Retunr all conditionalformats
        ///@return tString
        tString  JsonWorkBookConditionalFormats();

        /// @brief      Return JSON conditional format.
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString JsonConditionalFormats(tSheet* sSheet = nullptr);
     
        /// @brief Return JSON conditional format by intersect rect.
        /// @param[in] sRect tRect Rectangle to intersect with conditional formats
        /// @param[in] sSheet tSheet* (if nullptr take active sheet)
        /// @return tString JSON array containing conditional formats that intersect the rectangle
        tString JsonConditionalFormats(tRect& sRect, tSheet* sSheet = nullptr);

        /// @brief      Serialize merged print parameters (workbook layout + sheet orientation / FitToPage).
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString JsonPrintParameters(tSheet* sSheet = nullptr);

        /// @brief      Apply merged print JSON via @ref tUndoPrintParameters (orientation / FitToPage on the sheet, other keys on the workbook).
        /// @param[in]  sJsonPrintParameters tString
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tBool false if undo registration / apply failed
        tBool JsonPrintParameters(tString sJsonPrintParameters, tSheet* sSheet = nullptr);
        
        /// @brief 
        /// @param[in]  sSearch tString
        /// @param[in]  sMatchCase Excel "Respecter la casse"
        /// @param[in]  sMatchEntireCell Excel "Cellule entière"
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString JSON {"cells":["A1",...]}
        tString JsonFindCell(tString sSearch, tBool sMatchCase = false, tBool sMatchEntireCell = false,
                             tSheet* sSheet = nullptr);

        /// @brief Find cells on every user sheet of the active workbook.
        /// @return JSON {"cells":["Sheet1!A1",...]} (sheet-qualified refs)
        tString JsonFindCellWorkBook(tString sSearch, tBool sMatchCase = false, tBool sMatchEntireCell = false);

        /// @brief     Find unique value
        /// @param[in]  sRect tRect
        /// @param[in]  sSheet tSheet* (if nullptr take active sheet)
        /// @return     tString
        tString JsonFindUniqueValue(tRect sRect, tSheet* sSheet = nullptr);
        
       
        //=======================================================================
        /// @brief     Find unique value
        /// @brief      Get rectangle for sheet selection.
        /// @return     tRect
        tRect GetSheetSelect();

        /// @brief      Get rectangle for column selection.
        /// @param[in]  sColBegin tIndex
        /// @param[in]  sSize tIndex
        /// @return     tRect
        tRect GetColSelect(tIndex sColBegin, tIndex sSize);

        /// @brief      Get rectangle for row selection.
        /// @param[in]  sRowBegin tIndex
        /// @param[in]  sSize tIndex
        /// @return     tRect
        tRect GetRowSelect(tIndex sRowBegin, tIndex sSize);

        /// @brief      Find all ranges covering this cell.
        /// @param[in]  sCell tCell
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tColRow::tContainerRange::tResult*
        tColRow::tContainerRange::tResult* FindRangesCovered(tCell* sCell, tSheet* sSheet = nullptr);
        
        /// @brief      Find all ranges covering this cell by row and column.
        /// @param[in]  sRow tIndex Index row
        /// @param[in]  sCol tIndex Index column
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        /// @return     tColRow::tContainerRange::tResult*
        tColRow::tContainerRange::tResult* FindRangesCovered(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr);
    
        /// @brief      Find all ranges covering this rectangle.
        /// @param[in]  sRect tRect
        /// @param[out] sVectorRange tVectorRange
        /// @param[in]  sSheet tSheet (if tSheet==nullptr take active sheet)
        void FindRangesCovered(tRect sRect, tVectorRange* sVectorRange, tSheet* sSheet = nullptr);
         
        /// @brief      Post message to server.
        /// @param[in] sMessage tString
        /// @return tBool
        virtual tBool PostMessage(tString sMessage);
        
        /// @brief      Get message from server.
        /// @param[in] sMessage tString
        /// @return tBool
        virtual tBool GetMessage(tString sMessage);

        /// @brief      Get error type.
        /// @return     tString
        tString Error();

        /// @brief      Get error line.
        /// @return     tInt
        tInt ErrorLine();

        /// @brief      Get error column.
        /// @return     tInt
        tInt ErrorColumn();

        /// @brief      Get error with detail.
        /// @return     tString
        tString ErrorWithDetail();

    };

    /// @brief      Parse range.
    /// @param[in]  sRef tString
    /// @param[out] sTop tIndex
    /// @param[out] sLeft tIndex
    /// @param[out] sBottom tIndex
    /// @param[out] sRight tIndex
    /// @return     tBool
    tBool ParseRange(tString sRef, tIndex& sTop, tIndex& sLeft, tIndex& sBottom, tIndex& sRight);

    /// @brief      Parse cell.
    /// @param[in]  sRef tString
    /// @param[out] sRow tIndex
    /// @param[out] sCol tIndex
    /// @return     tBool
    tBool ParseCell(tString sRef, tIndex& sRow, tIndex& sCol);

} // End of namespace


#endif
