//=============================================================================
// SkSpreadSheet Range Sort - Implementation
//=============================================================================

#include "../include/SkRangeSort.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkVariant.hpp"
#include "../include/SkTools.hpp"
#include "../include/SkSpreadSheet.hpp"
#include <cstring>
#include <algorithm>

namespace SkSpreadSheet {

namespace {

    /// Scalar used for sort comparison; false when the cell has no comparable numeric/text key.
    tBool SortKeyFromVariant(const tVariant& sV, tDouble& outNum) {
        if (sV.IsExcelNull()) {
            return false;
        }
        switch (sV.Type()) {
            case tVariantType::t_int:
                outNum = tDouble(sV.Int());
                return true;
            case tVariantType::t_double:
                outNum = sV.Double();
                return true;
            case tVariantType::t_bool:
                outNum = sV.Bool() ? 1.0 : 0.0;
                return true;
            case tVariantType::t_string: {
                const tString wText = sV.String();
                if (wText.empty()) {
                    return false;
                }
                tVariant wParsed;
                wParsed.Parse(wText);
                if (wParsed.Type() == tVariantType::t_int
                    || wParsed.Type() == tVariantType::t_double) {
                    outNum = wParsed.Double();
                    return true;
                }
                return false;
            }
            default:
                return false;
        }
    }

    /// -1 / 0 / +1 in ascending order; non-numeric / empty sort last (treated as smallest).
    int CompareVariantsForSort(const tVariant& sV1, const tVariant& sV2) {
        tDouble wN1 = 0.0;
        tDouble wN2 = 0.0;
        const tBool wHas1 = SortKeyFromVariant(sV1, wN1);
        const tBool wHas2 = SortKeyFromVariant(sV2, wN2);

        if (wHas1 && wHas2) {
            if (wN1 < wN2) {
                return -1;
            }
            if (wN1 > wN2) {
                return 1;
            }
            return 0;
        }
        if (wHas1 != wHas2) {
            // Missing / non-numeric keys sort after numeric ones (ascending semantics).
            return wHas1 ? -1 : 1;
        }

        // Both non-numeric: fall back to string compare when both are strings.
        if (sV1.Type() == tVariantType::t_string && sV2.Type() == tVariantType::t_string) {
            const tString wA = sV1.String();
            const tString wB = sV2.String();
            if (wA < wB) {
                return -1;
            }
            if (wA > wB) {
                return 1;
            }
            return 0;
        }

        if (sV1 == sV2) {
            return 0;
        }
        if (sV1 < sV2) {
            return -1;
        }
        if (sV2 < sV1) {
            return 1;
        }
        return 0;
    }

} // namespace

    //=========================================================================
    // tSortOptions implementation
    //=========================================================================
    tSortOptions::tSortOptions() : m_HasHeader(false), m_ReorganizeMethod(tSortReorganizeMethod::ReorganizeRows) {
    }

    void tSortOptions::AddSortColumn(tIndex sColIndex, tSortOrder sOrder) {
        m_SortColumns.push_back(tSortColumn(sColIndex, sOrder));
    }

    void tSortOptions::HasHeader(tBool sHasHeader) {
        m_HasHeader = sHasHeader;
    }

    tBool tSortOptions::HasHeader() const {
        return m_HasHeader;
    }

    void tSortOptions::ReorganizeMethod(tSortReorganizeMethod sMethod) {
        m_ReorganizeMethod = sMethod;
    }

    tSortReorganizeMethod tSortOptions::ReorganizeMethod() const {
        return m_ReorganizeMethod;
    }

    const std::vector<tSortColumn>& tSortOptions::SortColumns() const {
        return m_SortColumns;
    }

    void tSortOptions::Clear() {
        m_SortColumns.clear();
        m_HasHeader = false;
        m_ReorganizeMethod = tSortReorganizeMethod::ReorganizeRows;
    }
    
    tBool tSortOptions::IsEmpty() const {
        for(auto wSortColumn : m_SortColumns) {
            if (wSortColumn.m_Order!=tSortOrder::None) {
                return(false);
            }
        }
        return (true);
    }

    //=========================================================================
    // tSortRowData implementation
    //=========================================================================
    tSortRowData::tSortRowData(tIndex sOriginalRowIndex) 
        : m_OriginalRowIndex(sOriginalRowIndex) {
    }

