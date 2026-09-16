//==============================================================================
// TestSkAllocator
// le 15/09/2021
//==============================================================================
#ifndef TestSkAllocator_hpp
#define TestSkAllocator_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>


using namespace SkRoot;

class tTestAllocator : public tClass {
	public:
		void*    Test1;
		void*	 Test2;
		tInt	 m_Int;
		tDouble m_Double;
		
		tVariant m_Variant1;
		
		tVariant m_Variant2;
		tVariant m_Variant3;
		tVariant m_Variant4;
		tVariant m_Variant5;
		

		tTestAllocator() : tClass(),
							Test1(nullptr),
							Test2(nullptr),
							m_Int(2),
							m_Double(3),
							m_Variant1("Coucou") ,
							m_Variant2(),
							m_Variant3(3),
							m_Variant4(4),
							m_Variant5(5) {


		}
		void Clear() {}
		virtual ~tTestAllocator() {}
		tTestAllocator& operator = (tTestAllocator& sSkTestAllocator) {
			Test1 = sSkTestAllocator.Test1;
			Test2 = sSkTestAllocator.Test2;
			m_Int = sSkTestAllocator.m_Int;
			m_Double = sSkTestAllocator.m_Double;
			m_Variant1 = sSkTestAllocator.m_Variant1;
			m_Variant2 = sSkTestAllocator.m_Variant2;
			m_Variant3 = sSkTestAllocator.m_Variant3;
			m_Variant4 = sSkTestAllocator.m_Variant4;
			m_Variant5 = sSkTestAllocator.m_Variant1;
			return(*this);
		}
		/*
		void Assign(SkTestAllocator& sSkTestAllocator) {
			Test1 = sSkTestAllocator.Test1;
			Test2 = sSkTestAllocator.Test2;
			m_Int = sSkTestAllocator.m_Int;
			m_Double = sSkTestAllocator.m_Double;
			m_Variant1 = sSkTestAllocator.m_Variant1;
		}
		*/
		void Delete() {
		}
};

class TestSkAllocator : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkAllocator);

	CPPUNIT_TEST(TestAllocator);

	CPPUNIT_TEST_SUITE_END();
private:
	typedef tAllocator<tTestAllocator, tAllocatorRef, 10000> SkAllocatorTest;
	SkAllocatorTest m_Allocator;
	tApplication* m_Application;
public:
	TestSkAllocator();
private:
	void TestAllocator();

public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */
