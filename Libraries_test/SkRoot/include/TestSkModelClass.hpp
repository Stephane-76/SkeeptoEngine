//==============================================================================
// TestSkModelClass
// le 02/01/2023
//==============================================================================
#ifndef TestSkTypesClass_hpp
#define TestSkTypesClass_hpp
#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>


using namespace SkRoot;


class TestModelClass : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestModelClass);
	CPPUNIT_TEST(TestCreateInstance);
	CPPUNIT_TEST(TestProperty);
	CPPUNIT_TEST(TestJson);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestModelClass();
private:
	void TestCreateInstance();
	void TestProperty();
	void TestJson();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */
