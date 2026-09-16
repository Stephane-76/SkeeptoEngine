#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <SkTypes.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

namespace SkExcel {

// Forward declaration
class WorkbookBuilder;

/// @brief One Excel ListObject column for OOXML export (metadata round-trip from RangeData).
struct StructuredTableColumnSpec {
    tString name;
    tString calculatedFormula;
    tBool filterButtonHidden = false;
    tString totalsRowLabel;
    tString totalsRowFunction;
    tString totalsRowFormula;
};

// Export a minimal XLSX at outputPath with cell A1 containing text "coucou"
// and style: blue font on yellow fill.
/// @brief Export a minimal XLSX at outputPath with cell A1 containing text "coucou"
/// @param sOutputPath The path to the output file
/// @return True if successful, false otherwise
tBool ExportDemoXlsx(const tString& sOutputPath);

// Export a loaded SkSpreadSheet::tApi workbook to an .xlsx file.
/// @brief Export a loaded SkSpreadSheet::tApi workbook to an .xlsx file
/// @param sApi The SkSpreadSheet::tApi workbook to export
/// @param sOutputPath The path to the output file
/// @return True if successful, false otherwise
tBool ExportApiToXlsx(tApi& sApi, const tString& sOutputPath);

// Set cell font color using hex string: "#RRGGBB" or "#AARRGGBB" (alpha optional)
/// @brief Set cell font color using hex string: "#RRGGBB" or "#AARRGGBB" (alpha optional)  
/// @param sCell The cell address
/// @param sHexColor The hex color to set
/// @return True if successful, false otherwise
tBool SetCellFontColor(const tString& sCell, const tString& sHexColor);

// Set cell background fill color using hex string: "#RRGGBB" or "#AARRGGBB"
/// @brief Set cell background fill color using hex string: "#RRGGBB" or "#AARRGGBB"
/// @param sCell The cell address
/// @param sHexColor The hex color to set
/// @return True if successful, false otherwise
tBool SetCellFillColor(const tString& sCell, const tString& sHexColor);

// Set cell border color (all sides) using hex string: "#RRGGBB" or "#AARRGGBB"
/// @brief Set cell border color (all sides) using hex string: "#RRGGBB" or "#AARRGGBB"
/// @param sCell The cell address
/// @param sHexColor The hex color to set
/// @return True if successful, false otherwise
tBool SetCellBorderColor(const tString& sCell, const tString& sHexColor);

// Set cell borders per side. style in {thin, medium, thick, dashed, dotted, double} or "".
// Colors accept "#RRGGBB" or "#AARRGGBB" (empty string to skip).
tBool SetCellBorderSides(   
    const tString& sCell,
    const tString& leftStyle,  const tString& leftColor,
    const tString& rightStyle, const tString& rightColor,
    const tString& topStyle,   const tString& topColor,
    const tString& bottomStyle,const tString& bottomColor);

// Set cell value
tBool SetCellValue(const tString& sCell, const tString& value);

// Set cell formula
tBool SetCellFormula(const tString& sCell, const tString& formula);

// Set custom number format for a cell
tBool SetCellNumberFormat(const tString& sCell, const tString& formatCode);

// Set cell as currency with symbol (default: $)
tBool SetCellCurrency(const tString& sCell, const tString& symbol = "$");

// Set cell as percentage with decimal places (default: 2)
tBool SetCellPercentage(const tString& sCell, tInt decimalPlaces = 2);

// Set cell as date with format (default: mm-dd-yy)
tBool SetCellDate(const tString& sCell, const tString& dateFormat = "mm-dd-yy");

// Set cell with Excel date serial number and French format
tBool SetCellDateSerial(const tString& sCell, tDouble excelSerial, const tString& frenchFormat = "[$-fr-FR]dd/MM/yyyy");

// Set cell with date components (year, month, day) and French format
tBool SetCellDateYMD(const tString& sCell, tInt year, tInt month, tInt day, const tString& frenchFormat = "[$-fr-FR]dd/MM/yyyy");

// Thread-safe WorkbookBuilder class
class WorkbookBuilder {
public:
    WorkbookBuilder();
    ~WorkbookBuilder();
    
