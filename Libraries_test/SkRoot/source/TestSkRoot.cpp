//==============================================================================
// TestSkRoot
// Test library SkRoot
//==============================================================================

#include "../include/TestSkRoot.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRoot);

// We can send it to the API of a feature 
TestSkRoot::TestSkRoot():CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};

class tClassTest : public tVirtualClass {
public:
	tClassTest(tString sValue) : tVirtualClass(), m_Value(sValue) {}

	tString m_Value;
};

void TestSkRoot::TestApplication() {
	tClassString wEtiquette;
	tString wTitle = "TestSkApplication";
	tString wSubTitle = "Global variable";

	m_Application->AddGlobal("Test",new tVirtualClass());
	tVirtualClass* wVirtualClass = m_Application->Global("Test");
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVirtualClass!=nullptr);
	m_Application->DeleteGlobal("Test");
	wVirtualClass = m_Application->Global("Test");
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wVirtualClass == nullptr);

	wSubTitle = "Global variable derive";
	tClassTest* wClassTest = new tClassTest("Coucou");
	m_Application->AddGlobal("Test_1", wClassTest);
	tClassTest* wResult = dynamic_cast<tClassTest*>(m_Application->Global("Test_1"));
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult->m_Value=="Coucou");


}

void TestSkRoot::TestAllocTemporary() {
	tString wTitle = "TestAllocTemporary";
	tString wSubTitle = "Alloc char";
	// This loop allocates more than 1 GB to cause an overflow
	for (int i = 0; i < 15000; i++) {
		tStringStream wStringStream;
		wStringStream << "Hello alloc temporary : "<< i << "width long char* ====================== ";
		tString wString = wStringStream.str();
		tChar* wChar = (tChar*)m_Application->AllocTemporary(wString.length() + 1);
#ifdef _M_X64
		strcpy_s(wChar, wString.length() + 1, wString.c_str());
#else
		strcpy(wChar, wString.c_str());
#endif // !__EMSCRIPTEN__
		CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, *wChar == *wString.c_str());
	}
};
void TestSkRoot::setUp() {
	m_Application = tApplication::Instance();
};

void TestSkRoot::tearDown() {
}

