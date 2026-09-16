//=============================================================================
// SkFillSeries implementation
//=============================================================================
#include "../include/SkFillSeries.hpp"

#include <SkApplication.hpp>
#include <SkLocale.hpp>
#include <SkTypesClass.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using namespace SkRoot;

namespace SkSpreadSheet {

    namespace {

        //! Normalize a token for name lookup: trim, drop trailing dots, lowercase.
        tString NormToken(const tString& sValue) {
            tString wStr = tClassString(sValue).Trim();
            while (!wStr.empty() && wStr.back() == '.') {
                wStr.pop_back();
            }
            return tClassString(wStr).Lower();
        }

        enum class tCaseStyle { Title, Upper, Lower };

        //! Preserve the seed's visual style on generated names.
        tCaseStyle DetectCase(const tString& sValue) {
            tString wStr = tClassString(sValue).Trim();
            if (wStr.empty()) {
                return tCaseStyle::Title;
            }
            // Count ASCII letters (non-ASCII bytes are ignored for the decision).
            tString wLetters;
            for (char wCh : wStr) {
                if (std::isalpha(static_cast<unsigned char>(wCh))) {
                    wLetters.push_back(wCh);
                }
            }
            if (wLetters.size() > 1 && wLetters == tClassString(wLetters).Upper()) {
                return tCaseStyle::Upper;
            }
            if (wStr == tClassString(wStr).Lower()) {
                return tCaseStyle::Lower;
            }
            return tCaseStyle::Title;
        }

        tString ApplyCase(const tString& sName, tCaseStyle sStyle) {
            if (sStyle == tCaseStyle::Upper) {
                return tClassString(sName).Upper();
            }
            if (sStyle == tCaseStyle::Lower) {
                return tClassString(sName).Lower();
            }
            // Title-case: first byte upper, rest as authored in the table.
            if (sName.empty()) {
                return sName;
            }
            tString wOut = sName;
            wOut[0] = static_cast<char>(
                std::toupper(static_cast<unsigned char>(wOut[0])));
            return wOut;
        }

        //---------------------------------------------------------------------
        // Number helpers.
        //---------------------------------------------------------------------
        struct tParsedNumber {
            tDouble m_Value = 0.0;
            tInt m_Decimals = 0;
            tBool m_Comma = false;
            tBool m_Ok = false;
        };

        //! Parse "-?\d+([.,]\d+)?" (whole string). Returns m_Ok == false otherwise.
        tParsedNumber ParseNumber(const tString& sValue) {
            tParsedNumber wOut;
            tString wStr = tClassString(sValue).Trim();
            tClassString wCheck(wStr);
            if (wStr.empty() || !wCheck.Regex_Match("^-?\\d+([.,]\\d+)?$")) {
                return wOut;
            }
            tSize wPos = 0;
            if (wStr[wPos] == '-') {
                wPos++;
            }
            tSize wIntStart = wPos;
            while (wPos < wStr.size() &&
                   std::isdigit(static_cast<unsigned char>(wStr[wPos]))) {
                wPos++;
            }
            if (wPos == wIntStart) {
                return wOut; // no integer digits
            }
            tInt wDecimals = 0;
            tBool wComma = false;
            if (wPos < wStr.size() && (wStr[wPos] == '.' || wStr[wPos] == ',')) {
                wComma = (wStr[wPos] == ',');
                wPos++;
                tSize wFracStart = wPos;
                while (wPos < wStr.size() &&
                       std::isdigit(static_cast<unsigned char>(wStr[wPos]))) {
                    wPos++;
                }
                if (wPos == wFracStart) {
                    return wOut; // separator without fractional digits
                }
                wDecimals = static_cast<tInt>(wPos - wFracStart);
            }
            if (wPos != wStr.size()) {
                return wOut; // trailing garbage
            }
            // Convert to double using a dot separator.
            tString wNormalized = wStr;
            std::replace(wNormalized.begin(), wNormalized.end(), ',', '.');
            wOut.m_Value = std::strtod(wNormalized.c_str(), nullptr);
            wOut.m_Decimals = wDecimals;
            wOut.m_Comma = wComma;
            wOut.m_Ok = true;
            return wOut;
        }

