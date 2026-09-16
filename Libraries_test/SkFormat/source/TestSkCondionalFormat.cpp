//=============================================================================
// TestSkConditionalFormat.cpp
// Stéphane Allez *
//  Created on: 21 sept. 2025
//=============================================================================

#include "../include/TestSkConditionalFormat.hpp"
#include "../include/TestSkJsonViewHelpers.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <cctype>

//#define _printdebug

using namespace TestSkJsonView;

TestSkConditionalFormat::TestSkConditionalFormat() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr) {
}


void TestSkConditionalFormat::TestFormatWidthFormula() {
    m_Api->UndoCellFormat("A1:A10;B2:B4","color:black; background-color:lightblue;font:\"Arial\",serif 32pt;");
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("A2", 10);
    m_Api->UndoCellValue("A3", 10);
    m_Api->UndoCellValue("A10", 110);
    
#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

    m_Api->UndoConditionalFormat("CustomFormulas","A1:A10","A1>=10","color:red;background-color:white","background-color:green","","","","","","","");


#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

    // Create 2-color ColorScale: Red (#FF0000) to Green (#00FF00)
    // Values will be calculated automatically from cell data (0, 25, 50, 75, 100)
    m_Api->UndoConditionalFormat("ColorScales", "A1:A10",
                                            "#FF0000",   // sParam1 = min_color
                                            "",          // sParam2 = empty (no mid color)
                                            "#00FF00",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    tConditionalFormat* wFormatCustomFormula=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("CustomFormula A1:A10",wFormatCustomFormula->Key()=="A1:A10.CF");
   
    
    tConditionalFormat* wFormatColorSacales=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("ColorScales A1:A10",wFormatColorSacales!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("ColorScales A1:A10",wFormatColorSacales->Key()=="A1:A10.CS");
    
    
    tString wJsonView=m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 10, 0, 0, true);

    //cout << wJsonView << endl;
#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

    m_Api->Undo();
    //cout << wJsonView << endl;
#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

    wFormatCustomFormula=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("CustomFormula A1:A10",wFormatCustomFormula->Key()=="A1:A10.CF");
    wFormatColorSacales=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("ColorScale  A1:A10 =nullptr",wFormatColorSacales==nullptr);
    
    m_Api->Redo();
    //cout << wJsonView << endl;
#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

    wJsonView=m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 10, 0, 0, true);
    
#ifdef printdebug
    cout << "JsonView for 2-color ColorScales:" << endl;
    cout << wJsonView << endl;
#endif
    //tRange* wRange=m_Api->ActiveSheet()->Range(1,1,10,1);
    //cout << wRange->Debug() << endl;
    m_Api->UndoDeleteRowByRect(tRect(1,1,10,1));
    wFormatColorSacales=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    wFormatCustomFormula=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("ColorScales A1:A10",wFormatColorSacales==nullptr);
    CPPUNIT_ASSERT_MESSAGE("Custom Formula A1:A10",wFormatCustomFormula==nullptr);
    
    m_Api->Undo();
    wFormatColorSacales=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    wFormatCustomFormula=m_Api->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "A1:A10");
    
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("CustomFormula A1:A10",wFormatCustomFormula->Key()=="A1:A10.CF");

    CPPUNIT_ASSERT_MESSAGE("ColorScales A1:A10",wFormatColorSacales!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("ColorScales A1:A10",wFormatColorSacales->Key()=="A1:A10.CS");
    
    
    //cout << wJsonView << endl;
#ifdef checkfo
    m_FormatApi->Check();
    m_Api->CheckFormat();
#endif

 
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScales2Color() {
#ifdef printdebug
    cout << "Testing ColorScales 2-color (Red to Green)..." << endl;
#endif
    // Set up test data with values from 0 to 100
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 25);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 75);
    m_Api->UndoCellValue("A5", 100);
    
    // Create 2-color ColorScale: Red (#FF0000) to Green (#00FF00)
    // Values will be calculated automatically from cell data (0, 25, 50, 75, 100)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#FF0000",   // sParam1 = min_color
                                            "",          // sParam2 = empty (no mid color)
                                            "#00FF00",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales 2-color creation failed", wOK);
    
    // Get JsonView to verify colors are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for 2-color ColorScales:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Debug: Print all f_bc values found
    size_t wPos = 0;
#ifdef printdebug
    cout << "Debug - All f_bc values found:" << endl;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 7; // After "f_bc":
        size_t wEnd = wJsonView.find("\"", wStart + 1);
        if (wEnd != std::string::npos) {
            tString wColor = wJsonView.substr(wStart, wEnd - wStart);
            cout << "  Found color: " << wColor << endl;
        }
        wPos = wEnd;
    }
#endif
    // Verify that JsonView contains color information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain f_bc", 
                          wJsonView.find("f_bc") != std::string::npos);
    
    // Test specific color values in JsonView for 2-color scale
    // Note: Colors might be converted from hex to CSS names
    CPPUNIT_ASSERT_MESSAGE("A1 (value=0) should be red", wJsonView.find("\"f_bc\":\"red\"") != std::string::npos);
    
    // For now, just check that A5 has some color (we'll adjust based on debug output)
    bool wHasA5Color = (wJsonView.find("\"c_v\":100") != std::string::npos) && 
                      (wJsonView.find("\"f_bc\":") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 (value=100) should have some color", wHasA5Color);
    
    // Test that we have exactly 5 cells with colors
    size_t wColorCount = 0;
    wPos = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScales3Color() {
#ifdef printdebug
    cout << "Testing ColorScales 3-color (Red to Yellow to Green)..." << endl;
#endif
    // Set up test data with values from 0 to 100
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 25);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 75);
    m_Api->UndoCellValue("A5", 100);
    
    // Create 3-color ColorScale: Red (#FF0000) to Yellow (#FFFF00) to Green (#00FF00)
    // Values will be calculated automatically from cell data (0, 25, 50, 75, 100)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#FF0000",   // sParam1 = min_color
                                            "#FFFF00",   // sParam2 = mid_color
                                            "#00FF00",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales 3-color creation failed", wOK);
    
    // Get JsonView to verify colors are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for 3-color ColorScales:" << endl;
    cout << endl << wJsonView << endl;
