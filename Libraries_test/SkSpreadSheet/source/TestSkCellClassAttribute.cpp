//=============================================================================
// TestSkCellClass
// le 04/10/2023 
//==============================================================================
#include "../include/TestSkCellClassAttribute.hpp"
// for test
#include <SkFloatingObject.hpp>
#include <SkRangeRefTransform.hpp>
#define _printdebug


class tTestClassAttribute : public tCellClassAttribute {
	private:
		tString m_Name;
		tInt     m_ValueInt;
		tInt     m_Temperature;
		tInt     m_Humidity;
		tVariant m_Variant;
		
		tInt	 m_Instance;
		static tInt m_Cpt;
public:
        tTestClassAttribute() : tCellClassAttribute(), m_Name(),m_ValueInt(0),m_Instance(m_Cpt++) {}
        tTestClassAttribute(tTestClassAttribute& sTestClass) : tCellClassAttribute(sTestClass) {
			m_Name = sTestClass.m_Name;
			m_ValueInt = sTestClass.m_ValueInt;
			m_Temperature = sTestClass.m_Temperature;
			m_Humidity = sTestClass.m_Humidity;

			m_Instance = m_Cpt++;
		}
		virtual ~tTestClassAttribute() {
#ifdef printdebug
			cout << "Destroy :" << m_Instance << endl;
#endif
			--m_Cpt;
		}

		/// @brief      return Name of class (for factory)
		/// @return     tString
		virtual tString ClassName() const override {
			return("tTestClass");
		}


		void Name(tString sValue) { m_Name = sValue; }
		tString Name() { return(m_Name); }

		void ValueInt(tInt sValue) { m_ValueInt = sValue; }
		tInt ValueInt() { return(m_ValueInt); }

		void Temperature(tInt sValue) { m_Temperature = sValue; }
		tInt Temperature() { return(m_Temperature); }

		void Humidity(tInt sValue) { m_Humidity = sValue; }
		tInt Humidity() { return(m_Humidity); }

        tCellClassAttribute* Clone() override {
			m_Instance=m_Cpt++;
#ifdef printdebug
			cout << "Clone :" << m_Instance << endl;
#endif
			return(new tTestClassAttribute(*this));
		}
};

tInt tTestClassAttribute::m_Cpt = 0;

tTestClassAttribute* CreateTest() {
	return(new tTestClassAttribute());
}


// We can send it to the API of a feature 
TestSkCellClassAttribute::TestSkCellClassAttribute() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}


void TestSkCellClassAttribute::DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // Drop
    cout << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tVariant wVariant = m_Api->CellValue(wRow, wCol);
            tString wFormula = m_Api->Formula(wRow, wCol);
            cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
        }
        cout << endl;
    }
}

void TestSkCellClassAttribute::CreateClass() {
	tString wTitle = "CreateInstance";
	tString wSubTitle = "Create Instance Class";

	// Register Model Class ===================================================
	tCellModelClassAttribute* wCellModelClassAttribute = new tCellModelClassAttribute("tTestClass","Test classe","Test", &CreateTest);
	tClassFactory::Instance()->Register(wCellModelClassAttribute);

	tTestClassAttribute* wTestClass = dynamic_cast<tTestClassAttribute*>(tClassFactory::Instance()->Create("tTestClass"));
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestClass != nullptr);

	wSubTitle = "GetAssociated Model";
	tModelClass* wCellModelClass = tClassFactory::Instance()->Get("tTestClass");
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wCellModelClass != nullptr);

	tSetterGetter wSetterGetter;
	wSetterGetter.Set<tTestClassAttribute>(&tTestClassAttribute::Name, &tTestClassAttribute::Name);
	wCellModelClass->AddProperty("Name", tVariantType::t_string, "Name", 0, "","n", wSetterGetter);

	wSetterGetter.Set<tTestClassAttribute>(&tTestClassAttribute::ValueInt, &tTestClassAttribute::ValueInt);
    wCellModelClass->AddProperty("Int", tVariantType::t_int, "Int", 1, "", "i",wSetterGetter);

	wSetterGetter.Set<tTestClassAttribute>(&tTestClassAttribute::Temperature, &tTestClassAttribute::Temperature);
	wCellModelClass->AddProperty("Temperature", tVariantType::t_int, "Temperature", 1, "","t", wSetterGetter);

	wSetterGetter.Set<tTestClassAttribute>(&tTestClassAttribute::Humidity, &tTestClassAttribute::Humidity);
	wCellModelClass->AddProperty("Humidity", tVariantType::t_int, "Humidity", 1, "", "h",wSetterGetter);
	delete(wTestClass);
}


void TestSkCellClassAttribute::TestCreateInstanceClassAttribute() {
	// Register Model Class ===================================================
	CreateClass();

	tString wTitle = "TestCreateInstance";
	tString wSubTitle = "Create Instance Class";

	m_Api->UndoCellClass("A1", "tTestClass");

	tCell* wCell = m_Api->Cell(1, 1);

	tVariant wVariantClass= wCell->Value();;
	tTestClassAttribute* wTestClassA1 = dynamic_cast<tTestClassAttribute*>(wVariantClass.Class());
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestClassA1->ClassName() == "tTestClass");
}


void TestSkCellClassAttribute::TestUndoMoveAttributeFormulaRefs() {
	CreateClass();
	m_Api->UndoCellClass("A10:A12", "tTestClass");
	m_Api->UndoCellAttribute("A10", "Int", tVariant(10));
	m_Api->UndoCellAttribute("A11", "Int", tVariant(11));
	m_Api->UndoCellAttribute("A12", "Int", tVariant(12));

	m_Api->UndoCellClass("C10", "tTestClass");
	m_Api->UndoCellAttribute("C10", "Int", tVariant("=SUM(A10:A12)"));

	CPPUNIT_ASSERT(m_Api->UndoMove("A10:A12", "A11:A13"));

	tCellAttribute* wAttribute = m_Api->CellAttribute("C10", "Int");
	CPPUNIT_ASSERT(wAttribute != nullptr);
	const tString wFormulaAfterMove = wAttribute->FormulaStr();
	CPPUNIT_ASSERT_MESSAGE("TestUndoMoveAttributeFormulaRefs move range start",
		wFormulaAfterMove.find("A11") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("TestUndoMoveAttributeFormulaRefs move range end",
		wFormulaAfterMove.find("A13") != tString::npos);

	CPPUNIT_ASSERT(m_Api->Undo());

	wAttribute = m_Api->CellAttribute("C10", "Int");
	CPPUNIT_ASSERT(wAttribute != nullptr);
	const tString wFormulaAfterUndo = wAttribute->FormulaStr();
	CPPUNIT_ASSERT_MESSAGE("TestUndoMoveAttributeFormulaRefs undo range start",
		wFormulaAfterUndo.find("A10") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("TestUndoMoveAttributeFormulaRefs undo range end",
		wFormulaAfterUndo.find("A12") != tString::npos);
}

void TestSkCellClassAttribute::TestCellAttributeJsonFormula() {
	// ComboBox comboOptions path: =JSON(range) on a string class attribute.
	CreateClass();
	m_Api->UndoCellValue("B2", "Alice");
	m_Api->UndoCellValue("B3", "Bob");
	m_Api->UndoCellValue("B4", "Carol");

	m_Api->UndoCellClass("C10", "tTestClass");
	CPPUNIT_ASSERT_MESSAGE(
		"TestCellAttributeJsonFormula set =JSON(B2:B4)",
		m_Api->UndoCellAttribute("C10", "Name", tVariant("=JSON(B2:B4)")));

	tCellAttribute* wAttr = m_Api->CellAttribute("C10", "Name");
	CPPUNIT_ASSERT(wAttr != nullptr);
	CPPUNIT_ASSERT_MESSAGE(
		"TestCellAttributeJsonFormula formula stored",
		wAttr->FormulaStr().find("JSON") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE(
		"TestCellAttributeJsonFormula evaluates to JSON array",
		wAttr->Value().IsString());
	CPPUNIT_ASSERT_EQUAL(
		tString("[\"Alice\",\"Bob\",\"Carol\"]"),
		wAttr->Value().String());

	// Same path after sheet qualification (panel Apply encode).
	const tString wQualified = QualifyRefsForSheet("Sheet1", "=JSON(B2:B4)");
	CPPUNIT_ASSERT_EQUAL(tString("=JSON(Sheet1!B2:B4)"), wQualified);
	CPPUNIT_ASSERT_MESSAGE(
		"TestCellAttributeJsonFormula set sheet-qualified JSON",
		m_Api->UndoCellAttribute("C10", "Name", tVariant(wQualified)));
	wAttr = m_Api->CellAttribute("C10", "Name");
	CPPUNIT_ASSERT(wAttr != nullptr);
	CPPUNIT_ASSERT_EQUAL(
		tString("[\"Alice\",\"Bob\",\"Carol\"]"),
		wAttr->Value().String());
}

void TestSkCellClassAttribute::TestClassAttributeUndoRedo() {
	// Instantiate Error Ref
	tClassError wClassErrorRef(tTypeError::t_ref, "");
	tVariant wVariantRef(wClassErrorRef);

	// Register Model Class ===================================================
	CreateClass();
	// Create Class 
	m_Api->UndoCellClass("A1:A3", "tTestClass");
    
    //cout << m_Api->Cell("A3")->Value() << endl;
	// Override
	//m_Api->UndoCellClass("A2", "tTestClass");
	//m_Api->UndoCellClass("A3", "tTestClass");

	m_Api->UndoCellValue("B2", tVariant(123));

	m_Api->UndoCellValue("C1", tVariant("=A3.Int+1"));
	m_Api->UndoCellValue("C2", tVariant("=A3.Name"));
	m_Api->UndoCellAttribute("A1:A3", "Int", 12);
	m_Api->UndoCellAttribute("A1:A3", "Name", "Ok");

	tCellAttribute* wCellAttribute = m_Api->CellAttribute("A3", "Int");
	tCell* wCellC1 = m_Api->Cell(1, 3);
	tCell* wCellC2 = m_Api->Cell(2, 3);
    
    //cout << endl << wCellC1->FormulaStr() << endl;
    //cout << endl << wCellC2->FormulaStr() << endl;
    
    // Overload and undo
    m_Api->UndoCellClass("A1", "tTestClass");
    // Verify Name
    tCell* wCellA1=m_Api->Cell("A1");
    tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wCellA1->Class());
    //cout << endl <<  wCellClassAttribute->ClassName() << ":" << wCellClassAttribute->RefName() << endl;
    tCellAttribute* wCellAttributeInt=wCellClassAttribute->Find("Int");
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo tTestClass A1 twice Do Undo" , wCellAttributeInt==nullptr);
    m_Api->Undo();
    
    wCellA1=m_Api->Cell("A1");
    wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wCellA1->Class());
    //cout << wCellClassAttribute->ClassName() << ":" << wCellClassAttribute->RefName() << endl;
    wCellAttributeInt=wCellClassAttribute->Find("Int");
    //cout << wCellAttributeInt->Value().Int() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo tTestClass A1 twice Do Undo" , wCellAttributeInt->Value().Int()==12);
    
    m_Api->UndoCellClass("J1", "tTestClass");
    
    m_Api->UndoCellValue("A8", "=SUM(A1:A3)");
    
#ifdef printdebug
    cout << wCellC1->FormulaStr() << endl;
	cout << wCellC1->StrRef() << "=" << wCellC1->FormulaStr() << "=" << wCellC1->Value() << endl;
	cout << wCellC2->StrRef() << "=" << wCellC2->FormulaStr() << "=" << wCellC2->Value() << endl;
#endif

	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1" , wCellC1->FormulaStr() == "tTestClass2.Int+1");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2" , wCellC2->FormulaStr() == "tTestClass2.Name");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1", wCellC1->Value().Int() == 13);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2", wCellC2->Value().String() =="Ok");

	m_Api->UndoRaz("A3");
#ifdef printdebug
	cout << wCellC1->StrRef() << "=" << wCellC1->FormulaStr() << "=" << wCellC1->Value() << endl;
	cout << wCellC2->StrRef() << "=" << wCellC2->FormulaStr() << "=" << wCellC2->Value() << endl;
#endif
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1", wCellC1->FormulaStr() == "#REF!.Int+1");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2", wCellC2->FormulaStr() == "#REF!.Name");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1", wCellC1->Value() == wVariantRef);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2", wCellC2->Value() == wVariantRef);

	m_Api->Undo();
#ifdef printdebug
    cout << wCellC1->StrRef() << "=" << wCellC1->FormulaStr() << "=" << wCellC1->Value() << endl;
    cout << wCellC2->StrRef() << "=" << wCellC2->FormulaStr() << "=" << wCellC2->Value() << endl;
    cout << wCellC1->StrRef() << "=" << wCellC1->Value() << "=" << wCellC1->Value() << endl;
    cout << wCellC2->StrRef() << "=" << wCellC2->Value() << "=" << wCellC2->Value() << endl;
#endif
    
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1", wCellC1->FormulaStr() == "tTestClass2.Int+1");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2", wCellC2->FormulaStr() == "tTestClass2.Name");

	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C1", wCellC1->Value().Int() == 13);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 1 C2", wCellC2->Value().String() == "Ok");

#ifdef printdebug
	cout << wCellC1->StrRef() << "=" << wCellC1->FormulaStr() << endl;
	cout << wCellC2->StrRef() << "=" << wCellC2->FormulaStr() << endl;
#endif
	//=========================================================================
	m_Api->UndoCellAttribute("A1", "Int", "=B2+1");
	m_Api->UndoCellAttribute("A1", "Temperature", "=A1.Int");

#ifdef printdebug
	tCell* wCellA1 = m_Api->Cell("A1");
	wCellA1->Debug();
#endif

	tCellAttribute* wA1_Int = m_Api->CellAttribute("A1", "Int");
	tCellAttribute* wA1_Temperature = m_Api->CellAttribute("A1", "Temperature");

#ifdef printdebug
	cout << wA1_Int->StrRef() << ":" << wA1_Int->FormulaStr(true) << "=" << wA1_Int->Value() << endl;
	cout << wA1_Temperature->StrRef() << ":" << wA1_Temperature->FormulaStr(true) << "=" << wA1_Temperature->Value() << endl;
#endif
    
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 "+ wA1_Int->StrRef(), wA1_Int->FormulaStr() == "B2+1");
    tString wFormulaTest=wA1_Temperature->FormulaStr();
        CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->FormulaStr() == "tTestClass.Int");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Int->StrRef(), wA1_Int->Value().Int() == 124);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->Value().Int() == 124);
#ifdef checksp
	m_Api->Check();
#endif

	m_Api->UndoRaz("A2:A3");

#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();

	wA1_Int = m_Api->CellAttribute("A1", "Int");
	wA1_Temperature = m_Api->CellAttribute("A1", "Temperature");

	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Int->StrRef(), wA1_Int->FormulaStr() == "B2+1");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->FormulaStr() == "tTestClass.Int");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Int->StrRef(), wA1_Int->Value().Int() == 124);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->Value().Int() == 124);

#ifdef checksp
	m_Api->Check();