    void tSortRowData::AddCell(const tVariant& sValue, tCell* sCell) {
        m_CellValues.push_back(sValue);
        m_CellPointers.push_back(sCell);
    }

    const tVariant& tSortRowData::CellValue(tIndex sColIndex) const {
        if (sColIndex < m_CellValues.size()) {
            return m_CellValues[sColIndex];
        }
        static tVariant Static_EmptyVariant;
        return Static_EmptyVariant;
    }

    tCell* tSortRowData::CellPointer(tIndex sColIndex) const {
        if (sColIndex < m_CellPointers.size()) {
            return m_CellPointers[sColIndex];
        }
        return nullptr;
    }

    tIndex tSortRowData::OriginalRowIndex() const {
        return m_OriginalRowIndex;
    }

    void tSortRowData::OriginalRowIndex(tIndex sRowIndex) {
        m_OriginalRowIndex = sRowIndex;
    }

    tSize tSortRowData::CellCount() const {
        return m_CellValues.size();
    }

    //=========================================================================
    // tSortRowComparator implementation
    //=========================================================================
    tSortRowComparator::tSortRowComparator(const std::vector<tSortColumn>& sSortColumns)
        : m_SortColumns(sSortColumns) {
    }

    int tSortRowComparator::CompareVariants(const tVariant& sV1, const tVariant& sV2) const {
        return CompareVariantsForSort(sV1, sV2);
    }

    tBool tSortRowComparator::operator()(const tSortRowData& sRow1, const tSortRowData& sRow2) const {
        // Strict weak ordering requires:
        // 1. comp(a, a) == false (irreflexivity)
        // 2. If comp(a, b) == true, then comp(b, a) == false (asymmetry)
        // 3. If comp(a, b) == true and comp(b, c) == true, then comp(a, c) == true (transitivity)
        
        for (const auto& wSortCol : m_SortColumns) {
            tIndex wColIdx = wSortCol.m_ColIndex;
            
            // Check bounds
            tBool wRow1HasCol = (wColIdx < sRow1.CellCount());
            tBool wRow2HasCol = (wColIdx < sRow2.CellCount());
            
            // Handle missing columns consistently for strict weak ordering
            if (!wRow1HasCol && !wRow2HasCol) {
                // Both missing - equivalent for this column, continue to next
                continue;
            }
            
            if (!wRow1HasCol) {
                // row1 missing, row2 has value
                // Missing values are always "less than" existing values (for ascending)
                // This ensures consistent ordering: missing < any value
                return (wSortCol.m_Order == tSortOrder::Ascending);
            }
            
            if (!wRow2HasCol) {
                // row2 missing, row1 has value
                // Missing values are always "less than" existing values
                // So row2 < row1, meaning row1 > row2
                return (wSortCol.m_Order == tSortOrder::Descending);
            }
            
            // Both rows have the column - compare values
            const tVariant& wV1 = sRow1.CellValue(wColIdx);
            const tVariant& vV2 = sRow2.CellValue(wColIdx);
            
            // Compare variants - this must be transitive
            // Use CompareVariants to ensure strict weak ordering
            tInt wCmp = CompareVariants(wV1, vV2);
            
            if (wCmp < 0) {
                // v1 < v2
                return (wSortCol.m_Order == tSortOrder::Ascending);
            } else if (wCmp > 0) {
                // v1 > v2
                return (wSortCol.m_Order == tSortOrder::Descending);
            }
            // v1 == v2 (or incomparable), continue to next sort column
        }
        
        // All columns are equivalent - rows are equal
        // Must return false to satisfy irreflexivity: comp(a, a) == false
        return false;
    }

    //=========================================================================
    // tRangeSort implementation
    //=========================================================================
    tRangeSort::tRangeSort(tColRowCellRange* sColRowCellRange) 
        : m_ColRowCellRange(sColRowCellRange) {
    }