        //! Round away tiny floating-point noise then format with a decimal count.
        tString FormatNumber(tDouble sValue, tInt sDecimals, tBool sComma) {
            tInt wDecimals = std::min<tInt>(12, std::max<tInt>(0, sDecimals));
            char wBuffer[64];
            std::snprintf(wBuffer, sizeof(wBuffer), "%.*f", wDecimals, sValue);
            tString wStr(wBuffer);
            // Guard against "-0" / "-0.00".
            if (!wStr.empty() && wStr[0] == '-') {
                tBool wAllZero = true;
                for (char wCh : wStr) {
                    if (wCh != '-' && wCh != '0' && wCh != '.') {
                        wAllZero = false;
                        break;
                    }
                }
                if (wAllZero) {
                    wStr = wStr.substr(1);
                }
            }
            if (sComma) {
                std::replace(wStr.begin(), wStr.end(), '.', ',');
            }
            return wStr;
        }

        //! Least-squares slope for y at x = 0..n-1 (n >= 2).
        tDouble LinearStep(const std::vector<tDouble>& sYs) {
            tSize wN = sYs.size();
            if (wN < 2) {
                return 0.0;
            }
            tDouble wMeanX = (static_cast<tDouble>(wN) - 1.0) / 2.0;
            tDouble wSum = 0.0;
            for (tDouble wY : sYs) {
                wSum += wY;
            }
            tDouble wMeanY = wSum / static_cast<tDouble>(wN);
            tDouble wNum = 0.0;
            tDouble wDen = 0.0;
            for (tSize i = 0; i < wN; i++) {
                tDouble wDx = static_cast<tDouble>(i) - wMeanX;
                wNum += wDx * (sYs[i] - wMeanY);
                wDen += wDx * wDx;
            }
            return wDen == 0.0 ? 0.0 : wNum / wDen;
        }

        //---------------------------------------------------------------------
        // Localized cyclic name detection (months / weekdays).
        //---------------------------------------------------------------------
        struct tNameHit {
            tInt m_Index = 0;
            tBool m_Abbr = false;
            tBool m_Ok = false;
        };

        //! Build the localized name table (full + abbreviated), 0-based.
        void BuildNames(tBool sMonths,
                        std::vector<tString>& oFull,
                        std::vector<tString>& oAbbr) {
            tLocale* wLocale = tApplication::Instance()->Locale();
            if (sMonths) {
                oFull.resize(12);
                oAbbr.resize(12);
                for (tInt i = 0; i < 12; i++) {
                    oFull[i] = wLocale->MonthStr(i + 1);
                    oAbbr[i] = wLocale->MonthStrAbbr(i + 1);
                }
            } else {
                // tLocale weekdays: 0 = Sunday .. 6 = Saturday.
                oFull.resize(7);
                oAbbr.resize(7);
                for (tInt i = 0; i < 7; i++) {
                    oFull[i] = wLocale->DayStr(i);
                    oAbbr[i] = wLocale->DayStrAbbr(i);
                }
            }
        }

        tNameHit LookupName(const tString& sSeed,
                            const std::vector<tString>& sFull,
                            const std::vector<tString>& sAbbr) {
            tNameHit wHit;
            tString wKey = NormToken(sSeed);
            for (tSize i = 0; i < sFull.size(); i++) {
                if (NormToken(sFull[i]) == wKey) {
                    wHit.m_Index = static_cast<tInt>(i);
                    wHit.m_Abbr = false;
                    wHit.m_Ok = true;
                    return wHit;
                }
            }
            for (tSize i = 0; i < sAbbr.size(); i++) {
                if (NormToken(sAbbr[i]) == wKey) {
                    wHit.m_Index = static_cast<tInt>(i);
                    wHit.m_Abbr = true;
                    wHit.m_Ok = true;
                    return wHit;
                }
            }
            return wHit;
        }

        //---------------------------------------------------------------------
        // Detectors. Each returns a generator (index -> string) or an empty
        // std::function when the pattern does not match.
        //---------------------------------------------------------------------
        typedef std::function<tString(tSize)> tGenerator;

