//=============================================================================
// TestSkConditionalFormat.hpp
// Stéphane Allez *
//  Created on: 21 sept. 2025
//=============================================================================

#ifndef TestSkConditionalFormat_hpp
#define TestSkConditionalFormat_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkConditionalFormat : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkConditionalFormat);
    CPPUNIT_TEST(TestFormatWidthFormula);
    CPPUNIT_TEST(TestConditionalFormatTypes);
    CPPUNIT_TEST(TestConditionalFormatConversions);
    CPPUNIT_TEST(TestConditionalFormatJson);
    CPPUNIT_TEST(TestConditionalFormatDebug);
    CPPUNIT_TEST(TestConditionalFormatUndoRedo);
    CPPUNIT_TEST(TestConditionalFormatJsonByRect);
    CPPUNIT_TEST(TestConditionalFormatDeleteColByRectKeepsRange);
    CPPUNIT_TEST(TestConditionalFormatDeleteRowByRectKeepsRange);
    CPPUNIT_TEST(TestConditionalFormatInsertColByRectKeepsRange);
    CPPUNIT_TEST(TestConditionalFormatInsertRowByRectKeepsRange);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    tApi* m_Api;
public:
    TestSkConditionalFormat();
private:
   void  TestFormatWidthFormula();
   void  TestConditionalFormatTypes();
   void  TestConditionalFormatConversions();
   void  TestConditionalFormatJson();
   void  TestConditionalFormatDebug();
   void  TestConditionalFormatUndoRedo();
   void  TestConditionalFormatJsonByRect();
   void  TestConditionalFormatDeleteColByRectKeepsRange();
   void  TestConditionalFormatDeleteRowByRectKeepsRange();
   void  TestConditionalFormatInsertColByRectKeepsRange();
   void  TestConditionalFormatInsertRowByRectKeepsRange();
public:
   void setUp() override;
   void tearDown() override;
};

#endif // TestSkConditionalFormat_hpp