    tBool tRangeSort::Sort(tRange* sRange, const tSortOptions& sOptions) {
        if (sRange == nullptr || m_ColRowCellRange == nullptr) {
            return false;
        }
        
        if (sRange->IsEmpty() ||  (sOptions.IsEmpty()) )  {
            return true; // Nothing to sort
        }
        
        // Collect all rows from range
        std::vector<tSortRowData> wRows;
        if (!CollectRows(sRange, wRows)) {
            return false;
        }
        
        // Handle header row
        tSortRowData wHeaderRow(0); // Temporary storage for header
        tBool wHasHeader = false;
        if (sOptions.HasHeader() && !wRows.empty()) {
            // Copy header row data
            wHeaderRow = wRows[0];
            wHasHeader = true;
            wRows.erase(wRows.begin());
        }
        
        // Sort rows
        tBool wWasSorted = false;
        if (!wRows.empty() && !sOptions.SortColumns().empty()) {
            tSortRowComparator comparator(sOptions.SortColumns());
            // Use std::sort instead of std::stable_sort
            // std::sort is generally faster and may be less strict in debug mode checks
            // Both require strict weak ordering, but std::sort may be more lenient
            std::sort(wRows.begin(), wRows.end(), comparator);
            wWasSorted = true;
        }
        
        // Reinsert header if present
        if (wHasHeader) {
            wRows.insert(wRows.begin(), wHeaderRow);
        }
        
        // Only reorganize rows if sorting was actually performed
        // This avoids unnecessary reorganization when Sort() is called but no sorting is needed
        if (wWasSorted) {
            // Choose reorganization method based on options
            if (sOptions.ReorganizeMethod() == tSortReorganizeMethod::ReorganizeRows) {
                // Use ReorganizeRows to sort directly the tColRow objects for better performance
                return ReorganizeRows(sRange, wRows);
            } else {
                // Use ReorganizeCells to reorganize cells individually (more compatible)
                return ReorganizeCells(sRange, wRows);
            }
        }
        
        return true; // No sorting was performed, nothing to reorganize
    }

    tBool tRangeSort::CollectRows(tRange* sRange, std::vector<tSortRowData>& sRows) {
        tIndex wTop = sRange->TopIndex();
        tIndex wBottom = sRange->IterateBottom();
        tIndex wLeft = sRange->LeftIndex();
        tIndex wRight = sRange->IterateRight();
        
        sRows.clear();
        sRows.reserve(wBottom - wTop + 1);
        
        for (tIndex wRow = wTop; wRow <= wBottom; wRow++) {
            tSortRowData rowData(wRow);
            
            for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
                tCell* wCell = m_ColRowCellRange->Cell(wRow, wCol);
                tVariant wValue;

                if (wCell != nullptr) {
                    wValue = wCell->Value();
                }

                rowData.AddCell(wValue, wCell);
            }
            
            sRows.push_back(rowData);
        }
        