        tGenerator DetectNumber(const std::vector<tString>& sSeeds) {
            std::vector<tParsedNumber> wParsed;
            wParsed.reserve(sSeeds.size());
            for (const tString& wSeed : sSeeds) {
                tParsedNumber wNum = ParseNumber(wSeed);
                if (!wNum.m_Ok) {
                    return tGenerator();
                }
                wParsed.push_back(wNum);
            }
            std::vector<tDouble> wValues;
            tInt wDecimals = 0;
            for (const tParsedNumber& wNum : wParsed) {
                wValues.push_back(wNum.m_Value);
                wDecimals = std::max(wDecimals, wNum.m_Decimals);
            }
            tBool wComma = wParsed.back().m_Comma;
            tDouble wStep = wParsed.size() >= 2 ? LinearStep(wValues) : 0.0;
            tDouble wBase = wParsed.back().m_Value;
            return [wBase, wStep, wDecimals, wComma](tSize sIndex) -> tString {
                tDouble wNext = wBase + wStep * static_cast<tDouble>(sIndex + 1);
                return FormatNumber(wNext, wDecimals, wComma);
            };
        }

        tGenerator DetectCyclicName(const std::vector<tString>& sSeeds, tBool sMonths) {
            std::vector<tString> wFull;
            std::vector<tString> wAbbr;
            BuildNames(sMonths, wFull, wAbbr);
            tInt wCycle = static_cast<tInt>(wFull.size());

            std::vector<tNameHit> wHits;
            wHits.reserve(sSeeds.size());
            for (const tString& wSeed : sSeeds) {
                tNameHit wHit = LookupName(wSeed, wFull, wAbbr);
                if (!wHit.m_Ok) {
                    return tGenerator();
                }
                wHits.push_back(wHit);
            }
            tBool wLastAbbr = wHits.back().m_Abbr;
            tCaseStyle wCase = DetectCase(sSeeds.back());
            tInt wStep = 1;
            if (sSeeds.size() >= 2) {
                tInt wSpan = wHits.back().m_Index - wHits.front().m_Index;
                wStep = static_cast<tInt>(std::lround(
                    static_cast<tDouble>(wSpan) /
                    static_cast<tDouble>(sSeeds.size() - 1)));
                if (wStep == 0) {
                    wStep = 1;
                }
            }
            tInt wBase = wHits.back().m_Index;
            std::vector<tString> wNames = wLastAbbr ? wAbbr : wFull;
            return [wNames, wBase, wStep, wCycle, wCase](tSize sIndex) -> tString {
                tInt wIdx = (wBase + wStep * static_cast<tInt>(sIndex + 1)) % wCycle;
                if (wIdx < 0) {
                    wIdx += wCycle;
                }
                return ApplyCase(wNames[static_cast<tSize>(wIdx)], wCase);
            };
        }

        struct tPrefixParts {
            tString m_Prefix;
            tString m_Digits;
            tString m_Suffix;
            tBool m_Ok = false;
        };

        //! Match "^(\D*?)(\d+)(\D*)$" (last digit run kept as the number).
        tPrefixParts SplitPrefixNumber(const tString& sValue) {
            tPrefixParts wParts;
            // Find the last contiguous digit run.
            tSize wEnd = tString::npos;
            tSize wStart = tString::npos;
            for (tSize i = sValue.size(); i-- > 0;) {
                if (std::isdigit(static_cast<unsigned char>(sValue[i]))) {
                    if (wEnd == tString::npos) {
                        wEnd = i;
                    }
                    wStart = i;
                } else if (wEnd != tString::npos) {
                    break;
                }
            }
            if (wEnd == tString::npos) {
                return wParts;
            }
            // Suffix must contain no further digits (regex \D*).
            for (tSize i = wEnd + 1; i < sValue.size(); i++) {
                if (std::isdigit(static_cast<unsigned char>(sValue[i]))) {
                    return wParts;
                }
            }
            wParts.m_Prefix = sValue.substr(0, wStart);
            wParts.m_Digits = sValue.substr(wStart, wEnd - wStart + 1);
            wParts.m_Suffix = sValue.substr(wEnd + 1);
            wParts.m_Ok = true;
            return wParts;
        }

