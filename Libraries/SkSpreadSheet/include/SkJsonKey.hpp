//
//  SkJsonKey.hpp
//  SkSpreadSheet
//
//  Created by stephane allez on 21/11/2025.
//
#ifndef SkJsonKey_hpp
#define SkJsonKey_hpp

#define _jsondebugkey
//! Human-readable JSON keys when `jsondebug` is defined; short keys otherwise (wire size).
//! Pass -Djsondebug (or define jsondebug in the build) for verbose JSON.

namespace SkSpreadSheet {

inline namespace JsonKeys {

inline constexpr const char* kJsonKeySharedString = "si";
inline constexpr const char* kJsonKeySharedFormula = "fi";

// User identity fields used in SkMessage
inline constexpr const char* kJsonKeyName = "nm";
inline constexpr const char* kJsonKeyEmail = "em";
inline constexpr const char* kJsonKeyFirstName = "fn";
inline constexpr const char* kJsonKeyUser = "usr";
inline constexpr const char* kJsonKeyUri = "uri";
inline constexpr const char* kJsonKeyOp = "op";
inline constexpr const char* kJsonKeyUndo = "ud";
inline constexpr const char* kJsonKeyMsg = "msg";
inline constexpr const char* kJsonKeySize = "sz";
inline constexpr const char* kJsonKeyStackSize = "ssz";
inline constexpr const char* kJsonKeyRef = "rf";
inline constexpr const char* kJsonKeySave = "sv";
inline constexpr const char* kJsonKeyKeepFormat = "kf";
inline constexpr const char* kJsonKeyFormatOnly = "fo";
inline constexpr const char* kJsonKeyFormat = "fmt";
inline constexpr const char* kJsonKeyClassName = "cn";
inline constexpr const char* kJsonKeyApplyNumeric = "an";
inline constexpr const char* kJsonKeyAttribute = "at";
inline constexpr const char* kJsonKeyAttributes = "ats";
inline constexpr const char* kJsonKeyCopy = "cp";
inline constexpr const char* kJsonKeyMoveSource = "msr";
inline constexpr const char* kJsonKeyDidGridRelocate = "dgr";
inline constexpr const char* kJsonKeyGridRelocateMoved = "grm";
inline constexpr const char* kJsonKeyProjectedBorderNeighbors = "pbn";
inline constexpr const char* kJsonKeyPosition = "ps";
//! Tree indent (true) / outdent (false) for tUndoChangeTree
inline constexpr const char* kJsonKeyTreeRight = "tr";
inline constexpr const char* kJsonKeySaveSelectErase = "svse";
inline constexpr const char* kJsonKeyWorkBook = "wb";
inline constexpr const char* kJsonKeyNewName = "nnm";
inline constexpr const char* kJsonKeySheetLeft = "shl";
inline constexpr const char* kJsonKeyBorder = "brd";
inline constexpr const char* kJsonKeyIsRow = "ir";
/** Target open state for tUndoOpenCloseTree (absolute, not toggle). */
inline constexpr const char* kJsonKeyTreeOpen = "to";
inline constexpr const char* kJsonKeyCell = "c";
inline constexpr const char* kJsonKeyBegin = "bg";
inline constexpr const char* kJsonKeyEnd = "ed";
inline constexpr const char* kJsonKeyDependent = "d";
inline constexpr const char* kJsonKeySelection = "sel";
inline constexpr const char* kJsonKeyFormula = "f";
inline constexpr const char* kJsonKeyOldFormula = "fold";
inline constexpr const char* kJsonKeyOldRef = "orf";
inline constexpr const char* kJsonKeyIndex = "i";
inline constexpr const char* kJsonKey = "ky";
inline constexpr const char* kJsonKeyTypeCell = "ty";
inline constexpr const char* kJsonKeyRefName = "rn";
inline constexpr const char* kJsonKeyFormulaCell = "fcell";
inline constexpr const char* kJsonKeyRange = "r";
inline constexpr const char* kJsonKeyMerged = "m";
inline constexpr const char* kJsonKeyListCell = "l_c";
inline constexpr const char* kJsonKeyListRange = "l_r";
inline constexpr const char* kJsonKeyFormatString = "fs";
inline constexpr const char* kJsonKeyColRow = "cr";
inline constexpr const char* kJsonKeyIndexColRow = "i";
inline constexpr const char* kJsonKeyParent = "p";
inline constexpr const char* kJsonKeyChildren = "ch";
inline constexpr const char* kJsonKeyRangeArray = "ra";
inline constexpr const char* kJsonKeyMergedRange = "mr";
inline constexpr const char* kJsonKeyDeleted = "del";
inline constexpr const char* kJsonKeyRazDeletedTables = "raztbl";
inline constexpr const char* kJsonKeyCoordAfter = "aft";
inline constexpr const char* kJsonKeySaveConditionalFormatList = "scf";
inline constexpr const char* kJsonKeySheet = "sh";
inline constexpr const char* kJsonKeySheetjson = "shj";
inline constexpr const char* kJsonKeyOldName = "onm";
inline constexpr const char* kJsonKeyDependentArray = "dep";
inline constexpr const char* kJsonKeyRect = "rect";
inline constexpr const char* kJsonKeyIsRect = "isrect";
inline constexpr const char* kJsonKeyDoesRow = "doesrow";
inline constexpr const char* kJsonKeyRectDeleteArea = "rc";
inline constexpr const char* kJsonKeyUniqueRange = "ur";
inline constexpr const char* kJsonKeyCoveredRange = "cr";
inline constexpr const char* kJsonKeyRangeNamed = "nr";
inline constexpr const char* kJsonKeyNamedMergedRange = "nmr";
inline constexpr const char* kJsonKeyName1 = "n1";
inline constexpr const char* kJsonKeyName2 = "n2";
inline constexpr const char* kJsonKeySheetAllocator = "sa";
inline constexpr const char* kJsonKeyInc = "in";
inline constexpr const char* kJsonKeyOperationId = "opid";
inline constexpr const char* kJsonKeyJsonPayload = "jp";
inline constexpr const char* kJsonKeyMapSaveJsonPayLoad = "mjp";
inline constexpr const char* kJsonKeyExtension = "ex";
inline constexpr const char* kJsonKeySplitV = "sv";
inline constexpr const char* kJsonKeySplitH = "sh";
inline constexpr const char* kJsonKeySplitViewCmd = "svm";
inline constexpr const char* kJsonKeySplitViewPrevV = "spv";
inline constexpr const char* kJsonKeySplitViewPrevH = "sph";
inline constexpr const char* kJsonKeySplitViewCaptured = "scp";
inline constexpr const char* kJsonKeyViewZoom = "viewZoom";
inline constexpr const char* kJsonKeyShowGridLines = "showGridLines";

// Sheet print layout JSON (tPrintParameters); stable camelCase independent of jsondebugkey wire keys.
inline constexpr const char* kJsonKeyPaperSize = "paperSize";
inline constexpr const char* kJsonKeyOrientation = "orientation";
inline constexpr const char* kJsonKeyScale = "scale";
inline constexpr const char* kJsonKeyFitToPage = "fitToPage";
inline constexpr const char* kJsonKeyFitToWidthPages = "fitToWidthPages";
inline constexpr const char* kJsonKeyFitToHeightPages = "fitToHeightPages";
inline constexpr const char* kJsonKeyPageOrder = "pageOrder";
inline constexpr const char* kJsonKeyBlackAndWhite = "blackAndWhite";
inline constexpr const char* kJsonKeyDraft = "draft";
inline constexpr const char* kJsonKeyUsePrinterDefaults = "usePrinterDefaults";
inline constexpr const char* kJsonKeyHorizontalDpi = "horizontalDpi";
inline constexpr const char* kJsonKeyVerticalDpi = "verticalDpi";
inline constexpr const char* kJsonKeyCopies = "copies";
inline constexpr const char* kJsonKeyFirstPageNumber = "firstPageNumber";
inline constexpr const char* kJsonKeyMarginLeft = "marginLeft";
inline constexpr const char* kJsonKeyMarginRight = "marginRight";
inline constexpr const char* kJsonKeyMarginTop = "marginTop";
inline constexpr const char* kJsonKeyMarginBottom = "marginBottom";
inline constexpr const char* kJsonKeyMarginHeader = "marginHeader";
inline constexpr const char* kJsonKeyMarginFooter = "marginFooter";
inline constexpr const char* kJsonKeyPrintGridLines = "printGridLines";
inline constexpr const char* kJsonKeyPrintHeadings = "printHeadings";
inline constexpr const char* kJsonKeyHorizontalCentered = "horizontalCentered";
inline constexpr const char* kJsonKeyVerticalCentered = "verticalCentered";
inline constexpr const char* kJsonKeyAutoPageBreaks = "autoPageBreaks";

//! Nested JSON object holding sparse tPrintParameters keys.
//! On the workbook root: shared layout (no orientation / fitToPage).
//! On each sheet: per-sheet overrides (orientation + fitToPage only). Legacy files may still
//! store the full object on the sheet — load migrates workbook fields onto tWorkBook.
inline constexpr const char* kJsonKeySheetPrintParameters = "printParameters";
//! Undo `tUndoPrintParameters`: whether Do() captured `printParametersPrev` for Undo/Redo replay.
inline constexpr const char* kJsonKeySheetPrintParametersCaptured = "printParametersCaptured";
//! Undo rollback snapshot (sparse object, same shape as `printParameters`).
inline constexpr const char* kJsonKeySheetPrintParametersPrev = "printParametersPrev";

// Data External interface
inline constexpr const char* kJsonKeyData = "data";
inline constexpr const char* kJsonKeyFilterOperator = "filterop";
inline constexpr const char* kJsonKeyFilterValue = "filtervalue";
inline constexpr const char* kJsonKeyFilterValues = "filtervalues";
//! OOXML <filterColumn hiddenButton="1"> — Excel hides the header dropdown affordance.
inline constexpr const char* kJsonKeyFilterButtonHidden = "filterButtonHidden";
inline constexpr const char* kJsonKeySortOrder = "order";
inline constexpr const char* kJsonKeySortIndex = "index";
//! Absolute 1-based sheet column (preferred over legacy "index" offset).
inline constexpr const char* kJsonKeyColumnCol = "col";
inline constexpr const char* kJsonKeyColumnDataVector = "columns";
inline constexpr const char* kJsonKeyUseFirstRowAsHeader = "firstrow";
inline constexpr const char* kJsonKeyLastRowAsTotalRow = "lastrow";
// Excel ListObject tableStyleInfo round-trip (import xlsx -> sker -> xlsx).
inline constexpr const char* kJsonKeyTableStyleName = "tableStyleName";
inline constexpr const char* kJsonKeyTableShowRowStripes = "tableShowRowStripes";
inline constexpr const char* kJsonKeyTableShowColumnStripes = "tableShowColumnStripes";
inline constexpr const char* kJsonKeyTableShowFirstColumn = "tableShowFirstColumn";
inline constexpr const char* kJsonKeyTableShowLastColumn = "tableShowLastColumn";
// Resolved OOXML table-style element CSS (headerRow, wholeTable, firstRowStripe, ...).
inline constexpr const char* kJsonKeyTableStyleElements = "tableStyleElements";
inline constexpr const char* kJsonKeyTableAutoFilter = "tableAutoFilter";
inline constexpr const char* kJsonKeyTableDisplayName = "tableDisplayName";
inline constexpr const char* kJsonKeyTotalsRowCount = "totalsRowCount";
inline constexpr const char* kJsonKeyTotalsRowLabel = "totalsRowLabel";
inline constexpr const char* kJsonKeyTotalsRowFunction = "totalsRowFunction";
inline constexpr const char* kJsonKeyTotalsRowFormula = "totalsRowFormula";
//! A1 ref of the label cell written with an insert-row undo (table totals row).
inline constexpr const char* kJsonKeyLabelRef = "lrf";
//! Plain-text label written after insert (e.g. "Total").
inline constexpr const char* kJsonKeyLabelValue = "lv";
inline constexpr const char* kJsonKeyOldData = "odata";
// Column
inline constexpr const char* kJsonKeyIndexData = "index";
inline constexpr const char* kJsonKeyNameData = "name";
inline constexpr const char* kJsonKeyTypeData = "type";
//! OOXML <calculatedColumnFormula> template (import xlsx -> sker -> xlsx).
inline constexpr const char* kJsonKeyCalculatedColumnFormula = "calculatedColumnFormula";
// Conditional Format
inline constexpr const char* kJsonKeyType = "type";
inline constexpr const char* kJsonKeyConditionalFormats = "cf";
inline constexpr const char* kJsonKeyConditionalFormatKey = "key";
inline constexpr const char* kJsonKeyConditionalFormatHR = "cfhr";
inline constexpr const char* kJsonKeyConditionalFormatDB = "cfdb";
inline constexpr const char* kJsonKeyConditionalFormatCS = "cfcs";
inline constexpr const char* kJsonKeyConditionalFormatIS = "cfis";
inline constexpr const char* kJsonKeyConditionalFormatCF = "cfcf";
inline constexpr const char* kJsonKeyConditionalFormatType = "conditionalformattype";
inline constexpr const char* kJsonKeyIconSetType = "iconsettype";
inline constexpr const char* kJsonKeyParam1 = "param1";
inline constexpr const char* kJsonKeyParam2 = "param2";
inline constexpr const char* kJsonKeyParam3 = "param3";
inline constexpr const char* kJsonKeyParam4 = "param4";
inline constexpr const char* kJsonKeyParam5 = "param5";
inline constexpr const char* kJsonKeyParam6 = "param6";
inline constexpr const char* kJsonKeyParam7 = "param7";
inline constexpr const char* kJsonKeyParam8 = "param8";
inline constexpr const char* kJsonKeyParam9 = "param9";
inline constexpr const char* kJsonKeyParam10 = "param10";
inline constexpr const char* kJsonKeyPercent = "percent";
inline constexpr const char* kJsonKeyColor = "color";
inline constexpr const char* kJsonKeyDirection = "direction";
inline constexpr const char* kJsonKeyStyle = "style";
inline constexpr const char* kJsonKeyMinValue = "minValue";
inline constexpr const char* kJsonKeyMaxValue = "maxValue";
inline constexpr const char* kJsonKeyIconString = "iconString";

//Anchor cell JSON keys
inline constexpr const char* kJsonKeyAnchorCell = "anchorCell";
inline constexpr const char* kJsonKeyDiffX = "diffX";
inline constexpr const char* kJsonKeyDiffY = "diffY";
inline constexpr const char* kJsonKeyWidth = "width";
inline constexpr const char* kJsonKeyHeight = "height";
inline constexpr const char* kJsonKeyOpacity = "opacity";
inline constexpr const char* kJsonKeyZIndex = "zIndex";
inline constexpr const char* kJsonKeyZIndexCompact = "zi";

//! Cell-class model schemas (tClassFactory / RegisterClassAttribute wire format).
inline constexpr const char* kJsonKeyModels = "models";

inline constexpr const char* kJsonKeyFloatingObjects = "floatingobjects";
inline constexpr const char* kJsonKeyTargetSheet = "tg";
inline constexpr const char* kJsonKeyExistedBefore = "ex";
inline constexpr const char* kJsonKeyHostRow = "r";
inline constexpr const char* kJsonKeyLayoutOld = "ol";
inline constexpr const char* kJsonKeyLayoutNew = "nl";

} // namespace JsonKeys

} // namespace SkSpreadSheet

#endif /* SkJsonKey_hpp */
