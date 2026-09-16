//==================================================================================================
//  File      : TestSkWasm.hpp
//  Date      : 2026/02/03
//  Author    : brwill
//==================================================================================================
#ifndef TestSkWasm_hpp
#define TestSkWasm_hpp
#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <cppunit/extensions/HelperMacros.h>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkWasm : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkWasm);
    CPPUNIT_TEST(TestJsonWasm);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
    tApi* m_Api;
public:
    TestSkWasm();
    void TestJsonWasm();
    void setUp();
    void tearDown();
};

#endif /* TestSkWasm_hpp */