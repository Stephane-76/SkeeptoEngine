//=============================================================================
// SkSpreadSheet Range Filter
//=============================================================================
#ifndef SkRangeFilter_hpp
#define SkRangeFilter_hpp

#include "SkRange.hpp"
#include "SkTools.hpp"
#include <vector>

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
    //! Filter operator for range filtering
    enum class tFilterOperator : tByte {
        None=0,
        Equals,              // ==
        NotEquals,               // !=
        GreaterThan,             // >
        GreaterThanOrEqual,      // >=
        LessThan,                // <
        LessThanOrEqual,         // <=
        Contains,                // Contains text (for strings)
        StartsWith,              // Starts with text (for strings)
        EndsWith,                // Ends with text (for strings)
        Between,                 // Between two values
        IsEmpty,                 // Is null or empty
        IsNotEmpty,              // Is not null and not empty
        DoesNotContain,          // Does not contain text (for strings)
        DoesNotBeginWith,        // Does not begin with text (for strings)
        DoesNotEndWith           // Does not end with text (for strings)
    };

    //=========================================================================
    //! Logical operator for combining multiple filter criteria
    enum class tFilterLogic : tByte {
        And = 0,                 // All criteria must match
        Or                      // At least one criterion must match
    };

    //=========================================================================
    //! Filter criterion for a single column
    struct tFilterCriterion {
        //! Column index (0-based relative to range left)
        tIndex m_ColIndex;
        //! Filter operator
        tFilterOperator m_Operator;
        //! Filter value (for operators that need one value)
        tVariant m_Value1;
        //! Second filter value (for Between operator)
        tVariant m_Value2;
        
        /// @brief Constructor for single value operators
        /// @param[in] sColIndex Column index
        /// @param[in] sOperator Filter operator
        /// @param[in] sValue Filter value
        tFilterCriterion(tIndex sColIndex, tFilterOperator sOperator, const tVariant& sValue);
        
        /// @brief Constructor for Between operator
        /// @param[in] sColIndex Column index
        /// @param[in] sValue1 First value
        /// @param[in] sValue2 Second value
        tFilterCriterion(tIndex sColIndex, const tVariant& sValue1, const tVariant& sValue2);
        
        /// @brief Constructor for operators without value (IsEmpty, IsNotEmpty)
        /// @param[in] sColIndex Column index
        /// @param[in] sOperator Filter operator (IsEmpty or IsNotEmpty)
        tFilterCriterion(tIndex sColIndex, tFilterOperator sOperator);
    };

    //=========================================================================
    //! Filter options for range filtering
    class tFilterOptions {
    private:
        //! Vector of filter criteria
        std::vector<tFilterCriterion> m_Criteria;
        //! Logical operator for combining criteria
        tFilterLogic m_Logic;
        //! True if first row is header (not filtered)
        tBool m_HasHeader;
        
    public:
        /// @brief Constructor
        tFilterOptions();
        
        /// @brief Add filter criterion
        /// @param[in] sCriterion Filter criterion
        void AddCriterion(const tFilterCriterion& sCriterion);
        
        /// @brief Set logical operator
        /// @param[in] sLogic Logical operator (And or Or)
        void Logic(tFilterLogic sLogic);
        
        /// @brief Get logical operator
        /// @return tFilterLogic
        tFilterLogic Logic() const;
        
        /// @brief Set has header flag
        /// @param[in] sHasHeader True if first row is header
        void HasHeader(tBool sHasHeader);
        
        /// @brief Get has header flag
        /// @return tBool
        tBool HasHeader() const;
        
        /// @brief Get filter criteria
        /// @return const std::vector<tFilterCriterion>&
        const std::vector<tFilterCriterion>& Criteria() const;
        
        /// @brief Clear all criteria
        void Clear();
        
        /// @brief Check if sort options are empty (no filterd  columns)
        /// @return tBool
        tBool IsEmpty() const;
    };

    //=========================================================================
    //! Range filter functionality
    class tRangeFilter {
    private:
        tColRowCellRange* m_ColRowCellRange;
        tRect             m_Rect;
    public:
        /// @brief Constructor
        /// @param[in] sColRowCellRange ColRowCellRange container
        tRangeFilter(tColRowCellRange* sColRowCellRange);
        
        ///@ breif Clear (St Visble data on row rtrue
        void Clear();
        
        /// @brief Filter range data (hide rows that don't match criteria)
        /// @param[in] sRange Range to filter
        /// @param[in] sOptions Filter options
        /// @return tBool true if successful
        tBool Filter(tRange* sRange, const tFilterOptions& sOptions);
        
        /// @brief Get filtered rows (returns vector of row indices that match)
        /// @param[in] sRange Range to filter
        /// @param[in] sOptions Filter options
        /// @param[out] sFilteredRows Vector to fill with matching row indices
        /// @return tBool true if successful
        tBool ApplyOnRows(tRange* sRange, const tFilterOptions& sOptions);
        
        /// @brief Check if a row matches filter criteria
        /// @param[in] sRange Range
        /// @param[in] sRowIndex Row index (relative to range top)
        /// @param[in] sOptions Filter options
        /// @return tBool true if row matches
        tBool RowMatches(tRange* sRange, tIndex sRowIndex, const tFilterOptions& sOptions);
        
    private:
        /// @brief Check if a cell value matches a criterion
        /// @param[in] sValue Cell value
        /// @param[in] sCriterion Filter criterion
        /// @return tBool true if matches
        tBool MatchesCriterion(const tVariant& sValue, const tFilterCriterion& sCriterion);
        
        /// @brief Get cell value from range
        /// @param[in] sRange Range
        /// @param[in] sRowIndex Row index (relative to range top)
        /// @param[in] sColIndex Column index (relative to range left)
        /// @param[in] sOperator Filter operator (controls display vs raw value)
        /// @return tVariant Cell value
        tVariant GetCellValue(tRange* sRange, tIndex sRowIndex, tIndex sColIndex,
                              tFilterOperator sOperator);
    };

    //=========================================================================
    //! Utility function to convert FilterOperator to string
    //=========================================================================
    const char* FilterOperatorToString(tFilterOperator op);

    //=========================================================================
    //! Utility function to convert string to FilterOperator
    //=========================================================================
    tFilterOperator StringToFilterOperator(const char* opString);

} // End of namespace

#endif // SkRangeFilter_hpp

