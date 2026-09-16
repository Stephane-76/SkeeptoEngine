//=============================================================================
//  SkJsonView.hpp
//  Skeepto interface with React (or other interfaces in the future)
//
//  Created by stephane allez on 29/03/2025.
//=============================================================================
#ifndef SkJsonView_hpp
#define SkJsonView_hpp

#include "SkColRowCellRange.hpp"

namespace SkSpreadSheet {
//=============================================================================
// SkJsonView
//=============================================================================
class tColRowCellRange;

class tJsonView : public tVirtualClass {
private:
    //! ColRowCellRange
    tColRowCellRange* m_ColRowCellRange;

    //! Conditional Format Container
    tConditionalFormatContainer* m_ConditionalFormatContainer;
    //! Key React    (change Key of Merged Cel)l
    //!  Bug React Scroll For Each merged cell change key each tilme
    tInt              m_KeyIndex;
    
    
    //! Only  one execution on Vien
    mutable std::mutex m_Mutex;
public:
    ///@brief Constructor
    tJsonView();
 
    /// @brief      Returns the cell, if it does not exist return nullptr. (Use TreeView)
    /// @param[in]  sColRowCellRange tColRowCellRange*
    void ColRowCellRange(tColRowCellRange* sColRowCellRange);
    
    /// @brief      Returns first visible Col (TreeView)
    /// @param[in]  sCol tIndex
    /// @return        tCell*
    tIndex FirstCol(tIndex sCol);

    /// @brief      Returns nextt visible Col (TreeView)
    /// @param[in]  sCol tIndex
    /// @return        tCell*
    tIndex NextCol(tIndex sCol);

    
    /// @brief      Returns first visible Col (TreeView)
    /// @param[in]  sCol tIndex
    /// @return        tCell*
    tIndex FirstRow(tIndex sRow);

    /// @brief      Returns nextt visible Col (TreeView)
    /// @param[in]  sRow  tIndex
    /// @return        tCell*
    tIndex NextRow(tIndex sRow);
    
    /// @brief      Returns the cell, if it does not exist return nullptr. (Use TreeView)
    /// @param[in]  sRow tIndex
    /// @param[in]  sCol tIndex
    /// @return        tCell*
    tCell* Cell(tIndex sRow, tIndex sCol);
    
    /// @brief        Return the col, if it does exist return nullptr (Use TreeView)
    /// @param[in]  sIndex tIndex
    /// @return        SkColRow*
    tColRow* Col(tIndex sIndex);

    /// @brief        Return the row, if it does exist return nullptr (Use TreeView)
    /// @param[in]  sIndex tIndex
    /// @return        SkColRow*
    tColRow* Row(tIndex sIndex);
    
    // brief SizeRow
    /// @param[in]    sRow tIndex
    /// @param[ind]     sUnit Unit  tUnit
    tDouble SizeRow(tIndex sRow,tUnitMetrics sUnit);

    // brief SizeCol
    /// @param[in]    sCol  tIndex
    /// @param[ind]     sUnit Unit  tUnit
    tDouble SizeCol(tIndex sRow,tUnitMetrics sUnit);

    /// @brief       return Merged Range.
    /// @param[in]  sRow tIndex
    /// @param[in]  sCol tIndex
    /// @return     tRange*
    tRange* MergedRange(tIndex sRow, tIndex sCol);
    
    /// @brief        Return Border Mask of Cell
    /// @param[in]    sCell tCell*
    /// @return       ttShort
    tShort CellBorder(tCell* sCell);

    /// @brief        Emit f_bor / f_bob for a merged range from edge cell formats.
    ///               Outer right/bottom are read on the merge's right column and bottom row
    ///               (cell-local owner model), then attached to the anchor JSON for canvas.
    /// @param[in]    sRange tRange* merged range
    /// @param[in]    sCss   tBool CSS string (true) or canvas object (false)
    /// @param[in]    sWriter Writer<StringBuffer>*
    void EmitMergedBorders(tRange* sRange, tBool sCss, Writer<StringBuffer>* sWriter);
    
    /// @brief        True when the cell has no value (Excel overflow may cross its borders).
    /// @param[in]    sCell tCell*
    /// @return       tBool
    tBool IsEmptyLeft(tCell* sCell);
    
    /// @brief        True when the cell has no value (Excel overflow may cross its borders).
    /// @param[in]    sCell tCell*
    /// @return       tBool
    tBool IsEmptyRight(tCell* sCell);
    
    /// @brief        Get ClipLeft
    /// @param[in]    sRow tIndex
    /// @param[in]    sCol tIndex
    /// @param[in]    sUnit tUnit
    /// @param[in]    sPos  tDouble
    /// @return       tDouble;
    tDouble GetLeftClip(tIndex sRow, tIndex sCol,tUnitMetrics sUnit,tDouble sPos);
    
    /// @brief        Get ClipRight
    /// @param[in]    sRow tIndex
    /// @param[in]    sCol tIndex
    /// @param[in]    sUnit tUnit
    /// @param[in]    sPos  tDouble
    /// @return       tDouble;
    tDouble GetRightClip(tIndex sRow, tIndex sCol,tUnitMetrics sUnit,tDouble sPos);


    /// @brief      Return item CF
    /// @param[in]  sCell tCell*
    /// @return      tCellConditionalFormat*
    tCellConditionalFormat* CellConditionalFormat(tCell* sCell);

    /// @brief        Get View.
    /// @param[in] Writer<StringBuffer>* sWriter
    /// @param[in]    sRow tIndex
    /// @param[in]    sCol tIndex
    /// @param[in]  sUnit tUnit
    /// @param[in]    sViewHeigt  tDouble
    /// @param[in]    sViewWidth tDouble
    /// @param[in]  sDiffY  tDouble
    /// @param[in]  sDiffX  tDouble
    /// @param[in]  sCss tBool
    void View(Writer<StringBuffer>* sWriter, tIndex sRow, tIndex sCol, tUnitMetrics sUnit, tDouble sViewHeight, tDouble sViewWidth,tDouble sDiffY, tDouble sDiffX,tBool sCss);

    /// @brief Return  Rigtt Justify OnView
    /// @param[in] Writer<StringBuffer>* sWriter
    /// @param[in] sCol tIndex
    /// @param[in] sUnit tUnit
    /// @param[in] sViewWidth tDouble
    void ReturnRightJustify(Writer<StringBuffer>* sWriter, tIndex sCol, tUnitMetrics sUnit,tDouble sViewWidth);
    
    /// @brief Return  Bottomt Justify OnView
    /// @param[in] Writer<StringBuffer>* sWriter
    /// @param[in] sRow  tIndex
    /// @param[in] sUnit tUnit
    /// @param[in] sViewHeight tDouble
    void ReturnBottomJustify(Writer<StringBuffer>* sWriter, tIndex sRow, tUnitMetrics sUnit,tDouble sViewHeight);
    
};

} // end of NameSpace

#endif
