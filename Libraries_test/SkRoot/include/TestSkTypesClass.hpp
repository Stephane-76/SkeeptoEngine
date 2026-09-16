//==============================================================================
// TestSkTypeClass
// le 02/01/2023
//==============================================================================
#ifndef TestSkTypesClass_hpp
#define TestSkTypesClass_hpp
#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>


using namespace SkRoot;


class TestTypesClass : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestTypesClass);
	CPPUNIT_TEST(TestClassString);
	CPPUNIT_TEST(TestClassDate);
	CPPUNIT_TEST(TestParseDateTimeLocale);
    CPPUNIT_TEST(TestClassDouble);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestTypesClass();
private:
	void TestClassString();
	void TestClassDate();
	void TestParseDateTimeLocale();
    void TestClassDouble();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */
