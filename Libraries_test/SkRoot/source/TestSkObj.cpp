//==============================================================================
// TestSkTables
// Test library SkTables
//==============================================================================

#include "../include/TestSkObj.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkObj);

// We can send it to the API of a feature 
TestSkObj::TestSkObj() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};

void TestSkObj::TestParse() {
#ifndef __EMSCRIPTEN__
    // Portable JSON: DataModel.json exists on this Mac but not on Windows,
    // and its root key is "Name" (not "name"). Always assert the known payload.
    tObj wObj;
    tString wJson = "{\"name\":\"test\",\"value\":1}";
    CPPUNIT_ASSERT(wObj.Parse(wJson));
    CPPUNIT_ASSERT(wObj.HasProperty("name"));
    CPPUNIT_ASSERT(wObj.HasPath("name"));
    CPPUNIT_ASSERT(wObj.Get("name").String() == "test");
    CPPUNIT_ASSERT(wObj.HasProperty("value"));
    CPPUNIT_ASSERT(wObj.Get("value").Int() == 1);

    tFile wFile("/Users/stephaneallez/Projects/library/libraries_test/SkFileTest/MetaModel/DataModel.json");
    if (wFile.Exist()) {
        tObj wModel;
        CPPUNIT_ASSERT(wModel.Parse(wFile.LoadString()));
        CPPUNIT_ASSERT(wModel.HasProperty("Name"));
        CPPUNIT_ASSERT(wModel.HasProperty("Tables"));
    }
#endif
}


void TestSkObj::TestPath() {
    tObj wObj;
    
    // Test SetPath - Create nested structure
    wObj.SetPath("user.name", tVariant("John"));
    wObj.SetPath("user.age", tVariant(30));
    wObj.SetPath("user.address.city", tVariant("Paris"));
    wObj.SetPath("user.address.zip", tVariant(75001));
    wObj.SetPath("user.active", tVariant(true));
    
    // Test GetPath - Read values
    tVariant wName = wObj.GetPath("user.name");
    CPPUNIT_ASSERT(!wName.IsNull());
    CPPUNIT_ASSERT(wName.IsString());
    CPPUNIT_ASSERT(wName.String() == "John");
    
    tVariant wAge = wObj.GetPath("user.age");
    CPPUNIT_ASSERT(!wAge.IsNull());
    CPPUNIT_ASSERT(wAge.IsInt());
    CPPUNIT_ASSERT(wAge.Int() == 30);
    
    tVariant wCity = wObj.GetPath("user.address.city");
    CPPUNIT_ASSERT(!wCity.IsNull());
    CPPUNIT_ASSERT(wCity.IsString());
    CPPUNIT_ASSERT(wCity.String() == "Paris");
    
    tVariant wZip = wObj.GetPath("user.address.zip");
    CPPUNIT_ASSERT(!wZip.IsNull());
    CPPUNIT_ASSERT(wZip.IsInt());
    CPPUNIT_ASSERT(wZip.Int() == 75001);
    
    tVariant wActive = wObj.GetPath("user.active");
    CPPUNIT_ASSERT(!wActive.IsNull());
    CPPUNIT_ASSERT(wActive.IsBool());
    CPPUNIT_ASSERT(wActive.Bool() == true);
    
    // Test HasPath - Check if paths exist
    CPPUNIT_ASSERT(wObj.HasPath("user.name") == true);
    CPPUNIT_ASSERT(wObj.HasPath("user.age") == true);
    CPPUNIT_ASSERT(wObj.HasPath("user.address.city") == true);
    CPPUNIT_ASSERT(wObj.HasPath("user.address.zip") == true);
    CPPUNIT_ASSERT(wObj.HasPath("user.nonexistent") == false);
    CPPUNIT_ASSERT(wObj.HasPath("nonexistent.path") == false);
    
    // Test GetPathObj - Get nested object
    tObj* wUserObj = wObj.GetPathObj("user");
    CPPUNIT_ASSERT(wUserObj != nullptr);
    CPPUNIT_ASSERT(wUserObj->HasProperty("name"));
    CPPUNIT_ASSERT(wUserObj->HasProperty("age"));
    
    tObj* wAddressObj = wObj.GetPathObj("user.address");
    CPPUNIT_ASSERT(wAddressObj != nullptr);
    CPPUNIT_ASSERT(wAddressObj->HasProperty("city"));
    CPPUNIT_ASSERT(wAddressObj->HasProperty("zip"));
    
    // Test GetPathObj with non-existent path
    tObj* wNonExistentObj = wObj.GetPathObj("nonexistent.path");
    CPPUNIT_ASSERT(wNonExistentObj == nullptr);
    
    // Test SetPath - Update existing value
    wObj.SetPath("user.age", tVariant(31));
    tVariant wUpdatedAge = wObj.GetPath("user.age");
    CPPUNIT_ASSERT(wUpdatedAge.Int() == 31);
    
    // Test SetPath - Create array path
    tArray* wArray = new tArray();
    wArray->Add(tVariant(1));
    wArray->Add(tVariant(2));
    wArray->Add(tVariant(3));
    tVariant wArrayVariant;
    wArrayVariant.SetClass(wArray);
    wObj.SetPath("user.scores", wArrayVariant);
    
    // Test GetPathArray
    tArray* wRetrievedArray = wObj.GetPathArray("user.scores");
    CPPUNIT_ASSERT(wRetrievedArray != nullptr);
    CPPUNIT_ASSERT(wRetrievedArray->Length() == 3);
    CPPUNIT_ASSERT(wRetrievedArray->operator()(0).Int() == 1);
    CPPUNIT_ASSERT(wRetrievedArray->operator()(1).Int() == 2);
    CPPUNIT_ASSERT(wRetrievedArray->operator()(2).Int() == 3);
    
    //cout << wUserObj->Stringify() << endl;
    // Test GetPathArray with non-existent path
    tArray* wNonExistentArray = wObj.GetPathArray("nonexistent.array");
    CPPUNIT_ASSERT(wNonExistentArray == nullptr);
    
    // Test GetPathArrayValue with invalid index (out of bounds)
    tVariant wInvalidIndex = wObj.GetPathArrayValue("user.scores", 10); // Index > Length
    CPPUNIT_ASSERT(wInvalidIndex.IsNull());
    
    // Test GetPathArrayValue with invalid index (negative would be unsigned, so test with very large number)
    tVariant wLargeIndex = wObj.GetPathArrayValue("user.scores", 999999);
    CPPUNIT_ASSERT(wLargeIndex.IsNull());
    
    // Test SetPathArrayValue with invalid index
    tBool wSetResult = wObj.SetPathArrayValue("user.scores", 10, tVariant(99)); // Index > Length
    CPPUNIT_ASSERT(wSetResult == false);
    
    // Test SetPathArrayValue with non-existent array path
    tBool wSetResult2 = wObj.SetPathArrayValue("nonexistent.array", 0, tVariant(99));
    CPPUNIT_ASSERT(wSetResult2 == false);
    
    // Test GetPathArrayValue with non-existent array path
    tVariant wNonExistentValue = wObj.GetPathArrayValue("nonexistent.array", 0);
    CPPUNIT_ASSERT(wNonExistentValue.IsNull());
    
    // Test GetPath with non-existent path
    tVariant wNonExistent = wObj.GetPath("nonexistent.path");
    CPPUNIT_ASSERT(wNonExistent.IsNull());
    
    // Test complex nested structure
    wObj.SetPath("company.department.team.leader.name", tVariant("Alice"));
    wObj.SetPath("company.department.team.leader.age", tVariant(35));
    
    tVariant wLeaderName = wObj.GetPath("company.department.team.leader.name");
    CPPUNIT_ASSERT(!wLeaderName.IsNull());
    CPPUNIT_ASSERT(wLeaderName.String() == "Alice");
    
    CPPUNIT_ASSERT(wObj.HasPath("company.department.team.leader.name") == true);
    CPPUNIT_ASSERT(wObj.HasPath("company.department.team.leader.age") == true);
    
    //cout << "TestPath: All tests passed!" << endl;
}

// Helper class for testing Call method
class TestObjWithMethod : public tObj {
public:
    TestObjWithMethod() : tObj() {}
    
    // Custom method that doubles a numeric value
    tObj DoubleValue(tObj sArg) {
        tObj wResult;
        if (sArg.HasPath("value")) {
            tVariant wValue = sArg.GetPath("value");
            if (wValue.IsInt()) {
                wResult.SetPath("result", tVariant(wValue.Int() * 2));
            } else if (wValue.IsDouble()) {
                wResult.SetPath("result", tVariant(wValue.Double() * 2.0));
            }
        }
        return wResult;
    }
    
    // Custom method that adds two numbers
    tObj Add(tObj sArg) {
        tObj wResult;
        tVariant wA = sArg.GetPath("a");
        tVariant wB = sArg.GetPath("b");
        if (wA.IsInt() && wB.IsInt()) {
            wResult.SetPath("sum", tVariant(wA.Int() + wB.Int()));
        }
        return wResult;
    }
};

void TestSkObj::TestCall() {
    // Test Call with non-existent method - should return argument unchanged
    tObj wObj;
    tObj wArg;
    wArg.SetPath("test.value", tVariant(42));
    
    tObj wResult = wObj.Call("nonexistentMethod", wArg);
    
    // When method doesn't exist, Call returns the argument unchanged
    CPPUNIT_ASSERT(wResult.HasPath("test.value"));
    CPPUNIT_ASSERT(wResult.GetPath("test.value").Int() == 42);
    
    // Test Call with empty argument
    tObj wEmptyArg;
    tObj wResult2 = wObj.Call("anotherMethod", wEmptyArg);
    CPPUNIT_ASSERT(wResult2.Length() == 0);
    
    // Test Call with argument containing multiple properties
    tObj wComplexArg;
    wComplexArg.SetPath("a", tVariant(1));
    wComplexArg.SetPath("b", tVariant("test"));
    wComplexArg.SetPath("c.nested", tVariant(3.14));
    
    tObj wResult3 = wObj.Call("unknownMethod", wComplexArg);
    CPPUNIT_ASSERT(wResult3.HasPath("a"));
    CPPUNIT_ASSERT(wResult3.HasPath("b"));
    CPPUNIT_ASSERT(wResult3.HasPath("c.nested"));
    CPPUNIT_ASSERT(wResult3.GetPath("a").Int() == 1);
    CPPUNIT_ASSERT(wResult3.GetPath("b").String() == "test");
    CPPUNIT_ASSERT(wResult3.GetPath("c.nested").Double() == 3.14);
    
    // Test Call with registered method
    // Create a model with methods
    tModelObj* wModel = new tModelObj("TestObjWithMethod", "Test Object with Methods", 
        []() -> tVirtualClass* { return new TestObjWithMethod(); });
    
    // Add methods to the model
    wModel->AddMethod<TestObjWithMethod>("DoubleValue", "Double a numeric value", 
        &TestObjWithMethod::DoubleValue);
    wModel->AddMethod<TestObjWithMethod>("Add", "Add two numbers", 
        &TestObjWithMethod::Add);
    
    // Register the model in factory
    tClassFactory::Instance()->Register(wModel);
    
    // Create an instance and set its model
    TestObjWithMethod* wTestObj = new TestObjWithMethod();
    wTestObj->ModelObj(wModel);
    
    // Test DoubleValue method
    tObj wDoubleArg;
    wDoubleArg.SetPath("value", tVariant(5));
    tObj wDoubleResult = wTestObj->Call("DoubleValue", wDoubleArg);
    CPPUNIT_ASSERT(wDoubleResult.HasPath("result"));
    CPPUNIT_ASSERT(wDoubleResult.GetPath("result").Int() == 10);
    
    // Test Add method
    tObj wAddArg;
    wAddArg.SetPath("a", tVariant(3));
    wAddArg.SetPath("b", tVariant(7));
    tObj wAddResult = wTestObj->Call("Add", wAddArg);
    CPPUNIT_ASSERT(wAddResult.HasPath("sum"));
    CPPUNIT_ASSERT(wAddResult.GetPath("sum").Int() == 10);
    
    // Test with non-existent method on registered object
    tObj wUnknownResult = wTestObj->Call("UnknownMethod", wArg);
    CPPUNIT_ASSERT(wUnknownResult.HasPath("test.value"));
    
    delete wTestObj;
    
    //cout << "TestCall: All tests passed!" << endl;
}

void TestSkObj::setUp() {
    m_Application = tApplication::Instance();
}


void TestSkObj::tearDown() {
}
