//==============================================================================
// TestSkFormatExcel
//==============================================================================
#ifndef TestSkFormatExcel_hpp
#define TestSkFormatExcel_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkFormatNumber.hpp>
#include <SkFormatDate.hpp>


using namespace SkRoot;

class TestSkFormatExcel : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkFormatExcel);
    CPPUNIT_TEST(TestSkFormatExcelNumber);
    CPPUNIT_TEST(TestSkFormatExcelDate);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
public:
    TestSkFormatExcel();
private:
    void TestSkFormatExcelNumber();
    void TestSkFormatExcelDate();
public:
    void setUp();
    void tearDown();
};

#endif  /* TestSkFormatExcel */
