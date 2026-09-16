//=============================================================================
// SkExcel2SpreadSheet.hpp
//=============================================================================
#ifndef SkExcel2SpreadSheet_hpp
#define SkExcel2SpreadSheet_hpp

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include <SkSpreadSheet.hpp>
#include <SkApi.hpp>
#include "SkExcelPugiXMLReader.hpp"
#include <SkExcelTools.hpp>
#include <SkMetrics.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

// Import diagnostics — leave commented for production (Node execFile maxBuffer ~10MB).
#define debugerror
#define debuginfo
// #define debugformula
// #define debugtablestyle

namespace SkExcel {

    // Structure to store Excel table metadata
    struct tTableMetadata {
        tString m_Name;              // Table name
        tString m_DisplayName;       // Display name
        tString mRef;               // Table range (e.g., "A1:C10")
        tString mSheetName;          // Sheet name containing the table
        std::vector<tString> columnNames;  // Column names in order
        // Table style information from <tableStyleInfo>. Empty m_StyleName
        // means the table inherits the workbook default and we should not
        // apply any overlay.
        tString m_StyleName;
        tBool   m_ShowRowStripes    = false;
        tBool   m_ShowColumnStripes = false;
        tBool   m_ShowFirstColumn   = false;
        tBool   m_ShowLastColumn    = false;
        tInt    m_HeaderRowCount    = 1;  // OOXML default
        tInt    m_TotalsRowCount    = 0;
        tBool   m_HasAutoFilter     = false;
    };
    
     struct tRangeNamed {
        tString m_SheetName;
        tString m_Ref;       // Ref
        tString m_Formula;               // Table range (e.g., "A1:C10")
    };
    
    typedef map<tString,tRangeNamed> tMapRangeNamed;
    
    // Claa Excel -> SkSpreadSheet
    class tExcel2SpreadSheet : public tClass {
    private:
        typedef tuple<tString,tString> tCellAddr;
        //! Fallback key when (sheet, StrRef) string form differs from OOXML attribute "r" (e.g. Base10ToAlpha drift).
        typedef std::tuple<tString, tInt, tInt> tArrayFormulaOriginKey;
        
        typedef std::map<tCellAddr, tString> tMapCell;
        tMapCell m_MapCss;
        tMapCell m_MapFormula;
        //! OOXML cached <v> for formula cells (applied after compile, skip RecalculateAll).
        std::map<tCellAddr, tVariant> m_MapFormulaCachedValue;
        //! OOXML CSE/dynamic array <f ref="..."> bounds per formula origin cell (sheet, A1 ref).
        std::map<tCellAddr, std::tuple<tIndex, tIndex, tIndex, tIndex>> m_MapArrayFormulaOutput;
        //! Same bounds keyed by (sheet, Excel 1-based row, Excel 1-based col) from RefToRowCol on origin cell.
        std::map<tArrayFormulaOriginKey, std::tuple<tIndex, tIndex, tIndex, tIndex>> m_MapArrayFormulaOutputByOrigin;
        
        tMapRangeNamed m_MapRangeNamed;

        // For named Range Formula
        tInt m_RowFormulaNamdedRange;
        
        // Store table metadata: table name -> metadata
        std::map<tString, tTableMetadata> m_TableMetadata;
        
        // Store cells that have ca="1" attribute (calculated column formulas already in XML)
        std::set<tCellAddr> m_CellsWithCalculatedColumn;
        
        // Store named ranges that point to single cells (for later formula check)
        std::map<tString, std::pair<tString, tString>> m_NamedRangesToCheck; // name -> (sheet, ref)

        //! Merged ranges collected during ProcessSheet. Each tuple is
        //! (sheetName, top, left, bottom, right) with 1-based indices.
        //! Used by AdjacencyConvertMergedBorders to rewrite Excel's
        //! per-anchor border-right / border-bottom as adjacent cells'
        //! border-left / border-top (our canonical adjacency model).
        std::vector<std::tuple<tString, tIndex, tIndex, tIndex, tIndex>> m_MergedRanges;

