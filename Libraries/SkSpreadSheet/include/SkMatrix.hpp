//=============================================================================
// SkSpreadSheet Matrix
//=============================================================================
#ifndef SkMatrix_hpp
#define SkMatrix_hpp

#include <SkApplication.hpp>
#include <SkVariant.hpp>
#include <vector>
#include "SkColRowCellRange.hpp"
#include "SkTools.hpp"

namespace SkSpreadSheet {

    class tFormulaNamed;

    class tMatrix : public tClass {
        tCell*  m_Cell;
        tTempoRect m_Rect;
        /// Grid where result values are written (formula cell's sheet).
        tColRowCellRange* m_ColRowCellRange;
        /// Grid where operand values are read (defaults to formula sheet; use range's sheet for cross-sheet refs).
        tColRowCellRange* m_ReadColRowCellRange;
        /// When non-null, spill result is stored in this named formula buffer instead of materializing cells (CstSheetNamed).
        tFormulaNamed* m_FormulaNamedOutput;
        tVariant m_Value;
    public:
        /// @brief      Constructor for tMatrix. Writes use sCell's ColRowCellRange; reads use the same unless sReadColRow is set.
        /// @param[in]  sRect tTempoRect rectangle (top, left, bottom, right) of the matrix
        /// @param[in]  sCell tCell* formula cell (top-left of result for writing); used to get ColRowCellRange for EnsureCell
        tMatrix(const tTempoRect& sRect, tCell* sCell);

        /// @brief      Operand reads from sReadColRow when non-null; otherwise same grid as the two-arg constructor.
        /// @param[in]  sReadColRow MatrixValue reads from this grid (e.g. range sheet: tRange::Sheet()->ColRowCellRange()).
        tMatrix(const tTempoRect& sRect, tCell* sCell, tColRowCellRange* sReadColRow);

        /// @brief      Same as three-arg constructor; if sFormulaNamedOutput is set, SetValue writes spill to its buffer only.
        tMatrix(const tTempoRect& sRect, tCell* sCell, tColRowCellRange* sReadColRow, tFormulaNamed* sFormulaNamedOutput);

        /// @brief      Destructor for tMatrix.
        ~tMatrix();

        /// @brief      Clear for tMatrix.
        void Clear();

        /// @brief      Get rectangle of matrix.
        /// @return     const tTempoRect&
        const tTempoRect& GetRect() const;

        /// @brief      Get value of matrix.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @return     tVariant&
        tVariant& MatrixValue(tIndex sRow, tIndex sCol);

        /// @brief      Set value of cell at (sRow, sCol) in matrix.
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[in]  sValue tVariant& value to set
        void SetValue(tIndex sRow, tIndex sCol, const tVariant& sValue);

        /// @brief Copy this operand matrix into the result rectangle (identity spill: e.g. =A1:A2 at B1 without +0).
        tVariant Copy(const tTempoRect& sResultRect);

        /// @brief      Get height of matrix.
        /// @return     tIndex
        tIndex Height();

        /// @brief      Get width of matrix.
        /// @return     tIndex
        tIndex Width();

        /// @brief      Excel-style broadcast output size for two operand matrices (height/width; 1 repeats).
        /// @return     false if shapes are incompatible
        static tBool BroadcastResultSize(tIndex h1, tIndex w1, tIndex h2, tIndex w2, tIndex& outH, tIndex& outW);

        /// @brief True if spill may write into destRect: blank cells, the formula origin, or MatExtend cells owned by sOrigin.
        /// @param sLogicalSpillRectBeforeClamp If non-null, logical result rect before ClampResultRectToArrayFormulaOutput; when
        ///        strictly larger than sDestRect (OOXML clamp), allow overwriting stale values inside the spill ref.
        static tBool SpillDestinationIsClear(tCell* sOrigin, tColRowCellRange* sCr, const tTempoRect& sDestRect,
            const tTempoRect* sLogicalSpillRectBeforeClamp = nullptr);

        // Binary operations: Matrix op Scalar
        /// @brief      Add scalar to matrix and store result in result rect.
        tVariant Plus(tVariant& sVariant, const tTempoRect& sResultRect);

        /// @brief      Subtract scalar from matrix (or matrix from scalar if sScalarOnLeft).
        tVariant Minus(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft = false);

        /// @brief      Multiply matrix by scalar and store result in result rect.
        tVariant Multiply(tVariant& sVariant, const tTempoRect& sResultRect);

        /// @brief      Divide matrix by scalar (or scalar by matrix if sScalarOnLeft).
        tVariant Divide(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft = false);

        // Binary operations: Matrix op Matrix
        /// @brief      Add two matrices and store result in result rect.
        tVariant Plus(tMatrix* sMatrix, const tTempoRect& sResultRect);

        /// @brief      Subtract two matrices and store result in result rect.
        tVariant Minus(tMatrix* sMatrix, const tTempoRect& sResultRect);

        /// @brief      Multiply two matrices element-wise and store result in result rect.
        tVariant Multiply(tMatrix* sMatrix, const tTempoRect& sResultRect);

        /// @brief      Divide two matrices element-wise and store result in result rect.
        tVariant Divide(tMatrix* sMatrix, const tTempoRect& sResultRect);

        // String concatenation
        /// @brief      Concatenate scalar and matrix.
        tVariant Ampersand(tVariant& sVariant, const tTempoRect& sResultRect, tBool sScalarOnLeft = false);

        /// @brief      Concatenate two matrices.
        tVariant Ampersand(tMatrix* sMatrix, const tTempoRect& sResultRect);

        // Unary operations
        /// @brief      Apply unary minus (negation) to matrix and store result in result rect.
        tVariant UnaryMinus(const tTempoRect& sResultRect);

        /// @brief      Reserve grid for literal array; use SetLiteralValue to fill (no writes to sheet cells).
        void InitLiteralStorage(tIndex sH, tIndex sW);
        /// @brief      Store one cell of a literal matrix (row/col relative to matrix rect).
        void SetLiteralValue(tIndex sRow, tIndex sCol, const tVariant& sValue);

    private:
        // Literal array {a,b,...} values: kept in-memory only so SetValue does not overwrite the formula cell
        // (e.g. B8) with 0 before B8+offset is evaluated — same issue as Excel not materializing literals into the sheet.
        std::vector<std::vector<tVariant>> m_LiteralStorage;

        enum tBinaryOperation {
            tOp_Add,
            tOp_Subtract,
            tOp_Multiply,
            tOp_Divide
        };

        tVariant ApplyBinaryOperation(tVariant& sVariant, const tTempoRect& sResultRect, tBinaryOperation sOperation, tBool sScalarOnLeft = false);
        tVariant ApplyBinaryOperation(tMatrix* sMatrix, const tTempoRect& sResultRect, tBinaryOperation sOperation);

        tMatrix& operator +=(tMatrix* sMatrix);
        tMatrix& operator -=(tMatrix* sMatrix);
        tMatrix& operator *=(tMatrix* sMatrix);
        tMatrix& operator /=(tMatrix* sMatrix);
        tMatrix& operator %=(tMatrix* sMatrix);
        tMatrix& operator ^=(tMatrix* sMatrix);
        tMatrix& operator &=(tMatrix* sMatrix);
        tMatrix& operator |=(tMatrix* sMatrix);
    };
}

#endif // SkMatrix_hpp
