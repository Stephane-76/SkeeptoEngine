//=============================================================================
// TestSkConditionalFormat.cpp
// Stéphane Allez *
//  Created on: 21 sept. 2025
//=============================================================================

#include "../include/TestSkConditionalFormat.hpp"


#define _printdebug

TestSkConditionalFormat::TestSkConditionalFormat() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr) {
}


void TestSkConditionalFormat::TestFormatWidthFormula() {
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("A2", 10);
    m_Api->UndoCellValue("A3", 10);
    m_Api->UndoCellValue("A10", 110);
    
    //m_Api->UndoConditionalFormat("formula","A1:A20;A2","% >= 10","color:white;","color:red;");
    m_Api->UndoConditionalFormat("CustomFormulas","A2;B2:B20","% >= 4","color:yellow;","color:green;","","","","","","","");
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->Undo();
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->Redo();
#ifdef printdebug
    cout << wSheet->Debug();
#endif

    
    m_Api->UndoApplyMerge("A1:A20");
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->UndoDeleteRow(2,1);
    //cout << wSheet->Debug();
    //tRect wRect(1, 1, 20, 1);
    //m_Api->UndoDeleteRowByRect(wRect);
 //   m_Api->UndoDeleteCol(1,1);
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->Undo();
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    tRect wRect(1, 1, 20, 2);
    m_Api->UndoDeleteRowByRect(wRect);
 //   m_Api->UndoDeleteCol(1,1);
    //tString wJsonValue=m_Api->WriteJson(m_Api->ActiveWorkBook()->Uri());
    //cout << wJsonValue << endl;;
    
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->Undo();
    
#ifdef printdebug
    cout << wSheet->Debug();
#endif
    m_Api->Undo();
#ifdef printdebug
    cout << wSheet->Debug();
#endif
 
}

void TestSkConditionalFormat::TestConditionalFormatTypes() {
    tSheet* wSheet = m_Api->ActiveSheet();
    
    // Test data
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellValue("A2", 20);
    m_Api->UndoCellValue("A3", 30);
    m_Api->UndoCellValue("B1", "Apple");
    m_Api->UndoCellValue("B2", "Banana");
    m_Api->UndoCellValue("B3", "Cherry");
    
    // Test HighlightCellsRules
    m_Api->UndoConditionalFormat("HighlightCellsRules", "A1:A3", "%>15", "color:red;", "color:green;", "", "", "", "", "", "", "", nullptr);
    tConditionalFormat* wFormat1 = wSheet->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat1 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat1->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat1->Param1(), tString("%>15"));  // Formula
    CPPUNIT_ASSERT_EQUAL(wFormat1->Param2(), tString("color:red;"));   // Format if true
    CPPUNIT_ASSERT_EQUAL(wFormat1->Param3(), tString("color:green;")); // Format if false
    
    m_Api->Undo();
    
    // Test DataBars
    m_Api->UndoConditionalFormat("DataBars", "A1:A3", "color:blue;", "left-to-right", "solid", "0", "100", "", "", "", "", "", nullptr);
    tConditionalFormat* wFormat2 = wSheet->ConditionalFormat(tConditionalFormatType::t_DataBars,"A1:A3");
    CPPUNIT_ASSERT(wFormat2 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat2->Type(), tConditionalFormatType::t_DataBars);
    CPPUNIT_ASSERT_EQUAL(wFormat2->Param1(), tString("color:blue;"));     // Color
    CPPUNIT_ASSERT_EQUAL(wFormat2->Param2(), tString("left-to-right"));   // Direction
    CPPUNIT_ASSERT_EQUAL(wFormat2->Param3(), tString("solid"));           // Style
    CPPUNIT_ASSERT_EQUAL(wFormat2->Param4(), tString("0"));               // Min value
    CPPUNIT_ASSERT_EQUAL(wFormat2->Param5(), tString("100"));             // Max value
  
    m_Api->Undo();
    
    // Test ColorScales
    m_Api->UndoConditionalFormat("ColorScales", "A1:A3", "#FF0000", "#FFFF00", "#00FF00", "0", "50", "100", "", "", "", "", nullptr);
    tConditionalFormat* wFormat3 = wSheet->ConditionalFormat(tConditionalFormatType::t_ColorScales,"A1:A3");
    CPPUNIT_ASSERT(wFormat3 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat3->Type(), tConditionalFormatType::t_ColorScales);
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param1(), tString("#FF0000"));  // Min color
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param2(), tString("#FFFF00"));  // Mid color
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param3(), tString("#00FF00"));  // Max color
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param4(), tString("0"));        // Min value
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param5(), tString("50"));       // Mid value
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param6(), tString("100"));      // Max value
    
    m_Api->Undo();
    
    // Test IconSets
    m_Api->UndoConditionalFormat("IconSets", "A1:A3", "🔴", "🟡", "🟢", "33", "66", "100", "Flags", "", "", "", nullptr);
    tConditionalFormat* wFormat4 = wSheet->ConditionalFormat(tConditionalFormatType::t_IconSets,"A1:A3");
    CPPUNIT_ASSERT(wFormat4 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat4->Type(), tConditionalFormatType::t_IconSets);
    CPPUNIT_ASSERT_EQUAL(wFormat4->IconType(), tIconType::t_Flags);
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param1(), tString("🔴"));  // Icon 1
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param2(), tString("🟡"));  // Icon 2
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param3(), tString("🟢"));  // Icon 3
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param4(), tString("33"));  // Threshold 1
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param5(), tString("66"));  // Threshold 2
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param6(), tString("100")); // Threshold 3
    CPPUNIT_ASSERT_EQUAL(wFormat4->Param7(), tString("Flags")); // IconType
    
    m_Api->Undo();
    
    // Test CustomFormulas
    m_Api->UndoConditionalFormat("CustomFormulas", "B1:B3", "B1=\"Apple\"", "color:purple;", "color:gray;", "", "", "", "", "", "", "", nullptr);
    tConditionalFormat* wFormat5 = wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas,"B1:B3");
    CPPUNIT_ASSERT(wFormat5 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat5->Type(), tConditionalFormatType::t_CustomFormulas);
    CPPUNIT_ASSERT_EQUAL(wFormat5->Param1(), tString("B1=\"Apple\""));  // Formula
    CPPUNIT_ASSERT_EQUAL(wFormat5->Param2(), tString("color:purple;")); // Format if true
    CPPUNIT_ASSERT_EQUAL(wFormat5->Param3(), tString("color:gray;"));   // Format if false
}

void TestSkConditionalFormat::TestConditionalFormatConversions() {
    // Test ConditionalFormatType conversions
    CPPUNIT_ASSERT_EQUAL(tString("None"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_None)));
    CPPUNIT_ASSERT_EQUAL(tString("HighlightCellsRules"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_HighlightCellsRules)));
    CPPUNIT_ASSERT_EQUAL(tString("DataBars"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_DataBars)));
    CPPUNIT_ASSERT_EQUAL(tString("ColorScales"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_ColorScales)));
    CPPUNIT_ASSERT_EQUAL(tString("IconSets"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_IconSets)));
    CPPUNIT_ASSERT_EQUAL(tString("CustomFormulas"), tString(ConditionalFormatTypeToString(tConditionalFormatType::t_CustomFormulas)));
    
    // Test reverse conversions
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_None, StringToConditionalFormatType("None"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_HighlightCellsRules, StringToConditionalFormatType("HighlightCellsRules"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_DataBars, StringToConditionalFormatType("DataBars"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_ColorScales, StringToConditionalFormatType("ColorScales"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_IconSets, StringToConditionalFormatType("IconSets"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_CustomFormulas, StringToConditionalFormatType("CustomFormulas"));
    
    // Test IconType conversions
    CPPUNIT_ASSERT_EQUAL(tString("None"), tString(IconTypeToString(tIconType::t_None)));
    CPPUNIT_ASSERT_EQUAL(tString("Flags"), tString(IconTypeToString(tIconType::t_Flags)));
    CPPUNIT_ASSERT_EQUAL(tString("Arrows"), tString(IconTypeToString(tIconType::t_Arrows)));
    CPPUNIT_ASSERT_EQUAL(tString("Shapes"), tString(IconTypeToString(tIconType::t_Shapes)));
    CPPUNIT_ASSERT_EQUAL(tString("Indicators"), tString(IconTypeToString(tIconType::t_Indicators)));
    CPPUNIT_ASSERT_EQUAL(tString("Ratings"), tString(IconTypeToString(tIconType::t_Ratings)));
    
    // Test reverse IconType conversions
    CPPUNIT_ASSERT_EQUAL(tIconType::t_None, StringToIconType("None"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_Flags, StringToIconType("Flags"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_Arrows, StringToIconType("Arrows"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_Shapes, StringToIconType("Shapes"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_Indicators, StringToIconType("Indicators"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_Ratings, StringToIconType("Ratings"));
    
    // Test invalid strings
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_None, StringToConditionalFormatType("Invalid"));
    CPPUNIT_ASSERT_EQUAL(tConditionalFormatType::t_None, StringToConditionalFormatType(nullptr));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_None, StringToIconType("Invalid"));
    CPPUNIT_ASSERT_EQUAL(tIconType::t_None, StringToIconType(nullptr));
}

void TestSkConditionalFormat::TestConditionalFormatJson() {
    tSheet* wSheet = m_Api->ActiveSheet();
    
    // Setup test data
    m_Api->UndoCellValue("A1", 25);
    m_Api->UndoCellValue("A2", 50);
    m_Api->UndoCellValue("A3", 75);
    
    // Create conditional format
    m_Api->UndoConditionalFormat("HighlightCellsRules", "A1:A3", "%>30", "color:red;", "color:green;", "", "", "", "", "", "", "", nullptr);
    tConditionalFormat* wFormat = wSheet->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat != nullptr);
    
    // Test JSON serialization
    rapidjson::StringBuffer wBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> wWriter(wBuffer);
    wFormat->Json(&wWriter);
    
    tString wJsonString = wBuffer.GetString();
    CPPUNIT_ASSERT(wJsonString.find("\"HighlightCellsRules\"") != std::string::npos);
    CPPUNIT_ASSERT(wJsonString.find("\"color:red;\"") != std::string::npos);
    CPPUNIT_ASSERT(wJsonString.find("\"color:green;\"") != std::string::npos);
    
    // Test JSON deserialization
    rapidjson::Document wDoc;
    wDoc.Parse(wJsonString.c_str());
    CPPUNIT_ASSERT(!wDoc.HasParseError());
    
    tConditionalFormat wNewFormat;
    wNewFormat.Json(wDoc, wSheet);
    
    CPPUNIT_ASSERT_EQUAL(wNewFormat.Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wNewFormat.Param1(), tString("%>30"));        // Formula
    CPPUNIT_ASSERT_EQUAL(wNewFormat.Param2(), tString("color:red;"));  // Format if true
    CPPUNIT_ASSERT_EQUAL(wNewFormat.Param3(), tString("color:green;")); // Format if false
}

void TestSkConditionalFormat::TestConditionalFormatDebug() {
    tSheet* wSheet = m_Api->ActiveSheet();
    
    // Setup test data
    m_Api->UndoCellValue("A1", 100);
    m_Api->UndoCellValue("A2", 200);
    
    // Create conditional format
    m_Api->UndoConditionalFormat("IconSets", "A1:A2", "🔴", "🟡", "🟢", "33", "66", "100", "Arrows", "", "", "", nullptr);
    tConditionalFormat* wFormat = wSheet->ConditionalFormat(tConditionalFormatType::t_IconSets,"A1:A2");
    CPPUNIT_ASSERT(wFormat != nullptr);
    
    // Test debug output
    tString wDebugString = wFormat->Debug();
    //cout << wDebugString;
    CPPUNIT_ASSERT(wDebugString.find("Type:IconSets") != std::string::npos);
    CPPUNIT_ASSERT(wDebugString.find("Icon Set Type:Arrows") != std::string::npos); // Arrows icon type
}

void TestSkConditionalFormat::TestConditionalFormatUndoRedo() {
    tSheet* wSheet = m_Api->ActiveSheet();
    
    // Setup test data
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellValue("A2", 20);
    m_Api->UndoCellValue("A3", 30);
    
    // Create conditional format
    m_Api->UndoConditionalFormat("HighlightCellsRules", "A1:A3", "%>15", "color:red;", "color:blue;", "", "", "", "", "", "", "", nullptr);
    tConditionalFormat* wFormat1 = wSheet->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat1 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat1->Type(), tConditionalFormatType::t_HighlightCellsRules);
    
    // Undo
    m_Api->Undo();
    tConditionalFormat* wFormat2 = wSheet->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat2 == nullptr);
    
    // Redo
    m_Api->Redo();
    tConditionalFormat* wFormat3 = wSheet->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat3 != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat3->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param1(), tString("%>15"));        // Formula
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param2(), tString("color:red;"));  // Format if true
    CPPUNIT_ASSERT_EQUAL(wFormat3->Param3(), tString("color:blue;")); // Format if false
}

void TestSkConditionalFormat::TestConditionalFormatJsonByRect() {
    // Setup test data
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellValue("A2", 20);
    m_Api->UndoCellValue("A3", 30);
    m_Api->UndoCellValue("B1", 40);
    m_Api->UndoCellValue("B2", 50);
    m_Api->UndoCellValue("B3", 60);
    m_Api->UndoCellValue("C1", 70);
    m_Api->UndoCellValue("C2", 80);
    m_Api->UndoCellValue("C3", 90);
    m_Api->UndoCellValue("D1", 100);
    m_Api->UndoCellValue("D2", 110);
    m_Api->UndoCellValue("D3", 120);
    
    // Create multiple conditional formats in different zones
    // Format 1: A1:A3 (HighlightCellsRules)
    m_Api->UndoConditionalFormat("HighlightCellsRules", "A1:A3", "%>15", "color:red;", "color:green;", "", "", "", "", "", "", "", nullptr);
    
    // Format 2: B1:B3 (DataBars)
    m_Api->UndoConditionalFormat("DataBars", "B1:B3", "color:blue;", "left-to-right", "solid", "0", "100", "", "", "", "", "", nullptr);
    
    // Format 3: C1:C3 (ColorScales)
    m_Api->UndoConditionalFormat("ColorScales", "C1:C3", "#FF0000", "#FFFF00", "#00FF00", "", "", "", "", "", "", "", nullptr);
    
    // Format 4: D1:D3 (IconSets)
    m_Api->UndoConditionalFormat("IconSets", "D1:D3", "🔴", "🟡", "🟢", "33", "66", "100", "Flags", "", "", "", nullptr);
    
    // Get all conditional formats (without rectangle)
    tString wJsonAll = m_Api->JsonConditionalFormats(m_Api->ActiveSheet());
    CPPUNIT_ASSERT(!wJsonAll.empty());
    
    // Parse JSON to count formats
    rapidjson::Document wDocAll;
    wDocAll.Parse(wJsonAll.c_str());
    CPPUNIT_ASSERT(!wDocAll.HasParseError());
    CPPUNIT_ASSERT(wDocAll.IsArray());
    CPPUNIT_ASSERT_EQUAL(4, (int)wDocAll.Size()); // Should contain all 4 formats
    
    // Test with rectangle that intersects only A1:B3 (should return formats for A1:A3 and B1:B3)
    tRect wRect1(1, 1, 3, 2); // A1:B3 (row 1-3, col 1-2)
    tString wJsonRect1 = m_Api->JsonConditionalFormats(wRect1);
    CPPUNIT_ASSERT(!wJsonRect1.empty());
    
    rapidjson::Document wDocRect1;
    wDocRect1.Parse(wJsonRect1.c_str());
    CPPUNIT_ASSERT(!wDocRect1.HasParseError());
    CPPUNIT_ASSERT(wDocRect1.IsArray());
    CPPUNIT_ASSERT_EQUAL(2, (int)wDocRect1.Size()); // Should contain only 2 formats
    
    // Verify that the returned formats are the correct ones by checking the key (ref) field
    tBool wFoundA1A3 = false;
    tBool wFoundB1B3 = false;
    for (rapidjson::SizeType i = 0; i < wDocRect1.Size(); i++) {
        const rapidjson::Value& wFormat = wDocRect1[i];
        // Check key field which contains the range reference
        if (wFormat.HasMember("key") && wFormat["key"].IsString()) {
            tString wKey = wFormat["key"].GetString();
            if (wKey.find("A1:A3") != std::string::npos || wKey == "A1:A3") {
                wFoundA1A3 = true;
            } else if (wKey.find("B1:B3") != std::string::npos || wKey == "B1:B3") {
                wFoundB1B3 = true;
            }
        }
    }
    CPPUNIT_ASSERT(wFoundA1A3);
    CPPUNIT_ASSERT(wFoundB1B3);
    
    // Test with rectangle that intersects only C1:D3 (should return formats for C1:C3 and D1:D3)
    tRect wRect2(1, 3, 3, 4); // C1:D3 (row 1-3, col 3-4)
    tString wJsonRect2 = m_Api->JsonConditionalFormats(wRect2);
    CPPUNIT_ASSERT(!wJsonRect2.empty());
    
    rapidjson::Document wDocRect2;
    wDocRect2.Parse(wJsonRect2.c_str());
    CPPUNIT_ASSERT(!wDocRect2.HasParseError());
    CPPUNIT_ASSERT(wDocRect2.IsArray());
    CPPUNIT_ASSERT_EQUAL(2, (int)wDocRect2.Size()); // Should contain only 2 formats
    
    // Verify that the returned formats are the correct ones by checking the key (ref) field
    tBool wFoundC1C3 = false;
    tBool wFoundD1D3 = false;
    for (rapidjson::SizeType i = 0; i < wDocRect2.Size(); i++) {
        const rapidjson::Value& wFormat = wDocRect2[i];
        // Check key field which contains the range reference
        if (wFormat.HasMember("key") && wFormat["key"].IsString()) {
            tString wKey = wFormat["key"].GetString();
            if (wKey.find("C1:C3") != std::string::npos || wKey == "C1:C3") {
                wFoundC1C3 = true;
            } else if (wKey.find("D1:D3") != std::string::npos || wKey == "D1:D3") {
                wFoundD1D3 = true;
            }
        }
    }
    CPPUNIT_ASSERT(wFoundC1C3);
    CPPUNIT_ASSERT(wFoundD1D3);
    
    // Test with rectangle that intersects only A1:A3 (should return only 1 format)
    tRect wRect3(1, 1, 3, 1); // A1:A3 (row 1-3, col 1)
    tString wJsonRect3 = m_Api->JsonConditionalFormats(wRect3);
    CPPUNIT_ASSERT(!wJsonRect3.empty());
    
    rapidjson::Document wDocRect3;
    wDocRect3.Parse(wJsonRect3.c_str());
    CPPUNIT_ASSERT(!wDocRect3.HasParseError());
    CPPUNIT_ASSERT(wDocRect3.IsArray());
    CPPUNIT_ASSERT_EQUAL(1, (int)wDocRect3.Size()); // Should contain only 1 format
    
    // Verify that the returned format is for A1:A3
    CPPUNIT_ASSERT(wDocRect3[0].HasMember("key"));
    CPPUNIT_ASSERT(wDocRect3[0]["key"].IsString());
    tString wKey3 = wDocRect3[0]["key"].GetString();
    CPPUNIT_ASSERT(wKey3.find("A1:A3") != std::string::npos || wKey3 == "A1:A3");
    
    // Test with rectangle that doesn't intersect any format (should return empty array)
    tRect wRect4(10, 10, 12, 12); // E10:F12 (row 10-12, col 5-6)
    tString wJsonRect4 = m_Api->JsonConditionalFormats(wRect4);
    CPPUNIT_ASSERT(!wJsonRect4.empty()); // Should return empty array JSON "[]"
    
    rapidjson::Document wDocRect4;
    wDocRect4.Parse(wJsonRect4.c_str());
    CPPUNIT_ASSERT(!wDocRect4.HasParseError());
    CPPUNIT_ASSERT(wDocRect4.IsArray());
    CPPUNIT_ASSERT_EQUAL(0, (int)wDocRect4.Size()); // Should be empty
}

void TestSkConditionalFormat::TestConditionalFormatDeleteColByRectKeepsRange() {
    // Regression: a Delete Col by rect (single cell E6:E6) only removes E6 and shifts
    // row 6 left. A conditional format spanning far beyond that rect vertically
    // (B4:I28) must keep its columns; it must NOT shrink to B4:H28 as if a whole
    // column had been deleted.
    tSheet* wSheet = m_Api->ActiveSheet();

    for (tInt wRow = 4; wRow <= 12; wRow++) {
        for (tInt wCol = 2; wCol <= 9; wCol++) {
            tCell* wCell = m_Api->EnsureCell(wRow, wCol);
            m_Api->UndoCellValue(wCell->StrRef(), wRow * 100 + wCol);
        }
    }

    m_Api->UndoConditionalFormat("CustomFormulas", "B4:I28", "%>0", "color:red;",
                                 "color:green;", "", "", "", "", "", "", "", nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF created on B4:I28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);

    // Delete Col by rect on E6:E6 -> tRect(Top, Left, Bottom, Right).
    const tRect wRect(6, 5, 6, 5);
    CPPUNIT_ASSERT_MESSAGE("Delete Col by rect E6:E6",
        m_Api->UndoDeleteColByRect(wRect));

    CPPUNIT_ASSERT_MESSAGE("CF range must stay B4:I28 after single-cell col delete",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT shrink to B4:H28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:H28") == nullptr);

    // Undo must restore the CF range to B4:I28 exactly. Regression: the undo re-inserts the
    // column and previously widened the CF range to B4:J28.
    CPPUNIT_ASSERT_MESSAGE("Undo Delete Col by rect E6:E6", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("CF range must be restored to B4:I28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT widen to B4:J28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:J28") == nullptr);
}

void TestSkConditionalFormat::TestConditionalFormatDeleteRowByRectKeepsRange() {
    // Symmetric regression for Delete Row by rect: a single-cell rect (E6:E6) must not
    // shrink a conditional format that extends past the rect horizontally (B4:I28).
    tSheet* wSheet = m_Api->ActiveSheet();

    for (tInt wRow = 4; wRow <= 12; wRow++) {
        for (tInt wCol = 2; wCol <= 9; wCol++) {
            tCell* wCell = m_Api->EnsureCell(wRow, wCol);
            m_Api->UndoCellValue(wCell->StrRef(), wRow * 100 + wCol);
        }
    }

    m_Api->UndoConditionalFormat("CustomFormulas", "B4:I28", "%>0", "color:red;",
                                 "color:green;", "", "", "", "", "", "", "", nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF created on B4:I28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);

    // Delete Row by rect on E6:E6.
    const tRect wRect(6, 5, 6, 5);
    CPPUNIT_ASSERT_MESSAGE("Delete Row by rect E6:E6",
        m_Api->UndoDeleteRowByRect(wRect));

    CPPUNIT_ASSERT_MESSAGE("CF range must stay B4:I28 after single-cell row delete",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT shrink to B4:I27",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I27") == nullptr);

    // Undo must restore the CF range to B4:I28 exactly. Regression: the undo re-inserts the
    // row and previously widened the CF range to B4:I29.
    CPPUNIT_ASSERT_MESSAGE("Undo Delete Row by rect E6:E6", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("CF range must be restored to B4:I28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT widen to B4:I29 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I29") == nullptr);
}

void TestSkConditionalFormat::TestConditionalFormatInsertColByRectKeepsRange() {
    // Regression: an Insert Col by rect (single cell D7:D7) only shifts row 7 to the right.
    // A conditional format spanning far beyond that rect vertically (B4:I28) must keep its
    // columns; it must NOT widen to B4:J28 as if a whole column had been inserted. Excel
    // does not grow a multi-row CF for a single-cell "shift right".
    tSheet* wSheet = m_Api->ActiveSheet();

    for (tInt wRow = 4; wRow <= 12; wRow++) {
        for (tInt wCol = 2; wCol <= 9; wCol++) {
            tCell* wCell = m_Api->EnsureCell(wRow, wCol);
            m_Api->UndoCellValue(wCell->StrRef(), wRow * 100 + wCol);
        }
    }

    m_Api->UndoConditionalFormat("CustomFormulas", "B4:I28", "%>0", "color:red;",
                                 "color:green;", "", "", "", "", "", "", "", nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF created on B4:I28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);

    // Insert Col by rect on D7:D7 -> tRect(Top, Left, Bottom, Right).
    const tRect wRect(7, 4, 7, 4);
    CPPUNIT_ASSERT_MESSAGE("Insert Col by rect D7:D7",
        m_Api->UndoInsertColByRect(wRect));

    CPPUNIT_ASSERT_MESSAGE("CF range must stay B4:I28 after single-cell col insert",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT widen to B4:J28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:J28") == nullptr);

    // Undo must leave the CF range untouched at B4:I28.
    CPPUNIT_ASSERT_MESSAGE("Undo Insert Col by rect D7:D7", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("CF range must still be B4:I28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT be B4:J28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:J28") == nullptr);
}

void TestSkConditionalFormat::TestConditionalFormatInsertRowByRectKeepsRange() {
    // Symmetric regression for Insert Row by rect: a single-cell rect (D7:D7) only shifts
    // column D down. A conditional format extending past the rect horizontally (B4:I28) must
    // NOT grow to B4:I29 as if a whole row had been inserted.
    tSheet* wSheet = m_Api->ActiveSheet();

    for (tInt wRow = 4; wRow <= 12; wRow++) {
        for (tInt wCol = 2; wCol <= 9; wCol++) {
            tCell* wCell = m_Api->EnsureCell(wRow, wCol);
            m_Api->UndoCellValue(wCell->StrRef(), wRow * 100 + wCol);
        }
    }

    m_Api->UndoConditionalFormat("CustomFormulas", "B4:I28", "%>0", "color:red;",
                                 "color:green;", "", "", "", "", "", "", "", nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF created on B4:I28",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);

    // Insert Row by rect on D7:D7.
    const tRect wRect(7, 4, 7, 4);
    CPPUNIT_ASSERT_MESSAGE("Insert Row by rect D7:D7",
        m_Api->UndoInsertRowByRect(wRect));

    CPPUNIT_ASSERT_MESSAGE("CF range must stay B4:I28 after single-cell row insert",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT grow to B4:I29",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I29") == nullptr);

    // Undo must leave the CF range untouched at B4:I28.
    CPPUNIT_ASSERT_MESSAGE("Undo Insert Row by rect D7:D7", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("CF range must still be B4:I28 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I28") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("CF range must NOT be B4:I29 after undo",
        wSheet->ConditionalFormat(tConditionalFormatType::t_CustomFormulas, "B4:I29") == nullptr);
}

void TestSkConditionalFormat::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();

    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");

}

void TestSkConditionalFormat::tearDown() {
    delete m_Api;
}
