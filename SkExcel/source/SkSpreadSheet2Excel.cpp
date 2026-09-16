#include <SkSpreadSheet2Excel.hpp>

#include <SkExcel2SpreadSheet.hpp>
#include <SkExcelProgress.hpp>
#include <SkExcelTools.hpp>
#include <SkColRow.hpp>
#include <SkRangeNamed.hpp>
#include <SkRangeData.hpp>
#include <SkSheet.hpp>
#include <SkWorkBook.hpp>
#include <SkConditionalFormat.hpp>
#include <SkMetrics.hpp>
#include <SkTools.hpp>
#include <SkTypesClass.hpp>
#include <SkFormatString.hpp>
#include <SkApplication.hpp>
#include <SkCellClassAttribute.hpp>
#include <SkCellClassContainer.hpp>
#include <SkRangeRefTransform.hpp>
#include <pugixml.hpp>
#include <zip.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>
#include <set>
#include <cmath>
#include <cstdio>
#include <climits>
#include <mutex>
#include <iostream>

namespace SkExcel {

// Forward declarations for utility functions
enum class CellType { String, Number, Date, Formula };
tBool Parse_cell_address(const tString& sAddr, tInt& sOutRow, tInt& sOutCol, tString& outA1);
tBool normalize_hex_to_argb(const tString& sInput, tString& sOutARGB);
tString normalize_border_style(const tString& sStyle);
CellType detect_cell_type(const tString& sValue);
tBool is_date_value(const tString& sValue);
tBool is_percentage_value(const tString& sValue);
tBool is_currency_value(const tString& sValue);
tInt column_letters_to_index(const tString& sCol);
tDouble date_to_excel_serial(tInt sYear, tInt sMonth, tInt sDay);

// Match SkRangeData::NormalizeTableColumnLabel — single-line labels for OOXML and structured refs.
static tString NormalizeTableColumnLabelForExport(tString sLabel) {
    tString wOut;
    wOut.reserve(sLabel.size());
    tBool wPrevSpace = true;
    for (tChar wCh : sLabel) {
        if (wCh == '\r' || wCh == '\n' || wCh == '\t') {
            wCh = ' ';
        }
        if (wCh == ' ') {
            if (wPrevSpace) {
                continue;
            }
            wPrevSpace = true;
        } else {
            wPrevSpace = false;
        }
        wOut.push_back(wCh);
    }
    while (!wOut.empty() && wOut.back() == ' ') {
        wOut.pop_back();
    }
    return wOut;
}

// OOXML tableColumn@name: keep Excel _x000a_ escapes; map real line breaks to _x000a_.
static tString OoxmlTableColumnName(tString sLabel) {
    if (sLabel.find("_x000a_") != tString::npos) {
        return sLabel;
    }
    tString wOut;
    wOut.reserve(sLabel.size() + 16);
    for (tSize wI = 0; wI < sLabel.size(); ++wI) {
        const tChar wCh = sLabel[wI];
        if (wCh == '\r') {
            continue;
        }
        if (wCh == '\n' || wCh == '\t') {
            wOut += "_x000a_";
            continue;
        }
        wOut.push_back(wCh);
    }
    return wOut;
}

// Excel table@displayName when derived from table@name (spaces -> underscores).
// Stored tableDisplayName from import is exported verbatim (may contain accents, e.g. Impôts).
static tString ExcelTableDisplayName(tString sName) {
    tString wOut;
    wOut.reserve(sName.size());
    for (tChar wCh : sName) {
        if (std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_') {
            wOut.push_back(wCh);
        } else if (wCh == ' ' || wCh == '-') {
            if (!wOut.empty() && wOut.back() != '_') {
                wOut.push_back('_');
            }
        }
    }
    while (!wOut.empty() && wOut.back() == '_') {
        wOut.pop_back();
    }
    if (wOut.empty()) {
        return "Table";
    }
    if (!std::isalpha(static_cast<unsigned char>(wOut.front())) && wOut.front() != '_') {
        wOut.insert(wOut.begin(), '_');
    }
    return wOut;
}

// OOXML autoFilter@ref covers header + data only (excludes totals row when totalsRowCount > 0).
static tString StructuredTableAutoFilterRef(const tString& sTableRef, tInt sTotalsRowCount) {
    tIndex wTop = 0;
    tIndex wLeft = 0;
    tIndex wBottom = 0;
    tIndex wRight = 0;
    if (!ParseRange(sTableRef, wTop, wLeft, wBottom, wRight)) {
        return sTableRef;
    }
    if (sTotalsRowCount > 0 && wBottom - sTotalsRowCount >= wTop) {
        wBottom -= sTotalsRowCount;
    }
    return Base10ToAlpha(wLeft) + std::to_string(wTop) + ":"
        + Base10ToAlpha(wRight) + std::to_string(wBottom);
}

// OOXML table formula nodes must be single-line XML text (Excel may split refs across lines).
static tString SanitizeTableFormulaForOoxmlXml(tString sFormula) {
    if (sFormula.empty()) {
        return sFormula;
    }
    tString wOut;
    wOut.reserve(sFormula.size());
    tBool wPrevSpace = false;
    for (tChar wCh : sFormula) {
        if (wCh == '\r') {
            continue;
        }
        if (wCh == '\n' || wCh == '\t') {
            wCh = ' ';
        }
        if (wCh == ' ') {
            if (wPrevSpace) {
                continue;
            }
            wPrevSpace = true;
        } else {
            wPrevSpace = false;
        }
        wOut.push_back(wCh);
    }
    while (!wOut.empty() && wOut.back() == ' ') {
        wOut.pop_back();
    }
    return wOut;
}

// Write a whole string to a file (UTF-8 without BOM)
static tBool write_text_file(const std::filesystem::path& sFile_path, const tString& sContent) {
	std::error_code wEc;
	std::filesystem::create_directories(sFile_path.parent_path(), wEc);
	std::ofstream wOfs(sFile_path, std::ios::binary);
	if (!wOfs) return false;
	wOfs.write(sContent.data(), static_cast<std::streamsize>(sContent.size()));
	return static_cast<tBool>(wOfs);
}

// Serialize a pugixml document to string
static tString to_string_xml(const pugi::xml_document& sDoc) {
	tStringStream wStream;
	// Compact XML (no indent): Excel is picky about x14 extension parts.
	sDoc.save(wStream, "", pugi::format_raw | pugi::format_no_declaration);
	return tString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n")
		+ wStream.str();
}

// Office theme required by Excel 2010+ features (sparklines use theme colors).
static const char* OfficeThemeXml() {
	return
		R"OOXMLTHEME(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Office Theme"><a:themeElements><a:clrScheme name="Office"><a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1><a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="1F497D"/></a:dk2><a:lt2><a:srgbClr val="EEECE1"/></a:lt2><a:accent1><a:srgbClr val="4F81BD"/></a:accent1><a:accent2><a:srgbClr val="C0504D"/></a:accent2><a:accent3><a:srgbClr val="9BBB59"/></a:accent3><a:accent4><a:srgbClr val="8064A2"/></a:accent4><a:accent5><a:srgbClr val="4BACC6"/></a:accent5><a:accent6><a:srgbClr val="F79646"/></a:accent6><a:hlink><a:srgbClr val="0000FF"/></a:hlink><a:folHlink><a:srgbClr val="800080"/></a:folHlink></a:clrScheme><a:fontScheme name="Office"><a:majorFont><a:latin typeface="Cambria"/><a:ea typeface=""/><a:cs typeface=""/><a:font script="Jpan" typeface="ＭＳ Ｐゴシック"/><a:font script="Hang" typeface="맑은 고딕"/><a:font script="Hans" typeface="宋体"/><a:font script="Hant" typeface="新細明體"/><a:font script="Arab" typeface="Times New Roman"/><a:font script="Hebr" typeface="Times New Roman"/><a:font script="Thai" typeface="Tahoma"/><a:font script="Ethi" typeface="Nyala"/><a:font script="Beng" typeface="Vrinda"/><a:font script="Gujr" typeface="Shruti"/><a:font script="Khmr" typeface="MoolBoran"/><a:font script="Knda" typeface="Tunga"/><a:font script="Guru" typeface="Raavi"/><a:font script="Cans" typeface="Euphemia"/><a:font script="Cher" typeface="Plantagenet Cherokee"/><a:font script="Yiii" typeface="Microsoft Yi Baiti"/><a:font script="Tibt" typeface="Microsoft Himalaya"/><a:font script="Thaa" typeface="MV Boli"/><a:font script="Deva" typeface="Mangal"/><a:font script="Telu" typeface="Gautami"/><a:font script="Taml" typeface="Latha"/><a:font script="Syrc" typeface="Estrangelo Edessa"/><a:font script="Orya" typeface="Kalinga"/><a:font script="Mlym" typeface="Kartika"/><a:font script="Laoo" typeface="DokChampa"/><a:font script="Sinh" typeface="Iskoola Pota"/><a:font script="Mong" typeface="Mongolian Baiti"/><a:font script="Viet" typeface="Times New Roman"/><a:font script="Uigh" typeface="Microsoft Uighur"/></a:majorFont><a:minorFont><a:latin typeface="Calibri"/><a:ea typeface=""/><a:cs typeface=""/><a:font script="Jpan" typeface="ＭＳ Ｐゴシック"/><a:font script="Hang" typeface="맑은 고딕"/><a:font script="Hans" typeface="宋体"/><a:font script="Hant" typeface="新細明體"/><a:font script="Arab" typeface="Arial"/><a:font script="Hebr" typeface="Arial"/><a:font script="Thai" typeface="Tahoma"/><a:font script="Ethi" typeface="Nyala"/><a:font script="Beng" typeface="Vrinda"/><a:font script="Gujr" typeface="Shruti"/><a:font script="Khmr" typeface="DaunPenh"/><a:font script="Knda" typeface="Tunga"/><a:font script="Guru" typeface="Raavi"/><a:font script="Cans" typeface="Euphemia"/><a:font script="Cher" typeface="Plantagenet Cherokee"/><a:font script="Yiii" typeface="Microsoft Yi Baiti"/><a:font script="Tibt" typeface="Microsoft Himalaya"/><a:font script="Thaa" typeface="MV Boli"/><a:font script="Deva" typeface="Mangal"/><a:font script="Telu" typeface="Gautami"/><a:font script="Taml" typeface="Latha"/><a:font script="Syrc" typeface="Estrangelo Edessa"/><a:font script="Orya" typeface="Kalinga"/><a:font script="Mlym" typeface="Kartika"/><a:font script="Laoo" typeface="DokChampa"/><a:font script="Sinh" typeface="Iskoola Pota"/><a:font script="Mong" typeface="Mongolian Baiti"/><a:font script="Viet" typeface="Arial"/><a:font script="Uigh" typeface="Microsoft Uighur"/></a:minorFont></a:fontScheme><a:fmtScheme name="Office"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:tint val="50000"/><a:satMod val="300000"/></a:schemeClr></a:gs><a:gs pos="35000"><a:schemeClr val="phClr"><a:tint val="37000"/><a:satMod val="300000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:tint val="15000"/><a:satMod val="350000"/></a:schemeClr></a:gs></a:gsLst><a:lin ang="16200000" scaled="1"/></a:gradFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:shade val="51000"/><a:satMod val="130000"/></a:schemeClr></a:gs><a:gs pos="80000"><a:schemeClr val="phClr"><a:shade val="93000"/><a:satMod val="130000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:shade val="94000"/><a:satMod val="135000"/></a:schemeClr></a:gs></a:gsLst><a:lin ang="16200000" scaled="0"/></a:gradFill></a:fillStyleLst><a:lnStyleLst><a:ln w="9525" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"><a:shade val="95000"/><a:satMod val="105000"/></a:schemeClr></a:solidFill><a:prstDash val="solid"/></a:ln><a:ln w="25400" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/></a:ln><a:ln w="38100" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst><a:outerShdw blurRad="40000" dist="20000" dir="5400000" rotWithShape="0"><a:srgbClr val="000000"><a:alpha val="38000"/></a:srgbClr></a:outerShdw></a:effectLst></a:effectStyle><a:effectStyle><a:effectLst><a:outerShdw blurRad="40000" dist="23000" dir="5400000" rotWithShape="0"><a:srgbClr val="000000"><a:alpha val="35000"/></a:srgbClr></a:outerShdw></a:effectLst></a:effectStyle><a:effectStyle><a:effectLst><a:outerShdw blurRad="40000" dist="23000" dir="5400000" rotWithShape="0"><a:srgbClr val="000000"><a:alpha val="35000"/></a:srgbClr></a:outerShdw></a:effectLst><a:scene3d><a:camera prst="orthographicFront"><a:rot lat="0" lon="0" rev="0"/></a:camera><a:lightRig rig="threePt" dir="t"><a:rot lat="0" lon="0" rev="1200000"/></a:lightRig></a:scene3d><a:sp3d><a:bevelT w="63500" h="25400"/></a:sp3d></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:tint val="40000"/><a:satMod val="350000"/></a:schemeClr></a:gs><a:gs pos="40000"><a:schemeClr val="phClr"><a:tint val="45000"/><a:shade val="99000"/><a:satMod val="350000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:shade val="20000"/><a:satMod val="255000"/></a:schemeClr></a:gs></a:gsLst><a:path path="circle"><a:fillToRect l="50000" t="-80000" r="50000" b="180000"/></a:path></a:gradFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:tint val="80000"/><a:satMod val="300000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:shade val="30000"/><a:satMod val="200000"/></a:schemeClr></a:gs></a:gsLst><a:path path="circle"><a:fillToRect l="50000" t="50000" r="50000" b="50000"/></a:path></a:gradFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>)OOXMLTHEME";
}

static tString FormatRowHeightXml(tDouble sPt) {
    // Excel stores row heights with at most ~2 decimal places (e.g. 12.5, 20.75).
    const tDouble wRounded = std::round(sPt * 100.0) / 100.0;
    const tInt wWhole = static_cast<tInt>(std::llround(wRounded));
    if (std::abs(wRounded - static_cast<tDouble>(wWhole)) < 0.001) {
        return std::to_string(wWhole);
    }
    return std::to_string(wRounded);
}

static constexpr tDouble kExcelBuiltinDefaultRowHeightPt = 15.0;

static tBool SheetUsesCustomDefaultRowHeight(tDouble sDefaultRowPt) {
    return sDefaultRowPt > 0.0
        && std::abs(sDefaultRowPt - kExcelBuiltinDefaultRowHeightPt) > 0.05;
}

static tBool IsBuiltinExcelTableStyleName(const tString& sStyleName) {
    return sStyleName.find("TableStyleLight") == 0
        || sStyleName.find("TableStyleMedium") == 0
        || sStyleName.find("TableStyleDark") == 0;
}

// Custom styles are not re-serialized in <tableStyles>; use a neutral built-in so Excel
// does not repair the ListObject. Cell xfs/CSS carry the real formatting.
static tBool UsesCustomTableStyleFallback(const tString& sStyleName) {
    return !sStyleName.empty() && !IsBuiltinExcelTableStyleName(sStyleName);
}

static tString ResolveExportTableStyleName(const tString& sStyleName) {
    if (!sStyleName.empty() && IsBuiltinExcelTableStyleName(sStyleName)) {
        return sStyleName;
    }
    return "TableStyleLight1";
}

static void NormalizeStructuredTableHeaderCellText(
    tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol, tString& ioText) {
    if (sWorkBook == nullptr || sSheet == nullptr || ioText.empty()) {
        return;
    }
    tString wTableName;
    tRange* wRange = nullptr;
    std::tie(wTableName, wRange) = sSheet->FindRangeDataCovered(sRow, sCol);
    if (wRange == nullptr || !wRange->IsData() || wRange->TopIndex() != sRow) {
        return;
    }
    tRangeData* wRangeData = sWorkBook->RangeData(wTableName);
    if (wRangeData == nullptr || !wRangeData->HasHeaders()) {
        return;
    }
    ioText = NormalizeTableColumnLabelForExport(std::move(ioText));
}

// Add a single file tInto an open zip under a given root folder
static tBool add_file_to_zip(zip_t* za, const std::filesystem::path& sRoot, const std::filesystem::path& sFile) {
	std::filesystem::path wPath = std::filesystem::relative(sFile, sRoot);
	tString wRealtivePath = wPath.generic_string();
	zip_source_t* wZipFile = zip_source_file(za, sFile.string().c_str(), 0, -1);
	if (wZipFile == nullptr) return false;
	zip_int64_t wZip64 = zip_file_add(za, wRealtivePath.c_str(), wZipFile, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
	if (wZip64 < 0) {
		zip_source_free(wZipFile);
		return false;
	}
	return true;
}

// Zip a whole directory tree
static tBool zip_directory(const std::filesystem::path& sDirectory, const std::filesystem::path& sOutputZip) {
	tInt err = 0;
	zip_t* wZip = zip_open(sOutputZip.string().c_str(), ZIP_TRUNCATE | ZIP_CREATE, &err);
	if (!wZip) return false;
	tBool wOk = true;
	for (auto const& entry : std::filesystem::recursive_directory_iterator(sDirectory)) {
		if (entry.is_regular_file()) {
			if (!add_file_to_zip(wZip, sDirectory, entry.path())) { wOk = false; break; }
		}
	}
	if (zip_close(wZip) != 0) wOk = false;
	return wOk;
}

// NOTE: Avoid reopening namespace multiple times accidentally.
// Keep a single SkExcel namespace spanning the entire implementation.

// WorkbookBuilder implementation details
class WorkbookBuilder::Impl {
public:
    // Border specifications
struct BorderSideSpec { tString style; tString argb; };
struct BorderSpec { BorderSideSpec left,right,top,bottom; };
    
    // Font properties structure
    struct FontSpec {
        tString name;        // Font family name (e.g., "Arial", "Calibri")
        tInt size = 11;      // Font size in points
        tBool bold = false;  // Bold style
        tBool italic = false; // Italic style
        tBool underline = false; // Underline style
        tString color;       // Font color in ARGB format
        
        // Comparison operator for std::set
        tBool operator<(const FontSpec& other) const {
            if (name != other.name) return name < other.name;
            if (size != other.size) return size < other.size;
            if (bold != other.bold) return bold < other.bold;
            if (italic != other.italic) return italic < other.italic;
            if (underline != other.underline) return underline < other.underline;
            return color < other.color;
        }
    };
    
    // Merged cell range structure
    struct MergedRange {
        tString topLeft;    // Top-left cell (e.g., "A1")
        tString bottomRight; // Bottom-right cell (e.g., "C3")
        
        // Constructor for easy creation
        MergedRange(const tString& tl, const tString& br) : topLeft(tl), bottomRight(br) {}
    };
    
    // Row and column dimensions
    struct RowDimensions {
        std::map<tInt, tDouble> m_RowHeights;  // row number -> height in points
    };
    
    struct ColumnDimensions {
        std::map<tInt, tDouble> m_ColumnWidths; // column number -> width in characters
        std::set<tInt> m_ColumnHidden;
    };
    
    // Conditional formatting structures
    struct tConditionalFormatRule {
        tString type;           // "expression", "cellIs", "colorScale", "dataBar", "iconSet"
        tString formula;        // Formula for expression type
        tString range;          // Cell range like "A1:C10"
        tString fontColor;      // Font color in hex format
        tString fillColor;      // Fill color in hex format
        tInt priority;          // Rule priority
        tBool stopIfTrue;       // Stop if true flag
        tString operator_;      // Operator for cellIs type: "greaterThan", "lessThan", "equal", etc.

        // Icon set specific fields
        tString iconSet;        // Icon set name: "3Arrows", "3TrafficLights1", ...
        tString iconStyle;      // Default cfvo type when per-threshold types are absent
        tBool showValue;        // Show cell value with icon
        tBool reverse;          // Reverse icon order
        std::vector<tString> iconThresholds; // Custom thresholds for icon sets
        std::vector<tString> iconThresholdTypes; // Per-cfvo OOXML types (percent, num, ...)

        // Data bar fields
        tString dataBarColor;
        tString dataBarMinVal;
        tString dataBarMaxVal;
        tString dataBarStyle;

        // Color scale fields (2 or 3 stops)
        std::vector<tString> colorScaleColors;
        std::vector<tString> colorScaleVals;
    };
    
    // Sheet data structure
    struct tSheetData {
        std::map<tString, tString> m_CellValues;
        std::map<tString, tString> m_CellFontColor;
        std::map<tString, FontSpec> m_CellFontSpec;  // Complete font specification
        std::map<tString, tString> m_CellFillColor;
        std::map<tString, tString> m_CellBorderColor;
        std::map<tString, tInt> m_CellNumberFormat;
        std::map<tString, BorderSpec> m_CellBorderSides;
        std::map<tString, tString> m_CellFormulas;
        // Alignments
        std::map<tString, tString> m_CellHorizAlign; // left, center, right, fill, justify, centerContinuous, distributed
        std::map<tString, tString> m_CellVertAlign;  // top, center, bottom, justify, distributed
        std::set<tString>         m_CellWrap;        // cells with wrapText=true
        std::map<tString, tInt>   m_CellTextRotation; // -90..90 or 255 for vertical
        std::vector<MergedRange> m_MergedRanges;  // List of merged cell ranges
        RowDimensions m_RowDimensions;           // Row heights
        ColumnDimensions m_ColumnDimensions;     // Column widths
        tDouble m_SheetDefaultRowHeightPt = 0.0;
        tDouble m_SheetDefaultColWidthChars = 0.0;
        tBool m_ShowGridLines = true;
        tInt m_ZoomScaleNormal = 100;
        std::vector<tConditionalFormatRule> m_ConditionalFormats; // Conditional formatting rules
        std::set<tString> m_TableColumnFormulas; // cells whose <f> needs ca="1"
        struct tSparklineExport {
            tString cellRef;
            tString sourceRange;
            tBool markers = true;
        };
        std::vector<tSparklineExport> m_Sparklines;
    };

    struct TableColumnExport {
        tString name;
        tString calculatedFormula; // without leading '='
        tBool filterButtonHidden = false;
        tString totalsRowLabel;
        tString totalsRowFunction;
        tString totalsRowFormula;
    };

    struct TableExport {
        tString sheetName;
        tString name;
        tString displayName;
        tString ref;
        tInt totalsRowCount = 0;
        tString styleName;
        tBool showRowStripes = true;
        tBool showColumnStripes = false;
        tBool showFirstColumn = false;
        tBool showLastColumn = false;
        tBool hasHeaderRow = true;
        tBool hasAutoFilter = false;
        std::map<tString, tString> styleElementCss;
        std::vector<TableColumnExport> columns;
    };
    
    // Multi-sheet management
    std::map<tString, tSheetData> m_MapSheets;
    std::vector<tString> m_SheetOrder;
    tString m_CurrentSheet;
    tString m_DefaultFontName = "Calibri";
    tInt m_DefaultFontSize = 11;
    std::vector<TableExport> m_Tables;
    // Named ranges (workbook scoped): name -> reference like Sheet1!$A$1 or Sheet1!$A$1:$C$3
    struct DefinedNameEntry {
        tString body;
        tInt localSheetId = -1;
    };
    std::map<tString, DefinedNameEntry> m_DefinedNames;
    
    // (backward compatibility maps removed)
    
    // Number format management
    struct tNumberFormat {
        tInt id;
        tString formatCode;
        tString description;
    };
    
    static const std::vector<tNumberFormat> m_BuiltinFormats;
    std::vector<tNumberFormat> m_CustomFormats;
    tInt m_NextCustomFormatId;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    Impl() : m_CurrentSheet("Sheet1"), m_NextCustomFormatId(164) {
        // Initialize default sheet (keep m_SheetOrder in sync with m_MapSheets — EnsureExportSheet uses GetSheetNames())
        m_MapSheets["Sheet1"] = tSheetData();
        m_SheetOrder.push_back("Sheet1");
    }
    
    // Internal helper methods
    tInt find_or_create_number_format(const tString& sFormatCode);
    tDouble date_to_excel_serial(tInt sYear, tInt sMonth, tInt sDay);
    tBool build_ooxml_minimal(const std::filesystem::path& sRoot);
};

// Helper method implementation
tBool SkExcel::WorkbookBuilder::GetA1(const tString& sCell, tString& wA1) const {
    tInt wRow=0, wCol=0;
    return Parse_cell_address(sCell, wRow, wCol, wA1);
}

// Built-in Excel number formats
const std::vector<SkExcel::WorkbookBuilder::Impl::tNumberFormat> SkExcel::WorkbookBuilder::Impl::m_BuiltinFormats = {
    {0, "General", "General"},
    {1, "0", "0"},
    {2, "0.00", "0.00"},
    {3, "#,##0", "#,##0"},
    {4, "#,##0.00", "#,##0.00"},
    {9, "0%", "0%"},
    {10, "0.00%", "0.00%"},
    {11, "0.00E+00", "0.00E+00"},
    {12, "# ?/?", "# ?/?"},
    {13, R"(# ??/??)", R"(# ??/??)"},
    {14, "mm-dd-yy", "mm-dd-yy"},
    {15, "d-mmm-yy", "d-mmm-yy"},
    {16, "d-mmm", "d-mmm"},
    {17, "mmm-yy", "mmm-yy"},
    {18, "h:mm AM/PM", "h:mm AM/PM"},
    {19, "h:mm:ss AM/PM", "h:mm:ss AM/PM"},
    {20, "h:mm", "h:mm"},
    {21, "h:mm:ss", "h:mm:ss"},
    {22, "m/d/yy h:mm", "m/d/yy h:mm"},
    {37, "#,##0 ;(#,##0)", "#,##0 ;(#,##0)"},
    {38, "#,##0 ;[Red](#,##0)", "#,##0 ;[Red](#,##0)"},
    {39, "#,##0.00;(#,##0.00)", "#,##0.00;(#,##0.00)"},
    {40, "#,##0.00;[Red](#,##0.00)", "#,##0.00;[Red](#,##0.00)"},
    {44, "_(\"$\"* #,##0.00_);_(\"$\"* \\(#,##0.00\\);_(\"$\"* \"-\"??_);_(@_)", "Currency"},
    {45, "mm:ss", "mm:ss"},
    {46, "[h]:mm:ss", "[h]:mm:ss"},
    {47, "mmss.0", "mmss.0"},
    {48, "##0.0E+0", "##0.0E+0"},
    {49, "@", "@"}
};

// Global instance for backward compatibility (deprecated)
static SkExcel::WorkbookBuilder g_globalWorkbook;

static tBool SheetNameNeedsQuotesForExcel(const tString& sSheetName) {
    if (sSheetName.empty()) {
        return false;
    }
    for (tChar wCh : sSheetName) {
        if (!(std::isalnum(static_cast<unsigned char>(wCh)) || wCh == '_')) {
            return true;
        }
    }
    return false;
}

static tString QuoteExcelSheetName(const tString& sSheetName) {
    if (!SheetNameNeedsQuotesForExcel(sSheetName)) {
        return sSheetName;
    }
    tString wQuoted = "'";
    for (tChar wCh : sSheetName) {
        if (wCh == '\'') {
            wQuoted += "''";
        } else {
            wQuoted += wCh;
        }
    }
    wQuoted += "'";
    return wQuoted;
}

static tString AnchorToAbsoluteA1(const tString& sAnchor) {
    tSize wPos = 0;
    if (wPos < sAnchor.size() && sAnchor[wPos] == '$') {
        ++wPos;
    }
    const tSize wColStart = wPos;
    while (wPos < sAnchor.size() && std::isalpha(static_cast<unsigned char>(sAnchor[wPos]))) {
        ++wPos;
    }
    const tString wCol = sAnchor.substr(wColStart, wPos - wColStart);
    if (wPos < sAnchor.size() && sAnchor[wPos] == '$') {
        ++wPos;
    }
    const tString wRow = sAnchor.substr(wPos);
    if (wCol.empty() && !wRow.empty()) {
        tString wAbs = sAnchor;
        if (!wAbs.empty() && wAbs.front() != '$') {
            wAbs = "$" + wAbs;
        }
        return wAbs;
    }
    return tString("$") + wCol + tString("$") + wRow;
}

static tString RefToAbsoluteA1(const tString& sRef) {
    const tSize wColon = sRef.find(':');
    if (wColon == tString::npos) {
        return AnchorToAbsoluteA1(sRef);
    }
    const tString wLeft = AnchorToAbsoluteA1(sRef.substr(0, wColon));
    const tString wRight = AnchorToAbsoluteA1(sRef.substr(wColon + 1));
    if (wLeft == wRight) {
        return wLeft;
    }
    return wLeft + ":" + wRight;
}

static tString FormatSheetQualifiedAbsRef(const tString& sSheetName, const tString& sRefA1) {
    return QuoteExcelSheetName(sSheetName) + "!" + RefToAbsoluteA1(sRefA1);
}

tBool SkExcel::WorkbookBuilder::AddDefinedName(const tString& sName, const tString& sDefinition, tInt sLocalSheetId) {
    if (sName.empty() || sDefinition.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    m_Impl->m_DefinedNames[sName] = Impl::DefinedNameEntry{sDefinition, sLocalSheetId};
    return true;
}

tBool SkExcel::WorkbookBuilder::AddNamedCell(const tString& sName, const tString& sCell, const tString& sSheetName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString sheet = sSheetName.empty() ? m_Impl->m_CurrentSheet : sSheetName;
    if (sheet.empty()) sheet = "Sheet1";
    m_Impl->m_DefinedNames[sName] = Impl::DefinedNameEntry{FormatSheetQualifiedAbsRef(sheet, wA1), -1};
    return true;
}

tBool SkExcel::WorkbookBuilder::AddNamedRange(const tString& sName, const tString& sTopLeft, const tString& sBottomRight, const tString& sSheetName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString tlA1, brA1; if (!GetA1(sTopLeft, tlA1) || !GetA1(sBottomRight, brA1)) return false;
    tString sheet = sSheetName.empty() ? m_Impl->m_CurrentSheet : sSheetName;
    if (sheet.empty()) sheet = "Sheet1";
    tString wRef = RefToAbsoluteA1(tlA1);
    if (tlA1 != brA1) {
        wRef += ":" + RefToAbsoluteA1(brA1);
    }
    m_Impl->m_DefinedNames[sName] = Impl::DefinedNameEntry{FormatSheetQualifiedAbsRef(sheet, wRef), -1};
    return true;
}

// WorkbookBuilder implementation
SkExcel::WorkbookBuilder::WorkbookBuilder() : m_Impl(std::make_unique<Impl>()) {}

SkExcel::WorkbookBuilder::~WorkbookBuilder() = default;


tBool SkExcel::WorkbookBuilder::SetCellValue(const tString& sCell, const tString& sValue) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellValues[wA1] = sValue;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontColor(const tString& sCell, const tString& sHexColor) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString wArgb;
    if (!normalize_hex_to_argb(sHexColor, wArgb)) return false;
    it->second.m_CellFontColor[wA1] = wArgb;
    it->second.m_CellFontSpec[wA1].color = wArgb;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontName(const tString& sCell, const tString& sFontName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellFontSpec[wA1].name = sFontName;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontSize(const tString& sCell, tInt sSize) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    if (sSize <= 0) return false;
    it->second.m_CellFontSpec[wA1].size = sSize;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontBold(const tString& sCell, tBool sBold) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellFontSpec[wA1].bold = sBold;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontItalic(const tString& sCell, tBool sItalic) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellFontSpec[wA1].italic = sItalic;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFontUnderline(const tString& sCell, tBool sUnderline) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellFontSpec[wA1].underline = sUnderline;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellMerge(const tString& sTopLeft, const tString& sBottomRight) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    
    // Validate cell addresses
    tString wTopLeft, wBottomRight;
    if (!GetA1(sTopLeft, wTopLeft) || !GetA1(sBottomRight, wBottomRight)) return false;
    
    // Parse cell addresses to check if topLeft is actually top-left
    tInt wTopRow, wTopCol, wBottomRow, wBottomCol;
    tString wTemp;
    if (!Parse_cell_address(wTopLeft, wTopRow, wTopCol, wTemp) || 
        !Parse_cell_address(wBottomRight, wBottomRow, wBottomCol, wTemp)) return false;
    
    // Ensure topLeft is actually top-left
    if (wTopRow > wBottomRow || wTopCol > wBottomCol) return false;
    
    // Add merged range
    it->second.m_MergedRanges.emplace_back(wTopLeft, wBottomRight);
    return true;
}

tBool SkExcel::WorkbookBuilder::SetRowHeight(tInt sRow, tDouble sHeight) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    
    if (sRow <= 0 || sHeight <= 0) return false;
    
    it->second.m_RowDimensions.m_RowHeights[sRow] = sHeight;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetColumnWidth(tInt sColumn, tDouble sWidth) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    
    if (sColumn <= 0 || sWidth <= 0) return false;
    
    it->second.m_ColumnDimensions.m_ColumnWidths[sColumn] = sWidth;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetColumnWidth(const tString& sColumn, tDouble sWidth) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    
    if (sWidth <= 0) return false;
    
    tInt wColumnIndex = column_letters_to_index(sColumn);
    if (wColumnIndex <= 0) return false;
    
    it->second.m_ColumnDimensions.m_ColumnWidths[wColumnIndex] = sWidth;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetColumnHidden(tInt sColumn) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    if (sColumn <= 0) return false;
    it->second.m_ColumnDimensions.m_ColumnHidden.insert(sColumn);
    return true;
}

tBool SkExcel::WorkbookBuilder::SetSheetDefaultRowHeight(tDouble sHeightPt) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    if (sHeightPt <= 0.0) return false;
    it->second.m_SheetDefaultRowHeightPt = sHeightPt;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetSheetDefaultColWidth(tDouble sWidthChars) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    if (sWidthChars <= 0.0) return false;
    it->second.m_SheetDefaultColWidthChars = sWidthChars;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetSheetShowGridLines(tBool sShow) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    it->second.m_ShowGridLines = sShow;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetSheetZoomScaleNormal(tInt sZoom) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    if (sZoom <= 0) return false;
    it->second.m_ZoomScaleNormal = sZoom;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetWorkbookDefaultFont(const tString& sFontName, tInt sSizePt) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    if (!sFontName.empty()) {
        m_Impl->m_DefaultFontName = sFontName;
    }
    if (sSizePt > 0) {
        m_Impl->m_DefaultFontSize = sSizePt;
    }
    return true;
}

tString SkExcel::WorkbookBuilder::DefaultFontName() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    return m_Impl->m_DefaultFontName;
}

tDouble SkExcel::WorkbookBuilder::DefaultFontSize() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    return static_cast<tDouble>(m_Impl->m_DefaultFontSize);
}

tBool SkExcel::WorkbookBuilder::SetCellAlignment(const tString& sCell, const tString& sHorizontal, const tString& sVertical, tBool sWrap) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    if (!sHorizontal.empty()) it->second.m_CellHorizAlign[wA1] = sHorizontal;
    if (!sVertical.empty()) it->second.m_CellVertAlign[wA1] = sVertical;
    if (sWrap) it->second.m_CellWrap.insert(wA1); else it->second.m_CellWrap.erase(wA1);
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellHorizontalAlign(const tString& sCell, const tString& sHorizontal) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellHorizAlign[wA1] = sHorizontal;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellVerticalAlign(const tString& sCell, const tString& sVertical) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    it->second.m_CellVertAlign[wA1] = sVertical;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellWrap(const tString& sCell, tBool sWrap) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    if (sWrap) it->second.m_CellWrap.insert(wA1); else it->second.m_CellWrap.erase(wA1);
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellTextRotation(const tString& sCell, tInt sDegrees) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    // Map UI-like degrees (-90..90, 255 vertical) to OOXML textRotation 0..180, 255 vertical.
    if (sDegrees == 255) {
        it->second.m_CellTextRotation[wA1] = 255;
        return true;
    }
    if (sDegrees < -90) sDegrees = -90;
    if (sDegrees > 90) sDegrees = 90;
    tInt rot = (sDegrees < 0) ? (90 + (-sDegrees)) : sDegrees; // -45 -> 135, -90 -> 180
    if (rot < 0) rot = 0; if (rot > 180) rot = 180;
    it->second.m_CellTextRotation[wA1] = rot;
    return true;
}


tBool SkExcel::WorkbookBuilder::SetCellFillColor(const tString& sCell, const tString& sHexColor) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString wArgb;
    if (!normalize_hex_to_argb(sHexColor, wArgb)) return false;
    it->second.m_CellFillColor[wA1] = wArgb;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellBorderColor(const tString& sCell, const tString& sHexColor) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString wArgb;
    if (!normalize_hex_to_argb(sHexColor, wArgb)) return false;
    it->second.m_CellBorderColor[wA1] = wArgb;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellBorderSides(const tString& sCell,
    const tString& sLeftStyle, const tString& sLeftColor,
    const tString& sRightStyle, const tString& sRightColor,
    const tString& sTopStyle, const tString& sTopColor,
    const tString& sBottomStyle, const tString& sBottomColor) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    auto wNorm = [](const tString& sColor, tString& sOut)->tBool{
        if (sColor.empty()) { sOut.clear(); return true; }
        return normalize_hex_to_argb(sColor, sOut);
    };
    Impl::BorderSpec wBorderSpec;
    if (!wNorm(sLeftColor,   wBorderSpec.left.argb)) return false;   wBorderSpec.left.style   = normalize_border_style(sLeftStyle);
    if (!wNorm(sRightColor,  wBorderSpec.right.argb)) return false;  wBorderSpec.right.style  = normalize_border_style(sRightStyle);
    if (!wNorm(sTopColor,    wBorderSpec.top.argb)) return false;    wBorderSpec.top.style    = normalize_border_style(sTopStyle);
    if (!wNorm(sBottomColor, wBorderSpec.bottom.argb)) return false; wBorderSpec.bottom.style = normalize_border_style(sBottomStyle);
    it->second.m_CellBorderSides[wA1] = wBorderSpec;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellNumberFormat(const tString& sCell, const tString& sFormatCode) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tInt formatId = m_Impl->find_or_create_number_format(sFormatCode);
    it->second.m_CellNumberFormat[wA1] = formatId;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellCurrency(const tString& sCell, const tString& sSymbol) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString wFormatCode = "_(\"" + sSymbol + "\"* #,##0.00_);_(\"" + sSymbol + "\"* \\(#,##0.00\\);_(\"" + sSymbol + "\"* \"-\"??_);_(@_)";
    tInt formatId = m_Impl->find_or_create_number_format(wFormatCode);
    it->second.m_CellNumberFormat[wA1] = formatId;
	return true;
}

tBool SkExcel::WorkbookBuilder::SetCellPercentage(const tString& sCell, tInt sDecimalPlaces) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tString wFormatCode = "0";
    if (sDecimalPlaces > 0) {
        wFormatCode += ".";
        for (tInt i = 0; i < sDecimalPlaces; i++) {
            wFormatCode += "0";
        }
    }
    wFormatCode += "%";
    tInt wFormatId = m_Impl->find_or_create_number_format(wFormatCode);
    it->second.m_CellNumberFormat[wA1] = wFormatId;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellDate(const tString& sCell, const tString& sDateFormat) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tInt sFormatId = m_Impl->find_or_create_number_format(sDateFormat);
    it->second.m_CellNumberFormat[wA1] = sFormatId;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellDateSerial(const tString& sCell, tDouble sExcelSerial, const tString& sFrenchFormat) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    it->second.m_CellValues[wA1] = std::to_string(sExcelSerial);
    tInt formatId = m_Impl->find_or_create_number_format(sFrenchFormat);
    it->second.m_CellNumberFormat[wA1] = formatId;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellDateYMD(const tString& sCell, tInt sYear, tInt sMonth, tInt sDay, const tString& sFrenchFormat) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    tDouble wSerial = m_Impl->date_to_excel_serial(sYear, sMonth, sDay);
    if (wSerial <= 0) return false;
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    it->second.m_CellValues[wA1] = std::to_string(wSerial);
    tInt wFormatId = m_Impl->find_or_create_number_format(sFrenchFormat);
    it->second.m_CellNumberFormat[wA1] = wFormatId;
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCellFormula(const tString& sCell, const tString& sFormula) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1; if (!GetA1(sCell, wA1)) return false;
    // Store in current sheet (with '=' prefix)
    tString wFormulaWithEquals = (sFormula[0] == '=') ? sFormula : "=" + sFormula;
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return false;
    it->second.m_CellFormulas[wA1] = wFormulaWithEquals;
    it->second.m_CellValues.erase(wA1);
    return true;
}

tBool SkExcel::WorkbookBuilder::AddStructuredTable(
    const tString& sSheetName,
    const tString& sName,
    const tString& sRef,
    const std::vector<StructuredTableColumnSpec>& sColumns,
    const tString& sStyleName,
    tBool sShowRowStripes,
    tBool sShowColumnStripes,
    tBool sShowFirstColumn,
    tBool sShowLastColumn,
    tBool sHasHeaderRow,
    const std::map<tString, tString>* sStyleElementCss,
    tBool sHasAutoFilter,
    tInt sTotalsRowCount,
    const tString* sDisplayName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    if (sSheetName.empty() || sName.empty() || sRef.empty() || sColumns.empty()) {
        return false;
    }
    WorkbookBuilder::Impl::TableExport wTable;
    wTable.sheetName = sSheetName;
    wTable.name = sName;
    wTable.displayName = (sDisplayName != nullptr && !sDisplayName->empty())
        ? *sDisplayName
        : ExcelTableDisplayName(sName);
    wTable.ref = sRef;
    wTable.totalsRowCount = sTotalsRowCount > 0 ? sTotalsRowCount : 0;
    wTable.styleName = sStyleName;
    wTable.showRowStripes = sShowRowStripes;
    wTable.showColumnStripes = sShowColumnStripes;
    wTable.showFirstColumn = sShowFirstColumn;
    wTable.showLastColumn = sShowLastColumn;
    wTable.hasHeaderRow = sHasHeaderRow;
    wTable.hasAutoFilter = sHasAutoFilter;
    if (sStyleElementCss != nullptr) {
        wTable.styleElementCss = *sStyleElementCss;
    }
    wTable.columns.reserve(sColumns.size());
    for (const auto& wCol : sColumns) {
        WorkbookBuilder::Impl::TableColumnExport wExportCol;
        wExportCol.name = wCol.name;
        wExportCol.calculatedFormula = SanitizeTableFormulaForOoxmlXml(wCol.calculatedFormula);
        if (!wExportCol.calculatedFormula.empty() && wExportCol.calculatedFormula.front() == '=') {
            wExportCol.calculatedFormula.erase(0, 1);
        }
        wExportCol.filterButtonHidden = wCol.filterButtonHidden;
        wExportCol.totalsRowLabel = wCol.totalsRowLabel;
        wExportCol.totalsRowFunction = wCol.totalsRowFunction;
        wExportCol.totalsRowFormula = SanitizeTableFormulaForOoxmlXml(wCol.totalsRowFormula);
        if (!wExportCol.totalsRowFormula.empty() && wExportCol.totalsRowFormula.front() == '=') {
            wExportCol.totalsRowFormula.erase(0, 1);
        }
        wTable.columns.push_back(std::move(wExportCol));
    }
    m_Impl->m_Tables.push_back(std::move(wTable));
    return true;
}

tBool SkExcel::WorkbookBuilder::MarkCellFormulaAsTableColumn(const tString& sCell) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1;
    if (!GetA1(sCell, wA1)) {
        return false;
    }
    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) {
        return false;
    }
    wIt->second.m_TableColumnFormulas.insert(wA1);
    return true;
}

tBool SkExcel::WorkbookBuilder::AddSparkline(const tString& sCell, const tString& sSourceRange, tBool sMarkers) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    tString wA1;
    if (!GetA1(sCell, wA1) || sSourceRange.empty()) {
        return false;
    }
    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) {
        return false;
    }
    Impl::tSheetData::tSparklineExport wSpark;
    wSpark.cellRef = wA1;
    wSpark.sourceRange = sSourceRange;
    wSpark.markers = sMarkers;
    wIt->second.m_Sparklines.push_back(std::move(wSpark));
    return true;
}

tBool SkExcel::WorkbookBuilder::Clear() {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    m_Impl->m_MapSheets.clear();
    m_Impl->m_SheetOrder.clear();
    m_Impl->m_CurrentSheet = "Sheet1";
    m_Impl->m_MapSheets["Sheet1"] = Impl::tSheetData();
    m_Impl->m_SheetOrder.push_back("Sheet1");
    m_Impl->m_CustomFormats.clear();
    m_Impl->m_NextCustomFormatId = 164;
    m_Impl->m_Tables.clear();
    m_Impl->m_DefinedNames.clear();
    return true;
}

tSize SkExcel::WorkbookBuilder::GetSheetCount() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    return m_Impl->m_SheetOrder.size();
}

tSize SkExcel::WorkbookBuilder::GetCellCount() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (it == m_Impl->m_MapSheets.end()) return 0;
    return it->second.m_CellValues.size();
}

tSize SkExcel::WorkbookBuilder::GetCellCount(const tString& sSheetName) const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    auto it = m_Impl->m_MapSheets.find(sSheetName);
    if (it == m_Impl->m_MapSheets.end()) return 0;
    return it->second.m_CellValues.size();
}

// Multi-sheet management methods
tBool SkExcel::WorkbookBuilder::AddSheet(const tString& sSheetName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    if (sSheetName.empty() || m_Impl->m_MapSheets.find(sSheetName) != m_Impl->m_MapSheets.end()) {
        return false; // Empty name or sheet already exists
    }
    m_Impl->m_MapSheets[sSheetName] = Impl::tSheetData();
    m_Impl->m_SheetOrder.push_back(sSheetName);
    return true;
}

tBool SkExcel::WorkbookBuilder::RemoveSheet(const tString& sSheetName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    if (m_Impl->m_MapSheets.size() <= 1) {
        return false;
    }
    auto wIt = m_Impl->m_MapSheets.find(sSheetName);
    if (wIt == m_Impl->m_MapSheets.end()) {
        return false;
    }
    if (m_Impl->m_CurrentSheet == sSheetName) {
        m_Impl->m_CurrentSheet = m_Impl->m_MapSheets.begin()->first;
        if (m_Impl->m_CurrentSheet == sSheetName) {
            m_Impl->m_CurrentSheet = std::next(m_Impl->m_MapSheets.begin())->first;
        }
    }
    m_Impl->m_MapSheets.erase(wIt);
    m_Impl->m_SheetOrder.erase(
        std::remove(m_Impl->m_SheetOrder.begin(), m_Impl->m_SheetOrder.end(), sSheetName),
        m_Impl->m_SheetOrder.end());
    return true;
}

tBool SkExcel::WorkbookBuilder::SetCurrentSheet(const tString& sSheetName) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    if (m_Impl->m_MapSheets.find(sSheetName) == m_Impl->m_MapSheets.end()) {
        return false; // Sheet doesn't exist
    }
    if (std::find(m_Impl->m_SheetOrder.begin(), m_Impl->m_SheetOrder.end(), sSheetName)
        == m_Impl->m_SheetOrder.end()) {
        m_Impl->m_SheetOrder.push_back(sSheetName);
    }
    m_Impl->m_CurrentSheet = sSheetName;
    return true;
}

tString SkExcel::WorkbookBuilder::GetCurrentSheet() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    return m_Impl->m_CurrentSheet;
}

std::vector<tString> SkExcel::WorkbookBuilder::GetSheetNames() const {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    return m_Impl->m_SheetOrder;
}

// Conditional formatting methods
tBool SkExcel::WorkbookBuilder::AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor, const tString& sFillColor, tInt sPriority) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    
    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) return false;
    
    Impl::tConditionalFormatRule wRule;
    wRule.type = "expression";
    wRule.formula = sFormula;
    wRule.range = sRange;
    wRule.fontColor = sFontColor;
    wRule.fillColor = sFillColor;
    wRule.priority = sPriority;
    wRule.stopIfTrue = false;
    
    wIt->second.m_ConditionalFormats.push_back(wRule);
    return true;
}

tBool SkExcel::WorkbookBuilder::AddConditionalFormatStripedRows(const tString& sRange, const tString& sFillColor) {
    return AddConditionalFormat(sRange, "MOD(ROW(),2)=0", "", sFillColor, 1);
}

tBool SkExcel::WorkbookBuilder::AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor, const tString& sFillColor, tInt sPriority, const tString& sType, const tString& sOperator) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    
    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) return false;
    
    Impl::tConditionalFormatRule wRule;
    wRule.type = sType;
    wRule.formula = sFormula;
    wRule.range = sRange;
    wRule.fontColor = sFontColor;
    wRule.fillColor = sFillColor;
    wRule.priority = sPriority;
    wRule.stopIfTrue = false;
    wRule.operator_ = sOperator;
    
    wIt->second.m_ConditionalFormats.push_back(wRule);
    return true;
}

tBool SkExcel::WorkbookBuilder::AddConditionalFormatIconSet(const tString& sRange, const tString& sIconSet, const tString& sIconStyle, tBool sShowValue, tBool sReverse, tInt sPriority, const std::vector<tString>& sThresholds, const std::vector<tString>& sThresholdTypes) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);

    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) return false;

    Impl::tConditionalFormatRule wRule;
    wRule.type = "iconSet";
    wRule.range = sRange;
    wRule.iconSet = sIconSet;
    wRule.iconStyle = sIconStyle;
    wRule.showValue = sShowValue;
    wRule.reverse = sReverse;
    wRule.priority = sPriority;
    wRule.stopIfTrue = false;
    wRule.iconThresholds = sThresholds;
    wRule.iconThresholdTypes = sThresholdTypes;

    wIt->second.m_ConditionalFormats.push_back(wRule);
    return true;
}

tBool SkExcel::WorkbookBuilder::AddConditionalFormatDataBar(
    const tString& sRange, const tString& sColor, const tString& sMinVal, const tString& sMaxVal,
    const tString& sStyle, tInt sPriority) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);

    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) {
        return false;
    }

    Impl::tConditionalFormatRule wRule;
    wRule.type = "dataBar";
    wRule.range = sRange;
    wRule.dataBarColor = sColor;
    wRule.dataBarMinVal = sMinVal;
    wRule.dataBarMaxVal = sMaxVal;
    wRule.dataBarStyle = sStyle.empty() ? "gradient" : sStyle;
    wRule.priority = sPriority;
    wRule.stopIfTrue = false;
    wIt->second.m_ConditionalFormats.push_back(std::move(wRule));
    return true;
}

tBool SkExcel::WorkbookBuilder::AddConditionalFormatColorScale(
    const tString& sRange, const tString& sMinColor, const tString& sMidColor, const tString& sMaxColor,
    const tString& sMinVal, const tString& sMidVal, const tString& sMaxVal, tInt sPriority) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);

    auto wIt = m_Impl->m_MapSheets.find(m_Impl->m_CurrentSheet);
    if (wIt == m_Impl->m_MapSheets.end()) {
        return false;
    }

    Impl::tConditionalFormatRule wRule;
    wRule.type = "colorScale";
    wRule.range = sRange;
    wRule.colorScaleColors = {sMinColor, sMidColor, sMaxColor};
    wRule.colorScaleVals = {sMinVal, sMidVal, sMaxVal};
    wRule.priority = sPriority;
    wRule.stopIfTrue = false;
    wIt->second.m_ConditionalFormats.push_back(std::move(wRule));
    return true;
}


tInt SkExcel::WorkbookBuilder::Impl::find_or_create_number_format(const tString& sFormatCode) {
    // First check built-in formats
    for (const auto& wFmt : m_BuiltinFormats) {
        if (wFmt.formatCode == sFormatCode) {
            return wFmt.id;
        }
    }
    
    // Then check custom formats
    for (const auto& wFmt : m_CustomFormats) {
        if (wFmt.formatCode == sFormatCode) {
            return wFmt.id;
        }
    }
    
    // Create new custom format
    Impl::tNumberFormat newFmt;
    newFmt.id = m_NextCustomFormatId++;
    newFmt.formatCode = sFormatCode;
    newFmt.description = "Custom";
    m_CustomFormats.push_back(newFmt);
    return newFmt.id;
}

tDouble SkExcel::WorkbookBuilder::Impl::date_to_excel_serial(tInt sYear, tInt sMonth, tInt sDay) {
    if (sYear < 1900) return 0.0;
    
    const tInt wDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    tDouble wTotalDays = 1.0;
    
    for (tInt wYear = 1900; wYear < sYear; wYear++) {
        wTotalDays += 365;
        if ((wYear % 4 == 0 && wYear % 100 != 0) || (wYear % 400 == 0)) {
            wTotalDays += 1;
        }
    }
    
    for (tInt wMonth = 1; wMonth < sMonth; wMonth++) {
        wTotalDays += wDaysInMonth[wMonth-1];
        if (wMonth == 2 && ((sYear % 4 == 0 && sYear % 100 != 0) || (sYear % 400 == 0))) {
            wTotalDays += 1;
        }
    }
    
    wTotalDays += sDay - 1;
    return wTotalDays;
}

static void AppendDxfFromTableStyleCss(pugi::xml_node& sDxf, const tString& sCss);

tBool SkExcel::WorkbookBuilder::Impl::build_ooxml_minimal(const std::filesystem::path& sRoot) {
    //using tString;
	std::error_code wErrorCode;
	std::filesystem::create_directories(sRoot, wErrorCode);

    // Stable tableN.xml numbering: sheet order, then top-left ref (matches sheet rel order).
    {
        std::map<tString, tInt> wSheetOrder;
        for (tSize wSheetIdx = 0; wSheetIdx < m_SheetOrder.size(); ++wSheetIdx) {
            wSheetOrder[m_SheetOrder[wSheetIdx]] = static_cast<tInt>(wSheetIdx);
        }
        auto wTableSortKey = [&](const TableExport& sTable) {
            const tInt wSheetIdx =
                wSheetOrder.count(sTable.sheetName) ? wSheetOrder.at(sTable.sheetName) : 9999;
            return std::make_pair(wSheetIdx, sTable.ref);
        };
        std::sort(m_Tables.begin(), m_Tables.end(),
                  [&](const TableExport& sA, const TableExport& sB) {
                      return wTableSortKey(sA) < wTableSortKey(sB);
                  });
    }

    // Build unique font specs and color sets
    std::set<Impl::FontSpec> wUniqFontSpecs;
    std::set<tString> wUniqFontColors;
    std::set<tString> wUniqFillColors;
    for (auto& sheetPair : this->m_MapSheets) {
        for (auto& kv : sheetPair.second.m_CellFontColor) wUniqFontColors.insert(kv.second);
        for (auto& kv : sheetPair.second.m_CellFillColor) wUniqFillColors.insert(kv.second);
        for (auto& kv : sheetPair.second.m_CellFontSpec) wUniqFontSpecs.insert(kv.second);
    }
    
    std::map<tString,tInt> wFontIdByColor; // ARGB -> fontId (0=default black)
    std::map<Impl::FontSpec, tInt> wFontIdBySpec; // FontSpec -> fontId
    std::map<tString,tInt> wFillIdByColor; // ARGB -> fillId (0 none, 1 gray125, >=2 solids)
    std::map<std::tuple<tInt,tInt,tInt,tInt,tString,tString,tInt,tInt>, tInt> wXfIndexByCombo; // (fontId,fillId,borderId,numFmtId,hAlign,vAlign,wrap,textRotation) -> xf index (0 default)
    
    struct tBorderKey { tString ls,lc, rs,rc, ts,tc, bs,bc; };
    auto wKeyToString = [](const tBorderKey& k){
        return k.ls+"|"+k.lc+"|"+k.rs+"|"+k.rc+"|"+k.ts+"|"+k.tc+"|"+k.bs+"|"+k.bc;
    };
    std::map<tString,tInt> wBorderIdByKey; // serialized BorderKey -> borderId

	// 1) [Content_Types].xml
	{
		pugi::xml_document wDoc;
		auto wTypes = wDoc.append_child("Types");
		wTypes.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/package/2006/content-types");
		auto wD1 = wTypes.append_child("Default"); wD1.append_attribute("Extension").set_value("rels"); wD1.append_attribute("ContentType").set_value("application/vnd.openxmlformats-package.relationships+xml");
		auto wD2 = wTypes.append_child("Default"); wD2.append_attribute("Extension").set_value("xml"); wD2.append_attribute("ContentType").set_value("application/xml");
		auto wO1 = wTypes.append_child("Override"); wO1.append_attribute("PartName").set_value("/xl/workbook.xml"); wO1.append_attribute("ContentType").set_value("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml");
        // Add worksheet overrides for all sheets
        tInt wSheetIndex = 1;
        for (tSize wSheetCount = this->m_MapSheets.size(); wSheetIndex <= static_cast<tInt>(wSheetCount); ++wSheetIndex) {
            auto wO = wTypes.append_child("Override");
            wO.append_attribute("PartName").set_value(("/xl/worksheets/sheet" + std::to_string(wSheetIndex) + ".xml").c_str());
            wO.append_attribute("ContentType").set_value("application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml");
        }
		auto wOTheme = wTypes.append_child("Override");
		wOTheme.append_attribute("PartName").set_value("/xl/theme/theme1.xml");
		wOTheme.append_attribute("ContentType").set_value(
			"application/vnd.openxmlformats-officedocument.theme+xml");
		auto wO3 = wTypes.append_child("Override"); wO3.append_attribute("PartName").set_value("/xl/styles.xml"); wO3.append_attribute("ContentType").set_value("application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml");
        
        // Check if we have string cells for sharedStrings
        tBool wHasStringCells = false;
        for (auto& sheetPair : this->m_MapSheets) {
            for (auto& kv : sheetPair.second.m_CellValues) { 
                if (detect_cell_type(kv.second) == CellType::String) { 
                    wHasStringCells = true; 
                    break; 
                } 
            }
            if (wHasStringCells) break;
        }
        
        if (wHasStringCells) {
			auto wO4 = wTypes.append_child("Override"); wO4.append_attribute("PartName").set_value("/xl/sharedStrings.xml"); wO4.append_attribute("ContentType").set_value("application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml");
		}
        for (tSize wTableIdx = 0; wTableIdx < this->m_Tables.size(); ++wTableIdx) {
            auto wOt = wTypes.append_child("Override");
            wOt.append_attribute("PartName").set_value(
                ("/xl/tables/table" + std::to_string(wTableIdx + 1) + ".xml").c_str());
            wOt.append_attribute("ContentType").set_value(
                "application/vnd.openxmlformats-officedocument.spreadsheetml.table+xml");
        }
		if (!write_text_file(sRoot / "[Content_Types].xml", to_string_xml(wDoc))) return false;
	}

	// 2) _rels/.rels
	{
		pugi::xml_document doc;
		auto rels = doc.append_child("Relationships");
		rels.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/package/2006/relationships");
		auto r = rels.append_child("Relationship");
		r.append_attribute("Id").set_value("rId1");
		r.append_attribute("Type").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument");
		r.append_attribute("Target").set_value("xl/workbook.xml");
		if (!write_text_file(sRoot / "_rels/.rels", to_string_xml(doc))) return false;
	}

	// 3) xl/workbook.xml
	{
		pugi::xml_document wDoc;
		auto wWb = wDoc.append_child("workbook");
		wWb.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
		wWb.append_attribute("xmlns:r").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships");
        auto wFileVersion = wWb.append_child("fileVersion");
        wFileVersion.append_attribute("appName").set_value("xl");
        wFileVersion.append_attribute("lastEdited").set_value("4");
        wFileVersion.append_attribute("lowestEdited").set_value("4");
        wFileVersion.append_attribute("rupBuild").set_value("4505");
        auto wWorkbookPr = wWb.append_child("workbookPr");
        wWorkbookPr.append_attribute("defaultThemeVersion").set_value("124226");
        auto wBookViews = wWb.append_child("bookViews");
        auto wWorkbookView = wBookViews.append_child("workbookView");
        wWorkbookView.append_attribute("xWindow").set_value("240");
        wWorkbookView.append_attribute("yWindow").set_value("15");
        wWorkbookView.append_attribute("windowWidth").set_value("16095");
        wWorkbookView.append_attribute("windowHeight").set_value("9660");
        auto wSheetsNode = wWb.append_child("sheets");
        tInt wSheetId = 1;
        tInt wRelId = 1;
        for (const tString& wSheetName : this->m_SheetOrder) {
            auto wSheet = wSheetsNode.append_child("sheet");
            wSheet.append_attribute("name").set_value(wSheetName.c_str());
            wSheet.append_attribute("sheetId").set_value(std::to_string(wSheetId).c_str());
            wSheet.append_attribute("r:id").set_value(("rId" + std::to_string(wRelId)).c_str());
            wSheetId++;
            wRelId++;
        }
        if (!m_DefinedNames.empty()) {
            auto wDef = wWb.append_child("definedNames");
            for (const auto& kv : m_DefinedNames) {
                auto dn = wDef.append_child("definedName");
                dn.append_attribute("name").set_value(kv.first.c_str());
                if (kv.second.localSheetId >= 0) {
                    dn.append_attribute("localSheetId").set_value(
                        std::to_string(kv.second.localSheetId).c_str());
                }
                dn.text().set(kv.second.body.c_str());
            }
        }
        auto wCalcPr = wWb.append_child("calcPr");
        wCalcPr.append_attribute("calcId").set_value("124519");
        wCalcPr.append_attribute("fullCalcOnLoad").set_value("1");
		if (!write_text_file(sRoot / "xl/workbook.xml", to_string_xml(wDoc))) return false;
	}

	// 4) xl/_rels/workbook.xml.rels
	{
		pugi::xml_document wDoc;
		auto wRelationShips = wDoc.append_child("Relationships");
		wRelationShips.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/package/2006/relationships");
        // Add worksheet relationships
        tInt wRelId = 1;
        tInt wSheetIndex = 1;
        for (tSize wSheetCount = this->m_MapSheets.size(); wSheetIndex <= static_cast<tInt>(wSheetCount); ++wSheetIndex) {
            auto wRelationShip = wRelationShips.append_child("Relationship");
            wRelationShip.append_attribute("Id").set_value(("rId" + std::to_string(wRelId)).c_str());
            wRelationShip.append_attribute("Type").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet");
            wRelationShip.append_attribute("Target").set_value(("worksheets/sheet" + std::to_string(wSheetIndex) + ".xml").c_str());
            wRelId++;
        }

        auto wThemeRel = wRelationShips.append_child("Relationship");
        wThemeRel.append_attribute("Id").set_value(("rId" + std::to_string(wRelId)).c_str());
        wThemeRel.append_attribute("Type").set_value(
            "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme");
        wThemeRel.append_attribute("Target").set_value("theme/theme1.xml");
        wRelId++;
        
        // Add styles relationship
		auto wStyleRelationShip = wRelationShips.append_child("Relationship");
        wStyleRelationShip.append_attribute("Id").set_value(("rId" + std::to_string(wRelId)).c_str());
		wStyleRelationShip.append_attribute("Type").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles");
		wStyleRelationShip.append_attribute("Target").set_value("styles.xml");
        wRelId++;
        
        tBool wHasStringCells = false;
        for (auto& sheetPair : this->m_MapSheets) {
            for (auto& kv : sheetPair.second.m_CellValues) { 
                if (detect_cell_type(kv.second) == CellType::String) { 
                    wHasStringCells = true; 
                    break; 
                } 
            }
            if (wHasStringCells) break;
        }
        
        if (wHasStringCells) {
			auto wRelationShip = wRelationShips.append_child("Relationship");
            wRelationShip.append_attribute("Id").set_value(("rId" + std::to_string(wRelId)).c_str());
			wRelationShip.append_attribute("Type").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings");
			wRelationShip.append_attribute("Target").set_value("sharedStrings.xml");
		}
		if (!write_text_file(sRoot / "xl/_rels/workbook.xml.rels", to_string_xml(wDoc))) return false;
	}

	if (!write_text_file(sRoot / "xl/theme/theme1.xml", OfficeThemeXml())) {
		return false;
	}

    // 5) xl/styles.xml: build from requested font/fill colors and map to xf indices
	{
		pugi::xml_document wDoc;
		auto wStyleSheet = wDoc.append_child("styleSheet");
		wStyleSheet.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
        
        // numFmts FIRST (required order)
        auto wNumFmts = wStyleSheet.append_child("numFmts");
        tInt wNumFmtCount = 0;
        for (const auto& fmt : m_CustomFormats) {
            auto wNumFmt = wNumFmts.append_child("numFmt");
            wNumFmt.append_attribute("numFmtId").set_value(std::to_string(fmt.id).c_str());
            wNumFmt.append_attribute("formatCode").set_value(fmt.formatCode.c_str());
            wNumFmtCount++;
        }
        if (wNumFmtCount > 0) {
            wNumFmts.append_attribute("count").set_value(std::to_string(wNumFmtCount).c_str());
        } else {
            wStyleSheet.remove_child(wNumFmts);
        }

            // fonts (build list from unique font specs; index 0 is always default)
            auto wFonts = wStyleSheet.append_child("fonts"); 
            wFonts.append_attribute("count").set_value(std::to_string(1 + (tInt)wUniqFontSpecs.size()).c_str());
            
            // Default font (index 0)
            auto wDefaultFont = wFonts.append_child("font");
            wDefaultFont.append_child("sz").append_attribute("val").set_value(std::to_string(m_DefaultFontSize).c_str());
            wDefaultFont.append_child("color").append_attribute("rgb").set_value("FF000000");
            wDefaultFont.append_child("name").append_attribute("val").set_value(m_DefaultFontName.c_str());
            
            // Map font specs to IDs
            tInt wRunningFontId = 1;
            for (const auto& fontSpec : wUniqFontSpecs) {
                wFontIdBySpec[fontSpec] = wRunningFontId;
                auto wFont = wFonts.append_child("font");
                
                // Font size
                wFont.append_child("sz").append_attribute("val").set_value(std::to_string(fontSpec.size > 0 ? fontSpec.size : 11).c_str());
                
                // Font color
                tString wColor = fontSpec.color.empty() ? "FF000000" : fontSpec.color;
                wFont.append_child("color").append_attribute("rgb").set_value(wColor.c_str());
                
                // Font name
                tString wFontName = fontSpec.name.empty() ? m_DefaultFontName : fontSpec.name;
                wFont.append_child("name").append_attribute("val").set_value(wFontName.c_str());
                
                // Font styles
                if (fontSpec.bold) {
                    wFont.append_child("b");
                }
                if (fontSpec.italic) {
                    wFont.append_child("i");
                }
                if (fontSpec.underline) {
                    wFont.append_child("u");
                }
                
                wRunningFontId++;
            }
            
            // Legacy mapping for backward compatibility
            for (const auto& argb : wUniqFontColors) {
                if (wFontIdByColor.find(argb) == wFontIdByColor.end()) {
                    // Find a font spec with this color or create a simple one
                    Impl::FontSpec simpleSpec;
                    simpleSpec.color = argb;
                    simpleSpec.size = m_DefaultFontSize;
                    simpleSpec.name = m_DefaultFontName;
                    if (wFontIdBySpec.find(simpleSpec) != wFontIdBySpec.end()) {
                        wFontIdByColor[argb] = wFontIdBySpec[simpleSpec];
                    }
                }
            }
        
            // fills (0 none, 1 gray125, then solids for uniqFillColors)
            auto wFills = wStyleSheet.append_child("fills"); wFills.append_attribute("count").set_value(std::to_string(2 + (tInt)wUniqFillColors.size()).c_str());
            wFills.append_child("fill").append_child("patternFill").append_attribute("patternType").set_value("none");
            wFills.append_child("fill").append_child("patternFill").append_attribute("patternType").set_value("gray125");
            tInt wRunningFillId = 2;
            for (const auto& argb : wUniqFillColors) {
                wFillIdByColor[argb] = wRunningFillId;
                auto fill = wFills.append_child("fill");
                auto pf = fill.append_child("patternFill"); pf.append_attribute("patternType").set_value("solid");
                pf.append_child("fgColor").append_attribute("rgb").set_value(argb.c_str());
                pf.append_child("bgColor").append_attribute("indexed").set_value("64");
                wRunningFillId++;
            }
        
        // Collect unique border specs from cellBorderSides and cellBorderColor across all sheets
        std::vector<tBorderKey> wUniqueBorders;
        auto wPushUnique = [&](const tBorderKey& k){
            tString s = wKeyToString(k);
            if (!wBorderIdByKey.count(s)) { wBorderIdByKey[s] = (tInt)wBorderIdByKey.size()+1; wUniqueBorders.push_back(k); }
        };
        for (auto& sheetPair : m_MapSheets) {
            const auto& wSheet = sheetPair.second;
            for (auto& wBorderSize : wSheet.m_CellBorderSides) {
                const auto& spec = wBorderSize.second;
                tBorderKey k{spec.left.style,  spec.left.argb,
                            spec.right.style, spec.right.argb,
                            spec.top.style,   spec.top.argb,
                            spec.bottom.style,spec.bottom.argb};
                wPushUnique(k);
            }
            for (auto& kv : wSheet.m_CellBorderColor) {
                tString thinStyle = normalize_border_style("thin");
                tBorderKey k{thinStyle, kv.second, thinStyle, kv.second, thinStyle, kv.second, thinStyle, kv.second};
                wPushUnique(k);
            }
        }
        
        // borders: index 0 = empty; then each unique spec
        auto wBorders = wStyleSheet.append_child("borders"); wBorders.append_attribute("count").set_value(std::to_string(1 + (tInt)wUniqueBorders.size()).c_str());
        wBorders.append_child("border");
        auto wEmitSide = [](pugi::xml_node parent, const char* name, const tString& sty, const tString& argb){
            auto n = parent.append_child(name);
            if (!sty.empty()) n.append_attribute("style").set_value(sty.c_str());
            if (!argb.empty()) n.append_child("color").append_attribute("rgb").set_value(argb.c_str());
        };
        for (const auto& wBorder : wUniqueBorders) {
            auto b = wBorders.append_child("border");
            wEmitSide(b, "left",   wBorder.ls, wBorder.lc);
            wEmitSide(b, "right",  wBorder.rs, wBorder.rc);
            wEmitSide(b, "top",    wBorder.ts, wBorder.tc);
            wEmitSide(b, "bottom", wBorder.bs, wBorder.bc);
        }
        
		// cellStyleXfs (required)
		auto wCsx = wStyleSheet.append_child("cellStyleXfs"); wCsx.append_attribute("count").set_value("1");
		auto wCsx_xf = wCsx.append_child("xf"); wCsx_xf.append_attribute("numFmtId").set_value("0"); wCsx_xf.append_attribute("fontId").set_value("0"); wCsx_xf.append_attribute("fillId").set_value("0"); wCsx_xf.append_attribute("borderId").set_value("0");
        
        // cellXfs: create one entry per (font,fill,border,numFmt) combo used; default at index 0
        // Collect style combos per sheet — same A1 on different sheets can have different styles (e.g. G1).
        std::set<std::tuple<tInt,tInt,tInt,tInt,tString,tString,tInt,tInt>> combos; // + textRotation
        for (auto& sheetPair : m_MapSheets) {
            const auto& wSheet = sheetPair.second;
            std::set<tString> wSheetA1;
            for (auto& kv : wSheet.m_CellValues) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellFontColor) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellFontSpec) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellFillColor) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellBorderColor) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellBorderSides) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellNumberFormat) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellHorizAlign) wSheetA1.insert(kv.first);
            for (auto& kv : wSheet.m_CellVertAlign) wSheetA1.insert(kv.first);
            for (auto& a : wSheet.m_CellWrap) wSheetA1.insert(a);
            for (const auto& wA1 : wSheetA1) {
                tInt wFontId = 0;
                tInt wFillId = 0;
                tInt wBorderId = 0;
                tInt wNumFmtId = 0;
                tString wHAlign;
                tString wVAlign;
                tInt wWrap = 0;
                tInt wRotation = INT_MIN;
                tBool wHasBorder = false;
                tBorderKey wBorderKey{};

                auto itFontSpec = wSheet.m_CellFontSpec.find(wA1);
                if (itFontSpec != wSheet.m_CellFontSpec.end()) {
                    auto it = wFontIdBySpec.find(itFontSpec->second);
                    if (it != wFontIdBySpec.end()) wFontId = it->second;
                } else {
                    auto itF = wSheet.m_CellFontColor.find(wA1);
                    if (itF != wSheet.m_CellFontColor.end()) {
                        auto it = wFontIdByColor.find(itF->second);
                        if (it != wFontIdByColor.end()) wFontId = it->second;
                    }
                }

                auto itB = wSheet.m_CellFillColor.find(wA1);
                if (itB != wSheet.m_CellFillColor.end()) {
                    auto it2 = wFillIdByColor.find(itB->second);
                    if (it2 != wFillIdByColor.end()) wFillId = it2->second;
                }
                auto itN = wSheet.m_CellNumberFormat.find(wA1);
                if (itN != wSheet.m_CellNumberFormat.end()) wNumFmtId = itN->second;
                {
                    auto itH = wSheet.m_CellHorizAlign.find(wA1);
                    if (itH != wSheet.m_CellHorizAlign.end()) wHAlign = itH->second;
                }
                {
                    auto itV = wSheet.m_CellVertAlign.find(wA1);
                    if (itV != wSheet.m_CellVertAlign.end()) wVAlign = itV->second;
                }
                if (wSheet.m_CellWrap.count(wA1)) {
                    wWrap = 1;
                }
                {
                    auto itR = wSheet.m_CellTextRotation.find(wA1);
                    if (itR != wSheet.m_CellTextRotation.end()) wRotation = itR->second;
                }
                auto itSides = wSheet.m_CellBorderSides.find(wA1);
                if (itSides != wSheet.m_CellBorderSides.end()) {
                    const auto& s = itSides->second;
                    wBorderKey = {s.left.style, s.left.argb, s.right.style, s.right.argb,
                                  s.top.style, s.top.argb, s.bottom.style, s.bottom.argb};
                    wHasBorder = true;
                } else {
                    auto itBorder = wSheet.m_CellBorderColor.find(wA1);
                    if (itBorder != wSheet.m_CellBorderColor.end()) {
                        tString thinStyle = normalize_border_style("thin");
                        wBorderKey = {thinStyle, itBorder->second, thinStyle, itBorder->second,
                                      thinStyle, itBorder->second, thinStyle, itBorder->second};
                        wHasBorder = true;
                    }
                }
                if (wHasBorder) {
                    auto itK = wBorderIdByKey.find(wKeyToString(wBorderKey));
                    if (itK != wBorderIdByKey.end()) wBorderId = itK->second;
                }
                combos.insert({wFontId, wFillId, wBorderId, wNumFmtId, wHAlign, wVAlign, wWrap, wRotation});
            }
        }
        // Ensure default present (0,0,0,0)
        combos.insert({0,0,0,0,tString(),tString(),0,INT_MIN});
            auto xfs = wStyleSheet.append_child("cellXfs"); xfs.append_attribute("count").set_value(std::to_string((tInt)combos.size()).c_str());
            // Build stable order and map indices
            tInt xfIndex = 0;
            for (auto combo : combos) {
                auto xf = xfs.append_child("xf");
            tInt fId = std::get<0>(combo); tInt fiId = std::get<1>(combo); tInt bId = std::get<2>(combo); tInt nId = std::get<3>(combo);
            tString hA = std::get<4>(combo); tString vA = std::get<5>(combo); tInt wrap = std::get<6>(combo); tInt rot = std::get<7>(combo);
            xf.append_attribute("numFmtId").set_value(std::to_string(nId).c_str()); if (nId != 0) xf.append_attribute("applyNumberFormat").set_value("1");
                xf.append_attribute("fontId").set_value(std::to_string(fId).c_str()); if (fId != 0) xf.append_attribute("applyFont").set_value("1");
                xf.append_attribute("fillId").set_value(std::to_string(fiId).c_str()); if (fiId != 0) xf.append_attribute("applyFill").set_value("1");
                xf.append_attribute("borderId").set_value(std::to_string(bId).c_str()); if (bId != 0) xf.append_attribute("applyBorder").set_value("1");
                xf.append_attribute("xfId").set_value("0");
                if (!hA.empty() || !vA.empty() || wrap != 0 || rot != INT_MIN) {
                    auto aln = xf.append_child("alignment");
                    if (!hA.empty()) aln.append_attribute("horizontal").set_value(hA.c_str());
                    if (!vA.empty()) aln.append_attribute("vertical").set_value(vA.c_str());
                    if (wrap != 0) aln.append_attribute("wrapText").set_value("1");
                    if (rot != INT_MIN) aln.append_attribute("textRotation").set_value(std::to_string(rot).c_str());
                    xf.append_attribute("applyAlignment").set_value("1");
                }
                // Note: alignment is per-cell, but stored in xf; we will emit alignment at cell time if needed
                wXfIndexByCombo[combo] = xfIndex++;
            }
        
        
		// cellStyles (required)
		auto cellStyles = wStyleSheet.append_child("cellStyles"); cellStyles.append_attribute("count").set_value("1");
		auto cs = cellStyles.append_child("cellStyle"); cs.append_attribute("name").set_value("Normal"); cs.append_attribute("xfId").set_value("0"); cs.append_attribute("builtinId").set_value("0");
		
		// dxfs (conditional formatting + custom table-style differential formats)
		tInt wDxfCount = 0;
		for (const auto& sheetPair : m_MapSheets) {
            for (const auto& wRule : sheetPair.second.m_ConditionalFormats) {
                if (wRule.type != "iconSet" && wRule.type != "dataBar" && wRule.type != "colorScale") {
                    wDxfCount++;
                }
            }
        }
        static const char* const kTableStyleElementOrder[] = {
            "wholeTable",
            "headerRow",
            "totalRow",
            "firstRowStripe",
            "secondRowStripe",
            "firstColumnStripe",
            "secondColumnStripe",
            "firstColumn",
            "lastColumn",
        };
        std::map<tString, std::map<tString, tInt>> wTableStyleElementDxfIds;
        std::vector<std::pair<tString, tString>> wTableStyleDxfCss;
        tString wDefaultTableStyleName = "TableStyleMedium2";
        for (const auto& wTable : m_Tables) {
            if (wTable.styleElementCss.empty() || wTable.styleName.empty()) {
                continue;
            }
            wDefaultTableStyleName = wTable.styleName;
            for (const char* wType : kTableStyleElementOrder) {
                auto wIt = wTable.styleElementCss.find(wType);
                if (wIt == wTable.styleElementCss.end()) {
                    continue;
                }
                if (wTableStyleElementDxfIds[wTable.styleName].count(wIt->first) > 0) {
                    continue;
                }
                wTableStyleElementDxfIds[wTable.styleName][wIt->first] =
                    wDxfCount + static_cast<tInt>(wTableStyleDxfCss.size());
                wTableStyleDxfCss.emplace_back(wIt->first, wIt->second);
            }
        }
        const tInt wTotalDxfCount = wDxfCount + static_cast<tInt>(wTableStyleDxfCss.size());
        if (wTotalDxfCount > 0) {
            auto wDxfs = wStyleSheet.append_child("dxfs");
            wDxfs.append_attribute("count").set_value(std::to_string(wTotalDxfCount).c_str());

            for (const auto& sheetPair : m_MapSheets) {
            for (const auto& wRule : sheetPair.second.m_ConditionalFormats) {
                if (wRule.type == "iconSet" || wRule.type == "dataBar" || wRule.type == "colorScale") {
                    continue;
                }

                    auto wDxf = wDxfs.append_child("dxf");

                    if (!wRule.fontColor.empty()) {
                        auto wFont = wDxf.append_child("font");
                        tString wColor = wRule.fontColor;
                        if (wColor[0] == '#') wColor = wColor.substr(1);
                        if (wColor.size() == 6) wColor = "FF" + wColor;
                        wFont.append_child("color").append_attribute("rgb").set_value(wColor.c_str());
                    }

                    if (!wRule.fillColor.empty()) {
                        auto wFill = wDxf.append_child("fill");
                        auto wPatternFill = wFill.append_child("patternFill");
                        wPatternFill.append_attribute("patternType").set_value("solid");
                        tString wColor = wRule.fillColor;
                        if (wColor[0] == '#') wColor = wColor.substr(1);
                        if (wColor.size() == 6) wColor = "FF" + wColor;
                        wPatternFill.append_child("bgColor").append_attribute("rgb").set_value(wColor.c_str());
                    }
                }
            }
            for (const auto& wPair : wTableStyleDxfCss) {
                auto wDxf = wDxfs.append_child("dxf");
                AppendDxfFromTableStyleCss(wDxf, wPair.second);
            }
        }

        if (!wTableStyleElementDxfIds.empty()) {
            auto wTableStyles = wStyleSheet.append_child("tableStyles");
            wTableStyles.append_attribute("count").set_value(
                std::to_string(wTableStyleElementDxfIds.size()).c_str());
            wTableStyles.append_attribute("defaultTableStyle").set_value(
                wDefaultTableStyleName.c_str());
            wTableStyles.append_attribute("defaultPivotStyle").set_value("PivotStyleLight16");
            for (const auto& wStyleEntry : wTableStyleElementDxfIds) {
                auto wTableStyle = wTableStyles.append_child("tableStyle");
                wTableStyle.append_attribute("name").set_value(wStyleEntry.first.c_str());
                wTableStyle.append_attribute("pivot").set_value("0");
                wTableStyle.append_attribute("count").set_value(
                    std::to_string(wStyleEntry.second.size()).c_str());
                for (const char* wType : kTableStyleElementOrder) {
                    auto wElIt = wStyleEntry.second.find(wType);
                    if (wElIt == wStyleEntry.second.end()) {
                        continue;
                    }
                    auto wEl = wTableStyle.append_child("tableStyleElement");
                    wEl.append_attribute("type").set_value(wElIt->first.c_str());
                    wEl.append_attribute("dxfId").set_value(std::to_string(wElIt->second).c_str());
                }
            }
        }

		if (!write_text_file(sRoot / "xl/styles.xml", to_string_xml(wDoc))) return false;
	}

    // 6) Optionally xl/sharedStrings.xml if there are string values
    std::map<tString, tInt> wSst_index;
    std::vector<tString> wSst_values;
    tBool wHasStringCells = false;
    for (auto& sheetPair : m_MapSheets) {
        for (auto& kv : sheetPair.second.m_CellValues) {
            if (detect_cell_type(kv.second) == CellType::String) { wHasStringCells = true; break; }
        }
        if (wHasStringCells) break;
    }
    
    if (wHasStringCells) {
        // Build frequency of string occurrences and unique list
        std::unordered_map<tString,tInt> freq;
        long long totalCount = 0;
        for (auto& sheetPair : m_MapSheets) {
            for (auto& kv : sheetPair.second.m_CellValues) {
                if (detect_cell_type(kv.second) == CellType::String) { freq[kv.second]++; totalCount++; }
            }
        }
        for (auto& kv : freq) {
            wSst_index[kv.first] = (tInt)wSst_values.size();
            wSst_values.push_back(kv.first);
        }
        pugi::xml_document doc;
        auto sst = doc.append_child("sst");
        sst.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
        sst.append_attribute("count").set_value(std::to_string(totalCount).c_str());
        sst.append_attribute("uniqueCount").set_value(std::to_string(wSst_values.size()).c_str());
        for (auto& val : wSst_values) {
            auto si = sst.append_child("si");
            auto t = si.append_child("t");
            t.append_attribute("xml:space").set_value("preserve");
            t.text().set(val.c_str());
        }
        if (!write_text_file(sRoot / "xl/sharedStrings.xml", to_string_xml(doc))) return false;
    }

    // 6b) xl/tables/tableN.xml (Excel ListObjects)
    for (tSize wTableIdx = 0; wTableIdx < this->m_Tables.size(); ++wTableIdx) {
        const auto& wTable = this->m_Tables[wTableIdx];
        pugi::xml_document wTableDoc;
        auto wTableNode = wTableDoc.append_child("table");
        wTableNode.append_attribute("xmlns").set_value(
            "http://schemas.openxmlformats.org/spreadsheetml/2006/main");
        wTableNode.append_attribute("id").set_value(std::to_string(wTableIdx + 1).c_str());
        wTableNode.append_attribute("name").set_value(wTable.name.c_str());
        {
            const tString wDisplayNameOut = wTable.displayName.empty()
                ? ExcelTableDisplayName(wTable.name)
                : wTable.displayName;
            wTableNode.append_attribute("displayName").set_value(wDisplayNameOut.c_str());
        }
        wTableNode.append_attribute("ref").set_value(wTable.ref.c_str());
        if (!wTable.hasHeaderRow) {
            wTableNode.append_attribute("headerRowCount").set_value("0");
        }
        if (wTable.totalsRowCount > 0) {
            wTableNode.append_attribute("totalsRowCount").set_value(
                std::to_string(wTable.totalsRowCount).c_str());
        } else {
            wTableNode.append_attribute("totalsRowShown").set_value("0");
        }
        if (wTable.hasAutoFilter) {
            auto wAutoFilter = wTableNode.append_child("autoFilter");
            wAutoFilter.append_attribute("ref").set_value(
                StructuredTableAutoFilterRef(wTable.ref, wTable.totalsRowCount).c_str());
            for (tSize wColIdx = 0; wColIdx < wTable.columns.size(); ++wColIdx) {
                if (!wTable.columns[wColIdx].filterButtonHidden) {
                    continue;
                }
                auto wFilterCol = wAutoFilter.append_child("filterColumn");
                wFilterCol.append_attribute("colId").set_value(std::to_string(wColIdx).c_str());
                wFilterCol.append_attribute("hiddenButton").set_value("1");
            }
        }
        auto wTableColumns = wTableNode.append_child("tableColumns");
        wTableColumns.append_attribute("count").set_value(std::to_string(wTable.columns.size()).c_str());
        tInt wColId = 1;
        for (const auto& wCol : wTable.columns) {
            auto wColNode = wTableColumns.append_child("tableColumn");
            wColNode.append_attribute("id").set_value(std::to_string(wColId++).c_str());
            wColNode.append_attribute("name").set_value(OoxmlTableColumnName(wCol.name).c_str());
            if (!wCol.totalsRowLabel.empty()) {
                wColNode.append_attribute("totalsRowLabel").set_value(wCol.totalsRowLabel.c_str());
            }
            if (!wCol.totalsRowFunction.empty()) {
                wColNode.append_attribute("totalsRowFunction").set_value(wCol.totalsRowFunction.c_str());
            }
            if (!wCol.calculatedFormula.empty()) {
                wColNode.append_child("calculatedColumnFormula").text().set(wCol.calculatedFormula.c_str());
            }
            if (!wCol.totalsRowFormula.empty()) {
                wColNode.append_child("totalsRowFormula").text().set(wCol.totalsRowFormula.c_str());
            }
        }
        auto wStyleInfo = wTableNode.append_child("tableStyleInfo");
        const tBool wHasStoredStyle = !wTable.styleElementCss.empty();
        const tBool wStyleFallback = !wHasStoredStyle && UsesCustomTableStyleFallback(wTable.styleName);
        const tString wStyleName = wHasStoredStyle
            ? wTable.styleName
            : ResolveExportTableStyleName(wTable.styleName);
        const tBool wExportRowStripes = wTable.showRowStripes;
        const tBool wExportColumnStripes = wTable.showColumnStripes;
        wStyleInfo.append_attribute("name").set_value(wStyleName.c_str());
        wStyleInfo.append_attribute("showFirstColumn").set_value(
            (!wStyleFallback && wTable.showFirstColumn) ? "1" : "0");
        wStyleInfo.append_attribute("showLastColumn").set_value(
            (!wStyleFallback && wTable.showLastColumn) ? "1" : "0");
        wStyleInfo.append_attribute("showRowStripes").set_value(
            (!wStyleFallback && wExportRowStripes) ? "1" : "0");
        wStyleInfo.append_attribute("showColumnStripes").set_value(
            (!wStyleFallback && wExportColumnStripes) ? "1" : "0");
        const tString wTablePath =
            "xl/tables/table" + std::to_string(wTableIdx + 1) + ".xml";
        const tString wTableXml = to_string_xml(wTableDoc);
        if (!write_text_file(sRoot / wTablePath, wTableXml)) {
            return false;
        }
    }

    // 7) Generate all worksheets
    tInt wSheetIndex = 1;
    const tSize wSheetTotal = this->m_SheetOrder.size();
    for (const tString& wSheetName : this->m_SheetOrder) {
        // Worksheet generation is the bulk of export: map it onto 10..90%.
        // Earlier passes (styles, shared strings, tables) count as the first
        // 10%, and packaging/zip closes to 100%.
        if (wSheetTotal > 0) {
            SkExcelReportProgress(
                10 + static_cast<int>((wSheetIndex - 1) * 80 / static_cast<tInt>(wSheetTotal)));
        }
        const auto wSheetIt = this->m_MapSheets.find(wSheetName);
        if (wSheetIt == this->m_MapSheets.end()) {
            continue;
        }
        const auto& wSheetData = wSheetIt->second;
        
		pugi::xml_document doc;
		auto ws = doc.append_child("worksheet");
		ws.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
        ws.append_attribute("xmlns:r").set_value(
            "http://schemas.openxmlformats.org/officeDocument/2006/relationships");
        ws.append_attribute("xmlns:mc").set_value(
            "http://schemas.openxmlformats.org/markup-compatibility/2006");
        ws.append_attribute("mc:Ignorable").set_value("x14ac");
        ws.append_attribute("xmlns:x14ac").set_value(
            "http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac");
        
        // Compute all addresses to emit (cells with values, formulas OR any style)
        std::set<tString> wEmitA1;
        for (auto& kv : wSheetData.m_CellValues) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellFormulas) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellFontColor) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellFontSpec) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellFillColor) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellBorderColor) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellBorderSides) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellNumberFormat) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellHorizAlign) wEmitA1.insert(kv.first);
        for (auto& kv : wSheetData.m_CellVertAlign) wEmitA1.insert(kv.first);
        for (auto& a : wSheetData.m_CellWrap) wEmitA1.insert(a);
        for (auto& kv : wSheetData.m_CellTextRotation) wEmitA1.insert(kv.first);

        // Add dimension (recommended)
        tInt wMinRow = INT_MAX, wMaxRow = 0, wMinCol = INT_MAX, wMaxCol = 0;
        for (const auto& a : wEmitA1) { tInt r=0,c=0; tString norm; if (Parse_cell_address(a, r, c, norm)) { wMinRow = std::min(wMinRow, r); wMaxRow = std::max(wMaxRow, r); wMinCol = std::min(wMinCol, c); wMaxCol = std::max(wMaxCol, c); } }
        {
            auto wDimenssion = ws.append_child("dimension");
            if (wEmitA1.empty()) {
                wDimenssion.append_attribute("ref").set_value("A1");
            } else {
                // Convert columns back to letters
                auto colToLetters = [](tInt col){ tString s; while (col>0){ tInt rem=(col-1)%26; s.insert(s.begin(), char('A'+rem)); col=(col-1)/26; } return s; };
                tString tl = colToLetters(wMinCol) + std::to_string(wMinRow);
                tString br = colToLetters(wMaxCol) + std::to_string(wMaxRow);
                tString ref = tl + ":" + br;
                wDimenssion.append_attribute("ref").set_value(ref.c_str());
            }
        }

        {
            auto wSheetViews = ws.append_child("sheetViews");
            auto wSheetView = wSheetViews.append_child("sheetView");
            wSheetView.append_attribute("workbookViewId").set_value("0");
            if (!wSheetData.m_ShowGridLines) {
                wSheetView.append_attribute("showGridLines").set_value("0");
            }
            if (wSheetIndex == 1) {
                wSheetView.append_attribute("tabSelected").set_value("1");
            }
            if (wSheetData.m_ZoomScaleNormal > 0 && wSheetData.m_ZoomScaleNormal != 100) {
                wSheetView.append_attribute("zoomScaleNormal")
                    .set_value(std::to_string(wSheetData.m_ZoomScaleNormal).c_str());
            } else {
                wSheetView.append_attribute("zoomScaleNormal").set_value("100");
            }
        }

        if (wSheetData.m_SheetDefaultRowHeightPt > 0.0 || wSheetData.m_SheetDefaultColWidthChars > 0.0) {
            auto wSheetFormat = ws.append_child("sheetFormatPr");
            wSheetFormat.append_attribute("baseColWidth").set_value("10");
            if (wSheetData.m_SheetDefaultColWidthChars > 0.0) {
                wSheetFormat.append_attribute("defaultColWidth")
                    .set_value(std::to_string(wSheetData.m_SheetDefaultColWidthChars).c_str());
            }
            if (wSheetData.m_SheetDefaultRowHeightPt > 0.0) {
                wSheetFormat.append_attribute("defaultRowHeight")
                    .set_value(FormatRowHeightXml(wSheetData.m_SheetDefaultRowHeightPt).c_str());
                // Excel ignores defaultRowHeight unless customHeight is set (falls back to ~15 pt).
                if (SheetUsesCustomDefaultRowHeight(wSheetData.m_SheetDefaultRowHeightPt)) {
                    wSheetFormat.append_attribute("customHeight").set_value("1");
                }
            }
        }

        // Add column definitions with widths
        if (!wSheetData.m_ColumnDimensions.m_ColumnWidths.empty()) {
            auto wCols = ws.append_child("cols");
            for (const auto& wColPair : wSheetData.m_ColumnDimensions.m_ColumnWidths) {
                auto wCol = wCols.append_child("col");
                wCol.append_attribute("min").set_value(std::to_string(wColPair.first).c_str());
                wCol.append_attribute("max").set_value(std::to_string(wColPair.first).c_str());
                wCol.append_attribute("width").set_value(std::to_string(wColPair.second).c_str());
                wCol.append_attribute("customWidth").set_value("1");
                if (wSheetData.m_ColumnDimensions.m_ColumnHidden.count(wColPair.first) > 0) {
                    wCol.append_attribute("hidden").set_value("1");
                }
            }
        }
        
        auto wSheetDataNode = ws.append_child("sheetData");
        if (!wEmitA1.empty()) {
			// Bucket cells by row
			std::map<tInt, std::vector<std::pair<tInt, tString>>> wRows; // row -> [(col, A1)]
            for (auto& key : wEmitA1) {
				tInt wIndexRow=0, wIndexCol=0; tString wA1;
                if (!Parse_cell_address(key, wIndexRow, wIndexCol, wA1)) continue;
				wRows[wIndexRow].push_back({wIndexCol, wA1});
			}
            for (const auto& wRowHt : wSheetData.m_RowDimensions.m_RowHeights) {
                wRows[wRowHt.first];
            }
            std::vector<tInt> wRowOrder;
            wRowOrder.reserve(wRows.size());
            for (const auto& wRowEntry : wRows) {
                wRowOrder.push_back(wRowEntry.first);
            }
            std::sort(wRowOrder.begin(), wRowOrder.end());
			for (tInt wRowIndex : wRowOrder) {
                auto& wCols = wRows[wRowIndex];
				std::sort(wCols.begin(), wCols.end(), [](auto& a, auto& b){ return a.first < b.first; });
                auto wRowNode = wSheetDataNode.append_child("row"); 
                wRowNode.append_attribute("r").set_value(std::to_string(wRowIndex).c_str());

                if (!wCols.empty()) {
                    tInt wSpanMinCol = wCols.front().first;
                    tInt wSpanMaxCol = wCols.back().first;
                    const tString wSpans =
                        std::to_string(wSpanMinCol) + ":" + std::to_string(wSpanMaxCol);
                    wRowNode.append_attribute("spans").set_value(wSpans.c_str());
                }
                
                // Add custom row height if specified
                auto wRowHeightIt = wSheetData.m_RowDimensions.m_RowHeights.find(wRowIndex);
                if (wRowHeightIt != wSheetData.m_RowDimensions.m_RowHeights.end()) {
                    wRowNode.append_attribute("ht")
                        .set_value(FormatRowHeightXml(wRowHeightIt->second).c_str());
                    wRowNode.append_attribute("customHeight").set_value("1");
                    wRowNode.append_attribute("x14ac:dyDescent").set_value("0.2");
                }
				for (auto& wEntry : wCols) {
					const tString& wA1 = wEntry.second;
                    auto wCnode = wRowNode.append_child("c");
                    wCnode.append_attribute("r").set_value(wA1.c_str());
                    // Resolve style index from (fontId, fillId, borderId, numFmtId)
                    tInt wFontId = 0; tInt wFillId = 0; tInt wBorderId = 0; tInt wNumFmtId = 0;
                    
                    // Check for complete font specification first
                    auto wItFontSpec = wSheetData.m_CellFontSpec.find(wA1);
                    if (wItFontSpec != wSheetData.m_CellFontSpec.end()) {
                        auto it = wFontIdBySpec.find(wItFontSpec->second);
                        if (it != wFontIdBySpec.end()) wFontId = it->second;
                    } else {
                        // Fallback to legacy font color
                        auto wItFontColor = wSheetData.m_CellFontColor.find(wA1); 
                        if (wItFontColor != wSheetData.m_CellFontColor.end()) { 
                            auto it = wFontIdByColor.find(wItFontColor->second); 
                            if (it != wFontIdByColor.end()) wFontId = it->second; 
                        }
                    }
                    
                    auto wItFillColor = wSheetData.m_CellFillColor.find(wA1); if (wItFillColor != wSheetData.m_CellFillColor.end()) { auto it2 = wFillIdByColor.find(wItFillColor->second); if (it2 != wFillIdByColor.end()) wFillId = it2->second; }
                    
                    auto wItNumberFormat = wSheetData.m_CellNumberFormat.find(wA1); if (wItNumberFormat != wSheetData.m_CellNumberFormat.end()) wNumFmtId = wItNumberFormat->second;
                    
                    tBool wHasBorder = false; tBorderKey bk{};
                    auto wItBorderSides = wSheetData.m_CellBorderSides.find(wA1);
                    if (wItBorderSides != wSheetData.m_CellBorderSides.end()) { const auto& s = wItBorderSides->second; bk = {s.left.style,s.left.argb, s.right.style,s.right.argb, s.top.style,s.top.argb, s.bottom.style,s.bottom.argb}; wHasBorder = true; }
                    else { auto wItBorderColor = wSheetData.m_CellBorderColor.find(wA1); if (wItBorderColor != wSheetData.m_CellBorderColor.end()) { tString thinStyle = normalize_border_style("thin"); bk = {thinStyle,wItBorderColor->second, thinStyle,wItBorderColor->second, thinStyle,wItBorderColor->second, thinStyle,wItBorderColor->second}; wHasBorder = true; } }
                    if (wHasBorder) { auto itK = wBorderIdByKey.find(wKeyToString(bk)); if (itK != wBorderIdByKey.end()) wBorderId = itK->second; }
                    
                    // Resolve alignment for this cell
                    tString wHAlign; tString wVAlign; tInt wWrap = 0; tInt wRotation = INT_MIN;
                    {
                        auto itH = wSheetData.m_CellHorizAlign.find(wA1); if (itH != wSheetData.m_CellHorizAlign.end()) wHAlign = itH->second;
                        auto itV = wSheetData.m_CellVertAlign.find(wA1); if (itV != wSheetData.m_CellVertAlign.end()) wVAlign = itV->second;
                        if (wSheetData.m_CellWrap.count(wA1)) wWrap = 1;
                        auto itR = wSheetData.m_CellTextRotation.find(wA1); if (itR != wSheetData.m_CellTextRotation.end()) wRotation = itR->second;
                    }
                    auto wIteratorXf = wXfIndexByCombo.find({wFontId, wFillId, wBorderId, wNumFmtId, wHAlign, wVAlign, wWrap, wRotation});
                    if (wIteratorXf != wXfIndexByCombo.end() && wIteratorXf->second != 0) wCnode.append_attribute("s").set_value(std::to_string(wIteratorXf->second).c_str());
                    // Emit formula and optional cached value
                    auto wItFormula = wSheetData.m_CellFormulas.find(wA1);
                    auto wItVal = wSheetData.m_CellValues.find(wA1);
                    if (wItFormula != wSheetData.m_CellFormulas.end()) {
                        auto wFnode = wCnode.append_child("f");
                        const tBool wHasCachedVal = wItVal != wSheetData.m_CellValues.end()
                            && !wItVal->second.empty();
                        if (wSheetData.m_TableColumnFormulas.count(wA1)) {
                            wFnode.append_attribute("ca").set_value("1");
                        }
                        wFnode.text().set(wItFormula->second.substr(1).c_str()); // Remove '=' prefix
                        if (wHasCachedVal) {
                            CellType wCellType = detect_cell_type(wItVal->second);
                            if (wCellType == CellType::String) {
                                wCnode.append_attribute("t").set_value("s");
                                tInt wIdx = wSst_index.count(wItVal->second) ? wSst_index[wItVal->second] : 0;
                                wCnode.append_child("v").text().set(std::to_string(wIdx).c_str());
                            } else if (wCellType == CellType::Number) {
                                wCnode.append_child("v").text().set(wItVal->second.c_str());
                            }
                        }
                    } else if (wItVal != wSheetData.m_CellValues.end() && !wItVal->second.empty()) {
                            CellType wCellType = detect_cell_type(wItVal->second);
                            if (wCellType == CellType::String) {
                        wCnode.append_attribute("t").set_value("s");
                                tInt wIdx = wSst_index.count(wItVal->second) ? wSst_index[wItVal->second] : 0;
                                wCnode.append_child("v").text().set(std::to_string(wIdx).c_str());
                            } else if (wCellType == CellType::Number) {
                                wCnode.append_child("v").text().set(wItVal->second.c_str());
                            } else if (wCellType == CellType::Formula) {
                                wCnode.append_child("f").text().set(wItVal->second.substr(1).c_str()); // Remove '=' prefix
                            }
                    }
                }
            }
        }
        
        // Add merged cells if any
        if (!wSheetData.m_MergedRanges.empty()) {
            auto wMergedCells = ws.append_child("mergeCells");
            wMergedCells.append_attribute("count").set_value(std::to_string(wSheetData.m_MergedRanges.size()).c_str());
            
            for (const auto& wRange : wSheetData.m_MergedRanges) {
                auto wMergeCell = wMergedCells.append_child("mergeCell");
                tString wRef = wRange.topLeft + ":" + wRange.bottomRight;
                wMergeCell.append_attribute("ref").set_value(wRef.c_str());
            }
        }
        
        // Add conditional formatting if any
        if (!wSheetData.m_ConditionalFormats.empty()) {
            tInt wDxfId = 0;
            
            for (const auto& wRule : wSheetData.m_ConditionalFormats) {
                // Create a separate conditionalFormatting element for each rule
                auto wConditionalFormatting = ws.append_child("conditionalFormatting");
                wConditionalFormatting.append_attribute("sqref").set_value(wRule.range.c_str());
                
                auto wCfRule = wConditionalFormatting.append_child("cfRule");
                wCfRule.append_attribute("type").set_value(wRule.type.c_str());
                wCfRule.append_attribute("priority").set_value(std::to_string(wRule.priority).c_str());
                
                // Only add dxfId for rules that use differential formatting
                if (wRule.type == "expression" || wRule.type == "cellIs") {
                    wCfRule.append_attribute("dxfId").set_value(std::to_string(wDxfId).c_str());
                }
                
                if (wRule.stopIfTrue) wCfRule.append_attribute("stopIfTrue").set_value("1");
                
                // Add operator for cellIs type
                if (wRule.type == "cellIs" && !wRule.operator_.empty()) {
                    wCfRule.append_attribute("operator").set_value(wRule.operator_.c_str());
                }
                
                // Handle icon sets
                if (wRule.type == "iconSet") {
                    auto wIconSet = wCfRule.append_child("iconSet");
                    if (!wRule.iconSet.empty()) {
                        wIconSet.append_attribute("iconSet").set_value(wRule.iconSet.c_str());
                    }
                    if (!wRule.showValue) {
                        wIconSet.append_attribute("showValue").set_value("0");
                    }
                    if (wRule.reverse) {
                        wIconSet.append_attribute("reverse").set_value("1");
                    }
                    
                    // Add custom thresholds if provided
                    if (!wRule.iconThresholds.empty()) {
                        for (tSize wIdx = 0; wIdx < wRule.iconThresholds.size(); ++wIdx) {
                            const auto& threshold = wRule.iconThresholds[wIdx];
                            auto wCfvo = wIconSet.append_child("cfvo");
                            tString wCfvoType = wRule.iconStyle;
                            if (wIdx < wRule.iconThresholdTypes.size() && !wRule.iconThresholdTypes[wIdx].empty()) {
                                wCfvoType = wRule.iconThresholdTypes[wIdx];
                            }
                            wCfvo.append_attribute("type").set_value(wCfvoType.c_str());
                            wCfvo.append_attribute("val").set_value(threshold.c_str());
                        }
                    } else {
                        // Use default thresholds based on icon set
                        if (wRule.iconSet == "3Arrows" || wRule.iconSet == "3TrafficLights1" || wRule.iconSet == "3Signs" || wRule.iconSet == "3Symbols" || wRule.iconSet == "3Symbols2") {
                            // 3-icon sets
                            auto wCfvo1 = wIconSet.append_child("cfvo");
                            wCfvo1.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo1.append_attribute("val").set_value("0");
                            
                            auto wCfvo2 = wIconSet.append_child("cfvo");
                            wCfvo2.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo2.append_attribute("val").set_value("33");
                            
                            auto wCfvo3 = wIconSet.append_child("cfvo");
                            wCfvo3.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo3.append_attribute("val").set_value("67");
                        } else if (wRule.iconSet == "4Arrows" || wRule.iconSet == "4TrafficLights" || wRule.iconSet == "4RedToBlack") {
                            // 4-icon sets
                            auto wCfvo1 = wIconSet.append_child("cfvo");
                            wCfvo1.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo1.append_attribute("val").set_value("0");
                            
                            auto wCfvo2 = wIconSet.append_child("cfvo");
                            wCfvo2.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo2.append_attribute("val").set_value("25");
                            
                            auto wCfvo3 = wIconSet.append_child("cfvo");
                            wCfvo3.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo3.append_attribute("val").set_value("50");
                            
                            auto wCfvo4 = wIconSet.append_child("cfvo");
                            wCfvo4.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo4.append_attribute("val").set_value("75");
                        } else if (wRule.iconSet == "5Arrows" || wRule.iconSet == "5ArrowsGray" || wRule.iconSet == "5Rating" || wRule.iconSet == "5Quarters" || wRule.iconSet == "5Boxes") {
                            // 5-icon sets
                            auto wCfvo1 = wIconSet.append_child("cfvo");
                            wCfvo1.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo1.append_attribute("val").set_value("0");
                            
                            auto wCfvo2 = wIconSet.append_child("cfvo");
                            wCfvo2.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo2.append_attribute("val").set_value("20");
                            
                            auto wCfvo3 = wIconSet.append_child("cfvo");
                            wCfvo3.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo3.append_attribute("val").set_value("40");
                            
                            auto wCfvo4 = wIconSet.append_child("cfvo");
                            wCfvo4.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo4.append_attribute("val").set_value("60");
                            
                            auto wCfvo5 = wIconSet.append_child("cfvo");
                            wCfvo5.append_attribute("type").set_value(wRule.iconStyle.c_str());
                            wCfvo5.append_attribute("val").set_value("80");
                        }
                    }
                } else if (wRule.type == "dataBar") {
                    auto wDataBar = wCfRule.append_child("dataBar");
                    auto wMinCfvo = wDataBar.append_child("cfvo");
                    if (wRule.dataBarMinVal.empty()) {
                        wMinCfvo.append_attribute("type").set_value("min");
                    } else {
                        wMinCfvo.append_attribute("type").set_value("num");
                        wMinCfvo.append_attribute("val").set_value(wRule.dataBarMinVal.c_str());
                    }
                    auto wMaxCfvo = wDataBar.append_child("cfvo");
                    if (wRule.dataBarMaxVal.empty()) {
                        wMaxCfvo.append_attribute("type").set_value("max");
                    } else {
                        wMaxCfvo.append_attribute("type").set_value("num");
                        wMaxCfvo.append_attribute("val").set_value(wRule.dataBarMaxVal.c_str());
                    }
                    if (!wRule.dataBarColor.empty()) {
                        tString wColor = wRule.dataBarColor;
                        if (wColor[0] == '#') {
                            wColor = wColor.substr(1);
                        }
                        if (wColor.size() == 6) {
                            wColor = "FF" + wColor;
                        }
                        wDataBar.append_child("color").append_attribute("rgb").set_value(wColor.c_str());
                    }
                } else if (wRule.type == "colorScale") {
                    auto wColorScale = wCfRule.append_child("colorScale");
                    const tSize wStopCount = std::min(wRule.colorScaleVals.size(), wRule.colorScaleColors.size());
                    for (tSize wStop = 0; wStop < wStopCount; ++wStop) {
                        auto wCfvo = wColorScale.append_child("cfvo");
                        wCfvo.append_attribute("type").set_value(
                            (wStop == 0) ? "min" : ((wStop + 1 == wStopCount) ? "max" : "percentile"));
                        if (!wRule.colorScaleVals[wStop].empty()) {
                            wCfvo.append_attribute("val").set_value(wRule.colorScaleVals[wStop].c_str());
                        }
                    }
                    for (tSize wStop = 0; wStop < wStopCount; ++wStop) {
                        tString wColor = wRule.colorScaleColors[wStop];
                        if (wColor[0] == '#') {
                            wColor = wColor.substr(1);
                        }
                        if (wColor.size() == 6) {
                            wColor = "FF" + wColor;
                        }
                        wColorScale.append_child("color").append_attribute("rgb").set_value(wColor.c_str());
                    }
                } else {
                    // Add formula for expression / cellIs rules
                    if (!wRule.formula.empty()) {
                        wCfRule.append_child("formula").text().set(wRule.formula.c_str());
                    }
                }
                
                // Only increment dxfId for expression / cellIs rules
                if (wRule.type == "expression" || wRule.type == "cellIs") {
                    wDxfId++;
                }
            }
        }
        
        auto wPageMargins = ws.append_child("pageMargins");
        wPageMargins.append_attribute("left").set_value("0.7");
        wPageMargins.append_attribute("right").set_value("0.7");
        wPageMargins.append_attribute("top").set_value("0.75");
        wPageMargins.append_attribute("bottom").set_value("0.75");
        wPageMargins.append_attribute("header").set_value("0.3");
        wPageMargins.append_attribute("footer").set_value("0.3");

        std::vector<tSize> wSheetTableIndices;
        for (tSize wTableIdx = 0; wTableIdx < this->m_Tables.size(); ++wTableIdx) {
            if (this->m_Tables[wTableIdx].sheetName == wSheetName) {
                wSheetTableIndices.push_back(wTableIdx);
            }
        }
        if (!wSheetTableIndices.empty()) {
            auto wTableParts = ws.append_child("tableParts");
            wTableParts.append_attribute("count").set_value(
                std::to_string(wSheetTableIndices.size()).c_str());
            for (tSize wLocalIdx = 0; wLocalIdx < wSheetTableIndices.size(); ++wLocalIdx) {
                auto wTablePart = wTableParts.append_child("tablePart");
                wTablePart.append_attribute("r:id").set_value(
                    ("rId" + std::to_string(wLocalIdx + 1)).c_str());
            }
            pugi::xml_document wSheetRelsDoc;
            auto wSheetRels = wSheetRelsDoc.append_child("Relationships");
            wSheetRels.append_attribute("xmlns").set_value(
                "http://schemas.openxmlformats.org/package/2006/relationships");
            for (tSize wLocalIdx = 0; wLocalIdx < wSheetTableIndices.size(); ++wLocalIdx) {
                const tSize wGlobalTableIdx = wSheetTableIndices[wLocalIdx];
                auto wRel = wSheetRels.append_child("Relationship");
                wRel.append_attribute("Id").set_value(
                    ("rId" + std::to_string(wLocalIdx + 1)).c_str());
                wRel.append_attribute("Type").set_value(
                    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/table");
                wRel.append_attribute("Target").set_value(
                    ("../tables/table" + std::to_string(wGlobalTableIdx + 1) + ".xml").c_str());
            }
            const tString wSheetRelsPath =
                "xl/worksheets/_rels/sheet" + std::to_string(wSheetIndex) + ".xml.rels";
            if (!write_text_file(sRoot / wSheetRelsPath, to_string_xml(wSheetRelsDoc))) {
                return false;
            }
        }

        // x14 sparklines must be last among worksheet children (extLst).
        if (!wSheetData.m_Sparklines.empty()) {
            auto wExtLst = ws.append_child("extLst");
            auto wExt = wExtLst.append_child("ext");
            // MS-XLSX 2.2.4.12: Excel only honors this exact sparklineGroups ext URI.
            wExt.append_attribute("xmlns:x14").set_value(
                "http://schemas.microsoft.com/office/spreadsheetml/2009/9/main");
            wExt.append_attribute("uri").set_value("{05C60535-1F16-4fd2-B633-F4F36F0B64E0}");
            auto wGroups = wExt.append_child("x14:sparklineGroups");
            wGroups.append_attribute("xmlns:xm").set_value(
                "http://schemas.microsoft.com/office/excel/2006/main");

            auto wWriteGroup = [&](tBool sMarkers) {
                tBool wAny = false;
                pugi::xml_node wSparks;
                for (const auto& wSpark : wSheetData.m_Sparklines) {
                    if (wSpark.markers != sMarkers) {
                        continue;
                    }
                    if (!wAny) {
                        auto wGroup = wGroups.append_child("x14:sparklineGroup");
                        wGroup.append_attribute("displayEmptyCellsAs").set_value("gap");
                        // 0.5 pt is closer to SkCellClassSparkline (1.5 px) than Excel's 0.75 default.
                        wGroup.append_attribute("lineWeight").set_value("0.5");
                        if (sMarkers) {
                            wGroup.append_attribute("markers").set_value("1");
                        }
                        auto wRgbColor = [&](const char* sName, const char* sRgb) {
                            auto wColor = wGroup.append_child(sName);
                            wColor.append_attribute("rgb").set_value(sRgb);
                        };
                        // Same greys as SkCellClassSparkline (line #A6A6A6, markers #7F7F7F).
                        wRgbColor("x14:colorSeries", "FFA6A6A6");
                        wRgbColor("x14:colorNegative", "FFD00000");
                        wRgbColor("x14:colorAxis", "FF000000");
                        wRgbColor("x14:colorMarkers", "FF7F7F7F");
                        wRgbColor("x14:colorFirst", "FF7F7F7F");
                        wRgbColor("x14:colorLast", "FF7F7F7F");
                        wRgbColor("x14:colorHigh", "FF7F7F7F");
                        wRgbColor("x14:colorLow", "FF7F7F7F");
                        wSparks = wGroup.append_child("x14:sparklines");
                        wAny = true;
                    }
                    auto wSparkNode = wSparks.append_child("x14:sparkline");
                    wSparkNode.append_child("xm:f").text().set(wSpark.sourceRange.c_str());
                    wSparkNode.append_child("xm:sqref").text().set(wSpark.cellRef.c_str());
                }
            };
            wWriteGroup(true);
            wWriteGroup(false);
        }

        if (!write_text_file(sRoot / ("xl/worksheets/sheet" + std::to_string(wSheetIndex) + ".xml"), to_string_xml(doc))) return false;
        wSheetIndex++;
	}

	return true;
}

