//=============================================================================
// SkSpreadSheet Range Data Array
//=============================================================================
#include "../include/SkTools.hpp"
#include "../include/SkRangeData.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkWorkBook.hpp"
#include <algorithm>
#include <cctype>
#include <map>
namespace SkSpreadSheet {


namespace {

    tBool UsesLegacyAbsoluteColumnIndices(const tColumnDataVector& sColumns,
                                          tIndex sRangeLeft, tIndex sRangeRight) {
        if (sColumns.empty() || sRangeRight < sRangeLeft) {
            return false;
        }
        const tIndex wMaxOffset = sRangeRight - sRangeLeft;
        tIndex wMaxStored = 0;
        for (const auto& wCol : sColumns) {
            wMaxStored = std::max(wMaxStored, wCol.SheetCol());
        }
        return wMaxStored > wMaxOffset;
    }

    tBool UsesZeroBasedLegacyOffsets(const tColumnDataVector& sColumns) {
        for (const auto& wCol : sColumns) {
            if (wCol.SheetCol() == 0) {
                return true;
            }
        }
        return false;
    }

    tBool AlreadyNormalizedSheetCols(const tColumnDataVector& sColumns,
                                     tIndex sRangeLeft, tIndex sRangeRight) {
        if (sColumns.empty() || sRangeRight < sRangeLeft) {
            return false;
        }
        const tIndex wWidth = sRangeRight - sRangeLeft + 1;
        if (static_cast<tIndex>(sColumns.size()) < wWidth) {
            return false;
        }
        std::vector<tBool> wSeen(static_cast<size_t>(wWidth), false);
        for (const auto& wCol : sColumns) {
            const tIndex wSheetCol = wCol.SheetCol();
            if (wSheetCol < sRangeLeft || wSheetCol > sRangeRight) {
                return false;
            }
            wSeen[static_cast<size_t>(wSheetCol - sRangeLeft)] = true;
        }
        for (tBool wHit : wSeen) {
            if (!wHit) {
                return false;
            }
        }
        return true;
    }

    tIndex NormalizeLegacyStoredCol(tIndex sStored, tIndex sRangeLeft, tIndex sRangeRight,
                                    tBool sAbsolute, tBool sZeroBased) {
        if (sRangeLeft < 1 || sRangeRight < sRangeLeft) {
            return sStored;
        }
        if (sAbsolute) {
            return sStored;
        }
        if (sZeroBased) {
            return sRangeLeft + sStored;
        }
        // Legacy 1-based offset within the table (Excel import / some tests).
        return sRangeLeft + sStored - 1;
    }

    tIndex RelativeColIndex(tIndex sSheetCol, tRange* sRange) {
        if (sRange == nullptr) {
            return sSheetCol;
        }
        return sSheetCol - sRange->LeftIndex();
    }

    // Excel often stores wrapped headers ("Extra\\npayment"); table refs use "Extra payment".
    tString NormalizeTableColumnLabel(tString sLabel) {
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

    tString ReadHeaderCellText(tSheet* sSheet, tIndex sHeaderRow, tIndex sSheetCol) {
        if (sSheet == nullptr) {
            return "";
        }
        tCell* wCell = sSheet->Cell(sHeaderRow, sSheetCol);
        if (wCell == nullptr) {
            return "";
        }
        // Header labels come from static text; formula/error cells fall back to import label.
        if (wCell->Formula() != nullptr || wCell->Value().IsError()) {
            return "";
        }
        tString wRaw = wCell->Value().Str();
        if (wRaw.empty()) {
            tWorkBook* wWorkBook = sSheet->WorkBook();
            if (wWorkBook != nullptr) {
                wRaw = wWorkBook->CellFormatString(wCell);
            }
        }
        return NormalizeTableColumnLabel(wRaw);
    }

} // namespace

    tColumnData::tColumnData() :
    tClass(),
    m_SheetCol(1),
    m_Type(""),
    m_FilterOperator(tFilterOperator::None),
    m_FilterValue(tVariant()),
    m_SortOrder(tSortOrder::None) {}

    tColumnData::tColumnData(tIndex sSheetCol, tString sType, tSortOrder sSortOrder) :
        tClass(),
        m_SheetCol(sSheetCol),
        m_Type(sType),
        m_FilterOperator(tFilterOperator::None),
        m_FilterValue(tVariant()),
        m_SortOrder(sSortOrder) {}

    tColumnData::tColumnData(tIndex sIndex, tString sName, tString sType, tSortOrder sSortOrder) :
        tColumnData(sIndex, sType, sSortOrder) {
        m_ImportLabel = sName;
    }

    tColumnData::tColumnData(tColumnData* sColumnData) :
        tClass(),
        m_SheetCol(sColumnData->m_SheetCol),
        m_Type(sColumnData->m_Type),
        m_FilterOperator(sColumnData->m_FilterOperator),
        m_FilterValue(sColumnData->m_FilterValue),
        m_FilterValues(sColumnData->m_FilterValues),
        m_SortOrder(sColumnData->m_SortOrder),
        m_FilterButtonHidden(sColumnData->m_FilterButtonHidden),
        m_ImportLabel(sColumnData->m_ImportLabel),
        m_CalculatedColumnFormula(sColumnData->m_CalculatedColumnFormula),
        m_TotalsRowLabel(sColumnData->m_TotalsRowLabel),
        m_TotalsRowFunction(sColumnData->m_TotalsRowFunction),
        m_TotalsRowFormula(sColumnData->m_TotalsRowFormula) {}

    tColumnData::~tColumnData() {}

    void tColumnData::CopyMetadataFrom(const tColumnData& sOther) {
        m_Type = sOther.m_Type;
        m_FilterOperator = sOther.m_FilterOperator;
        m_FilterValue = sOther.m_FilterValue;
        m_FilterValues = sOther.m_FilterValues;
        m_SortOrder = sOther.m_SortOrder;
        m_FilterButtonHidden = sOther.m_FilterButtonHidden;
        m_ImportLabel = sOther.m_ImportLabel;
        m_CalculatedColumnFormula = sOther.m_CalculatedColumnFormula;
        m_TotalsRowLabel = sOther.m_TotalsRowLabel;
        m_TotalsRowFunction = sOther.m_TotalsRowFunction;
        m_TotalsRowFormula = sOther.m_TotalsRowFormula;
    }

    void tColumnData::ShiftSheetCol(tIndex sFromCol, tIndex sDelta) {
        if (m_SheetCol >= sFromCol) {
            m_SheetCol += sDelta;
        }
    }

    void tColumnData::SetSheetCol(tIndex sSheetCol) {
        m_SheetCol = sSheetCol;
    }

    void tColumnData::SetImportLabel(tString sLabel) {
        m_ImportLabel = std::move(sLabel);
    }

    tString tColumnData::ImportLabel() const {
        return m_ImportLabel();
    }

    tString tColumnData::CalculatedColumnFormula() const {
        return m_CalculatedColumnFormula();
    }

    void tColumnData::SetCalculatedColumnFormula(tString sFormula) {
        if (!sFormula.empty() && sFormula.front() == '=') {
            sFormula.erase(0, 1);
        }
        m_CalculatedColumnFormula = std::move(sFormula);
    }

    tString tColumnData::TotalsRowLabel() const {
        return m_TotalsRowLabel();
    }

    void tColumnData::SetTotalsRowLabel(tString sValue) {
        m_TotalsRowLabel = std::move(sValue);
    }

    tString tColumnData::TotalsRowFunction() const {
        return m_TotalsRowFunction();
    }

    void tColumnData::SetTotalsRowFunction(tString sValue) {
        m_TotalsRowFunction = std::move(sValue);
    }

    tString tColumnData::TotalsRowFormula() const {
        return m_TotalsRowFormula();
    }

    void tColumnData::SetTotalsRowFormula(tString sFormula) {
        if (!sFormula.empty() && sFormula.front() == '=') {
            sFormula.erase(0, 1);
        }
        m_TotalsRowFormula = std::move(sFormula);
    }

    tString tColumnData::Type() const {
        return m_Type();
    }

    namespace {

    void CaptureImportLabelFromHeader(tColumnData& sColumnData, tSheet* sSheet, tIndex sHeaderRow,
                                      tRange* sRange, const tRangeData* sRangeData) {
        if (sSheet == nullptr) {
            return;
        }
        // headerRowCount=0 tables keep OOXML tableColumn@name labels; row 1 is data, not headers.
        if (sRangeData != nullptr && !sRangeData->HasHeaders()) {
            return;
        }
        const tString wLive = ReadHeaderCellText(sSheet, sHeaderRow, sColumnData.SheetCol());
        if (wLive.empty()) {
            return;
        }
        if (sRangeData != nullptr && sRange != nullptr
            && !sRangeData->CanAssignHeaderLabel(sSheet, sRange, sColumnData.SheetCol(), wLive)) {
            return;
        }
        sColumnData.SetImportLabel(wLive);
    }

    void WriteColumnJsonObject(const tColumnData& sColumnData, Writer<StringBuffer>* sWriter,
                               const tString* sSnapshotLabel) {
        sWriter->StartObject();
        sWriter->Key(kJsonKeyColumnCol);
        sWriter->Int(sColumnData.SheetCol());
        sWriter->Key(kJsonKeyTypeData);
        sWriter->String(sColumnData.Type().c_str());
        sWriter->Key(kJsonKeyFilterOperator);
        sWriter->String(FilterOperatorToString(sColumnData.FilterOperator()));
        sWriter->Key(kJsonKeyFilterValue);
        sWriter->StartObject();
        sColumnData.FilterValue().Json(sWriter);
        sWriter->EndObject();
        const vector<tVariant>& wFilterValues = sColumnData.FilterValues();
        if (!wFilterValues.empty()) {
            sWriter->Key(kJsonKeyFilterValues);
            sWriter->StartArray();
            for (const auto& wValue : wFilterValues) {
                sWriter->StartObject();
                wValue.Json(sWriter);
                sWriter->EndObject();
            }
            sWriter->EndArray();
        }
        sWriter->Key(kJsonKeySortOrder);
        sWriter->String(SortOrderToString(sColumnData.SortOrder()));
        if (sColumnData.FilterButtonHidden()) {
            sWriter->Key(kJsonKeyFilterButtonHidden);
            sWriter->Bool(true);
        }
        const tString wCalculatedColumnFormula = sColumnData.CalculatedColumnFormula();
        if (!wCalculatedColumnFormula.empty()) {
            sWriter->Key(kJsonKeyCalculatedColumnFormula);
            sWriter->String(wCalculatedColumnFormula.c_str());
        }
        const tString wTotalsRowLabel = sColumnData.TotalsRowLabel();
        if (!wTotalsRowLabel.empty()) {
            sWriter->Key(kJsonKeyTotalsRowLabel);
            sWriter->String(wTotalsRowLabel.c_str());
        }
        const tString wTotalsRowFunction = sColumnData.TotalsRowFunction();
        if (!wTotalsRowFunction.empty()) {
            sWriter->Key(kJsonKeyTotalsRowFunction);
            sWriter->String(wTotalsRowFunction.c_str());
        }
        const tString wTotalsRowFormula = sColumnData.TotalsRowFormula();
        if (!wTotalsRowFormula.empty()) {
            sWriter->Key(kJsonKeyTotalsRowFormula);
            sWriter->String(wTotalsRowFormula.c_str());
        }
        if (sSnapshotLabel != nullptr && !sSnapshotLabel->empty()) {
            sWriter->Key(kJsonKeyNameData);
            sWriter->String(sSnapshotLabel->c_str());
        } else {
            const tString wImportLabel = sColumnData.ImportLabel();
            if (!wImportLabel.empty()) {
                sWriter->Key(kJsonKeyNameData);
                sWriter->String(wImportLabel.c_str());
            }
        }
        sWriter->EndObject();
    }

    } // namespace