        tGenerator DetectPrefixNumber(const std::vector<tString>& sSeeds) {
            std::vector<tPrefixParts> wParts;
            wParts.reserve(sSeeds.size());
            for (const tString& wSeed : sSeeds) {
                tPrefixParts wPart = SplitPrefixNumber(wSeed);
                if (!wPart.m_Ok) {
                    return tGenerator();
                }
                wParts.push_back(wPart);
            }
            tString wPrefix = wParts.front().m_Prefix;
            tString wSuffix = wParts.front().m_Suffix;
            for (const tPrefixParts& wPart : wParts) {
                if (wPart.m_Prefix != wPrefix || wPart.m_Suffix != wSuffix) {
                    return tGenerator();
                }
            }
            std::vector<tDouble> wNums;
            for (const tPrefixParts& wPart : wParts) {
                wNums.push_back(std::strtod(wPart.m_Digits.c_str(), nullptr));
            }
            tInt wStep = wParts.size() >= 2
                             ? static_cast<tInt>(std::lround(LinearStep(wNums)))
                             : 1;
            tLong wBase = static_cast<tLong>(std::llround(wNums.back()));
            tSize wWidth = wParts.front().m_Digits.size();
            tBool wPadded = wWidth > 1;
            for (const tPrefixParts& wPart : wParts) {
                if (wPart.m_Digits.size() != wWidth) {
                    wPadded = false;
                    break;
                }
            }
            return [wPrefix, wSuffix, wBase, wStep, wPadded, wWidth](tSize sIndex) -> tString {
                tLong wNext = wBase +
                              static_cast<tLong>(wStep) * static_cast<tLong>(sIndex + 1);
                tLong wAbs = wNext < 0 ? -wNext : wNext;
                tString wDigits = std::to_string(wAbs);
                if (wPadded && wDigits.size() < wWidth) {
                    wDigits = tString(wWidth - wDigits.size(), '0') + wDigits;
                }
                tString wSign = wNext < 0 ? "-" : "";
                return wPrefix + wSign + wDigits + wSuffix;
            };
        }

        //---------------------------------------------------------------------
        // Date series detection (day / month step) via tClassDate — same parse
        // path as tVariant (FormatDateShort / FormatDateLong).
        //---------------------------------------------------------------------
        struct tDateLayout {
            tInt m_DayField = 0;
            tInt m_MonthField = 1;
            tInt m_YearField = 2;
        };

        //! Field order (day / month / year) from the locale short date pattern.
        tDateLayout LocaleDateLayout() {
            tDateLayout wLayout;
            tLocale* wLocale = tApplication::Instance()->Locale();
            tString wFmt = wLocale->FormatDateShort();
            tInt wField = 0;
            for (tSize i = 0; i + 1 < wFmt.size(); ++i) {
                if (wFmt[i] != '%') {
                    continue;
                }
                char wSpec = wFmt[i + 1];
                if (wSpec == 'd') {
                    wLayout.m_DayField = wField++;
                } else if (wSpec == 'm') {
                    wLayout.m_MonthField = wField++;
                } else if (wSpec == 'y' || wSpec == 'Y') {
                    wLayout.m_YearField = wField++;
                }
            }
            return wLayout;
        }

        //! Parse a seed the same way tVariant does for typed dates (long then short).
        tBool ParseSeedDate(const tString& sSeed, tClassDate& oDate) {
            tLocale* wLocale = tApplication::Instance()->Locale();
            tString wTrimmed = tClassString(sSeed).Trim();
            tClassDate wDate;
            if (wDate.ParseDateTime(wTrimmed.c_str(),
                                    wLocale->FormatDateLong().c_str())) {
                oDate = wDate;
                return true;
            }
            wDate.Clear();
            if (wDate.ParseDateTime(wTrimmed.c_str(),
                                    wLocale->FormatDateShort().c_str())) {
                oDate = wDate;
                return true;
            }
            return false;
        }

        tInt DaysInMonth(tInt sYear, tInt sMonth) {
            static const tInt wDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (sMonth == 2 &&
                ((sYear % 4 == 0 && sYear % 100 != 0) || (sYear % 400 == 0))) {
                return 29;
            }
            if (sMonth < 1 || sMonth > 12) {
                return 31;
            }
            return wDays[sMonth - 1];
        }

