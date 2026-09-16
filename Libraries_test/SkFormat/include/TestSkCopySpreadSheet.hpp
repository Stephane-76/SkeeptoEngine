//=============================================================================
// TestSkCopySpreadSheet
//=============================================================================
#ifndef TestSkCopySpreadSheet_hpp
#define TestSkCopySpreadSheet_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

#include <SkFormatRoot.hpp>

#include <SkLemonInterface.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

class TestSkCopySpreadSheet : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkCopySpreadSheet);
    CPPUNIT_TEST(TestCopy);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
    SkFormat::tFormatRoot* m_FormatRoot;
    tApi* m_Api;
    tVirtualClass* m_FormatApi;
public:
    TestSkCopySpreadSheet();
private:
    void TestCopy();
public:
    void setUp();
    void tearDown();
};

#endif    /* TestSkCopySpreadSheet_hpp */
