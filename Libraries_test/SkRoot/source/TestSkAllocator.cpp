//==============================================================================
// TestSkFile
// Test library SkAllocator
//==============================================================================

#include "../include/TestSkAllocator.hpp"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkAllocator);

// We can send it to the API of a feature 
TestSkAllocator::TestSkAllocator() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};


void TestSkAllocator::TestAllocator() {
#ifdef _DEBUG
	tInt wNb = 1000;
	tInt wModulo = wNb / 10;
#else 
	tInt wNb = 5000;
	tInt wModulo = wNb / 10;
#endif
	tInt wIndex = 0;

	//cout << "sizeof(SkTestAllocator)=" << sizeof(SkTestAllocator) << endl;
	tTestAllocator* wNew = new tTestAllocator();
	for (int wInd = 0; wInd < wNb; wInd++) {
		tClass* wClass;
		tie(wIndex,wClass) = m_Allocator.Alloc();
        if (wInd % wModulo==0) {
            //cout << "." << wInd << endl;
        }
	}
	
	tTestAllocator* wTestAllocator1= m_Allocator(4);
	wTestAllocator1->m_Variant1 = "Coucou St�phane";
	
	tTestAllocator* wTestAllocator2 = m_Allocator(4);

	CPPUNIT_ASSERT_MESSAGE("Variant equality ", wTestAllocator2->m_Variant1.String()== "Coucou St�phane");
	
	m_Allocator.Delete(8);
	for (int wInd = 1; wInd < 2000; wInd++) {
		int wIndex = rand() % 2000 ;
        if (!m_Allocator.IsNullptr(wIndex)) {
			m_Allocator.Delete(wIndex);
		}
	}
	tInt wIndex2; 
	tClass* wClass;
	tie (wIndex2,wClass) = m_Allocator.Alloc();
	for (int wInd = 0; wInd < wNb; wInd++) {
        if (!m_Allocator.IsNullptr(wInd)) {
            m_Allocator.Delete(wInd);
        }
		//if (i % wModulo==0) cout << ". " << i << endl;
	}

 	delete(wNew);
}

void TestSkAllocator::setUp() {
	m_Application = tApplication::Instance();
};

void TestSkAllocator::tearDown() {
}