        tClassDate AddMonths(tClassDate sFrom, tLongLong sMonths) {
            tInt wYear = sFrom.Year();
            tInt wMonth = sFrom.Month();
            tInt wDay = sFrom.Day();
            tLongLong wIdx =
                static_cast<tLongLong>(wYear) * 12 + (wMonth - 1) + sMonths;
            tInt wNewYear = static_cast<tInt>(wIdx / 12);
            tInt wNewMonth = static_cast<tInt>(wIdx % 12) + 1;
            if (wNewMonth < 1) {
                wNewMonth += 12;
                wNewYear -= 1;
            }
            tClassDate wOut;
            wOut.Set(wNewYear, wNewMonth,
                     std::min(wDay, DaysInMonth(wNewYear, wNewMonth)));
            return wOut;
        }

        //! Split a seed into exactly 3 numeric fields; capture separator for output.
        tBool DateFields(const tString& sValue, std::vector<tString>& oFields, char& oSep) {
            oFields.clear();
            oSep = '\0';
            tString wCur;
            tString wStr = tClassString(sValue).Trim();
            for (char wCh : wStr) {
                if (std::isdigit(static_cast<unsigned char>(wCh))) {
                    wCur.push_back(wCh);
                } else {
                    if (wCur.empty()) {
                        return false;
                    }
                    oFields.push_back(wCur);
                    wCur.clear();
                    if (oSep == '\0') {
                        oSep = wCh;
                    } else if (wCh != oSep) {
                        return false;
                    }
                }
            }
            if (wCur.empty()) {
                return false;
            }
            oFields.push_back(wCur);
            return oFields.size() == 3;
        }

        tGenerator DetectDate(const std::vector<tString>& sSeeds) {
            // A single seed keeps the historical "prefix+number" behaviour
            // (increments the year); only 2+ seeds define a date step.
            if (sSeeds.size() < 2) {
                return tGenerator();
            }

            std::vector<tClassDate> wDates;
            wDates.reserve(sSeeds.size());
            for (const tString& wSeed : sSeeds) {
                tClassDate wDate;
                if (!ParseSeedDate(wSeed, wDate)) {
                    return tGenerator();
                }
                wDates.push_back(wDate);
            }

            tDateLayout wLayout = LocaleDateLayout();
            std::vector<tString> wFirstFields;
            char wSep = '\0';
            DateFields(tClassString(sSeeds.front()).Trim(), wFirstFields, wSep);
            tSize wDayWidth = wFirstFields[wLayout.m_DayField].size();
            tSize wMonthWidth = wFirstFields[wLayout.m_MonthField].size();
            tSize wYearWidth = wFirstFields[wLayout.m_YearField].size();

            tBool wDayConstant = true;
            for (tSize i = 1; i < wDates.size(); ++i) {
                if (wDates[i].Day() != wDates.front().Day()) {
                    wDayConstant = false;
                    break;
                }
            }

            tBool wMonthMode = wDayConstant;
            tLongLong wStep = 0;
            if (wMonthMode) {
                std::vector<tDouble> wMonthIndex;
                for (tClassDate& wDate : wDates) {
                    wMonthIndex.push_back(static_cast<tDouble>(wDate.Year()) * 12.0 +
                                          static_cast<tDouble>(wDate.Month() - 1));
                }
                wStep = static_cast<tLongLong>(std::llround(LinearStep(wMonthIndex)));
                if (wStep == 0) {
                    wStep = 1;
                }
            } else {
                tClassDate wEpoch(1900, 1, 1);
                const tDate wEpochValue = wEpoch.Value();
                std::vector<tDouble> wDaySerials;
                for (tClassDate& wDate : wDates) {
                    wDaySerials.push_back(
                        static_cast<tDouble>(wDate.Value() - wEpochValue) / 86400.0);
                }
                wStep = static_cast<tLongLong>(std::llround(LinearStep(wDaySerials)));
                if (wStep == 0) {
                    wStep = 1;
                }
            }

            if (wSep == '\0') {
                wSep = '/';
            }
            tClassDate wLastDate = wDates.back();
            auto wFormat = [wLayout, wDayWidth, wMonthWidth, wYearWidth, wSep](
                               tInt sYear, tInt sMonth, tInt sDay) -> tString {
                char wBuf[16];
                tString wFields[3];
                std::snprintf(wBuf, sizeof(wBuf), "%0*d",
                              static_cast<int>(wDayWidth), sDay);
                wFields[wLayout.m_DayField] = wBuf;
                std::snprintf(wBuf, sizeof(wBuf), "%0*d",
                              static_cast<int>(wMonthWidth), sMonth);
                wFields[wLayout.m_MonthField] = wBuf;
                tInt wYearOut = sYear;
                if (wYearWidth <= 2) {
                    wYearOut = ((sYear % 100) + 100) % 100;
                }
                std::snprintf(wBuf, sizeof(wBuf), "%0*d",
                              static_cast<int>(wYearWidth), wYearOut);
                wFields[wLayout.m_YearField] = wBuf;
                tString wOut = wFields[0];
                wOut.push_back(wSep);
                wOut += wFields[1];
                wOut.push_back(wSep);
                wOut += wFields[2];
                return wOut;
            };

            if (wMonthMode) {
                return [wLastDate, wStep, wFormat](tSize sIndex) -> tString {
                    tClassDate wNext =
                        AddMonths(wLastDate, wStep * static_cast<tLongLong>(sIndex + 1));
                    return wFormat(wNext.Year(), wNext.Month(), wNext.Day());
                };
            }
            return [wLastDate, wStep, wFormat](tSize sIndex) -> tString {
                tClassDate wBase = wLastDate;
                tClassDate wNext =
                    wBase + static_cast<tInt>(wStep * static_cast<tLongLong>(sIndex + 1));
                return wFormat(wNext.Year(), wNext.Month(), wNext.Day());
            };
        }

