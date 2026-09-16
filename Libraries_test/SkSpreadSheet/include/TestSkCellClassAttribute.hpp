//==============================================================================
// TestSkCellClass
// le 04/10/2023 
//==============================================================================
#ifndef TestSkCellClassAttribute_hpp
#define TestSkCellClassAttribute_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkCellClassAttribute : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkCellClassAttribute);
    CPPUNIT_TEST(TestClassAttributeCopy);
    CPPUNIT_TEST(TestClassAttributeMethod);
	CPPUNIT_TEST(TestCreateInstanceClassAttribute);
	CPPUNIT_TEST(TestClassAttributeUndoRedo);
	CPPUNIT_TEST(TestUndoMoveAttributeFormulaRefs);
	CPPUNIT_TEST(TestCellAttributeJsonFormula);
    CPPUNIT_TEST(TestClassAttribute);
    CPPUNIT_TEST(TestClassAttributeMultiSheet);
    CPPUNIT_TEST(TestClassAttributeDeleteColRow);
	CPPUNIT_TEST(TestFloatingObjectLayout);
	CPPUNIT_TEST(TestFloatingObjectAttributesBatch);
	CPPUNIT_TEST(TestFloatingObjectLayoutAnchorCell);
	CPPUNIT_TEST(TestFloatingObjectJsonLoad);
	CPPUNIT_TEST(TestFloatingObjectJsonLoadLazyHost);
	CPPUNIT_TEST(TestFloatingObjectDataRangeInsertRow);
	CPPUNIT_TEST(TestUndoMoveDataRangeFloatingObject);
	CPPUNIT_TEST(TestFloatingObjectImageDataUrl);
	CPPUNIT_TEST(TestClassAttributeTitleRangeSpill);
	CPPUNIT_TEST(TestWorkBookModelsInJsonRoundTrip);
	CPPUNIT_TEST(TestFloatingObjectLoadWithoutHostSheetInSker);
	CPPUNIT_TEST(TestUnknownCellClassAutoStubOnLoad);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
    void DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
  
public:
	TestSkCellClassAttribute();
private:
	void CreateClass();
	void TestCreateInstanceClassAttribute();
	void TestClassAttributeUndoRedo();
	void TestUndoMoveAttributeFormulaRefs();
	void TestCellAttributeJsonFormula();
    
    void CreateGenericClass();
    void RegisterPieChartClass();
    void RegisterImageClass();
	void CreateGenericFloatingClass();
	void TestFloatingObjectLayout();
	void TestFloatingObjectAttributesBatch();
	void TestFloatingObjectLayoutAnchorCell();
	void TestFloatingObjectJsonLoad();
	void TestFloatingObjectJsonLoadLazyHost();
	void TestFloatingObjectDataRangeInsertRow();
	void TestUndoMoveDataRangeFloatingObject();
	void TestFloatingObjectImageDataUrl();
    void TestClassAttribute();
    void TestClassAttributeMultiSheet();
    void TestClassAttributeDeleteColRow();
    
    void TestClassAttributeMethod();
    
    void TestClassAttributeCopy();
	void TestClassAttributeTitleRangeSpill();
	void TestWorkBookModelsInJsonRoundTrip();
	void TestFloatingObjectLoadWithoutHostSheetInSker();
	void TestUnknownCellClassAutoStubOnLoad();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkCell */
