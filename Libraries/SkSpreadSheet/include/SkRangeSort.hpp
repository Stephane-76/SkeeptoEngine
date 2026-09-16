//=============================================================================
// SkSpreadSheet Range Sort
//=============================================================================
#ifndef SkRangeSort_hpp
#define SkRangeSort_hpp

#include "SkRange.hpp"
#include "SkTools.hpp"
#include <vector>
#include <algorithm>

// Forward declarations
namespace SkRoot {
    class tVariant;
}

namespace SkSpreadSheet {
    class tColRowCellRange;
    class tCell;
    class tRange;
}

using namespace SkRoot;

namespace SkSpreadSheet {

    //=========================================================================
    //! Sort order for range sorting
    enum class tSortOrder : tByte {
        None =0,
        Ascending = 1,
        Descending = 2
    };

    //=========================================================================
    //! Sort criteria for a single column
    struct tSortColumn {
        //! Column index (0-based relative to range left)
        tIndex m_ColIndex;
        //! Sort order
        tSortOrder m_Order;
        
        /// @brief Constructor
        /// @param[in] sColIndex Column index
        /// @param[in] sOrder Sort order
        tSortColumn(tIndex sColIndex, tSortOrder sOrder = tSortOrder::Ascending) 
            : m_ColIndex(sColIndex), m_Order(sOrder) {}
    };

    //=========================================================================
    //! Sort reorganization method
    enum class tSortReorganizeMethod : tByte {
        ReorganizeRows = 0,  // Sort tColRow directly (faster)
        ReorganizeCells = 1  // Reorganize cells individually (more compatible)
    };

    //=========================================================================
    //! Sort options for range sorting
    class tSortOptions {
    private:
        //! Vector of sort columns (primary, secondary, etc.)
        std::vector<tSortColumn> m_SortColumns;
        //! True if first row is header (not sorted)
        tBool m_HasHeader;
        //! Method to use for reorganization
        tSortReorganizeMethod m_ReorganizeMethod;
        
    public:
        /// @brief Constructor
        tSortOptions();
        
        /// @brief Add sort column
        /// @param[in] sColIndex Column index (0-based relative to range left)
        /// @param[in] sOrder Sort order
        void AddSortColumn(tIndex sColIndex, tSortOrder sOrder = tSortOrder::Ascending);
        
        /// @brief Set has header flag
        /// @param[in] sHasHeader True if first row is header
        void HasHeader(tBool sHasHeader);
        
        /// @brief Get has header flag
        /// @return tBool
        tBool HasHeader() const;
        
        /// @brief Set reorganization method
        /// @param[in] sMethod Method to use for reorganization
        void ReorganizeMethod(tSortReorganizeMethod sMethod);
        
        /// @brief Get reorganization method
        /// @return tSortReorganizeMethod
        tSortReorganizeMethod ReorganizeMethod() const;
        
        /// @brief Get sort columns
        /// @return const std::vector<tSortColumn>&
        const std::vector<tSortColumn>& SortColumns() const;
        
        /// @brief Clear all sort columns
        void Clear();
        
        /// @brief Check if sort options are empty (no sort columns)
        /// @return tBool
        tBool IsEmpty() const;
    };

    //=========================================================================
    //! Row data for sorting (represents one row in the range)
    class tSortRowData {
    private:
        //! Original row index in the sheet
        tIndex m_OriginalRowIndex;
        //! Vector of cell values (one per column in range)
        std::vector<tVariant> m_CellValues;
        //! Vector of cell pointers (for preserving formulas and formatting)
        std::vector<tCell*> m_CellPointers;
        
    public:
        /// @brief Constructor
        /// @param[in] sOriginalRowIndex Original row index
        tSortRowData(tIndex sOriginalRowIndex);
        
        /// @brief Add cell value
        /// @param[in] sValue Cell value
        /// @param[in] sCell Cell pointer (can be nullptr)
        void AddCell(const tVariant& sValue, tCell* sCell = nullptr);
        
        /// @brief Get cell value at column index
        /// @param[in] sColIndex Column index (0-based)
        /// @return const tVariant&
        const tVariant& CellValue(tIndex sColIndex) const;
        
        /// @brief Get cell pointer at column index
        /// @param[in] sColIndex Column index (0-based)
        /// @return tCell*
        tCell* CellPointer(tIndex sColIndex) const;
        
        /// @brief Get original row index
        /// @return tIndex
        tIndex OriginalRowIndex() const;
        
        /// @brief Set original row index
        /// @param[in] sRowIndex Row index
        void OriginalRowIndex(tIndex sRowIndex);
        
        /// @brief Get number of cells
        /// @return tSize
        tSize CellCount() const;
    };

    //=========================================================================
    //! Comparator for sorting rows
    class tSortRowComparator {
    private:
        const std::vector<tSortColumn>& m_SortColumns;
        
    public:
        /// @brief Constructor
        /// @param[in] sSortColumns Sort columns configuration
        tSortRowComparator(const std::vector<tSortColumn>& sSortColumns);
        
        /// @brief Compare two rows
        /// @param[in] sRow1 First row
        /// @param[in] sRow2 Second row
        /// @return tBool true if sRow1 < sRow2
        tBool operator()(const tSortRowData& sRow1, const tSortRowData& sRow2) const;
        
    private:
        /// @brief Compare two variants
        /// @param[in] sV1 First variant
        /// @param[in] sV2 Second variant
        /// @return int -1 if sV1 < sV2, 0 if equal, 1 if sV1 > sV2
        int CompareVariants(const tVariant& sV1, const tVariant& sV2) const;
    };

    //=========================================================================
    //! Range sort functionality
    class tRangeSort {
    private:
        tColRowCellRange* m_ColRowCellRange;
        
    public:
        /// @brief Constructor
        /// @param[in] sColRowCellRange ColRowCellRange container
        tRangeSort(tColRowCellRange* sColRowCellRange);
        
        /// @brief Sort range data
        /// @param[in] sRange Range to sort
        /// @param[in] sOptions Sort options
        /// @return tBool true if successful
        tBool Sort(tRange* sRange, const tSortOptions& sOptions);
        
    private:
        /// @brief Collect all rows from range
        /// @param[in] sRange Range to collect from
        /// @param[out] sRows Vector to fill with row data
        /// @return tBool true if successful
        tBool CollectRows(tRange* sRange, std::vector<tSortRowData>& sRows);
        
        /// @brief Reorganize rows directly (faster than reorganizing cells individually)
        /// @param[in] sRange Range to reorganize
        /// @param[in] sSortedRows Sorted row data
        /// @return tBool true if successful
        tBool ReorganizeRows(tRange* sRange, const std::vector<tSortRowData>& sSortedRows);
        
        /// @brief Reorganize cells in range after sorting
        /// @param[in] sRange Range to reorganize
        /// @param[in] sSortedRows Sorted row data
        /// @return tBool true if successful
        tBool ReorganizeCells(tRange* sRange, const std::vector<tSortRowData>& sSortedRows);
        
        /// @brief Copy cell data (value, formula, formatting)
        /// @param[in] sSource Source cell
        /// @param[in] sDest Destination cell
        void CopyCellData(tCell* sSource, tCell* sDest);
    };

    //=========================================================================
    //! Utility function to convert SortOrder to string
    //=========================================================================
    const char* SortOrderToString(tSortOrder order);

    //=========================================================================
    //! Utility function to convert string to SortOrder
    //=========================================================================
    tSortOrder StringToSortOrder(const char* orderString);

} // End of namespace

#endif // SkRangeSort_hpp