#endif
#ifdef printdebug
	wCellAttribute = m_Api->CellAttribute("A1", "Int");
	cout << "A1.int=" << wCellAttribute->Value() << " Formula " << wCellAttribute->FormulaStr() << endl;

	wCellAttribute = m_Api->CellAttribute("A1", "Name");
	cout << "A1.Name=" << wCellAttribute->Value() << " Formula " << wCellAttribute->FormulaStr() << endl;

	cout << "wCellC1=" << wCellC1->Value() << " Formula " << wCellC1->FormulaStr() << endl;
	cout << "wCellC2=" << wCellC2->Value() << " Formula " << wCellC2->FormulaStr() << endl;
#endif
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
#ifdef checksp
	m_Api->Check();
#endif

#ifdef printdebug
	cout << "wCellC1=" << wCellC1->Value() << " Formula " << wCellC1->FormulaStr() << endl;
	cout << wCellAttribute << ":" << wCellAttribute->StrRef() << wCellAttribute->Value() << endl;
#endif

	wCellAttribute = m_Api->CellAttribute("A1", "Int");

	tString wFormulaAttribute = "A3.Int+1";
	m_Api->UndoCellAttribute("A1", "Int", "="+ wFormulaAttribute);
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo A1.Int=" + wFormulaAttribute, wCellAttribute->FormulaStr() == "tTestClass2.Int+1");
	CPPUNIT_ASSERT_MESSAGE("TestUndoRedo A1.Int=13", wCellAttribute->Value().Int() == 13);

#ifdef checksp
	m_Api->Check();
#endif


	wCellAttribute = m_Api->CellAttribute("A2", "Int");;
#ifdef printdebug
	cout << endl;
	cout << "wCellAttribute(A2.Int->Value()=" << wCellAttribute->Value() << " Formula " << wCellAttribute->FormulaStr() << endl;
#endif

	
#ifdef checksp
	m_Api->Check();
#endif
	wCellAttribute = m_Api->CellAttribute("A1", "Int");

	m_Api->Undo();
	wCellAttribute = m_Api->CellAttribute("A1", "Int");
#ifdef printdebug
	cout << endl;
	cout << "wCellAttribute(A1.Int->Value()=" << wCellAttribute->Value() << " Formula " << wCellAttribute->FormulaStr() << endl;
#endif	

	m_Api->Undo();

	m_Api->Undo();

	// Test Lemon
	tBool wResult = m_Api->CellValue("A1", 8);
	wResult = m_Api->CellValue("A2", 8);
	wResult = m_Api->CellValue("A3", 2);
	wResult = m_Api->CellValue("A4", 3);
	CPPUNIT_ASSERT_MESSAGE("Test Lemon ", wResult);

	tString wFormula = "A1+(A3/A2+A4)";
	wResult=m_Api->CellValue("A10","="+wFormula);
	tCell* wCell = m_Api->Cell(10, 1);
	//cout << wCell->FormulaStr() << "=" << wCell->Value() << endl;
	//cout << wCell->FormulaStr() << endl;
	CPPUNIT_ASSERT_MESSAGE("Test Path  "+wFormula, wCell->FormulaStr() == wFormula);
}

void TestSkCellClassAttribute::RegisterPieChartClass() {
    // Mirrors wasm SkUISpreadSheet Call dispatch: RegisterClassAttribute + AddProperty
    // (SkCellClassPieChart.js registerClassAttribute).
    CPPUNIT_ASSERT_MESSAGE("RegisterClassAttribute SkCellClassPieChart",
        m_Api->RegisterClassAttribute("SkCellClassPieChart", "PieChart", "Javascript"));
    CPPUNIT_ASSERT_MESSAGE("AddProperty Title",
        m_Api->AddProperty("Title", "string", "Title", 0, ""));
    CPPUNIT_ASSERT_MESSAGE("AddProperty chartData",
        m_Api->AddProperty("chartData", "string", "Labels range (or JSON, or A:B combined)", 1, "", "range"));
    CPPUNIT_ASSERT_MESSAGE("AddProperty DataRange",
        m_Api->AddProperty("DataRange", "string", "Values range (or A:B combined)", 2, "", "range"));
    tString wJson = tClassFactory::Instance()->Json("SkCellClassPieChart");
    CPPUNIT_ASSERT_MESSAGE("JsonCellClass chartData kind",
        wJson.find("\"k\":\"range\"") != tString::npos || wJson.find("\"k\": \"range\"") != tString::npos);
}

void TestSkCellClassAttribute::TestWorkBookModelsInJsonRoundTrip() {
    const tString wTitle = "TestWorkBookModelsInJsonRoundTrip";
    RegisterPieChartClass();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoCellClass",
        m_Api->UndoCellClass("A1", "SkCellClassPieChart"));

    tString wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " WriteJson", !wSave.empty());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " models section",
        wSave.find("\"models\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " embedded class name",
        wSave.find("SkCellClassPieChart") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " embedded range kind",
        wSave.find("\"k\":\"range\"") != tString::npos || wSave.find("\"k\": \"range\"") != tString::npos);

    tClassFactory::Instance()->Clear();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " factory cleared",
        tClassFactory::Instance()->Get("SkCellClassPieChart") == nullptr);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson",
        m_Api->ReadJson(wSave));

    CPPUNIT_ASSERT_MESSAGE(wTitle + " model restored from sker",
        tClassFactory::Instance()->Get("SkCellClassPieChart") != nullptr);

    tCell* wCell = m_Api->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 cell", wCell != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 class attribute",
        wCell->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 class name",
        wCell->ClassAttribute()->ClassName() == "SkCellClassPieChart");
}

void TestSkCellClassAttribute::TestFloatingObjectLoadWithoutHostSheetInSker() {
    // Regression: Graphics.sker with floatingobjects but empty _$$A.cells (no models section).
    // Embedded JSON (WASM tests have no host filesystem for /Users/.../Graphics.sker).
    const tString wTitle = "TestFloatingObjectLoadWithoutHostSheetInSker";
    static const char kBrokenGraphicsSkerJson[] =
        R"json({"id":2,"uri":"/home/Graphics.sker","info":{"author":"test","email":"test@test.fr"},"sizecol":16.933,"sizerow":5.291,"sheets":[{"name":"Sheet1","rows":[{"i":4,"s":-1},{"i":5,"s":-1}],"cols":[{"i":2,"s":25.402},{"i":3,"s":-1}],"cells":[{"c":"B5","si":0,"fo":0},{"c":"C5","t":"i","v":10,"fo":0}],"merged":[],"printParameters":{}},{"name":"_$$A","rows":[{"i":1,"s":-1}],"cols":[{"i":1,"s":-1}],"cells":[],"merged":[],"printParameters":{}}],"namedranges":[],"floatingobjects":[{"n":"PieChart","c":"SkCellClassPieChart","t":"Sheet1","r":1,"anchorCell":"Sheet1!J8","diffX":-211,"diffY":-90,"width":420,"height":280,"opacity":1,"zIndex":1}],"si":["Stephane","Jean"],"f":{"formats":[{"f":"border-left:solid 1px black;"}]}})json";
    const tString wJson(kBrokenGraphicsSkerJson);

    tearDown();
    setUp();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson", m_Api->ReadJson(wJson));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " stub model registered",
        tClassFactory::Instance()->Get("SkCellClassPieChart") != nullptr);

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObject* wObject = wWorkBook->FloatingObjectContainer()->ByName("PieChart");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " registry", wObject != nullptr);

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host cell materialized", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host on _$$A",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host class after lazy ensure",
        wHost->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host class name",
        wHost->ClassAttribute()->ClassName() == "SkCellClassPieChart");

    tString wSave = m_Api->WriteJson(wWorkBook->Uri());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " WriteJson", !wSave.empty());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " _$$A host persisted",
        wSave.find("\"name\":\"_$$A\"") != tString::npos
            && wSave.find("SkCellClassPieChart") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " models section on save",
        wSave.find("\"models\"") != tString::npos);
}

