//==============================================================================
// TestSkRoot
// Test library SkRootVariant
//==============================================================================

#include "../include/TestSkRootVariant.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRootVariant);

// We can send it to the API of a feature 
TestSkRootVariant::TestSkRootVariant():CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};

void TestSkRootVariant::TestVariant() {
	tString wTitle = "Test variant";
	tString wSubTitle = "integer";
	//tInt wPhase = 1;
	tVariant wV1 = 1;
	tVariant wV2 = 2;

	tVariant wV3;
	//cout << endl;
	//cout << wPhase++ << endl;
	wV3 = wV1/ 0; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wV3.Type() == tVariantType::t_error) && (wV3.Error().Code() == tTypeError::t_div0)));
	//cout << wPhase++ << endl;
	wV3= wV1 + wV2; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wV3.Type()==tVariantType::t_int) && (wV3.Int()==3)));
	//cout << wPhase++ << endl;
	wV3 = wV1 - wV2; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wV3.Type() == tVariantType::t_int) && (wV3.Int() == -1)));
	{
		tVariant wHi = INT_MAX;
		tVariant wOne = 1;
		tVariant wSum = wHi + wOne;
		CPPUNIT_ASSERT_MESSAGE(wTitle + " int add overflow widens to double",
			wSum.IsDouble() && wSum.Double() == static_cast<tDouble>(INT_MAX) + 1.0);
		tVariant wLo = INT_MIN;
		tVariant wDiff = wLo - wOne;
		CPPUNIT_ASSERT_MESSAGE(wTitle + " int sub overflow widens to double",
			wDiff.IsDouble() && wDiff.Double() == static_cast<tDouble>(INT_MIN) - 1.0);
	}
	//cout << wPhase++ << endl;

	wV1 = 1233.34;
	wV3 = wV2 - wV1; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wV3.Type() == tVariantType::t_double) && (wV3.Double() == -1231.34)));
	//cout << wPhase++ << endl;
	wV3 = wV2 / 0; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wV3.Type() == tVariantType::t_error) && (wV3.Error().Code() == tTypeError::t_div0)));

	// String
	wSubTitle = "string";

	tVariant wS1 = "Hello";
	tVariant wS3 = wS1 + wV1; 	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, ((wS3.Type() == tVariantType::t_error) && (wS3.Error().Code() == tTypeError::t_value)));
	wS3 = wS1 + " Word"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wS3.String() == "Hello Word");

	tVariant wS2 = " Word";
	wS3 = wS1 + wS2; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wS3.String() == "Hello Word");
    
    wSubTitle= "Test 50 > 40";
    tVariant wVariant1=40;
    tVariant wVariant2=50.5;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVariant2>wVariant1);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant1>wVariant2));
    
    wSubTitle= "Test 50 < 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wVariant1<wVariant2));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant2<wVariant1));
    
    wSubTitle= "Test 50 => 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVariant2>=wVariant1);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant1>=wVariant2));
    
    wSubTitle= "Test 50 <= 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wVariant1<=wVariant2));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant2<=wVariant1));
 
    wVariant2=40;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant2>wVariant1));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant1>wVariant2));
    
    wSubTitle= "Test 40 < 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant1<wVariant2));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !(wVariant2<wVariant1));
    
    wSubTitle= "Test 40 => 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVariant2>=wVariant1);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wVariant1>=wVariant2));
    
    wSubTitle= "Test 40 <= 40";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wVariant1<=wVariant2));
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wVariant2<=wVariant1));
    
}

void TestSkRootVariant::TestVariantClass() {
	tString wTitle = "Test class variant";
	tString wSubTitle = "integer + SkVariant TestClass";

	tRootVariant wVariantClass(10, "cm");
	tVariant wI1 = 1;

	tVariant wResult = wI1 + &wVariantClass;

	tRootVariant* wResultClass = dynamic_cast<tRootVariant*>(wResult.Class());
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResultClass != nullptr);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle+" value ", wResultClass->Value()->Int()==11);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wResultClass->GetUnit() == "cm");
	
	tRootVariant wClass1(20, "cm");
	tRootVariant wClass2(10, "cm");
	tVariant wVariantClass1 = &wClass1;
	tVariant wVariantClass2 = &wClass2;
    
    // Multiply
    wSubTitle = "Multiply  TestClass";
	wResult = wVariantClass1 * wVariantClass2;

	wResultClass = dynamic_cast<tRootVariant*>(wResult.Class());
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResultClass != nullptr);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wResultClass->Value()->Int() == 200);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wResultClass->GetUnit() == "cm");

    // Divide
    wSubTitle = "Divide  TestClass";
    wResult = wVariantClass1 / wVariantClass2;
    wResultClass = dynamic_cast<tRootVariant*>(wResult.Class());
    
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wResultClass->Value()->Double() == 20);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wResultClass->GetUnit() == "cm");
}

void TestSkRootVariant::TestParse() {
    // US
    tApplication::Instance()->Locale("us");
    tString wTitle = "Test variant parse";
	tVariant wVariant;
    
	tString wStringValue = "Hello ///\\sqdsqdsqdqs";
	wVariant.Parse(wStringValue);
	tString wSubTitle = "Parse " + wStringValue;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.String() == wStringValue);

	wVariant.Parse("12345");
	wSubTitle = "Parse 12345";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Int() == 12345);

	wVariant.Parse("12345.67");
	wSubTitle = "Parse 12345.67";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", round(wVariant.Double() * 100.0) / 100.0 == 12345.67);
    
    tApplication::Instance()->Locale("fr");
    wVariant.Parse("12345,67");
    wSubTitle = "Parse 12345,67 fr";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", round(wVariant.Double() * 100.0) / 100.0 == 12345.67);
    
	tApplication::Instance()->Locale("us");
	wVariant.Parse("10-01-2023");
	wSubTitle = "Parse 10/01/2023";
	//cout << endl << wVariant.Str() << endl;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "10-01-2023");

	wVariant.Parse("01-02-2023");
	wSubTitle = "Parse 2023/02/01";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "01-02-2023");


	wVariant.Parse("9-1-2023");
	wSubTitle = "Parse 1/9/2023";
	//cout << endl << wVariant.Str() << endl;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "09-01-2023");

	wVariant.Parse("12-30-2023");
	wSubTitle = "Parse 2023/2/1";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "12-30-2023");
	
    tApplication::Instance()->Locale("fr");
    wVariant.Parse("01/02/2023");
    wSubTitle = "Parse 01/02/2023";
    //cout << endl << wVariant.Str() << endl;
    
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "02-01-2023");

    wVariant.Parse("28/02/2023");
    wSubTitle = "Parse 28/02/2023";
    //cout << endl << wVariant.Str() << endl;
    
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", wVariant.Str() == "02-28-2023");

    // Boolean value
    wSubTitle = "Parse boolean true false";
    wVariant.Parse("true");
    //cout << endl << wVariant.Str() << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ",( (wVariant.Bool() == true) && (wVariant.Type()==tVariantType::t_bool)) );
    wVariant.Parse("false");
    //cout << endl << wVariant.Str() << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle + " value ", ((wVariant.Bool() == false) && (wVariant.Type()==tVariantType::t_bool)) );

}


// =============================================================================
// Excel "&" concatenation operator
// Regression test for the bug where ="LABEL ("&Year-1&")" returned #VALUE!
// because Ampersand was routed through tVariant::operator+, which is strict
// on types. Ampersand must coerce every non-error operand to text.
// =============================================================================
void TestSkRootVariant::TestAmpersand() {
	tString wTitle = "Test Ampersand";
	// Force a stable locale: t_double formatting depends on Locale->Decimal()
	// (the FR locale would output "1,5" instead of "1.5").
	tApplication::Instance()->Locale("us");

	// string & string -------------------------------------------------------
	{
		tVariant wA = tString("Hello ");
		tVariant wB = tString("World");
		tVariant wR = Ampersand(wA, wB);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&string type",
			wR.Type() == tVariantType::t_string);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&string value",
			wR.String() == "Hello World");
	}

	// string & int — the bug from the user report --------------------------
	// ="ANNÉE DERNIÈRE ("&Year-1&")"  <=> ("LABEL (" & 2024) & ")"
	{
		tVariant wLabel = tString("LABEL (");
		tVariant wYear = tInt(2024);
		tVariant wClose = tString(")");
		tVariant wMid = Ampersand(wLabel, wYear);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&int type",
			wMid.Type() == tVariantType::t_string);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&int value",
			wMid.String() == "LABEL (2024");
		tVariant wFull = Ampersand(wMid, wClose);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " full chain value",
			wFull.String() == "LABEL (2024)");
	}

	// int & string (symmetric) ---------------------------------------------
	{
		tVariant wYear = tInt(2024);
		tVariant wSuffix = tString(" results");
		tVariant wR = Ampersand(wYear, wSuffix);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " int&string value",
			wR.String() == "2024 results");
	}

	// string & double (locale US) ------------------------------------------
	{
		tVariant wA = tString("x=");
		tVariant wB = tDouble(1.5);
		tVariant wR = Ampersand(wA, wB);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&double type",
			wR.Type() == tVariantType::t_string);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&double value (us)",
			wR.String() == "x=1.5");
	}

	// string & double (locale FR) — verifies decimal-separator localization
	{
		tApplication::Instance()->Locale("fr");
		tVariant wA = tString("x=");
		tVariant wB = tDouble(1.5);
		tVariant wR = Ampersand(wA, wB);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&double value (fr)",
			wR.String() == "x=1,5");
		tApplication::Instance()->Locale("us");
	}

	// string & bool (Excel renders TRUE/FALSE in uppercase) -----------------
	{
		tVariant wA = tString("flag=");
		tVariant wT = tBool(true);
		tVariant wF = tBool(false);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&bool true",
			Ampersand(wA, wT).String() == "flag=TRUE");
		CPPUNIT_ASSERT_MESSAGE(wTitle + " string&bool false",
			Ampersand(wA, wF).String() == "flag=FALSE");
	}

	// null treated as empty string -----------------------------------------
	{
		tVariant wA = tString("X");
		tVariant wNull;
		CPPUNIT_ASSERT_MESSAGE(wTitle + " null right",
			Ampersand(wA, wNull).String() == "X");
		CPPUNIT_ASSERT_MESSAGE(wTitle + " null left",
			Ampersand(wNull, wA).String() == "X");
		CPPUNIT_ASSERT_MESSAGE(wTitle + " null & null",
			Ampersand(wNull, wNull).String() == "");
	}

	// int & int — both coerced to text (Excel: ="1"&"2" -> "12") -----------
	{
		tVariant wA = tInt(12);
		tVariant wB = tInt(34);
		tVariant wR = Ampersand(wA, wB);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " int&int type",
			wR.Type() == tVariantType::t_string);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " int&int value",
			wR.String() == "1234");
	}

	// Large whole numbers: no scientific notation (INSIDE VLOOKUP keys like #G:20701000).
	{
		tVariant wPrefix = tString("#R:BA_AH#G:");
		tVariant wAccount = tDouble(20701000.0);
		tVariant wPart1 = Ampersand(wPrefix, wAccount);
		tVariant wPart2 = Ampersand(wPart1, tString("#E:"));
		tVariant wSuffix = tDouble(225.0);
		tVariant wKeyFull = Ampersand(wPart2, wSuffix);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " VLOOKUP key large account",
			wKeyFull.String() == "#R:BA_AH#G:20701000#E:225");
	}

	// Error propagates (left and right) ------------------------------------
	{
		tVariant wErr;
		wErr.SetError(tClassError(tTypeError::t_value, ""));
		tVariant wOk = tString("ok");
		tVariant wR1 = Ampersand(wErr, wOk);
		tVariant wR2 = Ampersand(wOk, wErr);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " error left propagates",
			wR1.Type() == tVariantType::t_error);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " error right propagates",
			wR2.Type() == tVariantType::t_error);
	}

	// "+" must remain strict — regression guard for ="abc"+1 ----------------
	{
		tVariant wA = tString("abc");
		tVariant wB = tInt(1);
		tVariant wR = wA + wB;
		CPPUNIT_ASSERT_MESSAGE(wTitle + " operator+ stays strict",
			(wR.Type() == tVariantType::t_error) &&
			(wR.Error().Code() == tTypeError::t_value));
	}
}

void TestSkRootVariant::setUp() {
	m_Application = tApplication::Instance();
};

void TestSkRootVariant::tearDown() {

}

void TestSkRootVariant::Error(tString sMessage) {
	//CPPUNIT_ASSERT_THROW_MESSAGE("Error " + sMessage, false, tException);
};

void TestSkRootVariant::Warning(tString sMessage) {
	CPPUNIT_ASSERT_MESSAGE("Warning " + sMessage, false);
};
