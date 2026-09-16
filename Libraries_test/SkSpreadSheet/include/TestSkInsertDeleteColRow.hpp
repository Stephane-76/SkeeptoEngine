//==============================================================================
// TestSkInsertEraseColRow
// le 08/01/2021 
//==============================================================================
#ifndef TestSkInsertEraseColRow_hpp
#define TestSkInsertEraseColRow_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Exception.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkInsertDeleteColRow : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkInsertDeleteColRow);
    CPPUNIT_TEST(TestDeleteColRowSimple);
    CPPUNIT_TEST(TestDeleteColRowFill);
    CPPUNIT_TEST(TestBottomRight);
    CPPUNIT_TEST(TestRangeNamed);
	CPPUNIT_TEST(TestInsertDeleteColRow);
    CPPUNIT_TEST(TestMerged);
	CPPUNIT_TEST(TestSkInsertDeleteColRowWithFormula);
	CPPUNIT_TEST(TestSkInsertDeleteColRowWithCoveredRange);
	CPPUNIT_TEST(TestSkInsertDeleteColRowWithFormulaUndo);
	CPPUNIT_TEST(TestSkInsertDeleteColRowWithCoveredRangeUndo);
    CPPUNIT_TEST(TestSkPressure);
    CPPUNIT_TEST(TestUndoRedoGridSnapshot);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
	// For Fill
	tInt m_NbCol;
	tInt m_NbRow;
public:
	TestSkInsertDeleteColRow();
    
private:
	void DrawCell(tString sOperation,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
	void UndoOperation();

	void DebugRange();

	void Fill();
    
    void DebugMerged(tRect sRect,tString sMessage);

    void TestDeleteColRowSimple();
    void TestDeleteColRowFill();
    void TestBottomRight();
	void TestInsertDeleteColRow();
    void TestMerged();
    void TestRangeNamed();
	void TestSkInsertDeleteColRowWithFormula();
	void TestSkInsertDeleteColRowWithCoveredRange();
	void TestSkInsertDeleteColRowWithFormulaUndo();
	void TestSkInsertDeleteColRowWithCoveredRangeUndo();
    
    
    void TestSkPressure();
    void TestUndoRedoGridSnapshot();

    tString SnapshotCell(tIndex sRow, tIndex sCol) const;
    tString SnapshotRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) const;
    void AssertGridMatchesSnapshot(const tString& sExpected, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight,
                                   const tString& sMessage) const;

public:
	void setUp();
	void tearDown();


};


#endif    /* TestSkCell */