    // Thread-safe cell operations
    /// @brief Set cell value
    /// @param sCell The cell address
    /// @param sValue The value to set
    /// @return True if successful, false otherwise
    tBool SetCellValue(const tString& sCell, const tString& sValue);
    /// @brief Set cell formula
    /// @param sCell The cell address
    /// @param sFormula The formula to set
    /// @return True if successful, false otherwise
    tBool SetCellFormula(const tString& sCell, const tString& sFormula);
    /// @brief Set cell font color
    /// @param sCell The cell address
    /// @param sHexColor The hex color to set
    /// @return True if successful, false otherwise
    tBool SetCellFontColor(const tString& sCell, const tString& sHexColor);
    /// @brief Set cell font name
    /// @param sCell The cell address
    /// @param sFontName The font name to set
    /// @return True if successful, false otherwise
    tBool SetCellFontName(const tString& sCell, const tString& sFontName);
    /// @brief Set cell font size
    /// @param sCell The cell address
    /// @param sSize The font size to set
    /// @return True if successful, false otherwise
    tBool SetCellFontSize(const tString& sCell, tInt sSize);
    /// @brief Set cell font bold
    /// @param sCell The cell address
    /// @param sBold True if bold, false otherwise
    /// @return True if successful, false otherwise
    tBool SetCellFontBold(const tString& sCell, tBool sBold);
    /// @brief Set cell font italic
    /// @param sCell The cell address
    /// @param sItalic True if italic, false otherwise
    /// @return True if successful, false otherwise
    tBool SetCellFontItalic(const tString& sCell, tBool sItalic);
    /// @brief Set cell font underline
    /// @param sCell The cell address
    /// @param sUnderline True if underline, false otherwise
    /// @return True if successful, false otherwise
    tBool SetCellFontUnderline(const tString& sCell, tBool sUnderline);
    /// @brief Set cell merge
    /// @param sTopLeft The top left cell address
    /// @param sBottomRight The bottom right cell address
    /// @return True if successful, false otherwise
    tBool SetCellMerge(const tString& sTopLeft, const tString& sBottomRight);
    /// @brief Set row height
    /// @param sRow The row index
    /// @param sHeight The height to set
    /// @return True if successful, false otherwise
    tBool SetRowHeight(tInt sRow, tDouble sHeight);
    /// @brief Set column width
    /// @param sColumn The column index
    /// @param sWidth The width to set
    /// @return True if successful, false otherwise
    tBool SetColumnWidth(tInt sColumn, tDouble sWidth);
    /// @brief Set column width
    /// @param sColumn The column address
    /// @param sWidth The width to set
    /// @return True if successful, false otherwise
    tBool SetColumnWidth(const tString& sColumn, tDouble sWidth);
    /// @brief Mark a column hidden (OOXML hidden="1"). Width is still written so Excel can unhide.
    tBool SetColumnHidden(tInt sColumn);
    /// @brief Set sheet default row height
    /// @param sHeightPt The height to set
    /// @return True if successful, false otherwise
    tBool SetSheetDefaultRowHeight(tDouble sHeightPt);
    /// @brief Set sheet default column width
    /// @param sWidthChars The width to set
    /// @return True if successful, false otherwise
    tBool SetSheetDefaultColWidth(tDouble sWidthChars);
    /// @brief Set sheet show grid lines
    /// @param sShow True if show grid lines, false otherwise
    /// @return True if successful, false otherwise
    tBool SetSheetShowGridLines(tBool sShow);
    /// @brief Set sheet zoom scale normal
    /// @param sZoom The zoom scale to set
    /// @return True if successful, false otherwise
    tBool SetSheetZoomScaleNormal(tInt sZoom);
    /// @brief Set workbook default font
    /// @param sFontName The font name to set
    /// @param sSizePt The font size to set
    /// @return True if successful, false otherwise
    tBool SetWorkbookDefaultFont(const tString& sFontName, tInt sSizePt);
    /// @brief Get default font name
    /// @return The default font name
    tString DefaultFontName() const;
    /// @brief Get default font size
    /// @return The default font size
    tDouble DefaultFontSize() const;
    // Text alignment APIs
    /// @brief Set cell alignment
    /// @param sCell The cell address
    /// @param sHorizontal The horizontal alignment
    /// @param sVertical The vertical alignment
    /// @param sWrap True if wrap text, false otherwise
    /// @return True if successful, false otherwise
    tBool SetCellAlignment(const tString& sCell, const tString& sHorizontal, const tString& sVertical, tBool sWrap = false);
    /// @brief Set cell horizontal alignment
    /// @param sCell The cell address
    tBool SetCellHorizontalAlign(const tString& sCell, const tString& sHorizontal);
    /// @brief Set cell vertical alignment
    /// @param sCell The cell address
    /// @param sVertical The vertical alignment
    /// @return True if successful, false otherwise
    tBool SetCellVerticalAlign(const tString& sCell, const tString& sVertical);
    /// @brief Set cell wrap
    /// @param sCell The cell address
    /// @param sWrap True if wrap text, false otherwise
    /// @return True if successful, false otherwise
    tBool SetCellWrap(const tString& sCell, tBool sWrap);
    /// @brief Set cell text rotation
    /// @param sCell The cell address
    /// @param sAngleDegrees The angle in degrees
    /// @return True if successful, false otherwise
    tBool SetCellTextRotation(const tString& sCell, tInt sAngleDegrees);
    /// @brief Set cell fill color
    /// @param sCell The cell address
    /// @param sHexColor The hex color to set
    /// @return True if successful, false otherwise
    tBool SetCellFillColor(const tString& sCell, const tString& sHexColor);
    /// @brief Set cell border color
    /// @param sCell The cell address
    /// @param sHexColor The hex color to set
    /// @return True if successful, false otherwise
    tBool SetCellBorderColor(const tString& sCell, const tString& sHexColor);
    /// @brief Set cell border sides
    /// @param sCell The cell address
    /// @param leftStyle The left border style
    /// @param sLeftColor The left border color
    /// @param rightStyle The right border style
    /// @param sRightColor The right border color
    /// @param topStyle The top border style
    /// @param sTopColor The top border color
    tBool SetCellBorderSides(const tString& sCell,
        const tString& leftStyle, const tString& sLeftColor,
        const tString& rightStyle, const tString& sRightColor,
        const tString& topStyle, const tString& sTopColor,
        const tString& bottomStyle, const tString& sBottomColor);
    /// @brief Set cell number format
    /// @param sCell The cell address
    /// @param sFormatCode The format code to set
    /// @return True if successful, false otherwise
    tBool SetCellNumberFormat(const tString& sCell, const tString& sFormatCode);
    /// @brief Set cell currency
    /// @param sCell The cell address
    /// @param sSymbol The currency symbol to set
    /// @return True if successful, false otherwise
    tBool SetCellCurrency(const tString& sCell, const tString& sSymbol);
    /// @brief Set cell percentage
    /// @param sCell The cell address
    /// @param sDecimalPlaces The number of decimal places to set
    /// @return True if successful, false otherwise
    tBool SetCellPercentage(const tString& sCell, tInt sDecimalPlaces);
    /// @brief Set cell date
    /// @param sCell The cell address
    /// @param sDateFormat The date format to set
    /// @return True if successful, false otherwise
    tBool SetCellDate(const tString& sCell, const tString& sDateFormat);
    /// @brief Set cell date serial
    /// @param sCell The cell address
    /// @param sExcelSerial The Excel serial number to set
    /// @param sFrenchFormat The French format to set
    /// @return True if successful, false otherwise
    tBool SetCellDateSerial(const tString& sCell, tDouble sExcelSerial, const tString& sFrenchFormat);
    /// @brief Set cell date YMD
    /// @param sCell The cell address
    /// @param sYear The year to set
    /// @param sMonth The month to set
    /// @param sDay The day to set
    /// @param sFrenchFormat The French format to set
    /// @return True if successful, false otherwise
    tBool SetCellDateYMD(const tString& sCell, tInt sYear, tInt sMonth, tInt sDay, const tString& sFrenchFormat);
    
    // Multi-sheet management
    /// @brief Add a sheet
    /// @param sSheetName The name of the sheet to add
    /// @return True if successful, false otherwise
    tBool AddSheet(const tString& sSheetName);
    /// @brief Remove a sheet
    /// @param sSheetName The name of the sheet to remove
    /// @return True if successful, false otherwise
    tBool RemoveSheet(const tString& sSheetName);
    /// @brief Set the current sheet
    /// @param sSheetName The name of the sheet to set as current
    /// @return True if successful, false otherwise
    tBool SetCurrentSheet(const tString& sSheetName);
    /// @brief Get the current sheet
    /// @return The name of the current sheet
    tString GetCurrentSheet() const;
    /// @brief Get the names of all sheets
    /// @return A vector of sheet names
    std::vector<tString> GetSheetNames() const;
    /// @brief Get the number of sheets
    /// @return The number of sheets
    tSize GetSheetCount() const;
    
    // Export functionality
    /// @brief Export the workbook to an .xlsx file
    /// @param sOutputPath The path to the output file
    /// @return True if successful, false otherwise
    tBool ExportToXlsx(const tString& sOutputPath);
    
    // Utility methods
    /// @brief Clear the workbook
    /// @return True if successful, false otherwise
    tBool Clear();
    /// @brief Get the number of cells
    /// @return The number of cells
    tSize GetCellCount() const;
    /// @brief Get the number of cells in a sheet
    /// @param sSheetName The name of the sheet
    /// @return The number of cells in the sheet
    tSize GetCellCount(const tString& sSheetName) const;
    
