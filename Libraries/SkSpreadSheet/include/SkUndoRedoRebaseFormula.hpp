/*
 * SkUndoRedoRebaseFormula.hpp
 *
 *  Created on: 23.11.2025
 *      Author: Stéphane Allez
 */

#ifndef SkUndoRedoRebaseFormula_hpp
#define SkUndoRedoRebaseFormula_hpp

#include <SkApplication.hpp>
#include "SkTools.hpp"
#include "SkInterfaceCompil.hpp"
#include "SkFormula.hpp"
#include "SkUndoRedoRebase.hpp"
#include "SkSharedFormula.hpp"
#include "SkUndoRedoSaveSp.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {
   

    class tUndoRedoRebaseFormula : public tSave {
    private:
        // Formula (formula to rebase)
        tString m_Formula;

        ///@brief Return SheetAlocatorRef
        ///@param[in] sName tString
        ///@return tAllocatorRef
        tAllocatorRef SheetAlocatorRef(tString sName);
        

        ///@brief Rebase Cell
        ///@param[out] sCell tString
        ///@param[in] sRebasePlan const tRebasePlan&
        tBool RebaseCell(tString& sCell,const tRebasePlan& sRebasePlan);

        /// @brief Flush a pending Sheet/Colon/Cell token stack into sOut (rebased).
        tBool AppendRebasedRefStack(tVectorLexerToken& sStack,
                                    tString& sOut,
                                    const tRebasePlan& sRebasePlan);

        ///@brief Remap a cell reference when a block is moved on a sheet.
        ///@param[out] sCell tString
        ///@param[in] sMoveSheetAllocator tAllocatorRef
        ///@param[in] sRemapRect const tRect&
        ///@param[in] sDeltaRow tIndex
        ///@param[in] sDeltaCol tIndex
        ///@param[in] sRemapLocalSheet tBool
        tBool RemapCellForMove(tString& sCell,
                               tAllocatorRef sMoveSheetAllocator,
                               const tRect& sRemapRect,
                               tIndex sDeltaRow,
                               tIndex sDeltaCol,
                               tBool sRemapLocalSheet);

        tBool AppendRemappedRefStack(tVectorLexerToken& sStack,
                                     tString& sOut,
                                     tAllocatorRef sMoveSheetAllocator,
                                     const tRect& sRemapRect,
                                     tIndex sDeltaRow,
                                     tIndex sDeltaCol);

        /// @brief True when sCell is on the move sheet and lies inside sRemapRect.
        tBool CellRefTouchesMoveRect(const tString& sCell,
                                     const tRect& sRemapRect,
                                     tBool sRemapLocalSheet);

        /// @brief True when a parsed ref stack contains a cell that would be remapped.
        tBool RefStackTouchesMoveRect(tVectorLexerToken& sStack,
                                      tAllocatorRef sMoveSheetAllocator,
                                      const tRect& sRemapRect);
    public:
        /// @brief constructor of tUndoRedoRebaseFormula.
        /// @return tUndoRedoRebaseFormula*
        tUndoRedoRebaseFormula();

        /// @brief constructor of tUndoRedoRebaseFormula.
        /// @param[in] sSheetAllcocator  tAllocatorRef
        /// @param[in] sRowIndex tIndex
        /// @param[in] sColIndex tIndex
        /// @param[in] sFormula tString
        /// @return tUndoRedoRebaseFormula*
        tUndoRedoRebaseFormula(tAllocatorRef sSheetAllocator,tIndex sRowIndex,tIndex sColIndex,tString sFormula);
        /// @brief destructor of tUndoRedoRebaseFormula.
        ~tUndoRedoRebaseFormula();
        
        /// @brief Get formula
        /// @return tString
        tString Formula();

        /// @brief Rebase formula
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return tBool true if rebase successful, false if formula was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan);

        /// @brief Remap formula references that point into a moved block.
        /// @param[in] sMoveSheetAllocator Sheet where the block was moved
        /// @param[in] sRemapRect Block coordinates to remap (source on Do, dest on Undo)
        /// @param[in] sDeltaRow Row offset to apply to refs inside sRemapRect
        /// @param[in] sDeltaCol Column offset to apply to refs inside sRemapRect
        /// @return tBool true if remap successful
        tBool RemapForMove(tAllocatorRef sMoveSheetAllocator,
                           const tRect& sRemapRect,
                           tIndex sDeltaRow,
                           tIndex sDeltaCol);

        /// @brief True when the formula contains a cell ref inside the moved block.
        /// Used by tUndoMove to skip unrelated formulas (Option A selective remap).
        tBool ReferencesMoveRect(tAllocatorRef sMoveSheetAllocator,
                                 const tRect& sRemapRect);

    };

}

#endif /* SkUndoRedoRebaseFormula_hpp */