        //---------------------------------------------------------------------
        // A1 reference shifting for formula continuation.
        //---------------------------------------------------------------------
        tBool IsRefLetter(char sCh) {
            return std::isalpha(static_cast<unsigned char>(sCh)) != 0;
        }

        //! "A" -> 1, "Z" -> 26, "AA" -> 27 ... (uppercase input).
        tLong ColLettersToNum(const tString& sLetters) {
            tLong wNum = 0;
            for (char wCh : sLetters) {
                wNum = wNum * 26 + (std::toupper(static_cast<unsigned char>(wCh)) - 'A' + 1);
            }
            return wNum;
        }

        tString ColNumToLetters(tLong sNum) {
            tString wOut;
            while (sNum > 0) {
                tLong wRem = (sNum - 1) % 26;
                wOut.insert(wOut.begin(), static_cast<char>('A' + wRem));
                sNum = (sNum - 1) / 26;
            }
            return wOut;
        }

    } // namespace

    //-------------------------------------------------------------------------
    tVectorString tFillSeries::Extend(const tVectorString& sSeeds, tSize sCount) {
        tVectorString wOut;
        if (sCount == 0 || sSeeds.empty()) {
            return wOut;
        }
        std::vector<tString> wTrimmed;
        wTrimmed.reserve(sSeeds.size());
        tBool wHasContent = false;
        tBool wAllContent = true;
        for (const tString& wSeed : sSeeds) {
            tString wT = tClassString(wSeed).Trim();
            if (!wT.empty()) {
                wHasContent = true;
            } else {
                wAllContent = false;
            }
            wTrimmed.push_back(wT);
        }

        tGenerator wGenerator;
        if (wHasContent && wAllContent) {
            wGenerator = DetectNumber(wTrimmed);
            if (!wGenerator) {
                wGenerator = DetectCyclicName(wTrimmed, true);
            }
            if (!wGenerator) {
                wGenerator = DetectCyclicName(wTrimmed, false);
            }
            if (!wGenerator) {
                wGenerator = DetectDate(wTrimmed);
            }
            if (!wGenerator) {
                wGenerator = DetectPrefixNumber(wTrimmed);
            }
        }

        wOut.reserve(sCount);
        if (wGenerator) {
            for (tSize i = 0; i < sCount; i++) {
                wOut.push_back(wGenerator(i));
            }
        } else {
            // Fallback: cyclically repeat the raw seeds (Excel copies non-series data).
            tSize wLen = sSeeds.size();
            for (tSize i = 0; i < sCount; i++) {
                wOut.push_back(sSeeds[i % wLen]);
            }
        }
        return wOut;
    }

