//==============================================================================
// TestSkDynamicRange - CppUnit tests for dynamic range (A1:INDEX(...), INDEX(...):B2, Fct1():Fct2())
//==============================================================================

#ifndef TestSkDynamicRange_hpp
#define TestSkDynamicRange_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <vector>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkDynamicRange : public CPPUNIT_NS::TestFixture {
	CPPUNIT_TEST_SUITE(TestSkDynamicRange);
	CPPUNIT_TEST(TestDynamicRangeRightCompil);
	CPPUNIT_TEST(TestDynamicRangeLeftCompil);
	CPPUNIT_TEST(TestDynamicRangeBothCompil);
	CPPUNIT_TEST(TestIndexResult);
	CPPUNIT_TEST(TestIndexWithNamedRangeResult);
	CPPUNIT_TEST(TestDynamicRangeWithNamedRangeResult);
	CPPUNIT_TEST(TestDynamicRangeLeftWithNamedRangeResult);
	CPPUNIT_TEST(TestDynamicRangeBothWithNamedRangeResult);
	CPPUNIT_TEST(TestIndexWithTableNamedRangeResult);
	CPPUNIT_TEST(TestDynamicRangeCombinedResult);
	CPPUNIT_TEST(TestIfSumDynamicRangeLeftLikeExcel);
	CPPUNIT_TEST(TestTableThisRowAndDynamicRange);
	CPPUNIT_TEST(TestIndexNestedIfIferrorCrossSheet);
	CPPUNIT_TEST(TestIndexSimpleCrossSheetRegression);
	CPPUNIT_TEST(TestSpillRefHashCompil);
	CPPUNIT_TEST_SUITE_END();

private:
	tApplication* m_Application;
	tApi* m_Api;

public:
	TestSkDynamicRange();

	void setUp();
	void tearDown();

private:
	void TestDynamicRangeRightCompil();
	void TestDynamicRangeLeftCompil();
	void TestDynamicRangeBothCompil();
	void TestIndexResult();
	void TestIndexWithNamedRangeResult();
	void TestDynamicRangeWithNamedRangeResult();
	void TestDynamicRangeLeftWithNamedRangeResult();
	void TestDynamicRangeBothWithNamedRangeResult();
	void TestIndexWithTableNamedRangeResult();

	void TestDynamicRangeCombinedResult();
	// Pattern from Excel: IF(condition<>"", SUM(INDEX(range,1,1):ThisRowCell), "")
	void TestIfSumDynamicRangeLeftLikeExcel();
	// Table with [#This Row] and dynamic range (Excel-style)
	void TestTableThisRowAndDynamicRange();
	// Nested IF/IFERROR/INDEX cross-sheet: repro for #N/A when INDEX(ref;$A15;C$6) with A15=1, C6=2
	void TestIndexNestedIfIferrorCrossSheet();
	// Regression: simple INDEX(cross-sheet range, row, col) still returns correct value (e.g. 125000)
	void TestIndexSimpleCrossSheetRegression();
	void TestSpillRefHashCompil();

	// Helper: compile formula and assert it contains the expected tKind in compiled formula
	void CompilFormulaAndAssertKind(const tChar* sFormula, tKind sExpectedKind);
	// Helper: insert a table (RangeData) with given columns for #This Row tests
	void InsertTableWithColumns(tString sName, tString sRef, const std::vector<tString>& sColumnNames);
};

#endif // TestSkDynamicRange_hpp
