//==============================================================================
// TesttFile
// Test library SkRoot file
//==============================================================================

#include "../include/TestSkRootFile.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRootFile);

// We can send it to the API of a feature 
TestSkRootFile::TestSkRootFile():CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};

tString FILECONTENT="Test d'ecriture dans un fichier.";

void TestSkRootFile::TestBufferSave() {
	tFile wFile("TestBufferChar.txt");
	const tChar* wBuffer = FILECONTENT.c_str();
	wFile.SaveBuffer(wBuffer, strlen(wBuffer));
	CPPUNIT_ASSERT_MESSAGE("File don't exist !", wFile.Exist());
}

void TestSkRootFile::TestBufferLoad() {
	tString wTitle = "TestBufferLoad";
	tFile wFile("TestBufferChar.txt");
	const tChar* wBuffer = wFile.LoadAndAllocBuffer();

	const tChar* wRef = FILECONTENT.c_str();
	CPPUNIT_ASSERT_MESSAGE(wTitle, *wBuffer == *wRef);
    tApplication::Instance()->FreeMem((void*)wBuffer);
	wFile.Delete();
	CPPUNIT_ASSERT_MESSAGE(wTitle, !wFile.Exist());
}

void TestSkRootFile::TestStringSave() {
	tFile wFile("TestString.txt");
	wFile.SaveString(FILECONTENT);
	CPPUNIT_ASSERT_MESSAGE("File don't exist !", wFile.Exist());
}
void TestSkRootFile::TestStringLoad() {
	tString wTitle = "TestStringLoad";
	tFile wFile("TestString.txt");
	tString wResult=wFile.LoadString();
	CPPUNIT_ASSERT_MESSAGE(wTitle, wResult == FILECONTENT);
	wFile.Delete();
	CPPUNIT_ASSERT_MESSAGE(wTitle, !wFile.Exist());
}

void TestSkRootFile::TestDirectory() {
    tDirectory wDirectory;
    wDirectory.LoadFile("./","^.*\\.json$");
#ifdef SKER_FILE_DIR
    wDirectory.LoadFile(SKER_FILE_DIR, "^.*\\.sker$");
#endif
    
    for(auto wFile : wDirectory.VectorFile()) {
        //cout << wFile.Directory() + wFile.FileName()  << endl;
    }
}

void TestSkRootFile::setUp() {
	m_Application = tApplication::Instance();
};

void TestSkRootFile::tearDown() {
}