    void tColumnData::Json(Writer<StringBuffer>* sWriter) {
        WriteColumnJsonObject(*this, sWriter, nullptr);
    }

    void tColumnData::Json(Writer<StringBuffer>* sWriter, const tString& sSnapshotLabel) {
        WriteColumnJsonObject(*this, sWriter, &sSnapshotLabel);
    }

    void tColumnData::Json(const rapidjson::Value& sValue) {
        if (!sValue.IsObject()) {
            return;
        }
        if (sValue.HasMember(kJsonKeyColumnCol)) {
            m_SheetCol = sValue[kJsonKeyColumnCol].GetInt();
        } else if (sValue.HasMember(kJsonKeySortIndex)) {
            m_SheetCol = sValue[kJsonKeySortIndex].GetInt();
        } else if (sValue.HasMember(kJsonKeyIndexData)) {
            m_SheetCol = sValue[kJsonKeyIndexData].GetInt();
        }
        if (sValue.HasMember(kJsonKeyTypeData)) {
            m_Type = sValue[kJsonKeyTypeData].GetString();
        }
        if (sValue.HasMember(kJsonKeyNameData)) {
            m_ImportLabel = sValue[kJsonKeyNameData].GetString();
        }
        if (sValue.HasMember(kJsonKeyFilterOperator)) {
            m_FilterOperator = StringToFilterOperator(sValue[kJsonKeyFilterOperator].GetString());
        } else {
            m_FilterOperator = tFilterOperator::None;
        }
        if (sValue.HasMember(kJsonKeyFilterValue)) {
            const rapidjson::Value& wFilterValue = sValue[kJsonKeyFilterValue];
            if (wFilterValue.IsObject()) {
                m_FilterValue.Json(wFilterValue);
            }
        }
        m_FilterValues.clear();
        if (sValue.HasMember(kJsonKeyFilterValues) && sValue[kJsonKeyFilterValues].IsArray()) {
            const rapidjson::Value& wFilterValues = sValue[kJsonKeyFilterValues];
            for (rapidjson::SizeType wI = 0; wI < wFilterValues.Size(); wI++) {
                if (!wFilterValues[wI].IsObject()) {
                    continue;
                }
                tVariant wValue;
                wValue.Json(wFilterValues[wI]);
                m_FilterValues.push_back(wValue);
            }
        }
        if (sValue.HasMember(kJsonKeySortOrder)) {
            m_SortOrder = StringToSortOrder(sValue[kJsonKeySortOrder].GetString());
        } else {
            m_SortOrder = tSortOrder::None;
        }
        if (sValue.HasMember(kJsonKeyFilterButtonHidden)) {
            m_FilterButtonHidden = sValue[kJsonKeyFilterButtonHidden].GetBool();
        } else {
            m_FilterButtonHidden = false;
        }
        if (sValue.HasMember(kJsonKeyCalculatedColumnFormula)
            && sValue[kJsonKeyCalculatedColumnFormula].IsString()) {
            SetCalculatedColumnFormula(sValue[kJsonKeyCalculatedColumnFormula].GetString());
        } else {
            m_CalculatedColumnFormula = "";
        }
        if (sValue.HasMember(kJsonKeyTotalsRowLabel)
            && sValue[kJsonKeyTotalsRowLabel].IsString()) {
            SetTotalsRowLabel(sValue[kJsonKeyTotalsRowLabel].GetString());
        } else {
            m_TotalsRowLabel = "";
        }
        if (sValue.HasMember(kJsonKeyTotalsRowFunction)
            && sValue[kJsonKeyTotalsRowFunction].IsString()) {
            SetTotalsRowFunction(sValue[kJsonKeyTotalsRowFunction].GetString());
        } else {
            m_TotalsRowFunction = "";
        }
        if (sValue.HasMember(kJsonKeyTotalsRowFormula)
            && sValue[kJsonKeyTotalsRowFormula].IsString()) {
            SetTotalsRowFormula(sValue[kJsonKeyTotalsRowFormula].GetString());
        } else {
            m_TotalsRowFormula = "";
        }
    }

    tIndex tColumnData::SheetCol() const {
        return m_SheetCol;
    }

    tIndex tColumnData::Index() const {
        return m_SheetCol;
    }

    tIndex tColumnData::Ordinal(tIndex sRangeLeft) const {
        return m_SheetCol - sRangeLeft;
    }

    tString tColumnData::HeaderLabel(tSheet* sSheet, tIndex sHeaderRow, tIndex sRangeLeft) const {
        tString wLabel = ReadHeaderCellText(sSheet, sHeaderRow, m_SheetCol);
        if (wLabel.empty() && !m_ImportLabel().empty()) {
            wLabel = NormalizeTableColumnLabel(m_ImportLabel());
        }
        if (!wLabel.empty()) {
            return wLabel;
        }
        tStringStream wFallback;
        wFallback << "Column" << (m_SheetCol - sRangeLeft + 1);
        return wFallback.str();
    }

    tString tColumnData::Name() const {
        return "";
    }

    tSortOrder tColumnData::SortOrder() const {
        return m_SortOrder;
    }

    tFilterOperator tColumnData::FilterOperator() const {
        return m_FilterOperator;
    }

    tVariant tColumnData::FilterValue() const {
        return m_FilterValue;
    }

    const vector<tVariant>& tColumnData::FilterValues() const {
        return m_FilterValues;
    }
#ifdef _DEBUGSK
    tString tColumnData::Debug(tSheet* sSheet, tIndex sHeaderRow, tIndex sRangeLeft) const {
        return(HeaderLabel(sSheet, sHeaderRow, sRangeLeft) + " " + m_Type() + " "
               + FilterOperatorToString(m_FilterOperator) + " " + m_FilterValue.Str() + " "
               + SortOrderToString(m_SortOrder));
    }
#endif
    tRangeData::tRangeData() :
        tClass(),
        m_UseFirstRowAsHeader(true),
        m_LastRowAsTotalRow(false),
        m_TotalsRowCount(0) {
    }

    tRangeData::tRangeData(tRangeData* sRangeData) :
    tClass(),
    m_UseFirstRowAsHeader(sRangeData->m_UseFirstRowAsHeader),
    m_LastRowAsTotalRow(sRangeData->m_LastRowAsTotalRow),
    m_TableStyleName(sRangeData->m_TableStyleName),
    m_TableShowRowStripes(sRangeData->m_TableShowRowStripes),
    m_TableShowColumnStripes(sRangeData->m_TableShowColumnStripes),
    m_TableShowFirstColumn(sRangeData->m_TableShowFirstColumn),
    m_TableShowLastColumn(sRangeData->m_TableShowLastColumn),
    m_TableAutoFilter(sRangeData->m_TableAutoFilter),
    m_TableDisplayName(sRangeData->m_TableDisplayName),
    m_TotalsRowCount(sRangeData->m_TotalsRowCount),
    m_TableStyleElementCss(sRangeData->m_TableStyleElementCss),
    m_ColumnDataVector(sRangeData->m_ColumnDataVector) {
    }

    tRangeData::~tRangeData() {
    }

    void tRangeData::Clear() {
        m_ColumnDataVector.clear();
    }

    void tRangeData::NormalizeLegacyColumnCols(tIndex sRangeLeft, tIndex sRangeRight) {
        if (AlreadyNormalizedSheetCols(m_ColumnDataVector, sRangeLeft, sRangeRight)) {
            return;
        }
        const tBool wAbsolute =
            UsesLegacyAbsoluteColumnIndices(m_ColumnDataVector, sRangeLeft, sRangeRight);
        const tBool wZeroBased = UsesZeroBasedLegacyOffsets(m_ColumnDataVector);
        for (auto& wColumnData : m_ColumnDataVector) {
            wColumnData.SetSheetCol(NormalizeLegacyStoredCol(
                wColumnData.SheetCol(), sRangeLeft, sRangeRight, wAbsolute, wZeroBased));
        }
    }

    tBool tRangeData::FillByRect(tSheet* sSheet, tRect sRect) {
        if (sSheet == nullptr) {
            return false;
        }
        if (!sRect.IsValid()) {
            return false;
        }
        if (sRect.Height() <= 1) {
            return false;
        }
        NormalizeLegacyColumnCols(sRect.Left(), sRect.Right());

        std::map<tIndex, tColumnData> wPreserved;
        for (const auto& wColumnData : m_ColumnDataVector) {
            tIndex wCol = wColumnData.SheetCol();
            if (wCol < sRect.Left() || wCol > sRect.Right()) {
                continue;
            }
            wPreserved[wCol] = wColumnData;
        }

        m_ColumnDataVector.clear();
        for (tIndex wCol = sRect.Left(); wCol <= sRect.Right(); wCol++) {
            tColumnData wEntry(wCol, "Text", tSortOrder::None);
            const auto wIt = wPreserved.find(wCol);
            if (wIt != wPreserved.end()) {
                wEntry.CopyMetadataFrom(wIt->second);
            }
            CaptureImportLabelFromHeader(wEntry, sSheet, sRect.Top(), nullptr, nullptr);
            m_ColumnDataVector.push_back(wEntry);
        }

        if (!m_ColumnDataVector.empty()) {
            m_UseFirstRowAsHeader = true;
        }
        return !m_ColumnDataVector.empty();
    }

    void tRangeData::SyncFromRange(tSheet* sSheet, tRange* sRange) {
        if (sSheet == nullptr || sRange == nullptr) {
            return;
        }
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wRight = sRange->RightIndex();
        NormalizeLegacyColumnCols(wLeft, wRight);

        std::map<tIndex, tColumnData> wPreserved;
        for (const auto& wColumnData : m_ColumnDataVector) {
            tIndex wCol = wColumnData.SheetCol();
            if (wCol < wLeft || wCol > wRight) {
                continue;
            }
            wPreserved[wCol] = wColumnData;
        }

        m_ColumnDataVector.clear();
        for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
            tColumnData wEntry(wCol, "Text", tSortOrder::None);
            const auto wIt = wPreserved.find(wCol);
            if (wIt != wPreserved.end()) {
                wEntry.CopyMetadataFrom(wIt->second);
            }
            CaptureImportLabelFromHeader(wEntry, sSheet, sRange->TopIndex(), sRange, this);
            m_ColumnDataVector.push_back(wEntry);
        }
    }

    void tRangeData::ShiftSheetCols(tIndex sFromCol, tIndex sDelta) {
        for (auto& wColumnData : m_ColumnDataVector) {
            wColumnData.ShiftSheetCol(sFromCol, sDelta);
        }
    }

    tColumnData* tRangeData::FindColumnByName(tSheet* sSheet, tRange* sRange, tString sName) {
#ifdef debugdata
        cout << "tRangeData::FindColumnByName("<< sName << ")" << endl;
#endif
        if (sRange == nullptr) {
            return FindColumnByName(sName);
        }
        const tIndex wHeaderRow = sRange->TopIndex();
        const tIndex wLeft = sRange->LeftIndex();
        const tString wNeedle = NormalizeTableColumnLabel(sName);
        for (auto& wColumnData : m_ColumnDataVector) {
            const tString wLabel = wColumnData.HeaderLabel(sSheet, wHeaderRow, wLeft);
#ifdef debugdata
            cout << "    "<< wLabel << endl;
#endif
            if (wNeedle == wLabel) {
                return &wColumnData;
            }
            const tString wImportLabel = NormalizeTableColumnLabel(wColumnData.ImportLabel());
            if (!wImportLabel.empty() && wNeedle == wImportLabel) {
                return &wColumnData;
            }
        }
        return(nullptr);
    }

    tBool tRangeData::CanAssignHeaderLabel(tSheet* sSheet, tRange* sRange, tIndex sSheetCol,
                                           tString sLabel) const {
        if (sSheet == nullptr || sRange == nullptr) {
            return true;
        }
        const tString wNeedle = NormalizeTableColumnLabel(sLabel);
        if (wNeedle.empty()) {
            return true;
        }
        const tIndex wHeaderRow = sRange->TopIndex();
        const tIndex wLeft = sRange->LeftIndex();
        for (const auto& wColumnData : m_ColumnDataVector) {
            if (wColumnData.SheetCol() == sSheetCol) {
                continue;
            }
            const tString wOther = wColumnData.HeaderLabel(sSheet, wHeaderRow, wLeft);
            if (wNeedle == NormalizeTableColumnLabel(wOther)) {
                return false;
            }
        }
        return true;
    }

    tBool tRangeData::ValidateHeaderCellValue(tWorkBook* sWorkBook, tSheet* sSheet, tIndex sRow,
                                              tIndex sCol, const tVariant& sVariant,
                                              tString* sErrorMessage) {
        if (sWorkBook == nullptr || sSheet == nullptr) {
            return true;
        }
        if (sVariant.HasFormula() || !sVariant.Formula().empty()) {
            return true;
        }
        if (sVariant.Type() == tVariantType::t_string) {
            const tString wValue = sVariant.String();
            if (!wValue.empty() && wValue[0] == '=') {
                return true;
            }
        }
        tString wTableName;
        tRange* wTableRange = nullptr;
        tie(wTableName, wTableRange) = sSheet->FindRangeDataCovered(sRow, sCol);
        if (wTableRange == nullptr || !wTableRange->IsData()) {
            return true;
        }
        tRangeData* wRangeData = sWorkBook->RangeData(wTableName);
        if (wRangeData == nullptr || !wRangeData->HasHeaders()) {
            return true;
        }
        if (sRow != wTableRange->TopIndex()) {
            return true;
        }
        const tString wLabel = sVariant.Str();
        if (!wRangeData->CanAssignHeaderLabel(sSheet, wTableRange, sCol, wLabel)) {
            if (sErrorMessage != nullptr) {
                tStringStream wStream;
                wStream << "Column " << wLabel << " already exists";
                *sErrorMessage = wStream.str();
            }
            return false;
        }
        return true;
    }

    void tRangeData::CaptureHeaderLabelFromCell(tSheet* sSheet, tRange* sRange, tIndex sSheetCol,
                                                tString sLabel) {
        if (sSheet == nullptr || sRange == nullptr) {
            return;
        }
        if (NormalizeTableColumnLabel(sLabel).empty()) {
            return;
        }
        if (!CanAssignHeaderLabel(sSheet, sRange, sSheetCol, sLabel)) {
            return;
        }
        for (auto& wColumnData : m_ColumnDataVector) {
            if (wColumnData.SheetCol() == sSheetCol) {
                wColumnData.SetImportLabel(std::move(sLabel));
                return;
            }
        }
    }

    tColumnData* tRangeData::FindColumnByName(tString sName) {
        for (auto& wColumnData : m_ColumnDataVector) {
            tStringStream wFallback;
            wFallback << "Column" << (wColumnData.Ordinal(1) + 1);
            if (sName == wFallback.str()) {
                return &wColumnData;
            }
        }
        return(nullptr);
    }

    tColumnData* tRangeData::FindColumnByOrdinal(tIndex sRangeLeft, tSize sOrdinal) {
        const tIndex wTargetCol = sRangeLeft + static_cast<tIndex>(sOrdinal);
        for (auto& wColumnData : m_ColumnDataVector) {
            if (wColumnData.SheetCol() == wTargetCol) {
                return &wColumnData;
            }
        }
        return(nullptr);
    }

    tColumnData* tRangeData::FindColumnByIndex(tSize sIndex) {
        if (sIndex < m_ColumnDataVector.size()) {
            return &m_ColumnDataVector[sIndex];
        }
        return(nullptr);
    }

    tBool tRangeData::IsEmpty() { return(m_ColumnDataVector.empty()); }

    void tRangeData::AddColumn(tColumnData sColumnData) {
        m_ColumnDataVector.push_back(sColumnData);
    }

    tBool tRangeData::HasHeaders() const { return(m_UseFirstRowAsHeader); }
    tBool tRangeData::HasTotals() { return TotalsRowCount() > 0; }

    tInt tRangeData::TotalsRowCount() const {
        if (m_TotalsRowCount > 0) {
            return m_TotalsRowCount;
        }
        return m_LastRowAsTotalRow ? 1 : 0;
    }

    void tRangeData::TotalsRowCount(tInt sValue) {
        m_TotalsRowCount = sValue < 0 ? 0 : sValue;
        m_LastRowAsTotalRow = m_TotalsRowCount > 0;
    }

    tString tRangeData::TableDisplayName() const { return m_TableDisplayName; }
    void tRangeData::TableDisplayName(tString sValue) { m_TableDisplayName = std::move(sValue); }

    tString tRangeData::TableStyleName() const { return m_TableStyleName; }
    void tRangeData::TableStyleName(tString sValue) { m_TableStyleName = std::move(sValue); }
    tBool tRangeData::TableShowRowStripes() const { return m_TableShowRowStripes; }
    void tRangeData::TableShowRowStripes(tBool sValue) { m_TableShowRowStripes = sValue; }
    tBool tRangeData::TableShowColumnStripes() const { return m_TableShowColumnStripes; }
    void tRangeData::TableShowColumnStripes(tBool sValue) { m_TableShowColumnStripes = sValue; }
    tBool tRangeData::TableShowFirstColumn() const { return m_TableShowFirstColumn; }
    void tRangeData::TableShowFirstColumn(tBool sValue) { m_TableShowFirstColumn = sValue; }
    tBool tRangeData::TableShowLastColumn() const { return m_TableShowLastColumn; }
    void tRangeData::TableShowLastColumn(tBool sValue) { m_TableShowLastColumn = sValue; }
    tBool tRangeData::TableAutoFilter() const { return m_TableAutoFilter; }
    void tRangeData::TableAutoFilter(tBool sValue) { m_TableAutoFilter = sValue; }

    void tRangeData::SetTableStyleElementCss(const tString& sType, tString sCss) {
        if (sType.empty() || sCss.empty()) {
            return;
        }
        m_TableStyleElementCss[sType] = std::move(sCss);
    }

    const std::map<tString, tString>& tRangeData::TableStyleElementCss() const {
        return m_TableStyleElementCss;
    }

    tBool tRangeData::HasTableStyleElementCss() const {
        return !m_TableStyleElementCss.empty();
    }

    namespace {
    void WriteRangeDataTableStyleJson(Writer<StringBuffer>* sWriter, const tRangeData& sRangeData) {
        if (!sRangeData.TableStyleName().empty()) {
            sWriter->Key(kJsonKeyTableStyleName);
            sWriter->String(sRangeData.TableStyleName().c_str());
        }
        if (sRangeData.TableShowRowStripes()) {
            sWriter->Key(kJsonKeyTableShowRowStripes);
            sWriter->Bool(true);
        }
        if (sRangeData.TableShowColumnStripes()) {
            sWriter->Key(kJsonKeyTableShowColumnStripes);
            sWriter->Bool(true);
        }
        if (sRangeData.TableShowFirstColumn()) {
            sWriter->Key(kJsonKeyTableShowFirstColumn);
            sWriter->Bool(true);
        }
        if (sRangeData.TableShowLastColumn()) {
            sWriter->Key(kJsonKeyTableShowLastColumn);
            sWriter->Bool(true);
        }
        if (!sRangeData.TableStyleElementCss().empty()) {
            sWriter->Key(kJsonKeyTableStyleElements);
            sWriter->StartObject();
            for (const auto& wEntry : sRangeData.TableStyleElementCss()) {
                sWriter->Key(wEntry.first.c_str());
                sWriter->String(wEntry.second.c_str());
            }
            sWriter->EndObject();
        }
        if (sRangeData.TableAutoFilter()) {
            sWriter->Key(kJsonKeyTableAutoFilter);
            sWriter->Bool(true);
        }
        if (!sRangeData.TableDisplayName().empty()) {
            sWriter->Key(kJsonKeyTableDisplayName);
            sWriter->String(sRangeData.TableDisplayName().c_str());
        }
        if (sRangeData.TotalsRowCount() > 0) {
            sWriter->Key(kJsonKeyTotalsRowCount);
            sWriter->Int(sRangeData.TotalsRowCount());
        }
    }