// Normalize color string to ARGB 8 hex chars without '#'. Accepts #RRGGBB or #AARRGGBB
tBool normalize_hex_to_argb(const tString& sInput, tString& sOutARGB) {
	if (sInput.empty()) return false;
	tString wInput = sInput;
	if (wInput[0] == '#') wInput = wInput.substr(1);
	for (char& ch : wInput) ch = (char)std::toupper((unsigned char)ch);
	if (wInput.size() == 6) {
		// RRGGBB -> AARRGGBB with AA=FF
		sOutARGB = tString("FF") + wInput;
		return true;
	} else if (wInput.size() == 8) {
		sOutARGB = wInput;
 	return true;
	}
	return false;
}

// Validate and normalize border style to Excel-compatible values
tString normalize_border_style(const tString& sStyle) {
	if (sStyle.empty()) return "";
	
	// Convert to lowercase for comparison
	tString wStyle = sStyle;
	for (char& ch : wStyle) ch = (char)std::tolower((unsigned char)ch);
	
	// Excel supported border styles
	static const std::unordered_map<tString, tString> validStyles = {
		{"none", "none"},
		{"thin", "thin"},
		{"medium", "medium"},
		{"thick", "thick"},
		{"dashed", "dashed"},
		{"dotted", "dotted"},
		{"double", "double"},
		{"hair", "hair"},
		{"mediumdashed", "mediumDashed"},
		{"dashdot", "dashDot"},
		{"mediumdashdot", "mediumDashDot"},
		{"dashdotdot", "dashDotDot"},
		{"mediumdashdotdot", "mediumDashDotDot"},
		{"slantdashdot", "slantDashDot"}
	};
	
	auto it = validStyles.find(wStyle);
	if (it != validStyles.end()) {
		return it->second;
	}
	
	// Default to thin if unknown style
	return "thin";
}

