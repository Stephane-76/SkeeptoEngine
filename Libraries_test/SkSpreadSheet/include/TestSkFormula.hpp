//==============================================================================
// TestSkFormula
// le 31/10/2021 
//==============================================================================

#ifndef TestSkFormula_hpp
#define TestSkFormula_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkFormula : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFormula);
	CPPUNIT_TEST(TestFormulaPriority);
	CPPUNIT_TEST(TestLeadingUnaryPlus);
	CPPUNIT_TEST(TestPercentLiteral);
	CPPUNIT_TEST(TestIfIsNaDateNamedRange);
	CPPUNIT_TEST(TestFormula);
	CPPUNIT_TEST(TestRecursive);
	CPPUNIT_TEST(TestClearStaleRecursive);
	CPPUNIT_TEST(TestRecursiveNamedRange);
	CPPUNIT_TEST(TestNamedRangeR1C1Offset);
	CPPUNIT_TEST(TestNamedRangeR1C1OffsetUnderscoreBeforeR);
	CPPUNIT_TEST(TestVlookupNamedRangeR1C1Offset);
	CPPUNIT_TEST(TestCalculate);
	CPPUNIT_TEST(TestCalculateString);
	CPPUNIT_TEST(TestFunctionLogical);
    CPPUNIT_TEST(TestFunctionDate);
    CPPUNIT_TEST(TestMultiSheet);
	CPPUNIT_TEST(TestFormulaRef);
	CPPUNIT_TEST(TestFormulaNamedSingleCellInConcat);
	CPPUNIT_TEST(TestFormulaOffset);
	CPPUNIT_TEST(TestFormulaIndirect);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
public:
	TestSkFormula();
private:
	void DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

	void PrintSize();

	void TestFormulaA1R1C1(tCell* sCell,tString sFormula);
	void TestFormulaPriority();
	void TestLeadingUnaryPlus();
	void TestPercentLiteral();
	void TestIfIsNaDateNamedRange();
	void TestFormula();
	void TestRecursive();
	void TestClearStaleRecursive();
	void TestRecursiveNamedRange();
	void TestNamedRangeR1C1Offset();
	void TestNamedRangeR1C1OffsetUnderscoreBeforeR();
	void TestVlookupNamedRangeR1C1Offset();
	void TestCalculate();
	void TestCalculateString();
	void TestFunctionLogical();
    void TestFunctionDate();
    void TestMultiSheet();
    void TestFormulaRef();
    void TestFormulaNamedSingleCellInConcat();
    void TestFormulaOffset();
    void TestFormulaIndirect();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkFormula */
