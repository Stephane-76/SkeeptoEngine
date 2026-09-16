//==============================================================================
// TestSkMatrix
// Tests for matrix operations
//==============================================================================
#ifndef TestSkMatrix_hpp
#define TestSkMatrix_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkMatrix : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkMatrix);
    CPPUNIT_TEST(TestMatrixEmpty);
    CPPUNIT_TEST(TestMatrixList);
    CPPUNIT_TEST(TestMatrixListFormulaNamed);
	CPPUNIT_TEST(TestMatrixSpillError);
	CPPUNIT_TEST(TestMatrixCalendar);
    CPPUNIT_TEST(TestMatrixCalendarOoxmlSingleRowRef);
	CPPUNIT_TEST(TestMatrixCalendarOoxmlSixBlocksLikeExcel);
	CPPUNIT_TEST(TestMatrixCalendarSkerReadJson);
	CPPUNIT_TEST(TestMatrixCalendar12MonthSkerReadJson);
	CPPUNIT_TEST(TestMatrixCalendarR1C1Headers);
    CPPUNIT_TEST(TestMatrixPlusScalar);
	CPPUNIT_TEST(TestMatrixPlusMatrix);
	CPPUNIT_TEST(TestMatrixMinusScalar);
	CPPUNIT_TEST(TestMatrixMinusMatrix);
	CPPUNIT_TEST(TestMatrixMultiplyScalar);
	CPPUNIT_TEST(TestMatrixMultiplyMatrix);
	CPPUNIT_TEST(TestMatrixDivideScalar);
	CPPUNIT_TEST(TestMatrixDivideMatrix);
	CPPUNIT_TEST(TestMatrixUnaryMinus);
	CPPUNIT_TEST(TestMatrixAmpersand);
	CPPUNIT_TEST(TestSequence);
	CPPUNIT_TEST(TestSort);
	CPPUNIT_TEST(TestSortNested);
	CPPUNIT_TEST(TestSortMultiKey);
	CPPUNIT_TEST(TestUnique);
	CPPUNIT_TEST(TestRangeCompareMask);
	CPPUNIT_TEST(TestFilter);
	CPPUNIT_TEST(TestTranspose);
	CPPUNIT_TEST(TestMUnit);
	CPPUNIT_TEST(TestTrimRange);
	CPPUNIT_TEST(TestMMult);
	CPPUNIT_TEST(TestMDeterm);
	CPPUNIT_TEST(TestMInverse);
	CPPUNIT_TEST(TestToColToRow);
	CPPUNIT_TEST(TestChooseColsRows);
	CPPUNIT_TEST(TestExpandWrap);
	CPPUNIT_TEST(TestTake);
	CPPUNIT_TEST(TestDrop);
	CPPUNIT_TEST(TestHStack);
	CPPUNIT_TEST(TestVStack);
	CPPUNIT_TEST(TestSortBy);
	CPPUNIT_TEST(TestCountSumIfsArray);
	CPPUNIT_TEST(TestLetLeagueTableLiterals);
	CPPUNIT_TEST(TestNamedLambda);
	CPPUNIT_TEST(TestNamedLambdaRecursive);
	CPPUNIT_TEST(TestLambdaHigherOrder);
	CPPUNIT_TEST(TestByRowByColMakeArray);
	CPPUNIT_TEST(TestInlineLambda);
	CPPUNIT_TEST(TestLetBoundLambdaCall);
	CPPUNIT_TEST(TestLambdaIsOmitted);
	CPPUNIT_TEST(TestMatrixReSpillOnModify);
	CPPUNIT_TEST(TestSpillSlaveDependentsRecalc);
	CPPUNIT_TEST(TestSpillSlaveCycleTerminates);
	CPPUNIT_TEST(TestSpillPersistOnReadJson);
	CPPUNIT_TEST(TestSpillAdjacentNoConflict);
	CPPUNIT_TEST(TestSpillEditOriginAfterReload);
	CPPUNIT_TEST(TestSpillLegacyMatrixEditAfterReload);
	CPPUNIT_TEST(TestMatrixResizeRespill);
	CPPUNIT_TEST(TestArrayLiteralSemicolonKeepsVertical);
	CPPUNIT_TEST(TestSpillRefHash);
	CPPUNIT_TEST_SUITE_END();

 
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
public:
	TestSkMatrix();
