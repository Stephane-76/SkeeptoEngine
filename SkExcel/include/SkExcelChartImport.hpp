#ifndef SkExcelChartImport_hpp
#define SkExcelChartImport_hpp

//=============================================================================
// SkExcelChartImport.hpp — shared Excel chart → Sker range helpers
//=============================================================================
#include "SkExcelPugiXMLReader.hpp"

#include <SkRangeRefTransform.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>

namespace SkExcel {

namespace ChartImport {

inline tString StripExcelRefDollars(tString sText) {
    tString wOut;
    wOut.reserve(sText.size());
    for (tChar wCh : sText) {
        if (wCh != '$') {
            wOut.push_back(wCh);
        }
    }
    return wOut;
}

struct tRefBounds {
    tString Sheet;
    tInt MinRow = std::numeric_limits<tInt>::max();
    tInt MaxRow = 0;
    tInt MinCol = std::numeric_limits<tInt>::max();
    tInt MaxCol = 0;
    tBool Valid = false;

    void ExpandCell(const tString& sSheet, tInt sRow, tInt sCol) {
        if (sSheet.empty() || sRow <= 0 || sCol <= 0) {
            return;
        }
        if (!Valid) {
            Sheet = sSheet;
        }
        Valid = true;
        MinRow = std::min(MinRow, sRow);
        MaxRow = std::max(MaxRow, sRow);
        MinCol = std::min(MinCol, sCol);
        MaxCol = std::max(MaxCol, sCol);
    }
};

inline tBool ParseA1CellToken(const tString& sToken, tString& oSheet, tInt& oRow, tInt& oCol) {
    tString wToken = sToken;
    while (!wToken.empty() && wToken.front() == ' ') {
        wToken.erase(wToken.begin());
    }
    while (!wToken.empty() && wToken.back() == ' ') {
        wToken.pop_back();
    }
    if (wToken.empty()) {
        return false;
    }

    oSheet.clear();
    tString wCellPart = wToken;
    const tSize wBang = wToken.find('!');
    if (wBang != tString::npos) {
        oSheet = wToken.substr(0, wBang);
        if (!oSheet.empty() && oSheet.front() == '\'') {
            oSheet.erase(oSheet.begin());
        }
        if (!oSheet.empty() && oSheet.back() == '\'') {
            oSheet.pop_back();
        }
        wCellPart = wToken.substr(wBang + 1);
    }

    const tSize wColon = wCellPart.find(':');
    if (wColon != tString::npos) {
        wCellPart = wCellPart.substr(0, wColon);
    }

    tInt wCol = 0;
    tSize wIdx = 0;
    while (wIdx < wCellPart.size() && std::isalpha(static_cast<unsigned char>(wCellPart[wIdx]))) {
        wCol = wCol * 26 + (std::toupper(static_cast<unsigned char>(wCellPart[wIdx])) - 'A' + 1);
        ++wIdx;
    }
    if (wCol <= 0 || wIdx >= wCellPart.size()) {
        return false;
    }
    const tInt wRow = std::atoi(wCellPart.c_str() + wIdx);
    if (wRow <= 0) {
        return false;
    }
    oRow = wRow;
    oCol = wCol;
    return true;
}

inline tString ColLettersFromBase10(tInt sCol) {
    tString wOut;
    tInt wCol = sCol;
    do {
        const tInt wRem = (wCol - 1) % 26;
        wOut.insert(wOut.begin(), static_cast<char>('A' + wRem));
        wCol = (wCol - 1) / 26;
    } while (wCol > 0);
    return wOut;
}

inline tBool SheetNameNeedsQuotes(const tString& sSheetName) {
    if (sSheetName.empty()) {
        return false;
    }
    for (tChar wChar : sSheetName) {
        if (!(std::isalnum(static_cast<unsigned char>(wChar)) != 0 || wChar == '_')) {
            return true;
        }
    }
    return false;
}

inline tString FormatSheetRangePrefix(const tString& sSheetName) {
    if (sSheetName.empty()) {
        return tString("");
    }
    if (SheetNameNeedsQuotes(sSheetName)) {
        return tString("'") + sSheetName + "'!";
    }
    return sSheetName + "!";
}

inline void ExpandFormulaPartBounds(const tString& sPart, tRefBounds& ioBounds) {
    tString wPart = StripExcelRefDollars(sPart);
    while (!wPart.empty() && wPart.front() == ' ') {
        wPart.erase(wPart.begin());
    }
    while (!wPart.empty() && wPart.back() == ' ') {
        wPart.pop_back();
    }
    if (wPart.empty()) {
        return;
    }

    tString wSheet;
    tString wCellPart = wPart;
    const tSize wBang = wPart.find('!');
    if (wBang != tString::npos) {
        wSheet = wPart.substr(0, wBang);
        if (!wSheet.empty() && wSheet.front() == '\'') {
            wSheet.erase(wSheet.begin());
        }
        if (!wSheet.empty() && wSheet.back() == '\'') {
            wSheet.pop_back();
        }
        wCellPart = wPart.substr(wBang + 1);
    }

    const tSize wColon = wCellPart.find(':');
    if (wColon == tString::npos) {
        tInt wRow = 0;
        tInt wCol = 0;
        if (ParseA1CellToken(wPart, wSheet, wRow, wCol)) {
            ioBounds.ExpandCell(wSheet, wRow, wCol);
        }
        return;
    }

    tString wLeft = wCellPart.substr(0, wColon);
    tString wRight = wCellPart.substr(wColon + 1);
    if (!wSheet.empty()) {
        if (wLeft.find('!') == tString::npos) {
            wLeft = wSheet + "!" + wLeft;
        }
        if (wRight.find('!') == tString::npos) {
            wRight = wSheet + "!" + wRight;
        }
    }

    tInt wRowA = 0;
    tInt wColA = 0;
    tInt wRowB = 0;
    tInt wColB = 0;
    tString wSheetA;
    tString wSheetB;
    if (!ParseA1CellToken(wLeft, wSheetA, wRowA, wColA)) {
        return;
    }
    if (!ParseA1CellToken(wRight, wSheetB, wRowB, wColB)) {
        ioBounds.ExpandCell(wSheetA, wRowA, wColA);
        return;
    }
    const tString wUseSheet = !wSheetA.empty() ? wSheetA : wSheetB;
    ioBounds.ExpandCell(wUseSheet, std::min(wRowA, wRowB), std::min(wColA, wColB));
    ioBounds.ExpandCell(wUseSheet, std::max(wRowA, wRowB), std::max(wColA, wColB));
}

inline tRefBounds BoundsFromExcelRefFormula(const tString& sFormula) {
    tRefBounds wBounds;
    tString wText = StripExcelRefDollars(sFormula);
    while (!wText.empty() && wText.front() == ' ') {
        wText.erase(wText.begin());
    }
    while (!wText.empty() && wText.back() == ' ') {
        wText.pop_back();
    }
    if (wText.empty()) {
        return wBounds;
    }
    if (wText.front() == '=') {
        wText.erase(wText.begin());
    }
    if (wText.front() == '(' && wText.back() == ')') {
        wText = wText.substr(1, wText.size() - 2);
    }

    // Union refs: (Sheet!A1:A2,Sheet!A5) — do not split on commas inside 'Sheet, Name' quotes.
    tString wPart;
    tInt wDepth = 0;
    tBool wInQuote = false;
    for (tSize wIdx = 0; wIdx <= wText.size(); ++wIdx) {
        const tChar wCh = (wIdx < wText.size()) ? wText[wIdx] : ',';
        if (wCh == '\'') {
            wInQuote = !wInQuote;
        } else if (!wInQuote) {
            if (wCh == '(') {
                ++wDepth;
            } else if (wCh == ')') {
                --wDepth;
            }
        }
        if ((wCh == ',' && wDepth == 0 && !wInQuote) || wIdx == wText.size()) {
            if (!wPart.empty()) {
                ExpandFormulaPartBounds(wPart, wBounds);
            }
            wPart.clear();
        } else {
            wPart.push_back(wCh);
        }
    }
    return wBounds;
}

inline tString RangeFromBounds(const tRefBounds& sBounds) {
    if (!sBounds.Valid) {
        return tString("");
    }
    tString wOut = FormatSheetRangePrefix(sBounds.Sheet);
    wOut += ColLettersFromBase10(sBounds.MinCol) + std::to_string(sBounds.MinRow);
    if (sBounds.MinRow != sBounds.MaxRow || sBounds.MinCol != sBounds.MaxCol) {
        wOut += ":" + ColLettersFromBase10(sBounds.MaxCol) + std::to_string(sBounds.MaxRow);
    }
    return wOut;
}

inline tString ToSkerRangeFormula(const tString& sHostSheet, const tString& sExcelFormula) {
    const tRefBounds wBounds = BoundsFromExcelRefFormula(sExcelFormula);
    const tString wRange = RangeFromBounds(wBounds);
    if (wRange.empty()) {
        return tString("");
    }
    return SkSpreadSheet::QualifyRefsForSheet(sHostSheet, tString("=") + wRange);
}

/** Range attribute wire for floating charts: =DATARANGE(Sheet1!A1:A5) (same as sparklines / UI Apply). */
inline tString ToSkerDataRangeFormula(const tString& sHostSheet, const tString& sRangeA1) {
    if (sRangeA1.empty()) {
        return tString("");
    }
    const tString wFormula = tString("=DATARANGE(") + sRangeA1 + ")";
    return SkSpreadSheet::QualifyRefsForSheet(sHostSheet, wFormula);
}

inline tBool BuildSkerChartRanges(
    const tExcelChartEntry& sEntry,
    tString& oChartData,
    tString& oDataRange,
    tString& oSeriesLabels) {
    if (sEntry.Series.empty()) {
        return false;
    }

    tRefBounds wLabelBounds;
    tRefBounds wValueBounds;
    tRefBounds wNameBounds;

    for (const tExcelChartSeriesEntry& wSeries : sEntry.Series) {
        if (!wSeries.CategoryRef.empty()) {
            const tRefBounds wCat = BoundsFromExcelRefFormula(wSeries.CategoryRef);
            if (wCat.Valid) {
                if (!wLabelBounds.Valid) {
                    wLabelBounds = wCat;
                } else {
                    wLabelBounds.MinRow = std::min(wLabelBounds.MinRow, wCat.MinRow);
                    wLabelBounds.MaxRow = std::max(wLabelBounds.MaxRow, wCat.MaxRow);
                    wLabelBounds.MinCol = std::min(wLabelBounds.MinCol, wCat.MinCol);
                    wLabelBounds.MaxCol = std::max(wLabelBounds.MaxCol, wCat.MaxCol);
                }
            }
        }
        if (!wSeries.ValueRef.empty()) {
            const tRefBounds wVal = BoundsFromExcelRefFormula(wSeries.ValueRef);
            if (wVal.Valid) {
                if (!wValueBounds.Valid) {
                    wValueBounds = wVal;
                } else {
                    wValueBounds.MinRow = std::min(wValueBounds.MinRow, wVal.MinRow);
                    wValueBounds.MaxRow = std::max(wValueBounds.MaxRow, wVal.MaxRow);
                    wValueBounds.MinCol = std::min(wValueBounds.MinCol, wVal.MinCol);
                    wValueBounds.MaxCol = std::max(wValueBounds.MaxCol, wVal.MaxCol);
                }
            }
        }
        if (!wSeries.NameRef.empty()) {
            const tRefBounds wName = BoundsFromExcelRefFormula(wSeries.NameRef);
            if (wName.Valid) {
                if (!wNameBounds.Valid) {
                    wNameBounds = wName;
                } else {
                    wNameBounds.MinRow = std::min(wNameBounds.MinRow, wName.MinRow);
                    wNameBounds.MaxRow = std::max(wNameBounds.MaxRow, wName.MaxRow);
                    wNameBounds.MinCol = std::min(wNameBounds.MinCol, wName.MinCol);
                    wNameBounds.MaxCol = std::max(wNameBounds.MaxCol, wName.MaxCol);
                }
            }
        }
    }

    if (wLabelBounds.Valid && wValueBounds.Valid) {
        // Sker LineChart: one label column + rectangular value block (gaps skipped at render).
        wLabelBounds.MaxCol = wLabelBounds.MinCol;
        if (wValueBounds.MinRow > wLabelBounds.MinRow) {
            wLabelBounds.MinRow = std::min(wLabelBounds.MinRow, wValueBounds.MinRow);
        }
        if (wValueBounds.MaxRow > wLabelBounds.MaxRow) {
            wLabelBounds.MaxRow = std::max(wLabelBounds.MaxRow, wValueBounds.MaxRow);
        }
    }

    oChartData = wLabelBounds.Valid
        ? ToSkerDataRangeFormula(sEntry.SheetName, RangeFromBounds(wLabelBounds))
        : tString("");
    oDataRange = wValueBounds.Valid
        ? ToSkerDataRangeFormula(sEntry.SheetName, RangeFromBounds(wValueBounds))
        : tString("");
    oSeriesLabels = (wNameBounds.Valid && sEntry.Series.size() > 1)
        ? ToSkerDataRangeFormula(sEntry.SheetName, RangeFromBounds(wNameBounds))
        : tString("");

    return !oChartData.empty() && !oDataRange.empty();
}

} // namespace ChartImport
} // namespace SkExcel

#endif