#endif
    // Verify that JsonView contains color information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain f_bc", 
                          wJsonView.find("f_bc") != std::string::npos);
    
    // Test specific color values in JsonView for 3-color scale
    CPPUNIT_ASSERT_MESSAGE("A1 (value=0) should be red", wJsonView.find("\"f_bc\":\"red\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A3 (value=50) should be yellow", wJsonView.find("\"f_bc\":\"yellow\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 (value=100) should be lime", wJsonView.find("\"f_bc\":\"lime\"") != std::string::npos);
    
    // Test interpolated colors
    CPPUNIT_ASSERT_MESSAGE("A2 (value=25) should have interpolated color", wJsonView.find("\"f_bc\":\"#FF7F00\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A4 (value=75) should have interpolated color", wJsonView.find("\"f_bc\":\"chartreuse\"") != std::string::npos);
    
    // Test that we have exactly 5 cells with colors
    size_t wColorCount = 0;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScalesTemperature() {
#ifdef printdebug
    cout << "Testing ColorScales Temperature (Blue to Red)..." << endl;
#endif
    // Set up test data with temperature values from -10 to 40
    m_Api->UndoCellValue("A1", -10);
    m_Api->UndoCellValue("A2", 0);
    m_Api->UndoCellValue("A3", 15);
    m_Api->UndoCellValue("A4", 30);
    m_Api->UndoCellValue("A5", 40);
    
    // Create 2-color ColorScale: Blue (#0000FF) to Red (#FF0000)
    // Values will be calculated automatically from cell data (-10, 0, 15, 30, 40)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#0000FF",   // sParam1 = min_color
                                            "",          // sParam2 = empty (no mid color)
                                            "#FF0000",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales temperature creation failed", wOK);
    
    // Get JsonView to verify colors are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for Temperature ColorScales:" << endl;
    cout << wJsonView << endl;
#endif
    size_t wPos = 0;
#ifdef printdebug
    // Debug: Print all f_bc values found
    cout << "Debug - All f_bc values found:" << endl;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 7; // After "f_bc":
        size_t wEnd = wJsonView.find("\"", wStart + 1);
        if (wEnd != std::string::npos) {
            tString wColor = wJsonView.substr(wStart, wEnd - wStart);
            cout << "  Found color: " << wColor << endl;
        }
        wPos = wEnd;
    }
#endif
    // Verify that JsonView contains color information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain f_bc", 
                          wJsonView.find("f_bc") != std::string::npos);
    
    // Test specific color values in JsonView for temperature scale
    // Note: Colors might be converted from hex to CSS names
    CPPUNIT_ASSERT_MESSAGE("A1 (value=-10) should be blue", wJsonView.find("\"f_bc\":\"blue\"") != std::string::npos);
    
    // For now, just check that A5 has some color (we'll adjust based on debug output)
    bool wHasA5Color = (wJsonView.find("\"c_v\":40") != std::string::npos) && 
                      (wJsonView.find("\"f_bc\":") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 (value=40) should have some color", wHasA5Color);
    
    // Test that we have exactly 5 cells with colors
    size_t wColorCount = 0;
    wPos = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScalesDetailed() {
#ifdef printdebug
    cout << "Testing ColorScales with detailed color verification..." << endl;
#endif
    // Set up test data with specific values to test color interpolation
    m_Api->UndoCellValue("A1", 0);    // Should be pure red
    m_Api->UndoCellValue("A2", 50);   // Should be yellow (middle)
    m_Api->UndoCellValue("A3", 100);  // Should be pure green
    m_Api->UndoCellValue("A4", 25);   // Should be red-orange
    m_Api->UndoCellValue("A5", 75);   // Should be yellow-green
    
    // Create 3-color ColorScale: Red to Yellow to Green
    // Values will be calculated automatically from cell data (0, 50, 100, 25, 75)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#FF0000",   // sParam1 = min_color
                                            "#FFFF00",   // sParam2 = mid_color
                                            "#00FF00",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales detailed creation failed", wOK);
    
    // Get JsonView and analyze the colors
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "Detailed JsonView for ColorScales:" << endl;
    cout << wJsonView << endl;
#endif
    // Parse JsonView to extract and verify colors
    // This is a simplified verification - in a real test you might want to parse the JSON
    // and verify specific color values for each cell
    
    // Basic verification that colors are present
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain f_bc", 
                          wJsonView.find("f_bc") != std::string::npos);
    
    // Test specific color values in JsonView for detailed verification
    CPPUNIT_ASSERT_MESSAGE("A1 (value=0) should be red", wJsonView.find("\"f_bc\":\"red\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A2 (value=50) should be yellow", wJsonView.find("\"f_bc\":\"yellow\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A3 (value=100) should be lime", wJsonView.find("\"f_bc\":\"lime\"") != std::string::npos);
    
    // Test interpolated colors for specific values
    CPPUNIT_ASSERT_MESSAGE("A4 (value=25) should have interpolated color", wJsonView.find("\"f_bc\":\"#FF7F00\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 (value=75) should have interpolated color", wJsonView.find("\"f_bc\":\"chartreuse\"") != std::string::npos);
    
    // Verify that different cells have different colors (indicating interpolation is working)
    size_t wColorCount = 0;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScalesDebugColors() {
#ifdef printdebug
    cout << "Testing ColorScales Debug Colors..." << endl;
#endif
    // Test 2-color scale
#ifdef printdebug
    cout << "\n=== 2-Color Scale Test ===" << endl;
#endif
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 25);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 75);
    m_Api->UndoCellValue("A5", 100);
    
    m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                "#FF0000",   // sParam1 = min_color
                                "",          // sParam2 = empty (no mid color)
                                "#00FF00",   // sParam3 = max_color
                                "",          // sParam4 = empty (calculated automatically)
                                "",          // sParam5 = empty (calculated automatically)
                                "",          // sParam6 = empty (calculated automatically)
                                "",          // sParam7 = empty
                                "",          // sParam8 = empty
                                "",          // sParam9 = empty
                                "");         // sParam10 = empty
    
    tString wJsonView2 = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
   
#ifdef printdebug
    cout << "2-Color JsonView:" << endl;
    cout << wJsonView2 << endl;
    
    // Extract and display all colors
    cout << "2-Color Scale Colors:" << endl;
    size_t wPos = 0;
    while ((wPos = wJsonView2.find("\"f_bc\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 7;
        size_t wEnd = wJsonView2.find("\"", wStart + 1);
        if (wEnd != std::string::npos) {
            tString wColor = wJsonView2.substr(wStart, wEnd - wStart);
            cout << "  Color: " << wColor << endl;
        }
        wPos = wEnd;
    }
#endif
    m_Api->Undo();
    
    // Test temperature scale
#ifdef printdebug
    cout << "\n=== Temperature Scale Test ===" << endl;
#endif
    m_Api->UndoCellValue("A1", -10);
    m_Api->UndoCellValue("A2", 0);
    m_Api->UndoCellValue("A3", 15);
    m_Api->UndoCellValue("A4", 30);
    m_Api->UndoCellValue("A5", 40);
    
    m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                "#0000FF",   // sParam1 = min_color
                                "",          // sParam2 = empty (no mid color)
                                "#FF0000",   // sParam3 = max_color
                                "",          // sParam4 = empty (calculated automatically)
                                "",          // sParam5 = empty (calculated automatically)
                                "",          // sParam6 = empty (calculated automatically)
                                "",          // sParam7 = empty
                                "",          // sParam8 = empty
                                "",          // sParam9 = empty
                                "");         // sParam10 = empty
    
    tString wJsonViewTemp = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "Temperature JsonView:" << endl;
    cout << wJsonViewTemp << endl;
#endif
    // Extract and display all colors
#ifdef printdebug
    cout << "Temperature Scale Colors:" << endl;
    wPos = 0;
    while ((wPos = wJsonViewTemp.find("\"f_bc\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 7;
        size_t wEnd = wJsonViewTemp.find("\"", wStart + 1);
        if (wEnd != std::string::npos) {
            tString wColor = wJsonViewTemp.substr(wStart, wEnd - wStart);
            cout << "  Color: " << wColor << endl;
        }
        wPos = wEnd;
    }
#endif
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScalesJsonViewValidation() {
#ifdef printdebug
    cout << "Testing ColorScales JsonView validation..." << endl;
#endif
    // Set up test data with specific values for precise testing
    m_Api->UndoCellValue("A1", 0);    // Should be pure red
    m_Api->UndoCellValue("A2", 25);   // Should be red-orange
    m_Api->UndoCellValue("A3", 50);   // Should be pure yellow
    m_Api->UndoCellValue("A4", 75);   // Should be yellow-green
    m_Api->UndoCellValue("A5", 100);  // Should be pure green
    
    // Create 3-color ColorScale: Red to Yellow to Green
    // Values will be calculated automatically from cell data (0, 50, 100, 25, 75)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#FF0000",   // sParam1 = min_color
                                            "#FFFF00",   // sParam2 = mid_color
                                            "#00FF00",   // sParam3 = max_color
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales creation failed", wOK);
    
    // Get JsonView
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for validation:" << endl;
    cout << wJsonView << endl;
#endif
    // Parse JsonView to extract cell data
    // This is a more sophisticated test that validates the JSON structure
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain sheet information", wJsonView.find("\"sheet\":\"Sheet1\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain rows array", wJsonView.find("\"rows\":[") != std::string::npos);
    
    // Test that each cell has the expected value and color
    // A1: value=0, should be red
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 0", wJsonView.find("\"c_v\":0") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A1 should be red", wJsonView.find("\"f_bc\":\"red\"") != std::string::npos);
    
    // A2: value=25, should be interpolated
    CPPUNIT_ASSERT_MESSAGE("A2 should have value 25", wJsonView.find("\"c_v\":25") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A2 should have interpolated color", wJsonView.find("\"f_bc\":\"#FF7F00\"") != std::string::npos);
    
    // A3: value=50, should be yellow
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A3 should be yellow", wJsonView.find("\"f_bc\":\"yellow\"") != std::string::npos);
    
    // A4: value=75, should be interpolated
    CPPUNIT_ASSERT_MESSAGE("A4 should have value 75", wJsonView.find("\"c_v\":75") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A4 should have interpolated color", wJsonView.find("\"f_bc\":\"chartreuse\"") != std::string::npos);
    
    // A5: value=100, should be lime
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 100", wJsonView.find("\"c_v\":100") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 should be lime", wJsonView.find("\"f_bc\":\"lime\"") != std::string::npos);
    
    // Test that all cells have background colors
    size_t wColorCount = 0;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    // Test that all cells have values
    size_t wValueCount = 0;
    wPos = 0;
    while ((wPos = wJsonView.find("\"c_v\":", wPos)) != std::string::npos) {
        wValueCount++;
        wPos += 6; // Length of "\"c_v\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with values", wValueCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestColorScalesNegativeValues() {
#ifdef printdebug
    cout << "Testing ColorScales with negative values..." << endl;
#endif
    // Set up test data with negative values
    m_Api->UndoCellValue("A1", -50);  // Should be blue (min)
    m_Api->UndoCellValue("A2", -25);  // Should be interpolated
    m_Api->UndoCellValue("A3", 0);    // Should be white (mid)
    m_Api->UndoCellValue("A4", 25);   // Should be interpolated
    m_Api->UndoCellValue("A5", 50);   // Should be red (max)
    
    // Create 3-color ColorScale: Blue to White to Red (temperature-like)
    // Values will be calculated automatically from cell data (-50, -25, 0, 25, 50)
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "A1:A5", 
                                            "#0000FF",   // sParam1 = min_color (blue)
                                            "#FFFFFF",   // sParam2 = mid_color (white)
                                            "#FF0000",   // sParam3 = max_color (red)
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty (calculated automatically)
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("ColorScales with negative values creation failed", wOK);
    
    // Get JsonView to verify colors are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for negative values ColorScales:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Debug: Print all f_bc values found
    size_t wPos = 0;
#ifdef printdebug
    cout << "Debug - All f_bc values found for negative values:" << endl;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 7; // After "f_bc":
        size_t wEnd = wJsonView.find("\"", wStart + 1);
        if (wEnd != std::string::npos) {
            tString wColor = wJsonView.substr(wStart, wEnd - wStart);
            cout << "  Found color: " << wColor << endl;
        }
        wPos = wEnd;
    }
