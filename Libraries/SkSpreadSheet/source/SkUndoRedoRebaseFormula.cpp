/* 
 * SkUndoRedoRebaseFormula.cpp
 *
 *  Created on: 23.11.2025
 *      Author: Stéphane Allez
 */

#include "../include/SkUndoRedoRebaseFormula.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkLemonInterface.hpp"

#define debugrebaseformula

namespace SkSpreadSheet {

    tUndoRedoRebaseFormula::tUndoRedoRebaseFormula() : tSave() {
    }
    tUndoRedoRebaseFormula::tUndoRedoRebaseFormula(tAllocatorRef sSheetAllocator,tIndex sRowIndex,tIndex sColIndex,tString sFormula) : tSave(sSheetAllocator),
        m_Formula(sFormula) {
        (void)sRowIndex;
        (void)sColIndex;
        (void)sSheetAllocator;
    }
    tUndoRedoRebaseFormula::~tUndoRedoRebaseFormula() {}


    tAllocatorRef tUndoRedoRebaseFormula::SheetAlocatorRef(tString sName) {
        tColRowCellRange* wColRowCellRange = ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return(0);
        }
        tWorkBook* wWorkBook = wColRowCellRange->Sheet()->WorkBook();
        tSheet* wSheet=wWorkBook->Sheet(sName);
        if (wSheet==nullptr) {
            return(0);
        }
        return(wSheet->AllocatorRef());
    }

    tBool tUndoRedoRebaseFormula::RebaseCell(tString& sCell,const tRebasePlan& sRebasePlan) {
        tTempoPoint wTempoPoint;
        wTempoPoint.ParseRef(sCell);

        tIndex wRow = wTempoPoint.Row();
        tIndex wCol = wTempoPoint.Col();

        auto wRebasedRow = sRebasePlan.RebaseRow(wRow);
        auto wRebasedCol = sRebasePlan.RebaseCol(wCol);

        // If cell was deleted during rebase, return empty string
        if (!wRebasedRow.has_value() || !wRebasedCol.has_value()) {
            return false;
        }

        wTempoPoint.Row(wRebasedRow.value());
        wTempoPoint.Col(wRebasedCol.value());
        sCell = wTempoPoint.StrRef();
        return(true);
    }

    tBool tUndoRedoRebaseFormula::AppendRebasedRefStack(tVectorLexerToken& sStack,
                                                        tString& sOut,
                                                        const tRebasePlan& sRebasePlan) {
        const tSize wSize = sStack.size();
        if (wSize == 0) {
            return(true);
        }
        switch (wSize) {
            case 1: {
                tString wCell = sStack[0]->Lexeme();
                if (!RebaseCell(wCell, sRebasePlan)) return(false);
                sOut += wCell;
                break;
            }
            case 2: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tString wCell = sStack[1]->Lexeme();
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) return(false);
                tRebasePlan wRebasePlan = IsRebaseAnotherSheet(sRebasePlan, wSheetAllocatorRef);
                if (!RebaseCell(wCell, wRebasePlan)) return(false);
                sOut += wSheetName + "!" + wCell;
                break;
            }
            case 3: {
                tString wCellTL = sStack[0]->Lexeme();
                tString wCellBR = sStack[2]->Lexeme();
                if (!RebaseCell(wCellTL, sRebasePlan)) return(false);
                if (!RebaseCell(wCellBR, sRebasePlan)) return(false);
                sOut += wCellTL + ":" + wCellBR;
                break;
            }
            case 4: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tString wCellTL = sStack[1]->Lexeme();
                tString wCellBR = sStack[3]->Lexeme();
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) return(false);
                tRebasePlan wRebasePlan = IsRebaseAnotherSheet(sRebasePlan, wSheetAllocatorRef);
                if (!RebaseCell(wCellTL, wRebasePlan)) return(false);
                if (!RebaseCell(wCellBR, wRebasePlan)) return(false);
                sOut += wSheetName + "!" + wCellTL + ":" + wCellBR;
                break;
            }
            default:
                break;
        }
        sStack.clear();
        return(true);
    }

    tBool tUndoRedoRebaseFormula::RemapCellForMove(tString& sCell,
                                                   tAllocatorRef sMoveSheetAllocator,
                                                   const tRect& sRemapRect,
                                                   tIndex sDeltaRow,
                                                   tIndex sDeltaCol,
                                                   tBool sRemapLocalSheet) {
        if (sDeltaRow == 0 && sDeltaCol == 0) {
            return(true);
        }

        tTempoPoint wTempoPoint;
        wTempoPoint.ParseRef(sCell);

        tRect wRect(sRemapRect);
        const tIndex wRow = wTempoPoint.Row();
        const tIndex wCol = wTempoPoint.Col();
        if (wRow < wRect.Top() || wRow > wRect.Bottom() ||
            wCol < wRect.Left() || wCol > wRect.Right()) {
            return(true);
        }

        if (!sRemapLocalSheet) {
            return(true);
        }

        wTempoPoint.Row(wRow + sDeltaRow);
        wTempoPoint.Col(wCol + sDeltaCol);
        sCell = wTempoPoint.StrRef();
        return(true);
    }
   
    tString tUndoRedoRebaseFormula::Formula() { return(m_Formula); }

    namespace {
        /// Re-emit a non-ref token. LabelSquare/quote lexemes exclude their delimiters.
        void AppendLexerTokenLexeme(tString& sOut, tKind sKind, const tString& sLexeme) {
            switch (sKind) {
                case tKind::LabelSquare:
                    sOut += "[";
                    sOut += sLexeme;
                    sOut += "]";
                    break;
                case tKind::LabelSimple:
                    sOut += "'";
                    sOut += sLexeme;
                    sOut += "'";
                    break;
                case tKind::LabelDouble:
                    sOut += "\"";
                    sOut += sLexeme;
                    sOut += "\"";
                    break;
                default:
                    sOut += sLexeme;
                    break;
            }
        }
    }

 
    tBool tUndoRedoRebaseFormula::Rebase(const tRebasePlan& sRebasePlan) {
        tBool wOk=true;
    
        tLexerToken wLexerToken;

        tVectorLexerToken wVectorLexerToken;
        // Lexer ===============================================================
        tLexer wLex(m_Formula.c_str());
        
        tString wNewFormula="";
        
        wLexerToken = wLex.next();
        tString wTokenLex="";

        // Loop until End or Unexpected caracter
        while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
            wTokenLex = wLexerToken.Lexeme();
            
            tKind wKind = wLexerToken.Kind();
            switch (wKind) {
                case tKind::Sheet :
                case tKind::Colon:
                case tKind::Cell : {
                    wVectorLexerToken.push_back(new tLexerToken(wLexerToken));
                    break;
                }
                default: {
                    // Flush pending refs, then append the non-ref token (e.g. "=", "+", ")").
                    if (!AppendRebasedRefStack(wVectorLexerToken, wNewFormula, sRebasePlan)) {
                        return(false);
                    }
                    AppendLexerTokenLexeme(wNewFormula, wKind, wTokenLex);
                    break;
                }
            }
            wLexerToken = wLex.next();
        }
        // Flush trailing refs (e.g. "=G17" ends on Cell — nothing after to trigger default).
        if (!AppendRebasedRefStack(wVectorLexerToken, wNewFormula, sRebasePlan)) {
            return(false);
        }
        m_Formula=wNewFormula;
        return(wOk);
    }

    tBool tUndoRedoRebaseFormula::AppendRemappedRefStack(tVectorLexerToken& sStack,
                                                       tString& sOut,
                                                       tAllocatorRef sMoveSheetAllocator,
                                                       const tRect& sRemapRect,
                                                       tIndex sDeltaRow,
                                                       tIndex sDeltaCol) {
        const tSize wSize = sStack.size();
        if (wSize == 0) {
            return(true);
        }
        switch (wSize) {
            case 1: {
                tString wCell = sStack[0]->Lexeme();
                if (!RemapCellForMove(wCell,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      true)) {
                    return(false);
                }
                sOut += wCell;
                break;
            }
            case 2: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tString wCell = sStack[1]->Lexeme();
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) {
                    return(false);
                }
                const tBool wRemapLocalSheet = (wSheetAllocatorRef == sMoveSheetAllocator);
                if (!RemapCellForMove(wCell,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      wRemapLocalSheet)) {
                    return(false);
                }
                sOut += sStack[0]->Lexeme() + wCell;
                break;
            }
            case 3: {
                tString wCellTL = sStack[0]->Lexeme();
                tString wCellBR = sStack[2]->Lexeme();
                if (!RemapCellForMove(wCellTL,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      true)) {
                    return(false);
                }
                if (!RemapCellForMove(wCellBR,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      true)) {
                    return(false);
                }
                sOut += wCellTL + ":" + wCellBR;
                break;
            }
            case 4: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tString wCellTL = sStack[1]->Lexeme();
                tString wCellBR = sStack[3]->Lexeme();
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) {
                    return(false);
                }
                const tBool wRemapLocalSheet = (wSheetAllocatorRef == sMoveSheetAllocator);
                if (!RemapCellForMove(wCellTL,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      wRemapLocalSheet)) {
                    return(false);
                }
                if (!RemapCellForMove(wCellBR,
                                      sMoveSheetAllocator,
                                      sRemapRect,
                                      sDeltaRow,
                                      sDeltaCol,
                                      wRemapLocalSheet)) {
                    return(false);
                }
                sOut += sStack[0]->Lexeme() + wCellTL + ":" + wCellBR;
                break;
            }
            default:
                break;
        }
        sStack.clear();
        return(true);
    }

    tBool tUndoRedoRebaseFormula::CellRefTouchesMoveRect(const tString& sCell,
                                                         const tRect& sRemapRect,
                                                         tBool sRemapLocalSheet) {
        if (!sRemapLocalSheet) {
            return(false);
        }
        tTempoPoint wTempoPoint;
        wTempoPoint.ParseRef(sCell);
        const tIndex wRow = wTempoPoint.Row();
        const tIndex wCol = wTempoPoint.Col();
        tRect wRect(sRemapRect);
        return(wRow >= wRect.Top() && wRow <= wRect.Bottom() &&
               wCol >= wRect.Left() && wCol <= wRect.Right());
    }

    tBool tUndoRedoRebaseFormula::RefStackTouchesMoveRect(tVectorLexerToken& sStack,
                                                          tAllocatorRef sMoveSheetAllocator,
                                                          const tRect& sRemapRect) {
        const tSize wSize = sStack.size();
        if (wSize == 0) {
            return(false);
        }
        switch (wSize) {
            case 1: {
                return(CellRefTouchesMoveRect(sStack[0]->Lexeme(), sRemapRect, true));
            }
            case 2: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) {
                    return(false);
                }
                const tBool wRemapLocalSheet = (wSheetAllocatorRef == sMoveSheetAllocator);
                return(CellRefTouchesMoveRect(sStack[1]->Lexeme(), sRemapRect, wRemapLocalSheet));
            }
            case 3: {
                return(CellRefTouchesMoveRect(sStack[0]->Lexeme(), sRemapRect, true) ||
                       CellRefTouchesMoveRect(sStack[2]->Lexeme(), sRemapRect, true));
            }
            case 4: {
                tString wSheetName = sStack[0]->Lexeme();
                if (wSheetName.length() > 0) {
                    wSheetName = wSheetName.substr(0, wSheetName.length() - 1);
                }
                tAllocatorRef wSheetAllocatorRef = SheetAlocatorRef(wSheetName);
                if (wSheetAllocatorRef == 0) {
                    return(false);
                }
                const tBool wRemapLocalSheet = (wSheetAllocatorRef == sMoveSheetAllocator);
                return(CellRefTouchesMoveRect(sStack[1]->Lexeme(), sRemapRect, wRemapLocalSheet) ||
                       CellRefTouchesMoveRect(sStack[3]->Lexeme(), sRemapRect, wRemapLocalSheet));
            }
            default:
                break;
        }
        return(false);
    }

    tBool tUndoRedoRebaseFormula::ReferencesMoveRect(tAllocatorRef sMoveSheetAllocator,
                                                     const tRect& sRemapRect) {
        if (m_Formula.empty()) {
            return(false);
        }

        tLexerToken wLexerToken;
        tVectorLexerToken wVectorLexerToken;
        tLexer wLex(m_Formula.c_str());
        wLexerToken = wLex.next();

        while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
            tKind wKind = wLexerToken.Kind();
            switch (wKind) {
                case tKind::Sheet:
                case tKind::Colon:
                case tKind::Cell: {
                    wVectorLexerToken.push_back(new tLexerToken(wLexerToken));
                    break;
                }
                default: {
                    if (!wVectorLexerToken.empty()) {
                        if (RefStackTouchesMoveRect(wVectorLexerToken,
                                                    sMoveSheetAllocator,
                                                    sRemapRect)) {
#ifdef _debugleak
                            for (tLexerToken* wToken : wVectorLexerToken) {
                                delete(wToken);
                            }
#endif
                            wVectorLexerToken.clear();
                            return(true);
                        }
                    }
                    wVectorLexerToken.clear();
                    break;
                }
            }
            wLexerToken = wLex.next();
        }
        if (!wVectorLexerToken.empty()) {
            if (RefStackTouchesMoveRect(wVectorLexerToken, sMoveSheetAllocator, sRemapRect)) {
#ifdef _debugleak
                for (tLexerToken* wToken : wVectorLexerToken) {
                    delete(wToken);
                }
#endif
                wVectorLexerToken.clear();
                return(true);
            }
        }
