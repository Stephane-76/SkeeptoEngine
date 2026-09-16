//=============================================================================
// SkPrintParameters.hpp
//=============================================================================
#ifndef SkPrintParameters_hpp
#define SkPrintParameters_hpp
#include "SkTools.hpp"

using namespace SkRoot;
namespace SkSpreadSheet {

//! Page orientation (OOXML pageSetup @orientation)
enum class tPrintOrientation : tByte {
    Portrait = 0,
    Landscape = 1
};

//! Multi-page tiling order (OOXML pageSetup @pageOrder)
enum class tPrintPageOrder : tByte {
    DownThenOver = 0,
    OverThenDown = 1
};

//! Which keys tPrintParameters::JsonWrite emits.
enum class tPrintJsonScope : tByte {
    //! Full merged snapshot (workbook fields + sheet orientation / FitToPage).
    All = 0,
    //! Shared layout stored on tWorkBook (omits orientation and FitToPage).
    WorkBook = 1,
    //! Per-sheet overrides only (orientation and FitToPage).
    Sheet = 2
};

//! Print layout: workbook-shared fields plus optional sheet overrides (orientation, FitToPage).
//! Explicit row/column page breaks are intentionally not modeled here.
class tPrintParameters {
public:
    /// @brief Construct with Excel-oriented defaults (see SkPrintParameters.cpp).
    tPrintParameters();

    //! OOXML paperSize (see ECMA-376 / MS-OE376). Default 9 = A4 (1 = Letter).
    tInt m_PaperSize;
    //! Page orientation (stored per sheet; ignored on the workbook object).
    tPrintOrientation m_Orientation;

    //! Scale percentage 10–400; active when FitToPage is false.
    tInt m_Scale;
    //! When true, use FitToWidthPages / FitToHeightPages instead of Scale (OOXML fitToPage workflow).
    //! Stored per sheet; ignored on the workbook object.
    tBool m_FitToPage;
    //! Pages wide when fitting (typically ≥ 1 while m_FitToPage).
    tInt m_FitToWidthPages;
    //! Pages tall when fitting (typically ≥ 1 while m_FitToPage).
    tInt m_FitToHeightPages;

    //! Order in which columns/rows spill across printed pages.
    tPrintPageOrder m_PageOrder;

    //! Print in black and white.
    tBool m_BlackAndWhite;
    //! Draft-quality output where applicable.
    tBool m_Draft;
    //! Prefer printer driver defaults where applicable (OOXML @usePrinterDefaults).
    tBool m_UsePrinterDefaults;

    //! Horizontal dots-per-inch hint.
    tInt m_HorizontalDpi;
    //! Vertical dots-per-inch hint.
    tInt m_VerticalDpi;

    //! Number of copies when the downstream pipeline honors it.
    tInt m_Copies;
    //! First printed page number; 0 means let the exporter/runtime choose (“automatic”).
    tInt m_FirstPageNumber;

    //! Margins in inches (OOXML pageMargins defaults as in Excel desktop).
    tDouble m_MarginLeft;
    tDouble m_MarginRight;
    tDouble m_MarginTop;
    tDouble m_MarginBottom;
    tDouble m_MarginHeader;
    tDouble m_MarginFooter;

    //! Print worksheet gridlines.
    tBool m_PrintGridLines;
    //! Print row and column headings.
    tBool m_PrintHeadings;
    //! Center the printed area horizontally on the paper.
    tBool m_HorizontalCentered;
    //! Center the printed area vertically on the paper.
    tBool m_VerticalCentered;

    //! Allow automatic pagination (OOXML pageSetUpPr @autoPageBreaks). Independent of storing manual breaks.
    tBool m_AutoPageBreaks;

    //! Serializes a JSON object (StartObject/EndObject); only keys whose value differs from
    //! default-constructed tPrintParameters are emitted (empty object means “all defaults”).
    /// @param[in]  sScope  All = merged API snapshot; WorkBook / Sheet = file split.
    void JsonWrite(Writer<StringBuffer>* sWriter, tPrintJsonScope sScope = tPrintJsonScope::All) const;

    //! Sparse JSON identical to JsonWrite (UTF-8 string).
    tString JsonString(tPrintJsonScope sScope = tPrintJsonScope::All) const;

    //! Fills from a sparse JSON object (same keys as JsonWrite): starts from defaults, then overlays known members.
    //! Unknown keys are ignored. Returns false when the payload is not a JSON object, or when a recognized key exists with the wrong JSON type.
    tBool JsonRead(const rapidjson::Value& sValue);

    //! Parses UTF-8 JSON (must encode a single JSON object); then forwards to JsonRead.
    tBool JsonParse(const tString& sJson);
    //! Parses JSON from a UTF-8 buffer without requiring a terminator at sLength (RapidJSON kParse-stop offset).
    tBool JsonParse(const tChar* sUtf8, size_t sLength);

    //! Copy paper/margins/scale/… from sSrc; leave m_Orientation and m_FitToPage unchanged.
    void CopyWorkBookFieldsFrom(const tPrintParameters& sSrc);

    //! Reset m_Orientation and m_FitToPage to constructor defaults.
    void ClearSheetFields();

    //! True when every workbook-shared field matches a default-constructed instance.
    tBool WorkBookFieldsAreDefault() const;
};

} // namespace SkSpreadSheet

#endif /* SkPrintParameters_hpp */
