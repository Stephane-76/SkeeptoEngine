//==============================================================================
// TestSkJsonFormatString
//==============================================================================
#ifndef TestSkJsonFormatString_hpp
#define TestSkJsonFormatString_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkJsonFormatString : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkJsonFormatString);
    CPPUNIT_TEST(TestJsonFormatStringStructure);
    CPPUNIT_TEST(TestJsonFormatStringContent);
    CPPUNIT_TEST(TestJsonFormatStringFamilies);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
    tApi* m_Api;
public:
    TestSkJsonFormatString();
private:
    void TestJsonFormatStringStructure();
    void TestJsonFormatStringContent();
    void TestJsonFormatStringFamilies();
public:
    void setUp() override;
    void tearDown() override;
};

#endif