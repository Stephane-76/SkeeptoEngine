//=============================================================================
// SkExcelTools.cpp
//=============================================================================
#include <SkExcelTools.hpp>
#include "SkExcelPugiXMLReader.hpp"
#include <SkTypesClass.hpp>
#include <SkFormatString.hpp>
#include <pugixml.hpp>
#include <cctype>
#include <cmath>
#include <cstring>
#include <vector>

namespace SkExcel {

    namespace {

    // Append UTF-8 for a BMP/supplementary code point (Excel _xHHHH_ / _xHHHHHHHH_ uses 4 hex digits only in practice).
    void AppendUtf8CodePoint(tString& sOut, unsigned sCp) {
        if (sCp <= 0x7FU) {
            sOut += static_cast<char>(sCp);
        } else if (sCp <= 0x7FFU) {
            sOut += static_cast<char>(0xC0 | static_cast<char>(sCp >> 6));
            sOut += static_cast<char>(0x80 | static_cast<char>(sCp & 0x3F));
        } else if (sCp <= 0xFFFFU) {
            sOut += static_cast<char>(0xE0 | static_cast<char>(sCp >> 12));
            sOut += static_cast<char>(0x80 | static_cast<char>((sCp >> 6) & 0x3F));
            sOut += static_cast<char>(0x80 | static_cast<char>(sCp & 0x3F));
        } else if (sCp <= 0x10FFFFU) {
            sOut += static_cast<char>(0xF0 | static_cast<char>(sCp >> 18));
            sOut += static_cast<char>(0x80 | static_cast<char>((sCp >> 12) & 0x3F));
            sOut += static_cast<char>(0x80 | static_cast<char>((sCp >> 6) & 0x3F));
            sOut += static_cast<char>(0x80 | static_cast<char>(sCp & 0x3F));
        }
    }

    void AppendSingleSpace(tString& sOut) {
        if (sOut.empty() || sOut.back() != ' ')
            sOut += ' ';
    }

    bool ParseHex4(const tString& s, tSize sStart, unsigned& outVal) {
        outVal = 0;
        for (tSize k = 0; k < 4; ++k) {
            char c = s[sStart + k];
            unsigned d;
            if (c >= '0' && c <= '9')
                d = static_cast<unsigned>(c - '0');
            else if (c >= 'A' && c <= 'F')
                d = static_cast<unsigned>(c - 'A' + 10);
            else if (c >= 'a' && c <= 'f')
                d = static_cast<unsigned>(c - 'a' + 10);
            else
                return false;
            outVal = (outVal << 4) | d;
        }
        return true;
    }

    tString TrimAsciiSpaces(const tString& sText) {
        tSize wBegin = 0;
        while (wBegin < sText.size() && std::isspace(static_cast<unsigned char>(sText[wBegin]))) {
            ++wBegin;
        }
        tSize wEnd = sText.size();
        while (wEnd > wBegin && std::isspace(static_cast<unsigned char>(sText[wEnd - 1]))) {
            --wEnd;
        }
        return sText.substr(wBegin, wEnd - wBegin);
    }

    tString ToUpperAscii(const tString& sText) {
        tString wOut;
        wOut.reserve(sText.size());
        for (const char wChar : sText) {
            wOut += static_cast<char>(std::toupper(static_cast<unsigned char>(wChar)));
        }
        return wOut;
    }

    tString EscapeFormulaStringLiteral(const tString& sText) {
        tString wOut;
        wOut.reserve(sText.size() + 4);
        for (const char wChar : sText) {
            if (wChar == '"') {
                wOut += "\"\"";
            } else {
                wOut += wChar;
            }
        }
        return wOut;
    }

    void SplitSheetPrefixFromRef(const tString& sRef, tString& oSheetPrefix, tString& oBody) {
        oSheetPrefix.clear();
        oBody = sRef;
        if (sRef.empty()) {
            return;
        }
        if (sRef.front() == '\'') {
            for (tSize wIndex = 1; wIndex < sRef.size(); ++wIndex) {
                if (sRef[wIndex] != '\'') {
                    continue;
                }
                if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '\'') {
                    ++wIndex;
                    continue;
                }
                if (wIndex + 1 < sRef.size() && sRef[wIndex + 1] == '!') {
                    oSheetPrefix = sRef.substr(0, wIndex + 2);
                    oBody = sRef.substr(wIndex + 2);
                    return;
                }
                break;
            }
            return;
        }
        const tSize wBangPos = sRef.rfind('!');
        if (wBangPos != tString::npos) {
            oSheetPrefix = sRef.substr(0, wBangPos + 1);
            oBody = sRef.substr(wBangPos + 1);
        }
    }

    tBool IsWholeColumnShorthandBody(const tString& sBody) {
        if (sBody.empty() || sBody.size() > 3) {
            return false;
        }
        for (const char wChar : sBody) {
            if (!std::isalpha(static_cast<unsigned char>(wChar))) {
                return false;
            }
        }
        return true;
    }

