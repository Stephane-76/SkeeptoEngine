//=============================================================================
// SkRangeRefTransform
// Qualify / strip sheet prefixes on A1 refs via Lexer
// Component Floating Object
//=============================================================================
#include "../include/SkRangeRefTransform.hpp"
#include "../include/SkLexerSpreadSheet.hpp"
#include "../include/SkLexerData.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkRangeData.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkMessage.hpp"
#include <SkApplication.hpp>
#include <SkTypesClass.hpp>
#include <algorithm>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace SkSpreadSheet {

    namespace {

        enum class tTransformMode { Qualify, Strip };

        tString NormalizeSheetName(tString sSheet) {
            tString wText = sSheet;
            while (!wText.empty() && (wText.back() == ' ' || wText.back() == '!')) {
                wText.pop_back();
            }
            return wText;
        }

        tString SheetNameFromSheetToken(const tLexerToken& sToken) {
            tString wLex = sToken.Lexeme();
            if (!wLex.empty() && wLex.back() == '!') {
                wLex.pop_back();
            }
            tClassString wClassString(wLex);
            return wClassString.Unquote();
        }

        tBool SheetNamesEqual(tString sLeft, tString sRight) {
            return NormalizeSheetName(sLeft) == NormalizeSheetName(sRight);
        }

        tBool SheetNameNeedsQuotes(const tString& sSheetName) {
            if (sSheetName.empty()) {
                return false;
            }
            for (const tChar wChar : sSheetName) {
                if (!(std::isalnum(static_cast<unsigned char>(wChar)) != 0 || wChar == '_')) {
                    return true;
                }
            }
            return false;
        }

        tString FormatSheetQualifiedPrefix(tString sSheetName) {
            sSheetName = NormalizeSheetName(sSheetName);
            if (sSheetName.empty()) {
                return "";
            }
            if (SheetNameNeedsQuotes(sSheetName)) {
                return "'" + sSheetName + "'!";
            }
            return sSheetName + "!";
        }

        tString BuildRefLexemeFromStack(const std::vector<tLexerToken>& sStack) {
            if (sStack.empty()) {
                return "";
            }
            // Cell or Name or ErrorRef or ErrorName
            if (sStack.size() == 1) {
                const tKind wKind = sStack[0].Kind();
                if (wKind == tKind::Cell || wKind == tKind::Name || wKind == tKind::ErrorRef || wKind == tKind::ErrorName) {
                    return sStack[0].Lexeme();
                }
                return "";
            }
            // Cell:Cell or Sheet:Cell or Sheet:Name or ErrorRef:ErrorRef
            if (sStack.size() == 2) {
                if (sStack[0].Kind() == tKind::Cell && sStack[1].Kind() == tKind::Cell) {
                    return sStack[0].Lexeme() + ":" + sStack[1].Lexeme();
                }
                if (sStack[0].Kind() == tKind::Sheet && sStack[1].Kind() == tKind::Cell) {
                    return sStack[0].Lexeme() + sStack[1].Lexeme();
                }
                if (sStack[0].Kind() == tKind::Sheet && sStack[1].Kind() == tKind::Name) {
                    return sStack[0].Lexeme() + sStack[1].Lexeme();
                }
                if (sStack[0].Kind() == tKind::ErrorRef || sStack[1].Kind() == tKind::ErrorRef) {
                    return sStack[0].Lexeme() + ":" + sStack[1].Lexeme();
                }
            }
            // Sheet:Cell:Cell or Sheet:Name:Cell or Sheet:ErrorRef:ErrorRef
            if (sStack.size() == 3 && sStack[0].Kind() == tKind::Sheet) {
                const tKind wMid = sStack[1].Kind();
                const tKind wRight = sStack[2].Kind();
                if ((wMid == tKind::Cell || wMid == tKind::ErrorRef)
                    && (wRight == tKind::Cell || wRight == tKind::ErrorName)) {
                    return sStack[0].Lexeme() + sStack[1].Lexeme() + ":" + sStack[2].Lexeme();
                }
            }
            return "";
        }

        tBool StackHasSheetToken(const std::vector<tLexerToken>& sStack) {
            for (const tLexerToken& wToken : sStack) {
                if (wToken.Kind() == tKind::Sheet) {
                    return true;
                }
            }
            return false;
        }

        tBool StackHasNamedRangeToken(const std::vector<tLexerToken>& sStack) {
            for (const tLexerToken& wToken : sStack) {
                if (wToken.Kind() == tKind::Name || wToken.Kind() == tKind::ErrorName) {
                    return true;
                }
            }
            return false;
        }

        const tLexerToken* SheetTokenFromStack(const std::vector<tLexerToken>& sStack) {
            for (const tLexerToken& wToken : sStack) {
                if (wToken.Kind() == tKind::Sheet) {
                    return &wToken;
                }
            }
            return nullptr;
        }

        tString TransformRefLexeme(tString sTargetSheet, tString sRefLexeme, tTransformMode sMode,
                                   const std::vector<tLexerToken>& sStack) {
            if (sRefLexeme.empty()) {
                return sRefLexeme;
            }
            sTargetSheet = NormalizeSheetName(sTargetSheet);
            if (StackHasNamedRangeToken(sStack)) {
                return sRefLexeme;
            }
            if (sMode == tTransformMode::Qualify) {
                if (StackHasSheetToken(sStack) || sTargetSheet.empty()) {
                    return sRefLexeme;
                }
                return FormatSheetQualifiedPrefix(sTargetSheet) + sRefLexeme;
            }
            const tLexerToken* const wSheetToken = SheetTokenFromStack(sStack);
            if (wSheetToken == nullptr || sTargetSheet.empty()) {
                return sRefLexeme;
            }
            if (!SheetNamesEqual(SheetNameFromSheetToken(*wSheetToken), sTargetSheet)) {
                return sRefLexeme;
            }
            std::vector<tLexerToken> wLocalStack = sStack;
            wLocalStack.erase(
                std::remove_if(wLocalStack.begin(), wLocalStack.end(),
                               [](const tLexerToken& sToken) { return sToken.Kind() == tKind::Sheet; }),
                wLocalStack.end());
            return BuildRefLexemeFromStack(wLocalStack);
        }

        tString AppendTokenLexeme(tString sTokenLex, tKind sKind, tChar sDecimal, tChar sArg) {
            if (sKind == tKind::Float && sDecimal != '.') {
                std::replace(sTokenLex.begin(), sTokenLex.end(), sDecimal, '.');
            } else if (sKind == tKind::NotEqual) {
                sTokenLex = "<>";
            } else if (sKind == tKind::GreaterThanOrEqual) {
                sTokenLex = ">=";
            } else if (sKind == tKind::LessThanOrEqual) {
                sTokenLex = "<=";
            } else if (sTokenLex.length() == 1 && sTokenLex[0] == sArg) {
                sTokenLex = ',';
            }
            return sTokenLex;
        }

        tBool IsRefBoundaryToken(tKind sCurrent, tKind sNext) {
            return (((sCurrent != tKind::Sheet) && (sCurrent != tKind::Colon) && (sCurrent != tKind::Cell)
                     && (sCurrent != tKind::Name) && (sCurrent != tKind::ErrorRef))
                    || (sNext == tKind::End));
        }

        tBool IsRefAccumulatorToken(tKind sKind) {
            return (sKind == tKind::Sheet || sKind == tKind::Cell || sKind == tKind::Name
                    || sKind == tKind::ErrorRef || sKind == tKind::ErrorName);
        }

        void FlushRefStack(tString& ioResult, tString sTargetSheet, tTransformMode sMode,
                           std::vector<tLexerToken>& ioStack) {
            if (ioStack.empty()) {
                return;
            }
            const tString wRefLexeme = BuildRefLexemeFromStack(ioStack);
            if (!wRefLexeme.empty()) {
                ioResult += TransformRefLexeme(sTargetSheet, wRefLexeme, sMode, ioStack);
            }
            ioStack.clear();
        }

        tString TransformRefsForSheet(tString sTargetSheet, tString sText, tTransformMode sMode) {
            if (sText.empty()) {
                return sText;
            }

            tLexer wLex(sText.c_str());
            tLocale* const wLocale = tApplication::Instance()->Locale();
            const tChar wDecimal = wLocale->Decimal();
            const tChar wArg = wLocale->Arg();
            wLex.SeparatorArg(wArg);
            wLex.SeparatorDecimal(wDecimal);

            tString wResult;
            std::vector<tLexerToken> wStackCell;
            tLexerToken wLexerToken = wLex.next();

            while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
                tString wTokenLex = wLexerToken.Lexeme();
                const tKind wKind = wLexerToken.Kind();
                const tLexerToken wNextToken = wLex.next();
                tBool wAccumulatedRef = IsRefAccumulatorToken(wKind);

                switch (wKind) {
                case tKind::Sheet:
                    wStackCell.push_back(wLexerToken);
                    break;
                case tKind::Cell:
                case tKind::Name:
                case tKind::ErrorRef:
                case tKind::ErrorName:
                    wStackCell.push_back(wLexerToken);
                    break;
                case tKind::Identifier:
                    if (!wStackCell.empty() && wStackCell.back().Kind() == tKind::Sheet
                        && !wNextToken.is(tKind::LeftParen)) {
                        tLexerToken wNameToken = wLexerToken;
                        wNameToken.Kind(tKind::Name);
                        wStackCell.push_back(wNameToken);
                        wAccumulatedRef = true;
                    }
                    break;
                default:
                    break;
                }

                if (IsRefBoundaryToken(wKind, wNextToken.Kind())) {
                    FlushRefStack(wResult, sTargetSheet, sMode, wStackCell);
                }

                if (!wAccumulatedRef && wKind != tKind::Colon) {
                    wResult += AppendTokenLexeme(wTokenLex, wKind, wDecimal, wArg);
                }

                wLexerToken = wNextToken;
            }

            if (wLexerToken.Kind() == tKind::Unexpected) {
                return sText;
            }

            return wResult;
        }

        struct tHostContext {
            tWorkBook* WorkBook = nullptr;
            tSheet* HostSheet = nullptr;
            tString HostSheetName;
            tIndex HostRow = 0;
            tIndex HostCol = 0;
            tChar ArgSep = ',';
        };

        struct tCollectedRef {
            tString kind;
            tString text;
            tString sheet;
            tIndex top = 0;
            tIndex left = 0;
            tIndex bottom = 0;
            tIndex right = 0;
            tBool resolved = false;
        };

        tString SheetNameFromToken(const tLexerToken& sToken) {
            tString wLex = sToken.Lexeme();
            if (!wLex.empty() && wLex.back() == '!') {
                wLex.pop_back();
            }
            tClassString wClassString(wLex);
            return wClassString.Unquote();
        }

        tString DefaultSheetName(const tHostContext& sHost, const tLexerToken* sSheetToken) {
            if (sSheetToken != nullptr) {
                return SheetNameFromToken(*sSheetToken);
            }
            return sHost.HostSheetName;
        }

        void ResolveColRowFromCellToken(tLexerToken sToken, const tHostContext& sHost, tIndex& outRow,
                                        tIndex& outCol) {
            if (sToken.R1C1()) {
                if (sToken.LockRow()) {
                    outRow = sToken.RowInt();
                } else {
                    outRow = sHost.HostRow + sToken.RowInt();
                }
                if (sToken.LockCol()) {
                    outCol = sToken.ColInt();
                } else {
                    outCol = sHost.HostCol + sToken.ColInt();
                }
            } else {
                outRow = sToken.RowInt();
                outCol = sToken.ColInt();
            }
        }

        void ExpandShorthandRange(tLexerToken sTop, tLexerToken sBottom, tIndex& ioTopRow,
                                  tIndex& ioTopCol, tIndex& ioBottomRow, tIndex& ioBottomCol) {
            const tBool wTopHasRow = !sTop.Row().empty();
            const tBool wBottomHasRow = !sBottom.Row().empty();
            const tBool wTopHasCol = !sTop.Col().empty();
            const tBool wBottomHasCol = !sBottom.Col().empty();
            if (!wTopHasRow && !wBottomHasRow && wTopHasCol && wBottomHasCol) {
                ioTopRow = 1;
                ioBottomRow = Cst_MaxRow;
            }
            if (!wTopHasCol && !wBottomHasCol && wTopHasRow && wBottomHasRow) {
                ioTopCol = 1;
                ioBottomCol = Cst_MaxCol;
            }
            if (ioTopRow > ioBottomRow) {
                std::swap(ioTopRow, ioBottomRow);
            }
            if (ioTopCol > ioBottomCol) {
                std::swap(ioTopCol, ioBottomCol);
            }
        }

        tString BuildTableRefText(const tLexerToken* sTableToken, const tLexerToken& sSquareToken) {
            const tString wBracket = "[" + sSquareToken.Lexeme() + "]";
            if (sTableToken == nullptr) {
                return wBracket;
            }
            return sTableToken->Lexeme() + wBracket;
        }

        tBool ResolveTableStructuredRef(const tLexerToken* sTableToken, const tLexerToken& sSquareToken,
                                        const tHostContext& sHost, tString& outSheet, tIndex& outTop, tIndex& outLeft,
                                        tIndex& outBottom, tIndex& outRight) {
            if (sHost.WorkBook == nullptr || sHost.HostSheet == nullptr) {
                return false;
            }

            tString wTableName;
            tRange* wTableRange = nullptr;
            if (sTableToken == nullptr) {
                std::tie(wTableName, wTableRange) =
                    sHost.HostSheet->ColRowCellRange()->FindRangeDataCovered(sHost.HostRow, sHost.HostCol);
                if (wTableName.empty() || wTableRange == nullptr) {
                    return false;
                }
            } else {
                wTableName = sTableToken->Lexeme();
                wTableRange = sHost.WorkBook->FindRangeNamed(wTableName);
                if (wTableRange == nullptr) {
                    return false;
                }
            }

            tLexerData wLexerData(const_cast<tLexerToken*>(sTableToken), sHost.ArgSep);
            if (!wLexerData.Parse(const_cast<tLexerToken*>(&sSquareToken))) {
                return false;
            }

            if (!wTableRange->IsData()) {
                return false;
            }

            tSheet* wSheet = wTableRange->Sheet();
            if (wSheet == nullptr) {
                return false;
            }
            outSheet = wSheet->Name();

            tIndex wTableTop = wTableRange->TopIndex();
            tIndex wTableBottom = wTableRange->BottomIndex();
            tIndex wTableLeft = wTableRange->LeftIndex();
            tIndex wTableRight = wTableRange->RightIndex();

            tRangeData* wRangeData = sHost.WorkBook->RangeData(wTableName);
            if (wRangeData == nullptr) {
                return false;
            }

            if (!wLexerData.Col1().empty()) {
                tColumnData* wColumnData1 = wRangeData->FindColumnByName(wSheet, wTableRange, wLexerData.Col1());
                if (wColumnData1 == nullptr) {
                    return false;
                }
                tIndex wColumnCol = wColumnData1->SheetCol();

                if (!wLexerData.Col2().empty()) {
                    tColumnData* wColumnData2 = wRangeData->FindColumnByName(wSheet, wTableRange, wLexerData.Col2());
                    if (wColumnData2 == nullptr) {
                        return false;
                    }
                    tIndex wFirstCol = wColumnData1->SheetCol();
                    tIndex wSecondCol = wColumnData2->SheetCol();
                    if (wFirstCol > wSecondCol) {
                        std::swap(wFirstCol, wSecondCol);
                    }
                    outTop = sHost.HostRow;
                    outLeft = wFirstCol;
                    outBottom = sHost.HostRow;
                    outRight = wSecondCol;
                    return true;
                }

                switch (wLexerData.SpecialKey()) {
                case tTypeData::t_ThisRow:
                    if (sHost.HostRow > wTableTop && sHost.HostRow <= wTableBottom) {
                        outTop = outBottom = sHost.HostRow;
                        outLeft = outRight = wColumnCol;
                        return true;
                    }
                    return false;
                case tTypeData::t_Headers:
                    outTop = outBottom = wTableTop;
                    outLeft = outRight = wColumnCol;
                    return true;
                case tTypeData::t_Totals:
                    outTop = outBottom = wTableBottom + 1;
                    outLeft = outRight = wColumnCol;
                    return true;
                case tTypeData::t_Column: {
                    tIndex wColTop = wTableTop;
                    if (wRangeData->HasHeaders() && wTableBottom > wTableTop) {
                        wColTop = wTableTop + 1;
                    }
                    outTop = wColTop;
                    outBottom = wTableBottom;
                    outLeft = outRight = wColumnCol;
                    return true;
                }
                default:
                    return false;
                }
            }

            switch (wLexerData.SpecialKey()) {
            case tTypeData::t_Headers:
                outTop = outBottom = wTableTop;
                outLeft = wTableLeft;
                outRight = wTableRight;
                return true;
            case tTypeData::t_Totals:
                outTop = outBottom = wTableBottom + 1;
                outLeft = wTableLeft;
                outRight = wTableRight;
                return true;
            case tTypeData::t_All:
                outTop = wTableTop;
                outBottom = wTableBottom;
                outLeft = wTableLeft;
                outRight = wTableRight;
                return true;
            default:
                return false;
            }
        }

        void ResolveCollectedRef(tCollectedRef& ioRef, const tHostContext& sHost,
                                 const std::vector<tLexerToken>& sStack) {
            if (sStack.empty()) {
                return;
            }

            if (sStack.size() == 1) {
                const tKind wKind = sStack[0].Kind();
                if (wKind == tKind::ErrorRef || wKind == tKind::ErrorName) {
                    ioRef.kind = "error";
                    ioRef.resolved = false;
                    return;
                }
                if (wKind == tKind::Name) {
                    ioRef.kind = "name";
                    if (sHost.WorkBook == nullptr) {
                        return;
                    }
                    const tString wName = sStack[0].Lexeme();
                    std::vector<tRange*> wRanges = sHost.WorkBook->RangeNamedContainer()->Ranges(wName);
                    if (wRanges.empty()) {
                        tRange* wRange = sHost.WorkBook->FindRangeNamed(wName);
                        if (wRange == nullptr) {
                            return;
                        }
                        wRanges.push_back(wRange);
                    }
                    if (wRanges.empty() || wRanges.front() == nullptr) {
                        return;
                    }
                    tRange* wRange = wRanges.front();
                    tSheet* wSheet = wRange->Sheet();
                    if (wSheet != nullptr) {
                        ioRef.sheet = wSheet->Name();
                    }
                    ioRef.top = wRange->TopIndex();
                    ioRef.left = wRange->LeftIndex();
                    ioRef.bottom = wRange->BottomIndex();
                    ioRef.right = wRange->RightIndex();
                    ioRef.resolved = true;
                    ioRef.kind = (ioRef.top == ioRef.bottom && ioRef.left == ioRef.right) ? "cell" : "range";
                    return;
                }
                if (wKind == tKind::Cell) {
                    ioRef.kind = "cell";
                    const tLexerToken* wSheetToken = SheetTokenFromStack(sStack);
                    ioRef.sheet = DefaultSheetName(sHost, wSheetToken);
                    if (sHost.HostSheet == nullptr && wSheetToken == nullptr) {
                        return;
                    }
                    ResolveColRowFromCellToken(sStack[0], sHost, ioRef.top, ioRef.left);
                    ioRef.bottom = ioRef.top;
                    ioRef.right = ioRef.left;
                    ioRef.resolved = true;
                    return;
                }
            }

            const tLexerToken* wSheetToken = SheetTokenFromStack(sStack);
            ioRef.sheet = DefaultSheetName(sHost, wSheetToken);

            tIndex wTopRow = 0;
            tIndex wTopCol = 0;
            tIndex wBottomRow = 0;
            tIndex wBottomCol = 0;

            if (sStack.size() == 2 && sStack[0].Kind() == tKind::Cell && sStack[1].Kind() == tKind::Cell) {
                ioRef.kind = "range";
                ResolveColRowFromCellToken(sStack[0], sHost, wTopRow, wTopCol);
                ResolveColRowFromCellToken(sStack[1], sHost, wBottomRow, wBottomCol);
                ExpandShorthandRange(sStack[0], sStack[1], wTopRow, wTopCol, wBottomRow, wBottomCol);
                ioRef.top = wTopRow;
                ioRef.left = wTopCol;
                ioRef.bottom = wBottomRow;
                ioRef.right = wBottomCol;
                ioRef.resolved = true;
                return;
            }

            if (sStack.size() == 3 && sStack[0].Kind() == tKind::Sheet && sStack[1].Kind() == tKind::Cell
                && sStack[2].Kind() == tKind::Cell) {
                ioRef.kind = "range";
                ResolveColRowFromCellToken(sStack[1], sHost, wTopRow, wTopCol);
                ResolveColRowFromCellToken(sStack[2], sHost, wBottomRow, wBottomCol);
                ExpandShorthandRange(sStack[1], sStack[2], wTopRow, wTopCol, wBottomRow, wBottomCol);
                ioRef.top = wTopRow;
                ioRef.left = wTopCol;
                ioRef.bottom = wBottomRow;
                ioRef.right = wBottomCol;
                ioRef.resolved = true;
                return;
            }

            if (sStack.size() == 2 && sStack[0].Kind() == tKind::Sheet && sStack[1].Kind() == tKind::Cell) {
                ioRef.kind = "cell";
                ResolveColRowFromCellToken(sStack[1], sHost, ioRef.top, ioRef.left);
                ioRef.bottom = ioRef.top;
                ioRef.right = ioRef.left;
                ioRef.resolved = true;
                return;
            }

            if (StackHasNamedRangeToken(sStack)) {
                ioRef.kind = "name";
                const tLexerToken* wNameToken = nullptr;
                for (const tLexerToken& wToken : sStack) {
                    if (wToken.Kind() == tKind::Name || wToken.Kind() == tKind::ErrorName) {
                        wNameToken = &wToken;
                        break;
                    }
                }
                if (wNameToken != nullptr && sHost.WorkBook != nullptr) {
                    tCollectedRef wNamedRef = ioRef;
                    wNamedRef.text = wNameToken->Lexeme();
                    std::vector<tLexerToken> wNameStack;
                    wNameStack.push_back(*wNameToken);
                    ResolveCollectedRef(wNamedRef, sHost, wNameStack);
                    if (wNamedRef.resolved) {
                        ioRef = wNamedRef;
                        ioRef.text = BuildRefLexemeFromStack(sStack);
                    }
                }
            }
        }

        void PushCollectedRef(std::vector<tCollectedRef>& ioRefs, tCollectedRef sRef) {
            if (sRef.text.empty()) {
                return;
            }
            ioRefs.push_back(sRef);
        }

        void FlushRefStackCollect(std::vector<tCollectedRef>& ioRefs, const tHostContext& sHost,
                                  std::vector<tLexerToken>& ioStack) {
            if (ioStack.empty()) {
                return;
            }
            const tString wRefLexeme = BuildRefLexemeFromStack(ioStack);
            if (wRefLexeme.empty()) {
                ioStack.clear();
                return;
            }
            tCollectedRef wRef;
            wRef.text = wRefLexeme;
            wRef.kind = "ref";
            ResolveCollectedRef(wRef, sHost, ioStack);
            if (wRef.kind == "ref") {
                wRef.kind = (ioStack.size() >= 2 && ioStack.back().Kind() == tKind::Cell
                             && std::count_if(ioStack.begin(), ioStack.end(),
                                              [](const tLexerToken& sToken) {
                                                  return sToken.Kind() == tKind::Cell;
                                              })
                                 >= 2)
                                ? "range"
                                : "cell";
            }
            PushCollectedRef(ioRefs, wRef);
            ioStack.clear();
        }

        void CollectTableRef(std::vector<tCollectedRef>& ioRefs, const tHostContext& sHost,
                             const tLexerToken* sTableToken, const tLexerToken& sSquareToken) {
            tCollectedRef wRef;
            wRef.kind = "table";
            wRef.text = BuildTableRefText(sTableToken, sSquareToken);
            wRef.sheet = DefaultSheetName(sHost, sTableToken != nullptr ? nullptr : nullptr);
            if (ResolveTableStructuredRef(sTableToken, sSquareToken, sHost, wRef.sheet, wRef.top, wRef.left,
                                          wRef.bottom, wRef.right)) {
                wRef.resolved = true;
                if (wRef.top == wRef.bottom && wRef.left == wRef.right) {
                    wRef.kind = "cell";
                } else {
                    wRef.kind = "range";
                }
            } else if (sHost.HostSheet != nullptr && sTableToken == nullptr) {
                wRef.sheet = sHost.HostSheetName;
            } else if (sTableToken != nullptr) {
                wRef.sheet = sHost.HostSheetName;
            }
            PushCollectedRef(ioRefs, wRef);
        }

        void CollectRefsFromText(const tHostContext& sHost, tString sText, std::vector<tCollectedRef>& outRefs,
                                 tBool& outSyntaxError, tInt& outErrorLine, tInt& outErrorColumn) {
            outRefs.clear();
            outSyntaxError = false;
            outErrorLine = 0;
            outErrorColumn = 0;

            if (sText.empty()) {
                return;
            }
            if (!sText.empty() && sText[0] == '=') {
                sText.erase(0, 1);
            }
            if (sText.empty()) {
                return;
            }

            tLexer wLex(sText.c_str());
            tLocale* const wLocale = tApplication::Instance()->Locale();
            const tChar wDecimal = wLocale->Decimal();
            const tChar wArg = wLocale->Arg();
            wLex.SeparatorArg(wArg);
            wLex.SeparatorDecimal(wDecimal);

            std::vector<tLexerToken> wStackCell;
            tLexerToken wLexerToken = wLex.next();
            tBool wSkipNextLabelSquare = false;

            while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
                const tKind wKind = wLexerToken.Kind();
                const tLexerToken wNextToken = wLex.next();

                switch (wKind) {
                case tKind::Sheet:
                    wStackCell.push_back(wLexerToken);
                    break;
                case tKind::Cell:
                case tKind::Name:
                case tKind::ErrorRef:
                case tKind::ErrorName:
                    wStackCell.push_back(wLexerToken);
                    break;
                case tKind::Identifier:
                    if (!wStackCell.empty() && wStackCell.back().Kind() == tKind::Sheet
                        && !wNextToken.is(tKind::LeftParen)) {
                        tLexerToken wNameToken = wLexerToken;
                        wNameToken.Kind(tKind::Name);
                        wStackCell.push_back(wNameToken);
                    } else if (!wNextToken.is(tKind::LeftParen) && !wNextToken.is(tKind::LabelSquare)) {
                        tCollectedRef wNameRef;
                        wNameRef.kind = "name";
                        wNameRef.text = wLexerToken.Lexeme();
                        std::vector<tLexerToken> wNameStack;
                        tLexerToken wAsName = wLexerToken;
                        wAsName.Kind(tKind::Name);
                        wNameStack.push_back(wAsName);
                        ResolveCollectedRef(wNameRef, sHost, wNameStack);
                        PushCollectedRef(outRefs, wNameRef);
                    } else if (wNextToken.is(tKind::LabelSquare)) {
                        CollectTableRef(outRefs, sHost, &wLexerToken, wNextToken);
                        wSkipNextLabelSquare = true;
                    }
                    break;
                case tKind::LabelSquare:
                    if (!wSkipNextLabelSquare) {
                        CollectTableRef(outRefs, sHost, nullptr, wLexerToken);
                    }
                    wSkipNextLabelSquare = false;
                    break;
                default:
                    break;
                }

                if (IsRefBoundaryToken(wKind, wNextToken.Kind())) {
                    FlushRefStackCollect(outRefs, sHost, wStackCell);
                }

                wLexerToken = wNextToken;
            }

            FlushRefStackCollect(outRefs, sHost, wStackCell);

            if (wLexerToken.Kind() == tKind::Unexpected) {
                outSyntaxError = true;
                outErrorLine = wLex.Line();
                outErrorColumn = wLex.Column();
            }
        }

        tString CollectFormulaRefsJsonImpl(const tHostContext& sHost, tString sText) {
            std::vector<tCollectedRef> wRefs;
            tBool wSyntaxError = false;
            tInt wErrorLine = 0;
            tInt wErrorColumn = 0;
            CollectRefsFromText(sHost, sText, wRefs, wSyntaxError, wErrorLine, wErrorColumn);

            StringBuffer wBuffer;
            Writer<StringBuffer> wWriter(wBuffer);
            wWriter.StartObject();
            wWriter.Key("syntaxError");
            wWriter.Bool(wSyntaxError);
            wWriter.Key("errorLine");
            wWriter.Int(wErrorLine);
            wWriter.Key("errorColumn");
            wWriter.Int(wErrorColumn);
            wWriter.Key("refs");
            wWriter.StartArray();
            for (const tCollectedRef& wRef : wRefs) {
                wWriter.StartObject();
                wWriter.Key("kind");
                wWriter.String(wRef.kind.c_str());
                wWriter.Key("text");
                wWriter.String(wRef.text.c_str());
                wWriter.Key("sheet");
                wWriter.String(wRef.sheet.c_str());
                wWriter.Key("top");
                wWriter.Int(static_cast<int>(wRef.top));
                wWriter.Key("left");
                wWriter.Int(static_cast<int>(wRef.left));
                wWriter.Key("bottom");
                wWriter.Int(static_cast<int>(wRef.bottom));
                wWriter.Key("right");
                wWriter.Int(static_cast<int>(wRef.right));
                wWriter.Key("resolved");
                wWriter.Bool(wRef.resolved);
                wWriter.EndObject();
            }
            wWriter.EndArray();
            wWriter.EndObject();
            return wBuffer.GetString();
        }

    } // namespace

    tString QualifyRefsForSheet(tString sSheet, tString sText) {
        return TransformRefsForSheet(sSheet, sText, tTransformMode::Qualify);
    }

    tString StripTargetSheetFromRefs(tString sSheet, tString sText) {
        return TransformRefsForSheet(sSheet, sText, tTransformMode::Strip);
    }

    tString CollectFormulaRefsJson(tWorkBook* sWorkBook, tString sHostSheet, tIndex sHostRow, tIndex sHostCol,
                                   tString sText) {
        tHostContext wHost;
        wHost.WorkBook = sWorkBook;
        wHost.HostSheetName = NormalizeSheetName(sHostSheet);
        wHost.HostRow = sHostRow;
        wHost.HostCol = sHostCol;
        if (sWorkBook != nullptr && !wHost.HostSheetName.empty()) {
            wHost.HostSheet = sWorkBook->Sheet(wHost.HostSheetName);
        }
        tLocale* const wLocale = tApplication::Instance()->Locale();
        wHost.ArgSep = wLocale != nullptr ? wLocale->Arg() : ',';
        return CollectFormulaRefsJsonImpl(wHost, sText);
    }

} // namespace SkSpreadSheet
