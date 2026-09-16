//==============================================================================
// TestSkTypesClass
// Test library SkTypesClass
//==============================================================================

#include "../include/TestSkModelClass.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestModelClass);


// We can send it to the API of a feature 
TestModelClass::TestModelClass() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};


class tTestProperty : public tVirtualClass {
private:
	tString m_Name;
public:
	tTestProperty() : tVirtualClass(), m_Name() {};
	tTestProperty(const tTestProperty& sPropertyClass) :
		tVirtualClass(sPropertyClass),
		m_Name(sPropertyClass.m_Name) {}
	virtual ~tTestProperty() {}


	tString ClassName() const override { return("tTestProperty"); }


	/// @brief      Clone derived class.
	tVirtualClass* Clone() override {
		return(new tTestProperty(*this));
	}
	void Name(tString sName) { m_Name = sName; }
	tString Name() { return(m_Name); }

};

class tTestClass : public tVirtualClass {
private:
	tInt    m_Int;
	tBool   m_Bool;
	tDouble m_Double;
	tString m_String;
	tDate	m_Date;
	tClassError m_Error;
	tVariant m_Variant;
	tVirtualClass* m_VirtualClass;
public:
	tTestClass() : tVirtualClass(), m_Int(0), m_Bool(false), m_Double(0), m_String(""), m_Date(), m_Error(), m_VirtualClass(nullptr) {}
	~tTestClass() {
		if (m_VirtualClass != nullptr) delete(m_VirtualClass);
	}

	tString ClassName() const override { return("tTestClass"); }

	void Int(tInt sInt) { m_Int = sInt; }
	tInt Int() { return(m_Int); }

	void Bool(tBool sBool) { m_Bool = sBool; }
	tBool Bool() { return(m_Bool); }

	void Double(tDouble sDouble) { m_Double = sDouble; }
	tDouble Double() { return(m_Double); }

	void String(tString sString) { m_String = sString; }
	tString String() { return(m_String); }

	void Date(tDate sDate) { m_Date = sDate; }
	tDate Date() { return(m_Date); }

	void Error(tClassError sError) { m_Error = sError; }
	tClassError Error() { return(m_Error); }

	void Variant(tVariant sVariant) { m_Variant = sVariant; }
	tVariant Variant() { return(m_Variant); }

	void VirtualClass(tVirtualClass* sVirtualClass) { 
		m_VirtualClass = sVirtualClass->Clone();
	}
	tVirtualClass* VirtualClass() { 
		return(m_VirtualClass); 
	}
#ifdef _DEBUGSK
	tString Debug() override {
        tStringStream wStringStream;
        wStringStream << "String:" << m_String << "Int " << m_Int;
        return(wStringStream.str());
	}
#endif
};

tTestClass* CreateTestClass() {
	return(new tTestClass());
}

tTestProperty* CreatePropertyClass() {
	return(new tTestProperty());
}


void TestModelClass::TestCreateInstance() {
	tString wTitle = "TestCreateInstance";

	tString wSubTitle = "is Create";
	
	tClassFactory::Instance()->Register("tTestClass","Test class", &CreateTestClass);
	tVirtualClass* wTestClass = tClassFactory::Instance()->Create("tTestClass");

	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestClass!=nullptr);
	
	delete(wTestClass);
}

void TestModelClass::TestProperty() {
	tString wTitle = "TestProperty";

	tString wSubTitle = "is Create";

	tClassFactory::Instance()->Register("tTestClass","Test class", &CreateTestClass);
	tVirtualClass* wTestClass = tClassFactory::Instance()->Create("tTestClass");

	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestClass != nullptr);

	tModelClass* wClassModel = tClassFactory::Instance()->Get("tTestClass");

	tSetterGetter wSetterGetter;
	wSetterGetter.Set<tTestClass>(&tTestClass::Int, &tTestClass::Int);
	wClassModel->AddProperty("Int", tVariantType::t_int, "Int", 1, tVariant(0),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::Bool, &tTestClass::Bool);
	wClassModel->AddProperty("Bool", tVariantType::t_bool, "Bool", 0, tVariant(false),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::Double, &tTestClass::Double);
	wClassModel->AddProperty("Double", tVariantType::t_double, "Double", 2, tVariant(0.0f),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::String, &tTestClass::String);
	wClassModel->AddProperty("String", tVariantType::t_string, "Label", 3, tVariant(""),"", wSetterGetter);
#ifndef __EMSCRIPTEN__
	wSetterGetter.Set<tTestClass>(&tTestClass::Date, &tTestClass::Date);
	wClassModel->AddProperty("Date", tVariantType::t_date, "Date", 4, tVariant(0), "",wSetterGetter);
#endif
	wSetterGetter.Set<tTestClass>(&tTestClass::Error, &tTestClass::Error);
	wClassModel->AddProperty("Error", tVariantType::t_error, "Error", 5, tClassError(), "",wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::VirtualClass, &tTestClass::VirtualClass);
	wClassModel->AddProperty("Class", tVariantType::t_class, "Class", 6, tVariant(), "",wSetterGetter);

	// Test 
	tVariant wDate;
	wDate.SetDate(1221);
	
	wClassModel->Property(wTestClass, "Int", 123);
	wClassModel->Property(wTestClass, "Bool", true);
	wClassModel->Property(wTestClass, "Double", 123.4567);
	wClassModel->Property(wTestClass, "String", "coucou");
	wClassModel->Property(wTestClass, "Date", wDate);

	tClassError wError(tTypeError::t_value, "Test Error");
	wClassModel->Property(wTestClass, "Error", wError);

	// Class =====================================================================
	tClassFactory::Instance()->Register("tTestProperty","Test property", &CreatePropertyClass);
	tModelClass* wPropertyModel = tClassFactory::Instance()->Get("tTestProperty");

	wSetterGetter.Set<tTestProperty>(&tTestProperty::Name, &tTestProperty::Name);
	wPropertyModel->AddProperty("Name", tVariantType::t_string, "TestClass", 0, tVariant(),"", wSetterGetter);

	tTestProperty wPropertyClass;
	wPropertyModel->Property(&wPropertyClass, "Name", tVariant("Coucou property"));

	wClassModel->Property(wTestClass, "Class", &wPropertyClass);


	tVariant wResult = wClassModel->Property(wTestClass, "Int");
	wSubTitle = "tInt";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type()==tVariantType::t_int) && (wResult.Int() == 123));

	wResult = wClassModel->Property(wTestClass, "Bool");
	wSubTitle = "tBool";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_bool) && (wResult.Bool() == true));

	wResult = wClassModel->Property(wTestClass, "Double");
	wSubTitle = "tDouble";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_double) && (wResult.Double() == 123.4567));

	wResult = wClassModel->Property(wTestClass, "String");
	wSubTitle = "tString";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_string) && (wResult.String() == "coucou"));

	wResult = wClassModel->Property(wTestClass, "Error");
	wSubTitle = "tError";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_error) && ((wResult.Error().Code()== tTypeError::t_value) && (wResult.Error().String()) == "Test Error"));

	wResult = wClassModel->Property(wTestClass, "Class");
	wSubTitle = "tClass";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_class));
	tTestProperty* wPropertyClassResult=dynamic_cast<tTestProperty*>(wResult.Class());

	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wPropertyClassResult->Name() == "Coucou property"));

    // Test Json of model =====================================================
    tString wJson = tClassFactory::Instance()->Json("tTestProperty");
    
    tModelClass wModelClass;
    Document wDocument;
    wDocument.Parse(wJson.c_str());
    wModelClass.Json(wDocument);
    
    wSubTitle="Test Json";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wModelClass.ClassName() == "tTestProperty");

    
	delete(wTestClass);
}
void TestModelClass::TestJson() {
	tString wTitle = "TestJson";

	tString wSubTitle = "is Create";

	tClassFactory::Instance()->Register("SkTestClass","Test class", &CreateTestClass);
	tVirtualClass* wTestClass = tClassFactory::Instance()->Create("SkTestClass");

	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestClass != nullptr);

	tModelClass* wClassModel = tClassFactory::Instance()->Get("SkTestClass");

	tSetterGetter wSetterGetter;
	wSetterGetter.Set<tTestClass>(&tTestClass::Int, &tTestClass::Int);
	wClassModel->AddProperty("Int", tVariantType::t_int, "Int", 1, tVariant(0),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::Bool, &tTestClass::Bool);
	wClassModel->AddProperty("Bool", tVariantType::t_bool, "Bool", 0, tVariant(false),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::Double, &tTestClass::Double);
	wClassModel->AddProperty("Double", tVariantType::t_double, "Double", 2, tVariant(0.0f),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::String, &tTestClass::String);
	wClassModel->AddProperty("String", tVariantType::t_string, "Label", 3, tVariant(""),"", wSetterGetter);
#ifndef __EMSCRIPTEN__
	wSetterGetter.Set<tTestClass>(&tTestClass::Date, &tTestClass::Date);
	wClassModel->AddProperty("Date", tVariantType::t_date, "Date", 4, tVariant(0),"", wSetterGetter);
#endif
	wSetterGetter.Set<tTestClass>(&tTestClass::Error, &tTestClass::Error);
	wClassModel->AddProperty("Error", tVariantType::t_error, "Error", 5, tClassError(),"", wSetterGetter);

	wSetterGetter.Set<tTestClass>(&tTestClass::VirtualClass, &tTestClass::VirtualClass);
	wClassModel->AddProperty("Class", tVariantType::t_class, "Class", 6, tVariant(), "",wSetterGetter);

	// Test 
	tVariant wDate;
	wDate.Parse("03/11/2023");

	wClassModel->Property(wTestClass, "Int", 123);
	wClassModel->Property(wTestClass, "Bool", true);
	wClassModel->Property(wTestClass, "Double", 123.4567);
	wClassModel->Property(wTestClass, "String", "coucou");
	wClassModel->Property(wTestClass, "Date", wDate);

	tClassError wError(tTypeError::t_value, "Test Error");
	wClassModel->Property(wTestClass, "Error", wError);

	// Class =====================================================================
	tClassFactory::Instance()->Register("tTestProperty","Test Property", &CreatePropertyClass);
	tModelClass* wPropertyModel = tClassFactory::Instance()->Get("tTestProperty");

	wSetterGetter.Set<tTestProperty>(&tTestProperty::Name, &tTestProperty::Name);
	wPropertyModel->AddProperty("Name", tVariantType::t_string, "TestClass", 0, tVariant(), "",wSetterGetter);

	tTestProperty wPropertyClass;
	wPropertyModel->Property(&wPropertyClass,"Name", tVariant("Coucou property"));
	
	wClassModel->Property(wTestClass, "Class", &wPropertyClass);

	StringBuffer wStringBuffer;
	Writer<StringBuffer> wWriter(wStringBuffer);
	
    wClassModel->JsonAssociated(&wWriter,wTestClass);
	
	tString wJsonStr = wStringBuffer.GetString();
	//cout << endl << wStringBuffer.GetString() << endl;

	Document wDocument;
	wDocument.Parse(wJsonStr.c_str());

	tVirtualClass* wTestClass1 = tClassFactory::Instance()->Create("SkTestClass");

    wClassModel->JsonAssociated(wDocument,wTestClass1);

	tVariant wResult = wClassModel->Property(wTestClass1, "Int");
	wSubTitle = "tInt";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_int) && (wResult.Int() == 123));

	wResult = wClassModel->Property(wTestClass1, "Bool");
	wSubTitle = "tBool";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_bool) && (wResult.Bool() == true));

	wResult = wClassModel->Property(wTestClass1, "Double");
	wSubTitle = "tDouble";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_double) && (wResult.Double() == 123.4567));

	wResult = wClassModel->Property(wTestClass1, "String");
	wSubTitle = "tString";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_string) && (wResult.String() == "coucou"));

	wResult = wClassModel->Property(wTestClass1, "Error");
	wSubTitle = "tError";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_error) && ((wResult.Error().Code() == tTypeError::t_value) && (wResult.Error().String()) == "Test Error"));

    
	wResult = wClassModel->Property(wTestClass1, "Class");
	wSubTitle = "tClass";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_class));
	tTestProperty* wPropertyClassResult = dynamic_cast<tTestProperty*>(wResult.Class());

	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wPropertyClassResult->Name() == "Coucou property"));

	delete(wTestClass);
	delete(wTestClass1);
}


void TestModelClass::setUp() {
	m_Application = tApplication::Instance();
};

void TestModelClass::tearDown() {
}



