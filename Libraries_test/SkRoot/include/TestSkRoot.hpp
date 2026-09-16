//==============================================================================
// TestSkRoot
// le 15/09/2021
//==============================================================================
#ifndef TestSkRoot_hpp
#define TestSkRoot_hpp
#include <cppunit/extensions/HelperMacros.h>


#include <SkApplication.hpp>
#include <string.h>

using namespace SkRoot;
 
class TestSkRoot : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRoot);

	CPPUNIT_TEST(TestApplication);
	CPPUNIT_TEST(TestAllocTemporary);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestSkRoot();
private:
	void TestApplication();
	void TestAllocTemporary();

public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */
