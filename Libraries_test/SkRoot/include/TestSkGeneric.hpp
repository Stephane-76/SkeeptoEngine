//==============================================================================
// TestSkGeneric
// le 02/01/2023
//==============================================================================

#ifndef TestSkGeneric_hpp
#define TestSkGeneric_hpp
#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>


using namespace SkRoot;


class TestGeneric : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestGeneric);
	CPPUNIT_TEST(TestJson);
    CPPUNIT_TEST(TestModelGeneric);
    CPPUNIT_TEST(TestJsonObject);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestGeneric();
private:

	void TestJson();
    void TestModelGeneric();
    void TestJsonObject();
public:
	void setUp();
	void tearDown();
};

#endif
