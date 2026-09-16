//==============================================================================
// TestSkSheet
// le 31/10/2021 
//==============================================================================

#ifndef TestSkSheet_hpp
#define TestSkSheet_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkSheet : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkSheet);
    CPPUNIT_TEST(TestAddSheet);
    CPPUNIT_TEST(TestAddSheetWithSpace);
	CPPUNIT_TEST(TestCalculateInterSheeet);
	CPPUNIT_TEST(TestInterSheeetDeleteColRowUndo);
	CPPUNIT_TEST(TestInterSheeetDeleteColRowUndoCovered);
	CPPUNIT_TEST(TestDeleteSheet);
    CPPUNIT_TEST(TestMoveToCell);
    CPPUNIT_TEST(TestJsonView);
    CPPUNIT_TEST(TestReadWriteJson);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

	// For Fill
	tInt m_NbCol;
	tInt m_NbRow;

public:
	TestSkSheet();
private:
	void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
	void DebugRange();

	void Fill();
    void TestAddSheet();
    void TestAddSheetWithSpace();
	void TestCalculateInterSheeet();
	void TestInterSheeetDeleteColRowUndo();
	void TestInterSheeetDeleteColRowUndoCovered();
	void TestDeleteSheet();
    void TestMoveToCell();
    void TestJsonView();
    void TestReadWriteJson();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkSheet */
