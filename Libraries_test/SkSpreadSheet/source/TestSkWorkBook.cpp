//==============================================================================
// TestSkWorkBook
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkWorkBook.hpp"

#define _printdebug

// We can send it to the API of a feature
TestSkWorkBook::TestSkWorkBook() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
};

tString  TestSkWorkBook::WorkBooksArray(tString sJson) {
    tString wResult="[";
    if (sJson!="") {
        rapidjson::Document wDocument;
        rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());
        
        if (wParseResult) {
            if (wDocument.HasMember("list")) {
                const rapidjson::Value& wValueList = wDocument["list"];
                assert(wValueList.IsArray());
                for (Value::ConstValueIterator wIterator = wValueList.Begin(); wIterator != wValueList.End(); ++wIterator) {
                    const rapidjson::Value& wValue=(*wIterator);
                    tString wUri = wValue["uri"].GetString();
                    if (wResult=="[") {
                        wResult+=wUri;
                    } else {
                        wResult+=","+wUri;
                    }
                }
            }
        }
    }
    wResult+="]";
    return(wResult);
}


void TestSkWorkBook::TestTools() {
	tInt wValue = AlphaToBase10("A");
	tString wResult=Base10ToAlpha(wValue);
	CPPUNIT_ASSERT_MESSAGE("Test AlphaToBase10", wResult=="A");

	wValue = AlphaToBase10("Z");
	wResult = Base10ToAlpha(wValue);
	CPPUNIT_ASSERT_MESSAGE("Test AlphaToBase10", wResult == "Z");

	wValue = AlphaToBase10("ZZ");
	wResult = Base10ToAlpha(wValue);
	CPPUNIT_ASSERT_MESSAGE("Test AlphaToBase10", wResult == "ZZ");

	wValue = AlphaToBase10("ZZZ");
	wResult = Base10ToAlpha(wValue);
	CPPUNIT_ASSERT_MESSAGE("Test AlphaToBase10", wResult == "ZZZ");

	wValue = AlphaToBase10("ABCDEF");
	wResult = Base10ToAlpha(wValue);
	CPPUNIT_ASSERT_MESSAGE("Test AlphaToBase10", wResult == "ABCDEF");
};

tString GetStringVector(tVectorString sVector) {
	tString wResult = "";
	for (auto wSheet : sVector) {
		wResult += wSheet + ";";
	}
	return(wResult);
}

void TestSkWorkBook::TestSheet() {
	// Test Sheet...
	CPPUNIT_ASSERT_MESSAGE("Test Sheet add Sheet1", !m_Api->AddSheet("Sheet1"));
	CPPUNIT_ASSERT_MESSAGE("Test Sheet add Sheet2", m_Api->AddSheet("Sheet2"));
	CPPUNIT_ASSERT_MESSAGE("Test Sheet add Sheet3", m_Api->AddSheet("Sheet3"));

	tVectorString wVectorSheet = m_Api->GetVectorOfSheet();
	CPPUNIT_ASSERT_MESSAGE("Test Sheet Sheet 1,2,3", GetStringVector(wVectorSheet) =="Sheet1;Sheet2;Sheet3;");

	CPPUNIT_ASSERT_MESSAGE("Test Sheet delete Sheet2", m_Api->DeleteSheet("Sheet2"));
	CPPUNIT_ASSERT_MESSAGE("Test Sheet delete Sheet2", !m_Api->DeleteSheet("Sheet2"));

	wVectorSheet = m_Api->GetVectorOfSheet();
	CPPUNIT_ASSERT_MESSAGE("Test Sheet Sheet 1,3", GetStringVector(wVectorSheet) == "Sheet1;Sheet3;");
	CPPUNIT_ASSERT_MESSAGE("TestWorkBook", true);
};

void TestSkWorkBook::TestAddDeleteWorkBook() {
    m_Api->AddWorkBook("www.monsite/monlivre1");
    
    tString wJson=m_Api->JsonWorkBooksList();
#ifdef printdebug
    cout << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Test AddWorkBook " ,wJson=="{\"list\":[\"www.monsite/monlivre1\",\"wwww.skeema.fr/w1\"]}");
    m_Api->AddWorkBook("www.aaa/monlivre1");
    
    wJson=m_Api->JsonWorkBooksList();
#ifdef printdebug
    cout << wJson << endl;
    cout << m_Api->WorkBook("www.aaa/monlivre1")->Uri() << endl;
#endif
    
    m_Api->AddWorkBook("C");
    m_Api->AddWorkBook("A");
    m_Api->AddWorkBook("B");
    
    m_Api->AddWorkBook("B");
    
    tString wList= WorkBooksArray(m_Api->JsonWorkBooks());
    tString wTest="[A,B,C,www.aaa/monlivre1,www.monsite/monlivre1,wwww.skeema.fr/w1]";
    //cout << wList << endl;
    
    CPPUNIT_ASSERT(wList==wTest);
    
    m_Api->DeleteWorkBook("B");
    
    wList= WorkBooksArray(m_Api->JsonWorkBooks());
    wTest="[A,C,www.aaa/monlivre1,www.monsite/monlivre1,wwww.skeema.fr/w1]";
    //cout <<wList << endl;
    CPPUNIT_ASSERT(wList==wTest);
    
    m_Api->DeleteWorkBook("A");
    m_Api->DeleteWorkBook("C");
    
    wList= WorkBooksArray(m_Api->JsonWorkBooks());
    wTest="[www.aaa/monlivre1,www.monsite/monlivre1,wwww.skeema.fr/w1]";
    //cout << wList << endl;
    CPPUNIT_ASSERT(wList==wTest);
    m_Api->DeleteWorkBook("wwww.skeema.fr/w1");
    m_Api->DeleteWorkBook("www.aaa/monlivre1");
    
    wList=WorkBooksArray(m_Api->JsonWorkBooks());
    wTest="[www.monsite/monlivre1]";
    //cout << wList << endl;
    CPPUNIT_ASSERT(wList==wTest);
    
    
    m_Api->DeleteWorkBook("www.monsite/monlivre1");
    wList= WorkBooksArray(m_Api->JsonWorkBooks());
    wTest="[]";
    CPPUNIT_ASSERT(wList==wTest);
    
#ifdef printdebug
   tString wDebug=tSpreadSheetContainer::Instance()->Debug();
    cout << wDebug;
#endif
    
}

void TestSkWorkBook::TestJsonRangeNamed() {
    const tString wPath="www.skeema.fr/RangeNamedExample";
    m_Api->AddWorkBook(wPath);
    m_Api->AddSheet("Sheet1");
    m_Api->UndoInsertRangeNamed("Stéphane Allez", "$A1:$Z255");
    tString wJsonRangeNamed=m_Api->JsonRangeNamed();
    //cout << endl << wJsonRangeNamed << endl;
    
    tString wJsonWorkBook=m_Api->WriteJson(wPath);
    //cout << wJsonWorkBook << endl;
    // Bug see later
    m_Api->DeleteWorkBook(wPath);
#ifdef checksp
    m_Api->Check();
#endif
    
    m_Api->AddWorkBook(wPath);
    
    m_Api->ReadJson(wJsonWorkBook);
    tString wJsonAfter=m_Api->JsonRangeNamed();
    CPPUNIT_ASSERT_MESSAGE("Test WorkBook Info" ,wJsonRangeNamed==wJsonAfter);
}

void TestSkWorkBook::TestInfo() {
    const tString wPath="www.skeema.fr/infoExample";
    m_Api->AddWorkBook(wPath);
    
    tString wJsonInfo="{\"author\":\"sallez\"}";
    m_Api->WorkBookInfo(wJsonInfo);
    
    tString wJson=m_Api->WriteJson(wPath);
    //cout << wJson << endl;
    
    m_Api->ReadJson(wJson);
    
    tWorkBook* wWorkBook=m_Api->WorkBook(wPath);
    
    tString wJsonReturn=wWorkBook->WorkBookInfo();
    tObj wObj;
    wObj.Parse(wJsonReturn);
    //cout << wObj.Debug() << endl;
    tVariant wAuthor=wObj["author"];
    CPPUNIT_ASSERT_MESSAGE("Test WorkBook Info" ,wAuthor.String()=="sallez");
}

void TestSkWorkBook::TestPath() {
    const tString wPath="/Root";
    m_Api->AddWorkBook(wPath);
    
    tString wJsonInfo="{\"author\":\"sallez\"}";
    m_Api->WorkBookInfo(wJsonInfo);
    m_Api->AddSheet("S1");
    m_Api->ActiveSheet("S1");
    m_Api->UndoSizeRow( 1, 3, 12.2);
    
    tString wJson=m_Api->WriteJson(wPath);
    //cout << wJson << endl;
    
    rapidjson::Document wDocument;
    wDocument.Parse(wJson.c_str());
    CPPUNIT_ASSERT(std::string(wDocument["uri"].GetString()) == "/Root");
    CPPUNIT_ASSERT(std::string(wDocument["info"]["author"].GetString()) == "sallez");
    const auto& sheet = wDocument["sheets"][0];
    CPPUNIT_ASSERT(std::string(sheet["name"].GetString()) == "S1");
    const auto& rows = sheet["rows"];
    CPPUNIT_ASSERT(rows.Size() == 3);
    CPPUNIT_ASSERT(fabs(rows[0]["s"].GetDouble() - 12.2) < 1e-9);

    m_Api->ReadJson(wJson);
    
}

void TestSkWorkBook::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkWorkBook::tearDown() {
	delete(m_Api);
}