void TestSkCellClassAttribute::TestUnknownCellClassAutoStubOnLoad() {
    const tString wTitle = "TestUnknownCellClassAutoStubOnLoad";
    const tString wUnknownClass = "SkCellClassUserWidget999";
    static const char kJson[] =
        R"json({"uri":"/home/unknown-class.sker","sizecol":10,"sizerow":5,"sheets":[{"name":"Sheet1","rows":[],"cols":[],"cells":[{"c":"A1","t":"c","v":{"n":"SkCellClassUserWidget999"},"class":"SkCellClassUserWidget999","cl":{"t":"n","v":null,"n":"Widget1","a":[{"n":"Title","t":"s","v":"Hello","e":0}]}}],"merged":[],"printParameters":{}}],"namedranges":[],"floatingobjects":[],"si":[],"f":{"formats":[]}})json";

    tearDown();
    setUp();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " factory cleared",
        tClassFactory::Instance()->Get(wUnknownClass) == nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson",
        m_Api->ReadJson(kJson));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " stub model registered",
        tClassFactory::Instance()->Get(wUnknownClass) != nullptr);

    tCell* wCell = m_Api->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 cell", wCell != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 class attribute",
        wCell->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " A1 class name",
        wCell->ClassAttribute()->ClassName() == wUnknownClass);
    tCellAttribute* wTitleAttr = wCell->ClassAttribute()->CellAttribute("Title");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Title attribute", wTitleAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Title value",
        wTitleAttr->Value().Str() == "Hello");
}

void TestSkCellClassAttribute::RegisterImageClass() {
    CPPUNIT_ASSERT_MESSAGE("RegisterClassAttribute SkCellClassImage",
        m_Api->RegisterClassAttribute("SkCellClassImage", "Image", "Javascript"));
    CPPUNIT_ASSERT_MESSAGE("AddProperty dataUrl",
        m_Api->AddProperty("dataUrl", "string", "Image data URL", 0, ""));
    CPPUNIT_ASSERT_MESSAGE("AddProperty altText",
        m_Api->AddProperty("altText", "string", "Alt text", 1, ""));
    CPPUNIT_ASSERT_MESSAGE("AddProperty opacity",
        m_Api->AddProperty("opacity", "string", "Opacity (0-1)", 2, "1"));
    CPPUNIT_ASSERT_MESSAGE("AddProperty rotation",
        m_Api->AddProperty("rotation", "string", "Rotation (degrees)", 3, "0"));
}

void TestSkCellClassAttribute::TestFloatingObjectImageDataUrl() {
    RegisterImageClass();
    const tString wTitle = "TestFloatingObjectImageDataUrl";
    const tString wDataUrl =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject(
            "ImgTest", "SkCellClassImage", "Sheet1", nullptr,
            0.0, 0.0, 120.0, 80.0, 1.0, "Sheet1!B3"));

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();
    tFloatingObject* wObject = wContainer->ByName("ImgTest");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " class name", wObject->ClassName() == "SkCellClassImage");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl attribute",
        m_Api->UndoFloatingObjectAttribute("ImgTest", "dataUrl", wDataUrl));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " altText attribute",
        m_Api->UndoFloatingObjectAttribute("ImgTest", "altText", "Test PNG"));

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);
    tCellAttribute* wDataUrlAttr = wHost->CellAttribute("dataUrl");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl CellAttribute", wDataUrlAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl value", wDataUrlAttr->Value().Str() == wDataUrl);

    tString wJson = m_Api->WriteJson(wWorkBook->Uri());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " WriteJson", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " class in json",
        wJson.find("SkCellClassImage") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl in json",
        wJson.find("data:image/png;base64,") != tString::npos);

    tearDown();
    setUp();
    RegisterImageClass();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson", m_Api->ReadJson(wJson));
    wWorkBook = m_Api->ActiveWorkBook();
    wContainer = wWorkBook->FloatingObjectContainer();
    wObject = wContainer->ByName("ImgTest");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " registry after load", wObject != nullptr);
    wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host after load", wHost != nullptr);
    wDataUrlAttr = wHost->CellAttribute("dataUrl");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl after load", wDataUrlAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " dataUrl value after load",
        wDataUrlAttr->Value().Str() == wDataUrl);
}

void TestSkCellClassAttribute::CreateGenericClass() {
    m_Api->ActiveSheet("Sheet1");
    m_Api->RegisterClassAttribute("SkCellButton","Button","Test");
    
    tBool wOk=m_Api->AddProperty("name", "string", "Name", 0, "Coucou Stéphane");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty name", wOk == true);
 
    wOk= m_Api->AddProperty("now","date","now",1,"31/12/2024");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty now", wOk == true);
 
    wOk= m_Api->AddProperty("now","string","now in string ",1,"31/12/2024");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty now twice", wOk == false);
}

void TestSkCellClassAttribute::TestClassAttribute() {
    CreateGenericClass();
#ifdef printdebug
    tString wJson=m_Api->GetWorkBooksList();
    cout << wJson << endl;
#endif
    m_Api->UndoInsertRangeNamed("ALLEZY", "A1:A3");
    m_Api->UndoCellClass("W3", "SkCellButton");
#ifdef printdebug
    tString wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif

    
    tString wSave=m_Api->WriteJson("wwww.skeema.fr/w1");
    tJsonObject wObj;
    if (!wObj.Parse(wSave)) {
        CPPUNIT_ASSERT_MESSAGE("Test WriteJson", false);
    }
    tBool wLoad=m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("Test ReadJson", wLoad == true);
    tCell* wCell=m_Api->Cell("W3");
    tCellClassAttribute* wCellClassAttribute=wCell->ClassAttribute();
    CPPUNIT_ASSERT_MESSAGE("Test CellClassAttribute", wCellClassAttribute!=nullptr);
    tString wName=wCellClassAttribute->ClassName();
    CPPUNIT_ASSERT_MESSAGE("Test CellClassAttribute Name", wName == "SkCellButton");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    //delete(m_Api);
    //m_Api = new tApi();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
#ifdef printdebug
    wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif

   m_Api->NewWorkBook("wwww.skeema.fr/w2");
   //CreateGenericClass();
    
    wLoad=m_Api->ReadJson(wSave);
    
#ifdef printdebug
    wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif
    
    wCell=m_Api->Cell("W3");
    wCellClassAttribute=wCell->ClassAttribute();
    CPPUNIT_ASSERT_MESSAGE("Undo CellClass", wCellClassAttribute!=nullptr);
    
    
#ifdef printdebug
    wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif
    
    m_Api->UndoCellValue("W2","W...");
    m_Api->UndoCellAttribute("W3", "name", "=$W$2+'Coucou Stéphane'");
    tVariant wResult=m_Api->CellAttributeValue("W3", "name");
    CPPUNIT_ASSERT_MESSAGE("Test W3.name", wResult.Str() == "W...Coucou Stéphane");
    
    //Copy Paste ===============================================================
    m_Api->Copy("W3");
    
    m_Api->UndoCellValue("W2","Ahhhh ..");
    m_Api->UndoPaste("A3:J3");
#ifdef printdebug
    tCellClassContainer* wCellClassContainer=m_Api->WorkBook()->ActiveSheet()->ColRowCellRange()->CellClassContainer();
    wCellClassContainer->Debug();
#endif
#ifdef checksp
    m_Api->Check();
#endif
    
    (void)m_Api->CellAttribute("A3", "name");
  
#ifdef printdebug
    wCellClassContainer->Debug();
#endif
    /*
    cout << "Formula -->" << wCellAttribute->FormulaStr() << endl;
    cout << wCellAttribute->Debug() << endl;
    */
    wResult=m_Api->CellAttributeValue("A3", "name");
 
#ifdef printdebug
    wCellClassContainer->Debug();
#endif
    CPPUNIT_ASSERT_MESSAGE("Test W3.name after paste", wResult.Str() == "Ahhhh ..Coucou Stéphane");
    m_Api->Undo();

#ifdef printdebug
    wCellClassContainer->Debug();
#endif

    wResult=m_Api->CellAttributeValue("A3", "name");
    CPPUNIT_ASSERT_MESSAGE("Test W3.name after Undo", wResult.Str() == "");
    
    m_Api->Redo();
    
    wResult=m_Api->CellAttributeValue("A3", "name");
    CPPUNIT_ASSERT_MESSAGE("Test W3.name after redo", wResult.Str() == "Ahhhh ..Coucou Stéphane");
    
    m_Api->UndoCellValue("A4", "=A3.name");
    
    wCell=m_Api->Cell("A4");
    
    //cout << wCell->Debug() << endl;
#ifdef printdebug
    wCellClassContainer->Debug();
#endif
    //cout << "wCell Formula " << wCell->FormulaStr() << endl;

    
    CPPUNIT_ASSERT_MESSAGE("Test  A4 SkCellButton.name", wCell->FormulaStr()  == "SkCellButton1.name");
    
#ifdef printdebug
    wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif
    m_Api->UndoInsertRangeNamed("ALLEZ", "A3:A3");
    
#ifdef printdebug
    wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug << endl;
#endif
    
#ifdef printdebug
    wCellClassContainer=m_Api->WorkBook()->ActiveSheet()->ColRowCellRange()->CellClassContainer();
    wCellClassContainer->Debug();
#endif
    wResult=m_Api->UndoCellValue("A5","=ALLEZ.name");
    CPPUNIT_ASSERT_MESSAGE("Test A5 ALLEZ.name ", wResult==false);


    wResult=m_Api->CellValue("ALLEZ");
    
    m_Api->UndoDeleteRow(2, 2);
    //cout << tApplication::Instance()->DebugUndo() << endl;
    m_Api->Undo();
    
    wCell=m_Api->Cell("A4");
   //cout << wCell->FormulaStr() << endl;
    CPPUNIT_ASSERT_MESSAGE("Test  A4 SkCellButton.name", wCell->FormulaStr()  == "SkCellButton1.name");
}

void TestSkCellClassAttribute::TestClassAttributeMultiSheet() {
    CreateGenericClass();
    tSheet* wSheet1 = m_Api->ActiveSheet();
    
    m_Api->UndoCellClass("A3", "SkCellButton");
    
    m_Api->UndoCellAttribute("A3", "name", "=W2+'Coucou Stéphane'");
    tSheet* wSheet2=m_Api->AddSheet("Sheet2");
 
    m_Api->UndoCellClass("A1", "SkCellButton");
    
    m_Api->UndoCellAttribute("A1", "name", "='Coucou Stéphane'+Sheet1!SkCellButton.name");
    //cout << m_Api->Cell("A1")->StrRef() << ":" << m_Api->Cell("A1")->FormulaStr()  << "=" << m_Api->Cell("A1")->Value() << endl;
    
    m_Api->UndoCellValue("A2", "=Sheet1!SkCellButton.name");
    m_Api->UndoCellValue("A4", "=Sheet1!A3.name");
    
    tCell* wCellA2=m_Api->Cell("A2");
   // cout << wCellA2->StrRef() << ":" << wCellA2->FormulaStr()  << "=" << wCellA2->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->FormulaStr()  == "Sheet1!SkCellButton.name");
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->Value().Str()=="Coucou Stéphane");
 
    tCell* wCellA4=m_Api->Cell("A4");
    //cout << wCellA4->StrRef() << ":" << wCellA4->FormulaStr()  << "=" << wCellA4->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("Test A4 Formula ", wCellA4->FormulaStr()  == "Sheet1!SkCellButton.name");

    // Delete Sheet2 ==========================================================
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoDeleteSheet(wSheet2->Name());
    
    m_Api->Undo();
    m_Api->ActiveSheet("Sheet2");
    
    wCellA2=m_Api->Cell("A2");
    
    //cout << wCellA2->StrRef() << ":" << wCellA2->FormulaStr()  << "=" << wCellA2->Value() << endl;
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->FormulaStr()  == "Sheet1!SkCellButton.name");
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->Value().Str()=="Coucou Stéphane");
  
    // Delete Sheet1 ==========================================================
    m_Api->UndoDeleteSheet(wSheet1->Name());
    
    m_Api->Undo();
    wCellA2=m_Api->Cell("A2");
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->FormulaStr()  == "Sheet1!SkCellButton.name");
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->Value().Str()=="Coucou Stéphane");
    
    m_Api->ActiveSheet("Sheet1");
    // Delete Col and Sheet 1
    m_Api->UndoDeleteCol(1,2);
    
    m_Api->Undo();
    
    m_Api->ActiveSheet("Sheet2");
    wCellA2=m_Api->Cell("A2");
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->FormulaStr()  == "Sheet1!SkCellButton.name");
    
    CPPUNIT_ASSERT_MESSAGE("Test A2 Formula ", wCellA2->Value().Str()=="Coucou Stéphane");
    
}
void TestSkCellClassAttribute::TestClassAttributeDeleteColRow() {
    CreateGenericClass();
#ifdef printdebug
    tString wJson=m_Api->GetWorkBooksList();
    cout << wJson << endl;
#endif
    m_Api->UndoInsertRangeNamed("ALLEZY", "A1:A3");
    m_Api->UndoCellClass("B3", "SkCellButton");
    
    m_Api->UndoCellAttribute("B3","name","Allez");
    m_Api->UndoCellAttribute("B3","firstname","Stéphane");
    m_Api->UndoCellValue("C4", "=B3.name+\"...\"");
    
    m_Api->UndoCellValue("C4", "=B3.name+\"...\"");
 
    tCell* wCellC4=m_Api->Cell("C4");
    //cout << endl << "C4:" << wCellC4->FormulaStr() << "=" << wCellC4->Value() << endl;
 
    m_Api->UndoCellValue("A3","=SUM(B3:B4)");
    
    tString wResult=m_Api->JsonView(3, 2, tUnitMetrics::pixels, 300, 300,0,0, true);
    //cout << wResult << endl;
    
    tObj wObj;
    wObj.Parse(wResult);
    for(auto wProperty : *wObj.VectorProperty()) {
        //cout << wProperty.Name() << "=" << wProperty.Variant() << endl;
    }
 
    
    //cout << endl << "A3:" << m_Api->Cell("A3")->FormulaStr() << "=" << m_Api->Cell("A3")->Value() << endl;
    
    
    CPPUNIT_ASSERT_MESSAGE("C4 Attribute ref  Value", wCellC4->Value().Str()=="Allez...");
    CPPUNIT_ASSERT_MESSAGE("Test Delete Col C4 Attribute ref  Formula", wCellC4->FormulaStr()=="SkCellButton.name+\"...\"");
    
    m_Api->UndoDeleteCol(1, 3);
#ifdef _DEBUG
    //cout << tApplication::Instance()->DebugUndo();
#endif
    //cout << endl << "B3:" << m_Api->Cell("B4")->FormulaStr() << "=" << m_Api->Cell("B4")->Value() << endl;
    
    m_Api->Undo();
    
    wCellC4=m_Api->Cell("C4");
    //cout << endl << "C4:" << wCellC4->FormulaStr() << "=" << wCellC4->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("Test Delete Col C4 Attribute ref  Value", wCellC4->Value().Str()=="Allez...");
    CPPUNIT_ASSERT_MESSAGE("Test Delete Col C4 Attribute ref  Formula", wCellC4->FormulaStr()=="SkCellButton.name+\"...\"");
    
    DrawCell(1, 1, 5, 5);
    m_Api->UndoDeleteRow(3, 1);
    DrawCell(1, 1, 5, 5);
    //cout << m_Api->ActiveSheet()->Debug();
    //cout << endl << "C3:" << m_Api->Cell("C3")->FormulaStr() << "=" << m_Api->Cell("C3")->Value() << endl;
    
    m_Api->Undo();
    
    wCellC4=m_Api->Cell("C4");
    //cout << endl << "C4:" << wCellC4->FormulaStr() << "=" << wCellC4->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("Test Delete Col C4 Attribute ref  Value", wCellC4->Value().Str()=="Allez...");
    CPPUNIT_ASSERT_MESSAGE("Test Delete Col C4 Attribute ref  Formula", wCellC4->FormulaStr()=="SkCellButton.name+\"...\"");
    
    m_Api->UndoCellAttribute("B3","now","31/12/2024");
    //cout << endl << m_Api->CellAttribute("B3", "now")->Value() << endl;
    
    m_Api->UndoCellValue("C5", "=B3.now+\"...\"");
    
    m_Api->UndoDeleteRow(3, 1);
    m_Api->UndoDeleteRow(3, 1);
    m_Api->UndoDeleteRow(3, 1);
    
    m_Api->Undo();
    m_Api->Undo();
#ifdef _DEBUG
    //cout << tApplication::Instance()->DebugUndo();
#endif
    m_Api->Undo();
    tRect wRect(3,3,7,10);
  
    
    
    m_Api->UndoDeleteRowByRect(wRect);
    
    m_Api->Undo();
    wCellC4=m_Api->Cell("C4");
    CPPUNIT_ASSERT_MESSAGE("Test DeleteByRect Col C4 Attribute ref  Value", wCellC4->Value().Str()=="Allez...");
    CPPUNIT_ASSERT_MESSAGE("Test DeleteByRect Col C4 Attribute ref  Formula", wCellC4->FormulaStr()=="SkCellButton.name+\"...\"");
    
    m_Api->UndoInsertRowByRect(wRect);
    m_Api->Undo();
}
void TestSkCellClassAttribute::TestClassAttributeMethod() {
    CreateGenericClass();
    m_Api->UndoCellClass("A1", "SkCellButton");
    
    //m_Api->UndoCellValue("A2","=SkCellButton.Method(A3)");
}

