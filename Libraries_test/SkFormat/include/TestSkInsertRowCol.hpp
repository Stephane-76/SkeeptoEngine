//=============================================================================
// TestSkInsertRowCol.hpp
// Stéphane Allez *
//  Created on: 15 juil. 2026
//=============================================================================

#ifndef TestSkInsertRowCol_hpp
#define TestSkInsertRowCol_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <SkFormatRoot.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

class TestSkInsertRowCol : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkInsertRowCol);
    CPPUNIT_TEST(TestInsertColRow);
    CPPUNIT_TEST(TestInsertColRowRect);
    CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application;
    tFormatRoot* m_FormatRoot;
    tFormatApi* m_FormatApi;
    tApi* m_Api;

    void TestInsertColRow();
    void TestInsertColRowRect();
public:
    TestSkInsertRowCol();

    void setUp() override;
    void tearDown() override;
};

#endif /* TestSkInsertRowCol_hpp */
