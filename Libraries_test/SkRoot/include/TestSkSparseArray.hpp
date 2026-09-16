//==============================================================================
// TestSkSparseArray
// le 15/09/2021
//==============================================================================
#ifndef TestSkSparseArray_hpp
#define TestSkSparseArray_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkSparseArray.hpp>

using namespace SkRoot;

class TestSkSparseArray : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkSparseArray);

	CPPUNIT_TEST(TestSparseArray);
	CPPUNIT_TEST(TestSparseArrayPt);

	CPPUNIT_TEST(TestSparseArrayInsertErase);
	CPPUNIT_TEST(TestSparseArrayPtInsertErase);
	CPPUNIT_TEST(TestSparseArrayCallBackOccupied);

	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestSkSparseArray();
private:
	void TestSparseArray();
	void TestSparseArrayPt();

	void TestSparseArrayInsertErase();
	void TestSparseArrayPtInsertErase();
	void TestSparseArrayCallBackOccupied();

public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkSparseArray */
