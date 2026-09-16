//=============================================================================
// SkExcelPugiXMLReader.hpp
//=============================================================================
#ifndef SKEXCELPUGIXMLREADER_HPP
#define SKEXCELPUGIXMLREADER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <functional>

#include <pugixml.hpp>
#include <zip.h>
#include <SkTypes.hpp>

using namespace std;
using namespace SkRoot;



// Structure to hold unzipped file content (memory, or disk for large worksheet XML).
struct UnzippedFile {
    tString filename;
    std::vector<char> content;
    tSize size = 0;
    tString diskPath;
    tBool HasPayload() const { return !diskPath.empty() || !content.empty(); }
    tBool ShouldStreamRows() const { return !diskPath.empty(); }
};

//! One Excel x14:sparkline (target cell + source data range).
struct tSparklineEntry {
    tString SheetName;
    tString CellRef;
    tString SourceRange;
    tBool Markers;
};

//! One Excel drawing text box (xdr:sp with xdr:txBody).
struct tExcelTextBoxEntry {
    tString SheetName;
    tString Name;
    tString Text;
    tString Position;
    tString AnchorType;
    tInt WidthPx = 0;
    tInt HeightPx = 0;
    tInt AnchorRow = 0;
    tInt AnchorCol = 0;
    tDouble DiffX = 0.0;
    tDouble DiffY = 0.0;
    tString Color;
    tString FontFamily;
    tDouble FontSize = 12.0;
    tString TextAlign;
    tBool Bold = false;
};

//! One data series inside an Excel chart (OOXML c:ser).
struct tExcelChartSeriesEntry {
    tString NameRef;
    tString CategoryRef;
    tString ValueRef;
};

//! One Excel drawing chart (graphicFrame + xl/charts/chartN.xml).
struct tExcelChartEntry {
    tString SheetName;
    tString Name;
    tString Title;
    tString SkerChartType;   // line | bar | area | pie
    tString BarDirection;    // vertical | horizontal (Excel barDir col | bar)
    tString Position;
    tString AnchorType;
    tInt WidthPx = 0;
    tInt HeightPx = 0;
    tInt AnchorRow = 0;
    tInt AnchorCol = 0;
    tDouble DiffX = 0.0;
    tDouble DiffY = 0.0;
    tInt ToAnchorRow = 0;
    tInt ToAnchorCol = 0;
    tDouble ToDiffX = 0.0;
    tDouble ToDiffY = 0.0;
    std::vector<tExcelChartSeriesEntry> Series;
};

//! One embedded worksheet image (xl/media + drawing anchor metadata).
struct tExcelImageEntry {
    tString SheetName;
    tString Name;
    tString MediaPath;
    tString ImageType;
    tString MimeType;
    tString Position;
    tString AnchorType;
    tString PositionDetails;
    tInt WidthPx = 0;
    tInt HeightPx = 0;
    tInt AnchorRow = 0;
    tInt AnchorCol = 0;
    tDouble DiffX = 0.0;
    tDouble DiffY = 0.0;
    tSize FileSize = 0;
    tString Base64;
    tString DataUrl;
};

class tExcelPugiXMLReader {
private:
	std::map<tString, UnzippedFile> m_UnzippedFiles;
	std::vector<tString> m_SharedStrings;
	// When a shared string uses OOXML rich text (<r><rPr>), uniform bold/italic/etc.
	// across all runs becomes extra CSS appended after cellXfs (mixed runs → "").
	std::vector<tString> m_SharedStringRichCss;
	std::vector<tString> m_WorksheetNames;
	tInt m_WorksheetCount;
	// Styles (from xl/styles.xml)
	std::vector<tInt> m_CellXfsNumFmtId; // index by style 's' -> numFmtId
	std::map<tInt, tString> m_CustomNumFmt; // numFmtId -> formatCode
	std::map<tInt, tString> m_BuiltinNumFmt; // Excel built-in formats

	// Style parts: fonts, fills, borders
	struct ExcelColor { tString rgb; tString theme; tString indexed; tString tint; };
	struct ExcelFont {
		tString name; tDouble size = 0.0; tBool bold=false; tBool italic=false; tBool underline=false; tBool strike=false; ExcelColor color;
	};
	struct ExcelFill { tString patternType; ExcelColor fgColor; ExcelColor bgColor; };
	struct ExcelBorderPr { tString style; ExcelColor color; };
	struct ExcelBorder { ExcelBorderPr left, right, top, bottom, diagonal; };
	std::vector<ExcelFont> m_Fonts;
	std::vector<ExcelFill> m_Fills;
	std::vector<ExcelBorder> m_Borders;
	// Differential formats (dxfs) for conditional formatting.
	// Excel keeps each dxf entry self-contained (fill + font + ...). We track
	// fill and font color in parallel vectors indexed by dxfId so the
	// resolver helpers stay cheap.
	std::vector<ExcelFill> m_DxFills;
	std::vector<ExcelColor> m_DxFontColors;
	// Full dxf font/border records, also keyed by dxfId. Used by the table
	// style overlay pipeline (header row, totals row, stripes...) where we
	// need bold/italic/underline + 4-edge borders, not only the fill color.
	std::vector<ExcelFont>  m_DxFonts;
	std::vector<ExcelBorder> m_DxBorders;
	// Table styles (xl/styles.xml -> <tableStyles>). Each entry maps an
	// element type (wholeTable, headerRow, totalsRow, firstRowStripe,
	// secondRowStripe, ...) to the dxfId that carries its formatting.
	struct TableStyleDef {
		std::map<tString, tInt> elementToDxfId;
	};
	std::map<tString, TableStyleDef> m_TableStyles;
	std::vector<tInt> m_CellXfsFontId;
	std::vector<tInt> m_CellXfsFillId;
	std::vector<tInt> m_CellXfsBorderId;
	std::vector<tString> m_CellXfsAlignH;
	std::vector<tString> m_CellXfsAlignV;
	std::vector<tBool> m_CellXfsWrapText;
	std::vector<tBool> m_CellXfsShrinkToFit;
	std::vector<tInt> m_CellXfsIndent;
	std::vector<tInt> m_CellXfsTextRotation;
	std::vector<tInt> m_CellXfsReadingOrder;

public:
	tExcelPugiXMLReader();
	~tExcelPugiXMLReader();

	tBool LoadExcelFile(const tString& sFilePath);
	void ParseWorkbook();
	void ParseStyles();
	void ParseTheme();
	void ParseSharedStrings();
	void ParseWorksheet(tInt sSheetNumber);
	void DisplayStructure();

	void ExtractWorksheetData(tInt sSheetNumber);
	void DisplayWorksheetNames();
	void ListWorksheetCells(tInt sSheetNumber);
	// american english: Display all sparklines (target cell and source range) for all worksheets
	void DisplaySparklines();
	// american english: Collect sparklines for SkSpreadSheet import (cell ref + source range).
	std::vector<tSparklineEntry> CollectSparklines() const;
	// american english: Display visual conditional formats (dataBars, colorScales, iconSets)
	void DisplayConditionalFormattingVisuals();
	// american english: Display cell comments/notes (legacy and threaded) across all worksheets
	void DisplayComments();
	// american english: Display charts (type and series formulas) across all worksheets
	void DisplayCharts();
	// Display conditional formatting rules and their formulas for all worksheets
	void DisplayConditionalFormattingFormulas();
	// american english: Display all Excel tables (name, range, columns) across all worksheets
	void DisplayTables();
	// american english: Display all images (name, size, position) across all worksheets
	void DisplayImages();
	// Collect embedded images with Base64 payload for JSON export.
	std::vector<tExcelImageEntry> CollectImages() const;
	// Collect drawing text boxes (xdr:sp + txBody) for SkSpreadSheet import.
	std::vector<tExcelTextBoxEntry> CollectTextBoxes() const;
	//! Collect embedded charts (graphicFrame) for SkCellClassLineChart / PieChart import.
	std::vector<tExcelChartEntry> CollectCharts() const;
	// Write {"images":[...]} JSON (dataUrl + base64 fields) to sOutputPath.
	// Returns image count, or -1 on failure.
	tInt SaveImagesJsonToFile(const tString& sOutputPath) const;
    
	tInt GetWorksheetCount() const { return m_WorksheetCount; }

	// Accessors for integration
	const std::vector<tString>& GetWorksheetNames() const { return m_WorksheetNames; }
	const std::vector<tString>& GetSharedStrings() const { return m_SharedStrings; }
	/// @brief Extra CSS from rich-text runs when formatting is uniform (may be empty).
	const tString& GetSharedStringRichCss(tSize idx) const;
	const UnzippedFile* GetUnzippedFile(const tString& path) const {
		auto it = m_UnzippedFiles.find(path);
		return (it==m_UnzippedFiles.end()) ? nullptr : &it->second;
	}
	/// Load worksheet XML without <sheetData> (print, cols, merges, CF). Safe for 100MB+ sheets.
	tBool LoadWorksheetShell(const tString& sSheetFile, pugi::xml_document& sOut) const;
	/// Stream each <row> in sheetData (parses one row at a time). Safe for 100MB+ sheets.
	tBool ForEachWorksheetRow(const tString& sSheetFile, const std::function<void(const pugi::xml_node&)>& sFn) const;
	/// @brief Get docProps/app.xml (tries docProps/app.xml and DocProps/app.xml, returns largest if both exist)
	const UnzippedFile* GetUnzippedFileAppXml() const;
	// Style accessors
	const std::vector<tInt>& GetCellXfsFontIds() const { return m_CellXfsFontId; }
	const std::vector<tInt>& GetCellXfsFillIds() const { return m_CellXfsFillId; }
	const std::vector<tInt>& GetCellXfsBorderIds() const { return m_CellXfsBorderId; }
	const std::vector<tString>& GetCellXfsAlignH() const { return m_CellXfsAlignH; }
	const std::vector<tString>& GetCellXfsAlignV() const { return m_CellXfsAlignV; }
	const std::vector<tBool>& GetCellXfsWrapText() const { return m_CellXfsWrapText; }
	const std::vector<tBool>& GetCellXfsShrinkToFit() const { return m_CellXfsShrinkToFit; }
	const std::vector<tInt>& GetCellXfsIndent() const { return m_CellXfsIndent; }
	const std::vector<tInt>& GetCellXfsTextRotation() const { return m_CellXfsTextRotation; }
	const std::vector<tInt>& GetCellXfsReadingOrder() const { return m_CellXfsReadingOrder; }
	const std::vector<ExcelFont>& GetFonts() const { return m_Fonts; }
	const std::vector<ExcelFill>& GetFills() const { return m_Fills; }
	const std::vector<ExcelBorder>& GetBorders() const { return m_Borders; }