// Detect cell value type for proper Excel formatting
CellType detect_cell_type(const tString& sValue) {
	if (sValue.empty()) return CellType::String;
	
	// Check for formula (starts with =)
	if (sValue[0] == '=') return CellType::Formula;
	
	// Check for number (basic detection)
	tBool wHasDigit = false;
	tBool wHasDecimal = false;
	
	for (tSize i = 0; i < sValue.length(); ++i) {
		char c = sValue[i];
		if (c >= '0' && c <= '9') {
			wHasDigit = true;
		} else if (c == '.' && !wHasDecimal) {
			wHasDecimal = true;
		} else if ((c == '+' || c == '-') && i == 0) {
			continue;
		} else if (c == 'e' || c == 'E') {
			// Scientific notation
			continue;
		} else {
			// Non-numeric character found
			return CellType::String;
		}
	}
	
	return wHasDigit ? CellType::Number : CellType::String;
}


// Detect if a value looks like a date
tBool is_date_value(const tString& sValue) {
	if (sValue.empty()) return false;
	
	// Check for common date patterns
	tString wLower = sValue;
	for (char& c : wLower) c = std::tolower(c);
	
	// Check for date separators
	if (sValue.find('/') != tString::npos || 
		sValue.find('-') != tString::npos || 
		sValue.find('.') != tString::npos) {
		
		// Check if it contains numbers and separators
		tBool wHasDigit = false;
		tBool wHasSeparator = false;
		for (char c : sValue) {
			if (c >= '0' && c <= '9') wHasDigit = true;
			if (c == '/' || c == '-' || c == '.') wHasSeparator = true;
		}
		return wHasDigit && wHasSeparator;
	}
	
	return false;
}

// Detect if a value looks like a percentage
tBool is_percentage_value(const tString& sValue) {
	if (sValue.empty()) return false;
	
	// Check if ends with %
	if (sValue.back() == '%') {
		// Check if the rest is numeric
		tString wNumPart = sValue.substr(0, sValue.length() - 1);
		tBool wHasDigit = false;
		for (char c : wNumPart) {
			if (c >= '0' && c <= '9') wHasDigit = true;
			else if (c != '.' && c != '+' && c != '-' && c != 'e' && c != 'E') {
				return false; // Invalid character
			}
		}
		return wHasDigit;
	}
	
	return false;
}

// Detect if a value looks like currency
tBool is_currency_value(const tString& sValue) {
    if (sValue.empty()) return false;

    const std::vector<tString> symbols = {"$", "€", "£", "¥"};
    auto wHasPrefixSymbol = [&](tString& symbolOut)->tBool{
        for (const auto& s : symbols) {
            if (sValue.rfind(s, 0) == 0) { symbolOut = s; return true; }
        }
        return false;
    };
    auto wHasSuffixSymbol = [&](tString& symbolOut)->tBool{
        for (const auto& s : symbols) {
            if (sValue.size() >= s.size() && sValue.compare(sValue.size()-s.size(), s.size(), s) == 0) {
                symbolOut = s; return true;
            }
        }
        return false;
    };

    tString sym;
    tString numPart;
    if (wHasPrefixSymbol(sym)) {
        numPart = sValue.substr(sym.size());
    } else if (wHasSuffixSymbol(sym)) {
        numPart = sValue.substr(0, sValue.size() - sym.size());
    } else {
        return false;
    }

    tBool hasDigit = false;
    for (char c : numPart) {
        if (c >= '0' && c <= '9') hasDigit = true;
        else if (c != '.' && c != ',' && c != '+' && c != '-' && c != 'e' && c != 'E') {
            return false; // Invalid character
        }
    }
    return hasDigit;
}

// Convert Excel column letters to 1-based index (A=1, Z=26, AA=27, ...)
tInt column_letters_to_index(const tString& sCol) {
	tInt wResult = 0;
	for (char ch : sCol) {
		if (ch >= 'A' && ch <= 'Z') wResult = wResult * 26 + (ch - 'A' + 1);
		else if (ch >= 'a' && ch <= 'z') wResult = wResult * 26 + (ch - 'a' + 1);
	}
	return wResult;
}

// Parse a cell address like "B12" tInto (row, col) 1-based. Returns false if invalid.
tBool Parse_cell_address(const tString& sAddr, tInt& sOutRow, tInt& sOutCol, tString& sOutA1) {
	if (sAddr.empty()) return false;
	tString wColLetters;
	tString wRowDigits;
	for (char ch : sAddr) {
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) wColLetters.push_back(ch);
		else if (ch >= '0' && ch <= '9') wRowDigits.push_back(ch);
		else return false;
	}
	if (wColLetters.empty() || wRowDigits.empty()) return false;
	sOutCol = column_letters_to_index(wColLetters);
	sOutRow = std::atoi(wRowDigits.c_str());
	if (sOutRow <= 0 || sOutCol <= 0) return false;
	// Normalize A1 reference (uppercase col)
	for (auto& c : wColLetters) c = (char)std::toupper(c);
	sOutA1 = wColLetters + wRowDigits;
    return true;
}

// Factory function
std::unique_ptr<WorkbookBuilder> CreateWorkbook() {
	return std::make_unique<WorkbookBuilder>();
}

// ExportToXlsx implementation for WorkbookBuilder
tBool SkExcel::WorkbookBuilder::ExportToXlsx(const tString& outputPath) {
    std::lock_guard<std::mutex> lock(m_Impl->mutex_);
    
    // Create temporary directory for OOXML files
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "skexcel_temp";
	std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);
    if (ec) return false;
    
    SkExcelReportProgress(0);

    // Generate OOXML files in temporary directory
    if (!m_Impl->build_ooxml_minimal(tempDir)) {
        std::filesystem::remove_all(tempDir, ec);
        return false;
    }
    SkExcelReportProgress(90);

    // Create zip file from temporary directory
    tBool success = zip_directory(tempDir, std::filesystem::path(outputPath));

    // Clean up temporary directory
    std::filesystem::remove_all(tempDir, ec);

    SkExcelReportProgress(100);
    return success;
}


