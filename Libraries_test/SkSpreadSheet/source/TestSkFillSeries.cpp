//==============================================================================
// TestSkFillSeries
//==============================================================================
#include "../include/TestSkFillSeries.hpp"

#include <SkFillSeries.hpp>

#include <string>
#include <vector>

using namespace SkSpreadSheet;

namespace {

void AssertExtendEqual(const tVectorString& sSeeds, tSize sCount,
                       const std::vector<tString>& sExpected, const tString& sMsg) {
    tVectorString wOut = tFillSeries::Extend(sSeeds, sCount);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(sMsg + " size", sExpected.size(), wOut.size());
    for (tSize i = 0; i < sExpected.size(); ++i) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
            sMsg + " [" + std::to_string(i) + "]",
            sExpected[i],
            wOut[i]);
    }
}

} // namespace

TestSkFillSeries::TestSkFillSeries()
    : CPPUNIT_NS::TestFixture(), m_Application(nullptr) {}

void TestSkFillSeries::setUp() {
    m_Application = tApplication::Instance();
    m_SavedLang = m_Application->Locale()->Lang();
    m_Application->Locale("fr");
}

void TestSkFillSeries::tearDown() {
    if (m_Application != nullptr && !m_SavedLang.empty()) {
        m_Application->Locale(m_SavedLang);
    }
}

void TestSkFillSeries::TestExtendNumbers() {
    const tString wTitle = "TestSkFillSeries ExtendNumbers";
    tVectorString wSeeds = {"1", "2"};
    AssertExtendEqual(wSeeds, 3, {"3", "4", "5"}, wTitle);

    wSeeds = {"1,5", "2,5"};
    AssertExtendEqual(wSeeds, 2, {"3,5", "4,5"}, wTitle + " comma");
}

void TestSkFillSeries::TestExtendMonthsFr() {
    const tString wTitle = "TestSkFillSeries ExtendMonthsFr";
    tVectorString wSeeds = {"Janvier", "Février"};
    AssertExtendEqual(wSeeds, 2, {"Mars", "Avril"}, wTitle);
}

void TestSkFillSeries::TestExtendDatesFrDayStep() {
    const tString wTitle = "TestSkFillSeries ExtendDatesFrDayStep";
    tVectorString wSeeds = {"01/01/2024", "02/01/2024"};
    AssertExtendEqual(wSeeds, 3,
                      {"03/01/2024", "04/01/2024", "05/01/2024"},
                      wTitle);
}

void TestSkFillSeries::TestExtendDatesFrMonthStep() {
    const tString wTitle = "TestSkFillSeries ExtendDatesFrMonthStep";
    tVectorString wSeeds = {"01/01/2024", "01/02/2024"};
    AssertExtendEqual(wSeeds, 2,
                      {"01/03/2024", "01/04/2024"},
                      wTitle);
}

void TestSkFillSeries::TestExtendDatesUsDayStep() {
    const tString wTitle = "TestSkFillSeries ExtendDatesUsDayStep";
    m_Application->Locale("us");
    tVectorString wSeeds = {"01-01-2024", "01-02-2024"};
    AssertExtendEqual(wSeeds, 2,
                      {"01-03-2024", "01-04-2024"},
                      wTitle);
    m_Application->Locale("fr");
}

void TestSkFillSeries::TestExtendPrefixNumber() {
    const tString wTitle = "TestSkFillSeries ExtendPrefixNumber";
    tVectorString wSeeds = {"Item1", "Item2"};
    AssertExtendEqual(wSeeds, 2, {"Item3", "Item4"}, wTitle);

    wSeeds = {"A01", "A02"};
    AssertExtendEqual(wSeeds, 1, {"A03"}, wTitle + " padded");
}

void TestSkFillSeries::TestExtendCyclicFallback() {
    const tString wTitle = "TestSkFillSeries ExtendCyclicFallback";
    tVectorString wSeeds = {"foo", "bar"};
    AssertExtendEqual(wSeeds, 3, {"foo", "bar", "foo"}, wTitle);
}

void TestSkFillSeries::TestShiftFormula() {
    const tString wTitle = "TestSkFillSeries ShiftFormula";

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        wTitle + " row",
        tString("=A3+B2"),
        tFillSeries::ShiftFormula("=A2+B1", 1, 0));

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        wTitle + " col",
        tString("=B2+C1"),
        tFillSeries::ShiftFormula("=A2+B1", 0, 1));

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        wTitle + " absolute",
        tString("=$A$2+B2"),
        tFillSeries::ShiftFormula("=$A$2+B1", 1, 0));

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        wTitle + " function name",
        tString("=SUM(A2:A10)"),
        tFillSeries::ShiftFormula("=SUM(A1:A9)", 1, 0));
}