void TestSkCellClassAttribute::TestClassAttributeCopy() {
    tString wTitle="TestClassAttributeCopy";
    tString wSubTitle="UndoCellClass:";
    m_Api->RegisterClassAttribute("SkCellButton","Button","Test");
    m_Api->UndoCellClass("A1", "SkCellButton");
    m_Api->UndoCellClass("A2", "SkCellButton");
    m_Api->UndoCellClass("A3", "SkCellButton");
    m_Api->UndoRaz("A1");
    m_Api->Undo();
    tCell* wCell=m_Api->Cell("A1");
    tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wCell->Class());
    tStringStream wStream;
    wStream <<  wCellClassAttribute->ClassName() << ":" << wCellClassAttribute->RefName();
    //cout << endl << wStream.str() << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle+" "+wSubTitle+"1", wStream.str()=="SkCellButton:SkCellButton");
    
    
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    wCell=m_Api->Cell("A3");
    wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wCell->Class());
    wStream.str("");
    wStream << wCellClassAttribute->ClassName() << ":" << wCellClassAttribute->RefName();
    //cout << endl << wStream.str() << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle+" "+wSubTitle+"2", wStream.str()=="SkCellButton:SkCellButton2");
   
    wSubTitle="Copy";
    m_Api->Copy("A1:A3");
    
    m_Api->UndoPaste("B1:B12");
    
    m_Api->UndoCellValue("B3", 12);
    m_Api->Undo();
    
    m_Api->Undo();
    m_Api->Redo();
    
    wCell=m_Api->Cell("B3");
    wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wCell->Class());
    wStream.str("");
    wStream << wCellClassAttribute->ClassName() << ":" << wCellClassAttribute->RefName();
    //cout << endl << wStream.str() << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle+" "+wSubTitle+"2", wStream.str()=="SkCellButton:SkCellButton21");
}

void TestSkCellClassAttribute::TestClassAttributeTitleRangeSpill() {
    // Regression TestDate.sker C12: Title = R[-1]C[-1]:R[1]C[-1] materializes a range
    // spill on the attribute. The host SkCellClassPieChart cell must stay a valid class.
    RegisterPieChartClass();

    m_Api->UndoCellValue("B11", tVariant(tString("alpha")));
    m_Api->UndoCellValue("B12", tVariant(3.14));
    m_Api->UndoCellValue("B13", tVariant(true));

    CPPUNIT_ASSERT_MESSAGE("place PieChart", m_Api->UndoCellClass("C12", "SkCellClassPieChart"));

    CPPUNIT_ASSERT_MESSAGE(
        "Title range spill formula",
        m_Api->UndoCellAttribute("C12", "Title", tVariant(tString("=R[-1]C[-1]:R[1]C[-1]"))));

    tCell* wHost = m_Api->Cell("C12");
    CPPUNIT_ASSERT(wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(
        "C12 ClassAttribute after Title range spill",
        wHost->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(
        "C12 class name",
        wHost->ClassAttribute()->ClassName() == "SkCellClassPieChart");

    tCellAttribute* wTitleAttr = m_Api->CellAttribute("C12", "Title");
    CPPUNIT_ASSERT_MESSAGE("Title attribute cell", wTitleAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(
        "Title keeps range formula",
        wTitleAttr->FormulaStr().find("R[-1]C[-1]") != tString::npos
            || wTitleAttr->FormulaStr().find("B11") != tString::npos);

    tString wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    CPPUNIT_ASSERT_MESSAGE("WriteJson", !wSave.empty());
    CPPUNIT_ASSERT_MESSAGE("ReadJson roundtrip", m_Api->ReadJson(wSave));

    wHost = m_Api->Cell("C12");
    CPPUNIT_ASSERT(wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(
        "C12 ClassAttribute after ReadJson",
        wHost->ClassAttribute() != nullptr);

#ifdef checksp
    m_Api->Check();
#endif
}

void TestSkCellClassAttribute::CreateGenericFloatingClass() {
    m_Api->ActiveSheet("Sheet1");
    m_Api->RegisterClassAttribute("Pie", "Graphic", "Test");
    m_Api->AddProperty("Label", "string", "Type", 0, "Sheet1!A1");
    m_Api->AddProperty("Data", "string", "Type", 0, "Sheet1!A1");
}

void TestSkCellClassAttribute::TestFloatingObjectLayout() {
    CreateGenericFloatingClass();
    tString wTitle = "TestFloatingObjectLayout";
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("Pie", "Pie", "Sheet1"));

    tFloatingObject* wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host sheet",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " cell class on host",
        dynamic_cast<tCellClassAttribute*>(wHost->Class()) != nullptr);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttribute",
        m_Api->UndoFloatingObjectAttribute("Pie", "Label", "Hello"));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " active sheet after host attribute",
        m_Api->ActiveSheet() != nullptr && m_Api->ActiveSheet()->Name() == "Sheet1");
    m_Api->ActiveSheet(CstSheetClassAnchor);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " active sheet after ActiveSheet(_$$A)",
        m_Api->ActiveSheet() != nullptr && m_Api->ActiveSheet()->Name() == "Sheet1");
    tCellAttribute* wLabelAttr = wHost->CellAttribute("Label");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label attribute", wLabelAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label value", wLabelAttr->Value().Str() == "Hello");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectLayout",
        m_Api->UndoFloatingObjectLayout("Pie", 12.12, wObject->Layout().DiffY(), wObject->Layout().Width(),
                                        wObject->Layout().Height(), wObject->Layout().Opacity()));
    CPPUNIT_ASSERT_MESSAGE("Test DiffX before json", wObject->Layout().DiffX() == 12.12);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Undo layout", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("Test DiffX after undo layout", wObject->Layout().DiffX() == 0.0);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Redo layout", m_Api->Redo());
    CPPUNIT_ASSERT_MESSAGE("Test DiffX after redo layout", wObject->Layout().DiffX() == 12.12);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Undo layout again", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Undo attribute", m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Undo insert", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " removed after undo insert", wContainer->ByName("Pie") == nullptr);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Redo insert", m_Api->Redo());
    wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " restored after redo insert", wObject != nullptr);
    wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout default after redo insert only",
        wObject->Layout().DiffX() == 0.0);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Redo attribute", m_Api->Redo());
    wLabelAttr = wHost->CellAttribute("Label");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label after redo attribute", wLabelAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label value after redo attribute",
        wLabelAttr->Value().Str() == "Hello");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Redo layout", m_Api->Redo());
    CPPUNIT_ASSERT_MESSAGE("Test DiffX before json write", wObject->Layout().DiffX() == 12.12);

    tString wJson = m_Api->WriteJson(wWorkBook->Uri());

    tearDown();
    setUp();

    CreateGenericFloatingClass();
    tBool wLoad = m_Api->ReadJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("Test ReadJson", wLoad == true);

    wWorkBook = m_Api->ActiveWorkBook();
    wContainer = wWorkBook->FloatingObjectContainer();
    wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName after load", wObject != nullptr);
    CPPUNIT_ASSERT_MESSAGE("Test DiffX", wObject->Layout().DiffX() == 12.12);
}

