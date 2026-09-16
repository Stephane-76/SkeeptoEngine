//==============================================================================
// TestSkFillSeries
// Tests for SkFillSeries (auto-fill / fill handle series engine).
//==============================================================================
#ifndef TestSkFillSeries_hpp
#define TestSkFillSeries_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>

using namespace SkRoot;

class TestSkFillSeries : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkFillSeries);
    CPPUNIT_TEST(TestExtendNumbers);
    CPPUNIT_TEST(TestExtendMonthsFr);
    CPPUNIT_TEST(TestExtendDatesFrDayStep);
    CPPUNIT_TEST(TestExtendDatesFrMonthStep);
    CPPUNIT_TEST(TestExtendDatesUsDayStep);
    CPPUNIT_TEST(TestExtendPrefixNumber);
    CPPUNIT_TEST(TestExtendCyclicFallback);
    CPPUNIT_TEST(TestShiftFormula);
    CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application;
    tString m_SavedLang;

public:
    TestSkFillSeries();

private:
    void TestExtendNumbers();
    void TestExtendMonthsFr();
    void TestExtendDatesFrDayStep();
    void TestExtendDatesFrMonthStep();
    void TestExtendDatesUsDayStep();
    void TestExtendPrefixNumber();
    void TestExtendCyclicFallback();
    void TestShiftFormula();

public:
    void setUp();
    void tearDown();
};

#endif /* TestSkFillSeries_hpp */