        //! When true, rewrite INDIRECT("LC(-1)",0) to native R1C1 (e.g. R[0]C[-1]) during ApplyFormulas.
        tBool m_MaterializeIndirectFrenchL1C1CellRefs = false;

        //! When true, run RecalculateAll at import instead of OOXML cached <v> (slow / risky on large workbooks).
        tBool m_RecalculateAtImport = false;

    public:
        void DrawCell(tApi& sApi,tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
        
        // Private helper methods for ImportXlsxToApi
        /// @brief Load and parse Excel file
        /// @param[in] sXlsxPath Path to Excel file
        /// @param[out] sReader Excel reader instance
        /// @return true if successful
        tBool LoadAndParseExcelFile(const tString& sXlsxPath, tExcelPugiXMLReader& sReader);
        
        /// @brief Create sheets in the API from Excel reader
        /// @param[in] sReader Excel reader instance
        /// @param[in,out] sApi API instance
        void CreateSheetsInApi(const tExcelPugiXMLReader& sReader, tApi& sApi);
        
        /// @brief Process defined names (named ranges) from workbook.xml
        /// @param[in] sReader Excel reader instance
        /// @param[in,out] sApi API instance
        void GetDefinedNames(const tExcelPugiXMLReader& sReader, tApi& sApi);
        
        /// @brief Process structured tables from Excel
        /// @param[in] sReader Excel reader instance
        /// @param[in,out] sApi API instance
        void ProcessStructuredTables(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Promote worksheet autoFilter / _FilterDatabase to RangeData at import.
        /// @param[in] sReader Excel reader instance
        /// @param[in,out] sApi API instance
        void ProcessWorksheetAutoFilters(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Project the formatting carried by <tableStyles> /
        ///        tableStyleInfo onto the cells of every imported table.
        ///        For each table we resolve the named style (custom in
        ///        styles.xml, otherwise built-in palette) and inject the
        ///        appropriate background / font / border CSS into
        ///        m_MapCss. Must run AFTER ProcessStructuredTables (so
        ///        m_TableMetadata is populated) and BEFORE ProcessSheet
        ///        (so direct cellXfs CSS, written later by concatenation,
        ///        wins on conflicting properties — matching Excel's rule
        ///        "direct cell formatting beats table style").
        void ApplyTableStyleOverlays(const tExcelPugiXMLReader& sReader, tApi& sApi);
        
        /// @brief Process a single sheet (merged cells, column widths, row heights, conditional formatting, cells)
        /// @param[in] sReader Excel reader instance
        /// @param[in] sSheetIndex Sheet index (1-based)
        /// @param[in,out] sApi API instance
        void ProcessSheet(const tExcelPugiXMLReader& sReader, tInt sSheetIndex, tApi& sApi);

        /// @brief Map OOXML print markup (sheetPr/pageSetUpPr, pageSetup, pageMargins, printOptions) onto SkSpreadSheet::tPrintParameters.
        /// Orientation and fitToPage stay on the sheet; other fields migrate onto the workbook (first non-default wins).
        /// @param[in] sWorksheet Root `<worksheet>` node from sheetN.xml
        /// @param[in,out] sSheet Target sheet (typically active sheet for this workbook XML)
        void ImportSheetPrintSettings(const pugi::xml_node& sWorksheet, tSheet* sSheet);


        /// @brief Apply CSS styles to cells
        /// @param[in,out] sApi API instance
        void ApplyCssStyles(tApi& sApi);

        /// @brief Convert Excel per-anchor merged-cell borders to our adjacency model.
        ///        For each collected merged range, the anchor's border-right is moved
        ///        to border-left on every cell of the neighbor column (right+1),
        ///        and the anchor's border-bottom is moved to border-top on every cell
        ///        of the neighbor row (bottom+1). The projection skips neighbors that
        ///        already carry the corresponding border (caller's existing style wins).
        ///        Must run on m_MapCss *before* ApplyCssStyles so the rewritten CSS is
        ///        what finally reaches the spreadsheet engine.
        void AdjacencyConvertMergedBorders();

        /// @brief Convert per-cell border-right / border-bottom (single cells, not
        ///        merged anchors) into the adjacency model: border-right on (r,c)
        ///        becomes border-left on (r,c+1); border-bottom on (r,c) becomes
        ///        border-top on (r+1,c). The renderer emits a cell's own
        ///        border-top / border-left, so without this rewrite the right
        ///        and bottom edges read from a .xlsx file are silently lost.
        ///        Preserves any border-left / border-top already declared on the
        ///        target neighbor (caller's existing style wins).
        ///        Must run *after* AdjacencyConvertMergedBorders so merge-anchor
        ///        borders are projected by their dedicated rule first.
        void AdjacencyConvertCellBorders();

        /// @brief Apply named ranges to cells
        /// @param[in,out] sApi API instance
        void ApplyNamedRanges(tApi& sApi);
        
        /// @brief Apply formulas to cells
        /// @param[in,out] sApi API instance
        void ApplyFormulas(tApi& sApi);

        /// @brief Restore Excel cached values on formula cells after ApplyFormulas.
        void ApplyCachedFormulaValues(tApi& sApi);

        /// @brief Import Excel sparklines as SkCellClassSparkline on target cells.
        void ApplySparklines(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Import Excel embedded images as SkCellClassImage floating objects.
        void ApplyImages(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Import Excel drawing text boxes as SkCellClassTextBox floating objects.
        void ApplyTextBoxes(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Import Excel charts as SkCellClassLineChart / SkCellClassPieChart floating objects.
        void ApplyCharts(const tExcelPugiXMLReader& sReader, tApi& sApi);

        /// @brief Ensure border-only CSS blocks inherit the workbook default font.
        void EnsureDefaultFontOnCssMap(tApi& sApi);
        
        /// @brief Save output file
        /// @param[in] sXlsxPath Original Excel file path
        /// @param[in] sApi API instance
        /// @return true if successful
        tBool SaveOutputFile(const tString& sXlsxPath, tApi& sApi);
        
    public:
        /// @brief Constructor
        tExcel2SpreadSheet();
        /// @brief Destructor
        ~tExcel2SpreadSheet();
        /// @brief Create formula named range on specialize Sheet
        /// @param[in] sApi tApi&
        /// @param[in] sName tString
        /// @param[in] sFormula tString
        void CreateFormulaNamedRange(tApi& sApi, const tString& sName, const tString& sFormula);
        
        /// @brief Register JS cell classes emitted by SkExcel (required before ReadJson after ~tApi clears factory).
        static void RegisterJavascriptCellClasses(tApi& sApi);

        /// @brief Import an .xlsx file into a SkSpreadSheet::tApi instance (creates sheets, sets values/formulas)
        tBool ImportXlsxToApi(const tString& sXlsxPath, tApi& sApi);

        /// @brief Opt-in: materialize French L1C1 INDIRECT cell refs to native R1C1 (drops INDIRECT volatility).
        void SetMaterializeIndirectFrenchL1C1CellRefs(tBool sEnabled) {
            m_MaterializeIndirectFrenchL1C1CellRefs = sEnabled;
        }
        tBool MaterializeIndirectFrenchL1C1CellRefs() const {
            return m_MaterializeIndirectFrenchL1C1CellRefs;
        }

        /// @brief Opt-in: RecalculateAll after ApplyFormulas (default: OOXML cached values).
        void SetRecalculateAtImport(tBool sEnabled) { m_RecalculateAtImport = sEnabled; }
        tBool RecalculateAtImport() const { return m_RecalculateAtImport; }
        /// @brief Get table metadata by table name
        const tTableMetadata* GetTableMetadata(const tString& sTableName) const;
        /// @brief Get all table names
        std::vector<tString> GetTableNames() const;
    };

}

#endif


