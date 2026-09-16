//=============================================================================
// SkSpreadSheet Range Filter - Implementation
//=============================================================================

#include "../include/SkRangeFilter.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkVariant.hpp"
#include "../include/SkTools.hpp"
#include "../include/SkSpreadSheet.hpp"
#include <cstring>
#include <map>

#define _debugfilter 

namespace SkSpreadSheet {

    //=========================================================================
    // tFilterCriterion implementation
    //=========================================================================
    tFilterCriterion::tFilterCriterion(tIndex sColIndex, tFilterOperator sOperator, const tVariant& sValue)
        : m_ColIndex(sColIndex), m_Operator(sOperator), m_Value1(sValue) {
    }

    tFilterCriterion::tFilterCriterion(tIndex sColIndex, const tVariant& sValue1, const tVariant& sValue2)
        : m_ColIndex(sColIndex), m_Operator(tFilterOperator::Between), m_Value1(sValue1), m_Value2(sValue2) {
    }

    tFilterCriterion::tFilterCriterion(tIndex sColIndex, tFilterOperator sOperator)
        : m_ColIndex(sColIndex), m_Operator(sOperator) {
    }

    //=========================================================================
    // tFilterOptions implementation
    //=========================================================================
    tFilterOptions::tFilterOptions() : m_Logic(tFilterLogic::And), m_HasHeader(false) {
    }

    void tFilterOptions::AddCriterion(const tFilterCriterion& sCriterion) {
        m_Criteria.push_back(sCriterion);
    }

    void tFilterOptions::Logic(tFilterLogic sLogic) {
        m_Logic = sLogic;
    }

    tFilterLogic tFilterOptions::Logic() const {
        return m_Logic;
    }

    void tFilterOptions::HasHeader(tBool sHasHeader) {
        m_HasHeader = sHasHeader;
    }

    tBool tFilterOptions::HasHeader() const {
        return m_HasHeader;
    }

    const std::vector<tFilterCriterion>& tFilterOptions::Criteria() const {
        return m_Criteria;
    }

    void tFilterOptions::Clear() {
        m_Criteria.clear();
        m_Logic = tFilterLogic::And;
        m_HasHeader = false;
    }
    
    tBool tFilterOptions::IsEmpty() const {
        for(auto wFilterOption : m_Criteria) {
            if (wFilterOption.m_Operator!=tFilterOperator::None) return(false);
        }
        return(true);
    }

    //=========================================================================
    // tRangeFilter implementation
    //=========================================================================
    tRangeFilter::tRangeFilter(tColRowCellRange* sColRowCellRange)
    : m_ColRowCellRange(sColRowCellRange),m_Rect() {
    }
    
    void tRangeFilter::Clear() {
        // Clear each row
        if (m_Rect.Top()!=-1) {
            for (tIndex wRow = m_Rect.Top(); wRow <= m_Rect.Bottom(); wRow++) {
                m_ColRowCellRange->Row(wRow)->DataVisible(wRow);
            }
        }
    }
    

    tBool tRangeFilter::Filter(tRange* sRange, const tFilterOptions& sOptions) {
        if (sRange == nullptr || m_ColRowCellRange == nullptr) {
            return false;
        }
        
        if (sRange->IsEmpty() || sOptions.Criteria().empty()) {
            return true; // Nothing to filter
        }
        ApplyOnRows(sRange, sOptions);
        
        return true;
    }

    tBool tRangeFilter::ApplyOnRows(tRange* sRange, const tFilterOptions& sOptions) {
        if (sRange == nullptr || m_ColRowCellRange == nullptr) {
            return false;
        }
        // Not Filter
        if (sOptions.IsEmpty()) {
            return(true);
        }
        
        tIndex wTop = sRange->TopIndex();
        tIndex wBottom = sRange->IterateBottom();
        tIndex wStartRow = wTop;
        
        // First, reset all rows in the range to visible (true)
        for (tIndex wRow = wTop; wRow <= wBottom; wRow++) {
            m_ColRowCellRange->Row(wRow)->DataVisible(true);
        }
        
        // Skip header if present
        if (sOptions.HasHeader()) {
            m_ColRowCellRange->Row(wTop)->DataVisible(true);
            wStartRow = wTop + 1;
        }
        
        // Check each row
        for (tIndex wRow = wStartRow; wRow <= wBottom; wRow++) {
            // Convert absolute row index to relative index (0-based relative to range top)
            tIndex wRelativeRow = wRow - wTop;
            tBool wMatches = RowMatches(sRange, wRelativeRow, sOptions);
            m_ColRowCellRange->Row(wRow)->DataVisible(wMatches);
        }
        
        return true;
    }