        return true;
    }

    tBool tRangeSort::ReorganizeRows(tRange* sRange, const std::vector<tSortRowData>& sSortedRows) {
        tIndex wTop = sRange->TopIndex();
        tIndex wLeft = sRange->LeftIndex();
        tIndex wRight = sRange->RightIndex();
        
        // Create temporary storage for row references and cell data
        // We'll reorganize rows by swapping tColRow references and their cells
        std::vector<tAllocatorRef> wTempRowRefs(sSortedRows.size());
        std::vector<std::vector<tAllocatorRef>> wTempCellRefs(sSortedRows.size());
        std::vector<tBool> wTempDataVisible(sSortedRows.size());
        
        // Store original row references and cell references
        for (tSize i = 0; i < sSortedRows.size(); i++) {
            tIndex wOriginalRow = sSortedRows[i].OriginalRowIndex();
            wTempRowRefs[i] = m_ColRowCellRange->RowAllocatorRef(wOriginalRow);
            
            // Store cell references for this row
            wTempCellRefs[i].resize(wRight - wLeft + 1);
            for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
                wTempCellRefs[i][wCol - wLeft] = m_ColRowCellRange->CellAllocatorRef(wOriginalRow, wCol);
            }
            
            // Store DataVisible state
            tColRow* wColRow = m_ColRowCellRange->Row(wOriginalRow);
            if (wColRow != nullptr) {
                wTempDataVisible[i] = wColRow->DataVisible();
            } else {
                wTempDataVisible[i] = true;
            }
        }
        
        // Clear destination rows and cells
        for (tIndex wRow = wTop; wRow <= wTop + (tIndex)sSortedRows.size() - 1; wRow++) {
            for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
                m_ColRowCellRange->SetCellAllocatorRef(0, wRow, wCol);
            }
        }
        
        // Write sorted rows back to their new positions
        for (tSize i = 0; i < sSortedRows.size(); i++) {
            tIndex wNewRow = wTop + (tIndex)i;
            
            // Set row reference in m_Rows (we need to access m_Rows directly)
            // Since m_Rows is private, we'll use EnsureRow to create/get the row
            tColRow* wColRow = m_ColRowCellRange->EnsureRow(wNewRow);
            if (wColRow != nullptr) {
                // Get the original ColRow to copy its properties
                tColRow* wOriginalColRow = m_ColRowCellRange->Row(sSortedRows[i].OriginalRowIndex());
                if (wOriginalColRow != nullptr) {
                    // Copy DataVisible state
                    wColRow->DataVisible(wTempDataVisible[i]);
                    // Copy other properties if needed (Size, Css, etc.)
                    wColRow->Size(wOriginalColRow->Size());
                    wColRow->Css(wOriginalColRow->Css());
                }
            }
            
            // Restore cell references
            for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
                tAllocatorRef wCellRef = wTempCellRefs[i][wCol - wLeft];
                if (wCellRef != 0) {
                    m_ColRowCellRange->SetCellAllocatorRef(wCellRef, wNewRow, wCol);
                    // Update cell's row reference
                    tCell* wCell = m_ColRowCellRange->Cell(wNewRow, wCol);
                    if (wCell != nullptr) {
                        wCell->SetColRow(m_ColRowCellRange->RowAllocatorRef(wNewRow), m_ColRowCellRange->ColAllocatorRef(wCol));
                    }
                }
            }
        }
        
        // Renumber rows starting from wTop
        m_ColRowCellRange->RenumRow(wTop);
        
        // Recalculate affected area
        tSheet* sheet = m_ColRowCellRange->Sheet();
        if (sheet != nullptr) {
            tTempoRect wRecalcRect(wTop, wLeft, wTop + (tIndex)sSortedRows.size() - 1, wRight);
            sheet->Calculate(&wRecalcRect);
        }
        
        return true;
    }

    tBool tRangeSort::ReorganizeCells(tRange* sRange, const std::vector<tSortRowData>& sSortedRows) {
        tIndex wTop = sRange->TopIndex();
        tIndex wLeft = sRange->LeftIndex();
        tIndex wRight = sRange->RightIndex();
        
        // Create temporary storage for new cell values
        // We need to copy all cell data first to avoid overwriting during reorganization
        std::vector<std::vector<tVariant>> wTempValues;
        std::vector<std::vector<tString>> wTempFormulas;
        std::vector<std::vector<tFormatRef>> wTempFormats;
        std::vector<tBool> wTempDataVisible; // Preserve DataVisible state
        
        wTempValues.resize(sSortedRows.size());
        wTempFormulas.resize(sSortedRows.size());
        wTempFormats.resize(sSortedRows.size());
        wTempDataVisible.resize(sSortedRows.size());
        
        // Copy all data to temporary storage
        // Use R1C1 format for formulas to preserve relative references during sort
        for (tSize i = 0; i < sSortedRows.size(); i++) {
            const tSortRowData& wRowData = sSortedRows[i];
            tSize colCount = wRowData.CellCount();
            
            wTempValues[i].resize(colCount);
            wTempFormulas[i].resize(colCount);
            wTempFormats[i].resize(colCount);
            
            // Preserve DataVisible state from original row
            tIndex wOriginalRow = wRowData.OriginalRowIndex();
            tColRow* wOriginalColRow = m_ColRowCellRange->Row(wOriginalRow);
            if (wOriginalColRow != nullptr) {
                wTempDataVisible[i] = wOriginalColRow->DataVisible();
            } else {
                wTempDataVisible[i] = true; // Default to visible if row doesn't exist
            }
            
            for (tIndex j = 0; j < static_cast<tIndex>(colCount); j++) {
                // Use CellValue which contains the actual value, not CellPointer
                // CellPointer may be nullptr or point to original position
                wTempValues[i][j] = wRowData.CellValue(j);
                
                tCell* wSourceCell = wRowData.CellPointer(j);
                if (wSourceCell != nullptr) {
                    // Save formula in R1C1 format relative to source cell position
                    // This preserves relative references when cells are moved during sort
                    wTempFormulas[i][j] = wSourceCell->FormulaStr(true); // true = R1C1 format
                    wTempFormats[i][j] = wSourceCell->Css();
                } else {
                    wTempFormulas[i][j] = "";
                    wTempFormats[i][j] = 0;
                }
            }
        }
        
        // Clear destination cells
        for (tIndex wRow = wTop; wRow <= wTop + (tIndex)sSortedRows.size() - 1; wRow++) {
            for (tIndex wCol = wLeft; wCol <= wRight; wCol++) {
                tCell* wCell = m_ColRowCellRange->Cell(wRow, wCol);
                if (wCell != nullptr) {
                    wCell->ClearFormulaAndVariant();
                }
            }
        }
        
        // Write sorted data back
        for (tSize wI = 0; wI < sSortedRows.size(); wI++) {
            tIndex wNewRow = wTop + (tIndex)wI;
            
            // Restore DataVisible state to the new row position
            tColRow* wNewColRow = m_ColRowCellRange->Row(wNewRow);
            if (wNewColRow != nullptr) {
                wNewColRow->DataVisible(wTempDataVisible[wI]);
            }
            
            for (tSize wJ = 0; wJ < wTempValues[wI].size(); wJ++) {
                tIndex newCol = wLeft + (tIndex)wJ;
                tCell* wCell = m_ColRowCellRange->EnsureCell(wNewRow, newCol);
                
                if (wCell != nullptr) {
                    // Restore format
                    wCell->Css(wTempFormats[wI][wJ]);
                    
                    // Restore formula or value
                    if (!wTempFormulas[wI][wJ].empty()) {
                        // Restore formula in R1C1 format
                        // The formula is already in R1C1 format relative to source cell
                        // When compiled in the new cell position, relative references will be automatically adjusted
                        tWorkBook* workBook = m_ColRowCellRange->Sheet()->WorkBook();
                        if (workBook != nullptr) {
                            // Compile formula: R1C1 format will be parsed and relative references
                            // will be recalculated based on the new cell position
                            workBook->CompilCell(wCell, wTempFormulas[wI][wJ].c_str());
                        }
                    } else {
                        // Restore value
                        wCell->Value(wTempValues[wI][wJ]);
                    }
                }
            }
        }
        
        // Recalculate affected area
        tSheet* sheet = m_ColRowCellRange->Sheet();
        if (sheet != nullptr) {
            tTempoRect wRecalcRect(wTop, wLeft, wTop + (tIndex)sSortedRows.size() - 1, wRight);
            sheet->Calculate(&wRecalcRect);
        }
        
        return true;
    }

    void tRangeSort::CopyCellData(tCell* sSource, tCell* sDest) {
        if (sSource == nullptr || sDest == nullptr) {
            return;
        }
        
        // Copy format
        sDest->Css(sSource->Css());
        
        // Copy formula if exists, otherwise copy value
        tString formula = sSource->FormulaStr();
        if (!formula.empty()) {
            tWorkBook* workBook = sDest->Sheet()->WorkBook();
            if (workBook != nullptr) {
                workBook->CompilCell(sDest, formula.c_str());
            }
        } else {
            sDest->Value(sSource->Value());
        }
    }

    //=========================================================================
    // Utility function to convert SortOrder to string
    //=========================================================================
    const tChar* SortOrderToString(tSortOrder sOrder) {
        switch (sOrder) {
            case tSortOrder::None:
                return("None");
            case tSortOrder::Ascending:
                return "Ascending";
            case tSortOrder::Descending:
                return "Descending";
            default:
                return "Unknown";
        }
    }

    //=========================================================================
    // Utility function to convert string to SortOrder
    //=========================================================================
    tSortOrder StringToSortOrder(const char* sOrderString) {
        if (sOrderString == nullptr) {
            return tSortOrder::None;
        }
        if (strcmp(sOrderString, "None") == 0) {
            return tSortOrder::None;
        } else if (strcmp(sOrderString, "Ascending") == 0) {
            return tSortOrder::Ascending;
        } else if (strcmp(sOrderString, "Descending") == 0) {
            return tSortOrder::Descending;
        } else {
            return tSortOrder::None; // Default fallback
        }
    }

} // End of namespace