#ifdef _debugleak
        for (tLexerToken* wToken : wVectorLexerToken) {
            delete(wToken);
        }
#endif
        wVectorLexerToken.clear();
        return(false);
    }

    tBool tUndoRedoRebaseFormula::RemapForMove(tAllocatorRef sMoveSheetAllocator,
                                               const tRect& sRemapRect,
                                               tIndex sDeltaRow,
                                               tIndex sDeltaCol) {
        if (sDeltaRow == 0 && sDeltaCol == 0) {
            return(true);
        }

        tLexerToken wLexerToken;
        tVectorLexerToken wVectorLexerToken;
        tLexer wLex(m_Formula.c_str());
        tString wNewFormula = "";
        wLexerToken = wLex.next();
        tString wTokenLex = "";

        while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
            wTokenLex = wLexerToken.Lexeme();
            tKind wKind = wLexerToken.Kind();
            switch (wKind) {
                case tKind::Sheet:
                case tKind::Colon:
                case tKind::Cell: {
                    wVectorLexerToken.push_back(new tLexerToken(wLexerToken));
                    break;
                }
                default: {
                    if (!wVectorLexerToken.empty()) {
                        if (!AppendRemappedRefStack(wVectorLexerToken,
                                                    wNewFormula,
                                                    sMoveSheetAllocator,
                                                    sRemapRect,
                                                    sDeltaRow,
                                                    sDeltaCol)) {
                            return(false);
                        }
                    }
                    AppendLexerTokenLexeme(wNewFormula, wKind, wTokenLex);
                    break;
                }
            }
            wLexerToken = wLex.next();
        }
        if (!wVectorLexerToken.empty()) {
            if (!AppendRemappedRefStack(wVectorLexerToken,
                                        wNewFormula,
                                        sMoveSheetAllocator,
                                        sRemapRect,
                                        sDeltaRow,
                                        sDeltaCol)) {
                return(false);
            }
        }
        m_Formula = wNewFormula;
        return(true);
    }

}
