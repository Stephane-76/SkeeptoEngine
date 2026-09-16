//==============================================================================
// TestSkRangeData
// Test for UndoInsertRangeData function
//==============================================================================
#ifndef TestSkRangeData_hpp
#define TestSkRangeData_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <SkRangeData.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkRangeData : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRangeData);
    CPPUNIT_TEST(TestTable);
	CPPUNIT_TEST(TestInsertRangeData);
	CPPUNIT_TEST(TestInsertRangeDataUndoRedo);
	CPPUNIT_TEST(TestApiUndoAddRangeNamedUndoRedo);
	CPPUNIT_TEST(TestApiUndoAddRangeDataUndoRedo);
	CPPUNIT_TEST(TestUndoAddRangeDataEmptyJsonFromTopRowSingleColumn);
	CPPUNIT_TEST(TestApiUndoApplyRangeDataUndoRedo);
	CPPUNIT_TEST(TestApiUndoDeleteRangeNamedUndoRedo);
	CPPUNIT_TEST(TestInsertRangeDataWithColumns);
 	CPPUNIT_TEST(TestDeleteColRowCovered);
    CPPUNIT_TEST(TestRangeDataSortSimple);
    CPPUNIT_TEST(TestRangeDataSortAscending);
    CPPUNIT_TEST(TestRangeDataSortDescending);
    CPPUNIT_TEST(TestRangeDataSortMultiColumn);
    CPPUNIT_TEST(TestRangeDataSortMultiColumnReorganizeCells);
    CPPUNIT_TEST(TestRangeDataFilterEquals);
    CPPUNIT_TEST(TestRangeDataFilterGreaterThan);
    CPPUNIT_TEST(TestRangeDataFilterContains);
    CPPUNIT_TEST(TestRangeDataFilterMultiColumn);
    CPPUNIT_TEST(TestRangeDataFilterMultiValueSameColumn);
    CPPUNIT_TEST(TestRangeDataFilterPersistsAfterSort);
    CPPUNIT_TEST(TestRangeDataFilterRecalculatesSubtotal);
    CPPUNIT_TEST(TestTableFormula);
    CPPUNIT_TEST(TestTableFormulaInPlace);
    CPPUNIT_TEST(TestTableColonColumnIsFullColumn);
    CPPUNIT_TEST(TestTableIndexMatchThisRowPerRow);
    CPPUNIT_TEST(TestTableJsonSaveAndLoad);
    CPPUNIT_TEST(TestTableHeaderDuplicateNameRejected);
    CPPUNIT_TEST(TestInsertRowAppliesCalculatedColumnFormula);
    
#ifndef __EMSCRIPTEN__
    CPPUNIT_TEST(TestCsvLoadAndSort);
#endif
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

	// For Fill
	tInt m_NbCol;
	tInt m_NbRow;
public:
	TestSkRangeData();
private:
	void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

	void Fill();
    
    void TestTable();
	void TestInsertRangeData();
	void TestInsertRangeDataUndoRedo();
	void TestApiUndoAddRangeNamedUndoRedo();
	void TestApiUndoAddRangeDataUndoRedo();
	void TestUndoAddRangeDataEmptyJsonFromTopRowSingleColumn();
	void TestApiUndoApplyRangeDataUndoRedo();
	void TestApiUndoDeleteRangeNamedUndoRedo();
	void TestInsertRangeDataWithColumns();
    void TestDeleteColRowCovered();
    void TestRangeDataSortSimple();
    void TestRangeDataSortAscending();
    void TestRangeDataSortDescending();
    void TestRangeDataSortMultiColumn();
    void TestRangeDataSortMultiColumnReorganizeCells();
    void TestRangeDataFilterEquals();
    void TestRangeDataFilterGreaterThan();
    void TestRangeDataFilterContains();
    void TestRangeDataFilterMultiColumn();
    void TestRangeDataFilterMultiValueSameColumn();
    void TestRangeDataFilterPersistsAfterSort();
    void TestRangeDataFilterRecalculatesSubtotal();
    void TestTableFormula();
    void TestTableFormulaInPlace();
    /// Excel Table[[Col]:[Col]] is the full data column (League-Table Part A MATCH on [[RANK]:[RANK]]).
    void TestTableColonColumnIsFullColumn();
    /// Each Table A3 row: MATCH([#This Row],[POS]:[POS], [[RANK]:[RANK]]) — not only the first row.
    void TestTableIndexMatchThisRowPerRow();
    void TestTableJsonSaveAndLoad();
    void TestTableHeaderDuplicateNameRejected();
    void TestInsertRowAppliesCalculatedColumnFormula();
    
#ifndef __EMSCRIPTEN__
    void TestCsvLoadAndSort();
#endif
public:
	void setUp();
	void tearDown();
};


#endif  /* TestSkRangeData_hpp */

