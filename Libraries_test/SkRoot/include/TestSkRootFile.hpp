//==============================================================================
// TestSkRootFile
// 15/09/2021
//==============================================================================
#ifndef TestSkRootFile_hpp
#define TestSkRootFile_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
 
using namespace SkRoot;

class TestSkRootFile : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRootFile);

	CPPUNIT_TEST(TestBufferSave);
	CPPUNIT_TEST(TestBufferLoad);

	CPPUNIT_TEST(TestStringSave);
	CPPUNIT_TEST(TestStringLoad);
    CPPUNIT_TEST(TestDirectory);

	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestSkRootFile();
private:
	void TestBufferSave();
	void TestBufferLoad();

	void TestStringSave();
	void TestStringLoad();
    
    void TestDirectory();

public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */
