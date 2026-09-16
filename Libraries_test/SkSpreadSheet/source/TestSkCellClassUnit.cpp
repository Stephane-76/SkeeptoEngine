//=============================================================================
// SkTestCellClassUnit — CppUnit tests for tCellClassUnit
//=============================================================================
#include "../include/TestSkCellClassUnit.hpp"
#include "../../../Libraries/SkSpreadSheet/include/SkCellClassUnit.hpp"
#include "../../../Libraries/SkRoot/include/SkClassUnitContainer.hpp"

#include <SkUnit.hpp>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#ifndef __EMSCRIPTEN__
#include <filesystem>
#include <fstream>
#include <iterator>
#endif
#ifdef __EMSCRIPTEN__
#include "UnitCurrencyModelEmbedded.hxx"
#endif

using namespace SkRoot;
using namespace SkSpreadSheet;

#ifndef __EMSCRIPTEN__
namespace {

/// Persists Api::WriteJson output to disk, then reloads via ReadJson (same bytes as an on-disk workbook .json).
tBool ReadWorkbookJsonFromFile(tApi* sApi, const tString& sJson, const tString& sPath) {
    const std::filesystem::path wPath(sPath.c_str());
    std::filesystem::create_directories(wPath.parent_path());
    {
        std::ofstream wOut(wPath, std::ios::binary);
        if (!wOut.good()) {
            return false;
        }
        wOut << sJson;
    }
    std::string wBuf;
    {
        std::ifstream wIn(wPath, std::ios::binary);
        if (!wIn.good()) {
            return false;
        }
        wBuf.assign(std::istreambuf_iterator<char>(wIn), std::istreambuf_iterator<char>());
    }
    return sApi->ReadJson(tString(wBuf));
}

} // namespace
#endif

TestSkCellClassUnit::TestSkCellClassUnit() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr) {}

void TestSkCellClassUnit::setUp() {
#ifndef __EMSCRIPTEN__
    std::filesystem::remove_all("./Spreadsheet");
#endif
    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkCellClassUnit::tearDown() {
    delete m_Api;
    m_Api = nullptr;
    m_Application = nullptr;
}

void TestSkCellClassUnit::UndoCellClassUnitAtRef(const tString& sRef, tDouble sValue, const tClassUnit& sUnit) {
    tVariant wValue = sValue;
    tCellClass wDummyCellClass(wValue);
    (void)wDummyCellClass;
    tCellClassUnit wCellUnit(wValue, sUnit);
    tVariant wWrapped(&wCellUnit);
    m_Api->UndoCellClass(sRef, &wWrapped);
}

tString TestSkCellClassUnit::Str(const tString& sRef) {
    tStringStream wStream;
    tCell* wCell = m_Api->Cell(sRef);
    wStream << sRef;
    if (wCell != nullptr) {
        if (wCell->FormulaStr() != "") {
            wStream << ":(" << wCell->FormulaStr() << ")";
        }

        if (wCell->Value().IsClass()) {
            tCellClassUnit* wCellClassUnit = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
            if (wCellClassUnit != nullptr) {
                wStream << "=" << wCellClassUnit->Str();
            } else {
#ifdef _DEBUG
                wStream << wCell->Value().Class()->Debug();
#endif
            }
        } else {
            wStream << "=" << wCell->Value();
        }
    }
    return wStream.str();
}

void TestSkCellClassUnit::TestSheetCellStoresKgUnit() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("A1", 3.5, tClassUnit(t_UnitMass::kg));

    tCell* wCell = m_Api->Cell("A1");
    CPPUNIT_ASSERT(wCell != nullptr);
    CPPUNIT_ASSERT(wCell->Value().IsClass());

    auto* wUnitCell = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
    CPPUNIT_ASSERT(wUnitCell != nullptr);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.5, wUnitCell->Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wUnitCell->Str().find("kg") != tString::npos);
}

void TestSkCellClassUnit::TestSheetCellStoresEurMoney() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("B2", 42.0, tClassUnit(t_UnitMoney::eur));

    tCell* wCell = m_Api->Cell("B2");
    CPPUNIT_ASSERT(wCell != nullptr);
    CPPUNIT_ASSERT(wCell->Value().IsClass());

    auto* wUnitCell = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
    CPPUNIT_ASSERT(wUnitCell != nullptr);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.0, wUnitCell->Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(!wUnitCell->Str().empty());
}

void TestSkCellClassUnit::TestSheetCellRangeReceivesSameUnitClass() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("C3:C4", 10.0, tClassUnit(t_UnitMass::g));

    const tString wAddrs[] = { "C3", "C4" };
    for (const auto& wAddr : wAddrs) {
        tCell* wCell = m_Api->Cell(wAddr);
        CPPUNIT_ASSERT(wCell != nullptr);
        CPPUNIT_ASSERT(wCell->Value().IsClass());
        auto* wUnitCell = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
        CPPUNIT_ASSERT(wUnitCell != nullptr);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, wUnitCell->Value()->Double(), 1e-9);
    }
}

void TestSkCellClassUnit::TestClassName() {
    tCellClassUnit wCell;
    CPPUNIT_ASSERT(wCell.ClassName() == "tCellUnit");
}

void TestSkCellClassUnit::TestClonePreservesKind() {
    tCellClassUnit wOrig(tVariant(7.5), tClassUnit(t_UnitMass::kg));
    tVirtualClass* wClone = wOrig.Clone();
    CPPUNIT_ASSERT(wClone != nullptr);
    auto* wUnit = dynamic_cast<tCellClassUnit*>(wClone);
    CPPUNIT_ASSERT(wUnit != nullptr);
    CPPUNIT_ASSERT(wUnit->ClassName() == "tCellUnit");
    CPPUNIT_ASSERT(wUnit->Value()->Type() == tVariantType::t_double);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.5, wUnit->Value()->Double(), 1e-9);
    delete wClone;
}

#ifndef __EMSCRIPTEN__
/// Path to Libraries_test/SkSpreadSheet/data/UnitCurrencyModel.json (same directory layout as this .cpp).
static tString SkUnitCurrencyModelJsonPath() {
    namespace fs = std::filesystem;
    fs::path wHere(__FILE__);
    wHere = wHere.parent_path() / ".." / "data" / "UnitCurrencyModel.json";
    return wHere.lexically_normal().string();
}
#endif

static tVariant VariantOwningCloneOfHeapCell(tCellClassUnit* sHeapCell) {
    // tVariant(VirtualClass*) stores sHeapCell->Clone(); it does not take ownership of *sHeapCell*
    // (see SkVariant.cpp). Caller must delete the transient heap instance after construction.
    tVariant wVariant(sHeapCell);
    delete sHeapCell;
    return wVariant;
}

void TestSkCellClassUnit::TestAddSameMassNoErrorFlagInStr() {
    tCellClassUnit wLeft(tVariant(2.0), tClassUnit(t_UnitMass::kg));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(3.0), tClassUnit(t_UnitMass::kg)));
    wLeft.Operator_plus(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, wLeft.Value()->Double(), 1e-9);
    tString wStr = wLeft.Str();
    CPPUNIT_ASSERT(wStr.find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestAddDifferentFamiliesSetsErrorInStr() {
    tCellClassUnit wLeft(tVariant(1.0), tClassUnit(t_UnitMass::kg));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(2.0), tClassUnit(t_UnitLength::meter)));
    wLeft.Operator_plus(true, wVR);
    tString wStr = wLeft.Str();
    CPPUNIT_ASSERT(wStr.find("Error") != tString::npos);
}

