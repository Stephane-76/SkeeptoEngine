//=============================================================================
// TestSkFunctionMath.hpp
//=============================================================================
#ifndef TestSkFunctionMath_hpp
#define TestSkFunctionMath_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkSpreadSheet;
#define TestFunctionAll
class TestSkFunction : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFunction);
#ifndef TestFunctionAll
    CPPUNIT_TEST(TestFunctionLet);
#endif
#ifdef TestFunctionAll
    CPPUNIT_TEST(TestFunctionDate);
    CPPUNIT_TEST(TestFunctionMath);
    CPPUNIT_TEST(TestFunctionSpreadSheet);
    CPPUNIT_TEST(TestFunctionSumIf);
    CPPUNIT_TEST(TestFunctionSumIfs);
    CPPUNIT_TEST(TestFunctionLogical);
    CPPUNIT_TEST(TestFunctionIfShortCircuit);
    CPPUNIT_TEST(TestFunctionLet);
    CPPUNIT_TEST(TestFunctionSwitch);
    CPPUNIT_TEST(TestFunctionSwitchArray);
    CPPUNIT_TEST(TestFunctionChoose);
    CPPUNIT_TEST(TestFunctionText);
    CPPUNIT_TEST(TestFunctionRowCol);
    CPPUNIT_TEST(TestFunctionVolatile);
    CPPUNIT_TEST(TestFunctionSubtotal);
    CPPUNIT_TEST(TestFunctionAggregate);
    CPPUNIT_TEST(TestFunctionExcelCalendar);
    CPPUNIT_TEST(TestRangeRefTransform);
    CPPUNIT_TEST(TestCollectFormulaRefs);
#endif
    CPPUNIT_TEST_SUITE_END();
    

public:
    TestSkFunction();
    ~TestSkFunction() override;
    
    void DrawCell(tInt sRowBegin,tInt sColBegin,tInt sRowEnd,tInt sColEnd);
    void DebugRow(tString sTitle);
    
    void TestFunctionDate();
    void TestFunctionExcelCalendar();
    void TestFunctionMath();
    void TestFunctionSpreadSheet();
    void TestFunctionSumIf();
    void TestFunctionSumIfs();
    void TestFunctionLogical();
    void TestFunctionIfShortCircuit();
    void TestFunctionLet();
    void TestFunctionSwitch();
    void TestFunctionSwitchArray();
    void TestFunctionChoose();
    void TestFunctionText();
    void TestFunctionRowCol();
    void TestFunctionVolatile();
    void TestFunctionSubtotal();
    void TestFunctionAggregate();
    void TestRangeRefTransform();
    void TestCollectFormulaRefs();
private:
    void TestVerifiyFormulaColRow(tIndex sRow,tIndex sCol,tIndex sSizeRow,tIndex sSizeCol,tBool sTestCellNull=true);
    tApplication* m_Application; // Pointer of static application
    tApi* m_Api;


public:
	void setUp() override;
	void tearDown() override;
};



#endif // TESTSKFUNCTIONMATH_HPP
