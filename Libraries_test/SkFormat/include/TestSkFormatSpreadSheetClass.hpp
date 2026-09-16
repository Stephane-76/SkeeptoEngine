//==============================================================================
// TestSkFormatSpreadSheetClass Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#ifndef TestSkFormatSpreadSheetClass_hpp
#define TestSkFormatSpreadSheetClass_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

#include <SkFormatRoot.hpp>

#include <SkLemonInterface.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

class TestSkFormatSpreadSheetClass : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkFormatSpreadSheetClass);
    CPPUNIT_TEST(TestSkClass);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    SkFormat::tFormatRoot* m_FormatRoot;
    SkSpreadSheet::tApi* m_Api;
    
    tVirtualClass* m_FormatApi; // Conflict with tSpreadSheet
    
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;
    
    void Fill();
    void DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    
  
public:
    TestSkFormatSpreadSheetClass();
    
    void SetValueMoney(tString sRef,t_UnitMoney sUnitMoney);
    void SetValueMoney(tString sRef,t_UnitMoney sUnitMoney,tDouble sValue);
    void SetValueLength(tString sRef,t_UnitLength sUnitLength,tDouble sValue);
    
    void DebugUnit(tString sRef);
    
    void TestSkClass();
    
public:
    void setUp();
    void tearDown();
};


#endif    /* TestSkFormatSpreadSheetClass */