    // Named ranges (workbook-scoped)
    /// @brief Add a named cell
    /// @param sName The name of the cell
    /// @param sCell The cell address
    /// @param sSheetName The name of the sheet
    /// @return True if successful, false otherwise
    tBool AddNamedCell(const tString& sName, const tString& sCell, const tString& sSheetName = "");
    /// @brief Add a named range
    /// @param sName The name of the range
    /// @param sTopLeft The top left cell address
    /// @param sBottomRight The bottom right cell address
    /// @param sSheetName The name of the sheet
    /// @return True if successful, false otherwise
    tBool AddNamedRange(const tString& sName, const tString& sTopLeft, const tString& sBottomRight, const tString& sSheetName = "");
    /// @brief Raw workbook.xml definedName body (sheet ref, formula, or Excel internal name text).
    /// @param sName The name of the defined name
    /// @param sDefinition The definition of the defined name
    /// @param sLocalSheetId The local sheet ID
    /// @return True if successful, false otherwise
    tBool AddDefinedName(const tString& sName, const tString& sDefinition, tInt sLocalSheetId = -1);
    
    // Conditional formatting
    /// @brief Add a conditional format
    /// @param sRange The range of the conditional format
    /// @param sFormula The formula of the conditional format
    /// @param sFontColor The font color of the conditional format
    /// @param sFillColor The fill color of the conditional format
    /// @param sPriority The priority of the conditional format
    /// @return True if successful, false otherwise
    tBool AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor = "", const tString& sFillColor = "", tInt sPriority = 1);
    /// @brief Add a conditional format with explicit type and operator (e.g. cellIs / greaterThan).
    /// @param sRange The range of the conditional format
    /// @param sFormula The formula or threshold value of the conditional format
    /// @param sFontColor The font color of the conditional format
    /// @param sFillColor The fill color of the conditional format
    /// @param sPriority The priority of the conditional format
    /// @param sType The type of the conditional format
    /// @param sOperator The operator of the conditional format
    /// @return True if successful, false otherwise
    tBool AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor, const tString& sFillColor, tInt sPriority, const tString& sType, const tString& sOperator);
    /// @brief Add striped rows conditional formatting
    /// @param sRange The range of the conditional format
    /// @param sFillColor The fill color of the conditional format
    /// @return True if successful, false otherwise
    tBool AddConditionalFormatStripedRows(const tString& sRange, const tString& sFillColor = "#F0F0F0");
    /// @brief Add a conditional format icon set
    /// @param sRange The range of the conditional format
    /// @param sIconSet The icon set of the conditional format
    /// @param sIconStyle The icon style of the conditional format
    /// @param sShowValue True if show value, false otherwise
    /// @param sReverse True if reverse, false otherwise
    /// @param sPriority The priority of the conditional format
    /// @param sThresholds The thresholds of the conditional format
    /// @return True if successful, false otherwise
    tBool AddConditionalFormatIconSet(const tString& sRange, const tString& sIconSet, const tString& sIconStyle = "percent", tBool sShowValue = true, tBool sReverse = false, tInt sPriority = 1, const std::vector<tString>& sThresholds = {}, const std::vector<tString>& sThresholdTypes = {});
    tBool AddConditionalFormatDataBar(const tString& sRange, const tString& sColor, const tString& sMinVal, const tString& sMaxVal, const tString& sStyle = "gradient", tInt sPriority = 1);
    tBool AddConditionalFormatColorScale(const tString& sRange, const tString& sMinColor, const tString& sMidColor, const tString& sMaxColor, const tString& sMinVal, const tString& sMidVal, const tString& sMaxVal, tInt sPriority = 1);
    /// @brief Register an Excel ListObject (xl/tables) for OOXML export.
    /// @param sSheetName The name of the sheet
    /// @param sName The name of the table
    /// @param sRef The reference of the table
    /// @param sColumns The columns of the table
    /// @param sStyleName The style name of the table
    /// @param sShowRowStripes True if show row stripes, false otherwise
    /// @param sShowColumnStripes True if show column stripes, false otherwise
    /// @param sShowFirstColumn True if show first column, false otherwise
    /// @param sShowLastColumn True if show last column, false otherwise
    /// @param sHasHeaderRow True if has header row, false otherwise
    /// @param sStyleElementCss The style element CSS
    /// @param sHasAutoFilter True if has auto filter, false otherwise
    /// @param sFilterButtonHidden The filter button hidden
    /// @return True if successful, false otherwise
    tBool AddStructuredTable(const tString& sSheetName,
                             const tString& sName,
                             const tString& sRef,
                             const std::vector<StructuredTableColumnSpec>& sColumns,
                             const tString& sStyleName = "",
                             tBool sShowRowStripes = true,
                             tBool sShowColumnStripes = false,
                             tBool sShowFirstColumn = false,
                             tBool sShowLastColumn = false,
                             tBool sHasHeaderRow = true,
                             const std::map<tString, tString>* sStyleElementCss = nullptr,
                             tBool sHasAutoFilter = false,
                             tInt sTotalsRowCount = 0,
                             const tString* sDisplayName = nullptr);

    /// @brief Mark a worksheet formula cell as a table calculated-column formula (ca="1").
    /// @param sCell The cell address
    /// @return True if successful, false otherwise
    tBool MarkCellFormulaAsTableColumn(const tString& sCell);

    /// @brief Register an Excel x14 sparkline on the current sheet (target cell + source data range).
    /// @param sCell The target cell (A1)
    /// @param sSourceRange Excel xm:f source (e.g. Sheet1!A1:A5)
    /// @param sMarkers Whether markers are shown
    /// @return True if successful, false otherwise
    tBool AddSparkline(const tString& sCell, const tString& sSourceRange, tBool sMarkers = true);
    
private:
    class Impl;
    std::unique_ptr<Impl> m_Impl;

    // Helper: normalize A1 from a cell address. Caller must hold the mutex.
    /// @brief Get the A1 address of a cell
    /// @param sCell The cell address
    /// @param wA1 The A1 address
    /// @return True if successful, false otherwise
    tBool GetA1(const tString& sCell, tString& wA1) const;
};

// Factory function for creating WorkbookBuilder instances
/// @brief Create a WorkbookBuilder instance
/// @return A unique pointer to the WorkbookBuilder instance
std::unique_ptr<WorkbookBuilder> CreateWorkbook();

// Named ranges API (workbook-level helpers)
// Adds a workbook-scoped named cell (on current sheet if sheetName empty)
/// @brief Add a named cell
/// @param sName The name of the cell
/// @param sCell The cell address
/// @param sSheetName The name of the sheet
/// @return True if successful, false otherwise
tBool AddNamedCell(const tString& sName, const tString& sCell, const tString& sSheetName = "");
// Adds a workbook-scoped named range (top-left to bottom-right) on given sheet (current if empty)
/// @brief Add a named range
/// @param sName The name of the range
/// @param sTopLeft The top left cell address
/// @param sBottomRight The bottom right cell address
/// @param sSheetName The name of the sheet
/// @return True if successful, false otherwise
tBool AddNamedRange(const tString& sName, const tString& sTopLeft, const tString& sBottomRight, const tString& sSheetName = "");

// Conditional formatting API (workbook-level helpers)
// Adds a conditional format rule with formula
/// @brief Add a conditional format
/// @param sRange The range of the conditional format
/// @param sFormula The formula of the conditional format
/// @param sFontColor The font color of the conditional format
/// @param sFillColor The fill color of the conditional format
/// @param sPriority The priority of the conditional format
/// @return True if successful, false otherwise
tBool AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor = "", const tString& sFillColor = "", tInt sPriority = 1);
// Adds striped rows conditional formatting
/// @brief Add striped rows conditional formatting
/// @param sRange The range of the conditional format
/// @param sFillColor The fill color of the conditional format
/// @return True if successful, false otherwise
tBool AddConditionalFormatStripedRows(const tString& sRange, const tString& sFillColor = "#F0F0F0");
/// @brief Add a conditional format icon set
/// @param sRange The range of the conditional format
/// @param sIconSet The icon set of the conditional format
/// @param sIconStyle The icon style of the conditional format

} // namespace SkExcel