	// High-level: build CSS from a style index (uses internal structures)
	tString BuildCssForStyle(tInt sStyleIdx) const;
	/// @brief OOXML <si> or <is>: CSS for bold/italic/underline/strike when all <r> runs agree (else "").
	tString BuildRichTextUniformCss(const pugi::xml_node& siOrIs) const;
	void DisplayStyles();
	// Write all unzipped XML/content entries to a target directory (created if needed)
	tBool SaveAllXmlToDirectory(const tString& sTargetDirectory) const;
	// Resolve background color from a dxfId (conditional formatting)
	tString ResolveDxfFillColor(tInt sDxfId) const;
	// Resolve visible fill color from an ExcelFill (cell or dxf). Honors
	// patternType="none" and indexed=64 (automatic/no fill).
	tString ResolvePatternFillBackground(const ExcelFill& sFill) const;
	// Returns the dxf font color (e.g. for conditional-format text color),
	// or empty if the dxf does not declare one.
	tString ResolveDxfFontColor(tInt sDxfId) const;
	// Build the CSS background-color declaration for a dxfId, or "" if the
	// dxf has no usable fill. Format: "background-color:#RRGGBB;".
	tString BuildDxfBackgroundCss(tInt sDxfId) const;
	// Build the CSS color/font-weight/font-style/text-decoration declarations
	// for a dxfId, or "" if the dxf does not declare any font property.
	tString BuildDxfFontCss(tInt sDxfId) const;
	// Build the CSS border-* declarations (left/top/right/bottom) for a
	// dxfId, or "" if the dxf has no border node. Skips edges that have
	// an empty style.
	tString BuildDxfBordersCss(tInt sDxfId) const;
	// Returns the dxfId that carries the formatting for the given element
	// type ("wholeTable", "headerRow", "totalsRow", "firstRowStripe",
	// "secondRowStripe", ...) of a custom table style declared in
	// <tableStyles>. Returns -1 when the style or element is unknown.
	tInt GetTableStyleElementDxfId(const tString& sStyleName, const tString& sElementType) const;
	// Builds the CSS overlay (background+font+border) for a built-in
	// Excel table style element. Used as a fallback when the workbook
	// references a built-in TableStyleMedium*/Light* by name and does not
	// redefine it through <tableStyles>. Returns "" when we have no
	// hardcoded mapping for the requested (style, element) pair.
	tString BuildBuiltinTableStyleElementCss(const tString& sStyleName, const tString& sElementType) const;
	// Resolve color from a <color> XML node (e.g. in dataBar/colorScale); returns hex without '#' or empty
	tString ResolveColorFromXmlNode(const pugi::xml_node& sColorNode) const;
	// american english: Returns the Excel formatCode string for a style index, or empty if none
	tString GetFormatCodeForStyle(tInt sStyleIdx) const;
	// american english: Returns the raw numFmtId for a style index, or -1 if the style is unknown.
	// Useful to recognize built-in date/time formats (e.g. ids 14..22, 27..36, 45..47, 50..58, 71..81)
	// that may not have an explicit formatCode in styles.xml.
	tInt GetNumFmtIdForStyle(tInt sStyleIdx) const;

private:
	// Maps an Excel/OOXML border style name to a (CSS style, width-in-px)
	// pair. Shared between BuildCssForStyle and BuildDxfBordersCss so both
	// emit identical declarations for the same input style. Excel does not
	// store widths separately: the style name implies the width
	// (thin=1, medium=2, thick=3) and the dash pattern.
	static void MapBorderStyle(const tString& sStyle, const char*& sCssStyle, tInt& sWidth);
	// Renders a single edge of an ExcelBorder as a CSS declaration. The
	// caller passes the CSS property name ("border-left", ...). Empty
	// styles produce no output.
	void EmitBorderEdgeCss(std::ostringstream& sCss, const char* sProperty, const ExcelBorderPr& sEdge) const;
	tBool ParseXMLFromMemory(const std::vector<char>& sXmlData, const tString& sContext);
	void ParsePugiXMLDirectly(const pugi::xml_node& sNode, tInt sDepth, const tString& sContext);
	std::map<tString, UnzippedFile> UnzipExcelInMemory(const tString& sFilePath);
	void CleanupUnzipSpill();
	tString m_UnzipTempDir;
	tString FormatNumericWithStyle(tDouble sValue, tInt sStyleIndex);
	void InitializeBuiltinFormats();
	tString ApplyFormatCode(tDouble sValue, const tString& sCode);
	static ExcelColor ReadColorNode(const pugi::xml_node& sNode);
	// Theme/Color resolution
	std::vector<tString> m_ThemeColors; // srgb without '#', uppercase 6 hex
	std::vector<tString> m_IndexedColors; // same format
	tString ResolveColor(const ExcelColor& sColor) const;
	static tString ApplyTintToSrgb(const tString& sSrgb, tDouble sTint);
};



#endif // SKEXCELPUGIXMLREADER_HPP
