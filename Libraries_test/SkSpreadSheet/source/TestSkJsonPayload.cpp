#include "../include/TestSkJsonPayload.hpp"

// We can send it to the API of a feature 
TestSkJsonPayload::TestSkJsonPayload() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}


void TestSkJsonPayload::TestJsonPayload() {

    tString wTest = R"({"ID":"#FF4569221"})";
    m_Api->UndoJsonPayload("A1", wTest);

    tString wResult=m_Api->CellJsonPayload("A1");
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("TestJsonPayload ", m_Api->CellJsonPayload("A1")==wTest);
}


void TestSkJsonPayload::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkJsonPayload::tearDown() {
    delete(m_Api);
}
