//=============================================================================
// SkSpreadSheet Range Data Array
//=============================================================================
#ifndef SkRangeData_hpp
#define SkRangeData_hpp

#include "SkTools.hpp"
#include "SkRange.hpp"
#include "SkRangeFilter.hpp"
#include "SkRangeSort.hpp"
#include "SkJsonKey.hpp"

#include <map>

namespace SkSpreadSheet {

    class tSheet;
    class tWorkBook;

    class tColumnData : public tClass {
    private:
        //! Absolute sheet column index (A=1).
        tIndex m_SheetCol;
        //! Type of the column
        tSharedString m_Type;
        //! Filter operator
        tFilterOperator m_FilterOperator;
        //! Filter value
        tVariant m_FilterValue;
        //! Optional multi-value filter (OR within the same column, Excel-style).
        vector<tVariant> m_FilterValues;
        //! Sort order
        tSortOrder m_SortOrder;
        //! OOXML autoFilter filterColumn@hiddenButton — suppress header filter dropdown UI.
        tBool m_FilterButtonHidden = false;
        //! Fallback label when header cell is blank, formula, or error; also legacy JSON "name" on import.
        tSharedString m_ImportLabel;
        //! OOXML <calculatedColumnFormula> round-trip (empty = infer from cells on export).
        tSharedString m_CalculatedColumnFormula;
        //! OOXML tableColumn totals row metadata (round-trip).
        tSharedString m_TotalsRowLabel;
        tSharedString m_TotalsRowFunction;
        tSharedString m_TotalsRowFormula;
    public:
        /// @brief Constructor
        tColumnData();

        /// @brief Copy constructor
        /// @param sColumnData tColumnData*
        tColumnData(tColumnData* sColumnData);

        /// @brief Constructor
        /// @param sSheetCol Absolute sheet column index
        /// @param sType tString
        /// @param sSortOrder tSortOrder
        tColumnData(tIndex sSheetCol, tString sType, tSortOrder sSortOrder=tSortOrder::None);

        /// @brief Legacy constructor (sIndex may be offset or absolute; normalized later).
        tColumnData(tIndex sIndex, tString sName, tString sType, tSortOrder sSortOrder);

        /// @brief Destructor
        ~tColumnData();

        /// @brief Copy filter/sort/type from another descriptor (sheet column unchanged).
        void CopyMetadataFrom(const tColumnData& sOther);

        /// @brief Adjust stored sheet columns after insert/delete.
        void ShiftSheetCol(tIndex sFromCol, tIndex sDelta);

        /// @brief Normalize legacy JSON column key to absolute sheet column.
        void SetSheetCol(tIndex sSheetCol);

        /// @brief Store fallback header label (plain-text headers or JSON import).
        void SetImportLabel(tString sLabel);

        /// @brief Fallback label from JSON import or plain-text header capture.
        tString ImportLabel() const;

        /// @brief OOXML calculated-column template from import (may differ from per-row cell text).
        tString CalculatedColumnFormula() const;
        void SetCalculatedColumnFormula(tString sFormula);

        tString TotalsRowLabel() const;
        void SetTotalsRowLabel(tString sValue);
        tString TotalsRowFunction() const;
        void SetTotalsRowFunction(tString sValue);
        tString TotalsRowFormula() const;
        void SetTotalsRowFormula(tString sFormula);

        /// @brief Column type string from RangeData metadata.
        tString Type() const;

        /// @brief
        /// @param sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief Serialize column; sSnapshotLabel is written as JSON "name" when non-empty.
        void Json(Writer<StringBuffer>* sWriter, const tString& sSnapshotLabel);

        /// @brief
        /// @param sValue const rapidjson::Value&
        void Json(const rapidjson::Value& sValue);

        /// @brief
        /// @param sJson tString
        void Json(tString sJson);

        /// @brief
        /// @return tString
        tString Json();