namespace {

using SkSpreadSheet::Base10ToAlpha;
using SkSpreadSheet::IsSystemSheetName;
using SkSpreadSheet::tCell;
using SkSpreadSheet::tColRow;
using SkSpreadSheet::tColRowCellRange;
using SkSpreadSheet::tConditionalFormat;
using SkSpreadSheet::tConditionalFormatContainer;
using SkSpreadSheet::tConditionalFormatType;
using SkSpreadSheet::tIconType;
using SkSpreadSheet::tRange;
using SkSpreadSheet::tSheet;
using SkSpreadSheet::tWorkBook;
using SkRoot::tClassDate;
using SkRoot::tFormatString;
using SkRoot::tFormatStringType;
using SkRoot::tVariant;
using SkRoot::tVariantType;

static tString MakeCellRef(tIndex sRow, tIndex sCol) {
    return Base10ToAlpha(sCol) + std::to_string(sRow);
}

static tString RawHeaderCellText(tSheet* sSheet, tIndex sHeaderRow, tIndex sSheetCol) {
    if (sSheet == nullptr) {
        return "";
    }
    tCell* wCell = sSheet->Cell(sHeaderRow, sSheetCol);
    if (wCell == nullptr || wCell->Formula() != nullptr || wCell->Value().IsError()) {
        return "";
    }
    return wCell->Value().Str();
}

static tBool IsExportableTableFormula(const tString& sFormula) {
    if (sFormula.empty()) {
        return false;
    }
    return sFormula.find("#NAME?") == tString::npos && sFormula.find("#REF!") == tString::npos;
}

static tBool IsTableRowCounterColumnFormula(const tString& sTemplate) {
    if (sTemplate.find("[[#") != tString::npos) {
        return false;
    }
    return sTemplate.find("ROWS($") != tString::npos;
}

// tblTotals-style layout row: SUM of many tables' [#Totals] cells (not the column template).
static tBool IsMultiTableTotalsSumHelperFormula(const tString& sFormula) {
    if (sFormula.find("SUM(") == tString::npos) {
        return false;
    }
    tSize wTotalsRefs = 0;
    tSize wPos = 0;
    while ((wPos = sFormula.find("[[#Totals]", wPos)) != tString::npos) {
        ++wTotalsRefs;
        wPos += 11;
    }
    return wTotalsRefs >= 2;
}

// Layout tables (Tableau5/7) keep one-off cell formulas out of <calculatedColumnFormula>.
static tBool ShouldExportAsCalculatedColumnFormula(const tString& sFormula) {
    if (sFormula.empty()) {
        return false;
    }
    if (sFormula.find("[[#") != tString::npos) {
        return true;
    }
    return IsTableRowCounterColumnFormula(sFormula);
}

static tBool ShouldMarkTableColumnFormulaCa(const tString& sTemplate, tIndex sRow, tIndex sDataRow) {
    if (sTemplate.empty() || IsTableRowCounterColumnFormula(sTemplate)) {
        return false;
    }
    // First data row of "Solde d'ouverture": Excel omits ca on the seed row (MontantPrêt branch).
    if (sRow == sDataRow && sTemplate.find("MontantPrêt") != tString::npos) {
        return false;
    }
    return true;
}

static tString FindExportableTableColumnFormula(tApi& sApi,
                                                tSheet* sSheet,
                                                tIndex sFirstRow,
                                                tIndex sLastRow,
                                                tIndex sCol) {
    auto wReadFormula = [&](tIndex sRow) -> tString {
        tString wFormula = sApi.Formula(sRow, sCol, sSheet, false);
        if (wFormula.empty()) {
            tCell* wCell = sSheet->Cell(sRow, sCol);
            if (wCell != nullptr) {
                const tVariant wRaw = wCell->Value();
                if (wRaw.Type() == tVariantType::t_string && !wRaw.String().empty()
                    && wRaw.String().front() == '=') {
                    wFormula = wRaw.String();
                }
            }
        }
        if (!wFormula.empty() && wFormula.front() != '=') {
            wFormula = "=" + wFormula;
        }
        if (!IsExportableTableFormula(wFormula)) {
            return "";
        }
        return PrepareFormulaForExcelExport(
            wFormula,
            static_cast<tInt>(sRow),
            static_cast<tInt>(sCol),
            sApi,
            sSheet);
    };

    tString wFirst;
    tString wLast;
    tString wPreferredStructured;
    for (tIndex wRow = sFirstRow; wRow <= sLastRow; ++wRow) {
        tString wFormula = wReadFormula(wRow);
        if (wFormula.empty()) {
            continue;
        }
        if (wFirst.empty()) {
            wFirst = wFormula;
        }
        wLast = wFormula;
        if (wFormula.find("[[#") != tString::npos && !IsMultiTableTotalsSumHelperFormula(wFormula)) {
            wPreferredStructured = std::move(wFormula);
        }
    }

    if (!wPreferredStructured.empty()) {
        return wPreferredStructured;
    }
    if (!wLast.empty()) {
        if (IsTableRowCounterColumnFormula(wLast)) {
            return wFirst.empty() ? wLast : wFirst;
        }
        return wLast;
    }
    return wFirst;
}

static tString TrimCssToken(tString sValue) {
    while (!sValue.empty() && std::isspace(static_cast<unsigned char>(sValue.front()))) {
        sValue.erase(0, 1);
    }
    while (!sValue.empty() && std::isspace(static_cast<unsigned char>(sValue.back()))) {
        sValue.pop_back();
    }
    return sValue;
}

// Match a CSS property key at sPos (after optional ';' boundary). Rejects font-family/font-size/... when sKey is "font".
static tBool CssPropertyKeyAt(const tString& sCss, tSize sPos, const tString& sKey) {
    if (sPos + sKey.size() >= sCss.size()) {
        return false;
    }
    if (sCss.compare(sPos, sKey.size(), sKey) != 0) {
        return false;
    }
    if (sCss[sPos + sKey.size()] != ':') {
        return false;
    }
    if (sKey == "font" && sPos + 4 < sCss.size() && sCss[sPos + 4] == '-') {
        return false;
    }
    return true;
}

static tSize FindCssPropertyValueStart(const tString& sCss, const tString& sKey) {
    tSize wLast = tString::npos;
    tSize wPos = 0;
    while (wPos < sCss.size()) {
        if (wPos == 0 || sCss[wPos - 1] == ';') {
            if (CssPropertyKeyAt(sCss, wPos, sKey)) {
                wLast = wPos + sKey.size() + 1;
            }
        }
        tSize wSemi = sCss.find(';', wPos);
        if (wSemi == tString::npos) {
            break;
        }
        wPos = wSemi + 1;
    }
    return wLast;
}

static tString ExtractCssProperty(const tString& sCss, const tString& sKey) {
    const tSize wPos = FindCssPropertyValueStart(sCss, sKey);
    if (wPos == tString::npos) {
        return "";
    }
    tSize wEnd = sCss.find(';', wPos);
    if (wEnd == tString::npos) {
        wEnd = sCss.size();
    }
    return TrimCssToken(sCss.substr(wPos, wEnd - wPos));
}

static void StripBackgroundColorFromCss(tString& ioCss) {
    const tSize wPos = FindCssPropertyValueStart(ioCss, "background-color");
    if (wPos == tString::npos) {
        return;
    }
    tSize wStart = ioCss.rfind(';', wPos);
    wStart = (wStart == tString::npos) ? 0 : wStart + 1;
    tSize wEnd = ioCss.find(';', wPos);
    if (wEnd == tString::npos) {
        wEnd = ioCss.size();
    }
    ioCss.erase(wStart, wEnd - wStart);
}

static tString UnquoteCssValue(tString sValue) {
    sValue = TrimCssToken(sValue);
    if (sValue.size() >= 2 && sValue.front() == '"' && sValue.back() == '"') {
        return sValue.substr(1, sValue.size() - 2);
    }
    return sValue;
}

static tString ExtractCssQuotedProperty(const tString& sCss, const tString& sKey) {
    const tSize wPos = FindCssPropertyValueStart(sCss, sKey);
    if (wPos == tString::npos) {
        return "";
    }
    tSize wScan = wPos;
    while (wScan < sCss.size() && std::isspace(static_cast<unsigned char>(sCss[wScan]))) {
        ++wScan;
    }
    if (wScan >= sCss.size()) {
        return "";
    }
    // Quoted value may contain semicolons (e.g. format-string:"h:mm;@")
    if (sCss[wScan] == '"') {
        ++wScan;
        tString wOut;
        while (wScan < sCss.size() && sCss[wScan] != '"') {
            wOut.push_back(sCss[wScan++]);
        }
        return wOut;
    }
    tSize wEnd = sCss.find(';', wScan);
    if (wEnd == tString::npos) {
        wEnd = sCss.size();
    }
    return UnquoteCssValue(sCss.substr(wScan, wEnd - wScan));
}

static tString CssColorNameToHexRgb(const tString& sLowerName) {
    static const std::unordered_map<tString, tString> kNamed = {
        {"white", "FFFFFF"},
        {"black", "000000"},
        {"red", "FF0000"},
        {"green", "008000"},
        {"blue", "0000FF"},
        {"yellow", "FFFF00"},
        {"gray", "808080"},
        {"grey", "808080"},
        {"silver", "C0C0C0"},
        {"orange", "FFA500"},
        {"purple", "800080"},
        {"aliceblue", "F0F8FF"},
        {"lightblue", "ADD8E6"},
        {"lightgreen", "90EE90"},
        {"lightgray", "D3D3D3"},
        {"lightgrey", "D3D3D3"},
    };
    const auto wIt = kNamed.find(sLowerName);
    return wIt != kNamed.end() ? wIt->second : "";
}

static tBool NormalizeCssColorToHexHash(const tString& sInput, tString& oOutHash) {
    tString wToken = TrimCssToken(sInput);
    if (wToken.empty()) {
        return false;
    }
    tString wArgb;
    if (normalize_hex_to_argb(wToken, wArgb)) {
        oOutHash = "#" + wArgb.substr(wArgb.size() >= 6 ? wArgb.size() - 6 : 0);
        return true;
    }
    tString wLower = wToken;
    for (char& wCh : wLower) {
        wCh = static_cast<char>(std::tolower(static_cast<unsigned char>(wCh)));
    }
    const tString wRgb = CssColorNameToHexRgb(wLower);
    if (!wRgb.empty()) {
        oOutHash = "#" + wRgb;
        return true;
    }
    return false;
}

static tBool ParseCssBorderSide(const tString& sCssSide, tString& oStyle, tString& oColor) {
    tString wSide = TrimCssToken(sCssSide);
    if (wSide.empty()) {
        return false;
    }
    tSize wHash = wSide.rfind('#');
    if (wHash != tString::npos) {
        oColor = wSide.substr(wHash);
        wSide = TrimCssToken(wSide.substr(0, wHash));
    } else {
        const tSize wLastSpace = wSide.find_last_of(" \t");
        if (wLastSpace != tString::npos) {
            const tString wLastToken = TrimCssToken(wSide.substr(wLastSpace + 1));
            tString wHexHash;
            if (NormalizeCssColorToHexHash(wLastToken, wHexHash)) {
                oColor = wHexHash;
                wSide = TrimCssToken(wSide.substr(0, wLastSpace));
            }
        }
    }
    if (wSide.find("double") != tString::npos) {
        oStyle = "double";
    } else if (wSide.find("dashed") != tString::npos) {
        oStyle = "dashed";
    } else if (wSide.find("dotted") != tString::npos) {
        oStyle = "dotted";
    } else if (wSide.find("medium") != tString::npos) {
        oStyle = "medium";
    } else if (wSide.find("thick") != tString::npos) {
        oStyle = "thick";
    } else {
        oStyle = "thin";
    }
    return true;
}

static tString SkRootFormatKeyToExcelFormat(const tString& sKey) {
    tString wKey = TrimCssToken(sKey);
    while (!wKey.empty() && wKey.front() == '"') {
        wKey.erase(0, 1);
    }
    while (!wKey.empty() && wKey.back() == '"') {
        wKey.pop_back();
    }
    if (wKey.empty() || wKey == "General") {
        return "";
    }

    tFormatString wFmt;
    if (wFmt.ExcelFormat(wKey)) {
        return wFmt.FormatExcel();
    }

    wFmt.Clear();
    wFmt.Format(wKey);
    if (wFmt.FormatType() != tFormatStringType::none) {
        const tString wExcel = wFmt.ExcelEquivalent("");
        if (!wExcel.empty() && wExcel != "None") {
            return wExcel;
        }
    }

    return wKey;
}

static tInt CssTextRotationToExcel(tInt sCssDegrees) {
    if (sCssDegrees == 255) {
        return 255;
    }
    if (sCssDegrees < 0) {
        return 90 - sCssDegrees;
    }
    return sCssDegrees;
}

static tString CssTokenAt(const tString& sBody, tSize& ioPos) {
    while (ioPos < sBody.size() && std::isspace(static_cast<unsigned char>(sBody[ioPos]))) {
        ++ioPos;
    }
    if (ioPos >= sBody.size()) {
        return "";
    }
    const tSize wStart = ioPos;
    while (ioPos < sBody.size() && !std::isspace(static_cast<unsigned char>(sBody[ioPos]))) {
        ++ioPos;
    }
    return sBody.substr(wStart, ioPos - wStart);
}

// SkRoot CSS font shorthand: font:"Tahoma",serif 30pt bold;
static void ApplySkRootFontShorthand(WorkbookBuilder& sWorkbook, const tString& sCell, const tString& sFontBody) {
    tString wBody = TrimCssToken(sFontBody);
    if (wBody.empty()) {
        return;
    }

    tSize wPos = 0;
    tString wName;
    if (wPos < wBody.size() && wBody[wPos] == '"') {
        ++wPos;
        while (wPos < wBody.size() && wBody[wPos] != '"') {
            wName.push_back(wBody[wPos++]);
        }
        if (wPos < wBody.size() && wBody[wPos] == '"') {
            ++wPos;
        }
    }

    if (wPos < wBody.size() && wBody[wPos] == ',') {
        ++wPos;
        while (wPos < wBody.size() && !std::isspace(static_cast<unsigned char>(wBody[wPos]))) {
            ++wPos;
        }
    }

    while (wPos < wBody.size()) {
        const tString wToken = CssTokenAt(wBody, wPos);
        if (wToken.empty()) {
            break;
        }
        if (wToken.size() >= 2 && wToken.substr(wToken.size() - 2) == "pt") {
            const tInt wSize = std::atoi(wToken.c_str());
            if (wSize > 0) {
                sWorkbook.SetCellFontSize(sCell, wSize);
            }
        } else if (wToken == "bold" || wToken == "bolder") {
            sWorkbook.SetCellFontBold(sCell, true);
        } else if (wToken == "italic" || wToken == "oblique") {
            sWorkbook.SetCellFontItalic(sCell, true);
        }
    }

    if (!wName.empty()) {
        sWorkbook.SetCellFontName(sCell, wName);
    }
}

// Materialize table-style overlay via tTableStyleContainer (built-in + custom).
static tString TableStyleOverlayCssForExport(tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow,
                                             tIndex sCol) {
    if (sWorkBook == nullptr || sSheet == nullptr) {
        return "";
    }
    return sWorkBook->TableStyleOverlayCss(sSheet, sRow, sCol);
}

static void MergeTableStyleCssOnExport(tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                                       tString& ioCss) {
    const tString wTableCss = TableStyleOverlayCssForExport(sWorkBook, sSheet, sRow, sCol);
    if (wTableCss.empty()) {
        return;
    }
    if (wTableCss.find("background-color") != tString::npos) {
        StripBackgroundColorFromCss(ioCss);
    }
    ioCss += wTableCss;
}
static void ApplyCssToWorkbookCell(WorkbookBuilder& sWorkbook, const tString& sCell, const tString& sCss,
                                   tBool sSkipTableFill) {
    if (sCss.empty()) {
        return;
    }

    tString wCss = sCss;
    if (!CssDeclaresFont(wCss)) {
        // Border-only / fill-only CSS must inherit the workbook default font (OOXML fonts[0]).
        wCss = PrependDefaultFontCss(wCss, sWorkbook.DefaultFontName(), sWorkbook.DefaultFontSize());
    }

    const tString wFontShorthand = ExtractCssProperty(wCss, "font");
    if (!wFontShorthand.empty()) {
        ApplySkRootFontShorthand(sWorkbook, sCell, wFontShorthand);
    }

    tString wFontFamily = ExtractCssQuotedProperty(sCss, "font-family");
    if (!wFontFamily.empty()) {
        tSize wComma = wFontFamily.find(',');
        if (wComma != tString::npos) {
            wFontFamily = TrimCssToken(wFontFamily.substr(0, wComma));
        }
        sWorkbook.SetCellFontName(sCell, wFontFamily);
    }

    tString wFontSize = ExtractCssProperty(sCss, "font-size");
    if (!wFontSize.empty() && wFontSize.size() > 2 && wFontSize.substr(wFontSize.size() - 2) == "pt") {
        const tInt wSize = std::atoi(wFontSize.c_str());
        if (wSize > 0) {
            sWorkbook.SetCellFontSize(sCell, wSize);
        }
    }

    if (ExtractCssProperty(sCss, "font-weight") == "bold") {
        sWorkbook.SetCellFontBold(sCell, true);
    }
    if (ExtractCssProperty(sCss, "font-style") == "italic") {
        sWorkbook.SetCellFontItalic(sCell, true);
    }
    if (ExtractCssProperty(sCss, "text-decoration-line").find("underline") != tString::npos) {
        sWorkbook.SetCellFontUnderline(sCell, true);
    }

    tString wColor = ExtractCssProperty(sCss, "color");
    if (!wColor.empty()) {
        tString wHexHash;
        if (NormalizeCssColorToHexHash(wColor, wHexHash)) {
            sWorkbook.SetCellFontColor(sCell, wHexHash);
        } else {
            sWorkbook.SetCellFontColor(sCell, wColor);
        }
    }

    tString wFill = ExtractCssProperty(sCss, "background-color");
    if (!sSkipTableFill && !wFill.empty()) {
        tString wHexHash;
        if (NormalizeCssColorToHexHash(wFill, wHexHash)) {
            sWorkbook.SetCellFillColor(sCell, wHexHash);
        } else {
            sWorkbook.SetCellFillColor(sCell, wFill);
        }
    }

    tString wLeftStyle, wLeftColor, wRightStyle, wRightColor, wTopStyle, wTopColor, wBottomStyle, wBottomColor;
    const tBool wHasLeft = ParseCssBorderSide(ExtractCssProperty(sCss, "border-left"), wLeftStyle, wLeftColor);
    const tBool wHasRight = ParseCssBorderSide(ExtractCssProperty(sCss, "border-right"), wRightStyle, wRightColor);
    const tBool wHasTop = ParseCssBorderSide(ExtractCssProperty(sCss, "border-top"), wTopStyle, wTopColor);
    const tBool wHasBottom = ParseCssBorderSide(ExtractCssProperty(sCss, "border-bottom"), wBottomStyle, wBottomColor);
    if (wHasLeft || wHasRight || wHasTop || wHasBottom) {
        sWorkbook.SetCellBorderSides(
            sCell,
            wHasLeft ? wLeftStyle : "", wHasLeft ? wLeftColor : "",
            wHasRight ? wRightStyle : "", wHasRight ? wRightColor : "",
            wHasTop ? wTopStyle : "", wHasTop ? wTopColor : "",
            wHasBottom ? wBottomStyle : "", wHasBottom ? wBottomColor : "");
    }

    tString wAlignH = ExtractCssProperty(sCss, "text-align");
    tString wAlignV = ExtractCssProperty(sCss, "vertical-align");
    if (wAlignV == "middle") {
        wAlignV = "center";
    }
    const tBool wWrap = ExtractCssProperty(sCss, "text-wrap") == "wrap";
    if (!wAlignH.empty() || !wAlignV.empty() || wWrap) {
        sWorkbook.SetCellAlignment(
            sCell,
            wAlignH.empty() ? "general" : wAlignH,
            wAlignV.empty() ? "bottom" : wAlignV,
            wWrap);
    }

    tString wRotate = ExtractCssProperty(sCss, "text-rotate");
    if (!wRotate.empty()) {
        sWorkbook.SetCellTextRotation(sCell, CssTextRotationToExcel(std::atoi(wRotate.c_str())));
    }

    tString wFormat = ExtractCssQuotedProperty(sCss, "format-string");
    if (!wFormat.empty()) {
        const tString wExcelFormat = SkRootFormatKeyToExcelFormat(wFormat);
        if (!wExcelFormat.empty()) {
            sWorkbook.SetCellNumberFormat(sCell, wExcelFormat);
        }
    }
}

static tInt EstimateExportMdw(tApi& sApi, tSheet* sSheet) {
    tString wFontName = "Calibri";
    tDouble wSizePt = 11.0;
    if (sSheet != nullptr) {
        if (tWorkBook* wBook = sSheet->WorkBook()) {
            if (!wBook->DefaultFontName().empty()) {
                wFontName = wBook->DefaultFontName();
            }
            if (wBook->DefaultFontSize() > 0.0) {
                wSizePt = wBook->DefaultFontSize();
            }
        }
    }
    if (wFontName == "Calibri" && wSizePt == 11.0) {
        const tIndex wLastRow = std::min<tIndex>(sSheet != nullptr ? sSheet->LastRow() : 0, 50);
        const tIndex wLastCol = std::min<tIndex>(sSheet != nullptr ? sSheet->LastCol() : 0, 20);
        for (tIndex wRow = 1; wRow <= wLastRow && wFontName == "Calibri"; ++wRow) {
            for (tIndex wCol = 1; wCol <= wLastCol; ++wCol) {
                const tString wCss = sApi.CellFormat(wRow, wCol, sSheet);
                if (wCss.empty()) {
                    continue;
                }
                const tSize wFontPos = wCss.find("font:\"");
                if (wFontPos == tString::npos) {
                    continue;
                }
                const tSize wEndQuote = wCss.find('"', wFontPos + 6);
                if (wEndQuote == tString::npos) {
                    continue;
                }
                tDouble wCellSizePt = 11.0;
                const tSize wPtPos = wCss.find("pt", wEndQuote);
                if (wPtPos != tString::npos) {
                    tSize wStart = wPtPos;
                    while (wStart > wEndQuote && (std::isdigit(static_cast<unsigned char>(wCss[wStart - 1]))
                           || wCss[wStart - 1] == '.')) {
                        --wStart;
                    }
                    if (wStart < wPtPos) {
                        wCellSizePt = std::atof(wCss.substr(wStart, wPtPos - wStart).c_str());
                    }
                }
                if (wCellSizePt > 14.0) {
                    continue;
                }
                wFontName = wCss.substr(wFontPos + 6, wEndQuote - (wFontPos + 6));
                break;
            }
        }
    }
    return EstimateMaxDigitWidthPxFromFont(wFontName, wSizePt);
}

static tString InferWorkbookDefaultFontName(tApi& sApi, tWorkBook* sWorkBook) {
    if (sWorkBook == nullptr) {
        return "Calibri";
    }
    if (!sWorkBook->DefaultFontName().empty() && sWorkBook->DefaultFontName() != "Calibri") {
        return sWorkBook->DefaultFontName();
    }
    // Legacy workbooks without defaultfontname in .sker JSON: infer from body cell CSS.
    for (tSheet* wSheet : sWorkBook->VectorPtSheet()) {
        if (IsSystemSheetName(wSheet->Name())) {
            continue;
        }
        const tIndex wLastRow = std::min<tIndex>(wSheet->LastRow(), 50);
        const tIndex wLastCol = std::min<tIndex>(wSheet->LastCol(), 20);
        for (tIndex wRow = 1; wRow <= wLastRow; ++wRow) {
            for (tIndex wCol = 1; wCol <= wLastCol; ++wCol) {
                const tString wCss = sApi.CellFormat(wRow, wCol, wSheet);
                if (wCss.empty()) {
                    continue;
                }
                const tString wFamily = ExtractCssQuotedProperty(wCss, "font-family");
                if (!wFamily.empty()) {
                    tString wName = TrimCssToken(wFamily);
                    const tSize wComma = wName.find(',');
                    if (wComma != tString::npos) {
                        wName = TrimCssToken(wName.substr(0, wComma));
                    }
                    if (!wName.empty()) {
                        return wName;
                    }
                }
                const tSize wFontPos = wCss.find("font:\"");
                if (wFontPos == tString::npos) {
                    continue;
                }
                const tSize wEndQuote = wCss.find('"', wFontPos + 6);
                if (wEndQuote == tString::npos) {
                    continue;
                }
                tDouble wSizePt = 11.0;
                const tSize wPtPos = wCss.find("pt", wEndQuote);
                if (wPtPos != tString::npos) {
                    tSize wStart = wPtPos;
                    while (wStart > wEndQuote && (std::isdigit(static_cast<unsigned char>(wCss[wStart - 1]))
                           || wCss[wStart - 1] == '.')) {
                        --wStart;
                    }
                    if (wStart < wPtPos) {
                        wSizePt = std::atof(wCss.substr(wStart, wPtPos - wStart).c_str());
                    }
                }
                if (wSizePt > 14.0) {
                    continue;
                }
                return wCss.substr(wFontPos + 6, wEndQuote - (wFontPos + 6));
            }
        }
    }
    return sWorkBook->DefaultFontName().empty() ? "Calibri" : sWorkBook->DefaultFontName();
}

static tDouble RoundExcelRowHeightPt(tDouble sPt) {
    if (sPt <= 0.0) {
        return 0.0;
    }
    return std::round(sPt * 1000.0) / 1000.0;
}

static tDouble SnapExcelRowHeightPt(tDouble sPt, tIndex /*sRow*/) {
    return RoundExcelRowHeightPt(sPt);
}

static tInt ExtractCssFontSizePt(const tString& sCss) {
    const tSize wFontPos = sCss.find("font:\"");
    if (wFontPos == tString::npos) {
        return 0;
    }
    const tSize wPtPos = sCss.find("pt", wFontPos);
    if (wPtPos == tString::npos || wPtPos <= wFontPos) {
        return 0;
    }
    tSize wStart = wPtPos;
    while (wStart > wFontPos && std::isdigit(static_cast<unsigned char>(sCss[wStart - 1]))) {
        --wStart;
    }
    if (wStart == wPtPos) {
        return 0;
    }
    return std::atoi(sCss.substr(wStart, wPtPos - wStart).c_str());
}

static tString CellFormatDirect(tApi& sApi, tSheet* sSheet, tIndex sRow, tIndex sCol) {
    tCell* wCell = sSheet->Cell(sRow, sCol);
    tString wCellCss;
    if (wCell != nullptr && wCell->Css() != 0) {
        if (tWorkBook* wBook = sSheet->WorkBook()) {
            wCellCss = wBook->CellFormat(wCell->Css());
        }
    }
    const tString wApiCss = sApi.CellFormat(sRow, sCol, sSheet);
    if (wApiCss.empty()) {
        return wCellCss;
    }
    if (wCellCss.empty()) {
        return wApiCss;
    }
    // Row/col cascades can mask an explicit cell fill (e.g. G1 fo:3 vs border-only col css).
    const tBool wApiHasBg = wApiCss.find("background-color") != tString::npos;
    const tBool wCellHasBg = wCellCss.find("background-color") != tString::npos;
    if (wCellHasBg && !wApiHasBg) {
        return wCellCss;
    }
    if (wCellHasBg && wApiHasBg) {
        return wCellCss;
    }
    return wApiCss;
}

static tString PickBestCssInMerge(tApi& sApi, tSheet* sSheet, tRange* sMerged) {
    tString wBest;
    tInt wBestFontPt = 0;
    tBool wBestHasBg = false;
    for (tIndex wRow = sMerged->TopIndex(); wRow <= sMerged->BottomIndex(); ++wRow) {
        for (tIndex wCol = sMerged->LeftIndex(); wCol <= sMerged->RightIndex(); ++wCol) {
            const tString wPart = CellFormatDirect(sApi, sSheet, wRow, wCol);
            if (wPart.empty()) {
                continue;
            }
            const tBool wHasBg = wPart.find("background-color") != tString::npos;
            const tInt wFontPt = ExtractCssFontSizePt(wPart);
            if (wBest.empty()
                || (wHasBg && !wBestHasBg)
                || (wHasBg == wBestHasBg && wFontPt > wBestFontPt)) {
                wBest = wPart;
                wBestHasBg = wHasBg;
                wBestFontPt = wFontPt;
            }
        }
    }
    return wBest;
}

static tString CssForExportCell(tApi& sApi, tSheet* sSheet, tIndex sRow, tIndex sCol) {
    tString wCss = CellFormatDirect(sApi, sSheet, sRow, sCol);
    tRange* wMerged = sSheet->MergedRange(sRow, sCol);
    if (wMerged == nullptr || !wMerged->IsMerged()) {
        return wCss;
    }
    const tString wMergeCss = PickBestCssInMerge(sApi, sSheet, wMerged);
    if (!wMergeCss.empty()) {
        return wMergeCss;
    }
    return wCss;
}

static tBool ShouldSkipDecoratedEmptyCell(tCell* sCell, tBool sHasContent, const tString& sCss) {
    if (sHasContent || sCell == nullptr || sCss.empty()) {
        return false;
    }
    if (!sCell->IsEmpty() && !sCell->IsValueEmpty()) {
        return false;
    }
    if (sCss.find("font:\"") != tString::npos || sCss.find("background-color") != tString::npos) {
        return false;
    }
    // G1-style orphan: border-left only, no value (merge blocks are elsewhere).
    return sCss.find("border-left") != tString::npos
        && sCss.find("border-right") == tString::npos
        && sCss.find("border-top") == tString::npos
        && sCss.find("border-bottom") == tString::npos;
}

static void ExportSheetDimensions(tApi& sApi, tSheet* sSheet, WorkbookBuilder& sWorkbook) {
    const tIndex wLastRow = sSheet->LastRow();
    const tIndex wLastCol = sSheet->LastCol();
    if (wLastRow <= 0 || wLastCol <= 0) {
        return;
    }

    // Must match import MDW (workbook default font, e.g. Verdana -> 8).
    const tInt wMdw = EstimateExportMdw(sApi, sSheet);

    tDouble wDefaultRowPt = 0.0;
    if (tWorkBook* wWorkBook = sSheet->WorkBook()) {
        wDefaultRowPt =
            SnapExcelRowHeightPt(SkMmToExcelRowHeightPt(wWorkBook->DefaultSizeRow()), 0);
    }

    const tBool wCustomSheetRowDefault = SheetUsesCustomDefaultRowHeight(wDefaultRowPt);

    for (tIndex wRow = 1; wRow <= wLastRow; ++wRow) {
        const tDouble wPt = SnapExcelRowHeightPt(
            SkMmToExcelRowHeightPt(sApi.SizeRow(wRow, sSheet)), wRow);
        if (wPt <= 0.0) {
            continue;
        }
        const tBool wMatchesDefault =
            wDefaultRowPt > 0.0 && std::abs(wPt - wDefaultRowPt) < 0.05;
        // Standard sheets: omit per-row ht when it matches the workbook default.
        // Custom defaults (e.g. Horaires 30 pt): emit ht on every row like Excel templates.
        if (wMatchesDefault && !wCustomSheetRowDefault) {
            continue;
        }
        sWorkbook.SetRowHeight(static_cast<tInt>(wRow), wPt);
    }
    for (tIndex wCol = 1; wCol <= wLastCol; ++wCol) {
        const tDouble wMm = sApi.SizeCol(wCol, sSheet);
        if (wMm <= 0.0) {
            // Size 0 is Sker's hidden-column flag (Excel hidden="1"). Keep a default
            // width in OOXML so Excel can restore the column on unhide.
            sWorkbook.SetColumnHidden(static_cast<tInt>(wCol));
            sWorkbook.SetColumnWidth(static_cast<tInt>(wCol), 8.43);
            continue;
        }
        const tDouble wChars = SkMmToExcelColWidthChars(wMm, wMdw);
        if (wChars > 0.0) {
            sWorkbook.SetColumnWidth(static_cast<tInt>(wCol), wChars);
        }
    }

    if (tWorkBook* wWorkBook = sSheet->WorkBook()) {
        if (wDefaultRowPt > 0.0) {
            sWorkbook.SetSheetDefaultRowHeight(wDefaultRowPt);
        }
        const tDouble wDefaultColChars =
            SkMmToExcelColWidthChars(wWorkBook->DefaultSizeCol(), wMdw);
        if (wDefaultColChars > 0.0) {
            sWorkbook.SetSheetDefaultColWidth(wDefaultColChars);
        }
    }
}

static tString BuildDefinedNameBodyForRange(tRange* sRange, const tString& sName) {
    if (sRange == nullptr || sRange->Sheet() == nullptr) {
        return "";
    }
    if (IsSystemSheetName(sRange->Sheet()->Name())) {
        return "";
    }
    if (sName.find("Print_Titles") != tString::npos
        && sRange->TopIndex() == sRange->BottomIndex()
        && sRange->LeftIndex() == sRange->RightIndex()) {
        const tString wRow = std::to_string(sRange->TopIndex());
        return QuoteExcelSheetName(sRange->Sheet()->Name()) + "!$" + wRow + ":$" + wRow;
    }
    return FormatSheetQualifiedAbsRef(sRange->Sheet()->Name(), sRange->StrRef(false));
}

static std::map<tString, tInt> BuildExportSheetLocalIdMap(tWorkBook* sWorkBook) {
    std::map<tString, tInt> wMap;
    if (sWorkBook == nullptr) {
        return wMap;
    }
    tInt wIdx = 0;
    for (tSheet* wSheet : sWorkBook->VectorPtSheet()) {
        if (wSheet == nullptr || IsSystemSheetName(wSheet->Name())) {
            continue;
        }
        wMap[wSheet->Name()] = wIdx++;
    }
    return wMap;
}

// Workbook-global names are visible on every sheet; only _xlnm.* stay sheet-local (Print_Area, etc.).
static tInt ResolveDefinedNameLocalSheetId(const tString& sName,
                                           const std::vector<tRange*>& sRanges,
                                           const std::map<tString, tInt>& sSheetLocalIds) {
    if (sName.size() >= 6 && sName.compare(0, 6, "_xlnm.") == 0) {
        for (tRange* wRange : sRanges) {
            if (wRange != nullptr && wRange->Sheet() != nullptr) {
                const auto wIt = sSheetLocalIds.find(wRange->Sheet()->Name());
                if (wIt != sSheetLocalIds.end()) {
                    return wIt->second;
                }
            }
        }
    }
    return -1;
}

static tBool LooksLikeExcelDefinedNameRefList(const tString& sBody) {
    if (sBody.empty() || sBody.front() == '=') {
        return false;
    }
    if (sBody.find('!') == tString::npos) {
        return false;
    }
  // Union refs and simple sheet refs — not formula expressions.
    return sBody.find('(') == tString::npos && sBody.find('{') == tString::npos;
}

static tString PrepareDefinedNameFormulaForExcelExport(tString sFormula, tApi& sApi) {
    if (LooksLikeExcelDefinedNameRefList(sFormula)) {
        return sFormula;
    }
    if (!sFormula.empty() && sFormula.front() == '=') {
        sFormula.erase(0, 1);
    }
    return PrepareFormulaForExcelExport(sFormula, 0, 0, sApi, nullptr);
}

static void ExportWorkbookDefinedNames(tWorkBook* sWorkBook, WorkbookBuilder& sWorkbook, tApi& sApi) {
    if (sWorkBook == nullptr) {
        return;
    }
    SkSpreadSheet::tRangeNamedContainer* wNamedContainer = sWorkBook->RangeNamedContainer();
    if (wNamedContainer == nullptr) {
        return;
    }

    const std::map<tString, tInt> wSheetLocalIds = BuildExportSheetLocalIdMap(sWorkBook);

    for (const tString& wName : wNamedContainer->AllNames()) {
        const std::vector<tRange*> wRanges = wNamedContainer->Ranges(wName);
        tString wBody;
        const tInt wLocalSheetId =
            ResolveDefinedNameLocalSheetId(wName, wRanges, wSheetLocalIds);
        for (tRange* wRange : wRanges) {
            if (wRange != nullptr && wRange->IsData()) {
                continue;
            }
            const tString wPart = BuildDefinedNameBodyForRange(wRange, wName);
            if (wPart.empty()) {
                continue;
            }
            if (!wBody.empty()) {
                wBody += ",";
            }
            wBody += wPart;
        }
        if (!wBody.empty()) {
            sWorkbook.AddDefinedName(wName, wBody, wLocalSheetId);
        }
    }

    for (const tString& wName : wNamedContainer->AllFormulaNamedNames()) {
        tFormulaNamed* wFormulaNamed = wNamedContainer->FormulaNamed(wName);
        if (wFormulaNamed == nullptr) {
            continue;
        }
        tString wBody = wFormulaNamed->FormulaStr();
        if (wBody.empty()) {
            continue;
        }
        wBody = PrepareDefinedNameFormulaForExcelExport(wBody, sApi);
        sWorkbook.AddDefinedName(wName, wBody, -1);
    }
}

static tInt TotalsRowSubtotalFunctionNum(const tString& sFunc) {
    if (sFunc == "count") {
        return 103;
    }
    if (sFunc == "countNums") {
        return 102;
    }
    if (sFunc == "average") {
        return 101;
    }
    if (sFunc == "max") {
        return 104;
    }
    if (sFunc == "min") {
        return 105;
    }
    if (sFunc == "stdDev") {
        return 107;
    }
    if (sFunc == "var") {
        return 110;
    }
    if (sFunc == "none") {
        return 0;
    }
    return 109; // sum (default OOXML totalsRowFunction)
}

static tBool SkerCellHasExportableFormula(tSheet* sSheet, tIndex sRow, tIndex sCol) {
    if (sSheet == nullptr) {
        return false;
    }
    tCell* wCell = sSheet->Cell(sRow, sCol);
    return wCell != nullptr && wCell->Formula() != nullptr;
}

static tBool SkerCellHasExportableValue(tSheet* sSheet, tIndex sRow, tIndex sCol) {
    if (sSheet == nullptr) {
        return false;
    }
    tCell* wCell = sSheet->Cell(sRow, sCol);
    if (wCell == nullptr) {
        return false;
    }
    if (wCell->Formula() != nullptr) {
        return true;
    }
    if (wCell->Value().IsError()) {
        return true;
    }
    return !wCell->IsEmpty() && !wCell->IsValueEmpty();
}

// Excel ListObject repair: tableColumn@name must match header-row cell text; totals row needs
// SUBTOTAL cells when totalsRowFunction is declared in table metadata.
static void MaterializeStructuredTableOverlayCells(
    tSheet* sSheet,
    tRange* sRange,
    tRangeData* sRangeData,
    const std::vector<tString>& sColumnNames,
    const std::vector<StructuredTableColumnSpec>& sColumns,
    WorkbookBuilder& sWorkbook) {
    if (sSheet == nullptr || sRange == nullptr || sRangeData == nullptr) {
        return;
    }

    if (sRangeData->HasHeaders()) {
        const tIndex wHeaderRow = sRange->TopIndex();
        for (tIndex wCol = sRange->LeftIndex(); wCol <= sRange->RightIndex(); ++wCol) {
            const tSize wOrdinal = static_cast<tSize>(wCol - sRange->LeftIndex());
            if (wOrdinal >= sColumnNames.size()) {
                continue;
            }
            const tString& wColName = sColumnNames[wOrdinal];
            if (wColName.empty() || !RawHeaderCellText(sSheet, wHeaderRow, wCol).empty()) {
                continue;
            }
            sWorkbook.SetCellValue(MakeCellRef(wHeaderRow, wCol), wColName);
        }
    }

    if (!sRangeData->HasTotals() || sRangeData->TotalsRowCount() <= 0) {
        return;
    }

    const tIndex wTotalsRow = sRange->BottomIndex() + sRangeData->TotalsRowCount();
    const tIndex wDataTop = sRange->TopIndex() + (sRangeData->HasHeaders() ? 1 : 0);
    const tIndex wDataBottom = sRange->BottomIndex();
    if (wDataTop > wDataBottom) {
        return;
    }

    for (tIndex wCol = sRange->LeftIndex(); wCol <= sRange->RightIndex(); ++wCol) {
        const tSize wOrdinal = static_cast<tSize>(wCol - sRange->LeftIndex());
        if (wOrdinal >= sColumns.size()) {
            continue;
        }
        const StructuredTableColumnSpec& wSpec = sColumns[wOrdinal];
        const tString wCellRef = MakeCellRef(wTotalsRow, wCol);

        if (!wSpec.totalsRowFormula.empty()) {
            if (!SkerCellHasExportableFormula(sSheet, wTotalsRow, wCol)) {
                sWorkbook.SetCellFormula(wCellRef, wSpec.totalsRowFormula);
            }
            continue;
        }
        if (!wSpec.totalsRowLabel.empty()) {
            if (!SkerCellHasExportableValue(sSheet, wTotalsRow, wCol)) {
                sWorkbook.SetCellValue(wCellRef, wSpec.totalsRowLabel);
            }
            continue;
        }
        if (wSpec.totalsRowFunction.empty() || wSpec.totalsRowFunction == "none") {
            continue;
        }
        if (SkerCellHasExportableFormula(sSheet, wTotalsRow, wCol)
            || SkerCellHasExportableValue(sSheet, wTotalsRow, wCol)) {
            continue;
        }
        const tInt wFn = TotalsRowSubtotalFunctionNum(wSpec.totalsRowFunction);
        if (wFn <= 0) {
            continue;
        }
        const tString wDataRef =
            MakeCellRef(wDataTop, wCol) + ":" + MakeCellRef(wDataBottom, wCol);
        sWorkbook.SetCellFormula(
            wCellRef, "SUBTOTAL(" + std::to_string(wFn) + "," + wDataRef + ")");
    }
}

static void ExportStructuredTables(tApi& sApi, tWorkBook* sWorkBook, WorkbookBuilder& sWorkbook) {
    if (sWorkBook == nullptr) {
        return;
    }
    SkSpreadSheet::tRangeNamedContainer* wNamedContainer = sWorkBook->RangeNamedContainer();
    if (wNamedContainer == nullptr) {
        return;
    }

    for (const tString& wName : wNamedContainer->AllNames()) {
        if (wName.size() >= 6 && wName.compare(0, 6, "_xlnm.") == 0) {
            continue;
        }
        const std::vector<tRange*> wRanges = wNamedContainer->Ranges(wName);
        if (wRanges.size() != 1) {
            continue;
        }
        tRange* wRange = wRanges.front();
        if (wRange == nullptr || !wRange->IsData() || wRange->Sheet() == nullptr) {
            continue;
        }
        tRangeData* wRangeData = sWorkBook->RangeData(wName);
        if (wRangeData == nullptr || wRangeData->IsEmpty()) {
            continue;
        }

        tSheet* wSheet = wRange->Sheet();
        const tIndex wDataRow = wRange->TopIndex() + (wRangeData->HasHeaders() ? 1 : 0);
        if (wDataRow > wRange->BottomIndex()) {
            continue;
        }

        std::vector<StructuredTableColumnSpec> wColumns;
        std::vector<tString> wColumnNames;
        wColumnNames.reserve(static_cast<tSize>(wRange->RightIndex() - wRange->LeftIndex() + 1));
        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->RightIndex(); ++wCol) {
            const tSize wOrdinal = static_cast<tSize>(wCol - wRange->LeftIndex());
            tColumnData* wColData =
                wRangeData->FindColumnByOrdinal(wRange->LeftIndex(), wOrdinal);
            tString wColName;
            if (!wRangeData->HasHeaders()) {
                // headerRowCount=0: OOXML uses generic ColumnN names; row 1 holds labels/values.
                wColName = "Column" + std::to_string(wOrdinal + 1);
            } else {
                if (wColData != nullptr && !wColData->ImportLabel().empty()) {
                    wColName = wColData->ImportLabel();
                } else {
                    wColName = RawHeaderCellText(wSheet, wRange->TopIndex(), wCol);
                }
                if (wColName.empty() && wColData != nullptr) {
                    wColName = wColData->HeaderLabel(wSheet, wRange->TopIndex(), wRange->LeftIndex());
                }
                if (wColName.empty()) {
                    wColName = "Column" + std::to_string(wOrdinal + 1);
                }
            }
            wColumnNames.push_back(OoxmlTableColumnName(std::move(wColName)));
        }
        const tBool wHasStoredTotalsMetadata = [&]() {
            for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->RightIndex(); ++wCol) {
                const tSize wOrdinal = static_cast<tSize>(wCol - wRange->LeftIndex());
                tColumnData* wColData =
                    wRangeData->FindColumnByOrdinal(wRange->LeftIndex(), wOrdinal);
                if (wColData == nullptr) {
                    continue;
                }
                if (!wColData->TotalsRowLabel().empty()
                    || !wColData->TotalsRowFunction().empty()
                    || !wColData->TotalsRowFormula().empty()) {
                    return true;
                }
            }
            return false;
        }();
        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->RightIndex(); ++wCol) {
            const tSize wOrdinal = static_cast<tSize>(wCol - wRange->LeftIndex());
            tColumnData* wColData =
                wRangeData->FindColumnByOrdinal(wRange->LeftIndex(), wOrdinal);
            StructuredTableColumnSpec wSpec;
            wSpec.name = wColumnNames[wOrdinal];
            if (wColData != nullptr) {
                wSpec.calculatedFormula = wColData->CalculatedColumnFormula();
                wSpec.filterButtonHidden = wColData->FilterButtonHidden();
                wSpec.totalsRowLabel = wColData->TotalsRowLabel();
                wSpec.totalsRowFunction = wColData->TotalsRowFunction();
                wSpec.totalsRowFormula = wColData->TotalsRowFormula();
            }
            if (wSpec.calculatedFormula.empty()) {
                wSpec.calculatedFormula = FindExportableTableColumnFormula(
                    sApi,
                    wSheet,
                    wDataRow,
                    wRange->BottomIndex(),
                    wCol);
            }
            if (!ShouldExportAsCalculatedColumnFormula(wSpec.calculatedFormula)) {
                wSpec.calculatedFormula.clear();
            }
            // Legacy .sker files may only have lastrow/totalsRowCount without per-column OOXML metadata.
            if (wRangeData->HasTotals() && !wHasStoredTotalsMetadata) {
                if (wOrdinal == 0) {
                    wSpec.totalsRowLabel = "Total";
                } else if (wSpec.totalsRowFunction.empty() && wSpec.totalsRowFormula.empty()) {
                    wSpec.totalsRowFunction = "sum";
                }
            }
            wColumns.push_back(std::move(wSpec));
        }
        if (wColumns.empty()) {
            continue;
        }

        tIndex wBottom = wRange->BottomIndex();
        if (wRangeData->HasTotals()) {
            wBottom += wRangeData->TotalsRowCount();
        }
        const tString wRef = MakeCellRef(wRange->TopIndex(), wRange->LeftIndex()) + ":"
            + MakeCellRef(wBottom, wRange->RightIndex());
        sWorkbook.SetCurrentSheet(wSheet->Name());
        MaterializeStructuredTableOverlayCells(
            wSheet, wRange, wRangeData, wColumnNames, wColumns, sWorkbook);
        tString wStyleName = wRangeData->TableStyleName();
        const std::map<tString, tString>* wStyleElements =
            wRangeData->HasTableStyleElementCss() ? &wRangeData->TableStyleElementCss() : nullptr;
        tString wDisplayName = wRangeData->TableDisplayName();
        if (wDisplayName.empty()) {
            wDisplayName = ExcelTableDisplayName(wName);
        }
        sWorkbook.AddStructuredTable(
            wSheet->Name(),
            wName,
            wRef,
            wColumns,
            wStyleName,
            wRangeData->TableShowRowStripes(),
            wRangeData->TableShowColumnStripes(),
            wRangeData->TableShowFirstColumn(),
            wRangeData->TableShowLastColumn(),
            wRangeData->HasHeaders(),
            wStyleElements,
            wRangeData->TableAutoFilter(),
            wRangeData->TotalsRowCount(),
            &wDisplayName);

        for (tIndex wRow = wDataRow; wRow <= wRange->BottomIndex(); ++wRow) {
            for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->RightIndex(); ++wCol) {
                const tSize wOrdinal = static_cast<tSize>(wCol - wRange->LeftIndex());
                if (wOrdinal >= wColumns.size()) {
                    continue;
                }
                const tString& wColumnFormula = wColumns[wOrdinal].calculatedFormula;
                if (wColumnFormula.empty()) {
                    continue;
                }
                const tString wCellRef = MakeCellRef(wRow, wCol);
                // ExportCell already wrote per-row <f> and cached <v>; only mark ca="1" here.
                if (ShouldMarkTableColumnFormulaCa(wColumnFormula, wRow, wDataRow)) {
                    sWorkbook.MarkCellFormulaAsTableColumn(wCellRef);
                }
            }
        }
    }
}

static void ExportMergedRanges(tSheet* sSheet, WorkbookBuilder& sWorkbook) {
    std::set<tString> wSeen;
    const tIndex wLastRow = sSheet->LastRow();
    for (tIndex wRow = 1; wRow <= wLastRow; ++wRow) {
        tColRow* wColRow = sSheet->Row(wRow);
        if (wColRow == nullptr) {
            continue;
        }
        tColRow::tContainerRange* wContainer = wColRow->ContainerRange();
        if (wContainer == nullptr) {
            continue;
        }
        for (auto wRangeAllocatorRef : *wContainer->Container()) {
            tRange* wRange = sSheet->ColRowCellRange()->Range(wRangeAllocatorRef);
            if (wRange == nullptr || !wRange->IsMerged()) {
                continue;
            }
            if (wRange->TopIndex() != wRow) {
                continue;
            }
            const tString wRef = wRange->StrRef();
            if (!wSeen.insert(wRef).second) {
                continue;
            }
            sWorkbook.SetCellMerge(
                MakeCellRef(wRange->TopIndex(), wRange->LeftIndex()),
                MakeCellRef(wRange->BottomIndex(), wRange->RightIndex()));
        }
    }
}

static tBool CssDeclaresTimeFormat(const tString& sCss) {
    return SkExcel::CssDeclaresTimeFormat(sCss);
}

static tString ExcelTimeFormatFromCss(const tString& sCss) {
    const tString wFmtKey = ExtractCssQuotedProperty(sCss, "format-string");
    if (!wFmtKey.empty()) {
        const tString wExcelFormat = SkRootFormatKeyToExcelFormat(wFmtKey);
        if (!wExcelFormat.empty()) {
            return wExcelFormat;
        }
    }
    return "h:mm";
}

static tBool ExportCellValue(tApi& sApi, tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                             const tString& sCellRef, WorkbookBuilder& sWorkbook,
                             const tString& sCss);

static tBool IsNamedRangeAnchorCell(SkSpreadSheet::tRangeNamedContainer* sNamedContainer,
                                    tSheet* sSheet, tIndex sRow, tIndex sCol) {
    if (sNamedContainer == nullptr || sSheet == nullptr) {
        return false;
    }
    for (const tString& wName : sNamedContainer->AllNames()) {
        const std::vector<tRange*> wRanges = sNamedContainer->Ranges(wName);
        if (wRanges.size() != 1) {
            continue;
        }
        tRange* wRange = wRanges.front();
        if (wRange == nullptr || wRange->Sheet() != sSheet) {
            continue;
        }
        if (wRange->TopIndex() == wRange->BottomIndex()
            && wRange->LeftIndex() == wRange->RightIndex()
            && wRange->TopIndex() == sRow
            && wRange->LeftIndex() == sCol) {
            return true;
        }
    }
    return false;
}

static tBool TryExportNamedRangeAliasFormula(tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                                             tApi& sApi, const tString& sCellRef, WorkbookBuilder& sWorkbook) {
    if (sWorkBook == nullptr || sSheet == nullptr) {
        return false;
    }
    tCell* wCell = sSheet->Cell(sRow, sCol);
    if (wCell != nullptr && wCell->Formula() != nullptr) {
        return false;
    }
    if (wCell != nullptr) {
        const tString wFormulaStr = wCell->FormulaStr(false, false);
        if (!wFormulaStr.empty()) {
            return false;
        }
        if (wCell->Css() != 0) {
            return false;
        }
    }
    {
        tString wTableName;
        tRange* wTableRange = nullptr;
        std::tie(wTableName, wTableRange) = sSheet->FindRangeDataCovered(sRow, sCol);
        if (wTableRange != nullptr && wTableRange->IsData()) {
            tRangeData* wRangeData = sWorkBook->RangeData(wTableName);
            if (wRangeData != nullptr && wRangeData->HasHeaders()
                && sRow > wTableRange->TopIndex()) {
                return false;
            }
        }
    }
    const tVariant wValue = sApi.CellValue(sRow, sCol, sSheet);
    if (wValue.Type() == tVariantType::t_null) {
        return false;
    }

    SkSpreadSheet::tRangeNamedContainer* wNamedContainer = sWorkBook->RangeNamedContainer();
    if (wNamedContainer == nullptr) {
        return false;
    }
    if (IsNamedRangeAnchorCell(wNamedContainer, sSheet, sRow, sCol)) {
        return false;
    }

    tString wMatchName;
    for (const tString& wName : wNamedContainer->AllNames()) {
        if (wName.size() >= 6 && wName.compare(0, 6, "_xlnm.") == 0) {
            continue;
        }
        const std::vector<tRange*> wRanges = wNamedContainer->Ranges(wName);
        if (wRanges.size() != 1) {
            continue;
        }
        tRange* wRange = wRanges.front();
        if (wRange == nullptr || wRange->Sheet() == nullptr) {
            continue;
        }
        if (wRange->TopIndex() != wRange->BottomIndex() || wRange->LeftIndex() != wRange->RightIndex()) {
            continue;
        }
        if (wRange->Sheet() == sSheet
            && wRange->TopIndex() == sRow
            && wRange->LeftIndex() == sCol) {
            continue;
        }
        const tVariant wNamedValue = sApi.CellValue(
            wRange->TopIndex(), wRange->LeftIndex(), wRange->Sheet());
        if (!VariantsEqualForNamedRangeAlias(wValue, wNamedValue)) {
            continue;
        }
        if (!wMatchName.empty() && wMatchName != wName) {
            return false;
        }
        wMatchName = wName;
    }

    if (wMatchName.empty()) {
        return false;
    }
    sWorkbook.SetCellFormula(sCellRef, "=" + wMatchName);
    ExportCellValue(sApi, nullptr, sSheet, sRow, sCol, sCellRef, sWorkbook, "");
    return true;
}

static tBool ExportCellValue(tApi& sApi, tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                             const tString& sCellRef, WorkbookBuilder& sWorkbook,
                             const tString& sCss) {
    const tVariant wValue = sApi.CellValue(sRow, sCol, sSheet);
    switch (wValue.Type()) {
        case tVariantType::t_null:
            return false;
        case tVariantType::t_bool:
            sWorkbook.SetCellValue(sCellRef, wValue.Bool() ? "TRUE" : "FALSE");
            return true;
        case tVariantType::t_int:
            sWorkbook.SetCellValue(sCellRef, std::to_string(wValue.Int()));
            return true;
        case tVariantType::t_double:
            sWorkbook.SetCellValue(sCellRef, std::to_string(wValue.Double()));
            return true;
        case tVariantType::t_string:
            if (!wValue.String().empty()) {
                tString wText = wValue.String();
                NormalizeStructuredTableHeaderCellText(sWorkBook, sSheet, sRow, sCol, wText);
                sWorkbook.SetCellValue(sCellRef, wText);
                return true;
            }
            return false;
        case tVariantType::t_date: {
            tClassDate wDate(wValue);
            // IsHours() is true for 1900-01-01 anchor dates (_DATE_19000101); only treat as
            // time-only when the format is time-only or the cell has a non-midnight clock time.
            const tBool wTimeOnlyExport =
                CssDeclaresTimeFormat(sCss)
                || (wDate.IsHours()
                    && (wDate.Hour() != 0 || wDate.Minute() != 0 || wDate.Second() != 0));
            if (wTimeOnlyExport) {
                const tDouble wSerial = SkDateToExcelTimeSerial(wDate.Value());
                sWorkbook.SetCellDateSerial(sCellRef, wSerial, ExcelTimeFormatFromCss(sCss));
                return true;
            }
            sWorkbook.SetCellDateYMD(
                sCellRef, wDate.Year(), wDate.Month(), wDate.Day(), "[$-fr-FR]dd/mm/yyyy");
            return true;
        }
        case tVariantType::t_error:
            sWorkbook.SetCellValue(sCellRef, wValue.Error().Error());
            return true;
        default:
            return false;
    }
}

static void ExportCell(tApi& sApi, tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                       WorkbookBuilder& sWorkbook) {
    tRange* wMerged = sSheet->MergedRange(sRow, sCol);
    tBool wIsMergeMaster = false;
    tBool wIsMergeSlave = false;
    if (wMerged != nullptr && wMerged->IsMerged()) {
        if (wMerged->TopIndex() != sRow || wMerged->LeftIndex() != sCol) {
            wIsMergeSlave = true;
        } else {
            wIsMergeMaster = true;
        }
    }

    tCell* wCell = sSheet->Cell(sRow, sCol);
    const tString wCellRef = MakeCellRef(sRow, sCol);
    tString wCss = wIsMergeSlave
        ? CellFormatDirect(sApi, sSheet, sRow, sCol)
        : CssForExportCell(sApi, sSheet, sRow, sCol);
    MergeTableStyleCssOnExport(sWorkBook, sSheet, sRow, sCol, wCss);

    // Excel keeps styled empty fragments inside merges (e.g. C1/D1/E1 fill for B1:E1).
    if (wIsMergeSlave) {
        if (wCss.empty() && (wCell == nullptr || wCell->Css() == 0)) {
            return;
        }
        if (ShouldSkipDecoratedEmptyCell(wCell, false, wCss)) {
            return;
        }
        if (!wCss.empty()) {
            ApplyCssToWorkbookCell(sWorkbook, wCellRef, wCss, false);
        }
        return;
    }

    if (ShouldSkipDecoratedEmptyCell(wCell, false, wCss)) {
        return;
    }

    tString wFormula = sApi.Formula(sRow, sCol, sSheet, false);
    if (wFormula.empty() && wCell != nullptr) {
        const tString wFormulaStr = wCell->FormulaStr(false, false);
        if (!wFormulaStr.empty()) {
            wFormula = wFormulaStr;
        }
    }
    if (wFormula.empty() && wCell != nullptr) {
        const tVariant wRaw = wCell->Value();
        if (wRaw.Type() == tVariantType::t_string && !wRaw.String().empty() && wRaw.String().front() == '=') {
            wFormula = wRaw.String();
        }
    }

    tBool wHasContent = false;
    if (!wFormula.empty() && (wCell == nullptr || !wCell->IsMatExtend())) {
        tString wExcelFormula = wFormula;
        if (wExcelFormula.front() != '=') {
            wExcelFormula = "=" + wExcelFormula;
        }
        wExcelFormula = SkExcel::PrepareFormulaForExcelExport(
            wExcelFormula,
            static_cast<tInt>(sRow),
            static_cast<tInt>(sCol),
            sApi,
            sSheet,
            wCss.empty() ? nullptr : &wCss);
        sWorkbook.SetCellFormula(wCellRef, wExcelFormula);
        ExportCellValue(sApi, sWorkBook, sSheet, sRow, sCol, wCellRef, sWorkbook, wCss);
        wHasContent = true;
    } else if (wCss.empty() && TryExportNamedRangeAliasFormula(
                   sSheet->WorkBook(), sSheet, sRow, sCol, sApi, wCellRef, sWorkbook)) {
        wHasContent = true;
    } else if (wCell == nullptr || !wCell->IsEmpty() || !wCell->IsValueEmpty()) {
        wHasContent = ExportCellValue(sApi, sWorkBook, sSheet, sRow, sCol, wCellRef, sWorkbook, wCss);
    }

    // Row/column formatting cascades to coordinates without a physical cell; do not
    // materialize those as empty OOXML cells unless table style supplies a fill.
    if (!wHasContent && wCell == nullptr && !wIsMergeMaster && wCss.empty()) {
        return;
    }

    if (!wCss.empty()) {
        ApplyCssToWorkbookCell(sWorkbook, wCellRef, wCss, false);
    }
    if (wIsMergeMaster) {
        sWorkbook.SetCellWrap(wCellRef, true);
    }
}

static tBool EnsureExportSheet(WorkbookBuilder& sWorkbook, const tString& sSheetName, tBool& sNeedRemoveSheet1) {
    if (sSheetName.empty()) {
        return false;
    }
    // Default placeholder Sheet1 lives in m_MapSheets before m_SheetOrder is populated.
    if (sWorkbook.SetCurrentSheet(sSheetName)) {
        return true;
    }
    const std::vector<tString> wNames = sWorkbook.GetSheetNames();
    if (wNames.size() == 1 && wNames[0] == "Sheet1" && sSheetName != "Sheet1") {
        if (!sWorkbook.AddSheet(sSheetName)) {
            return false;
        }
        sNeedRemoveSheet1 = true;
    } else if (!sWorkbook.AddSheet(sSheetName)) {
        return false;
    }
    return sWorkbook.SetCurrentSheet(sSheetName);
}

} // namespace

static tBool CssHexToArgb(const tString& sCssColor, tString& sOutArgb) {
    tString wHash;
    if (!NormalizeCssColorToHexHash(sCssColor, wHash)) {
        return false;
    }
    if (!wHash.empty() && wHash.front() == '#') {
        wHash.erase(0, 1);
    }
    if (wHash.size() == 6) {
        sOutArgb = "FF" + wHash;
        return true;
    }
    if (wHash.size() == 8) {
        sOutArgb = wHash;
        return true;
    }
    return false;
}

static void AppendDxfBorderSideFromCss(pugi::xml_node& sBorder, const char* sSide,
                                       const tString& sCss, const char* sCssKey) {
    tString wStyle;
    tString wColor;
    if (!ParseCssBorderSide(ExtractCssProperty(sCss, sCssKey), wStyle, wColor)) {
        return;
    }
    auto wNode = sBorder.append_child(sSide);
    wNode.append_attribute("style").set_value(normalize_border_style(wStyle).c_str());
    tString wArgb;
    if (CssHexToArgb(wColor, wArgb)) {
        wNode.append_child("color").append_attribute("rgb").set_value(wArgb.c_str());
    }
}

static void AppendDxfFromTableStyleCss(pugi::xml_node& sDxf, const tString& sCss) {
    if (sCss.empty()) {
        return;
    }
    const tString wWeight = ExtractCssProperty(sCss, "font-weight");
    const tString wColor = ExtractCssProperty(sCss, "color");
    const tString wBg = ExtractCssProperty(sCss, "background-color");
    if (wWeight == "bold" || !wColor.empty()) {
        auto wFont = sDxf.append_child("font");
        if (wWeight == "bold") {
            wFont.append_child("b");
        }
        tString wArgb;
        if (CssHexToArgb(wColor, wArgb)) {
            wFont.append_child("color").append_attribute("rgb").set_value(wArgb.c_str());
        }
    }
    if (!wBg.empty()) {
        tString wArgb;
        if (CssHexToArgb(wBg, wArgb)) {
            auto wFill = sDxf.append_child("fill");
            auto wPattern = wFill.append_child("patternFill");
            wPattern.append_attribute("patternType").set_value("solid");
            wPattern.append_child("fgColor").append_attribute("rgb").set_value(wArgb.c_str());
            wPattern.append_child("bgColor").append_attribute("indexed").set_value("64");
        }
    }
    if (sCss.find("border-") != tString::npos) {
        auto wBorder = sDxf.append_child("border");
        AppendDxfBorderSideFromCss(wBorder, "left", sCss, "border-left");
        AppendDxfBorderSideFromCss(wBorder, "right", sCss, "border-right");
        AppendDxfBorderSideFromCss(wBorder, "top", sCss, "border-top");
        AppendDxfBorderSideFromCss(wBorder, "bottom", sCss, "border-bottom");
    }
}

static tString NormalizeConditionalFormatRangeForExcel(tString sRef) {
    for (char& wCh : sRef) {
        if (wCh == ';') {
            wCh = ' ';
        }
    }
    return sRef;
}

static tString StripLeadingEquals(tString sFormula) {
    if (!sFormula.empty() && sFormula.front() == '=') {
        sFormula.erase(0, 1);
    }
    return sFormula;
}

static tString HexColorFromCssProperty(const tString& sCss, const tString& sKey) {
    const tString wRaw = ExtractCssProperty(sCss, sKey);
    if (wRaw.empty()) {
        return "";
    }
    tString wHash;
    if (!NormalizeCssColorToHexHash(wRaw, wHash)) {
        return "";
    }
    return wHash;
}

static tBool ParseHighlightCellsRule(
    const tString& sParam1, tString& oOperator, tString& oThreshold) {
    if (sParam1.size() < 2 || sParam1.front() != '%') {
        return false;
    }
    struct OpMap { const char* prefix; const char* excelOp; };
    static const OpMap kOps[] = {
        {">=", "greaterThanOrEqual"},
        {"<=", "lessThanOrEqual"},
        {"<>", "notEqual"},
        {">", "greaterThan"},
        {"<", "lessThan"},
        {"=", "equal"},
    };
    for (const OpMap& wOp : kOps) {
        const tSize wLen = std::strlen(wOp.prefix);
        if (sParam1.compare(1, wLen, wOp.prefix) == 0) {
            oOperator = wOp.excelOp;
            oThreshold = sParam1.substr(1 + wLen);
            return !oThreshold.empty();
        }
    }
    return false;
}

static tString ExcelIconSetNameFromSk(tIconType sIconType, tBool sFiveIcons) {
    switch (sIconType) {
        case tIconType::t_Arrows:
            return sFiveIcons ? "5Arrows" : "3Arrows";
        case tIconType::t_Flags:
            return sFiveIcons ? "5Flags" : "3Flags";
        case tIconType::t_Shapes:
            return sFiveIcons ? "5Quarters" : "3Symbols2";
        case tIconType::t_Indicators:
            return sFiveIcons ? "5ArrowsGray" : "3Signs";
        case tIconType::t_Ratings:
            return sFiveIcons ? "5Rating" : "3TrafficLights1";
        default:
            return sFiveIcons ? "5Arrows" : "3Arrows";
    }
}

static void ExportConditionalFormats(tSheet* sSheet, WorkbookBuilder& sWorkbook) {
    if (sSheet == nullptr) {
        return;
    }
    tColRowCellRange* wColRow = sSheet->ColRowCellRange();
    if (wColRow == nullptr) {
        return;
    }
    tConditionalFormatContainer* wContainer = wColRow->ConditionalFormatContainer();
    if (wContainer == nullptr || wContainer->IsEmpty()) {
        return;
    }

    tInt wPriority = 1;
    for (tSize wIndex = 0; wIndex < wContainer->Size(); ++wIndex) {
        tConditionalFormat* wCf = wContainer->ConditionalFormatAt(wIndex);
        if (wCf == nullptr || wCf->Type() == tConditionalFormatType::t_None || wCf->IsEmpty()) {
            continue;
        }

        const tString wRange = NormalizeConditionalFormatRangeForExcel(wCf->Ref());
        if (wRange.empty()) {
            continue;
        }

        switch (wCf->Type()) {
            case tConditionalFormatType::t_CustomFormulas: {
                tString wFormula = StripLeadingEquals(wCf->Param1());
                if (wFormula.empty()) {
                    break;
                }
                const tString wFont = HexColorFromCssProperty(wCf->Param2(), "color");
                const tString wFill = HexColorFromCssProperty(wCf->Param2(), "background-color");
                sWorkbook.AddConditionalFormat(wRange, wFormula, wFont, wFill, wPriority++);
                break;
            }
            case tConditionalFormatType::t_HighlightCellsRules: {
                tString wOperator;
                tString wThreshold;
                if (!ParseHighlightCellsRule(wCf->Param1(), wOperator, wThreshold)) {
                    break;
                }
                const tString wFont = HexColorFromCssProperty(wCf->Param2(), "color");
                const tString wFill = HexColorFromCssProperty(wCf->Param2(), "background-color");
                sWorkbook.AddConditionalFormat(
                    wRange, wThreshold, wFont, wFill, wPriority++, "cellIs", wOperator);
                break;
            }
            case tConditionalFormatType::t_DataBars: {
                tString wColor = wCf->Param1();
                if (!wColor.empty() && wColor.front() != '#') {
                    wColor = "#" + wColor;
                }
                sWorkbook.AddConditionalFormatDataBar(
                    wRange,
                    wColor,
                    wCf->Param4(),
                    wCf->Param5(),
                    wCf->Param3().empty() ? "gradient" : wCf->Param3(),
                    wPriority++);
                break;
            }
            case tConditionalFormatType::t_ColorScales: {
                tString wMinC = wCf->Param1();
                tString wMidC = wCf->Param2();
                tString wMaxC = wCf->Param3();
                auto wEnsureHash = [](tString sColor) {
                    if (!sColor.empty() && sColor.front() != '#') {
                        sColor = "#" + sColor;
                    }
                    return sColor;
                };
                sWorkbook.AddConditionalFormatColorScale(
                    wRange,
                    wEnsureHash(wMinC),
                    wEnsureHash(wMidC.empty() ? wMinC : wMidC),
                    wEnsureHash(wMaxC.empty() ? wMinC : wMaxC),
                    wCf->Param4(),
                    wCf->Param5(),
                    wCf->Param6(),
                    wPriority++);
                break;
            }
            case tConditionalFormatType::t_IconSets: {
                const tBool wParam4IsNumeric =
                    !wCf->Param4().empty() && tClassString(wCf->Param4()).IsNumber();
                const tBool wParam5IsNumeric =
                    !wCf->Param5().empty() && tClassString(wCf->Param5()).IsNumber();
                const tBool wParam6IsNumeric =
                    !wCf->Param6().empty() && tClassString(wCf->Param6()).IsNumber();
                const tBool wFiveIcons =
                    !wParam4IsNumeric && !wParam5IsNumeric && wParam6IsNumeric;
                tString wIconSet;
                if (!wFiveIcons) {
                    wIconSet = wCf->Param8();
                }
                if (wIconSet.empty()) {
                    wIconSet = ExcelIconSetNameFromSk(wCf->IconType(), wFiveIcons);
                }
                std::vector<tString> wThresholds;
                std::vector<tString> wThresholdTypes;
                if (wFiveIcons) {
                    if (!wCf->Param6().empty()) wThresholds.push_back(wCf->Param6());
                    if (!wCf->Param7().empty()) wThresholds.push_back(wCf->Param7());
                    if (!wCf->Param8().empty()) wThresholds.push_back(wCf->Param8());
                    if (!wCf->Param9().empty()) wThresholds.push_back(wCf->Param9());
                } else {
                    if (!wCf->Param4().empty()) wThresholds.push_back(wCf->Param4());
                    if (!wCf->Param5().empty()) wThresholds.push_back(wCf->Param5());
                    if (!wCf->Param6().empty()) wThresholds.push_back(wCf->Param6());
                    wThresholdTypes = tClassString(wCf->Param9()).Split(",");
                    if (wThresholdTypes.size() == 1 && wThresholdTypes[0].empty()) {
                        wThresholdTypes.clear();
                    }
                }
                sWorkbook.AddConditionalFormatIconSet(
                    wRange, wIconSet, "percent", true, false,
                    wCf->Param10().empty() ? wPriority++ : static_cast<tInt>(tClassString(wCf->Param10()).ToDouble()),
                    wThresholds, wThresholdTypes);
                break;
            }
            default:
                break;
        }
    }
}

static tString TrimAsciiCopy(tString sText) {
    while (!sText.empty() && std::isspace(static_cast<unsigned char>(sText.front()))) {
        sText.erase(sText.begin());
    }
    while (!sText.empty() && std::isspace(static_cast<unsigned char>(sText.back()))) {
        sText.pop_back();
    }
    return sText;
}

static tBool StartsWithIgnoreCaseAscii(const tString& sText, const tString& sPrefix) {
    if (sText.size() < sPrefix.size()) {
        return false;
    }
    for (tSize wI = 0; wI < sPrefix.size(); ++wI) {
        if (std::tolower(static_cast<unsigned char>(sText[wI]))
            != std::tolower(static_cast<unsigned char>(sPrefix[wI]))) {
            return false;
        }
    }
    return true;
}

static tString FirstJsonArrayString(const tString& sText) {
    const tSize wQuote = sText.find('"');
    if (wQuote == tString::npos) {
        return "";
    }
    const tSize wEnd = sText.find('"', wQuote + 1);
    if (wEnd == tString::npos || wEnd <= wQuote + 1) {
        return "";
    }
    return sText.substr(wQuote + 1, wEnd - wQuote - 1);
}

// Unwrap =DATARANGE(Sheet1!A1:A5) / JSON ["A1:A5"] into an Excel xm:f source range.
static tString SparklineSourceFromDataRange(tString sRaw) {
    tString wText = TrimAsciiCopy(std::move(sRaw));
    if (wText.empty()) {
        return "";
    }
    if (wText.front() == '=') {
        wText.erase(wText.begin());
        wText = TrimAsciiCopy(wText);
    }
    if (StartsWithIgnoreCaseAscii(wText, "DATARANGE")) {
        const tSize wOpen = wText.find('(');
        const tSize wClose = wText.rfind(')');
        if (wOpen != tString::npos && wClose != tString::npos && wClose > wOpen) {
            return TrimAsciiCopy(wText.substr(wOpen + 1, wClose - wOpen - 1));
        }
    }
    if (!wText.empty() && wText.front() == '[') {
        return FirstJsonArrayString(wText);
    }
    return wText;
}

static tBool SparklineShowMarkers(tCellAttribute* sAttr) {
    if (sAttr == nullptr) {
        return true;
    }
    const tVariant& wVal = sAttr->Value();
    if (wVal.Type() == tVariantType::t_bool) {
        return wVal.Bool();
    }
    tString wText = TrimAsciiCopy(wVal.Str());
    for (char& wCh : wText) {
        wCh = static_cast<char>(std::tolower(static_cast<unsigned char>(wCh)));
    }
    return wText != "false" && wText != "0" && wText != "no";
}

static tString SparklineClassName(tCellClassAttribute* sAttr) {
    if (sAttr == nullptr) {
        return "";
    }
    if (sAttr->ModelClass() != nullptr && !sAttr->ModelClass()->ClassName().empty()) {
        return sAttr->ModelClass()->ClassName();
    }
    return sAttr->ClassName();
}

static void ExportSparklines(tSheet* sSheet, WorkbookBuilder& sWorkbook) {
    if (sSheet == nullptr) {
        return;
    }
    tColRowCellRange* wColRow = sSheet->ColRowCellRange();
    if (wColRow == nullptr) {
        return;
    }
    tCellClassContainer* wContainer = wColRow->CellClassContainer();
    if (wContainer == nullptr) {
        return;
    }
    for (const auto& wPair : wContainer->MapRef()) {
        tCell* wCell = wContainer->CellByRef(wPair.first);
        if (wCell == nullptr) {
            continue;
        }
        tCellClassAttribute* wClass = wCell->ClassAttribute();
        if (SparklineClassName(wClass) != "SkCellClassSparkline") {
            continue;
        }
        tCellAttribute* wDataRange = wClass->Find("DataRange");
        if (wDataRange == nullptr) {
            wDataRange = wClass->CellAttribute("DataRange");
        }
        if (wDataRange == nullptr) {
            continue;
        }
        tString wFormula = wDataRange->FormulaWire(false, false);
        if (wFormula.empty()) {
            wFormula = wDataRange->FormulaStr(false, false);
        }
        if (wFormula.empty()) {
            wFormula = wDataRange->Value().Str();
        }
        tString wSource = SparklineSourceFromDataRange(wFormula);
        if (wSource.empty()) {
            continue;
        }
        if (wSource.find('!') == tString::npos) {
            tString wQualified = QualifyRefsForSheet(sSheet->Name(), tString("=") + wSource);
            if (!wQualified.empty() && wQualified.front() == '=') {
                wQualified.erase(wQualified.begin());
            }
            if (!wQualified.empty()) {
                wSource = wQualified;
            }
        }
        const tString wCellRef = wCell->StrRef(false);
        if (wCellRef.empty()) {
            continue;
        }
        sWorkbook.AddSparkline(wCellRef, wSource, SparklineShowMarkers(wClass->Find("showMarkers")));
    }
}

tBool ExportApiToXlsx(tApi& sApi, const tString& sOutputPath) {
    tWorkBook* wWorkBook = sApi.ActiveWorkBook();
    if (wWorkBook == nullptr) {
        std::cerr << "SkExcel: ExportApiToXlsx — no active workbook" << std::endl;
        return false;
    }

    auto wWorkbook = CreateWorkbook();
    {
        const tString wDefaultFont = InferWorkbookDefaultFontName(sApi, wWorkBook);
        const tInt wDefaultFontSize = static_cast<tInt>(std::lround(
            wWorkBook->DefaultFontSize() > 0.0 ? wWorkBook->DefaultFontSize() : 11.0));
        wWorkbook->SetWorkbookDefaultFont(wDefaultFont, wDefaultFontSize);
    }
    tBool wNeedRemoveSheet1 = false;
    tBool wExportedAnySheet = false;
    for (tSheet* wSheet : wWorkBook->VectorPtSheet()) {
        if (IsSystemSheetName(wSheet->Name())) {
            continue;
        }
        if (!EnsureExportSheet(*wWorkbook, wSheet->Name(), wNeedRemoveSheet1)) {
            std::cerr << "SkExcel: failed to prepare sheet " << wSheet->Name() << std::endl;
            return false;
        }

        sApi.ActiveSheet(wSheet->Name());
        ExportSheetDimensions(sApi, wSheet, *wWorkbook);
        wWorkbook->SetSheetShowGridLines(wSheet->ShowGridLines());
        if (wSheet->ViewZoomScaleNormal() > 0) {
            wWorkbook->SetSheetZoomScaleNormal(wSheet->ViewZoomScaleNormal());
        } else {
            wWorkbook->SetSheetZoomScaleNormal(100);
        }
        ExportMergedRanges(wSheet, *wWorkbook);
        ExportConditionalFormats(wSheet, *wWorkbook);
        ExportSparklines(wSheet, *wWorkbook);

        const tIndex wLastRow = wSheet->LastRow();
        const tIndex wLastCol = wSheet->LastCol();
        for (tIndex wRow = 1; wRow <= wLastRow; ++wRow) {
            for (tIndex wCol = 1; wCol <= wLastCol; ++wCol) {
                ExportCell(sApi, wWorkBook, wSheet, wRow, wCol, *wWorkbook);
            }
        }
        wExportedAnySheet = true;
    }

    ExportStructuredTables(sApi, wWorkBook, *wWorkbook);
    ExportWorkbookDefinedNames(wWorkBook, *wWorkbook, sApi);

    if (wNeedRemoveSheet1) {
        wWorkbook->RemoveSheet("Sheet1");
    }

    if (!wExportedAnySheet) {
        std::cerr << "SkExcel: ExportApiToXlsx — no exportable sheets" << std::endl;
        return false;
    }

    if (!wWorkbook->ExportToXlsx(sOutputPath)) {
        std::cerr << "SkExcel: ExportToXlsx failed for " << sOutputPath << std::endl;
        return false;
    }
    return true;
}

tBool ExportDemoXlsx(const tString& outputPath) {
	auto workbook = CreateWorkbook();
	workbook->SetCellValue("A1", "coucou");
	workbook->SetCellFontColor("A1", "#0000FF");
	workbook->SetCellFillColor("A1", "#FFFF00");

    workbook->SetColumnWidth(2, 20.0); 
    workbook->SetColumnWidth("C", 20.0); 
    workbook->SetRowHeight(2, 20.0); 
    workbook->SetRowHeight(3, 40.0); 

    // Dégradé vertical A2..A101: fond du jaune (#FFFF00) vers blanc (#FFFFFF), police bleue
    for(tInt i=0; i<100; i++) {
        tString cell = tString("A") + std::to_string(i+2);
        tString text = tString("coucou") + std::to_string(i+2);
        tDouble t = (100==1)?0.0: (tDouble)i/99.0; // 0..1
        tInt r0=0x00,g0=0xFF,b0=0x00; // jaune
        tInt r1=0xFF,g1=0xFF,b1=0xFF; // blanc
        tInt r = (tInt)std::round(r0 + (r1 - r0) * t);
        tInt g = (tInt)std::round(g0 + (g1 - g0) * t);
        tInt b = (tInt)std::round(b0 + (b1 - b0) * t);
        char rgb[6+1]; // RRGGBB
        std::snprintf(rgb, sizeof(rgb), "%02X%02X%02X", r, g, b);
        workbook->SetCellValue(cell, text);
        workbook->SetCellFontColor(cell, "#0000FF");
        workbook->SetCellFillColor(cell, tString("#") + rgb); // #RRGGBB
    }

	workbook->SetCellValue("B1", "coucou2");
    workbook->SetCellFontColor("B1", "#0000FF");
    workbook->SetCellFillColor("B1", "#FFFF00");
    for(tInt i=0; i<100; i++) {
        tString cell = tString("B") + std::to_string(i+2);
        tString text = tString("coucou") + std::to_string(i+2);
        tDouble t = (100==1)?0.0: (tDouble)i/99.0; // 0..1
        tInt r0=0xFF,g0=0x00,b0=0x00; // jaune
        tInt r1=0xFF,g1=0xFF,b1=0xFF; // blanc
        tInt r = (tInt)std::round(r0 + (r1 - r0) * t);
        tInt g = (tInt)std::round(g0 + (g1 - g0) * t);
        tInt b = (tInt)std::round(b0 + (b1 - b0) * t);
        char rgb[6+1]; // RRGGBB
        std::snprintf(rgb, sizeof(rgb), "%02X%02X%02X", r, g, b);
        workbook->SetCellValue(cell, text);
        workbook->SetCellFontColor(cell, "#0000FF");
        workbook->SetCellFillColor(cell, tString("#") + rgb); // #RRGGBB
    }

    workbook->SetCellBorderColor("C1", "#0000FF");
    workbook->SetCellBorderSides("C1", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF");
	workbook->SetCellValue("C1", "coucou3");
    workbook->SetCellBorderSides("D1", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF");
    workbook->SetCellValue("E1", "coucou5");
    workbook->SetCellBorderSides("E1", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF");
    workbook->SetCellValue("F1", "coucou6");
    workbook->SetCellBorderSides("F1", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF", "thin", "#0000FF");
	workbook->SetCellValue("G1", "coucou7");

    // Examples of number formatting
    workbook->SetCellValue("H1", "1234.56");
    workbook->SetCellCurrency("H1", "$");
    
    workbook->SetCellValue("H2", "0.15");
    workbook->SetCellPercentage("H2", 2);
    
    // French date examples
    workbook->SetCellDateYMD("H3", 2023, 12, 25, "[$-fr-FR]dd/mm/yyyy"); // 25/12/2023
    workbook->SetCellDateYMD("H4", 2024, 1, 15, "[$-fr-FR]dd mmmm yyyy"); // 15 janvier 2024
    
    workbook->SetCellValue("H5", "1234567.89");
    workbook->SetCellNumberFormat("H5", "#,##0.00");
    
    workbook->SetCellValue("H6", "0.75");
    workbook->SetCellPercentage("H6", 0); // No decimal places
    
    workbook->SetCellValue("H7", "€1,234.56");
    workbook->SetCellCurrency("H7", "€");

    // Named cells/ranges examples
    workbook->AddNamedCell("CelluleTitre", "A1");
    workbook->AddNamedRange("ZoneDonnees", "L5", "O6");

    workbook->SetCellValue("J5", "bordures");
    workbook->SetCellBorderSides("J5",
      "thin",   "#0000FF",   // left
      "medium", "#00AA00",   // right
      "dashed", "#FF0000",   // top
      "thick",  "#000000"    // bottom
    );

    // Examples of font properties
    workbook->SetCellValue("I1", "Polices");
    workbook->SetCellFontName("I1", "Arial");
    workbook->SetCellFontSize("I1", 14);
    workbook->SetCellFontBold("I1", true);
    workbook->SetCellFontColor("I1", "#0000FF");
    
    workbook->SetCellValue("I2", "Texte en italique");
    workbook->SetCellFontItalic("I2", true);
    workbook->SetCellFontName("I2", "Times New Roman");
    workbook->SetCellFontSize("I2", 12);
    
    workbook->SetCellValue("I3", "Texte souligné");
    workbook->SetCellFontUnderline("I3", true);
    workbook->SetCellFontBold("I3", true);
    workbook->SetCellFontColor("I3", "#FF0000");

    // Examples of formulas
    workbook->SetCellValue("K1", "Formules");
    workbook->SetCellFontColor("K1", "#FF0000");
    workbook->SetCellValue("K2", "10");
    workbook->SetCellValue("K3", "20");
    workbook->SetCellFormula("K4", "=K2+K3");  // Addition: 30
    workbook->SetCellFormula("K5", "=K3-K2");  // Soustraction: 10
    workbook->SetCellFormula("K6", "=K2*K3");  // Multiplication: 200
    workbook->SetCellFormula("K7", "=K3/K2");  // Division: 2
    workbook->SetCellFormula("K8", "=SUM(K2:K3)");  // Somme: 30
    workbook->SetCellFormula("K9", "=AVERAGE(K2:K3)");  // Moyenne: 15
    workbook->SetCellFormula("K10", "=MAX(K2:K3)");  // Maximum: 20
    workbook->SetCellFormula("K11", "=MIN(K2:K3)");  // Minimum: 10
    workbook->SetCellFormula("K12", "=IF(K2>K3,\"K2 plus grand\",\"K3 plus grand\")");  // Condition: "K3 plus grand"

    // Examples of merged cells
    workbook->SetCellValue("L1", "Cellules Mergées");
    workbook->SetCellFontBold("L1", true);
    workbook->SetCellFontColor("L1", "#800080");
    workbook->SetCellMerge("L1", "N1");  // Merge L1 to N1 (3 columns)
    
    workbook->SetCellValue("L2", "Titre principal");
    workbook->SetCellFontSize("L2", 16);
    workbook->SetCellFontBold("L2", true);
    workbook->SetCellFillColor("L2", "#E6E6FA");
    workbook->SetCellMerge("L2", "O3");  // Merge L2 to O3 (4 columns, 2 rows)
    
    workbook->SetCellValue("L4", "Sous-titre");
    workbook->SetCellFontItalic("L4", true);
    workbook->SetCellMerge("L4", "M4");  // Merge L4 to M4 (2 columns)
    
    workbook->SetCellValue("L5", "Données");
    workbook->SetCellValue("M5", "Valeur 1");
    workbook->SetCellValue("N5", "Valeur 2");
    workbook->SetCellValue("O5", "Total");
    workbook->SetCellMerge("L5", "L6");  // Merge L5 to L6 (1 column, 2 rows)
    workbook->SetCellMerge("M5", "M6");  // Merge M5 to M6
    workbook->SetCellMerge("N5", "N6");  // Merge N5 to N6
    workbook->SetCellMerge("O5", "O6");  // Merge O5 to O6

    // Examples of text alignments
    workbook->SetCellValue("Q1", "Alignements");
    workbook->SetCellFontBold("Q1", true);
    workbook->SetCellHorizontalAlign("Q1", "center");
    
    workbook->SetCellValue("Q2", "Gauche");
    workbook->SetCellHorizontalAlign("Q2", "left");
    
    workbook->SetCellValue("Q3", "Centre");
    workbook->SetCellHorizontalAlign("Q3", "center");
    
    workbook->SetCellValue("Q4", "Droite");
    workbook->SetCellHorizontalAlign("Q4", "right");
    
    workbook->SetCellValue("Q5", "Haut");
    workbook->SetCellVerticalAlign("Q5", "top");
    workbook->SetRowHeight(5, 40.0);
    
    workbook->SetCellValue("Q6", "Milieu");
    workbook->SetCellVerticalAlign("Q6", "center");
    workbook->SetRowHeight(6, 40.0);
    
    workbook->SetCellValue("Q7", "Bas");
    workbook->SetCellVerticalAlign("Q7", "bottom");
    workbook->SetRowHeight(7, 40.0);
    
    workbook->SetCellValue("Q8", "Wrap text wrap text wrap text");
    workbook->SetCellWrap("Q8", true);
    workbook->SetColumnWidth("Q", 12.0);

    // Examples of text rotation
    workbook->SetCellValue("R1", "Rot 45");
    workbook->SetCellTextRotation("R1", 45);
    workbook->SetCellValue("R2", "Rot -45");
    workbook->SetCellTextRotation("R2", -45);
    workbook->SetCellValue("R3", "Vertical");
    workbook->SetCellTextRotation("R3", 255);
    workbook->SetColumnWidth("R", 12.0);
    workbook->SetRowHeight(3, 40.0);

    // Examples of row heights and column widths
    workbook->SetCellValue("P1", "Dimensions");
    workbook->SetCellFontBold("P1", true);
    workbook->SetCellFontColor("P1", "#006400");
    
    // Set different row heights
    workbook->SetRowHeight(1, 30.0);   // Row 1: 30 points high
    workbook->SetRowHeight(2, 20.0);   // Row 2: 20 points high
    workbook->SetRowHeight(3, 25.0);   // Row 3: 25 points high
    
    // Set different column widths
    workbook->SetColumnWidth("A", 15.0);  // Column A: 15 characters wide
    workbook->SetColumnWidth("B", 20.0);  // Column B: 20 characters wide
    workbook->SetColumnWidth("C", 10.0);  // Column C: 10 characters wide
    workbook->SetColumnWidth("D", 25.0);  // Column D: 25 characters wide
    
    // Set column widths by index
    workbook->SetColumnWidth(5, 18.0);   // Column E: 18 characters wide
    workbook->SetColumnWidth(6, 12.0);   // Column F: 12 characters wide
    
    // Add some content to demonstrate the sizing
    workbook->SetCellValue("A1", "Colonne A (15 chars)");
    workbook->SetCellValue("B1", "Colonne B (20 chars)");
    workbook->SetCellValue("C1", "Col C (10)");
    workbook->SetCellValue("D1", "Colonne D (25 caractères)");
    workbook->SetCellValue("E1", "Col E (18 chars)");
    workbook->SetCellValue("F1", "Col F (12)");
    
    workbook->SetCellValue("A2", "Ligne 2 (20 pts)");
    workbook->SetCellValue("B2", "Texte plus long pour tester la largeur");
    workbook->SetCellValue("C2", "Court");
    workbook->SetCellValue("D2", "Texte très long pour colonne large");
    
    workbook->SetCellValue("A3", "Ligne 3 (25 pts)");
    workbook->SetCellValue("B3", "Hauteur moyenne");
    workbook->SetCellValue("C3", "Test");
    workbook->SetCellValue("D3", "Formatage des dimensions");

    tString wSheetName="Stéphane ALLEZ";
    workbook->AddSheet(wSheetName);
    workbook->SetCurrentSheet(wSheetName);
    workbook->SetCellValue("A1", "coucou2");
    workbook->SetCellFontColor("A1", "#0000FF");
    workbook->SetCellFillColor("A1", "#FFFF00");

    // Test conditional formatting - striped rows with visible color
    workbook->AddConditionalFormatStripedRows("A1:C10", "#FFE6E6");
    
    // Add a simple conditional format for specific cell values
    workbook->AddConditionalFormat("A1:A10", "A1=\"Ligne 1\"", "#FFFFFF", "#0000FF", 2);
    
    // Add conditional format for values > 10
    workbook->AddConditionalFormat("G7:G13", "10", "", "#00FF00", 3, "cellIs", "greaterThan");
    
    // Add some test data
    for(tInt i = 1; i <= 10; i++) {
        workbook->SetCellValue("A" + std::to_string(i), "Ligne " + std::to_string(i));
        workbook->SetCellValue("B" + std::to_string(i), "Donnée " + std::to_string(i));
        workbook->SetCellValue("C" + std::to_string(i), "Test " + std::to_string(i));
    }
    
    // Add test data for the G7:G13 > 10 conditional format
    for(tInt i = 7; i <= 13; i++) {
        workbook->SetCellValue("G" + std::to_string(i), std::to_string(5 + i)); // Values 12-18
    }
    
    // Test icon sets - add some data for icon sets
    workbook->SetCellValue("I1", "Jeux d'icônes");
    workbook->SetCellFontBold("I1", true);
    
    // Add test data for icon sets (percentages)
    for(tInt i = 1; i <= 10; i++) {
        workbook->SetCellValue("I" + std::to_string(i + 1), std::to_string(i * 10)); // Values 10, 20, 30, ..., 100
    }
    
    // Add 3-arrow icon set with custom thresholds
    std::vector<tString> customThresholds = {"0", "33", "67"};
    workbook->AddConditionalFormatIconSet("I2:I11", "3Arrows", "percent", true, false, 4, customThresholds);

	return workbook->ExportToXlsx(outputPath);
}

// Global conditional formatting functions
tBool AddConditionalFormat(const tString& sRange, const tString& sFormula, const tString& sFontColor, const tString& sFillColor, tInt sPriority) {
    auto workbook = CreateWorkbook();
    if (!workbook) return false;
    return workbook->AddConditionalFormat(sRange, sFormula, sFontColor, sFillColor, sPriority);
}

tBool AddConditionalFormatStripedRows(const tString& sRange, const tString& sFillColor) {
    auto workbook = CreateWorkbook();
    if (!workbook) return false;
    return workbook->AddConditionalFormatStripedRows(sRange, sFillColor);
}

} // namespace SkExcel
