//==============================================================================
// TestSkSheet
// le 31/10/2021 
//==============================================================================

#ifndef TestSkTree_hpp
#define TestSkTree_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkTree : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkTree);
	CPPUNIT_TEST(TestTreeRow);
    CPPUNIT_TEST(TestTreeCol);
	CPPUNIT_TEST(TestTreeInsertDeleteRow);
    CPPUNIT_TEST(TestTreeInsertDeleteCol);
    CPPUNIT_TEST(TestTreeChangeUndoNoHang);
    CPPUNIT_TEST(TestTreeOpenCloseRow1);
	CPPUNIT_TEST(TestJsonViewRow);
	CPPUNIT_TEST(TestJsonViewCol);
    CPPUNIT_TEST(TestWeb);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

public:
    TestSkTree();
private:
    tString DebugColRow(tColRowCellRange* sColRowCellRange,tIndex sIndex,tBool sIsRow);
    void DebugTree(tBool sIsRow,tString sTitle,tIndex sBegin,tIndex sEnd);
    void AssertColRow(tColRowCellRange* sColRowCellRange,tIndex sIndex,tBool sIsRow,
                      const tString& sExpected,const tString& sMessage);
    void AssertTreeLinks(tColRowCellRange* sColRowCellRange,tBool sIsRow,
                         tIndex sBegin,tIndex sEnd,const tString& sMessage);
    void TestTreeRow();
    void TestTreeCol();
    void TestTreeInsertDeleteRow();
    void TestTreeInsertDeleteCol();
    void TestTreeChangeUndoNoHang();
    void TestTreeOpenCloseRow1();

	void TestJsonViewRow();
	void TestJsonViewCol();
    
    void TestWeb();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkSheet */
