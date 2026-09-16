//=============================================================================  
// TestSkRangeInsertDelete
// author : Stéphane Allez
//=============================================================================  
#ifndef TestSkInsertDeleteColRowByRect_hpp
#define TestSkInsertDeleteColRowByRect_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkInsertDeleteColRowByRect : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkInsertDeleteColRowByRect);
    CPPUNIT_TEST(TestInsertRowByRect);
    CPPUNIT_TEST(TestInsertColByRect);
    CPPUNIT_TEST(TestUndoInsertRowByRect);
    CPPUNIT_TEST(TestUndoInsertColByRect);
    CPPUNIT_TEST(TestUndoDeleteRowByRect);
    CPPUNIT_TEST(TestUndoDeleteColByRect);
    CPPUNIT_TEST(TestUndoRedoGridSnapshotByRect);
	CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application; // Pointer of static application
    tApi* m_Api;
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;

    void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    
    void DebugRange();

    void Fill();
    
    void FillWithSum();
    
    tBool TestValue(tString sRef,tVariant sValue);

    tString SnapshotCell(tIndex sRow, tIndex sCol) const;
    tString SnapshotRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) const;
    void AssertGridMatchesSnapshot(const tString& sExpected, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight,
                                   const tString& sMessage) const;

public:
    TestSkInsertDeleteColRowByRect();
public:
    void TestInsertRowByRect();
    void TestInsertColByRect();
    void TestUndoInsertRowByRect();
    void TestUndoInsertColByRect();
    void TestUndoDeleteRowByRect();
    void TestUndoDeleteColByRect();
    void TestUndoRedoGridSnapshotByRect();

    void setUp();
    void tearDown();
};
#endif