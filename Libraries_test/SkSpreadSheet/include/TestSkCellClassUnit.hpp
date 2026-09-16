//=============================================================================
// TestSkCellClassUnit.hpp
//
// CppUnit fixture for SkSpreadSheet::tCellClassUnit (constructors, clone,
// arithmetic with tClassUnit, JSON round-trip, UnitCurrencyModel via tClassUnitContainer,
// and tCell storing tCellClassUnit via tApi).
//
// WASM: model JSON is embedded in the test TU (see UnitCurrencyModelEmbedded.hxx).
//
// Link SkTestCellClassUnit.cpp with CppUnit, SkApplication, SkApi, SkRoot (SkClassUnitContainer).
//=============================================================================
#ifndef TestSkCellClassUnit_hpp
#define TestSkCellClassUnit_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

/// CppUnit fixture: derives from CPPUNIT_NS::TestFixture (CPPUNIT_TEST_SUITE registers tests).
class TestSkCellClassUnit : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkCellClassUnit);
    CPPUNIT_TEST(TestClassName);
    CPPUNIT_TEST(TestClonePreservesKind);
    CPPUNIT_TEST(TestAddSameMassNoErrorFlagInStr);
    CPPUNIT_TEST(TestAddDifferentFamiliesSetsErrorInStr);
    CPPUNIT_TEST(TestAddSameCurrency);
    CPPUNIT_TEST(TestOperatorMinusSameKg);
    CPPUNIT_TEST(TestOperatorMultiplyKgByScalar);
    CPPUNIT_TEST(TestOperatorMultiplyMeterTimesMeter);
    CPPUNIT_TEST(TestOperatorMultiply12mBy13m);
    CPPUNIT_TEST(TestOperatorMultiplyCmTimesCmPreservesCmSquared);
    CPPUNIT_TEST(TestOperatorMultiplyMeterIntOperands156);
    CPPUNIT_TEST(TestOperatorMultiplyMeterNullScalarsMarksError);
    CPPUNIT_TEST(TestSiAddMeterAndMillimeter);
    CPPUNIT_TEST(TestSiMultiplyMeterTimesKilometer);
    CPPUNIT_TEST(TestSiDivideMeterByMillimeter);
    CPPUNIT_TEST(TestSiCompoundAddVelocityMpsAndKmh);
    CPPUNIT_TEST(TestSiCompoundMultiplyVelocityByTime);
    CPPUNIT_TEST(TestOperatorDivideSameEurReturnsScalarDouble);
    CPPUNIT_TEST(TestOperatorDivideKgByKgReturnsScalarDouble);
    CPPUNIT_TEST(TestJsonRoundtripDoubleAndMass);
    CPPUNIT_TEST(TestClassUnitJsonRoundtripPower);
    CPPUNIT_TEST(TestSheetWriteReadJsonPreservesKgCell);
    CPPUNIT_TEST(TestSheetWriteReadJsonPreservesMeterSquaredFormula);
    CPPUNIT_TEST(TestSheetFileWriteReadJsonPreservesKgCell);
    CPPUNIT_TEST(TestSheetFileWriteReadJsonPreservesMeterSquaredFormula);
    CPPUNIT_TEST(TestLoadUnitCurrencyModelJson);
    CPPUNIT_TEST(TestSheetCellStoresKgUnit);
    CPPUNIT_TEST(TestSheetCellStoresEurMoney);
    CPPUNIT_TEST(TestSheetCellRangeReceivesSameUnitClass);
    CPPUNIT_TEST(TestSheetFormulaAddSameEur);
    CPPUNIT_TEST(TestSheetFormulaSubtractSameKg);
    CPPUNIT_TEST(TestSheetFormulaMultiplyMeterSquared);
    CPPUNIT_TEST(TestSheetFormulaMultiply12mBy13m);
    CPPUNIT_TEST(TestSheetFormulaDivideSameEurRatio);
    CPPUNIT_TEST_SUITE_END();

public:
    TestSkCellClassUnit();

    void setUp();
    void tearDown();

private:
    tApplication* m_Application;
    tApi* m_Api;

    /// Writes a scalar with unit into the sheet (same pattern as TestSkCellClass::SetValueMoney).
    void UndoCellClassUnitAtRef(const tString& sRef, tDouble sValue, const tClassUnit& sUnit);

    /// Debug string for one cell: ref, optional formula, typed value (matches TestSkCellClass::Str).
    tString Str(const tString& sRef);

    void TestClassName();
    void TestClonePreservesKind();
    void TestAddSameMassNoErrorFlagInStr();
    void TestAddDifferentFamiliesSetsErrorInStr();
    void TestAddSameCurrency();
    void TestOperatorMinusSameKg();
    void TestOperatorMultiplyKgByScalar();
    void TestOperatorMultiplyMeterTimesMeter();
    void TestOperatorMultiply12mBy13m();
    void TestOperatorMultiplyCmTimesCmPreservesCmSquared();
    void TestOperatorMultiplyMeterIntOperands156();
    void TestOperatorMultiplyMeterNullScalarsMarksError();
    void TestSiAddMeterAndMillimeter();
    void TestSiMultiplyMeterTimesKilometer();
    void TestSiDivideMeterByMillimeter();
    void TestSiCompoundAddVelocityMpsAndKmh();
    void TestSiCompoundMultiplyVelocityByTime();
    void TestOperatorDivideSameEurReturnsScalarDouble();
    void TestOperatorDivideKgByKgReturnsScalarDouble();
    void TestJsonRoundtripDoubleAndMass();
    void TestClassUnitJsonRoundtripPower();
    void TestSheetWriteReadJsonPreservesKgCell();
    void TestSheetWriteReadJsonPreservesMeterSquaredFormula();
    void TestSheetFileWriteReadJsonPreservesKgCell();
    void TestSheetFileWriteReadJsonPreservesMeterSquaredFormula();
    void TestLoadUnitCurrencyModelJson();

    void TestSheetCellStoresKgUnit();
    void TestSheetCellStoresEurMoney();
    void TestSheetCellRangeReceivesSameUnitClass();

    void TestSheetFormulaAddSameEur();
    void TestSheetFormulaSubtractSameKg();
    void TestSheetFormulaMultiplyMeterSquared();
    void TestSheetFormulaMultiply12mBy13m();
    void TestSheetFormulaDivideSameEurRatio();
};

namespace SkTestSpreadSheet {

    /// Runs only the TestSkCellClassUnit suite via CppUnit TextUi runner (single-suite runner).
    int RunSkCellClassUnitTests();

} // namespace SkTestSpreadSheet

#endif // TestSkCellClassUnit_hpp