        /// @brief Absolute sheet column index.
        /// @return tIndex
        tIndex SheetCol() const;

        /// @brief Legacy alias for SheetCol().
        tIndex Index() const;

        /// @brief 0-based offset within a table whose left edge is sRangeLeft.
        tIndex Ordinal(tIndex sRangeLeft) const;

        /// @brief Header text from the sheet (fallback Column{N} when blank).
        tString HeaderLabel(tSheet* sSheet, tIndex sHeaderRow, tIndex sRangeLeft) const;

        /// @brief Legacy alias: requires sheet context; returns empty when sSheet is null.
        tString Name() const;

        /// @brief Get sort order
        /// @return tSortOrder
        tSortOrder SortOrder() const;

        /// @brief Get filter operator
        /// @return tFilterOperator
        tFilterOperator FilterOperator() const;

        /// @brief Whether the Excel header filter dropdown button is hidden for this column.
        tBool FilterButtonHidden() const { return m_FilterButtonHidden; }
        void FilterButtonHidden(tBool sValue) { m_FilterButtonHidden = sValue; }

        /// @brief Get filter value
        /// @return tVariant
        tVariant FilterValue() const;

        /// @brief Get multi-value filter list (empty when single-value filter is used).
        /// @return const vector<tVariant>&
        const vector<tVariant>& FilterValues() const;
#ifdef _DEBUGSK
        /// @brief Debug
        /// @return tString
        tString Debug(tSheet* sSheet, tIndex sHeaderRow, tIndex sRangeLeft) const;
#endif
    };

    typedef vector<tColumnData> tColumnDataVector;

    class tRangeData : public tClass {
    private:
        //! Use first row as header
        tBool m_UseFirstRowAsHeader;
        //! Use last row as total
        tBool m_LastRowAsTotalRow;

        //! Excel <tableStyleInfo> round-trip (empty = inherit workbook default on export).
        tString m_TableStyleName;
        tBool m_TableShowRowStripes = false;
        tBool m_TableShowColumnStripes = false;
        tBool m_TableShowFirstColumn = false;
        tBool m_TableShowLastColumn = false;
        tBool m_TableAutoFilter = false;
        //! OOXML table@displayName round-trip (empty = derive on export).
        tString m_TableDisplayName;
        //! OOXML totalsRowCount (0 = none; legacy lastrow:true implies 1).
        tInt m_TotalsRowCount = 0;
        //! OOXML <tableStyleElement> visuals resolved at import (type -> CSS fragment).
        std::map<tString, tString> m_TableStyleElementCss;

        //! Column Data Array Vector (sorted by SheetCol when synced)
        tColumnDataVector m_ColumnDataVector;

        /// @brief Convert legacy JSON index/name fields to absolute sheet columns.
        void NormalizeLegacyColumnCols(tIndex sRangeLeft, tIndex sRangeRight);
    public:
        /// @brief Constructor
        tRangeData();

        /// @brief Copy constructor
        /// @param sRangeData tRangeData*
        tRangeData(tRangeData* sRangeData);

        /// @brief Destructor
        ~tRangeData();

        /// @brief Clear column descriptors only (header/total flags unchanged).
        void Clear();

        /// @brief Rebuild column descriptors for sRect; preserve filter/sort by sheet column.
        /// @param sSheet Non-null sheet owning the cells.
        /// @param sRect Rectangle satisfying tRect::IsValid().
        /// @return false if sSheet is null, rectangle invalid, Height() less than 2, or no columns.
        tBool FillByRect(tSheet* sSheet, tRect sRect);

        /// @brief Align descriptors with table geometry; preserve filter/sort by sheet column.
        void SyncFromRange(tSheet* sSheet, tRange* sRange);

        /// @brief Shift stored sheet columns at/after sFromCol (call before SyncFromRange on insert).
        void ShiftSheetCols(tIndex sFromCol, tIndex sDelta);

