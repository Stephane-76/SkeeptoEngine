//==============================================================================
// TestSkGenerics
// Test library SkTypesClass
//==============================================================================

#include "../include/TestSkGeneric.hpp"

// Registers the fixture into the 'registry'
//#ifndef __EMSCRIPTEN__
// __EMSCRIPTEN__ BUG

CPPUNIT_TEST_SUITE_REGISTRATION(TestGeneric);
//#endif

// We can send it to the API of a feature 
TestGeneric::TestGeneric() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};


void TestGeneric::TestJson() {
#ifndef __EMSCRIPTEN__
    tString wTitle="Test generic Json";
    tString wSubTitle="simple parse";
    tObj wObj;
    tString wJson="{\"name\":\"John\", \"age\":30, \"car\":null}";
    tBool wResult=wObj.Parse(wJson);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult == true);

    wSubTitle="simple stringify";
    tString wTest=wObj.Stringify();
    wObj.Parse(wTest);
    wTest=wObj.Stringify();
    
    
    //cout << endl << wObj["name"] << endl;
    
    wObj[0]="ALLEZ";
    wObj["value"]=12.23;
    
    //cout << wObj.Stringify() << endl;
    
    //cout << wTest;
    tString wJsonResult="{\"name\":\"John\",\"age\":30,\"car\":null}";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJsonResult == wTest);
    wSubTitle = "array parse elem";
    //wJson ="{\"test\":[1,2,3,4]}";
    //cout << wJson;
    wJson ="{\"test\":[1,2,3,4]}";
    tObj wList;
    wList.Parse(wJson);
    
    wTest=wList.Stringify();
    
    //cout << wTest << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJson == wTest);

    
    wSubTitle = "array parse object";
    wJson="{\"employees\":[\
{\"name\":\"Shyam\",\"email\":\"shyamjaiswal@gmail.com\"},\
{\"name\":\"Bob\",\"email\":\"bob32@gmail.com\"},\
{\"name\":\"Jai\",\"email\":\"jai87@gmail.com\"}\
]}";
    
    tObj wListObject;
    wListObject.Parse(wJson);
    
    wTest=wListObject.Stringify();
    /* Search Error
    cout << wTest << endl;
    cout << wJson << endl;
    for(tInt wInd=0; wInd<wTest.length();wInd++) {
        cout << wTest[wInd] << "=" << wJson[wInd] << endl;
        if (wTest[wInd]!=wJson[wInd]) {
            int a=1;
        }
    }
     */
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJson == wTest);

    //cout << wTest << endl;
#endif
}

class tTestObj : public tObj {
private:
    tString m_String;
    tDate    m_Date;
public:
    
    
    tTestObj() : tObj(),  m_String(""), m_Date() {}
    ~tTestObj() {
    }
    
    tString ClassName() const override { return("tTestObj"); }

  
    void String(tString sString) { m_String = sString; }
    tString String() { return(m_String); }

    void Date(tDate sDate) { m_Date = sDate; }
    tDate Date() { return(m_Date); }

    tObj Sum(tObj sObj) {
        sObj["test"]=1788.78;
        //cout << endl << "------> Call  tTestObj::Sum()" << endl;
        return(sObj);
    }
    
#ifdef _DEBUGSK
    tString Debug() override {
        tStringStream wStringStream;
        tClassDate wDate(m_Date);
        wStringStream << "String:" << m_String << "Date:" << wDate.UsDate();
        return(wStringStream.str());
    }
#endif
};

tTestObj* CreateTestObj() {
    return(new tTestObj());
}

void TestGeneric::TestModelGeneric() {
    tString wTitle = "TestModelGeneric";

    tString wSubTitle = "is Create";

    tModelObj* wModelObj=new tModelObj("tTestObj","Test object", &CreateTestObj);
    
    //tClassFactory::Instance()->Register("tTestObj", &CreateTestObj);
    tClassFactory::Instance()->Register(wModelObj);
    tTestObj* wTestObj = dynamic_cast<tTestObj*>(tClassFactory::Instance()->Create("tTestObj"));

    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestObj != nullptr);

    tModelObj* wClassObj = dynamic_cast<tModelObj*>(tClassFactory::Instance()->Get("tTestObj"));
    tSetterGetter wSetterGetter;
    wSetterGetter.Set<tTestObj>(&tTestObj::String, &tTestObj::String);
    
    wClassObj->AddProperty("String", tVariantType::t_string, "Label", 3, tVariant(""),"", wSetterGetter);
#ifndef __EMSCRIPTEN__
    wSetterGetter.Set<tTestObj>(&tTestObj::Date, &tTestObj::Date);
    wClassObj->AddProperty("Date", tVariantType::t_date, "Date", 4, tVariant(0),"", wSetterGetter);
#endif

    // Test
    tVariant wDate;
    wDate.SetDate(1221);
    
    wClassObj->Property(wTestObj, "String", "coucou");
    wClassObj->Property(wTestObj, "Date", wDate);

    tVariant wResult = wClassObj->Property(wTestObj, "String");
    wSubTitle = "tString";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, (wResult.Type() == tVariantType::t_string) && (wResult.String() == "coucou"));

    // Generic Call
    //tObj (tTestObj::*wCallSum)(tObj);
    tObj wArg;
 
   tString wJson ="{\"array\":[1,2,3,4]}";
    
    wArg.Parse(wJson);
    tVariant wArray=wArg["array"];
    tVirtualClass* wVirtualClass=wArray.Class();
    
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVirtualClass !=nullptr);
    
    (*wTestObj)["Bonjour"]=12;
    wModelObj->AddMethod<tTestObj>("Sum","sum element",&tTestObj::Sum);
    tObj wRes=wTestObj->Call("Sum",wArg);
    
    wTestObj->ObjType("tTestObj");
    wJson=wTestObj->Stringify();
    
     tObj* wObj=tClassFactory::Instance()->CreateByJson(wJson);
    //wObj.Parse(wTestObj->Stringify());
    wJson=wObj->Stringify();
    
    //cout << wArg.Stringify() << endl;
    wRes=wObj->Call("Sum",wArg);
    
    tString wTest=wRes.Stringify();
    wJson="{\"array\":[1,2,3,4],\"test\":1788.78}";
    //cout << wTest << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJson == wTest);

    //cout << wRes.Stringify() << endl;
  
    
    delete(wObj);
    
    delete(wTestObj);
}

void TestGeneric::TestJsonObject() {
    tString wTitle = "TestJsonObject";
    tString wSubTitle = "test 1";
    tJsonObject wJsonObject;
    wJsonObject.Parse("{\"name\":\"John\",\"age\":30}");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJsonObject.IsValid());
    
    wSubTitle = "test 2";
    tJsonObject wJsonObject2;
    wJsonObject2.Root()["name"] = "John";
    wJsonObject2.Root()["age"] = 30;
    wJsonObject2.Root()["city"] = "New York";
    wJsonObject2.Root()["country"] = "USA";
    wJsonObject2.Root()["zip"] = "10001";
    wJsonObject2.Root()["phone"] = "1234567890";
    wJsonObject2.Root()["email"] = "john@example.com";
    wJsonObject2.Root()["website"] = "https://www.example.com";
    wJsonObject2.Root()["company"] = "Example Inc.";
    wJsonObject2.Root()["position"] = "Software Engineer";
    wJsonObject2.Root().Array("test")->Add(1);
    wJsonObject2.Root().Array("test")->Add(2);
    wJsonObject2.Root().Array("test")->Add(3);
    wJsonObject2.Root().Array("test")->Add(4);
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJsonObject2.IsValid());
    
    tString wJson = wJsonObject2.Stringify();
    //cout << wJson << endl;
    tString wTest = R"({"name":"John","age":30,"city":"New York","country":"USA","zip":"10001","phone":"1234567890","email":"john@example.com","website":"https://www.example.com","company":"Example Inc.","position":"Software Engineer","test":[1,2,3,4]})";
   
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJson ==wTest);
    
    // Example: array of classes (objects) with tJsonObject
    // american-english: Demonstrates building an array of object-like tObj items
    tArray* wPeople = wJsonObject2.Root().Array("people");
    // Person 1: Create object on stack to ensure it lives long enough
    // tVariant should make a copy when storing the object
    tObj wPerson1;
    wPerson1["name"] = "Alice";
    wPerson1["age"] = 30;
    wPeople->Add(tVariant(&wPerson1));
    
    // Person 2: Also on stack to ensure it lives long enough
    tObj wPerson2;
    wPerson2["name"] = "Bob";
    wPerson2["age"] = 28;
    wPeople->Add(tVariant(&wPerson2));
    
    // Optional: print resulting JSON
    tString wPeopleJson = wJsonObject2.Stringify();
    //cout << wPeopleJson << endl;
    tJsonObject wJsonObject3;
    wJsonObject3.Parse(wPeopleJson);
    
    wJson=wJsonObject3.Stringify(false);
    wTest=R"({"name":"John","age":30,"city":"New York","country":"USA","zip":"10001","phone":"1234567890","email":"john@example.com","website":"https://www.example.com","company":"Example Inc.","position":"Software Engineer","test":[1,2,3,4],"people":[{"name":"Alice","age":30},{"name":"Bob","age":28}]})";
    wSubTitle = "test 3";
    //cout << wJson << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wJson ==wTest);
}
 

void TestGeneric::setUp() {
	m_Application = tApplication::Instance();
};

void TestGeneric::tearDown() {
    
}



