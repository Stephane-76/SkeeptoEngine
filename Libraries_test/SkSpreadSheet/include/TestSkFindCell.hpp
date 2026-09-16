//==============================================================================
// TestSkFindCell
// Tests for FindCell match case / entire cell options
//==============================================================================
#ifndef TestSkFindCell_hpp
#define TestSkFindCell_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkFindCell : public CPPUNIT_NS::TestFixture {
	CPPUNIT_TEST_SUITE(TestSkFindCell);
	CPPUNIT_TEST(TestFindCellPartialIgnoreCase);
	CPPUNIT_TEST(TestFindCellPartialMatchCase);
	CPPUNIT_TEST(TestFindCellEntireIgnoreCase);
	CPPUNIT_TEST(TestFindCellEntireMatchCase);
	CPPUNIT_TEST(TestFindCellNoMatch);
	CPPUNIT_TEST(TestFindCellNumbers);
	CPPUNIT_TEST(TestJsonFindCell);
	CPPUNIT_TEST_SUITE_END();

private:
	tApplication* m_Application;
	tApi* m_Api;
	tFormatApi* m_FormatApi;

	void SetupFindGrid();
	void TestFindCellPartialIgnoreCase();
	void TestFindCellPartialMatchCase();
	void TestFindCellEntireIgnoreCase();
	void TestFindCellEntireMatchCase();
	void TestFindCellNoMatch();
	void TestFindCellNumbers();
	void TestJsonFindCell();

public:
	TestSkFindCell();
	void setUp();
	void tearDown();
};


#endif /* TestSkFindCell_hpp */