private:
	void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
	
    void TestMatrixEmpty();
	void TestMatrixUndoRedoSpill();
    void TestMatrixList();
    void TestMatrixListFormulaNamed();
	void TestMatrixSpillError();
	void TestMatrixCalendar();
	/// Same workbook setup as TestMatrixCalendar, then SetArrayFormulaOutputRect + RecalculateAll (SkExcel OOXML ref).
	void TestMatrixCalendarOoxmlSingleRowRef();
	/// Six blocks (one row × seven cols ref each), spaced like Calendar.xlsx rows 8/10/12… — SkExcel ApplyFormulas pattern per block.
	void TestMatrixCalendarOoxmlSixBlocksLikeExcel();
	/// Calendar.sker has spillrange but no persisted v/t on formula cells; ReadJson must EndCalculate them.
	void TestMatrixCalendarSkerReadJson();
	/// 12-month calendar .sker: date blocks on B8/B10/… ; E3 d/l must respill C–H (not column B only).
	void TestMatrixCalendar12MonthSkerReadJson();
	/// Calendar.sker weekday row: TEXT(R[2]C[0];"jjj") above a one-row spill (origin + MatExtend).
	void TestMatrixCalendarR1C1Headers();
	void TestMatrixPlusScalar();
	void TestMatrixPlusMatrix();
	void TestMatrixMinusScalar();
	void TestMatrixMinusMatrix();
	void TestMatrixMultiplyScalar();
	void TestMatrixMultiplyMatrix();
	void TestMatrixDivideScalar();
	void TestMatrixDivideMatrix();
	void TestMatrixUnaryMinus();
	void TestMatrixAmpersand();
	/// Dynamic arrays: SEQUENCE / RANDARRAY spill a generated grid.
	void TestSequence();
	/// Dynamic arrays: SORT reorders a range (ascending, descending).
	void TestSort();
	void TestUnique();
	void TestRangeCompareMask();
	void TestFilter();
	void TestTranspose();
	/// Dynamic arrays: MUNIT(n) spills an n×n identity matrix.
	void TestMUnit();
	/// Dynamic arrays: TRIMRANGE strips leading/trailing blank rows/cols.
	void TestTrimRange();
	/// Dynamic arrays: MMULT true matrix product.
	void TestMMult();
	/// Dynamic arrays: MDETERM determinant of a square matrix.
	void TestMDeterm();
	/// Dynamic arrays: MINVERSE inverse of a square matrix (singular → #NUM!).
	void TestMInverse();
	/// Dynamic arrays: TOCOL / TOROW flatten (row-major / column-major, ignore blanks).
	void TestToColToRow();
	/// Dynamic arrays: CHOOSECOLS / CHOOSEROWS pick by 1-based (negative = from end).
	void TestChooseColsRows();
	/// Dynamic arrays: EXPAND pads to rows/cols; WRAPROWS / WRAPCOLS reshape a vector.
	void TestExpandWrap();
	void TestTake();
	void TestDrop();
	void TestHStack();
	void TestVStack();
	void TestSortBy();
	/// Dynamic arrays: COUNTIF(S)/SUMIF(S) with an array criterion (e.g. UNIQUE(...)) spill one aggregate per element.
	void TestCountSumIfsArray();
	/// End-to-end league table (Excel D17 pattern): LET with array COUNTIFS + a SWITCH literal header + a
	/// multi-key SORT with literal keys. Several literals ({…}) coexist in one formula as in-memory arrays.
	void TestLetLeagueTableLiterals();
	/// Excel LAMBDA (named): define SURFACE=LAMBDA(l;h;l*h) then call SURFACE(3;4); literal args, cell args
	/// with dependency propagation, and wrong-arity -> #VALUE!.
	void TestNamedLambda();
	/// Recursive named LAMBDA: FACT=LAMBDA(n;IF(n<=1;1;n*FACT(n-1))). The body references its own name, which
	/// resolves as a lambda call (no self-ref in the dependency graph), so recursion runs via nested evaluation.
	void TestNamedLambdaRecursive();
	/// Higher-order functions taking a named LAMBDA as a first-class value: MAP, REDUCE, SCAN. The bare lambda
	/// name compiles to a LambdaRef opcode (pushing a t_Lambda) and each function invokes it per element.
	void TestLambdaHigherOrder();
	/// BYROW / BYCOL / MAKEARRAY (lambda HOFs that shape or generate arrays).
	void TestByRowByColMakeArray();
	void TestInlineLambda();
	void TestLetBoundLambdaCall();
	void TestLambdaIsOmitted();
	/// Dynamic arrays: SORT composes with an in-memory array (SORT(SEQUENCE(...))) and a literal.
	void TestSortNested();
	void TestSortMultiKey();
	/// Regression: modifying an existing matrix/spill formula must re-spill without a self #SPILL!.
	void TestMatrixReSpillOnModify();
	/// Changing a spill input must recalc formulas that reference a slave cell (not only the origin).
	void TestSpillSlaveDependentsRecalc();
	/// Origin formula that reads a slave-dependent must not Reduce forever (spill-mediated cycle).
	void TestSpillSlaveCycleTerminates();
	/// Persistence: an active spill (dynamic array + matrix) must survive WriteJson/ReadJson without a full RecalculateAll.
	void TestSpillPersistOnReadJson();
	/// Adjacent (non-overlapping) spills must coexist; only overlapping spills yield #SPILL!.
	void TestSpillAdjacentNoConflict();
	/// Reload (WriteJson/ReadJson, no RecalculateAll) then edit the spill origin's formula: must re-spill without #SPILL!.
	void TestSpillEditOriginAfterReload();
	/// Legacy .sker (matrix spill saved without AFO/slaves): editing the origin after load must re-spill without #SPILL!.
	void TestSpillLegacyMatrixEditAfterReload();
	/// A native matrix formula resized to a larger extent must re-derive its footprint, not clamp to the old one.
	void TestMatrixResizeRespill();
	/// FR locale: ';' inside an array literal {…} is a ROW separator, not the arg separator — it must NOT be
	/// rewritten to ',' in the stored formula key, otherwise a vertical literal reloads as horizontal (#SPILL!).
	void TestArrayLiteralSemicolonKeepsVertical();
	/// Excel D14#: COUNTA / SEQUENCE over a dynamic-array spill; #REF! when the cell is not an origin.
	void TestSpillRefHash();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkMatrix_hpp */
