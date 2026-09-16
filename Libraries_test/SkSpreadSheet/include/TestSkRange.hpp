//==============================================================================
// TestSkRange
// le 31/10/2021 
//==============================================================================
#ifndef TestSkRange_hpp
#define TestSkRange_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkRange : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRange);
    CPPUNIT_TEST(TestRange);
    CPPUNIT_TEST(TestRectIsValid);
    CPPUNIT_TEST(TestFormulaRange);
	CPPUNIT_TEST(TestRangeNamed);
    CPPUNIT_TEST(TestRangeNamedCalcul);
	CPPUNIT_TEST(TestRangeNamedWidthRecover);
    CPPUNIT_TEST(TestRangeNamedMultiArea);
    CPPUNIT_TEST(TestRangeNamedMultiAreaOverlap);
    CPPUNIT_TEST(TestRangeNamedUpdate);
    CPPUNIT_TEST(TestRangeNamedRenameGating);
    CPPUNIT_TEST(TestRangeMerged);
    CPPUNIT_TEST(TestRangeMergedWithContainer);
    CPPUNIT_TEST(TestRangeWithUTF8);
    CPPUNIT_TEST(TestFormulaWithOutRange);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
public:
	void DebugRange();
    void DebugCell(tString sRef);
	TestSkRange();

private:
	void TestRange();
	void TestRectIsValid();
	void TestFormulaRange();
	void TestRangeNamed();
    void TestRangeNamedCalcul();
	void TestRangeNamedWidthRecover();
    void TestRangeNamedMultiArea();
    void TestRangeNamedMultiAreaOverlap();
    void TestRangeNamedUpdate();
    void TestRangeNamedRenameGating();
    void TestRangeMerged();
    void TestRangeMergedWithContainer();
    void TestRangeWithUTF8();
    void TestFormulaWithOutRange();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRange  */