    //-------------------------------------------------------------------------
    tString tFillSeries::ShiftFormula(const tString& sFormula, tInt sDeltaRow, tInt sDeltaCol) {
        if (sFormula.empty()) {
            return sFormula;
        }
        tString wOut;
        wOut.reserve(sFormula.size() + 8);
        tSize wPos = 0;
        tSize wLen = sFormula.size();
        tBool wInString = false; // inside a "..." literal
        char wPrev = '\0';       // last non-shifted character emitted from source

        while (wPos < wLen) {
            char wCh = sFormula[wPos];

            // String literals are copied verbatim (handles "" escapes loosely).
            if (wInString) {
                wOut.push_back(wCh);
                if (wCh == '"') {
                    wInString = false;
                }
                wPrev = wCh;
                wPos++;
                continue;
            }
            if (wCh == '"') {
                wInString = true;
                wOut.push_back(wCh);
                wPrev = wCh;
                wPos++;
                continue;
            }

            // Try to match an A1 reference token starting here:
            //   [$]? letters [$]? digits
            tSize wScan = wPos;
            tBool wColAbs = false;
            if (sFormula[wScan] == '$') {
                wColAbs = true;
                wScan++;
            }
            tSize wLettersStart = wScan;
            while (wScan < wLen && IsRefLetter(sFormula[wScan])) {
                wScan++;
            }
            tSize wLettersLen = wScan - wLettersStart;
            tBool wRowAbs = false;
            tSize wDollarRowPos = wScan;
            if (wScan < wLen && sFormula[wScan] == '$') {
                wRowAbs = true;
                wScan++;
            }
            tSize wDigitsStart = wScan;
            while (wScan < wLen &&
                   std::isdigit(static_cast<unsigned char>(sFormula[wScan]))) {
                wScan++;
            }
            tSize wDigitsLen = wScan - wDigitsStart;

            // A valid, standalone A1 reference needs 1..3 letters + digits, must
            // not be glued to a preceding identifier char, and must not be a
            // function call (followed by '(') or a sheet-qualified name we keep.
            tBool wPrevGlued = (wPrev == '_' ||
                                std::isalnum(static_cast<unsigned char>(wPrev)));
            tBool wNextGlued = (wScan < wLen) &&
                               (sFormula[wScan] == '(' || sFormula[wScan] == '_' ||
                                IsRefLetter(sFormula[wScan]));
            tBool wValid = (wLettersLen >= 1 && wLettersLen <= 3 && wDigitsLen >= 1 &&
                            !wPrevGlued && !wNextGlued);
            // The '$' between letters and digits is only allowed when we actually
            // have both parts; otherwise treat as non-ref.
            if (wValid && wRowAbs && wDigitsLen == 0) {
                wValid = false;
            }
            (void)wDollarRowPos;

            if (wValid) {
                tString wLetters = sFormula.substr(wLettersStart, wLettersLen);
                tString wDigits = sFormula.substr(wDigitsStart, wDigitsLen);
                tString wRef;
                // Column part.
                if (wColAbs) {
                    wRef += "$";
                    wRef += wLetters;
                } else {
                    tLong wCol = ColLettersToNum(tClassString(wLetters).Upper()) + sDeltaCol;
                    if (wCol < 1) {
                        wCol = 1;
                    }
                    wRef += ColNumToLetters(wCol);
                }
                // Row part.
                if (wRowAbs) {
                    wRef += "$";
                    wRef += wDigits;
                } else {
                    tLong wRow = std::strtol(wDigits.c_str(), nullptr, 10) + sDeltaRow;
                    if (wRow < 1) {
                        wRow = 1;
                    }
                    wRef += std::to_string(wRow);
                }
                wOut += wRef;
                wPrev = wRef.empty() ? wPrev : wRef.back();
                wPos = wScan;
                continue;
            }

            // Not a reference: copy one character.
            wOut.push_back(wCh);
            wPrev = wCh;
            wPos++;
        }
        return wOut;
    }

}