    tBool tRangeFilter::RowMatches(tRange* sRange, tIndex sRowIndex, const tFilterOptions& sOptions) {
        if (sRange == nullptr || sOptions.Criteria().empty()) {
            return true; // No criteria = all rows match
        }
        
        const std::vector<tFilterCriterion>& criteria = sOptions.Criteria();
        
        if (sOptions.Logic() == tFilterLogic::And) {
            // Excel-style: OR within the same column, AND across columns.
            std::map<tIndex, std::vector<const tFilterCriterion*>> wByColumn;
            for (const auto& criterion : criteria) {
                wByColumn[criterion.m_ColIndex].push_back(&criterion);
            }
            for (const auto& wEntry : wByColumn) {
                tBool wColumnMatches = false;
                for (const tFilterCriterion* wCriterion : wEntry.second) {
                    tVariant cellValue = GetCellValue(
                        sRange, sRowIndex, wCriterion->m_ColIndex, wCriterion->m_Operator);
                    tBool wMatches = MatchesCriterion(cellValue, *wCriterion);
#ifdef debugfilter
                    cout << "RowMatches: Row=" << sRowIndex << " Col=" << wCriterion->m_ColIndex
                         << " Value=" << cellValue.Str() << " Criterion=" << wCriterion->m_Value1.Str()
                         << " Matches=" << wMatches << endl;
#endif
                    if (wMatches) {
                        wColumnMatches = true;
                        break;
                    }
                }
                if (!wColumnMatches) {
                    return false;
                }
            }
            return true;
        } else {
            // At least one criterion must match
            for (const auto& criterion : criteria) {
                tVariant cellValue = GetCellValue(
                    sRange, sRowIndex, criterion.m_ColIndex, criterion.m_Operator);
                if (MatchesCriterion(cellValue, criterion)) {
                    return true;
                }
            }
            return false;
        }
    }

    tBool tRangeFilter::MatchesCriterion(const tVariant& sValue, const tFilterCriterion& sCriterion) {
        // Create non-const copies to use operators
        tVariant value = sValue;
        tVariant value1 = sCriterion.m_Value1;
        tVariant value2 = sCriterion.m_Value2;
        
        switch (sCriterion.m_Operator) {
            case tFilterOperator::Equals:
                return (value == value1);
                
            case tFilterOperator::NotEquals:
                return !(value == value1);
                
            case tFilterOperator::GreaterThan:
                return (value > value1);
                
            case tFilterOperator::GreaterThanOrEqual:
                return (value >= value1);
                
            case tFilterOperator::LessThan:
                return (value < value1);
                
            case tFilterOperator::LessThanOrEqual:
                return (value <= value1);
                
            case tFilterOperator::Between:
                return (value >= value1 && value <= value2);
                
            case tFilterOperator::IsEmpty: {
                // Check if value is null or empty string
                tVariant empty;
                tBool isEmpty = (value == empty);
                // Also check for empty string
                tBool isEmptyString = false;
                if (value.IsString()) {
                    tString str = value.String();
                    isEmptyString = str.empty();
                }
                return (isEmpty || isEmptyString);
            }
                
            case tFilterOperator::IsNotEmpty: {
                // Check if value is not null and not empty
                tVariant empty;
                tBool isEmpty = (value == empty);
                tBool isEmptyString = false;
                if (value.IsString()) {
                    tString str = value.String();
                    isEmptyString = str.empty();
                }
                return !(isEmpty || isEmptyString);
            }
                
            case tFilterOperator::Contains: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString search = value1.String();
                size_t pos = str.find(search);
                return (pos != tString::npos);
            }
                
            case tFilterOperator::StartsWith: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString prefix = value1.String();
                if (str.length() < prefix.length()) {
                    return false;
                }
                return (str.substr(0, prefix.length()) == prefix);
            }
                
