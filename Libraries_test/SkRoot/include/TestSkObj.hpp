//==============================================================================
// TestSkObj
// le 20/12/2025
//==============================================================================

#ifndef TestSkObj_hpp
#define TestSkObj_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkApplication.hpp>
#include <SkGenericClass.hpp>
#include <SkTableModel.hpp>
#include <SkFile.hpp>

using namespace SkRoot;

class TestSkObj : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkObj);
    CPPUNIT_TEST(TestParse);
    CPPUNIT_TEST(TestPath);
    CPPUNIT_TEST(TestCall);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application;
public:
    TestSkObj();
private:
    void TestParse();
    void TestPath();
    void TestCall();
public:
    void setUp();
    void tearDown();
};

#endif
