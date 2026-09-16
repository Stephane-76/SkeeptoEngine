//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet Web
// le 29/11/2024
//==============================================================================
#ifndef TestSkFormatWeb_hpp
#define TestSkFormatWeb_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

#include <SkFormatRoot.hpp>

#include <SkLemonInterface.hpp>

class TestSkFormatWeb : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkFormatWeb);
    CPPUNIT_TEST(TestSkWebBorder);
    CPPUNIT_TEST(TestSkWebPressure);
    CPPUNIT_TEST(TestSkWebDemo);
    CPPUNIT_TEST(TestSkWebBudget);
    CPPUNIT_TEST(TestSkWebClass);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    SkFormat::tFormatRoot* m_FormatRoot;
    SkSpreadSheet::tApi* m_Api;
    
    tVirtualClass* m_FormatApi; // Conflict with tSpreadSheet
    
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;
    
    void UndoOperation();
    void Debug();
    
    void Fill();
    void DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    
    void Pressure(tInt sDynamicNbRow, tInt sDynamicNbCol);
public:
    TestSkFormatWeb();
    
    void TestSkWebBorder();
    void TestSkWebPressure();
    void TestSkWebDemo();
    void TestSkWebBudget();
    void TestSkWebClass();
    
    void setUp();
    void tearDown();
};

#endif
