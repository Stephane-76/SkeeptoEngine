//=============================================================================
// TestSkJsonPayload
//=============================================================================
#ifndef TestSkJsonPayload_hpp
#define TestSkJsonPayload_hpp


#include <stdio.h>


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Exception.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>


using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkJsonPayload : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkJsonPayload);
    CPPUNIT_TEST(TestJsonPayload);
    CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

public:
	TestSkJsonPayload();
private:
	void TestJsonPayload();
public:
	void setUp();
	void tearDown();
};

#endif /* TestSkJsonPayload_hpp */