void TestSkCellClassUnit::TestAddSameCurrency() {
    tCellClassUnit wLeft(tVariant(10.0), tClassUnit(t_UnitMoney::eur));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(20.0), tClassUnit(t_UnitMoney::eur)));
    wLeft.Operator_plus(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestOperatorMinusSameKg() {
    tCellClassUnit wLeft(tVariant(5.0), tClassUnit(t_UnitMass::kg));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(2.0), tClassUnit(t_UnitMass::kg)));
    wLeft.Operator_minus(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiplyKgByScalar() {
    tCellClassUnit wLeft(tVariant(2.5), tClassUnit(t_UnitMass::kg));
    tVariant wScalar(3.0);
    wLeft.Operator_multiply(true, wScalar);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.5, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiplyMeterTimesMeter() {
    tCellClassUnit wLeft(tVariant(3.0), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(4.0), tClassUnit(t_UnitLength::meter)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    CPPUNIT_ASSERT(wLeft.Str().find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiply12mBy13m() {
    // Common case: two length operands, same unit (meter) — result is area (m^2).
    tCellClassUnit wLeft(tVariant(12.0), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(13.0), tClassUnit(t_UnitLength::meter)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(156.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    CPPUNIT_ASSERT(wLeft.Str().find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiplyCmTimesCmPreservesCmSquared() {
    // Same display unit on both operands: preserve it with combined power (cm²), do not canonically rewrite to m².
    tCellClassUnit wLeft(tVariant(1.0), tClassUnit(t_UnitLength::centimeter));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(2.0), tClassUnit(t_UnitLength::centimeter)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    // Do not use find("m2")==npos: "cm2" contains the substring "m2". Match the spaced unit token from Str().
    CPPUNIT_ASSERT_MESSAGE(wLeft.Str().c_str(), wLeft.Str().find(" cm2") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wLeft.Str().c_str(), wLeft.Str().find(" m2") == tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiplyMeterIntOperands156() {
    // Sheet cells often hold t_int; must not call tVariant::Double() on non-double (WASM abort).
    tCellClassUnit wLeft(tVariant(static_cast<tInt>(12)), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(
        tVariant(static_cast<tInt>(13)), tClassUnit(t_UnitLength::meter)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(156.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    CPPUNIT_ASSERT(wLeft.Str().find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestOperatorMultiplyMeterNullScalarsMarksError() {
    // ApplyUnit uses null placeholder until numeric merge; product must not throw.
    tVariant wNull;
    tCellClassUnit wLeft(wNull, tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(wNull, tClassUnit(t_UnitLength::meter)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") != tString::npos);
}

void TestSkCellClassUnit::TestSiAddMeterAndMillimeter() {
    tCellClassUnit wLeft(tVariant(1.0), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(
        new tCellClassUnit(tVariant(1000.0), tClassUnit(t_UnitLength::millimeter)));
    wLeft.Operator_plus(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestSiMultiplyMeterTimesKilometer() {
    tCellClassUnit wLeft(tVariant(2.0), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(
        new tCellClassUnit(tVariant(3.0), tClassUnit(t_UnitLength::kilometer)));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6000.0, wLeft.Value()->Double(), 1e-6);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    CPPUNIT_ASSERT(wLeft.Str().find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestSiDivideMeterByMillimeter() {
    tCellClassUnit wLeft(tVariant(5.0), tClassUnit(t_UnitLength::meter));
    tVariant wVR = VariantOwningCloneOfHeapCell(
        new tCellClassUnit(tVariant(1000.0), tClassUnit(t_UnitLength::millimeter)));
    tVariant wOut = wLeft.Operator_divide(true, wVR);
    CPPUNIT_ASSERT(!wOut.IsClass());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, wOut.Double(), 1e-9);
}

void TestSkCellClassUnit::TestSiCompoundAddVelocityMpsAndKmh() {
    tCellClassUnit wLeft(
        tVariant(0.0),
        tClassUnit(t_UnitLength::meter),
        tClassUnit(t_UnitTime::second));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(
        tVariant(3.6),
        tClassUnit(t_UnitLength::kilometer),
        tClassUnit(t_UnitTime::hour)));
    wLeft.Operator_plus(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestSiCompoundMultiplyVelocityByTime() {
    tCellClassUnit wLeft(
        tVariant(10.0),
        tClassUnit(t_UnitLength::meter),
        tClassUnit(t_UnitTime::second));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(
        tVariant(2.0),
        tClassUnit(t_UnitTime::second),
        tClassUnit()));
    wLeft.Operator_multiply(true, wVR);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(20.0, wLeft.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wLeft.Str().find("Error") == tString::npos);
    CPPUNIT_ASSERT(wLeft.Str().find("m") != tString::npos);
}

void TestSkCellClassUnit::TestOperatorDivideSameEurReturnsScalarDouble() {
    tCellClassUnit wLeft(tVariant(12.0), tClassUnit(t_UnitMoney::eur));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(4.0), tClassUnit(t_UnitMoney::eur)));
    tVariant wOut = wLeft.Operator_divide(true, wVR);
    CPPUNIT_ASSERT(!wOut.IsClass());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, wOut.Double(), 1e-9);
}

void TestSkCellClassUnit::TestOperatorDivideKgByKgReturnsScalarDouble() {
    tCellClassUnit wLeft(tVariant(6.0), tClassUnit(t_UnitMass::kg));
    tVariant wVR = VariantOwningCloneOfHeapCell(new tCellClassUnit(tVariant(2.0), tClassUnit(t_UnitMass::kg)));
    tVariant wOut = wLeft.Operator_divide(true, wVR);
    CPPUNIT_ASSERT(!wOut.IsClass());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, wOut.Double(), 1e-9);
}

void TestSkCellClassUnit::TestJsonRoundtripDoubleAndMass() {
    tCellClassUnit wCell(tVariant(4.5), tClassUnit(t_UnitMass::kg));
    rapidjson::StringBuffer wBuf;
    rapidjson::Writer<rapidjson::StringBuffer> wWriter(wBuf);
    wCell.Json(&wWriter);

    rapidjson::Document wDoc;
    wDoc.Parse(wBuf.GetString());
    CPPUNIT_ASSERT(!wDoc.HasParseError());
    CPPUNIT_ASSERT(wDoc.IsObject());

    tCellClassUnit wOut;
    wOut.Json(wDoc);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.5, wOut.Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wOut.Str().find("kg") != tString::npos);
}

void TestSkCellClassUnit::TestClassUnitJsonRoundtripPower() {
    tClassUnit wLen(t_UnitLength::meter);
    wLen.Power(2);
    rapidjson::StringBuffer wBuf;
    rapidjson::Writer<rapidjson::StringBuffer> wWriter(wBuf);
    wLen.Json(&wWriter);

    rapidjson::Document wDoc;
    wDoc.Parse(wBuf.GetString());
    CPPUNIT_ASSERT(!wDoc.HasParseError());
    CPPUNIT_ASSERT(wDoc.HasMember("pw"));
    CPPUNIT_ASSERT(wDoc["pw"].GetInt() == 2);

    tClassUnit wOut;
    wOut.Json(wDoc);
    CPPUNIT_ASSERT(wOut.Power() == 2);
    CPPUNIT_ASSERT(wOut.GetSymbol() == "m");
    CPPUNIT_ASSERT(wOut.Str().find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestSheetWriteReadJsonPreservesKgCell() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("J1", 42.5, tClassUnit(t_UnitMass::kg));
    const tString wUri = "wwww.skeema.fr/w1";
    const tString wJson = m_Api->WriteJson(wUri);
    //cout << wJson << endl;
    CPPUNIT_ASSERT_MESSAGE("WriteJson empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE("ReadJson failed", m_Api->ReadJson(wJson));

    tCell* wCell = m_Api->Cell("J1");
    CPPUNIT_ASSERT(wCell != nullptr);
    CPPUNIT_ASSERT(wCell->Value().IsClass());
    auto* wUnit = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
    CPPUNIT_ASSERT(wUnit != nullptr);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, wUnit->Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wUnit->Str().find("kg") != tString::npos);
}

void TestSkCellClassUnit::TestSheetWriteReadJsonPreservesMeterSquaredFormula() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("K1", 30.0, tClassUnit(t_UnitLength::meter));
    m_Api->UndoCellValue("K2", "=K1*K1");
    const tString wBefore = Str("K2");
    CPPUNIT_ASSERT_MESSAGE(wBefore.c_str(), wBefore.find("900") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wBefore.c_str(), wBefore.find("m2") != tString::npos);

    const tString wUri = "wwww.skeema.fr/w1";
    const tString wJson = m_Api->WriteJson(wUri);
    CPPUNIT_ASSERT_MESSAGE("WriteJson empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE("ReadJson failed", m_Api->ReadJson(wJson));

    const tString wAfter = Str("K2");
    CPPUNIT_ASSERT_MESSAGE(wAfter.c_str(), wAfter.find("900") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wAfter.c_str(), wAfter.find("m2") != tString::npos);
}

void TestSkCellClassUnit::TestSheetFileWriteReadJsonPreservesKgCell() {
#ifndef __EMSCRIPTEN__
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("J1", 42.5, tClassUnit(t_UnitMass::kg));
    const tString wUri = "wwww.skeema.fr/w1";
    const tString wJson = m_Api->WriteJson(wUri);
    CPPUNIT_ASSERT_MESSAGE("WriteJson empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE(
        "ReadJson from workbook .json file failed",
        ReadWorkbookJsonFromFile(m_Api, wJson, "./Spreadsheet/wb_roundtrip_kg.json"));

    tCell* wCell = m_Api->Cell("J1");
    CPPUNIT_ASSERT(wCell != nullptr);
    CPPUNIT_ASSERT(wCell->Value().IsClass());
    auto* wUnit = dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
    CPPUNIT_ASSERT(wUnit != nullptr);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, wUnit->Value()->Double(), 1e-9);
    CPPUNIT_ASSERT(wUnit->Str().find("kg") != tString::npos);
#endif
}

void TestSkCellClassUnit::TestSheetFileWriteReadJsonPreservesMeterSquaredFormula() {
#ifndef __EMSCRIPTEN__
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("K1", 30.0, tClassUnit(t_UnitLength::meter));
    m_Api->UndoCellValue("K2", "=K1*K1");
    const tString wBefore = Str("K2");
    CPPUNIT_ASSERT_MESSAGE(wBefore.c_str(), wBefore.find("900") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wBefore.c_str(), wBefore.find("m2") != tString::npos);

    const tString wUri = "wwww.skeema.fr/w1";
    const tString wJson = m_Api->WriteJson(wUri);
    CPPUNIT_ASSERT_MESSAGE("WriteJson empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE(
        "ReadJson from workbook .json file failed",
        ReadWorkbookJsonFromFile(m_Api, wJson, "./Spreadsheet/wb_roundtrip_m2.json"));

    const tString wAfter = Str("K2");
    CPPUNIT_ASSERT_MESSAGE(wAfter.c_str(), wAfter.find("900") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wAfter.c_str(), wAfter.find("m2") != tString::npos);
#endif
}

void TestSkCellClassUnit::TestLoadUnitCurrencyModelJson() {
    tClassUnitContainer wModel;
    tBool wOk = false;
#ifndef __EMSCRIPTEN__
    const tString wPath = SkUnitCurrencyModelJsonPath();
    CPPUNIT_ASSERT_MESSAGE(wPath.c_str(), std::filesystem::exists(wPath));
    wOk = wModel.LoadFromFile(wPath);
#else
    // WASM has no host filesystem path to Libraries_test/data; parse embedded UTF-8 JSON instead.
    wOk = wModel.ParseJsonString(tString(SkUnitCurrencyModelEmbeddedJsonUtf8()));
#endif
    CPPUNIT_ASSERT_MESSAGE(wModel.LastError().c_str(), wOk);
    CPPUNIT_ASSERT(wModel.SchemaVersion() == "1.1");
    CPPUNIT_ASSERT(wModel.DimensionBasis().find("L") != wModel.DimensionBasis().end());

    const tClassUnitModelPhysicalUnit* wM = wModel.FindPhysicalUnit("m");
    CPPUNIT_ASSERT(wM != nullptr);
    CPPUNIT_ASSERT(wM->m_Id == "m");
    CPPUNIT_ASSERT(wM->m_Dimensions.find("L") != wM->m_Dimensions.end());

    const tClassUnitModelPhysicalUnit* wKm = wModel.FindPhysicalUnit("km");
    CPPUNIT_ASSERT(wKm != nullptr);
    CPPUNIT_ASSERT(wKm->m_SiEquivalent.m_HasValue);
    CPPUNIT_ASSERT(wKm->m_SiEquivalent.m_UnitId == "m");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1000.0, wKm->m_SiEquivalent.m_Factor, 1e-9);

    const tClassUnitModelCurrency* wEur = wModel.FindCurrency("EUR");
    CPPUNIT_ASSERT(wEur != nullptr);
    CPPUNIT_ASSERT(wEur->m_CurrencyDimension.m_Code == "EUR");
    CPPUNIT_ASSERT(wModel.MoneyRules().m_AddSubtractRequireSameCurrencyDimension);

    CPPUNIT_ASSERT(!wModel.ExplicitIncompatibilities().empty());
    CPPUNIT_ASSERT(wModel.HasCompatibilityHints());
}

void TestSkCellClassUnit::TestSheetFormulaAddSameEur() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("E1", 6.0, tClassUnit(t_UnitMoney::eur));
    UndoCellClassUnitAtRef("E2", 8.0, tClassUnit(t_UnitMoney::eur));
    m_Api->UndoCellValue("E3", "=E1+E2");
    CPPUNIT_ASSERT_MESSAGE(Str("E3").c_str(), Str("E3") == "E3:(E1+E2)=14.00 €");
}

void TestSkCellClassUnit::TestSheetFormulaSubtractSameKg() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("F1", 5.0, tClassUnit(t_UnitMass::kg));
    UndoCellClassUnitAtRef("F2", 2.0, tClassUnit(t_UnitMass::kg));
    m_Api->UndoCellValue("F3", "=F1-F2");
    CPPUNIT_ASSERT_MESSAGE(Str("F3").c_str(), Str("F3") == "F3:(F1-F2)=3.00 kg");
}

void TestSkCellClassUnit::TestSheetFormulaMultiplyMeterSquared() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("G1", 30.0, tClassUnit(t_UnitLength::meter));
    m_Api->UndoCellValue("G2", "=G1*G1");
    CPPUNIT_ASSERT_MESSAGE(Str("G2").c_str(), Str("G2") == "G2:(G1*G1)=900.00 m2");
}

void TestSkCellClassUnit::TestSheetFormulaMultiply12mBy13m() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("I1", 12.0, tClassUnit(t_UnitLength::meter));
    UndoCellClassUnitAtRef("I2", 13.0, tClassUnit(t_UnitLength::meter));
    m_Api->UndoCellValue("I3", "=I1*I2");
    const tString wS3 = Str("I3");
    CPPUNIT_ASSERT_MESSAGE(wS3.c_str(), wS3.find("156") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wS3.c_str(), wS3.find("m2") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wS3.c_str(), wS3.find("Error") == tString::npos);
}

void TestSkCellClassUnit::TestSheetFormulaDivideSameEurRatio() {
    CPPUNIT_ASSERT(m_Api != nullptr);
    UndoCellClassUnitAtRef("H1", 12.0, tClassUnit(t_UnitMoney::eur));
    UndoCellClassUnitAtRef("H2", 4.0, tClassUnit(t_UnitMoney::eur));
    m_Api->UndoCellValue("H3", "=H1/H2");
    CPPUNIT_ASSERT_MESSAGE(Str("H3").c_str(), Str("H3") == "H3:(H1/H2)=3.00");
}

namespace SkTestSpreadSheet {

int RunSkCellClassUnitTests() {
    CppUnit::TestResult wController;
    CppUnit::TestResultCollector wResult;
    wController.addListener(&wResult);
    CPPUNIT_NS::BriefTestProgressListener wProgress;
    wController.addListener(&wProgress);

    CppUnit::TestRunner wRunner;
    wRunner.addTest(TestSkCellClassUnit::suite());
    wRunner.run(wController, "");

    return wResult.wasSuccessful() ? 0 : 1;
}

} // namespace SkTestSpreadSheet
