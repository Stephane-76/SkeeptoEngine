//=============================================================================
// SkExcelTools.hpp
//=============================================================================
#ifndef SkExcelTools_hpp
#define SkExcelTools_hpp

#include <SkTypes.hpp>
#include <SkClass.hpp>
#include <SkVariant.hpp>
#include <SkTools.hpp>
#include <SkApi.hpp>
#include "SkExcelPugiXMLReader.hpp"

namespace SkExcel {

    using namespace SkRoot;
    using namespace SkSpreadSheet;

    // INDIRECT import: 1 = rewrite French L1C1 / column shorthand / materialize A1 and compile as formula.
    inline constexpr tBool kSkExcelTransformIndirectEnabled = 1;

    /// @brief True when sFormula contains an INDIRECT(...) call outside string literals.
    tBool FormulaContainsIndirect(const tString& sFormula);

    /// @brief Store sFormulaText as a cell text value (keeps leading '=' visible, no CompilCell).
    void SetCellFormulaAsText(tCell* sCell, const tString& sFormulaText);

    /// @brief Replace Excel _xHHHH_ escapes (e.g. _x000a_) and real \\n \\r \\t with spaces; collapse runs.
    /// @param[in] sString tString
    /// @return tString
    tString& TrimSpacesBeforeNewline(tString& sString);

    /// @brief Normalize INDIRECT literals for SkSpreadSheet (French L1C1, column shorthand, @ removal).
    /// @param[in] sString tString
    /// @return tString
    tString& TransformFormulaIndirectNotation(tString& sString);

    /// @brief Same as above but expand R1C1 INDIRECT column shorthands. Does not bound A:A
    /// (that happens in ApplyFormulas once every sheet LastRow is known).
    tString& TransformFormulaIndirectNotation(tString& sString, tApi& sApi, tSheet* sFormulaSheet);

    /// @brief Replace whole-column refs ($G:$G) with bounded $G$1:$G$lastRow using sheet used extent.
    /// Leaves the shorthand unchanged when LastRow < 2 (target sheet not imported yet).
    tString& BoundWholeColumnRefsInFormula(tString& sString, tApi& sApi, tSheet* sFormulaSheet);

    /// @brief Replace French column-axis INDIRECT(...,FALSE|0) with bounded column range (A1 or R1C1).
    /// Call from ApplyFormulas per host cell so shared formulas get the correct column.
    tString& MaterializeIndirectR1C1RefsToA1(tString& sFormula,
                                            tInt sFormulaRow,
                                            tInt sFormulaCol,
                                            tApi& sApi,
                                            tSheet* sFormulaSheet);

    /// @brief Transform formula syntax (basic; whole-column bounds applied via overload with sApi/sFormulaSheet).
    /// @param[in] sString tString
    /// @return tString
    tString& TransFormFormulaSyntax(tString& sString);

    /// @brief Transform formula syntax with bounded whole-column INDIRECT / $C:$C expansion.
    tString& TransFormFormulaSyntax(tString& sString, tApi& sApi, tSheet* sFormulaSheet);

    /// @brief Same as above plus per-cell materialization of INDIRECT("C",FALSE) column-axis refs.
    tString& TransFormFormulaSyntax(tString& sString,
                                    tApi& sApi,
                                    tSheet* sFormulaSheet,
                                    tInt sFormulaRow,
                                    tInt sFormulaCol);

    /// @brief Escape JSON string
    /// @param[in] sStr tString
    /// @return tString
    tString EscapeJsonString(const tString& sStr);
    
    /// @brief Convert reference to row and column
    /// @param[in] sRef tString
    /// @return std::pair<tInt,tInt>
    std::pair<tInt,tInt> RefToRowCol(const tString& sRef);

    /// @brief Replace relative R1C1 refs like R[1]C[0] with A1 for the given formula cell (import parity vs SkSpreadSheet A1).
    tString ConvertR1C1RelativeRefsToA1(const tString& sFormula, tInt sFormulaRow, tInt sFormulaCol);

    /// @brief True when the formula uses Excel structured table references (TableName[[#Headers],...]).
    tBool FormulaContainsStructuredTableRef(const tString& sFormula);

    /// @brief Normalize a SkSpreadSheet user formula for OOXML export (R1C1 offsets, INDIRECT shorthands).
    tString PrepareFormulaForExcelExport(const tString& sFormula,
                                         tInt sFormulaRow,
                                         tInt sFormulaCol,
                                         tApi& sApi,
                                         tSheet* sFormulaSheet,
                                         const tString* sCellCss = nullptr);

    /// @brief Convert A1 cell refs to R1C1 relative to the shared-formula master cell (Excel $B5 at row 5 -> R[0]C2).
    tString ConvertA1CellRefsToR1C1(const tString& sFormula, tInt sHostRow, tInt sHostCol);

    /// @brief True when sFormula already uses R1C1 cell references (R[0]C[-1], R1C1, ...).
    tBool FormulaUsesR1C1CellRefs(const tString& sFormula);
    
    /// @brief Check if the first row of the table range contains the column header labels (e.g. "Période 0", "Articles").
    /// Some tables (e.g. Encaissements) have no header row in the sheet; others (e.g. Décaissements) do.
    /// @return true if at least 2 cells in the first row match the table column names.
    /// @param[in] sReader tExcelPugiXMLReader
    /// @param[in] sSheetIndex tInt
    /// @param[in] wTop tInt
    /// @param[in] wLeft tInt
    /// @param[in] wColumnNames tVectorString
    /// @return tBool
    tBool FirstRowMatchesColumnNames(const tExcelPugiXMLReader& sReader, tInt sSheetIndex,
        tInt wTop, tInt wLeft, const tVectorString& wColumnNames);

    /// @brief Convert points to millimeters (1 pt = 25.4/72 mm)
    /// @param[in] sPt tDouble
    /// @return tDouble
    tDouble PointsToMillimeters(tDouble sPt);

    /// @brief Convert millimeters to points (inverse of PointsToMillimeters)
    tDouble MillimetersToPoints(tDouble sMm);

    /// @brief Column-width scale for xlsx import/export (1.0 = Excel pixel width at 96 DPI).
    tDouble ExcelColWidthKScale();

    /// @brief Excel row height (pt) -> SkSpreadSheet storage (mm).
    tDouble ExcelRowHeightPtToSkMm(tDouble sPt);

    /// @brief SkSpreadSheet storage (mm) -> Excel row height (pt).
    tDouble SkMmToExcelRowHeightPt(tDouble sMm);

    /// @brief Excel column width (chars) -> SkSpreadSheet storage (mm).
    tDouble ExcelColWidthCharsToSkMm(tDouble sWidthChars, tInt sMdw);

    /// @brief SkSpreadSheet storage (mm) -> Excel column width (chars).
    tDouble SkMmToExcelColWidthChars(tDouble sMm, tInt sMdw);
    
    /// @brief Convert pixels to millimeters (approximate)
    /// @param[in] sPx tDouble
    /// @return tDouble
    tDouble PixelsToMillimeters(tDouble sPx);

    /// @brief Estimate MaxDigitWidth in pixels from workbook styles
    /// @param[in] sReader tExcelPugiXMLReader
    /// @return tInt

    /// @brief Estimate MaxDigitWidth in pixels from workbook styles
    /// @param[in] sReader tExcelPugiXMLReader
    /// @return tInt
    tInt EstimateMaxDigitWidthPx(const tExcelPugiXMLReader& sReader);

    /// @brief Same MDW heuristic as EstimateMaxDigitWidthPx, from a font name/size.
    tInt EstimateMaxDigitWidthPxFromFont(const tString& sFontName, tDouble sSizePt);

    /// @brief Excel column width to pixels (Microsoft formula)
    /// @param[in] sWidth tDouble
    /// @param[in] sMdw tInt
    /// @return tInt
    tInt ExcelWidthToPixels(tDouble sWidth, tInt sMdw);

    /// @brief True for built-in OOXML numFmtIds that are time-of-day or duration (no calendar date).
    tBool IsBuiltinTimeOnlyNumFmtId(tInt sNumFmtId);

    /// @brief True when the format code displays time only (h:mm, [h]:mm:ss, …) without d/y date parts.
    tBool FormatCodeLooksLikeTimeOnly(const tString& sFormatCode);

    /// @brief Decide whether an Excel serial cell value should use the time-only import path.
    tBool ShouldImportExcelSerialAsTimeOnly(tDouble sSerial, tInt sNumFmtId, const tString& sFormatCode);

    /// @brief Excel time serial (day fraction, e.g. 0.3125 = 7:30) -> SkSpreadSheet tDate anchored at 1900-01-01.
    tDate ExcelTimeOnlySerialToSkDate(tDouble sSerial);

    /// @brief SkSpreadSheet time-of-day tDate -> Excel day fraction for h:mm export.
    tDouble SkDateToExcelTimeSerial(tDate sValue);

    /// @brief SkSpreadSheet calendar tDate (Unix time_t) -> Excel date serial (1900-based).
    tDouble SkDateToExcelSerial(tDate sValue);

    /// @brief True when CSS format-string is time-only (h:mm), not a date format.
    tBool CssDeclaresTimeFormat(const tString& sCss);

    /// @brief Compare cell values when detecting named-range alias formulas on export.
    tBool VariantsEqualForNamedRangeAlias(const tVariant& sA, const tVariant& sB);

    /// @brief True when CSS already declares a font (shorthand or font-family).
    tBool CssDeclaresFont(const tString& sCss);

    /// @brief Prepend workbook default font properties when CSS has none.
    tString PrependDefaultFontCss(const tString& sCss, const tString& sFontName, tDouble sFontSizePt);
    
}

#endif // SkExcelTools_hpp
