//=============================================================================
// SkSpreadSheet Function Array (dynamic arrays: SEQUENCE, SORT, ...)
//=============================================================================
// These functions return an in-memory array value (tStackType::t_Array) instead
// of a scalar. The evaluator (tCell::InternalCalculation) spills the top-level
// array result into the grid, and functions can compose in memory without
// materializing intermediate cells, e.g. =SORT(SEQUENCE(5)).
//=============================================================================
#ifndef SkFunctionArray_hpp
#define SkFunctionArray_hpp

#include "SkFunction.hpp"

namespace SkSpreadSheet {

    //=========================================================================
    //! SEQUENCE(rows, [columns], [start], [step])
    //! Generates an array of sequential numbers, filled row by row.
    //=========================================================================
    class tFunctionSequence : public tFunction {
    public:
        tFunctionSequence();

        /// @brief      Build the sequential array (returns a t_Array stack element).
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! RANDARRAY([rows], [columns], [min], [max], [integer])
    //! Spills a grid of random numbers (volatile). Defaults: 1×1, min=0, max=1, integer=FALSE.
    //=========================================================================
    class tFunctionRandArray : public tFunction {
    public:
        tFunctionRandArray();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! SORT(array, [sort_index], [sort_order], [by_col])
    //! Sorts the rows (or columns) of a range or in-memory array.
    //!   sort_index : 1-based key column (or row when by_col is TRUE). Default 1.
    //!   sort_order : 1 ascending (default), -1 descending.
    //!   by_col     : FALSE sorts rows (default), TRUE sorts columns.
    //=========================================================================
    class tFunctionSort : public tFunction {
    public:
        tFunctionSort();

        /// @brief      Sort a range or array and return a t_Array stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! UNIQUE(array, [by_col], [exactly_once])
    //! Returns the distinct rows (or columns) of a range or in-memory array,
    //! preserving first-occurrence order and spilling the result.
    //!   by_col       : FALSE compares/returns rows (default), TRUE columns.
    //!   exactly_once : FALSE returns each distinct value once (default),
    //!                  TRUE returns only values that occur exactly once.
    //=========================================================================
    class tFunctionUnique : public tFunction {
    public:
        tFunctionUnique();

        /// @brief      Deduplicate a range or array and return a t_Array stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! FILTER(array, include, [if_empty])
    //! Keeps the rows (or columns) of array for which the include mask is TRUE.
    //!   include  : a boolean vector matching array's height (filter rows) or width
    //!              (filter columns), e.g. B2:B10>100.
    //!   if_empty : value returned when nothing matches; without it an empty result is #CALC!.
    //=========================================================================
    class tFunctionFilter : public tFunction {
    public:
        tFunctionFilter();

        /// @brief      Filter a range or array by a boolean mask and return a t_Array stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! TRANSPOSE(array) — swap rows and columns; returns a t_Array stack element.
    //=========================================================================
    class tFunctionTranspose : public tFunction {
    public:
        tFunctionTranspose();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MMULT(array1, array2) — true matrix product (not element-wise).
    //=========================================================================
    class tFunctionMMult : public tFunction {
    public:
        tFunctionMMult();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MDETERM(array) — determinant of a square numeric matrix (scalar).
    //=========================================================================
    class tFunctionMDeterm : public tFunction {
    public:
        tFunctionMDeterm();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MINVERSE(array) — inverse of a square numeric matrix (spills). Singular → #NUM!.
    //=========================================================================
    class tFunctionMInverse : public tFunction {
    public:
        tFunctionMInverse();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! TOCOL / TOROW — flatten to one column (sToCol=true) or one row.
    //! TOCOL/TOROW(array, [ignore], [scan_by_col])
    //!   ignore: 0 keep all, 1 skip blanks, 2 skip errors, 3 skip blanks+errors.
    //!   scan_by_col: FALSE row-major (default), TRUE column-major.
    //=========================================================================
    class tFunctionToColRow : public tFunction {
    private:
        tBool m_ToCol;
    public:
        explicit tFunctionToColRow(tBool sToCol);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! CHOOSECOLS / CHOOSEROWS — pick columns (sByCol=true) or rows by 1-based indices.
    //! Negative indices count from the end. Order of indices is preserved.
    //=========================================================================
    class tFunctionChooseDim : public tFunction {
    private:
        tBool m_ByCol;
    public:
        explicit tFunctionChooseDim(tBool sByCol);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! EXPAND(array, rows, [columns], [pad_with])
    //! Pads array to at least the given dimensions. Omitted/non-positive rows or
    //! columns keep the source size (parser maps empty args to 0). Shrinking -> #VALUE!.
    //! Default pad_with is #N/A.
    //=========================================================================
    class tFunctionExpand : public tFunction {
    public:
        tFunctionExpand();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! WRAPROWS / WRAPCOLS — reshape a vector into 2D after wrap_count elements.
    //! WRAPROWS(vector, wrap_count, [pad_with]) fills by rows (sByRow=true).
    //! WRAPCOLS(vector, wrap_count, [pad_with]) fills by columns (sByRow=false).
    //! Default pad_with is #N/A. Non-vector (2D) input -> #VALUE!.
    //=========================================================================
    class tFunctionWrap : public tFunction {
    private:
        tBool m_ByRow;
    public:
        explicit tFunctionWrap(tBool sByRow);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! TAKE / DROP — shared implementation (sDrop selects Excel TAKE vs DROP semantics).
    //! TAKE keeps first/last N rows/cols (clamps). DROP excludes them (#CALC! if empty).
    //=========================================================================
    class tFunctionTakeDrop : public tFunction {
    private:
        tBool m_Drop;
    public:
        explicit tFunctionTakeDrop(tBool sDrop);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! HSTACK / VSTACK — shared implementation (sHorizontal selects the axis).
    //! Pads the shorter dimension with #N/A.
    //=========================================================================
    class tFunctionStack : public tFunction {
    private:
        tBool m_Horizontal;
    public:
        explicit tFunctionStack(tBool sHorizontal);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! SORTBY(array, by_array1, [sort_order1], [by_array2, sort_order2], ...)
    //! Reorders rows of array using one or more external key arrays.
    //=========================================================================
    class tFunctionSortBy : public tFunction {
    public:
        tFunctionSortBy();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MAP(array1, [array2, ...], lambda)
    //! Applies a LAMBDA element-wise across one or more equally-shaped arrays and
    //! returns an array of the same shape. The lambda arity must equal the number
    //! of arrays (each element is passed as one argument).
    //=========================================================================
    class tFunctionMap : public tFunction {
    public:
        tFunctionMap();

        /// @brief      Element-wise LAMBDA application; returns a t_Array stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! BYROW(array, lambda) / BYCOL(array, lambda) — shared implementation.
    //! Applies a 1-arg LAMBDA to each row (BYROW) or column (BYCOL), passing the
    //! slice as an in-memory array. Returns a column (BYROW) or row (BYCOL) of results.
    //=========================================================================
    class tFunctionByRowCol : public tFunction {
    private:
        tBool m_ByRow;
    public:
        explicit tFunctionByRowCol(tBool sByRow);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MAKEARRAY(rows, cols, lambda(row, col))
    //! Builds a rows×cols array by calling a 2-arg LAMBDA with 1-based indices.
    //=========================================================================
    class tFunctionMakeArray : public tFunction {
    public:
        tFunctionMakeArray();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! MUNIT(dimension) — n×n identity matrix (spills).
    //=========================================================================
    class tFunctionMUnit : public tFunction {
    public:
        tFunctionMUnit();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! TRIMRANGE(range, [row_trim_mode], [col_trim_mode])
    //! Trims leading/trailing blank rows and/or columns. Modes: 0 none, 1 leading,
    //! 2 trailing, 3 both (default). All-blank after trim → #CALC!.
    //=========================================================================
    class tFunctionTrimRange : public tFunction {
    public:
        tFunctionTrimRange();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! REDUCE(initial_value, array, lambda(accumulator, value))
    //! Folds array into a single accumulated value: acc starts at initial_value,
    //! then acc = lambda(acc, element) for each element (row-major). Returns a scalar.
    //=========================================================================
    class tFunctionReduce : public tFunction {
    public:
        tFunctionReduce();

        /// @brief      Fold an array with a 2-arg LAMBDA; returns a scalar stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! SCAN(initial_value, array, lambda(accumulator, value))
    //! Like REDUCE but returns the array of running accumulators (same shape as array).
    //=========================================================================
    class tFunctionScan : public tFunction {
    public:
        tFunctionScan();

        /// @brief      Running fold of an array with a 2-arg LAMBDA; returns a t_Array stack element.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

}; // end of namespace

#endif