        /// @brief Find column by live header label.
        tColumnData* FindColumnByName(tSheet* sSheet, tRange* sRange, tString sName);

        /// @brief False when sLabel (normalized) is already used by another column in this table.
        tBool CanAssignHeaderLabel(tSheet* sSheet, tRange* sRange, tIndex sSheetCol, tString sLabel) const;

        /// @brief Reject header-cell edits that would duplicate another column name in the same table.
        static tBool ValidateHeaderCellValue(tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow, tIndex sCol,
                                             const tVariant& sVariant, tString* sErrorMessage = nullptr);

        /// @brief Persist live header text into import label after a successful manual edit.
        void CaptureHeaderLabelFromCell(tSheet* sSheet, tRange* sRange, tIndex sSheetCol, tString sLabel);

        /// @brief Legacy find (no sheet): compares fallback Column{N} labels only.
        tColumnData* FindColumnByName(tString sName);

        /// @brief Find column by 0-based ordinal within sRange (left + ordinal).
        tColumnData* FindColumnByOrdinal(tIndex sRangeLeft, tSize sOrdinal);

        /// @brief Legacy alias for FindColumnByOrdinal when sRangeLeft is unknown (linear scan by stored offset).
        tColumnData* FindColumnByIndex(tSize sIndex);

        /// @brief True if no column descriptors
        /// @return tBool
        tBool IsEmpty();

        /// @brief Whether the range is described as having a header row
        /// @return tBool
        tBool HasHeaders() const;

        /// @brief Whether the range uses the last row as totals
        /// @return tBool
        tBool HasTotals();

        /// @brief OOXML totalsRowCount (legacy lastrow:true without count => 1).
        tInt TotalsRowCount() const;
        void TotalsRowCount(tInt sValue);

        tString TableDisplayName() const;
        void TableDisplayName(tString sValue);

        tString TableStyleName() const;
        void TableStyleName(tString sValue);
        tBool TableShowRowStripes() const;
        void TableShowRowStripes(tBool sValue);
        tBool TableShowColumnStripes() const;
        void TableShowColumnStripes(tBool sValue);
        tBool TableShowFirstColumn() const;
        void TableShowFirstColumn(tBool sValue);
        tBool TableShowLastColumn() const;
        void TableShowLastColumn(tBool sValue);
        tBool TableAutoFilter() const;
        void TableAutoFilter(tBool sValue);
        void SetTableStyleElementCss(const tString& sType, tString sCss);
        const std::map<tString, tString>& TableStyleElementCss() const;
        tBool HasTableStyleElementCss() const;

        /// @brief Append a column at the back of the column vector.
        void AddColumn(tColumnData sColumnData);

        /// @brief Column descriptors (absolute sheet columns when synced).
        const tColumnDataVector& Columns() const { return m_ColumnDataVector; }

        /// @brief
        /// @param sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief Serialize table metadata; snapshots live header labels when sheet context is known.
        void Json(Writer<StringBuffer>* sWriter, tSheet* sSheet, tIndex sHeaderRow, tIndex sRangeLeft);

        /// @brief
        /// @param sValue const rapidjson::Value&
        /// @param sRangeLeft When >= 0, normalizes legacy "index" values after load.
        /// @param sRangeRight When >= 0, used with sRangeLeft for legacy normalization.
        void Json(const rapidjson::Value& sValue, tIndex sRangeLeft=-1, tIndex sRangeRight=-1);

        /// @brief
        /// @param sJson tString
        void Json(tString sJson);

        /// @brief Sort
        /// @param sRange tRange*
        void Sort(tRange* sRange);

        /// @brief Filter
        /// @param sRange tRange*
        void Filter(tRange* sRange);

        /// @brief
        /// @return tString
        tString Json();
#ifdef _DEBUGSK
        /// @brief Debug
        /// @return tString
        tString Debug(tSheet* sSheet, tRange* sRange);
#endif
    };
}

#endif // SkRangeData_hpp
