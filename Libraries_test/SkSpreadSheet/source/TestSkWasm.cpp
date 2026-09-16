//==================================================================================================
//  File      : TestSkWasm.cpp
//  Date      : 2026/02/03
//  Author    : brwill
//==================================================================================================
#include "../include/TestSkWasm.hpp"

TestSkWasm::TestSkWasm() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
    m_Application = nullptr;
    m_Api = nullptr;
}

void TestSkWasm::TestJsonWasm() {
    m_Api->AddWorkBook("www.stephane.fr");
    m_Api->DeleteSheet("Sheet1");
    m_Api->AddSheet("Stéphane");
    m_Api->AddSheet("Allez");
    tObj wObj;
    //wObj.Parse(m_Api->WorkBookInfo());
    wObj.SetPath("author", tVariant("Stéphane Allez"));
    wObj.SetPath("email",tVariant("sallez@yuyu.com"));
    tClassDate wDate(tClassDate::Now());
    tVariant wVariantNow;
    wVariantNow.SetDate(wDate.Value());
    wObj.SetPath("date-creation", wVariantNow);
    wObj.SetPath("date-modification", wVariantNow);
    wObj.SetPath("comment", tVariant("commentaire ancun"));
    
    tClassDate wClassDateCreation(wVariantNow.Date());
    CPPUNIT_ASSERT_MESSAGE("Test Date 32 bits",wClassDateCreation.UsDate()==wDate.UsDate());
    
    tString wJsonInfo=wObj.Stringify();
    m_Api->WorkBookInfo(wJsonInfo);
    
    tString wJson=m_Api->WriteJson("www.stephane.fr");
    //cout << endl << wJson << endl;
    // Reset
    tearDown();
    setUp();
    
    m_Api->ReadJson(wJson);
    tString wJsonLoad=m_Api->WriteJson("www.stephane.fr");
    
    tString wJsonInfoLoad=m_Api->WorkBookInfo();
    tObj wObjInfoLoad;
    wObjInfoLoad.Parse(wJsonInfoLoad);
    tVariant wVariantLoadDate=wObjInfoLoad.GetPath("date-creation");
    tClassDate wLoadDate;
    wLoadDate.UsDate(wVariantLoadDate.String());
    //cout << wLoadDate.UsDate() << endl;
    
       //wClassDateCreation.UsDate(wDateCreation.String());
    CPPUNIT_ASSERT_MESSAGE("Test Date 32 bits",wLoadDate.UsDate()==wDate.UsDate());
    
    //cout << wJsonLoad << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestJsonWasm",
                           wJson==wJsonLoad);    

    m_Api->AddWorkBook("www.thomas.fr");
    tString wJsonWorkBookList=m_Api->JsonWorkBooksList();
    
    //cout << wJsonWorkBookList << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestJsonWorkBookList",
                           wJsonWorkBookList.find("www.thomas.fr") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestJsonWorkBookList",
                           wJsonWorkBookList.find("www.stephane.fr") != tString::npos);
    
    
    
 }


void TestSkWasm::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_Api = new tApi;
}

void TestSkWasm::tearDown() {
    delete(m_Api);
}