    tBool LooksLikeA1CellRefBody(const tString& sBody) {
        if (sBody.empty()) {
            return false;
        }
        tBool wHasLetter = false;
        tBool wHasDigit = false;
        for (const char wChar : sBody) {
            if (wChar == '$') {
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(wChar))) {
                wHasLetter = true;
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(wChar))) {
                wHasDigit = true;
                continue;
            }
            return false;
        }
        return wHasLetter && wHasDigit;
    }

    tBool LooksLikeA1CellRef(const tString& sRefText) {
        tString wSheetPrefix;
        tString wBody;
        SplitSheetPrefixFromRef(sRefText, wSheetPrefix, wBody);
        return LooksLikeA1CellRefBody(wBody);
    }

    tIndex NormalizeSheetLastRow(tIndex sLastRow) {
        return sLastRow >= 1 ? sLastRow : 1;
    }

    tString SheetNameFromPrefix(const tString& sSheetPrefix) {
        if (sSheetPrefix.empty()) {
            return {};
        }
        tString wName = sSheetPrefix;
        if (!wName.empty() && wName.back() == '!') {
            wName.pop_back();
        }
        if (wName.size() >= 2 && wName.front() == '\'' && wName.back() == '\'') {
            wName = wName.substr(1, wName.size() - 2);
            tString wUnescaped;
            wUnescaped.reserve(wName.size());
            for (tSize wIndex = 0; wIndex < wName.size(); ++wIndex) {
                if (wName[wIndex] == '\'' && wIndex + 1 < wName.size() && wName[wIndex + 1] == '\'') {
                    wUnescaped += '\'';
                    ++wIndex;
                } else {
                    wUnescaped += wName[wIndex];
                }
            }
            wName.swap(wUnescaped);
        }
        return wName;
    }

    tIndex LastRowForSheetPrefix(tApi& sApi, const tString& sSheetPrefix, tSheet* sFormulaSheet) {
        if (sSheetPrefix.empty()) {
            if (sFormulaSheet != nullptr) {
                return NormalizeSheetLastRow(sFormulaSheet->LastRow());
            }
            return 1;
        }
        const tString wSheetName = SheetNameFromPrefix(sSheetPrefix);
        if (wSheetName.empty()) {
            return 1;
        }
        if (tSheet* wSheet = sApi.Sheet(wSheetName)) {
            return NormalizeSheetLastRow(wSheet->LastRow());
        }
        if (sFormulaSheet != nullptr && sFormulaSheet->Name() == wSheetName) {
            return NormalizeSheetLastRow(sFormulaSheet->LastRow());
        }
        return 1;
    }

    tString WholeColumnRangeFromShorthand(const tString& sRefText, tIndex sLastRow) {
        tString wSheetPrefix;
        tString wBody;
        SplitSheetPrefixFromRef(sRefText, wSheetPrefix, wBody);
        if (!IsWholeColumnShorthandBody(wBody)) {
            return {};
        }
        if (sLastRow < 1) {
            return {};
        }
        tString wCol;
        wCol.reserve(wBody.size());
        for (const char wChar : wBody) {
            wCol += static_cast<char>(std::toupper(static_cast<unsigned char>(wChar)));
        }
        return wSheetPrefix + "$" + wCol + "$1:$" + wCol + "$" + std::to_string(sLastRow);
    }

    tString BoundedWholeColumnRangeRef(const tString& sSheetPrefix, const tString& sColLetters, tIndex sLastRow) {
        if (sColLetters.empty() || sLastRow < 1) {
            return {};
        }
        return sSheetPrefix + "$" + sColLetters + "$1:$" + sColLetters + "$" + std::to_string(sLastRow);
    }

    tString BoundedColumnSpanRangeRef(const tString& sSheetPrefix,
                                      const tString& sColLeft,
                                      const tString& sColRight,
                                      tIndex sLastRow) {
        if (sColLeft.empty() || sColRight.empty() || sLastRow < 1) {
            return {};
        }
        return sSheetPrefix + "$" + sColLeft + "$1:$" + sColRight + "$" + std::to_string(sLastRow);
    }

    tBool TryParseWholeColumnLetters(const tString& sText, tSize sPos, tSize& oEnd, tString& oColLetters) {
        oColLetters.clear();
        tSize wPos = sPos;
        if (wPos < sText.size() && sText[wPos] == '$') {
            ++wPos;
        }
        while (wPos < sText.size() && std::isalpha(static_cast<unsigned char>(sText[wPos]))) {
            oColLetters += static_cast<char>(std::toupper(static_cast<unsigned char>(sText[wPos])));
            ++wPos;
        }
        if (oColLetters.empty() || oColLetters.size() > 3) {
            return false;
        }
        oEnd = wPos;
        return true;
    }

    tBool TryParseSheetPrefixAt(const tString& sText, tSize sPos, tString& oSheetPrefix, tSize& oNextPos) {
        oSheetPrefix.clear();
        oNextPos = sPos;
        if (sPos >= sText.size()) {
            return false;
        }
        if (sText[sPos] == '\'') {
            for (tSize wIndex = sPos + 1; wIndex < sText.size(); ++wIndex) {
                if (sText[wIndex] != '\'') {
                    continue;
                }
                if (wIndex + 1 < sText.size() && sText[wIndex + 1] == '\'') {
                    ++wIndex;
                    continue;
                }
                if (wIndex + 1 < sText.size() && sText[wIndex + 1] == '!') {
                    oSheetPrefix = sText.substr(sPos, wIndex - sPos + 2);
                    oNextPos = wIndex + 2;
                    return true;
                }
                break;
            }
            return false;
        }
        for (tSize wIndex = sPos; wIndex < sText.size(); ++wIndex) {
            if (sText[wIndex] == '!') {
                oSheetPrefix = sText.substr(sPos, wIndex - sPos + 1);
                oNextPos = wIndex + 1;
                return true;
            }
        }
        return false;
    }

    tBool TryParseWholeColumnRefAt(const tString& sText, tSize sPos, tString& oSheetPrefix, tString& oColLetters, tSize& oEnd) {
        oSheetPrefix.clear();
        oColLetters.clear();
        oEnd = sPos;
        tSize wPos = sPos;
        tString wMaybePrefix;
        tSize wAfterPrefix = wPos;
        if (TryParseSheetPrefixAt(sText, wPos, wMaybePrefix, wAfterPrefix)) {
            oSheetPrefix = wMaybePrefix;
            wPos = wAfterPrefix;
        }
        tString wCol1;
        tString wCol2;
        tSize wEnd1 = 0;
        tSize wEnd2 = 0;
        if (!TryParseWholeColumnLetters(sText, wPos, wEnd1, wCol1)) {
            return false;
        }
        if (wEnd1 >= sText.size() || sText[wEnd1] != ':') {
            return false;
        }
        if (!TryParseWholeColumnLetters(sText, wEnd1 + 1, wEnd2, wCol2)) {
            return false;
        }
        if (wCol1 != wCol2) {
            return false;
        }
        if (wEnd2 < sText.size() && std::isdigit(static_cast<unsigned char>(sText[wEnd2]))) {
            return false;
        }
        oColLetters = wCol1;
        oEnd = wEnd2;
        return true;
    }

    // Excel column span shorthand: SUM(J:L) -> $J$1:$L$lastRow (distinct columns).
    tBool TryParseColumnSpanRefAt(const tString& sText,
                                  tSize sPos,
                                  tString& oSheetPrefix,
                                  tString& oColLeft,
                                  tString& oColRight,
                                  tSize& oEnd) {
        oSheetPrefix.clear();
        oColLeft.clear();
        oColRight.clear();
        oEnd = sPos;
        tSize wPos = sPos;
        tString wMaybePrefix;
        tSize wAfterPrefix = wPos;
        if (TryParseSheetPrefixAt(sText, wPos, wMaybePrefix, wAfterPrefix)) {
            oSheetPrefix = wMaybePrefix;
            wPos = wAfterPrefix;
        }
        tSize wEnd1 = 0;
        tSize wEnd2 = 0;
        if (!TryParseWholeColumnLetters(sText, wPos, wEnd1, oColLeft)) {
            return false;
        }
        if (wEnd1 >= sText.size() || sText[wEnd1] != ':') {
            return false;
        }
        if (!TryParseWholeColumnLetters(sText, wEnd1 + 1, wEnd2, oColRight)) {
            return false;
        }
        if (oColLeft == oColRight) {
            return false;
        }
        if (wEnd2 < sText.size() && std::isdigit(static_cast<unsigned char>(sText[wEnd2]))) {
            return false;
        }
        oEnd = wEnd2;
        return true;
    }

    tString& BoundWholeColumnRefsInFormulaImpl(tString& sString, tApi& sApi, tSheet* sFormulaSheet) {
        if (sString.empty()) {
            return sString;
        }
        tString wResult;
        wResult.reserve(sString.size() + 32);
        tBool wInString = false;
        for (tSize wIndex = 0; wIndex < sString.size(); ) {
            const char wChar = sString[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                wResult += wChar;
                ++wIndex;
                continue;
            }
            if (!wInString) {
                tString wSheetPrefix;
                tString wColLetters;
                tSize wEnd = 0;
                if (TryParseWholeColumnRefAt(sString, wIndex, wSheetPrefix, wColLetters, wEnd)) {
                    const tIndex wLastRow = LastRowForSheetPrefix(sApi, wSheetPrefix, sFormulaSheet);
                    // LastRow < 2 means the target sheet is still empty (imported later
                    // in ProcessSheet). Collapsing A:A to $A$1:$A$1 breaks MATCH/INDEX.
                    if (wLastRow >= 2) {
                        wResult += BoundedWholeColumnRangeRef(wSheetPrefix, wColLetters, wLastRow);
                        wIndex = wEnd;
                        continue;
                    }
                }
                tString wColLeft;
                tString wColRight;
                if (TryParseColumnSpanRefAt(sString, wIndex, wSheetPrefix, wColLeft, wColRight, wEnd)) {
                    const tIndex wLastRow = LastRowForSheetPrefix(sApi, wSheetPrefix, sFormulaSheet);
                    if (wLastRow >= 2) {
                        wResult += BoundedColumnSpanRangeRef(wSheetPrefix, wColLeft, wColRight, wLastRow);
                        wIndex = wEnd;
                        continue;
                    }
                }
            }
            wResult += wChar;
            ++wIndex;
        }
        sString.swap(wResult);
        return sString;
    }

    void AppendFrenchL1C1AxisSpec(const tString& sBody, tSize& ioPos, tString& oOut, tBool sAllowImplicitZero) {
        const tSize n = sBody.size();
        if (ioPos >= n) {
            if (sAllowImplicitZero) {
                oOut += "[0]";
            }
            return;
        }
        if (sBody[ioPos] == '[') {
            while (ioPos < n) {
                oOut += sBody[ioPos];
                if (sBody[ioPos] == ']') {
                    ++ioPos;
                    break;
                }
                ++ioPos;
            }
            return;
        }
        if (sBody[ioPos] == '(') {
            ++ioPos;
            tBool wNegative = false;
            if (ioPos < n && sBody[ioPos] == '+') {
                ++ioPos;
            } else if (ioPos < n && sBody[ioPos] == '-') {
                wNegative = true;
                ++ioPos;
            }
            tInt wOffset = 0;
            tBool wHasDigits = false;
            while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
                wHasDigits = true;
                wOffset = wOffset * 10 + static_cast<tInt>(sBody[ioPos] - '0');
                ++ioPos;
            }
            if (!wHasDigits || ioPos >= n || sBody[ioPos] != ')') {
                return;
            }
            ++ioPos;
            if (wNegative) {
                wOffset = -wOffset;
            }
            oOut += '[';
            oOut += std::to_string(wOffset);
            oOut += ']';
            return;
        }
        if (std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
            while (ioPos < n && std::isdigit(static_cast<unsigned char>(sBody[ioPos]))) {
                oOut += sBody[ioPos++];
            }
            return;
        }
        if (sAllowImplicitZero) {
            const char wNext = sBody[ioPos];
            const char wNextUpper = static_cast<char>(std::toupper(static_cast<unsigned char>(wNext)));
            if (wNextUpper == 'L' || wNextUpper == 'R' || wNextUpper == 'C' || wNext == ':') {
                oOut += "[0]";
            }
        }
    }

    tString ConvertFrenchL1C1AddressToR1C1(const tString& sAddr) {
        tString wSheetPrefix;
        tString wBody;
        SplitSheetPrefixFromRef(sAddr, wSheetPrefix, wBody);
        if (wBody.empty()) {
            return sAddr;
        }

        tString wOut;
        wOut.reserve(sAddr.size() + 16);
        wOut += wSheetPrefix;

        for (tSize wIndex = 0; wIndex < wBody.size(); ) {
            const char wCharUpper =
                static_cast<char>(std::toupper(static_cast<unsigned char>(wBody[wIndex])));
            if (wCharUpper == 'L' || wCharUpper == 'R') {
                wOut += 'R';
                ++wIndex;
                AppendFrenchL1C1AxisSpec(wBody, wIndex, wOut, true);
            } else if (wCharUpper == 'C') {
                wOut += 'C';
                ++wIndex;
                AppendFrenchL1C1AxisSpec(wBody, wIndex, wOut, true);
            } else if (wCharUpper == ':') {
                wOut += ':';
                ++wIndex;
            } else {
                return sAddr;
            }
        }
        return wOut;
    }

    static tString TrimFormulaToken(const tString& sToken) {
        tSize wStart = 0;
        while (wStart < sToken.size() &&
               std::isspace(static_cast<unsigned char>(sToken[wStart]))) {
            ++wStart;
        }
        tSize wEnd = sToken.size();
        while (wEnd > wStart && std::isspace(static_cast<unsigned char>(sToken[wEnd - 1]))) {
            --wEnd;
        }
        return sToken.substr(wStart, wEnd - wStart);
    }

    static void SplitTopLevelCommaList(const tString& sList, std::vector<tString>& oParts) {
        oParts.clear();
        tSize wDepthParen = 0;
        tSize wDepthBracket = 0;
        tBool wInString = false;
        tSize wStart = 0;
        for (tSize wIndex = 0; wIndex < sList.size(); ++wIndex) {
            const char wChar = sList[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                continue;
            }
            if (wInString) {
                continue;
            }
            if (wChar == '(') {
                ++wDepthParen;
                continue;
            }
            if (wChar == ')' && wDepthParen > 0) {
                --wDepthParen;
                continue;
            }
            if (wChar == '[') {
                ++wDepthBracket;
                continue;
            }
            if (wChar == ']' && wDepthBracket > 0) {
                --wDepthBracket;
                continue;
            }
            if (wChar == ',' && wDepthParen == 0 && wDepthBracket == 0) {
                oParts.push_back(TrimFormulaToken(sList.substr(wStart, wIndex - wStart)));
                wStart = wIndex + 1;
            }
        }
        oParts.push_back(TrimFormulaToken(sList.substr(wStart)));
    }

    static tSize FindMatchingClosingParen(const tString& sText, tSize sOpenParen) {
        if (sOpenParen >= sText.size() || sText[sOpenParen] != '(') {
            return tString::npos;
        }
        tSize wDepthParen = 0;
        tSize wDepthBracket = 0;
        tBool wInString = false;
        for (tSize wIndex = sOpenParen; wIndex < sText.size(); ++wIndex) {
            const char wChar = sText[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                continue;
            }
            if (wInString) {
                continue;
            }
            if (wChar == '(') {
                ++wDepthParen;
                continue;
            }
            if (wChar == ')') {
                --wDepthParen;
                if (wDepthParen == 0) {
                    return wIndex;
                }
                continue;
            }
            if (wChar == '[') {
                ++wDepthBracket;
                continue;
            }
            if (wChar == ']' && wDepthBracket > 0) {
                --wDepthBracket;
            }
        }
        return tString::npos;
    }

  // Excel SUM often nests comma-union groups: SUM(A,(B,(C,D))) — flatten to SUM(A,B,C,D).
    static tString FlattenExcelCommaUnionGroup(tString sGroup) {
        sGroup = TrimFormulaToken(sGroup);
        while (!sGroup.empty() && sGroup.front() == '(') {
            const tSize wClose = FindMatchingClosingParen(sGroup, 0);
            if (wClose == tString::npos || wClose + 1 != sGroup.size()) {
                break;
            }
            sGroup = TrimFormulaToken(sGroup.substr(1, wClose - 1));
        }
        std::vector<tString> wParts;
        SplitTopLevelCommaList(sGroup, wParts);
        if (wParts.empty()) {
            return sGroup;
        }
        tString wFlat;
        for (const tString& wPart : wParts) {
            if (wPart.empty()) {
                continue;
            }
            if (!wFlat.empty()) {
                wFlat += ',';
            }
            if (!wPart.empty() && wPart.front() == '(') {
                wFlat += FlattenExcelCommaUnionGroup(wPart);
            } else {
                wFlat += wPart;
            }
        }
        return wFlat;
    }

    static tBool IsSumIdentifierAt(const tString& sFormula, tSize sIndex) {
        if (sIndex + 3 > sFormula.size()) {
            return false;
        }
        if (std::toupper(static_cast<unsigned char>(sFormula[sIndex])) != 'S' ||
            std::toupper(static_cast<unsigned char>(sFormula[sIndex + 1])) != 'U' ||
            std::toupper(static_cast<unsigned char>(sFormula[sIndex + 2])) != 'M') {
            return false;
        }
        if (sIndex > 0) {
            const unsigned char wPrev = static_cast<unsigned char>(sFormula[sIndex - 1]);
            if (std::isalnum(wPrev) || wPrev == '_') {
                return false;
            }
        }
        if (sIndex + 3 < sFormula.size()) {
            const unsigned char wNext = static_cast<unsigned char>(sFormula[sIndex + 3]);
            if (std::isalnum(wNext) || wNext == '_') {
                return false;
            }
        }
        return true;
    }

    static void FlattenExcelSumNestedUnionArgs(tString& sFormula) {
        tSize wSearchFrom = 0;
        while (wSearchFrom < sFormula.size()) {
            if (!IsSumIdentifierAt(sFormula, wSearchFrom)) {
                ++wSearchFrom;
                continue;
            }
            tSize wOpenParen = wSearchFrom + 3;
            while (wOpenParen < sFormula.size() &&
                   std::isspace(static_cast<unsigned char>(sFormula[wOpenParen]))) {
                ++wOpenParen;
            }
            if (wOpenParen >= sFormula.size() || sFormula[wOpenParen] != '(') {
                ++wSearchFrom;
                continue;
            }
            const tSize wCloseParen = FindMatchingClosingParen(sFormula, wOpenParen);
            if (wCloseParen == tString::npos) {
                break;
            }
            const tString wArgs = sFormula.substr(wOpenParen + 1, wCloseParen - wOpenParen - 1);
            const tString wFlatArgs = FlattenExcelCommaUnionGroup(wArgs);
            if (wFlatArgs != wArgs) {
                tString wNewFormula;
                wNewFormula.reserve(sFormula.size() + wFlatArgs.size());
                wNewFormula += sFormula.substr(0, wOpenParen + 1);
                wNewFormula += wFlatArgs;
                wNewFormula += sFormula.substr(wCloseParen);
                sFormula.swap(wNewFormula);
                wSearchFrom = wOpenParen + 1 + wFlatArgs.size();
                continue;
            }
            wSearchFrom = wCloseParen + 1;
        }
    }

    void RemoveExcelImplicitAtOperators(tString& sFormula) {
        tString wOut;
        wOut.reserve(sFormula.size());
        tBool wInString = false;
        for (tSize wIndex = 0; wIndex < sFormula.size(); ++wIndex) {
            const char wChar = sFormula[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                wOut += wChar;
                continue;
            }
            if (!wInString && wChar == '@') {
                tSize wNext = wIndex + 1;
                while (wNext < sFormula.size() &&
                       std::isspace(static_cast<unsigned char>(sFormula[wNext]))) {
                    ++wNext;
                }
                if (wNext < sFormula.size()) {
                    const unsigned char wNextChar =
                        static_cast<unsigned char>(sFormula[wNext]);
                    if (std::isalpha(wNextChar) || wNextChar == '_' || wNextChar == '$') {
                        continue;
                    }
                }
            }
            wOut += wChar;
        }
        sFormula.swap(wOut);
    }

    tBool MatchesIndirectKeywordAt(const tString& sFormula, tSize sIndex, tSize& oKeywordLen) {
        static const char* const kIndirect = "INDIRECT";
        if (sIndex > 0) {
            const unsigned char wPrev = static_cast<unsigned char>(sFormula[sIndex - 1]);
            if (std::isalnum(wPrev) || wPrev == '_') {
                return false;
            }
        }
        for (tSize wOffset = 0; kIndirect[wOffset] != '\0'; ++wOffset) {
            if (sIndex + wOffset >= sFormula.size()) {
                return false;
            }
            if (std::toupper(static_cast<unsigned char>(sFormula[sIndex + wOffset])) !=
                static_cast<unsigned char>(kIndirect[wOffset])) {
                return false;
            }
        }
        oKeywordLen = 8;
        return true;
    }

    tBool IsR1C1StyleSecondArg(const tString& sArg) {
        const tString wTrimmed = ToUpperAscii(TrimAsciiSpaces(sArg));
        return wTrimmed == "0" || wTrimmed == "FALSE" || wTrimmed == "FAUX";
    }

    tBool ParseIndirectCall(const tString& sFormula,
                            tSize sKeywordIndex,
                            tSize& oCallEnd,
                            tString& oRefText,
                            tBool& oHasSecondArg,
                            tString& oSecondArg) {
        oRefText.clear();
        oSecondArg.clear();
        oHasSecondArg = false;
        oCallEnd = sKeywordIndex;

        tSize wIndex = sKeywordIndex + 8;
        while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
            ++wIndex;
        }
        if (wIndex >= sFormula.size() || sFormula[wIndex] != '(') {
            return false;
        }
        ++wIndex;
        while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
            ++wIndex;
        }
        if (wIndex >= sFormula.size() || sFormula[wIndex] != '"') {
            return false;
        }
        ++wIndex;
        while (wIndex < sFormula.size()) {
            if (sFormula[wIndex] == '"') {
                if (wIndex + 1 < sFormula.size() && sFormula[wIndex + 1] == '"') {
                    oRefText += '"';
                    wIndex += 2;
                    continue;
                }
                ++wIndex;
                break;
            }
            oRefText += sFormula[wIndex++];
        }
        while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
            ++wIndex;
        }
        if (wIndex < sFormula.size() && (sFormula[wIndex] == ',' || sFormula[wIndex] == ';')) {
            ++wIndex;
            oHasSecondArg = true;
            while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
                ++wIndex;
            }
            const tSize wArgStart = wIndex;
            tInt wDepth = 0;
            while (wIndex < sFormula.size()) {
                const char wChar = sFormula[wIndex];
                if (wChar == '(') {
                    ++wDepth;
                } else if (wChar == ')') {
                    if (wDepth == 0) {
                        break;
                    }
                    --wDepth;
                } else if ((wChar == ',' || wChar == ';') && wDepth == 0) {
                    break;
                }
                ++wIndex;
            }
            oSecondArg = sFormula.substr(wArgStart, wIndex - wArgStart);
        }
        while (wIndex < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[wIndex]))) {
            ++wIndex;
        }
        if (wIndex >= sFormula.size() || sFormula[wIndex] != ')') {
            return false;
        }
        ++wIndex;
        oCallEnd = wIndex;
        return true;
    }

    tString BuildIndirectA1Call(const tString& sRefText) {
        return tString("INDIRECT(\"") + EscapeFormulaStringLiteral(sRefText) + "\",TRUE)";
    }

    // French INSIDE / Excel L1C1: INDIRECT("C",FALSE) is the current column (C[0]), not A1 column C.
    tBool IsFrenchColumnAxisBody(const tString& sBody) {
        if (sBody.empty() || std::toupper(static_cast<unsigned char>(sBody[0])) != 'C') {
            return false;
        }
        const tString wConverted = ConvertFrenchL1C1AddressToR1C1(sBody);
        if (wConverted.empty() || std::toupper(static_cast<unsigned char>(wConverted[0])) != 'C') {
            return false;
        }
        for (tSize wIndex = 1; wIndex < wConverted.size(); ++wIndex) {
            const char wChar = wConverted[wIndex];
            if (wChar == '[' || wChar == ']' || wChar == '-' || wChar == '+' ||
                std::isdigit(static_cast<unsigned char>(wChar))) {
                continue;
            }
            return false;
        }
        return true;
    }

    tBool ParseR1C1ColumnOnlyOffset(const tString& sR1c1, tInt& oColOffset) {
        if (sR1c1.empty() || std::toupper(static_cast<unsigned char>(sR1c1[0])) != 'C') {
            return false;
        }
        tSize wIndex = 1;
        oColOffset = 0;
        if (wIndex >= sR1c1.size()) {
            return true;
        }
        if (sR1c1[wIndex] != '[') {
            return false;
        }
        ++wIndex;
        tInt wSign = 1;
        if (wIndex < sR1c1.size() && sR1c1[wIndex] == '-') {
            wSign = -1;
            ++wIndex;
        } else if (wIndex < sR1c1.size() && sR1c1[wIndex] == '+') {
            ++wIndex;
        }
        tInt wValue = 0;
        tBool wHasDigits = false;
        while (wIndex < sR1c1.size() && std::isdigit(static_cast<unsigned char>(sR1c1[wIndex]))) {
            wHasDigits = true;
            wValue = wValue * 10 + static_cast<tInt>(sR1c1[wIndex] - '0');
            ++wIndex;
        }
        if (!wHasDigits || wIndex >= sR1c1.size() || sR1c1[wIndex] != ']') {
            return false;
        }
        ++wIndex;
        if (wIndex != sR1c1.size()) {
            return false;
        }
        oColOffset = wSign * wValue;
        return true;
    }

    tString BoundedColumnRangeFromFrenchColumnAxis(const tString& sRefText,
                                                   tInt sFormulaCol,
                                                   tIndex sLastRow) {
        tString wSheetPrefix;
        tString wBody;
        SplitSheetPrefixFromRef(sRefText, wSheetPrefix, wBody);
        if (!IsFrenchColumnAxisBody(wBody) || sLastRow < 1 || sFormulaCol < 1) {
            return {};
        }
        const tString wColAxisR1C1 = ConvertFrenchL1C1AddressToR1C1(wBody);
        tInt wColOffset = 0;
        if (!ParseR1C1ColumnOnlyOffset(wColAxisR1C1, wColOffset)) {
            return {};
        }
        const tInt wTargetCol = sFormulaCol + wColOffset;
        if (wTargetCol < 1) {
            return {};
        }
        return BoundedWholeColumnRangeRef(wSheetPrefix, Base10ToAlpha(wTargetCol), sLastRow);
    }

    tString BoundedColumnRangeFromFrenchColumnAxisR1C1(const tString& sRefText,
                                                       tInt sFormulaCol,
                                                       tIndex sLastRow) {
        tString wSheetPrefix;
        tString wBody;
        SplitSheetPrefixFromRef(sRefText, wSheetPrefix, wBody);
        if (!IsFrenchColumnAxisBody(wBody) || sLastRow < 1 || sFormulaCol < 1) {
            return {};
        }
        const tString wColAxisR1C1 = ConvertFrenchL1C1AddressToR1C1(wBody);
        tInt wColOffset = 0;
        if (!ParseR1C1ColumnOnlyOffset(wColAxisR1C1, wColOffset)) {
            return {};
        }
        const tInt wTargetCol = sFormulaCol + wColOffset;
        if (wTargetCol < 1) {
            return {};
        }
        tString wRange = tString("R1C") + std::to_string(wTargetCol) + ":R" + std::to_string(sLastRow) +
                           "C" + std::to_string(wTargetCol);
        return wSheetPrefix + wRange;
    }

    tString& TransformFormulaIndirectNotationImpl(tString& sString,
                                                  tBool sExpandWholeColumnShorthand,
                                                  tApi* sApi,
                                                  tSheet* sFormulaSheet) {
        if (sString.empty()) {
            return sString;
        }
        RemoveExcelImplicitAtOperators(sString);
        if (!kSkExcelTransformIndirectEnabled) {
            return sString;
        }

        tString wResult;
        wResult.reserve(sString.size() + 32);
        tSize wIndex = 0;
        while (wIndex < sString.size()) {
            tSize wKeywordLen = 0;
            if (MatchesIndirectKeywordAt(sString, wIndex, wKeywordLen)) {
                tSize wCallEnd = 0;
                tString wRefText;
                tBool wHasSecondArg = false;
                tString wSecondArg;
                if (ParseIndirectCall(sString, wIndex, wCallEnd, wRefText, wHasSecondArg, wSecondArg)) {
                    const tBool wR1C1 = wHasSecondArg && IsR1C1StyleSecondArg(wSecondArg);
                    tString wReplacement;
                    if (wR1C1) {
                        {
                            tString wSheetPrefix;
                            tString wBody;
                            SplitSheetPrefixFromRef(wRefText, wSheetPrefix, wBody);
                            // Leave French column-axis refs (INDIRECT("C",FALSE)) for per-cell materialization.
                            if (IsFrenchColumnAxisBody(wBody)) {
                                wReplacement.clear();
                            } else if (sExpandWholeColumnShorthand && sApi != nullptr &&
                                       IsWholeColumnShorthandBody(wBody)) {
                                const tIndex wLastRow =
                                    LastRowForSheetPrefix(*sApi, wSheetPrefix, sFormulaSheet);
                                wReplacement = WholeColumnRangeFromShorthand(wRefText, wLastRow);
                            }
                        }
                        // Keep French L1C1 cell refs (LC(-1), L13C3, …) literal: runtime INDIRECT resolves them
                        // relative to the formula cell (shared formulas stay identical on every row).
                        if (wReplacement.empty() && LooksLikeA1CellRef(wRefText)) {
                            // Excel may pass A1 addresses with a1=FALSE; SkSpreadSheet expects TRUE.
                            wReplacement = BuildIndirectA1Call(wRefText);
                        }
                    }
                    if (!wReplacement.empty()) {
                        wResult += wReplacement;
                        wIndex = wCallEnd;
                        continue;
                    }
                }
            }
            wResult += sString[wIndex++];
        }
        sString.swap(wResult);
        return sString;
    }

    } // namespace

    tBool FormulaContainsStructuredTableRef(const tString& sFormula) {
        return sFormula.find("[[#") != tString::npos
            || sFormula.find("[[@") != tString::npos
            || sFormula.find("[#This Row]") != tString::npos
            || sFormula.find("[#Headers]") != tString::npos;
    }

    tBool FormulaUsesR1C1CellRefs(const tString& sFormula) {
        tBool wInString = false;
        for (tSize wIndex = 0; wIndex + 2 < sFormula.size(); ++wIndex) {
            const char wChar = sFormula[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                continue;
            }
            if (wInString) {
                continue;
            }
            if (wChar == 'R' && sFormula[wIndex + 1] == '1' && sFormula[wIndex + 2] == 'C') {
                return true;
            }
            if (wChar == 'R' && sFormula[wIndex + 1] == '[') {
                return true;
            }
        }
        return false;
    }

    tBool FormulaContainsIndirect(const tString& sFormula) {
        tBool wInString = false;
        for (tSize wIndex = 0; wIndex < sFormula.size(); ++wIndex) {
            const char wChar = sFormula[wIndex];
            if (wChar == '"') {
                wInString = !wInString;
                continue;
            }
            if (wInString) {
                continue;
            }
            tSize wKeywordLen = 0;
            if (MatchesIndirectKeywordAt(sFormula, wIndex, wKeywordLen)) {
                return true;
            }
        }
        return false;
    }

    void SetCellFormulaAsText(tCell* sCell, const tString& sFormulaText) {
        if (sCell == nullptr) {
            return;
        }
        tString wText = sFormulaText;
        if (!wText.empty() && wText.front() != '=') {
            wText = "=" + wText;
        }
        sCell->ClearFormulaAndVariant();
        sCell->Value(tVariant(wText.c_str()));
    }

    // Excel stores line breaks in XML as escaped control chars, e.g. _x000a_ (LF), _x000d_ (CR), _x0009_ (tab).
    // Normalize those and real \n \r \t to a single ASCII space (collapse runs).
    tString& TrimSpacesBeforeNewline(tString& sString) {
        tString wOut;
        wOut.reserve(sString.size());
        const tSize n = sString.size();
        for (tSize i = 0; i < n; ) {
            // Pattern _xHHHH_ (7 chars: _ x + 4 hex + _) — OOXML / Excel escaped Unicode
            if (i + 7 <= n && sString[i] == '_' && sString[i + 1] == 'x') {
                unsigned wCp = 0;
                if (ParseHex4(sString, i + 2, wCp) && sString[i + 6] == '_') {
                    const bool wIsControlWs =
                        (wCp == 0x09 || wCp == 0x0A || wCp == 0x0D || wCp == 0x0B || wCp == 0x0C ||
                         (wCp < 0x20U && wCp != 0));
                    if (wIsControlWs) {
                        AppendSingleSpace(wOut);
                        i += 7;
                        continue;
                    }
                    AppendUtf8CodePoint(wOut, wCp);
                    i += 7;
                    continue;
                }
            }
            const unsigned char wCh = static_cast<unsigned char>(sString[i]);
            if (wCh == '\n' || wCh == '\r' || wCh == '\t') {
                AppendSingleSpace(wOut);
                ++i;
                continue;
            }
            wOut += sString[i];
            ++i;
        }
        // Trim trailing whitespace produced by normalization
        while (!wOut.empty() && (wOut.back() == ' ' || wOut.back() == '\t'))
            wOut.pop_back();
        sString.swap(wOut);
        return sString;
    }

    tString& TransformFormulaIndirectNotation(tString& sString) {
        return TransformFormulaIndirectNotationImpl(sString, false, nullptr, nullptr);
    }

    tString& TransformFormulaIndirectNotation(tString& sString, tApi& sApi, tSheet* sFormulaSheet) {
        TransformFormulaIndirectNotationImpl(sString, true, &sApi, sFormulaSheet);
        // Do not bound A:A / $B:$B here. ProcessSheet runs this while later sheets
        // still have LastRow==0, which used to rewrite INDEX/MATCH lookups to $A$1:$A$1
        // (Rapport annuel Charges 2025b row 134+). ApplyFormulas bounds after import.
        return sString;
    }

    tString& BoundWholeColumnRefsInFormula(tString& sString, tApi& sApi, tSheet* sFormulaSheet) {
        return BoundWholeColumnRefsInFormulaImpl(sString, sApi, sFormulaSheet);
    }

    tString& MaterializeIndirectR1C1RefsToA1(tString& sFormula,
                                              tInt sFormulaRow,
                                              tInt sFormulaCol,
                                              tApi& sApi,
                                              tSheet* sFormulaSheet) {
        (void)sFormulaRow;
        if (sFormula.empty() || sFormulaCol < 1 || !kSkExcelTransformIndirectEnabled) {
            return sFormula;
        }
        const tBool wUseR1C1Range = FormulaUsesR1C1CellRefs(sFormula);

        tString wResult;
        wResult.reserve(sFormula.size() + 32);
        tSize wIndex = 0;
        while (wIndex < sFormula.size()) {
            tSize wKeywordLen = 0;
            if (MatchesIndirectKeywordAt(sFormula, wIndex, wKeywordLen)) {
                tSize wCallEnd = 0;
                tString wRefText;
                tBool wHasSecondArg = false;
                tString wSecondArg;
                if (ParseIndirectCall(sFormula, wIndex, wCallEnd, wRefText, wHasSecondArg, wSecondArg)) {
                    const tBool wR1C1 = wHasSecondArg && IsR1C1StyleSecondArg(wSecondArg);
                    tString wReplacement;
                    if (wR1C1) {
                        tString wSheetPrefix;
                        tString wBody;
                        SplitSheetPrefixFromRef(wRefText, wSheetPrefix, wBody);
                        if (IsFrenchColumnAxisBody(wBody)) {
                            const tIndex wLastRow =
                                LastRowForSheetPrefix(sApi, wSheetPrefix, sFormulaSheet);
                            if (wUseR1C1Range) {
                                wReplacement = BoundedColumnRangeFromFrenchColumnAxisR1C1(
                                    wRefText, sFormulaCol, wLastRow);
                            } else {
                                wReplacement =
                                    BoundedColumnRangeFromFrenchColumnAxis(wRefText, sFormulaCol, wLastRow);
                            }
                        }
                    }
                    if (!wReplacement.empty()) {
                        wResult += wReplacement;
                        wIndex = wCallEnd;
                        continue;
                    }
                }
            }
            wResult += sFormula[wIndex++];
        }
        sFormula.swap(wResult);
        return sFormula;
    }

    // Rewrite Excel dotted function names that the sker lexer cannot parse as a single
    // identifier ('.' starts an Attribute token). Longest source names first.
    // Only rewrite when followed by '(' (optional spaces), outside string literals.
    // Example: STDEV.S(A1:A3) -> STDEV_S(A1:A3). Engine registers the undotted name.
    static tString& RewriteDottedExcelFunctionNames(tString& sString) {
        if (sString.find('.') == tString::npos) {
            return sString;
        }
        struct tRewrite {
            const char* m_From;
            const char* m_To;
        };
        // Keep in sync with SkSpreadSheet AddFunctionRef undotted targets.
        // Longer dotted names first so NETWORKDAYS.INTL is not partially matched.
        static const tRewrite kMap[] = {
            { "NETWORKDAYS.INTL", "NETWORKDAYS_INTL" },
            { "WORKDAY.INTL", "WORKDAY_INTL" },
            { "ERROR.TYPE", "ERROR_TYPE" },
            { "CEILING.PRECISE", "CEILING_PRECISE" },
            { "FLOOR.PRECISE", "FLOOR_PRECISE" },
            { "CEILING.MATH", "CEILING_MATH" },
            { "FLOOR.MATH", "FLOOR_MATH" },
            { "ISO.CEILING", "ISO_CEILING" },
            { "MODE.SNGL", "MODE_SNGL" },
            { "MODE.MULT", "MODE_MULT" },
            { "RANK.EQ", "RANK_EQ" },
            { "STDEV.S", "STDEV_S" },
            { "STDEV.P", "STDEV_P" },
            { "VAR.S", "VAR_S" },
            { "VAR.P", "VAR_P" },
            { "COVARIANCE.P", "COVARIANCE_P" },
            { "COVARIANCE.S", "COVARIANCE_S" },
            { "PERCENTILE.INC", "PERCENTILE_INC" },
            { "PERCENTILE.EXC", "PERCENTILE_EXC" },
            { "QUARTILE.INC", "QUARTILE_INC" },
            { "QUARTILE.EXC", "QUARTILE_EXC" },
            { "FORECAST.LINEAR", "FORECAST_LINEAR" },
            { "PERCENTRANK.INC", "PERCENTRANK_INC" },
            { "PERCENTRANK.EXC", "PERCENTRANK_EXC" },
            { "SKEW.P", "SKEW_P" },
            { "RANK.AVG", "RANK_AVG" },
            { "GAMMALN.PRECISE", "GAMMALN_PRECISE" },
            { "NORM.S.DIST", "NORM_S_DIST" },
            { "NORM.S.INV", "NORM_S_INV" },
            { "NORM.DIST", "NORM_DIST" },
            { "NORM.INV", "NORM_INV" },
        };
        auto wIsIdentChar = [](char sCh) -> tBool {
            return (sCh >= 'A' && sCh <= 'Z') || (sCh >= 'a' && sCh <= 'z') ||
                   (sCh >= '0' && sCh <= '9') || sCh == '_';
        };
        tString wOut;
        wOut.reserve(sString.size());
        tBool wInString = false;
        for (tSize i = 0; i < sString.size(); ) {
            const char wCh = sString[i];
            if (wCh == '"') {
                wInString = !wInString;
                wOut += wCh;
                ++i;
                continue;
            }
            if (!wInString) {
                tBool wMatched = false;
                for (const tRewrite& wRule : kMap) {
                    const tSize wFromLen = std::strlen(wRule.m_From);
                    if (i + wFromLen > sString.size()) {
                        continue;
                    }
                    // Case-insensitive match of the dotted name.
                    tBool wEq = true;
                    for (tSize k = 0; k < wFromLen; ++k) {
                        const char wA = static_cast<char>(std::toupper(static_cast<unsigned char>(sString[i + k])));
                        const char wB = static_cast<char>(std::toupper(static_cast<unsigned char>(wRule.m_From[k])));
                        if (wA != wB) {
                            wEq = false;
                            break;
                        }
                    }
                    if (!wEq) {
                        continue;
                    }
                    // Must be a whole token (not inside a longer identifier).
                    if (i > 0 && wIsIdentChar(sString[i - 1])) {
                        continue;
                    }
                    tSize wAfter = i + wFromLen;
                    while (wAfter < sString.size() &&
                           (sString[wAfter] == ' ' || sString[wAfter] == '\t')) {
                        ++wAfter;
                    }
                    if (wAfter >= sString.size() || sString[wAfter] != '(') {
                        continue;
                    }
                    wOut += wRule.m_To;
                    i += wFromLen;
                    wMatched = true;
                    break;
                }
                if (wMatched) {
                    continue;
                }
            }
            wOut += wCh;
            ++i;
        }
        sString.swap(wOut);
        return sString;
    }

    // Remove OOXML function/parameter markers Excel writes into stored formulas:
    //   _xlfn._xlws.  worksheet-only dynamic-array functions (SORT, FILTER, ...)
    //   _xlfn.        future functions (LET, SEQUENCE, UNIQUE, SWITCH, XLOOKUP, ...)
    //   _xlws.        same dynamic-array family, occasionally standalone
    //   _xlpm.        LET / LAMBDA parameter names (e.g. _xlpm.TEAM -> TEAM)
    //   _xll.         add-in (XLL) functions
    // These prefixes are pure serialization artifacts; the engine knows the bare names.
    // Text inside double-quoted string literals is left untouched.
    static tString& StripOoxmlFormulaMarkers(tString& sString) {
        if (sString.find("_xl") == tString::npos) {
            return sString;
        }
        // Longest markers first so "_xlfn._xlws." is consumed before "_xlfn.".
        static const tString kMarkers[] = {
            tString("_xlfn._xlws."), tString("_xlfn."), tString("_xlws."),
            tString("_xlpm."), tString("_xll.")
        };
        tString wOut;
        wOut.reserve(sString.size());
        tBool wInString = false;
        for (tSize i = 0; i < sString.size(); ) {
            const char wCh = sString[i];
            if (wCh == '"') {
                wInString = !wInString;
                wOut += wCh;
                ++i;
                continue;
            }
            if (!wInString) {
                tBool wMatched = false;
                for (const tString& wMark : kMarkers) {
                    if (sString.compare(i, wMark.size(), wMark) == 0) {
                        i += wMark.size();
                        wMatched = true;
                        break;
                    }
                }
                if (wMatched) {
                    continue;
                }
            }
            wOut += wCh;
            ++i;
        }
        sString.swap(wOut);
        return sString;
    }

    // Excel stores the spilled-range operator D14# as ANCHORARRAY(D14) (usually _xlfn.ANCHORARRAY).
    // After StripOoxmlFormulaMarkers, rewrite ANCHORARRAY(ref) → ref# so the engine's SpillRef path runs.
    // League-Table-Examples.xlsx Part B: SEQUENCE(COUNTA(ANCHORARRAY(D14))) and COUNTIFS(...,ANCHORARRAY(D14)).
    static tString& RewriteAnchorArrayToSpillHash(tString& sString) {
        static const char kName[] = "ANCHORARRAY";
        static const tSize kNameLen = 11;
        auto wIsIdentChar = [](char sCh) -> tBool {
            return (sCh >= 'A' && sCh <= 'Z') || (sCh >= 'a' && sCh <= 'z') ||
                   (sCh >= '0' && sCh <= '9') || sCh == '_';
        };
        auto wStartsAnchor = [&](tSize sAt) -> tBool {
            if (sAt + kNameLen > sString.size()) {
                return false;
            }
            if (sAt > 0 && wIsIdentChar(sString[sAt - 1])) {
                return false;
            }
            for (tSize k = 0; k < kNameLen; ++k) {
                const char wA = static_cast<char>(std::toupper(static_cast<unsigned char>(sString[sAt + k])));
                if (wA != kName[k]) {
                    return false;
                }
            }
            return true;
        };
        tBool wHas = false;
        for (tSize i = 0; i < sString.size(); ++i) {
            if (wStartsAnchor(i)) {
                wHas = true;
                break;
            }
        }
        if (!wHas) {
            return sString;
        }
        tString wOut;
        wOut.reserve(sString.size());
        tBool wInString = false;
        for (tSize i = 0; i < sString.size(); ) {
            const char wCh = sString[i];
            if (wCh == '"') {
                wInString = !wInString;
                wOut += wCh;
                ++i;
                continue;
            }
            if (!wInString && wStartsAnchor(i)) {
                tSize wAfter = i + kNameLen;
                while (wAfter < sString.size() &&
                       (sString[wAfter] == ' ' || sString[wAfter] == '\t')) {
                    ++wAfter;
                }
                if (wAfter < sString.size() && sString[wAfter] == '(') {
                    tInt wDepth = 0;
                    tBool wInArgStr = false;
                    tSize wClose = tString::npos;
                    for (tSize j = wAfter; j < sString.size(); ++j) {
                        const char wC = sString[j];
                        if (wC == '"') {
                            wInArgStr = !wInArgStr;
                            continue;
                        }
                        if (wInArgStr) {
                            continue;
                        }
                        if (wC == '(') {
                            ++wDepth;
                        } else if (wC == ')') {
                            --wDepth;
                            if (wDepth == 0) {
                                wClose = j;
                                break;
                            }
                        }
                    }
                    if (wClose != tString::npos) {
                        tString wArg = sString.substr(wAfter + 1, wClose - (wAfter + 1));
                        tSize wBegin = 0;
                        while (wBegin < wArg.size() &&
                               (wArg[wBegin] == ' ' || wArg[wBegin] == '\t')) {
                            ++wBegin;
                        }
                        tSize wEnd = wArg.size();
                        while (wEnd > wBegin &&
                               (wArg[wEnd - 1] == ' ' || wArg[wEnd - 1] == '\t')) {
                            --wEnd;
                        }
                        wArg = wArg.substr(wBegin, wEnd - wBegin);
                        wOut += wArg;
                        if (wArg.empty() || wArg.back() != '#') {
                            wOut += '#';
                        }
                        i = wClose + 1;
                        continue;
                    }
                }
            }
            wOut += wCh;
            ++i;
        }
        sString.swap(wOut);
        return sString;
    }

    // Strip a legacy CSE array-formula wrapper "{= ... }" down to its inner formula.
    // A bare "{ ... }" (no leading '=') is an array *constant* (e.g. {"POS","TEAM"}) and
    // must be preserved — the parser handles literal arrays via LEFTCURLY ... RIGHTCURLY.
    static void StripCseArrayFormulaBraces(tString& sString) {
        if (sString.size() >= 3 && sString.front() == '{' && sString[1] == '=' &&
            sString.back() == '}') {
            sString.erase(0, 2); // remove "{="
            sString.pop_back();  // remove "}"
        }
    }

    tString& TransFormFormulaSyntax(tString& sString) {
        if (sString.find('\n') != tString::npos || sString.find('\r') != tString::npos ||
            sString.find('\t') != tString::npos || sString.find("_x") != tString::npos) {
            TrimSpacesBeforeNewline(sString);
        }
        StripOoxmlFormulaMarkers(sString);
        RewriteAnchorArrayToSpillHash(sString);
        RewriteDottedExcelFunctionNames(sString);
        StripCseArrayFormulaBraces(sString);
        FlattenExcelSumNestedUnionArgs(sString);
        TransformFormulaIndirectNotation(sString);
        return sString;
    }

    tString& TransFormFormulaSyntax(tString& sString, tApi& sApi, tSheet* sFormulaSheet) {
        if (sString.find('\n') != tString::npos || sString.find('\r') != tString::npos ||
            sString.find('\t') != tString::npos || sString.find("_x") != tString::npos) {
            TrimSpacesBeforeNewline(sString);
        }
        StripOoxmlFormulaMarkers(sString);
        RewriteAnchorArrayToSpillHash(sString);
        RewriteDottedExcelFunctionNames(sString);
        StripCseArrayFormulaBraces(sString);
        FlattenExcelSumNestedUnionArgs(sString);
        TransformFormulaIndirectNotation(sString, sApi, sFormulaSheet);
        return sString;
    }

    tString& TransFormFormulaSyntax(tString& sString,
                                    tApi& sApi,
                                    tSheet* sFormulaSheet,
                                    tInt sFormulaRow,
                                    tInt sFormulaCol) {
        TransFormFormulaSyntax(sString, sApi, sFormulaSheet);
        MaterializeIndirectR1C1RefsToA1(sString, sFormulaRow, sFormulaCol, sApi, sFormulaSheet);
        return sString;
    }

    // Helper function to escape JSON string
    tString EscapeJsonString(const tString& sStr) {
        tString wResult;
        wResult.reserve(sStr.size() + 10); // Reserve space for potential escapes
        for (char wCh : sStr) {
            switch (wCh) {
                case '"':  wResult += "\\\""; break;
                case '\\': wResult += "\\\\"; break;
                case '\b': wResult += "\\b"; break;
                case '\f': wResult += "\\f"; break;
                case '\n': wResult += "\\n"; break;
                case '\r': wResult += "\\r"; break;
                case '\t': wResult += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(wCh) < 0x20) {
                        // Control character: escape as \uXXXX
                        char wBuf[7];
                        std::snprintf(wBuf, sizeof(wBuf), "\\u%04x", static_cast<unsigned char>(wCh));
                        wResult += wBuf;
                    } else {
                        wResult += wCh;
                    }
                    break;
            }
        }
        return wResult;
    }
   
    std::pair<tInt,tInt> RefToRowCol(const tString& sRef) {
        tInt wCol = 0; tInt wRow = 0; tInt wI = 0;
        while (wI < (tInt)sRef.size() && std::isalpha((unsigned char)sRef[wI])) {
            wCol = wCol * 26 + (std::toupper((unsigned char)sRef[wI]) - 'A' + 1);
            ++wI;
        }
        while (wI < (tInt)sRef.size() && std::isdigit((unsigned char)sRef[wI])) {
            wRow = wRow * 10 + (sRef[wI] - '0');
            ++wI;
        }
        return {wRow, wCol};
    }

    static void AppendR1C1Axis(tString& sOut, tChar sAxis, tInt sAbs, tBool sLocked, tInt sHost) {
        sOut += sAxis;
        if (sLocked) {
            sOut += std::to_string(sAbs);
        } else {
            const tInt wOff = sAbs - sHost;
            sOut += '[';
            sOut += std::to_string(wOff);
            sOut += ']';
        }
    }

    static tBool ParseColLetters(const tString& s, tSize& ioPos, tInt& oCol) {
        const tSize wStart = ioPos;
        oCol = 0;
        while (ioPos < s.size() && std::isalpha(static_cast<unsigned char>(s[ioPos]))) {
            oCol = oCol * 26 + (std::toupper(static_cast<unsigned char>(s[ioPos])) - 'A' + 1);
            ++ioPos;
        }
        return ioPos > wStart;
    }

    static tBool ParseA1Anchor(const tString& s, tSize& ioPos, tBool& oLockCol, tBool& oLockRow,
                               tInt& oCol, tInt& oRow, tBool& oHasCol, tBool& oHasRow) {
        oLockCol = false;
        oLockRow = false;
        oCol = 0;
        oRow = 0;
        oHasCol = false;
        oHasRow = false;
        tSize wPos = ioPos;
        if (wPos < s.size() && s[wPos] == '$') {
            oLockCol = true;
            ++wPos;
        }
        const tSize wColStart = wPos;
        if (!ParseColLetters(s, wPos, oCol)) {
            if (!oLockCol) {
                return false;
            }
        }
        oHasCol = (wPos > wColStart) || oLockCol;
        if (oHasCol && (wPos - wColStart) > 3) {
            return false;
        }
        if (wPos < s.size() && s[wPos] == '$') {
            oLockRow = true;
            ++wPos;
        }
        const tSize wRowStart = wPos;
        while (wPos < s.size() && std::isdigit(static_cast<unsigned char>(s[wPos]))) {
            oRow = oRow * 10 + static_cast<tInt>(s[wPos] - '0');
            ++wPos;
        }
        oHasRow = (wPos > wRowStart) || oLockRow;
        if (!oHasCol && !oHasRow) {
            return false;
        }
        // Excel column/row shorthands: J:L, 5:10 ($J:$L uses locks above).
        if (oHasCol && !oHasRow) {
            oRow = 1;
            oHasRow = true;
        }
        if (oHasRow && !oHasCol) {
            oCol = 1;
            oHasCol = true;
        }
        ioPos = wPos;
        return true;
    }

    static tBool IsRefBoundaryChar(tChar sCh) {
        switch (sCh) {
            case '(':
            case ')':
            case ',':
            case ';':
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
            case '&':
            case '=':
            case '<':
            case '>':
            case '%':
                return true;
            default:
                return false;
        }
    }

    static tString FormatA1AnchorAsR1C1(tBool sLockCol, tBool sLockRow, tInt sCol, tInt sRow,
                                        tInt sHostRow, tInt sHostCol) {
        tString wOut;
        AppendR1C1Axis(wOut, 'R', sRow, sLockRow, sHostRow);
        AppendR1C1Axis(wOut, 'C', sCol, sLockCol, sHostCol);
        return wOut;
    }

    tString ConvertA1CellRefsToR1C1(const tString& sFormula, tInt sHostRow, tInt sHostCol) {
        tString wOut;
        wOut.reserve(sFormula.size() + 32);
        tBool wInString = false;
        tChar wStringQuote = 0;
        for (tSize i = 0; i < sFormula.size();) {
            const tChar wCh = sFormula[i];
            if (wInString) {
                wOut += wCh;
                if (wCh == wStringQuote) {
                    wInString = false;
                }
                ++i;
                continue;
            }
            if (wCh == '"' || wCh == '\'') {
                wInString = true;
                wStringQuote = wCh;
                wOut += wCh;
                ++i;
                continue;
            }

            tString wSheetPrefix;
            tSize wRefStart = i;
            if (wCh == '\'') {
                tSize j = i + 1;
                while (j < sFormula.size()) {
                    if (sFormula[j] == '\'' && j + 1 < sFormula.size() && sFormula[j + 1] == '\'') {
                        j += 2;
                        continue;
                    }
                    if (sFormula[j] == '\'') {
                        break;
                    }
                    ++j;
                }
                if (j < sFormula.size() && j + 1 < sFormula.size() && sFormula[j + 1] == '!') {
                    wSheetPrefix = sFormula.substr(i, j - i + 2);
                    i = j + 2;
                    wRefStart = i;
                }
            } else if (std::isalpha(static_cast<unsigned char>(wCh)) || wCh == '_') {
                tSize j = i;
                while (j < sFormula.size()) {
                    const tChar c = sFormula[j];
                    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.') {
                        ++j;
                        continue;
                    }
                    break;
                }
                if (j < sFormula.size() && sFormula[j] == '!') {
                    wSheetPrefix = sFormula.substr(i, j - i + 1);
                    i = j + 1;
                    wRefStart = i;
                } else if (wCh == '_' && j > i) {
                    // Named range (_RUBRIQUES_STATS_N1): keep whole token; do not parse N1 as A1.
                    wOut += sFormula.substr(i, j - i);
                    i = j;
                    continue;
                } else if (j > i) {
                    tSize k = j;
                    while (k < sFormula.size() && (sFormula[k] == ' ' || sFormula[k] == '\t')) {
                        ++k;
                    }
                    if (k < sFormula.size() && sFormula[k] == '(') {
                        // Function name (SUM, COUNTIF, OFFSET, ...).
                        wOut += sFormula.substr(i, j - i);
                        i = j;
                        continue;
                    }
                }
            }

            tSize wAnchorPos = i;
            tBool wLockCol = false;
            tBool wLockRow = false;
            tInt wCol = 0;
            tInt wRow = 0;
            tBool wHasCol = false;
            tBool wHasRow = false;
            if (!ParseA1Anchor(sFormula, wAnchorPos, wLockCol, wLockRow, wCol, wRow, wHasCol, wHasRow)) {
                if (!wSheetPrefix.empty()) {
                    wOut += wSheetPrefix;
                } else {
                    wOut += wCh;
                    ++i;
                }
                continue;
            }
            if (wAnchorPos == i && wSheetPrefix.empty()) {
                wOut += wCh;
                ++i;
                continue;
            }
            if (wAnchorPos < sFormula.size() && std::isalpha(static_cast<unsigned char>(sFormula[wAnchorPos]))) {
                if (!wSheetPrefix.empty()) {
                    wOut += wSheetPrefix;
                } else {
                    wOut += wCh;
                    ++i;
                }
                continue;
            }
            if (wRefStart > 0 && std::isalnum(static_cast<unsigned char>(sFormula[wRefStart - 1]))) {
                if (!wSheetPrefix.empty()) {
                    wOut += wSheetPrefix;
                } else {
                    wOut += wCh;
                    ++i;
                }
                continue;
            }
            if (wAnchorPos < sFormula.size() && !IsRefBoundaryChar(sFormula[wAnchorPos])) {
                if (!wSheetPrefix.empty()) {
                    wOut += wSheetPrefix;
                } else {
                    wOut += wCh;
                    ++i;
                }
                continue;
            }

            // A column-only anchor (letters, no explicit row) is a real reference only inside a
            // full-column range like P:P. Standing alone it is a NAME — typically a LET/LAMBDA
            // parameter (P, W, GD, PTS after _xlpm. stripping) or a short defined name — and must
            // stay verbatim, otherwise it would be rewritten to a cell (e.g. P -> R[..]C[12]) and
            // break the parser (letBinding expects an ID, not a cell). ParseA1Anchor forces row=1
            // for column-only shorthand, so detect the missing row from the source text itself.
            {
                tBool wAnchorHasDigit = false;
                for (tSize p = wRefStart; p < wAnchorPos; ++p) {
                    if (std::isdigit(static_cast<unsigned char>(sFormula[p]))) {
                        wAnchorHasDigit = true;
                        break;
                    }
                }
                const tBool wFollowedByColon =
                    (wAnchorPos < sFormula.size() && sFormula[wAnchorPos] == ':');
                const tBool wPrecededByColon =
                    (wRefStart > 0 && sFormula[wRefStart - 1] == ':');
                if (!wAnchorHasDigit && !wFollowedByColon && !wPrecededByColon) {
                    wOut += wSheetPrefix;
                    wOut += sFormula.substr(wRefStart, wAnchorPos - wRefStart);
                    i = wAnchorPos;
                    continue;
                }
                // Keep A:A / $B:$B / J:L as whole-column shorthand. ParseA1Anchor
                // forces row=1, which would otherwise become R1C1:R1C1 ($A$1:$A$1).
                if (!wAnchorHasDigit && wFollowedByColon) {
                    tSize wSecondPos = wAnchorPos + 1;
                    tBool wLockCol2 = false;
                    tBool wLockRow2 = false;
                    tInt wCol2 = 0;
                    tInt wRow2 = 0;
                    tBool wHasCol2 = false;
                    tBool wHasRow2 = false;
                    if (ParseA1Anchor(sFormula, wSecondPos, wLockCol2, wLockRow2, wCol2, wRow2,
                                      wHasCol2, wHasRow2)) {
                        tBool wSecondHasDigit = false;
                        for (tSize p = wAnchorPos + 1; p < wSecondPos; ++p) {
                            if (std::isdigit(static_cast<unsigned char>(sFormula[p]))) {
                                wSecondHasDigit = true;
                                break;
                            }
                        }
                        if (!wSecondHasDigit) {
                            wOut += wSheetPrefix;
                            wOut += sFormula.substr(wRefStart, wSecondPos - wRefStart);
                            i = wSecondPos;
                            continue;
                        }
                    }
                }
            }

            wOut += wSheetPrefix;
            wOut += FormatA1AnchorAsR1C1(wLockCol, wLockRow, wCol, wRow, sHostRow, sHostCol);
            i = wAnchorPos;
            if (i < sFormula.size() && sFormula[i] == ':') {
                ++i;
                tSize wSecondPos = i;
                tBool wLockCol2 = false;
                tBool wLockRow2 = false;
                tInt wCol2 = 0;
                tInt wRow2 = 0;
                tBool wHasCol2 = false;
                tBool wHasRow2 = false;
                if (ParseA1Anchor(sFormula, wSecondPos, wLockCol2, wLockRow2, wCol2, wRow2, wHasCol2, wHasRow2)) {
                    wOut += ':';
                    wOut += FormatA1AnchorAsR1C1(wLockCol2, wLockRow2, wCol2, wRow2, sHostRow, sHostCol);
                    i = wSecondPos;
                    continue;
                }
                wOut += ':';
            }
        }
        return wOut;
    }

    tString ConvertR1C1RelativeRefsToA1(const tString& sFormula, tInt sFormulaRow, tInt sFormulaCol) {
        tString out;
        out.reserve(sFormula.size() + 16);
        const tSize n = sFormula.size();
        tSize i = 0;
        while (i < n) {
            if (i + 2 < n && sFormula[i] == 'R' && sFormula[i + 1] == '[') {
                tSize j = i + 2;
                tInt rowOff = 0;
                tBool neg = false;
                if (j < n && sFormula[j] == '-') {
                    neg = true;
                    ++j;
                }
                while (j < n && std::isdigit((unsigned char)sFormula[j])) {
                    rowOff = rowOff * 10 + static_cast<tInt>(sFormula[j] - '0');
                    ++j;
                }
                if (neg) rowOff = -rowOff;
                if (j >= n || sFormula[j] != ']') {
                    out += sFormula[i++];
                    continue;
                }
                ++j;
                if (j + 1 >= n || sFormula[j] != 'C' || sFormula[j + 1] != '[') {
                    out += sFormula[i++];
                    continue;
                }
                j += 2;
                tInt colOff = 0;
                neg = false;
                if (j < n && sFormula[j] == '-') {
                    neg = true;
                    ++j;
                }
                while (j < n && std::isdigit((unsigned char)sFormula[j])) {
                    colOff = colOff * 10 + static_cast<tInt>(sFormula[j] - '0');
                    ++j;
                }
                if (neg) colOff = -colOff;
                if (j >= n || sFormula[j] != ']') {
                    out += sFormula[i++];
                    continue;
                }
                ++j;
                const tInt tr = sFormulaRow + rowOff;
                const tInt tc = sFormulaCol + colOff;
                out += Base10ToAlpha(tc);
                out += std::to_string(tr);
                i = j;
                continue;
            }
            out += sFormula[i++];
        }
        return out;
    }

    static tBool ParseExcelA1RefAt(const tString& sFormula, tSize sPos, tSize* oEnd) {
        tSize p = sPos;
        if (p < sFormula.size() && sFormula[p] == '$') {
            ++p;
        }
        const tSize wColStart = p;
        while (p < sFormula.size() && std::isalpha(static_cast<unsigned char>(sFormula[p]))) {
            ++p;
        }
        if (p == wColStart) {
            return false;
        }
        if (p < sFormula.size() && sFormula[p] == '$') {
            ++p;
        }
        const tSize wRowStart = p;
        while (p < sFormula.size() && std::isdigit(static_cast<unsigned char>(sFormula[p]))) {
            ++p;
        }
        if (p == wRowStart) {
            return false;
        }
        *oEnd = p;
        return true;
    }

    // Excel rejects TEXT(D17:D17); collapse redundant single-cell ranges (D17:D17 -> D17).
    // Only match real A1 refs (letters+digits), skip string literals — do not touch 00:00:01.
    static tString CollapseRedundantSingleCellRanges(const tString& sFormula) {
        tString wOut;
        wOut.reserve(sFormula.size());
        tBool wInString = false;
        for (tSize i = 0; i < sFormula.size(); ) {
            const char wCh = sFormula[i];
            if (wCh == '"') {
                wInString = !wInString;
                wOut.push_back(wCh);
                ++i;
                continue;
            }
            if (wInString) {
                wOut.push_back(wCh);
                ++i;
                continue;
            }
            tSize wEnd1 = 0;
            if (!ParseExcelA1RefAt(sFormula, i, &wEnd1)) {
                wOut.push_back(wCh);
                ++i;
                continue;
            }
            if (wEnd1 < sFormula.size() && sFormula[wEnd1] == ':') {
                tSize wEnd2 = 0;
                if (ParseExcelA1RefAt(sFormula, wEnd1 + 1, &wEnd2)) {
                    tString wRef1 = sFormula.substr(i, wEnd1 - i);
                    tString wRef2 = sFormula.substr(wEnd1 + 1, wEnd2 - wEnd1 - 1);
                    tString wNorm1, wNorm2;
                    for (char c : wRef1) {
                        if (c != '$') wNorm1.push_back(c);
                    }
                    for (char c : wRef2) {
                        if (c != '$') wNorm2.push_back(c);
                    }
                    if (wNorm1 == wNorm2) {
                        wOut.append(wRef1);
                        i = wEnd2;
                        continue;
                    }
                }
            }
            wOut.push_back(wCh);
            ++i;
        }
        return wOut;
    }

    static tBool MatchTextFunctionOpenAt(const tString& sFormula, tSize sStart, tSize* oArgStart) {
        if (sStart + 4 > sFormula.size()) {
            return false;
        }
        if (std::toupper(static_cast<unsigned char>(sFormula[sStart])) != 'T'
            || std::toupper(static_cast<unsigned char>(sFormula[sStart + 1])) != 'E'
            || std::toupper(static_cast<unsigned char>(sFormula[sStart + 2])) != 'X'
            || std::toupper(static_cast<unsigned char>(sFormula[sStart + 3])) != 'T') {
            return false;
        }
        tSize i = sStart + 4;
        while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
            ++i;
        }
        if (i >= sFormula.size() || sFormula[i] != '(') {
            return false;
        }
        if (oArgStart != nullptr) {
            *oArgStart = i + 1;
        }
        return true;
    }

    static tString ExtractCssQuotedProperty(const tString& sCss, const tString& sKey) {
        const tString wNeedle = sKey + ":\"";
        const tSize wPos = sCss.find(wNeedle);
        if (wPos == tString::npos) {
            return "";
        }
        tSize wScan = wPos + wNeedle.size();
        tString wOut;
        while (wScan < sCss.size() && sCss[wScan] != '"') {
            wOut.push_back(sCss[wScan++]);
        }
        return wOut;
    }

    static tString SkFormatKeyToExcelFormatMask(const tString& sKey) {
        tString wKey = sKey;
        while (!wKey.empty() && wKey.front() == '"') {
            wKey.erase(0, 1);
        }
        while (!wKey.empty() && wKey.back() == '"') {
            wKey.pop_back();
        }
        if (wKey.empty() || wKey == "General") {
            return "";
        }
        SkRoot::tFormatString wFmt;
        if (wFmt.ExcelFormat(wKey)) {
            return wFmt.FormatExcel();
        }
        wFmt.Clear();
        wFmt.Format(wKey);
        if (wFmt.FormatType() != SkRoot::tFormatStringType::none) {
            const tString wExcel = wFmt.ExcelEquivalent("");
            if (!wExcel.empty() && wExcel != "None") {
                return wExcel;
            }
        }
        return wKey;
    }

    static tString InferTextFormatMaskFromCellCss(tApi& sApi, tInt sRow, tInt sCol, tSheet* sSheet,
                                                  const tString* sCellCss) {
        if (sCellCss != nullptr) {
            const tString wFmtKey = ExtractCssQuotedProperty(*sCellCss, "format-string");
            const tString wMask = SkFormatKeyToExcelFormatMask(wFmtKey);
            if (!wMask.empty()) {
                return wMask;
            }
        }
        if (sSheet == nullptr) {
            return "";
        }
        const tString wCss = sApi.CellFormat(sRow, sCol, sSheet);
        const tString wFmtKey = ExtractCssQuotedProperty(wCss, "format-string");
        return SkFormatKeyToExcelFormatMask(wFmtKey);
    }

    static tBool FormulaContainsTextCall(const tString& sFormula) {
        tBool wInString = false;
        for (tSize pos = 0; pos < sFormula.size(); ++pos) {
            if (sFormula[pos] == '"') {
                wInString = !wInString;
                continue;
            }
            if (!wInString && MatchTextFunctionOpenAt(sFormula, pos, nullptr)) {
                return true;
            }
        }
        return false;
    }

    static tString ExtractTextFormatLiteralFromFormula(const tString& sFormula) {
        tBool wInString = false;
        for (tSize pos = 0; pos < sFormula.size(); ) {
            const char wCh = sFormula[pos];
            if (wCh == '"') {
                wInString = !wInString;
                ++pos;
                continue;
            }
            if (wInString) {
                ++pos;
                continue;
            }
            tSize wArgStart = 0;
            if (!MatchTextFunctionOpenAt(sFormula, pos, &wArgStart)) {
                ++pos;
                continue;
            }
            tInt wDepth = 1;
            tBool wInnerString = false;
            for (tSize i = wArgStart; i < sFormula.size(); ++i) {
                const char wC = sFormula[i];
                if (wC == '"') {
                    wInnerString = !wInnerString;
                    continue;
                }
                if (wInnerString) {
                    continue;
                }
                if (wC == '(') {
                    ++wDepth;
                } else if (wC == ')') {
                    --wDepth;
                    if (wDepth == 0) {
                        break;
                    }
                } else if (wC == ',' && wDepth == 1) {
                    tSize j = i + 1;
                    while (j < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[j]))) {
                        ++j;
                    }
                    if (j < sFormula.size() && sFormula[j] == '"') {
                        ++j;
                        tString wFmt;
                        while (j < sFormula.size()) {
                            if (sFormula[j] == '"') {
                                if (j + 1 < sFormula.size() && sFormula[j + 1] == '"') {
                                    wFmt.push_back('"');
                                    j += 2;
                                    continue;
                                }
                                break;
                            }
                            wFmt.push_back(sFormula[j++]);
                        }
                        if (!wFmt.empty()) {
                            return wFmt;
                        }
                    }
                    break;
                }
            }
            pos = wArgStart + 1;
        }
        return "";
    }

    static tString InferTextFormatFromSheetPeers(tApi& sApi, tSheet* sSheet, tInt sRow, tInt sCol) {
        if (sSheet == nullptr) {
            return "";
        }
        // Only scan nearby rows — full-sheet scan per exported cell is O(rows × cells).
        const tInt wMinRow = std::max(1, sRow - 40);
        const tInt wMaxRow = std::min(static_cast<tInt>(sSheet->LastRow()), sRow + 40);
        for (tInt wRow = wMinRow; wRow <= wMaxRow; ++wRow) {
            if (wRow == sRow) {
                continue;
            }
            tString wPeer = sApi.Formula(static_cast<tIndex>(wRow), static_cast<tIndex>(sCol), sSheet, false);
            if (wPeer.empty()) {
                continue;
            }
            if (wPeer.front() == '=') {
                wPeer.erase(0, 1);
            }
            const tString wFmt = ExtractTextFormatLiteralFromFormula(wPeer);
            if (!wFmt.empty()) {
                return wFmt;
            }
        }
        return "";
    }

    static tString InferTextFormatMaskForExport(tApi& sApi,
                                                tInt sFormulaRow,
                                                tInt sFormulaCol,
                                                tSheet* sFormulaSheet,
                                                const tString* sCellCss,
                                                const tString& sFormulaBody) {
        tString wMask = InferTextFormatMaskFromCellCss(sApi, sFormulaRow, sFormulaCol, sFormulaSheet, sCellCss);
        if (!wMask.empty()) {
            return wMask;
        }
        wMask = ExtractTextFormatLiteralFromFormula(sFormulaBody);
        if (!wMask.empty()) {
            return wMask;
        }
        wMask = InferTextFormatFromSheetPeers(sApi, sFormulaSheet, sFormulaRow, sFormulaCol);
        if (!wMask.empty()) {
            return wMask;
        }
        return "";
    }

    static tBool ParseExcelNamedIdentifierAt(const tString& sFormula, tSize sPos, tSize* oEnd) {
        tSize i = sPos;
        if (i >= sFormula.size()) {
            return false;
        }
        if (!(std::isalpha(static_cast<unsigned char>(sFormula[i])) || sFormula[i] == '_')) {
            return false;
        }
        ++i;
        while (i < sFormula.size()
               && (std::isalnum(static_cast<unsigned char>(sFormula[i])) || sFormula[i] == '_')) {
            ++i;
        }
        if (i == sPos) {
            return false;
        }
        *oEnd = i;
        return true;
    }

    static tString CellRefForMultiAreaNameAtRow(tWorkBook* sBook, const tString& sName, tInt sRow) {
        if (sBook == nullptr) {
            return "";
        }
        tRangeNamedContainer* wContainer = sBook->RangeNamedContainer();
        if (wContainer == nullptr) {
            return "";
        }
        const std::vector<tRange*> wRanges = wContainer->Ranges(sName);
        if (wRanges.size() <= 1) {
            return "";
        }
        for (tRange* wRange : wRanges) {
            if (wRange == nullptr) {
                continue;
            }
            if (sRow >= static_cast<tInt>(wRange->TopIndex())
                && sRow <= static_cast<tInt>(wRange->BottomIndex())) {
                return Base10ToAlpha(wRange->LeftIndex()) + std::to_string(sRow);
            }
        }
        return "";
    }

    // TEXT(multi-area named range) is invalid in Excel; use the area on the formula row (e.g. TEXT(D14,...)).
    static tString ReplaceMultiAreaNamedRefsInTextCalls(const tString& sFormula,
                                                        tInt sFormulaRow,
                                                        tSheet* sFormulaSheet) {
        if (sFormulaSheet == nullptr) {
            return sFormula;
        }
        tWorkBook* wBook = sFormulaSheet->WorkBook();
        if (wBook == nullptr) {
            return sFormula;
        }
        tString wOut;
        wOut.reserve(sFormula.size());
        tBool wInString = false;
        for (tSize pos = 0; pos < sFormula.size(); ) {
            const char wCh = sFormula[pos];
            if (wCh == '"') {
                wInString = !wInString;
                wOut.push_back(wCh);
                ++pos;
                continue;
            }
            if (wInString) {
                wOut.push_back(wCh);
                ++pos;
                continue;
            }
            tSize wArgStart = 0;
            if (MatchTextFunctionOpenAt(sFormula, pos, &wArgStart)) {
                tSize i = wArgStart;
                while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
                    ++i;
                }
                tSize wIdEnd = 0;
                if (ParseExcelNamedIdentifierAt(sFormula, i, &wIdEnd)) {
                    const tString wName = sFormula.substr(i, wIdEnd - i);
                    const tString wCellRef = CellRefForMultiAreaNameAtRow(wBook, wName, sFormulaRow);
                    if (!wCellRef.empty()) {
                        wOut.append(sFormula.substr(pos, wArgStart - pos));
                        wOut.append(wCellRef);
                        pos = wIdEnd;
                        continue;
                    }
                }
            }
            wOut.push_back(wCh);
            ++pos;
        }
        return wOut;
    }

    static tString ExcelQuoteFormatLiteral(const tString& sFmt) {
        tString wOut = "\"";
        for (char wCh : sFmt) {
            if (wCh == '"') {
                wOut += "\"\"";
            } else {
                wOut.push_back(wCh);
            }
        }
        wOut += "\"";
        return wOut;
    }

    static tBool TryRepairSingleTextCall(const tString& sFormula,
                                         tSize sTextStart,
                                         const tString& sFormatMask,
                                         tString* oReplacement,
                                         tSize* oEnd) {
        if (sFormatMask.empty()) {
            return false;
        }
        tSize wArgStart = 0;
        if (!MatchTextFunctionOpenAt(sFormula, sTextStart, &wArgStart)) {
            return false;
        }
        tSize i = wArgStart;
        while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
            ++i;
        }
        tSize wEnd1 = 0;
        if (!ParseExcelA1RefAt(sFormula, i, &wEnd1)) {
            return false;
        }
        i = wEnd1;
        while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
            ++i;
        }
        if (i < sFormula.size() && sFormula[i] == ':') {
            tSize wEnd2 = 0;
            if (!ParseExcelA1RefAt(sFormula, i + 1, &wEnd2)) {
                return false;
            }
            i = wEnd2;
            while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
                ++i;
            }
        }
        if (i >= sFormula.size() || sFormula[i] != ')') {
            return false;
        }
        *oReplacement = sFormula.substr(sTextStart, i - sTextStart)
                        + "," + ExcelQuoteFormatLiteral(sFormatMask) + ")";
        *oEnd = i + 1;
        return true;
    }

    static tBool TextCallNeedsFormatArg(const tString& sFormula, tSize sTextStart) {
        tSize wArgStart = 0;
        if (!MatchTextFunctionOpenAt(sFormula, sTextStart, &wArgStart)) {
            return false;
        }
        tSize i = wArgStart;
        while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
            ++i;
        }
        tSize wEnd1 = 0;
        if (!ParseExcelA1RefAt(sFormula, i, &wEnd1)) {
            tSize wIdEnd = 0;
            if (!ParseExcelNamedIdentifierAt(sFormula, i, &wIdEnd)) {
                return false;
            }
            i = wIdEnd;
        } else {
            i = wEnd1;
        }
        while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
            ++i;
        }
        if (i < sFormula.size() && sFormula[i] == ':') {
            tSize wEnd2 = 0;
            if (!ParseExcelA1RefAt(sFormula, i + 1, &wEnd2)) {
                return false;
            }
            i = wEnd2;
            while (i < sFormula.size() && std::isspace(static_cast<unsigned char>(sFormula[i]))) {
                ++i;
            }
        }
        return i < sFormula.size() && sFormula[i] == ')';
    }

    // SkSpreadSheet may store TEXT(R[0]C[-1]:R[0]C[-1]) without the format literal; after export
    // normalization that becomes TEXT(D17), which Excel rejects. Re-add the format mask from the
    // formula cell CSS (any TEXT(...) in the formula, not only a top-level call).
    static tString RepairTextMissingFormatArgs(const tString& sFormula,
                                               tInt sFormulaRow,
                                               tInt sFormulaCol,
                                               tApi& sApi,
                                               tSheet* sFormulaSheet,
                                               const tString* sCellCss) {
        if (!FormulaContainsTextCall(sFormula)) {
            return sFormula;
        }
        tString wFormatMask;
        tString wOut;
        wOut.reserve(sFormula.size() + 16);
        tBool wInString = false;
        for (tSize pos = 0; pos < sFormula.size(); ) {
            const char wCh = sFormula[pos];
            if (wCh == '"') {
                wInString = !wInString;
                wOut.push_back(wCh);
                ++pos;
                continue;
            }
            if (wInString) {
                wOut.push_back(wCh);
                ++pos;
                continue;
            }
            if (MatchTextFunctionOpenAt(sFormula, pos, nullptr)) {
                tString wReplacement;
                tSize wEnd = 0;
                if (TextCallNeedsFormatArg(sFormula, pos)) {
                    if (wFormatMask.empty()) {
                        wFormatMask = InferTextFormatMaskForExport(
                            sApi, sFormulaRow, sFormulaCol, sFormulaSheet, sCellCss, sFormula);
                    }
                }
                if (!wFormatMask.empty()
                    && TryRepairSingleTextCall(sFormula, pos, wFormatMask, &wReplacement, &wEnd)) {
                    wOut.append(wReplacement);
                    pos = wEnd;
                    continue;
                }
            }
            wOut.push_back(wCh);
            ++pos;
        }
        return wOut;
    }

    tString PrepareFormulaForExcelExport(const tString& sFormula,
                                           tInt sFormulaRow,
                                           tInt sFormulaCol,
                                           tApi& sApi,
                                           tSheet* sFormulaSheet,
                                           const tString* sCellCss) {
        if (sFormula.empty()) {
            return sFormula;
        }
        tString wBody = sFormula;
        if (wBody.front() == '=') {
            wBody.erase(0, 1);
        }
        // Body is already A1 from FormulaStr(); do not rewrite ranges (e.g. ROWS($C$6:C6)).
        wBody = ConvertR1C1RelativeRefsToA1(wBody, sFormulaRow, sFormulaCol);
        if (FormulaContainsTextCall(wBody)) {
            wBody = CollapseRedundantSingleCellRanges(wBody);
            wBody = ReplaceMultiAreaNamedRefsInTextCalls(wBody, sFormulaRow, sFormulaSheet);
            wBody = RepairTextMissingFormatArgs(wBody, sFormulaRow, sFormulaCol, sApi, sFormulaSheet, sCellCss);
        }
        return wBody;
    }


    /// @brief Check if the first row of the table range contains the column header labels (e.g. "Période 0", "Articles").
    /// Some tables (e.g. Encaissements) have no header row in the sheet; others (e.g. Décaissements) do.
    /// @return true if at least 2 cells in the first row match the table column names.
    tBool FirstRowMatchesColumnNames(const tExcelPugiXMLReader& sReader, tInt sSheetIndex,
        tInt wTop, tInt wLeft, const tVectorString& wColumnNames) {
        if (wColumnNames.empty()) return false;
        auto wItSheet = sReader.GetUnzippedFile("xl/worksheets/sheet" + std::to_string(sSheetIndex) + ".xml");
        if (!wItSheet) return false;
        pugi::xml_document wDoc;
        if (!wDoc.load_buffer(wItSheet->content.data(), wItSheet->content.size())) return false;
        pugi::xml_node wSheetData = wDoc.child("worksheet").child("sheetData");
        if (!wSheetData) return false;
        const tString wTopStr = std::to_string(wTop);
        pugi::xml_node wRow;
        for (wRow = wSheetData.child("row"); wRow; wRow = wRow.next_sibling("row")) {
            const char* wR = wRow.attribute("r").value();
            if (wR && wTopStr == wR) break;
        }
        if (!wRow) return false;
        // Collect cell values in first row for columns wLeft..wLeft+columnCount-1 (sparse row)
        std::map<tInt, tString> wCellValues; // col index -> display string
        for (pugi::xml_node wC : wRow.children("c")) {
            const char* wR = wC.attribute("r").value();
            if (!wR || !*wR) continue;
            auto wRc = RefToRowCol(wR);
            tInt wRowNum = wRc.first, wColNum = wRc.second;
            if (wRowNum != wTop) continue;
            if (wColNum < wLeft || wColNum >= wLeft + (tInt)wColumnNames.size()) continue;
            tString wVal;
            const char* wTAttr = wC.attribute("t").value();
            if (tString(wTAttr) == "inlineStr") {
                if (auto wIs = wC.child("is"); wIs) {
                    if (pugi::xml_node wT = wIs.child("t"))
                        wVal = wT.child_value();
                    for (pugi::xml_node wR : wIs.children("r")) {
                        if (pugi::xml_node wRt = wR.child("t"))
                            wVal += wRt.child_value();
                    }
                }
            } else if (auto wV = wC.child("v")) {
                wVal = wV.child_value();
                if (tString(wTAttr) == "s") {
                    tInt wIdx = std::atoi(wVal.c_str());
                    const auto& wSst = sReader.GetSharedStrings();
                    if (wIdx >= 0 && wIdx < (tInt)wSst.size()) wVal = wSst[wIdx];
                }
            }
            wCellValues[wColNum] = wVal;
        }
        tInt wMatchCount = 0;
        for (tSize wI = 0; wI < wColumnNames.size(); ++wI) {
            tInt wCol = wLeft + (tInt)wI;
            auto it = wCellValues.find(wCol);
            if (it != wCellValues.end() && it->second == wColumnNames[wI])
                wMatchCount++;
        }
        // Require 2 matches to avoid false positives on multi-column tables, but
        // accept a single match for single-column tables (otherwise their header
        // row is never recognized, so the autofilter/sort button is dropped).
        const tInt wRequired = (tInt)wColumnNames.size() >= 2 ? 2 : (tInt)wColumnNames.size();
        return wMatchCount >= wRequired;
    }

    // Excel unit conversions (output in mm for SkSpreadSheet API)
    // Points: 1 pt = 1/72 inch, 1 inch = 25.4 mm => mm = pt * 25.4/72
    tDouble PointsToMillimeters(tDouble sPt) { return sPt * 25.4 / 72.0; }
    tDouble MillimetersToPoints(tDouble sMm) { return sMm * 72.0 / 25.4; }
    // Pixels: 96 DPI => 1 px = 25.4/96 mm
    tDouble PixelsToMillimeters(tDouble sPx) { return sPx * (25.4 / 96.0); }

    tDouble ExcelColWidthKScale() { return 1.0; }

    tDouble ExcelRowHeightPtToSkMm(tDouble sPt) {
        if (sPt <= 0.0) {
            return 0.0;
        }
        // Store the Excel point value faithfully so xlsx round-trip stays exact.
        return PointsToMillimeters(sPt);
    }

    tDouble SkMmToExcelRowHeightPt(tDouble sMm) {
        if (sMm <= 0.0) {
            return 0.0;
        }
        return MillimetersToPoints(sMm);
    }

    tDouble ExcelColWidthCharsToSkMm(tDouble sWidthChars, tInt sMdw) {
        if (sWidthChars <= 0.0 || sMdw <= 0) {
            return 0.0;
        }
        const tInt wPx = ExcelWidthToPixels(sWidthChars, sMdw);
        return PixelsToMillimeters(static_cast<tDouble>(wPx)) * ExcelColWidthKScale();
    }

    tDouble SkMmToExcelColWidthChars(tDouble sMm, tInt sMdw) {
        if (sMm <= 0.0 || sMdw <= 0) {
            return 0.0;
        }
        const tDouble wKColScale = ExcelColWidthKScale();
        const tInt wTargetPx = static_cast<tInt>(std::lround((sMm / wKColScale) * 96.0 / 25.4));
        tDouble wBest = 8.43;
        for (tDouble wGuess = 0.5; wGuess <= 255.0; wGuess += 1.0 / 1024.0) {
            const tInt wPx = ExcelWidthToPixels(wGuess, sMdw);
            if (wPx == wTargetPx) {
                return wGuess;
            }
            if (wPx < wTargetPx) {
                wBest = wGuess;
            }
        }
        for (tDouble wGuess = wBest; wGuess <= 255.0; wGuess += 1.0 / 1024.0) {
            if (ExcelWidthToPixels(wGuess, sMdw) >= wTargetPx) {
                wBest = wGuess;
                break;
            }
        }
        // JSON stores mm with 3 decimals; nudge up when truncation would shrink vs import.
        if (ExcelColWidthCharsToSkMm(wBest, sMdw) + 0.0005 < sMm) {
            for (tDouble wGuess = wBest + 1.0 / 1024.0; wGuess <= 255.0; wGuess += 1.0 / 1024.0) {
                if (ExcelColWidthCharsToSkMm(wGuess, sMdw) + 0.0005 >= sMm) {
                    return wGuess;
                }
            }
        }
        return wBest;
    }

    namespace {
        /// Digit advance width as a fraction of the em box.
        struct tDigitEmWidth {
            const char* m_Name;
            tDouble     m_Em;
        };

        /// Measured on the shipped font files (widest of glyphs 0-9, taken at a
        /// large ppem so hinting rounds out). Longer names must precede the
        /// families they extend ("Arial Narrow" before "Arial") since lookup is
        /// a substring match.
        const tDigitEmWidth kDigitEmWidths[] = {
            { "arial narrow",    0.456 },
            { "times new roman", 0.500 },
            { "palatino",        0.500 },
            { "gill sans",       0.500 },
            { "aptos",           0.510 },
            { "calibri",         0.510 },
            { "trebuchet",       0.524 },
            { "tahoma",          0.546 },
            { "helvetica",       0.556 },
            { "arial",           0.556 },
            { "optima",          0.556 },
            { "rockwell",        0.587 },
            { "monaco",          0.600 },
            { "courier",         0.600 },
            { "menlo",           0.602 },
            { "comic sans",      0.610 },
            { "georgia",         0.614 },
            { "futura",          0.617 },
            { "lucida",          0.632 },
            { "verdana",         0.636 },
        };

        /// Calibri / Aptos, the Excel defaults, for unlisted families.
        const tDouble kDefaultDigitEm = 0.510;
    }

    // Excel derives column pixel widths from MDW, the width of the widest digit
    // of the workbook default font at 96 DPI. Guessing it from a handful of font
    // names is what made imported columns too narrow: every family absent from
    // the list fell back to Calibri's 7 px, so a Lucida Sans 11 workbook (MDW 9)
    // lost a fifth of every column and wrapped headers Excel shows on one line.
    tInt EstimateMaxDigitWidthPxFromFont(const tString& sFontName, tDouble sSizePt) {
        tString wNeedle;
        wNeedle.reserve(sFontName.size());
        for (char wC : sFontName) {
            wNeedle.push_back(static_cast<char>(
                std::tolower(static_cast<unsigned char>(wC))));
        }
        tDouble wDigitEm = kDefaultDigitEm;
        for (const tDigitEmWidth& wEntry : kDigitEmWidths) {
            if (wNeedle.find(wEntry.m_Name) != tString::npos) {
                wDigitEm = wEntry.m_Em;
                break;
            }
        }
        const tDouble wSizePt = (sSizePt > 0.0) ? sSizePt : 11.0;
        const tDouble wPx = wDigitEm * wSizePt * 96.0 / 72.0;
        return std::max(1, static_cast<tInt>(std::lround(wPx)));
    }

    // More precise conversion using MaxDigitWidth (MDW) from default font (styles.xml)
    tInt EstimateMaxDigitWidthPx(const tExcelPugiXMLReader& sReader) {
        tDouble wSizePt = 11.0;
        tString wFontName;
        const auto& wFonts = sReader.GetFonts();
        if (!wFonts.empty()) {
            wFontName = wFonts[0].name;
            if (wFonts[0].size > 0.0) {
                wSizePt = wFonts[0].size;
            }
        }
        return EstimateMaxDigitWidthPxFromFont(wFontName, wSizePt);
    }

    tBool IsBuiltinTimeOnlyNumFmtId(tInt sNumFmtId) {
        if (sNumFmtId >= 18 && sNumFmtId <= 21) {
            return true;
        }
        if (sNumFmtId >= 45 && sNumFmtId <= 47) {
            return true;
        }
        return false;
    }

    tBool FormatCodeLooksLikeTimeOnly(const tString& sFormatCode) {
        if (sFormatCode.empty()) {
            return false;
        }
        tBool wInQuote = false;
        tBool wInBracket = false;
        tBool wHasTime = false;
        tBool wHasDate = false;
        for (tSize wI = 0; wI < sFormatCode.size(); ++wI) {
            char wC = sFormatCode[wI];
            if (wC == '\\' && wI + 1 < sFormatCode.size()) {
                ++wI;
                continue;
            }
            if (wC == '"') {
                wInQuote = !wInQuote;
                continue;
            }
            if (!wInQuote) {
                if (wC == '[') {
                    wInBracket = true;
                    continue;
                }
                if (wC == ']') {
                    wInBracket = false;
                    continue;
                }
                if (wInBracket) {
                    const char wLc = static_cast<char>(std::tolower(static_cast<unsigned char>(wC)));
                    if (wLc == 'h') {
                        wHasTime = true;
                    }
                    continue;
                }
                const char wLc = static_cast<char>(std::tolower(static_cast<unsigned char>(wC)));
                if (wLc == 'h' || wLc == 's') {
                    wHasTime = true;
                } else if (wLc == 'd' || wLc == 'y') {
                    wHasDate = true;
                } else if (wLc == 'm' && wI + 1 < sFormatCode.size()) {
                    const char wNext = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(sFormatCode[wI + 1])));
                    if (wNext == 'm' || wNext == 's') {
                        wHasTime = true;
                    }
                }
            }
        }
        return wHasTime && !wHasDate;
    }

    tBool ShouldImportExcelSerialAsTimeOnly(tDouble sSerial, tInt sNumFmtId, const tString& sFormatCode) {
        if (IsBuiltinTimeOnlyNumFmtId(sNumFmtId)) {
            return true;
        }
        if (FormatCodeLooksLikeTimeOnly(sFormatCode)) {
            return true;
        }
        // Pure time-of-day values in Excel are stored as a fractional day in [0, 1).
        return sSerial >= 0.0 && sSerial < 1.0;
    }

    tDate ExcelTimeOnlySerialToSkDate(tDouble sSerial) {
        tDouble wFrac = sSerial - std::floor(sSerial);
        if (wFrac < 0.0) {
            wFrac = 0.0;
        }
        const tInt wTotalSec =
            static_cast<tInt>(std::llround(wFrac * 86400.0)) % 86400;
        tClassDate wDate;
        wDate.SetDateHour(1900, 1, 1, wTotalSec / 3600, (wTotalSec % 3600) / 60, wTotalSec % 60);
        return wDate.Value();
    }

    tDouble SkDateToExcelTimeSerial(tDate sValue) {
        tClassDate wDate(sValue);
        if (wDate.IsHours()) {
            const tInt wSec = wDate.Hour() * 3600 + wDate.Minute() * 60 + wDate.Second();
            return static_cast<tDouble>(wSec) / 86400.0;
        }
        // Legacy import bug: frac*86400 was stored as raw Unix seconds on 1970-01-01.
        if (sValue > 0 && sValue < 86400) {
            tInt wYear = 0;
            tInt wMonth = 0;
            tInt wDay = 0;
            wDate.YearMonthDay(wYear, wMonth, wDay);
            if (wYear == 1970 && wMonth == 1 && wDay == 1) {
                return static_cast<tDouble>(sValue) / 86400.0;
            }
        }
        const tInt wSec = wDate.Hour() * 3600 + wDate.Minute() * 60 + wDate.Second();
        return static_cast<tDouble>(wSec) / 86400.0;
    }

    tDouble SkDateToExcelSerial(tDate sValue) {
        // Match SkFormatDate::ExcelSerialFromUnixSeconds (25569 = 1970-01-01 in Excel serial space).
        const tDouble wSec = static_cast<tDouble>(sValue);
        const tDouble wDay = std::floor(wSec / 86400.0);
        const tDouble wFrac = (wSec - wDay * 86400.0) / 86400.0;
        return wDay + 25569.0 + wFrac;
    }

    tBool CssDeclaresTimeFormat(const tString& sCss) {
        const tSize wKeyPos = sCss.find("format-string:\"");
        if (wKeyPos == tString::npos) {
            return false;
        }
        const tSize wStart = wKeyPos + 15;
        const tSize wEnd = sCss.find('"', wStart);
        if (wEnd == tString::npos || wEnd <= wStart) {
            return false;
        }
        tString wFmt = sCss.substr(wStart, wEnd - wStart);
        for (char& wCh : wFmt) {
            wCh = static_cast<char>(std::tolower(static_cast<unsigned char>(wCh)));
        }
        const tBool wHasTime = wFmt.find("h:mm") != tString::npos || wFmt.find("[h]") != tString::npos;
        if (!wHasTime) {
            return false;
        }
        // dd/mm/yyyy + h:mm is datetime, not time-only.
        const tBool wHasDatePart =
            wFmt.find('y') != tString::npos
            || wFmt.find('d') != tString::npos
            || wFmt.find("am/pm") != tString::npos;
        return !wHasDatePart;
    }

    tBool VariantsEqualForNamedRangeAlias(const tVariant& sA, const tVariant& sB) {
        if (sA.Type() != sB.Type()) {
            return false;
        }
        switch (sA.Type()) {
            case tVariantType::t_null:
                return true;
            case tVariantType::t_bool:
                return sA.Bool() == sB.Bool();
            case tVariantType::t_int:
                return sA.Int() == sB.Int();
            case tVariantType::t_double:
                return std::fabs(sA.Double() - sB.Double()) < 1e-9;
            case tVariantType::t_string:
                return sA.String() == sB.String();
            case tVariantType::t_date: {
                tClassDate wA(sA.Date());
                tClassDate wB(sB.Date());
                return wA.Hour() == wB.Hour()
                    && wA.Minute() == wB.Minute()
                    && wA.Second() == wB.Second();
            }
            default:
                return false;
        }
    }

    tInt ExcelWidthToPixels(tDouble sWidth, tInt sMdw) {
        // Microsoft formula: pixels = Truncate(((256*width + Truncate(128/MDW)) * MDW) / 256)
        tInt wPart = (tInt)std::floor(128.0 / std::max(1, sMdw));
        tInt wPixels = (tInt)std::floor(((256.0 * sWidth + wPart) * sMdw) / 256.0);
        return wPixels;
    }

    tBool CssDeclaresFont(const tString& sCss) {
        if (sCss.empty()) {
            return false;
        }
        return sCss.find("font-family:") != tString::npos
            || sCss.find("font:\"") != tString::npos;
    }

    tString PrependDefaultFontCss(const tString& sCss, const tString& sFontName, tDouble sFontSizePt) {
        if (CssDeclaresFont(sCss) || sFontName.empty()) {
            return sCss;
        }
        tString wOut;
        wOut.reserve(sCss.size() + sFontName.size() + 32);
        wOut += "font-family:'";
        wOut += sFontName;
        wOut += "',serif;";
        if (sFontSizePt > 0.0) {
            wOut += "font-size:";
            wOut += std::to_string(static_cast<tInt>(std::lround(sFontSizePt)));
            wOut += "pt;";
        }
        wOut += sCss;
        return wOut;
    }

} // End Of Namesapce