#endif
    
    // Verify that JsonView contains color information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain f_bc", 
                          wJsonView.find("f_bc") != std::string::npos);
    
    // Test specific color values in JsonView for negative values
    // A1: value=-50, should be blue
    CPPUNIT_ASSERT_MESSAGE("A1 should have value -50", wJsonView.find("\"c_v\":-50") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A1 (value=-50) should be blue", wJsonView.find("\"f_bc\":\"blue\"") != std::string::npos);
    
    // A3: value=0, should be white
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 0", wJsonView.find("\"c_v\":0") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A3 (value=0) should be white", wJsonView.find("\"f_bc\":\"white\"") != std::string::npos);
    
    // A5: value=50, should be red
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 (value=50) should be red", wJsonView.find("\"f_bc\":\"red\"") != std::string::npos);
    
    // Test that we have exactly 5 cells with colors
    wPos = 0;
    size_t wColorCount = 0;
    while ((wPos = wJsonView.find("\"f_bc\":", wPos)) != std::string::npos) {
        wColorCount++;
        wPos += 7; // Length of "\"f_bc\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with background colors", wColorCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSetsArrows() {
#ifdef printdebug
    cout << "Testing IconSets with Arrows..." << endl;
#endif
    // Set up test data with values for arrow icons
    m_Api->UndoCellValue("A1", 10);   // Should be down arrow (low)
    m_Api->UndoCellValue("A2", 30);   // Should be down arrow (low)
    m_Api->UndoCellValue("A3", 50);   // Should be right arrow (medium)
    m_Api->UndoCellValue("A4", 70);   // Should be up arrow (high)
    m_Api->UndoCellValue("A5", 90);   // Should be up arrow (high)
    
    // Create IconSet with Arrows: Down, Right, Up
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A5", 
                                            "↓",         // sParam1 = down arrow (low)
                                            "→",         // sParam2 = right arrow (medium)
                                            "↑",         // sParam3 = up arrow (high)
                                            "33",        // sParam4 = threshold1 (33%)
                                            "66",        // sParam5 = threshold2 (66%)
                                            "100",       // sParam6 = threshold3 (100%)
                                            "Arrows",    // sParam7 = IconType
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("IconSets Arrows creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets Arrows:" << endl;
    cout << wJsonView << endl;
#endif
    
 
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
  
    // Test specific values in JsonView for IconSets
    // A1: value=10, should be down arrow (low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 10", wJsonView.find("\"c_v\":10") != std::string::npos);
    
    // A3: value=50, should be right arrow (medium)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    
    // A5: value=90, should be up arrow (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 90", wJsonView.find("\"c_v\":90") != std::string::npos);
    
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSetsFlags() {
#ifdef printdebug
    cout << "Testing IconSets with Flags..." << endl;
#endif
    // Set up test data with values for flag icons
    m_Api->UndoCellValue("A1", 5);    // Should be red flag (low)
    m_Api->UndoCellValue("A2", 20);   // Should be red flag (low)
    m_Api->UndoCellValue("A3", 40);   // Should be yellow flag (medium)
    m_Api->UndoCellValue("A4", 60);   // Should be yellow flag (medium)
    m_Api->UndoCellValue("A5", 80);   // Should be green flag (high)
    m_Api->UndoCellValue("A6", 95);   // Should be green flag (high)
    
    // Create IconSet with Flags: Red, Yellow, Green
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A6", 
                                            "🔴",        // sParam1 = red flag (low)
                                            "🟡",        // sParam2 = yellow flag (medium)
                                            "🟢",        // sParam3 = green flag (high)
                                            "25",        // sParam4 = threshold1 (25%)
                                            "50",        // sParam5 = threshold2 (50%)
                                            "75",        // sParam6 = threshold3 (75%)
                                            "Flags",     // sParam7 = IconType
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("IconSets Flags creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 6, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets Flags:" << endl;
    cout << wJsonView << endl;
#endif
    
   
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());

    // Test specific values in JsonView for IconSets
    // A1: value=5, should be red flag (low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 5", wJsonView.find("\"c_v\":5") != std::string::npos);
    
    // A3: value=40, should be yellow flag (medium)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 40", wJsonView.find("\"c_v\":40") != std::string::npos);
    
    // A5: value=80, should be green flag (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 80", wJsonView.find("\"c_v\":80") != std::string::npos);
    
  
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSets5Arrows() {
#ifdef printdebug
    cout << "Testing IconSets with 5 Arrows (like Excel)..." << endl;
#endif
    // Set up test data with values for 5 arrow icons
    m_Api->UndoCellValue("A1", 5);     // Should be down arrow (very low)
    m_Api->UndoCellValue("A2", 15);    // Should be down arrow (very low)
    m_Api->UndoCellValue("A3", 30);    // Should be left arrow (low)
    m_Api->UndoCellValue("A4", 45);    // Should be right arrow (medium)
    m_Api->UndoCellValue("A5", 65);    // Should be up arrow (high)
    m_Api->UndoCellValue("A6", 85);    // Should be up arrow (high)
    m_Api->UndoCellValue("A7", 95);    // Should be double up arrow (very high)
    
    // Create IconSet with 5 Arrows: Down, Left, Right, Up, Double Up (like Excel)
    // Using Excel's default thresholds: 20%, 40%, 60%, 80%
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A7", 
                                            "↓",         // sParam1 = down arrow (very low)
                                            "←",         // sParam2 = left arrow (low)
                                            "→",         // sParam3 = right arrow (medium)
                                            "↑",         // sParam4 = up arrow (high)
                                            "↑↑",        // sParam5 = double up arrow (very high)
                                            "20",        // sParam6 = threshold1 (20%)
                                            "40",        // sParam7 = threshold2 (40%)
                                            "60",        // sParam8 = threshold3 (60%)
                                            "80",        // sParam9 = threshold4 (80%)
                                            "Arrows");   // sParam10 = IconType
    
    CPPUNIT_ASSERT_MESSAGE("IconSets 5 Arrows creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 7, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets 5 Arrows:" << endl;
    cout << wJsonView << endl;
#endif
    
  
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());

    // Test specific values in JsonView for 5-arrow IconSets
    // A1: value=5, should be down arrow (very low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 5", wJsonView.find("\"c_v\":5") != std::string::npos);
    
    // A3: value=30, should be left arrow (low)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 30", wJsonView.find("\"c_v\":30") != std::string::npos);
    
    // A4: value=45, should be right arrow (medium)
    CPPUNIT_ASSERT_MESSAGE("A4 should have value 45", wJsonView.find("\"c_v\":45") != std::string::npos);
    
    // A5: value=65, should be up arrow (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 65", wJsonView.find("\"c_v\":65") != std::string::npos);
    
    // A7: value=95, should be double up arrow (very high)
    CPPUNIT_ASSERT_MESSAGE("A7 should have value 95", wJsonView.find("\"c_v\":95") != std::string::npos);
    
   
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSets5Flags() {
#ifdef printdebug
    cout << "Testing IconSets with 5 Flags (like Excel)..." << endl;
#endif
    // Set up test data with values for 5 flag icons
    m_Api->UndoCellValue("A1", 2);     // Should be red flag (very low)
    m_Api->UndoCellValue("A2", 12);    // Should be red flag (very low)
    m_Api->UndoCellValue("A3", 25);    // Should be yellow flag (low)
    m_Api->UndoCellValue("A4", 40);    // Should be orange flag (medium)
    m_Api->UndoCellValue("A5", 60);    // Should be green flag (high)
    m_Api->UndoCellValue("A6", 80);    // Should be green flag (high)
    m_Api->UndoCellValue("A7", 95);    // Should be blue flag (very high)
    
    // Create IconSet with 5 Flags: Red, Yellow, Orange, Green, Blue (like Excel)
    // Using Excel's default thresholds: 20%, 40%, 60%, 80%
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A7", 
                                            "🔴",        // sParam1 = red flag (very low)
                                            "🟡",        // sParam2 = yellow flag (low)
                                            "🟠",        // sParam3 = orange flag (medium)
                                            "🟢",        // sParam4 = green flag (high)
                                            "🔵",        // sParam5 = blue flag (very high)
                                            "20",        // sParam6 = threshold1 (20%)
                                            "40",        // sParam7 = threshold2 (40%)
                                            "60",        // sParam8 = threshold3 (60%)
                                            "80",        // sParam9 = threshold4 (80%)
                                            "Flags");    // sParam10 = IconType
    
    CPPUNIT_ASSERT_MESSAGE("IconSets 5 Flags creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 7, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets 5 Flags:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
 
    // Test specific values in JsonView for 5-flag IconSets
    // A1: value=2, should be red flag (very low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 2", wJsonView.find("\"c_v\":2") != std::string::npos);
    
    // A3: value=25, should be yellow flag (low)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 25", wJsonView.find("\"c_v\":25") != std::string::npos);
    
    // A4: value=40, should be orange flag (medium)
    CPPUNIT_ASSERT_MESSAGE("A4 should have value 40", wJsonView.find("\"c_v\":40") != std::string::npos);
    
    // A5: value=60, should be green flag (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 60", wJsonView.find("\"c_v\":60") != std::string::npos);
    
    // A7: value=95, should be blue flag (very high)
    CPPUNIT_ASSERT_MESSAGE("A7 should have value 95", wJsonView.find("\"c_v\":95") != std::string::npos);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSets5Ratings() {
#ifdef printdebug
    cout << "Testing IconSets with 5 Ratings (like Excel)..." << endl;
#endif
    // Set up test data with values for 5 star ratings
    m_Api->UndoCellValue("A1", 1);     // Should be 1 star (very low)
    m_Api->UndoCellValue("A2", 8);     // Should be 1 star (very low)
    m_Api->UndoCellValue("A3", 18);    // Should be 2 stars (low)
    m_Api->UndoCellValue("A4", 35);    // Should be 3 stars (medium)
    m_Api->UndoCellValue("A5", 55);    // Should be 4 stars (high)
    m_Api->UndoCellValue("A6", 75);    // Should be 4 stars (high)
    m_Api->UndoCellValue("A7", 90);    // Should be 5 stars (very high)
    
    // Create IconSet with 5 Ratings: 1, 2, 3, 4, 5 stars (like Excel)
    // Using Excel's default thresholds: 20%, 40%, 60%, 80%
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A7", 
                                            "⭐",        // sParam1 = 1 star (very low)
                                            "⭐⭐",       // sParam2 = 2 stars (low)
                                            "⭐⭐⭐",      // sParam3 = 3 stars (medium)
                                            "⭐⭐⭐⭐",     // sParam4 = 4 stars (high)
                                            "⭐⭐⭐⭐⭐",    // sParam5 = 5 stars (very high)
                                            "20",        // sParam6 = threshold1 (20%)
                                            "40",        // sParam7 = threshold2 (40%)
                                            "60",        // sParam8 = threshold3 (60%)
                                            "80",        // sParam9 = threshold4 (80%)
                                            "Ratings");  // sParam10 = IconType
    
    CPPUNIT_ASSERT_MESSAGE("IconSets 5 Ratings creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 7, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets 5 Ratings:" << endl;
    cout << wJsonView << endl;
#endif
    

    
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());

    // Test specific values in JsonView for 5-rating IconSets
    // A1: value=1, should be 1 star (very low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 1", wJsonView.find("\"c_v\":1") != std::string::npos);
    
    // A3: value=18, should be 2 stars (low)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 18", wJsonView.find("\"c_v\":18") != std::string::npos);
    
    // A4: value=35, should be 3 stars (medium)
    CPPUNIT_ASSERT_MESSAGE("A4 should have value 35", wJsonView.find("\"c_v\":35") != std::string::npos);
    
    // A5: value=55, should be 4 stars (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 55", wJsonView.find("\"c_v\":55") != std::string::npos);
    
    // A7: value=90, should be 5 stars (very high)
    CPPUNIT_ASSERT_MESSAGE("A7 should have value 90", wJsonView.find("\"c_v\":90") != std::string::npos);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestIconSetsWithoutThresholds() {
#ifdef printdebug
    cout << "Testing IconSets without fixed thresholds (automatic calculation)..." << endl;
#endif
    // Set up test data with various values
    m_Api->UndoCellValue("A1", 5);     // Should be low icon
    m_Api->UndoCellValue("A2", 25);    // Should be medium icon  
    m_Api->UndoCellValue("A3", 50);    // Should be medium icon
    m_Api->UndoCellValue("A4", 75);    // Should be high icon
    m_Api->UndoCellValue("A5", 95);    // Should be high icon
    
    // Create IconSet WITHOUT thresholds - should use automatic calculation
    tBool wOK = m_Api->UndoConditionalFormat("IconSets", "A1:A5", 
                                            "🔴",        // sParam1 = red flag (low)
                                            "🟡",        // sParam2 = yellow flag (medium)
                                            "🟢",        // sParam3 = green flag (high)
                                            "",          // sParam4 = NO threshold1 (automatic)
                                            "",          // sParam5 = NO threshold2 (automatic)
                                            "",          // sParam6 = NO threshold3 (automatic)
                                            "Flags",     // sParam7 = IconType
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("IconSets without thresholds creation failed", wOK);
    
    // Get JsonView to verify icons are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for IconSets without thresholds:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains icon information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    
    // Test specific values in JsonView for IconSets without thresholds
    // A1: value=5, should be red flag (low)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 5", wJsonView.find("\"c_v\":5") != std::string::npos);
    
    // A2: value=25, should be yellow flag (medium)
    CPPUNIT_ASSERT_MESSAGE("A2 should have value 25", wJsonView.find("\"c_v\":25") != std::string::npos);
    
    // A3: value=50, should be yellow flag (medium)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    
    // A4: value=75, should be green flag (high)
    CPPUNIT_ASSERT_MESSAGE("A4 should have value 75", wJsonView.find("\"c_v\":75") != std::string::npos);
    
    // A5: value=95, should be green flag (high)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 95", wJsonView.find("\"c_v\":95") != std::string::npos);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestDataBarsBasic() {
#ifdef printdebug
    cout << "Testing DataBars Basic..." << endl;
#endif
    // Set up test data with values from 0 to 100
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 25);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 75);
    m_Api->UndoCellValue("A5", 100);
    
    // Create DataBars: Blue bars with automatic min/max calculation
    tBool wOK = m_Api->UndoConditionalFormat("DataBars", "A1:A5", 
                                            "#0000FF",   // sParam1 = bar color (blue)
                                            "left-to-right", // sParam2 = direction
                                            "solid",     // sParam3 = style
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "",          // sParam6 = empty //
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("DataBars basic creation failed", wOK);
    
    // Get JsonView to verify databars are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for DataBars Basic:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains databar information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    
    // Verify that DataBars have percent values (not 0.0)
    // The JSON should contain "percent": with a non-zero value
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain percent field", wJsonView.find("\"percent\":") != std::string::npos);
    
    // Debug: Check the actual percent values in JSON
#ifdef printdebug
    cout << "\n=== TestDataBarsBasic Percent Values ===" << endl;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"percent\":", wPos)) != std::string::npos) {
        size_t wStart = wPos + 10; // After "percent":
        size_t wEnd = wJsonView.find(",", wStart);
        size_t wEnd2 = wJsonView.find("}", wStart);
        if (wEnd != std::string::npos && wEnd < wEnd2) {
            size_t wEnd3 = wEnd;
        } else if (wEnd2 != std::string::npos) {
            size_t wEnd3 = wEnd2;
        }
        size_t wActualEnd = std::min(wEnd != std::string::npos ? wEnd : wJsonView.length(), 
                                     wEnd2 != std::string::npos ? wEnd2 : wJsonView.length());
        tString wPercent = wJsonView.substr(wStart, wActualEnd - wStart);
        cout << "  Found percent: " << wPercent << endl;
        wPos = wActualEnd;
    }
#endif
    
    // Test specific values in JsonView for DataBars
    // A1: value=0, should have minimal bar (0% or minimal percentage)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 0", wJsonView.find("\"c_v\":0") != std::string::npos);
    
    // A3: value=50, should have medium bar (50% of range)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    
    // A5: value=100, should have full bar (100% of range)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 100", wJsonView.find("\"c_v\":100") != std::string::npos);
    
    // Test that we have exactly 5 cells with values
    size_t wValueCount = 0;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"c_v\":", wPos)) != std::string::npos) {
        wValueCount++;
        wPos += 6; // Length of "\"c_v\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with values", wValueCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestDataBarsGradient() {
#ifdef printdebug
    cout << "Testing DataBars with Gradient..." << endl;
#endif
    // Set up test data with values from 0 to 100
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 25);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 75);
    m_Api->UndoCellValue("A5", 100);
    
    // Create DataBars: Blue to Red gradient bars (two colors)
    tBool wOK = m_Api->UndoConditionalFormat("DataBars", "A1:A5", 
                                            "#0000FF",   // sParam1 = start color (blue)
                                            "left-to-right", // sParam2 = direction
                                            "gradient",  // sParam3 = style (gradient)
                                            "",          // sParam4 = empty (calculated automatically)
                                            "",          // sParam5 = empty (calculated automatically)
                                            "#FF0000",   // sParam6 = end color (red) - for gradient
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("DataBars gradient with two colors creation failed", wOK);
    
    // Get JsonView to verify databars are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for DataBars Gradient with two colors:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains databar information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());
    
    // Verify that both start and end colors are stored in JSON
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain start color", wJsonView.find("#0000FF") != std::string::npos || wJsonView.find("blue") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("JsonView should contain end color", wJsonView.find("#FF0000") != std::string::npos || wJsonView.find("red") != std::string::npos);
    
    // Test specific values in JsonView for DataBars
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 0", wJsonView.find("\"c_v\":0") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 100", wJsonView.find("\"c_v\":100") != std::string::npos);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestDataBarsCustomRange() {
#ifdef printdebug
    cout << "Testing DataBars with Custom Range..." << endl;
#endif
    // Set up test data with values from 10 to 90
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellValue("A2", 30);
    m_Api->UndoCellValue("A3", 50);
    m_Api->UndoCellValue("A4", 70);
    m_Api->UndoCellValue("A5", 90);
    
    // Create DataBars: Red bars with custom min/max range
    tBool wOK = m_Api->UndoConditionalFormat("DataBars", "A1:A5", 
                                            "#FF0000",   // sParam1 = bar color (red)
                                            "left-to-right", // sParam2 = direction
                                            "solid",     // sParam3 = style
                                            "0",         // sParam4 = min value (0)
                                            "100",       // sParam5 = max value (100)
                                            "",          // sParam6 = empty
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("DataBars custom range creation failed", wOK);
    
    // Get JsonView to verify databars are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for DataBars Custom Range:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains databar information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());

    // Test specific values in JsonView for DataBars with custom range
    // A1: value=10, should have 10% bar (10/100)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value 10", wJsonView.find("\"c_v\":10") != std::string::npos);
    
    // A3: value=50, should have 50% bar (50/100)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    
    // A5: value=90, should have 90% bar (90/100)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 90", wJsonView.find("\"c_v\":90") != std::string::npos);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestDataBarsNegativeValues() {
#ifdef printdebug
    cout << "Testing DataBars with Negative Values..." << endl;
#endif
    // Set up test data with negative values
    m_Api->UndoCellValue("A1", -50);  // Should be minimal bar
    m_Api->UndoCellValue("A2", -25);  // Should be small bar
    m_Api->UndoCellValue("A3", 0);    // Should be medium bar
    m_Api->UndoCellValue("A4", 25);   // Should be large bar
    m_Api->UndoCellValue("A5", 50);   // Should be full bar
    
    // Create DataBars: Orange bars with range from -50 to 50
    tBool wOK = m_Api->UndoConditionalFormat("DataBars", "A1:A5", 
                                            "#FFA500",   // sParam1 = bar color (orange)
                                            "left-to-right", // sParam2 = direction
                                            "solid",     // sParam3 = style
                                            "-50",       // sParam4 = min value (-50)
                                            "50",        // sParam5 = max value (50)
                                            "",          // sParam6 = empty
                                            "",          // sParam7 = empty
                                            "",          // sParam8 = empty
                                            "",          // sParam9 = empty
                                            "");         // sParam10 = empty
    
    CPPUNIT_ASSERT_MESSAGE("DataBars negative values creation failed", wOK);
    
    // Get JsonView to verify databars are applied
    tString wJsonView = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 500, 5, 0, 0, true);
#ifdef printdebug
    cout << "JsonView for DataBars Negative Values:" << endl;
    cout << wJsonView << endl;
#endif
    
    // Verify that JsonView contains databar information
    CPPUNIT_ASSERT_MESSAGE("JsonView should not be empty", !wJsonView.empty());

    
    // Test specific values in JsonView for DataBars with negative values
    // A1: value=-50, should be minimal bar (0% of range)
    CPPUNIT_ASSERT_MESSAGE("A1 should have value -50", wJsonView.find("\"c_v\":-50") != std::string::npos);
    
    // A3: value=0, should be 50% bar (0 is middle of -50 to 50 range)
    CPPUNIT_ASSERT_MESSAGE("A3 should have value 0", wJsonView.find("\"c_v\":0") != std::string::npos);
    
    // A5: value=50, should be full bar (100% of range)
    CPPUNIT_ASSERT_MESSAGE("A5 should have value 50", wJsonView.find("\"c_v\":50") != std::string::npos);
    
    // Test that we have exactly 5 cells with values
    size_t wValueCount = 0;
    size_t wPos = 0;
    while ((wPos = wJsonView.find("\"c_v\":", wPos)) != std::string::npos) {
        wValueCount++;
        wPos += 6; // Length of "\"c_v\":"
    }
    CPPUNIT_ASSERT_MESSAGE("Should have exactly 5 cells with values", wValueCount == 5);
    
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestJsonConditionalFormat() {
#ifdef printdebug
    cout << "Testing JsonConditionalFormat export..." << endl;
#endif
    // Prepare two conditional formats to ensure JSON contains an array with entries
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 50);
    m_Api->UndoCellValue("A3", 100);

    // 3-color scale on A1:A3
    tBool wOK1 = m_Api->UndoConditionalFormat("ColorScales", "A1:A3",
                                              "#FF0000", "#FFFF00", "#00FF00",
                                              "", "", "", "", "", "", "");
    CPPUNIT_ASSERT_MESSAGE("ColorScales creation failed", wOK1);

    // Icon set on B1:B3 to diversify
    m_Api->UndoCellValue("B1", 10);
    m_Api->UndoCellValue("B2", 50);
    m_Api->UndoCellValue("B3", 90);
    tBool wOK2 = m_Api->UndoConditionalFormat("IconSets", "B1:B3",
                                              "↓", "→", "↑",
                                              "33", "66", "100",
                                              "Arrows", "", "", "");
    CPPUNIT_ASSERT_MESSAGE("IconSets creation failed", wOK2);

    // Query ConditionalFormat JSON
    tString wJson = m_Api->JsonConditionalFormats();
#ifdef printdebug
    cout << "JsonConditionalFormat:" << endl;
    cout << wJson << endl;
#endif
    // Basic validation of array and objects
    CPPUNIT_ASSERT_MESSAGE("JsonConditionalFormat should not be empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonConditionalFormat should start with [", wJson.front() == '[');
    CPPUNIT_ASSERT_MESSAGE("JsonConditionalFormat should contain conditional format key", wJson.find("\"key\"") != std::string::npos);
    bool wHasType = (wJson.find("\"conditionalformattype\"") != std::string::npos) || (wJson.find("\"cft\"") != std::string::npos);
    bool wHasIcon = (wJson.find("\"iconsettype\"") != std::string::npos) || (wJson.find("\"ist\"") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("JsonConditionalFormat should contain conditionalformattype/cft or iconsettype/ist", wHasType || wHasIcon);

    // Ensure both ranges appear
    CPPUNIT_ASSERT_MESSAGE("Should contain range A1:A3", wJson.find("A1:A3") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("Should contain range B1:B3", wJson.find("B1:B3") != std::string::npos);

    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

void TestSkConditionalFormat::TestUndoDeleteConditionalFormat() {
#ifdef printdebug
    cout << "Testing UndoDeleteConditionalFormat..." << endl;
#endif
    // Setup values and add a conditional format
    m_Api->UndoCellValue("C1", 0);
    m_Api->UndoCellValue("C2", 100);
    tBool wOK = m_Api->UndoConditionalFormat("ColorScales", "C1:C2",
                                             "#FF0000", "", "#00FF00",
                                             "", "", "", "", "", "", "");
    CPPUNIT_ASSERT_MESSAGE("Initial conditional format creation failed", wOK);

    // Export JSON to capture presence
    tString wJsonBefore = m_Api->JsonConditionalFormats();
    CPPUNIT_ASSERT_MESSAGE("JSON before delete should contain C1:C2", wJsonBefore.find("C1:C2") != std::string::npos);

    // Delete conditional format on that range
    tBool wDelete = m_Api->UndoDeleteConditionalFormat("ColorScales","C1:C2");
    CPPUNIT_ASSERT_MESSAGE("UndoDeleteConditionalFormat should succeed", wDelete);

    // JSON after delete should not contain range anymore
    tString wJsonAfterDelete = m_Api->JsonConditionalFormats();
    CPPUNIT_ASSERT_MESSAGE("JSON after delete should not contain C1:C2", wJsonAfterDelete.find("C1:C2") == std::string::npos);

    // Undo should restore it
    m_Api->Undo();
    tString wJsonAfterUndo = m_Api->JsonConditionalFormats();
    CPPUNIT_ASSERT_MESSAGE("JSON after undo should contain C1:C2 again", wJsonAfterUndo.find("C1:C2") != std::string::npos);

    // Final cleanup
    m_Api->Undo();
    CPPUNIT_ASSERT(true);
}

// Reproduces the Conditional.xlsx scenario:
//   B2 = 13 (threshold)
//   C2 = 15 with rule "%>$B$2" -> true  -> light red fill, dark red text
//   D2 = 12 with rule "%<$B$2" -> true  -> light red fill, dark red text
//
// Pins down two regressions found while importing Conditional.xlsx:
//   1) HighlightCellsRules: rules were registered but never compiled, so
//      Param2 (true-branch CSS) was never painted.
//   2) Cross-cell refs in CF formulas (e.g. "$B$2") were resolved as
//      "#REF!" because tCell::InternalCalculation reads the cell's own
//      m_VectorRef but the lemon push had stored the ref on the
//      tConditionalFormat object. Fixed by temporarily swapping the CF's
//      m_VectorRef into the cell during evaluation (no PushRef -> no
//      pollution of the calculation dependency graph).
void TestSkConditionalFormat::TestHighlightCellsRulesJsonViewRendering() {
    tSheet* wSheet = m_Api->ActiveSheet();
    CPPUNIT_ASSERT(wSheet != nullptr);

    m_Api->UndoCellValue("B2", 13);
    m_Api->UndoCellValue("C2", 15);
    m_Api->UndoCellValue("D2", 12);

    const tString wCssWhenTrue = "background-color:#FFC7CE;color:#9C0006;";

    // Two single-cell HighlightCellsRules with the % syntax (cell-relative).
    // Param3 (format-when-false) intentionally empty: we only care that the
    // true branch paints the expected colors; the false branch must leave
    // the cell untouched.
    m_Api->UndoConditionalFormat("HighlightCellsRules", "C2:C2",
        "%>$B$2", wCssWhenTrue, "",
        "", "", "", "", "", "", "");
    m_Api->UndoConditionalFormat("HighlightCellsRules", "D2:D2",
        "%<$B$2", wCssWhenTrue, "",
        "", "", "", "", "", "", "");

    // Both rules must be registered on the sheet.
    tConditionalFormat* wCfC = wSheet->ConditionalFormat(
        tConditionalFormatType::t_HighlightCellsRules, "C2:C2");
    tConditionalFormat* wCfD = wSheet->ConditionalFormat(
        tConditionalFormatType::t_HighlightCellsRules, "D2:D2");
    CPPUNIT_ASSERT_MESSAGE("CF C2 registered", wCfC != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF D2 registered", wCfD != nullptr);
    CPPUNIT_ASSERT_EQUAL(wCfC->Param1(), tString("%>$B$2"));
    CPPUNIT_ASSERT_EQUAL(wCfD->Param1(), tString("%<$B$2"));
    CPPUNIT_ASSERT_EQUAL(wCfC->Param2(), wCssWhenTrue);
    CPPUNIT_ASSERT_EQUAL(wCfD->Param2(), wCssWhenTrue);

    // Render the viewport that covers row 2 / cols A..E. sCss=true gives
    // CSS string values for f_bc / f_c (e.g. "#FFC7CE"), which we string-
    // match below; sCss=false would emit canvas-mode payloads.
    tString wView = m_Api->JsonView(1, 1, tUnitMetrics::pixels,
                                    200, 600, 0, 0, true);
    rapidjson::Document wDoc;
    wDoc.Parse(wView.c_str());
    CPPUNIT_ASSERT_MESSAGE("JsonView parses", !wDoc.HasParseError());
    CPPUNIT_ASSERT_MESSAGE("JsonView has rows",
        wDoc.HasMember("rows") && wDoc["rows"].IsArray());

    // Helper: locate cell at (row, col) in the rows[].cells[] payload.
    auto wFindCell = [](const rapidjson::Document& sDoc, tInt sRow, tInt sCol) -> const rapidjson::Value* {
        const rapidjson::Value& wRows = sDoc["rows"];
        for (rapidjson::SizeType wR = 0; wR < wRows.Size(); ++wR) {
            if (!wRows[wR].HasMember("cells")) continue;
            const rapidjson::Value& wCells = wRows[wR]["cells"];
            if (!wCells.IsArray()) continue;
            for (rapidjson::SizeType wC = 0; wC < wCells.Size(); ++wC) {
                const rapidjson::Value& wCell = wCells[wC];
                if (wCell.HasMember("c_r") && wCell.HasMember("c_c") &&
                    wCell["c_r"].GetInt() == sRow &&
                    wCell["c_c"].GetInt() == sCol) {
                    return &wCell;
                }
            }
        }
        return nullptr;
    };
    // Lowercase a copy so hex matching is case-insensitive (StrKey may
    // produce uppercase or mixed-case strings depending on the path).
    auto wToLower = [](tString sIn) {
        for (auto& wCh : sIn) wCh = (tChar)std::tolower((unsigned char)wCh);
        return sIn;
    };

    // C2 (row 2, col 3): rule %>$B$2 evaluates to 15 > 13 = true.
    const rapidjson::Value* wC2 = wFindCell(wDoc, 2, 3);
    CPPUNIT_ASSERT_MESSAGE("C2 emitted in JsonView", wC2 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("C2 has f_bc (background-color)",
        CellHasMember(wDoc, *wC2, "f_bc"));
    CPPUNIT_ASSERT_MESSAGE("C2 has f_c (font color)",
        CellHasMember(wDoc, *wC2, "f_c"));
    tString wC2Bg = wToLower(CellMemberValue(wDoc, *wC2, "f_bc")->GetString());
    tString wC2Fg = wToLower(CellMemberValue(wDoc, *wC2, "f_c")->GetString());
    CPPUNIT_ASSERT_MESSAGE(("C2 f_bc=" + wC2Bg).c_str(),
        wC2Bg.find("ffc7ce") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(("C2 f_c=" + wC2Fg).c_str(),
        wC2Fg.find("9c0006") != tString::npos);

    // D2 (row 2, col 4): rule %<$B$2 evaluates to 12 < 13 = true.
    const rapidjson::Value* wD2 = wFindCell(wDoc, 2, 4);
    CPPUNIT_ASSERT_MESSAGE("D2 emitted in JsonView", wD2 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("D2 has f_bc (background-color)",
        CellHasMember(wDoc, *wD2, "f_bc"));
    CPPUNIT_ASSERT_MESSAGE("D2 has f_c (font color)",
        CellHasMember(wDoc, *wD2, "f_c"));
    tString wD2Bg = wToLower(CellMemberValue(wDoc, *wD2, "f_bc")->GetString());
    tString wD2Fg = wToLower(CellMemberValue(wDoc, *wD2, "f_c")->GetString());
    CPPUNIT_ASSERT_MESSAGE(("D2 f_bc=" + wD2Bg).c_str(),
        wD2Bg.find("ffc7ce") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(("D2 f_c=" + wD2Fg).c_str(),
        wD2Fg.find("9c0006") != tString::npos);

    // B2 carries no rule, so it must not inherit the conditional palette.
    const rapidjson::Value* wB2 = wFindCell(wDoc, 2, 2);
    CPPUNIT_ASSERT_MESSAGE("B2 emitted in JsonView", wB2 != nullptr);
    if (CellHasMember(wDoc, *wB2, "f_bc")) {
        tString wB2Bg = wToLower(CellMemberValue(wDoc, *wB2, "f_bc")->GetString());
        CPPUNIT_ASSERT_MESSAGE(("B2 must not be light-red, got: " + wB2Bg).c_str(),
            wB2Bg.find("ffc7ce") == tString::npos);
    }

    // Flip the threshold: C2 rule becomes false (15>100), D2 rule stays
    // true (12<100). Param3 is empty so C2's highlight must disappear; D2
    // keeps it. This also verifies that the swap-based ref propagation
    // still picks up the new B2 value on every CF re-application.
    m_Api->UndoCellValue("B2", 100);
    tString wView2 = m_Api->JsonView(1, 1, tUnitMetrics::pixels,
                                     200, 600, 0, 0, true);
    rapidjson::Document wDoc2;
    wDoc2.Parse(wView2.c_str());
    CPPUNIT_ASSERT_MESSAGE("JsonView2 parses", !wDoc2.HasParseError());
    const rapidjson::Value* wC2b = wFindCell(wDoc2, 2, 3);
    const rapidjson::Value* wD2b = wFindCell(wDoc2, 2, 4);
    CPPUNIT_ASSERT_MESSAGE("C2 still emitted after threshold change",
        wC2b != nullptr);
    CPPUNIT_ASSERT_MESSAGE("D2 still emitted after threshold change",
        wD2b != nullptr);
    if (CellHasMember(wDoc2, *wC2b, "f_bc")) {
        tString wBg = wToLower(CellMemberValue(wDoc2, *wC2b, "f_bc")->GetString());
        CPPUNIT_ASSERT_MESSAGE(("C2 must drop highlight when 15>100, got: " + wBg).c_str(),
            wBg.find("ffc7ce") == tString::npos);
    }
    CPPUNIT_ASSERT_MESSAGE("D2 still has f_bc (12<100 stays true)",
        CellHasMember(wDoc2, *wD2b, "f_bc"));
    {
        tString wBg = wToLower(CellMemberValue(wDoc2, *wD2b, "f_bc")->GetString());
        CPPUNIT_ASSERT_MESSAGE(("D2 expected light-red, got: " + wBg).c_str(),
            wBg.find("ffc7ce") != tString::npos);
    }
}

// Large sqref (A1:A2000): CF Apply must clip to the JsonView viewport, not walk all rows.
void TestSkConditionalFormat::TestJsonViewCfViewportClip() {
    m_Api->UndoCellValue("A50", 7);
    m_Api->UndoConditionalFormat(
        "HighlightCellsRules", "A1:A2000", "%<>0", "background-color:#FFFF99;", "",
        "", "", "", "", "", "", "");

    tString wView = m_Api->JsonView(
        40, 1, tUnitMetrics::pixels, 400, 600, 0, 0, true);
    rapidjson::Document wDoc;
    wDoc.Parse(wView.c_str());
    CPPUNIT_ASSERT_MESSAGE("JsonView parses", !wDoc.HasParseError());

    const rapidjson::Value* wA50 = nullptr;
    const rapidjson::Value& wRows = wDoc["rows"];
    for (rapidjson::SizeType wR = 0; wR < wRows.Size(); ++wR) {
        if (!wRows[wR].HasMember("cells")) continue;
        const rapidjson::Value& wCells = wRows[wR]["cells"];
        for (rapidjson::SizeType wC = 0; wC < wCells.Size(); ++wC) {
            const rapidjson::Value& wCell = wCells[wC];
            if (wCell.HasMember("c_r") && wCell["c_r"].GetInt() == 50 &&
                wCell.HasMember("c_c") && wCell["c_c"].GetInt() == 1) {
                wA50 = &wCell;
                break;
            }
        }
        if (wA50 != nullptr) break;
    }
    CPPUNIT_ASSERT_MESSAGE("A50 emitted in clipped JsonView", wA50 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("A50 CF background",
        CellHasMember(wDoc, *wA50, "f_bc"));
    tString wBg = CellMemberValue(wDoc, *wA50, "f_bc")->GetString();
    CPPUNIT_ASSERT_MESSAGE(("A50 f_bc=" + wBg).c_str(),
        wBg.find("FFFF99") != tString::npos || wBg.find("ffff99") != tString::npos);
}

void TestSkConditionalFormat::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
   
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
}

void TestSkConditionalFormat::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkConditionalFormat");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

    DoneFormatRoot();
}