void TestSkCellClassAttribute::TestFloatingObjectAttributesBatch() {
    CreateGenericFloatingClass();
    tString wTitle = "TestFloatingObjectAttributesBatch";
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("Pie", "Pie", "Sheet1"));

    tFloatingObject* wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " seed Label",
        m_Api->UndoFloatingObjectAttribute("Pie", "Label", "Hello"));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " seed Data",
        m_Api->UndoFloatingObjectAttribute("Pie", "Data", "OldData"));

    tVectorCellAttributeWire wBatch;
    tCellAttributeWire wLabel;
    wLabel.Name = "Label";
    wLabel.Value = tVariant("World");
    wBatch.push_back(wLabel);
    tCellAttributeWire wData;
    wData.Name = "Data";
    wData.Value = tVariant("Series A");
    wBatch.push_back(wData);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttributes batch",
        m_Api->UndoFloatingObjectAttributes("Pie", wBatch));

    tCellAttribute* wLabelAttr = wHost->CellAttribute("Label");
    tCellAttribute* wDataAttr = wHost->CellAttribute("Data");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label attribute", wLabelAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data attribute", wDataAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label value", wLabelAttr->Value().Str() == "World");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data value", wDataAttr->Value().Str() == "Series A");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " single undo step", m_Api->LastUndo() != nullptr);
    tUndoRaz* wBatchUndo = dynamic_cast<tUndoRaz*>(m_Api->LastUndo());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " undo cast", wBatchUndo != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " undo class name",
        wBatchUndo->ClassName() == "tUndoCellClassAttributes");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Undo batch", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label after undo",
        wHost->CellAttribute("Label") != nullptr
            && wHost->CellAttribute("Label")->Value().Str() == "Hello");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data after undo",
        wHost->CellAttribute("Data") != nullptr
            && wHost->CellAttribute("Data")->Value().Str() == "OldData");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " Redo batch", m_Api->Redo());
    wLabelAttr = wHost->CellAttribute("Label");
    wDataAttr = wHost->CellAttribute("Data");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label after redo", wLabelAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data after redo", wDataAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label value after redo", wLabelAttr->Value().Str() == "World");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data value after redo", wDataAttr->Value().Str() == "Series A");

    tVectorCellAttributeWire wBadBatch;
    tCellAttributeWire wBadLabel;
    wBadLabel.Name = "Label";
    wBadLabel.Value = tVariant("ShouldNotStick");
    wBadBatch.push_back(wBadLabel);
    tCellAttributeWire wBadData;
    wBadData.Name = "Data";
    wBadData.Value = tVariant(tString("=SUM(A1:OK^"));
    wBadBatch.push_back(wBadData);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " syntax error returns false",
        !m_Api->UndoFloatingObjectAttributes("Pie", wBadBatch));

    wLabelAttr = wHost->CellAttribute("Label");
    wDataAttr = wHost->CellAttribute("Data");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label unchanged after syntax error",
        wLabelAttr != nullptr && wLabelAttr->Value().Str() == "World");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Data unchanged after syntax error",
        wDataAttr != nullptr && wDataAttr->Value().Str() == "Series A");

#ifdef checksp
    m_Api->Check();
#endif
}

void TestSkCellClassAttribute::TestFloatingObjectLayoutAnchorCell() {
    CreateGenericFloatingClass();
    tString wTitle = "TestFloatingObjectLayoutAnchorCell";
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("Pie", "Pie", "Sheet1"));

    tFloatingObject* wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout anchor on empty I10",
        m_Api->UndoFloatingObjectLayout("Pie", 0.0, 0.0, 120.0, 80.0, 1.0, "Sheet1!I10"));

    tCell* wAnchor = wObject->Layout().AnchorCell(wWorkBook);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor I10", wAnchor != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor I10 row", wAnchor->RowIndex() == 10);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor I10 col", wAnchor->ColIndex() == 9);
}

void TestSkCellClassAttribute::TestFloatingObjectJsonLoad() {
    CreateGenericFloatingClass();
    tString wTitle = "TestFloatingObjectJsonLoad";
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " sheet data",
        m_Api->UndoCellValue("A1", "Sheet data"));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("Pie", "Pie", "Sheet1"));

    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();
    tFloatingObject* wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host on _$$A",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttribute",
        m_Api->UndoFloatingObjectAttribute("Pie", "Label", "Hello"));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectLayout",
        m_Api->UndoFloatingObjectLayout("Pie", 5.5, 6.6, 120.0, 80.0, 0.9, "Sheet1!A1"));

    tCell* wAnchor = wObject->Layout().AnchorCell(wWorkBook);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor before save", wAnchor != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor sheet before save",
        wAnchor->Sheet() != nullptr && wAnchor->Sheet()->Name() == "Sheet1");

    tString wJson = m_Api->WriteJson(wWorkBook->Uri());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " WriteJson", !wJson.empty());

    tearDown();
    setUp();
    CreateGenericFloatingClass();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson", m_Api->ReadJson(wJson));

    wWorkBook = m_Api->ActiveWorkBook();
    wContainer = wWorkBook->FloatingObjectContainer();
    wObject = wContainer->ByName("Pie");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " registry after load", wObject != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " target sheet after load",
        wObject->TargetSheetName() == "Sheet1");

    wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host after load", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host sheet after load",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);

    tCellClassAttribute* wHostClass = wHost->ClassAttribute();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " cell class on host after load", wHostClass != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " class name on host after load",
        wHostClass->ClassName() == "Pie");

    tCellAttribute* wLabelAttr = wHost->CellAttribute("Label");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label attribute after load", wLabelAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Label value after load",
        wLabelAttr->Value().Str() == "Hello");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout DiffX after load", wObject->Layout().DiffX() == 5.5);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout DiffY after load", wObject->Layout().DiffY() == 6.6);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout Width after load", wObject->Layout().Width() == 120.0);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout Height after load", wObject->Layout().Height() == 80.0);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " layout Opacity after load", wObject->Layout().Opacity() == 0.9);

    wAnchor = wObject->Layout().AnchorCell(wWorkBook);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor after load", wAnchor != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor sheet after load",
        wAnchor->Sheet() != nullptr && wAnchor->Sheet()->Name() == "Sheet1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " anchor cell after load",
        wAnchor->RowIndex() == 1 && wAnchor->ColIndex() == 1);

    CPPUNIT_ASSERT_MESSAGE(wTitle + " active sheet after load",
        m_Api->ActiveSheet() != nullptr && m_Api->ActiveSheet()->Name() == "Sheet1");
    tCell* wSheetCell = m_Api->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Sheet1 A1 after load", wSheetCell != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " Sheet1 data after load",
        wSheetCell->Value().Str() == "Sheet data");
}

void TestSkCellClassAttribute::TestFloatingObjectJsonLoadLazyHost() {
    // PieChart visible in session but host A2 was missing from _$$A in .sker when no attribute undo ran.
    RegisterPieChartClass();
    tString wTitle = "TestFloatingObjectJsonLoadLazyHost";
    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("PieChart", "SkCellClassPieChart", "Sheet1"));

    tFloatingObject* wObject = wWorkBook->FloatingObjectContainer()->ByName("PieChart");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);
    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell before save", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host class before save", wHost->ClassAttribute() != nullptr);

    tString wJson = m_Api->WriteJson(wWorkBook->Uri());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " WriteJson", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host class in json",
        wJson.find("SkCellClassPieChart") != tString::npos);

    tearDown();
    setUp();
    RegisterPieChartClass();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " ReadJson", m_Api->ReadJson(wJson));

    wWorkBook = m_Api->ActiveWorkBook();
    wObject = wWorkBook->FloatingObjectContainer()->ByName("PieChart");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " registry after load", wObject != nullptr);

    wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host after load", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host sheet after load",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " cell class on host after load", wHost->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " class name on host after load",
        wHost->ClassAttribute()->ClassName() == "SkCellClassPieChart");
}

void TestSkCellClassAttribute::TestFloatingObjectDataRangeInsertRow() {
    // Production path: RegisterClassAttribute + AddProperty (wasm) then floating object + DATARANGE attribute.
    RegisterPieChartClass();
    tString wTitle = "TestFloatingObjectDataRangeInsertRow";

    m_Api->UndoCellValue("A1", tVariant(1.0));
    m_Api->UndoCellValue("A2", tVariant(2.0));
    m_Api->UndoCellValue("A3", tVariant(3.0));
    m_Api->UndoCellValue("A4", tVariant(4.0));
    m_Api->UndoCellValue("A5", tVariant(5.0));

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    tFloatingObjectContainer* wContainer = wWorkBook->FloatingObjectContainer();

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("PieChart1", "SkCellClassPieChart", "Sheet1"));

    tFloatingObject* wObject = wContainer->ByName("PieChart1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " target sheet", wObject->TargetSheetName() == "Sheet1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " floating class name", wObject->ClassName() == "SkCellClassPieChart");

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host sheet",
        wHost->Sheet() != nullptr && wHost->Sheet()->Name() == CstSheetClassAnchor);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " cell class on host",
        dynamic_cast<tCellClassAttribute*>(wHost->Class()) != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ClassAttribute on host", wHost->ClassAttribute() != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " host class name",
        wHost->ClassAttribute()->ClassName() == "SkCellClassPieChart");
    // Attributes are lazy: Size()==0 until UndoFloatingObjectAttribute / CellAttribute(name).

    const tString wDisplayFormula = "=DATARANGE(A1:A5)";
    const tString wStoredFormula = QualifyRefsForSheet("Sheet1", wDisplayFormula);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " qualify refs",
        wStoredFormula == "=DATARANGE(Sheet1!A1:A5)");

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttribute DataRange",
        m_Api->UndoFloatingObjectAttribute("PieChart1", "DataRange", tVariant(wStoredFormula)));

    tCellAttribute* wDataRangeAttr = wHost->CellAttribute("DataRange");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " DataRange attribute", wDataRangeAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " initial formula keeps range",
        wDataRangeAttr->FormulaStr().find("A1:A5") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " initial DATARANGE json",
        wDataRangeAttr->Value().Str().find("A1:A5") != tString::npos);

    m_Api->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertRow", m_Api->UndoInsertRow(3, 1));

    wDataRangeAttr = wHost->CellAttribute("DataRange");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " DataRange attribute after insert", wDataRangeAttr != nullptr);
    // InsertRow: FormulaStr follows ColRow range resize; Value() cache is unchanged until Calculate.
    CPPUNIT_ASSERT_MESSAGE(wTitle + " formula rebased to A1:A6",
        wDataRangeAttr->FormulaStr().find("A1:A6") != tString::npos);

    m_Api->Undo();
    wDataRangeAttr = wHost->CellAttribute("DataRange");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " undo restores A1:A5 formula",
        wDataRangeAttr->FormulaStr().find("A1:A5") != tString::npos);
}

void TestSkCellClassAttribute::TestUndoMoveDataRangeFloatingObject() {
    RegisterPieChartClass();
    tString wTitle = "TestUndoMoveDataRangeFloatingObject";

    m_Api->UndoCellValue("A1", tVariant(1.0));
    m_Api->UndoCellValue("A2", tVariant(2.0));
    m_Api->UndoCellValue("A3", tVariant(3.0));
    m_Api->UndoCellValue("A4", tVariant(4.0));
    m_Api->UndoCellValue("A5", tVariant(5.0));
    m_Api->UndoCellValue("B1", tVariant("L1"));
    m_Api->UndoCellValue("B2", tVariant("L2"));
    m_Api->UndoCellValue("B3", tVariant("L3"));
    m_Api->UndoCellValue("B4", tVariant("L4"));
    m_Api->UndoCellValue("B5", tVariant("L5"));

    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoInsertFloatingObject",
        m_Api->UndoInsertFloatingObject("PieChart1", "SkCellClassPieChart", "Sheet1"));

    tFloatingObject* wObject = m_Api->ActiveWorkBook()->FloatingObjectContainer()->ByName("PieChart1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " ByName", wObject != nullptr);

    tCell* wHost = wObject->HostCell();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " HostCell", wHost != nullptr);

    const tString wValuesFormula = QualifyRefsForSheet("Sheet1", "=DATARANGE(A1:A5)");
    const tString wLabelsFormula = QualifyRefsForSheet("Sheet1", "=DATARANGE(B1:B5)");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttribute DataRange",
        m_Api->UndoFloatingObjectAttribute("PieChart1", "DataRange", tVariant(wValuesFormula)));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoFloatingObjectAttribute chartData",
        m_Api->UndoFloatingObjectAttribute("PieChart1", "chartData", tVariant(wLabelsFormula)));

    m_Api->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " UndoMove", m_Api->UndoMove("A1:B5", "A2:B6"));

    tCellAttribute* wDataRangeAttr = wHost->CellAttribute("DataRange");
    tCellAttribute* wChartDataAttr = wHost->CellAttribute("chartData");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " DataRange attribute after move", wDataRangeAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " chartData attribute after move", wChartDataAttr != nullptr);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " DataRange formula rebased to A2:A6",
        wDataRangeAttr->FormulaStr().find("A2:A6") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " chartData formula rebased to B2:B6",
        wChartDataAttr->FormulaStr().find("B2:B6") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " DataRange json rebased to A2:A6",
        wDataRangeAttr->Value().Str().find("A2:A6") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " chartData json rebased to B2:B6",
        wChartDataAttr->Value().Str().find("B2:B6") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    wDataRangeAttr = wHost->CellAttribute("DataRange");
    wChartDataAttr = wHost->CellAttribute("chartData");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " undo restores DataRange A1:A5",
        wDataRangeAttr->FormulaStr().find("A1:A5") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " undo restores chartData B1:B5",
        wChartDataAttr->FormulaStr().find("B1:B5") != tString::npos);
}

void TestSkCellClassAttribute::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkCellClassAttribute::tearDown() {
	delete(m_Api);
	tClassFactory::Instance()->Clear();
}
