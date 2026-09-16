//==============================================================================
// TestSkExcel
//==============================================================================
#ifndef TestSkExcel_hpp
#define TestSkExcel_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkFormatCss.hpp>
#include <SkFormatCssApi.hpp>
#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkFormat;
using namespace SkSpreadSheet;

class TestSkExcel : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkExcel);
    CPPUNIT_TEST(TestSkInterfaceBudget);
    CPPUNIT_TEST(TestSkFillFormat);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
    tApi* m_Api;
    SkFormat::tFormatRoot* m_FormatRoot;
    
    
    tVirtualClass* m_FormatApi; // Conflict with tSpreadSheet
public:
    TestSkExcel();
private:
    tString DebugCell(tString sRef);
    
    void TestSkInterfaceBudget();
    void TestSkFillFormat();
public:
    void setUp();
    void tearDown();
};

#endif