    void ReadRangeDataTableStyleJson(const rapidjson::Value& sValue, tRangeData& sRangeData) {
        if (sValue.HasMember(kJsonKeyTableStyleName)) {
            sRangeData.TableStyleName(sValue[kJsonKeyTableStyleName].GetString());
        }
        if (sValue.HasMember(kJsonKeyTableShowRowStripes)) {
            sRangeData.TableShowRowStripes(sValue[kJsonKeyTableShowRowStripes].GetBool());
        }
        if (sValue.HasMember(kJsonKeyTableShowColumnStripes)) {
            sRangeData.TableShowColumnStripes(sValue[kJsonKeyTableShowColumnStripes].GetBool());
        }
        if (sValue.HasMember(kJsonKeyTableShowFirstColumn)) {
            sRangeData.TableShowFirstColumn(sValue[kJsonKeyTableShowFirstColumn].GetBool());
        }
        if (sValue.HasMember(kJsonKeyTableShowLastColumn)) {
            sRangeData.TableShowLastColumn(sValue[kJsonKeyTableShowLastColumn].GetBool());
        }
        if (sValue.HasMember(kJsonKeyTableAutoFilter)) {
            sRangeData.TableAutoFilter(sValue[kJsonKeyTableAutoFilter].GetBool());
        }
        if (sValue.HasMember(kJsonKeyTableDisplayName)
            && sValue[kJsonKeyTableDisplayName].IsString()) {
            sRangeData.TableDisplayName(sValue[kJsonKeyTableDisplayName].GetString());
        }
        if (sValue.HasMember(kJsonKeyTotalsRowCount)) {
            sRangeData.TotalsRowCount(sValue[kJsonKeyTotalsRowCount].GetInt());
        }
        if (sValue.HasMember(kJsonKeyTableStyleElements)
            && sValue[kJsonKeyTableStyleElements].IsObject()) {
            const rapidjson::Value& wElements = sValue[kJsonKeyTableStyleElements];
            for (auto wIt = wElements.MemberBegin(); wIt != wElements.MemberEnd(); ++wIt) {
                if (wIt->value.IsString()) {
                    sRangeData.SetTableStyleElementCss(wIt->name.GetString(), wIt->value.GetString());
                }
            }
        }
    }
    } // namespace

    void tRangeData::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        if (!m_UseFirstRowAsHeader) {
            sWriter->Key(kJsonKeyUseFirstRowAsHeader);
            sWriter->Bool(m_UseFirstRowAsHeader);
        }
        if (m_LastRowAsTotalRow) {
            sWriter->Key(kJsonKeyLastRowAsTotalRow);
            sWriter->Bool(m_LastRowAsTotalRow);
        }
        WriteRangeDataTableStyleJson(sWriter, *this);
        sWriter->Key(kJsonKeyColumnDataVector);
        sWriter->StartArray();
        for (auto wColumnData : m_ColumnDataVector) {
            wColumnData.Json(sWriter);
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tRangeData::Json(Writer<StringBuffer>* sWriter, tSheet* sSheet, tIndex sHeaderRow,
                          tIndex sRangeLeft) {
        sWriter->StartObject();
        if (!m_UseFirstRowAsHeader) {
            sWriter->Key(kJsonKeyUseFirstRowAsHeader);
            sWriter->Bool(m_UseFirstRowAsHeader);
        }
        if (m_LastRowAsTotalRow) {
            sWriter->Key(kJsonKeyLastRowAsTotalRow);
            sWriter->Bool(m_LastRowAsTotalRow);
        }
        WriteRangeDataTableStyleJson(sWriter, *this);
        sWriter->Key(kJsonKeyColumnDataVector);
        sWriter->StartArray();
        for (auto wColumnData : m_ColumnDataVector) {
            tString wSnapshot;
            if (sSheet != nullptr) {
                wSnapshot = wColumnData.HeaderLabel(sSheet, sHeaderRow, sRangeLeft);
            }
            wColumnData.Json(sWriter, wSnapshot);
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tRangeData::Json(const rapidjson::Value& sValue, tIndex sRangeLeft, tIndex sRangeRight) {
        if (!sValue.IsObject()) {
            return;
        }
        const char* wKeyUseFirstRowAsHeader = kJsonKeyUseFirstRowAsHeader;
        if (sValue.HasMember(wKeyUseFirstRowAsHeader)) {
            m_UseFirstRowAsHeader = sValue[wKeyUseFirstRowAsHeader].GetBool();
        } else {
            m_UseFirstRowAsHeader = true;
        }
        const char* wKeyLastRowAsTotalRow = kJsonKeyLastRowAsTotalRow;
        if (sValue.HasMember(wKeyLastRowAsTotalRow)) {
            m_LastRowAsTotalRow = sValue[wKeyLastRowAsTotalRow].GetBool();
        } else {
            m_LastRowAsTotalRow = false;
        }
        if (!sValue.HasMember(kJsonKeyTotalsRowCount) && m_LastRowAsTotalRow) {
            m_TotalsRowCount = 1;
        }
        ReadRangeDataTableStyleJson(sValue, *this);
        if (sValue.HasMember(kJsonKeyColumnDataVector)) {
            const rapidjson::Value& wColumnDataVector = sValue[kJsonKeyColumnDataVector];
            for (rapidjson::SizeType wIndex = 0; wIndex < wColumnDataVector.Size(); wIndex++) {
                const rapidjson::Value& wColumnDataValue = wColumnDataVector[wIndex];
                tColumnData wColumnData;
                wColumnData.Json(wColumnDataValue);
                m_ColumnDataVector.push_back(wColumnData);
            }
            if (sRangeLeft >= 1 && sRangeRight >= sRangeLeft) {
                NormalizeLegacyColumnCols(sRangeLeft, sRangeRight);
            }
        } else {
            tColumnData wColumnData(1, "Text", tSortOrder::None);
            m_ColumnDataVector.push_back(wColumnData);
        }
    }

    void tRangeData::Sort(tRange* sRange) {
        tSortOptions wSortOptions;
        wSortOptions.HasHeader(m_UseFirstRowAsHeader);
        for (const auto& wColumnData : m_ColumnDataVector) {
            if (wColumnData.SortOrder() != tSortOrder::None) {
                tIndex wColIndex = RelativeColIndex(wColumnData.SheetCol(), sRange);
                if (wColIndex < 0) {
                  return;
                }
                wSortOptions.AddSortColumn(wColIndex, wColumnData.SortOrder());
            }
        }
        if (!wSortOptions.IsEmpty()) {
            tRangeSort wRangeSort(sRange->ColRowCellRange());
            wRangeSort.Sort(sRange, wSortOptions);
        }
    }

    void tRangeData::Filter(tRange* sRange) {
        tFilterOptions wFilterOptions;
        wFilterOptions.HasHeader(m_UseFirstRowAsHeader);
        for (const auto& wColumnData : m_ColumnDataVector) {
            if (wColumnData.FilterOperator() != tFilterOperator::None) {
                tIndex wColIndex = RelativeColIndex(wColumnData.SheetCol(), sRange);
                if (wColIndex < 0) {
                    tStringStream wStream;
                    wStream << "throw:  tRangeData::Filter Check error on ColIndex " << wColIndex << " !";
                    cerr << wStream.str() << endl;
					throw(tExceptionInternalError(wStream.str()));
                }
                tFilterOperator wOperator = wColumnData.FilterOperator();
#ifdef debugfilter
                cout << "Filter: Adding criterion Col=" << wColIndex << " Op=" << (int)wOperator
                     << " Value=" << wColumnData.FilterValue().Str() << endl;
#endif
                if (wOperator == tFilterOperator::IsEmpty || wOperator == tFilterOperator::IsNotEmpty) {
                    wFilterOptions.AddCriterion(tFilterCriterion(wColIndex, wOperator));
                } else if (!wColumnData.FilterValues().empty()) {
                    for (const auto& wValue : wColumnData.FilterValues()) {
                        wFilterOptions.AddCriterion(
                            tFilterCriterion(wColIndex, wOperator, wValue));
                    }
                } else {
                    wFilterOptions.AddCriterion(tFilterCriterion(wColIndex, wOperator, wColumnData.FilterValue()));
                }
            }
        }
        tRangeFilter wRangeFilter(sRange->ColRowCellRange());
        if (!wFilterOptions.IsEmpty()) {
#ifdef debugfilter
            cout << "Filter: Applying filter with " << wFilterOptions.Criteria().size() << " criteria" << endl;
#endif
            wRangeFilter.Filter(sRange, wFilterOptions);
        } else {
            const tIndex wTop = sRange->TopIndex();
            const tIndex wBottom = sRange->BottomIndex();
            for (tIndex wRow = wTop; wRow <= wBottom; wRow++) {
                sRange->ColRowCellRange()->Row(wRow)->DataVisible(true);
            }
        }
    }


    void tRangeData::Json(tString sJson) {
        rapidjson::Document wDocument;
        wDocument.Parse(sJson.c_str());
        Json(wDocument);
    }

    tString tRangeData::Json() {
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        Json(&wWriter);
        return wBuffer.GetString();
    }

#ifdef _DEBUGSK
    tString tRangeData::Debug(tSheet* sSheet, tRange* sRange) {
        tStringStream wStringStream;
        const tIndex wHeaderRow = sRange != nullptr ? sRange->TopIndex() : 1;
        const tIndex wLeft = sRange != nullptr ? sRange->LeftIndex() : 1;
        wStringStream << "RangeData: " << endl;
        wStringStream << "UseFirstRowAsHeader: " << (m_UseFirstRowAsHeader ? "true" : "false") << endl;
        wStringStream << "ColumnDataVector: " << endl;
        for (auto wColumnData : m_ColumnDataVector) {
            wStringStream << wColumnData.Debug(sSheet, wHeaderRow, wLeft) << endl;
        }
        return(wStringStream.str());
    }
#endif
}