            case tFilterOperator::EndsWith: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString suffix = value1.String();
                if (str.length() < suffix.length()) {
                    return false;
                }
                return (str.substr(str.length() - suffix.length()) == suffix);
            }
                
            case tFilterOperator::DoesNotContain: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString search = value1.String();
                size_t pos = str.find(search);
                return (pos == tString::npos);
            }
                
            case tFilterOperator::DoesNotBeginWith: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString prefix = value1.String();
                if (str.length() < prefix.length()) {
                    return true; // String is shorter than prefix, so doesn't begin with it
                }
                return (str.substr(0, prefix.length()) != prefix);
            }
                
            case tFilterOperator::DoesNotEndWith: {
                if (!value.IsString() || !value1.IsString()) {
                    return false;
                }
                tString str = value.String();
                tString suffix = value1.String();
                if (str.length() < suffix.length()) {
                    return true; // String is shorter than suffix, so doesn't end with it
                }
                return (str.substr(str.length() - suffix.length()) != suffix);
            }
                
            default:
                return false;
        }
    }

    namespace {
        tBool UsesDisplayTextForFilter(tFilterOperator sOperator) {
            switch (sOperator) {
                case tFilterOperator::Equals:
                case tFilterOperator::NotEquals:
                case tFilterOperator::Contains:
                case tFilterOperator::StartsWith:
                case tFilterOperator::EndsWith:
                case tFilterOperator::DoesNotContain:
                case tFilterOperator::DoesNotBeginWith:
                case tFilterOperator::DoesNotEndWith:
                    return true;
                default:
                    return false;
            }
        }
    }

    tVariant tRangeFilter::GetCellValue(tRange* sRange, tIndex sRowIndex, tIndex sColIndex,
                                        tFilterOperator sOperator) {
        if (sRange == nullptr || m_ColRowCellRange == nullptr) {
            return tVariant();
        }
        
        tIndex absoluteRow = sRange->TopIndex() + sRowIndex;
        tIndex absoluteCol = sRange->LeftIndex() + sColIndex;
        
        tCell* cell = m_ColRowCellRange->Cell(absoluteRow, absoluteCol);
        if (cell != nullptr) {
            if (UsesDisplayTextForFilter(sOperator)) {
                tWorkBook* wWorkBook = m_ColRowCellRange->Sheet()->WorkBook();
                if (wWorkBook != nullptr) {
                    tString wDisplay = wWorkBook->CellFormatString(cell);
                    if (!wDisplay.empty()) {
                        return tVariant(wDisplay.c_str());
                    }
                }
            }
            return cell->Value();
        }
        
        return tVariant(); // Empty cell
    }

    //=========================================================================
    // Utility function to convert FilterOperator to string
    //=========================================================================
    const char* FilterOperatorToString(tFilterOperator op) {
        switch (op) {
            case tFilterOperator::None:
                return "None";
            case tFilterOperator::Equals:
                return "Equals";
            case tFilterOperator::NotEquals:
                return "NotEquals";
            case tFilterOperator::GreaterThan:
                return "GreaterThan";
            case tFilterOperator::GreaterThanOrEqual:
                return "GreaterThanOrEqual";
            case tFilterOperator::LessThan:
                return "LessThan";
            case tFilterOperator::LessThanOrEqual:
                return "LessThanOrEqual (<=)";
            case tFilterOperator::Contains:
                return "Contains";
            case tFilterOperator::StartsWith:
                return "StartsWith";
            case tFilterOperator::EndsWith:
                return "EndsWith";
            case tFilterOperator::Between:
                return "Between";
            case tFilterOperator::IsEmpty:
                return "IsEmpty";
            case tFilterOperator::IsNotEmpty:
                return "IsNotEmpty";
            case tFilterOperator::DoesNotContain:
                return "DoesNotContain";
            case tFilterOperator::DoesNotBeginWith:
                return "DoesNotBeginWith";
            case tFilterOperator::DoesNotEndWith:
                return "DoesNotEndWith";
            default:
                return "Unknown";
        }
    }

    //=========================================================================
    // Utility function to convert string to FilterOperator
    //=========================================================================
    tFilterOperator StringToFilterOperator(const char* opString) {
        if (opString == nullptr) {
            return tFilterOperator::None;
        }
        
        if (strcmp(opString, "None") == 0) {
            return tFilterOperator::None;
        } else if (strcmp(opString, "Equals") == 0) {
            return tFilterOperator::Equals;
        } else if (strcmp(opString, "NotEquals") == 0) {
            return tFilterOperator::NotEquals;
        } else if (strcmp(opString, "GreaterThan") == 0) {
            return tFilterOperator::GreaterThan;
        } else if (strcmp(opString, "GreaterThanOrEqual") == 0) {
            return tFilterOperator::GreaterThanOrEqual;
        } else if (strcmp(opString, "LessThan") == 0) {
            return tFilterOperator::LessThan;
        } else if (strcmp(opString, "LessThanOrEqual") == 0) {
            return tFilterOperator::LessThanOrEqual;
        } else if (strcmp(opString, "Contains") == 0) {
            return tFilterOperator::Contains;
        } else if (strcmp(opString, "StartsWith") == 0) {
            return tFilterOperator::StartsWith;
        } else if (strcmp(opString, "EndsWith") == 0) {
            return tFilterOperator::EndsWith;
        } else if (strcmp(opString, "Between") == 0) {
            return tFilterOperator::Between;
        } else if (strcmp(opString, "IsEmpty") == 0) {
            return tFilterOperator::IsEmpty;
        } else if (strcmp(opString, "IsNotEmpty") == 0) {
            return tFilterOperator::IsNotEmpty;
        } else if (strcmp(opString, "DoesNotContain") == 0) {
            return tFilterOperator::DoesNotContain;
        } else if (strcmp(opString, "DoesNotBeginWith") == 0) {
            return tFilterOperator::DoesNotBeginWith;
        } else if (strcmp(opString, "DoesNotEndWith") == 0) {
            return tFilterOperator::DoesNotEndWith;
        } else {
            return tFilterOperator::None;
        }
    }

} // End of namespace

