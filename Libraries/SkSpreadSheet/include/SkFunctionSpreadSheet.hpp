//=============================================================================
// SkSpreadSheet Function SpreadSheet
//=============================================================================
#ifndef SkFunctionSpreadSheet_hpp
#define SkFunctionSpreadSheet_hpp

#include "SkFunction.hpp"
#include <vector>

namespace SkSpreadSheet {

    //=========================================================================
    //! Function Count
    //=========================================================================
    class tFunctionCount;
	//! Call back for function count
	class tCallBackRangeCount : public tCallBackRangeFunction {
        private:
        public:
            /// @brief		Constructor SkCallBackRangeCount with owner sColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            tCallBackRangeCount(tColRowCellRange* sColRowCellRange);
            
            /// @brief		Call method for calculate m_Value if return false stop process.
            /// @param[in]	sAllocatorRef tInt Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override; // Return false for Stop
        };

    //=========================================================================
    //! Function count
    //=========================================================================
    class tFunctionCount : public tFunction {
        public:
            /// @brief		Constructor SkFunctionCount.
            tFunctionCount();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Call back for function counta
    class tCallBackRangeCountA : public tCallBackRangeFunction {
        public:
            /// @brief		Constructor SkCallBackRangeCountA with owner sColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            tCallBackRangeCountA(tColRowCellRange* sColRowCellRange);
            
            /// @brief		Call method for calculate m_Value if return false stop process.
            /// @param[in]	sAllocatorRef tInt Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override; // Return false for Stop
    };

    //=========================================================================
    //! Function CountA
    //=========================================================================
    class tFunctionCountA : public tFunction {
        public:
            /// @brief		Constructor SkFunctionCountA.
            tFunctionCountA();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Call back for function countblank
    class tCallBackRangeCountBlank : public tCallBackRangeFunction {
        public:
            /// @brief		Constructor SkCallBackRangeCountBlank with owner sColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            tCallBackRangeCountBlank(tColRowCellRange* sColRowCellRange);
            
            /// @brief		Call method for calculate m_Value if return false stop process.
            /// @param[in]	sAllocatorRef tInt Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override; // Return false for Stop
    };

    //=========================================================================
    //! Function CountBlank
    //=========================================================================
    class tFunctionCountBlank : public tFunction {
        public:
            /// @brief		Constructor SkFunctionCountBlank.
            tFunctionCountBlank();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Index  
    //=========================================================================
    class tFunctionIndex : public tFunction {
        public:
            tFunctionIndex();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function OFFSET — Excel-style: OFFSET(reference, rows, cols, [height], [width])
    //=========================================================================
    class tFunctionOffset : public tFunction {
        public:
            tFunctionOffset();

            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function DATARANGE — serialize range refs to a JSON array string
    //=========================================================================
    class tFunctionDataRange : public tFunction {
        public:
            tFunctionDataRange();

            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function JSON — serialize range cell values to a JSON array string (ComboBox, …)
    //=========================================================================
    class tFunctionJson : public tFunction {
        public:
            tFunctionJson();

            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function LOOKUP (vector form)
    //! LOOKUP(lookup_value, lookup_vector, [result_vector])
    //! Approximate match on a sorted ascending vector (largest key <= lookup).
    //=========================================================================
    class tFunctionLookup : public tFunction {
        public:
            tFunctionLookup();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Index  
    //=========================================================================
    class tFunctionVLookup : public tFunction {
        public:
            tFunctionVLookup();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Index  
    //=========================================================================
    class tFunctionHLookup : public tFunction {
        public:
            tFunctionHLookup();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Match
    //=========================================================================
    class tFunctionMatch : public tFunction {
        public:
            tFunctionMatch();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function XLOOKUP (Excel XLOOKUP)
    //! XLOOKUP(lookup_value, lookup_array, return_array, [if_not_found], [match_mode], [search_mode])
    //! MVP: exact match (match_mode 0 / default), search first-to-last. lookup_array must be a
    //! single row or column; return_array may be 1D (scalar) or 2D (matching row/column spill).
    //=========================================================================
    class tFunctionXLookup : public tFunction {
        public:
            tFunctionXLookup();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function XMATCH (Excel XMATCH)
    //! XMATCH(lookup_value, lookup_array, [match_mode], [search_mode])
    //! MVP: exact match (match_mode 0), search_mode 1 or -1. Returns 1-based position or #N/A.
    //=========================================================================
    class tFunctionXMatch : public tFunction {
        public:
            tFunctionXMatch();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };
  
   
    //=========================================================================
    //! Function Row
    //=========================================================================
    class tFunctionRow : public tFunction {
        private:
            tItem* m_ItemRef;
        public:
            tFunctionRow();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        
            void PassByRef(tItem* sItem) override;
            tBool ByRef() override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Col
    //=========================================================================
    class tFunctionColumn : public tFunction {
        private:
            tItem* m_ItemRef;
        public:
            tFunctionColumn();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        
            void PassByRef(tItem* sItem) override;
            tBool ByRef() override;
            tFunctionSpillKind SpillKind() const override;
    };


    //=========================================================================
    //! Function Rows  
    //=========================================================================
    class tFunctionRows : public tFunction {
        public:
            tFunctionRows();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tBool ByRef() override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function Columns  
    //=========================================================================
    class tFunctionColumns : public tFunction {
        public:
            tFunctionColumns();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tBool ByRef() override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function ADDRESS (Excel: row_num, column_num, [abs_num], [a1], [sheet_text])
    //=========================================================================
    class tFunctionAddress : public tFunction {
        public:
            tFunctionAddress();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function INDIRECT (Excel: ref_text, [a1])
    //=========================================================================
    class tFunctionIndirect : public tFunction {
        private:
            tItem* m_ItemRef;
        public:
            tFunctionIndirect();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            void PassByRef(tItem* sItem) override;
            tBool ByRef() override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Call back for function SUMIF
    class tCallBackRangeSumIf : public tCallBackRangeFunction {
        private:
            tVariant m_Criteria;
            tRange* m_SumRange;
            tRange* m_CriteriaRange;
        public:
            /// @brief		Constructor tCallBackRangeSumIf with owner sColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            /// @param[in]  sCriteria tVariant criteria to match
            /// @param[in]  sSumRange tRange* range to sum (optional)
            /// @param[in]  sCriteriaRange tRange* criteria range for position calculation
            tCallBackRangeSumIf(tColRowCellRange* sColRowCellRange, tVariant sCriteria, tRange* sSumRange = nullptr, tRange* sCriteriaRange = nullptr);
            
            /// @brief		Call method for calculate m_Value if return false stop process.
            /// @param[in]	sAllocatorRef tInt Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override; // Return false for Stop
        };

    //=========================================================================
    //! Function SUMIF
    //=========================================================================
    class tFunctionSumIf : public tFunction {
        public:
            /// @brief		Constructor tFunctionSumIf.
            tFunctionSumIf();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function COUNTIF
    //=========================================================================
    class tFunctionCountIf : public tFunction {
        public:
            /// @brief		Constructor tFunctionCountIf.
            tFunctionCountIf();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Shared aggregator for SUMIFS / MAXIFS / MINIFS
    //!   (value_range, criteria_range1, criteria1, [criteria_range2, criteria2], ...)
    //=========================================================================
    enum class tAggIfsKind { Sum, Max, Min };

    class tFunctionAggIfs : public tFunction {
        private:
            tAggIfsKind m_Kind;
        public:
            explicit tFunctionAggIfs(tAggIfsKind sKind);

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function COUNTIFS (multiple criteria)
    //=========================================================================
    class tFunctionCountIfs : public tFunction {
        public:
            /// @brief		Constructor tFunctionCountIfs.
            tFunctionCountIfs();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function AVERAGEIF
    //=========================================================================
    class tFunctionAverageIf : public tFunction {
        public:
            /// @brief		Constructor tFunctionAverageIf.
            tFunctionAverageIf();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Function AVERAGEIFS (multiple criteria)
    //=========================================================================
    class tFunctionAverageIfs : public tFunction {
        public:
            /// @brief		Constructor tFunctionAverageIfs.
            tFunctionAverageIfs();

            /// @brief        Call function with arguments.
            /// @param[in]  sArgVector tStackElemVector* vector of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Call back for function SUBTOTAL
    class tCallBackRangeSubtotal : public tCallBackRangeFunction {
        private:
            tInt m_FunctionNum;  // Function code (1-11 or 101-111)
            tBool m_IgnoreHidden; // True if function_num >= 101
            tInt m_Count;        // For AVERAGE, COUNT, etc.
            tBool m_FirstCall;   // For MIN, MAX
        public:
            /// @brief Get count (for AVERAGE, COUNT, COUNTA)
            tInt Count() const { return m_Count; }
            /// @brief		Constructor tCallBackRangeSubtotal with owner sColRowCellRange.
            /// @param[in]  sColRowCellRange tColRowCellRange*
            /// @param[in]  sFunctionNum tInt function code (1-11 or 101-111)
            tCallBackRangeSubtotal(tColRowCellRange* sColRowCellRange, tInt sFunctionNum);
            
            /// @brief		Call method for calculate m_Value if return false stop process.
            /// @param[in]	sAllocatorRef tInt Index on allocator cell
            tBool CallBack(tAllocatorRef sAllocatorRef) override; // Return false for Stop
    };

    //=========================================================================
    //! Function SUBTOTAL
    //=========================================================================
    class tFunctionSubtotal : public tFunction {
        public:
            /// @brief		Constructor tFunctionSubtotal.
            tFunctionSubtotal();

            /// @brief        Call function with arguments.
            /// @param[in]  sStackElems tStackElems* stack containing arguments
            /// @param[in]  sNbArg tShort number of arguments
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };

    //=========================================================================
    //! Call back for function AGGREGATE (collects filtered cell values)
    //=========================================================================
    class tCallBackRangeAggregate : public tCallBackRangeFunction {
        private:
            tBool m_IgnoreHidden;
            tBool m_IgnoreErrors;
            tBool m_IgnoreNested;
            tInt m_CountA;
            std::vector<tDouble> m_NumericValues;
        public:
            tInt CountA() const { return m_CountA; }
            const std::vector<tDouble>& NumericValues() const { return m_NumericValues; }

            tCallBackRangeAggregate(tColRowCellRange* sColRowCellRange,
                                    tBool sIgnoreHidden, tBool sIgnoreErrors, tBool sIgnoreNested);

            tBool CallBack(tAllocatorRef sAllocatorRef) override;
    };

    //=========================================================================
    //! Function AGGREGATE
    //=========================================================================
    class tFunctionAggregate : public tFunction {
        public:
            tFunctionAggregate();
            tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
            tFunctionSpillKind SpillKind() const override;
    };
    
} // End of namespace

#endif // SK_FUNCTION_SPREADSHEET_HPP
