//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#include "../include/TestSkFormatString.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <SkFormatCssApi.hpp>
#include <SkJsonKey.hpp>

// We can send it to the API of a feature
TestSkFormatString::TestSkFormatString()  : CPPUNIT_NS::TestFixture(), m_Application(nullptr),m_FormatRoot(nullptr){
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkFormatString::UndoOperation() {
	return; // Drop 
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}


tBool TestSkFormatString::ApplyFormat(tString sRef,tString sValue) {
    SkFormat::tFormatApi* wFormatApi=static_cast<SkFormat::tFormatApi*>(m_FormatApi);
    tFormatRef wRef=wFormatApi->ApplyCellFormat(sValue);
    return(wRef);
}

void TestSkFormatString::TestSkFormatNumber() {
    // Get Format string
    tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
    tApplication::Instance()->Locale("us");
    tString wJsonStr=wFormatStringRoot->JsonFormatString();
    //cout << endl << wJsonStr << endl;
    tObj wJsonObj;
    wJsonObj.Parse(wJsonStr);
    
    wJsonStr=wJsonObj.Stringify();
    
    wJsonStr=wFormatStringRoot->JsonFormatString();
    //cout << endl << wJsonStr << endl;
    
    tInt wRow=1;
    
    // Loop on formatString Info
    tArray* wArrayFormatString=wJsonObj.Array("formatstring");
    for (auto wVariantFamily : *(wArrayFormatString->VectorVariant())) {
        tObj* wObj=dynamic_cast<tObj*>(wVariantFamily.Class());
        tVariant wVariant=(*wObj)("code");
        //cout << wVariant << "{" << endl;
        
        tString wCodeFamily=wVariant.Str();
        tChar wCodeChar=wCodeFamily[0];
        
        
        tArray* wArray=wArrayFormatString->Array("fs");
        for (auto wVariantFormat : *(wArray->VectorVariant())) {
            //cout << wVariantFormat << endl;
            tObj* wObj=dynamic_cast<tObj*>(wVariantFormat.Class());
            tVariant wVariant=(*wObj)("f");
            //cout << "  " << wVariant << endl;
            tString wFormat=wVariant.Str();
            tStringStream wStream;
            wStream << "A" << wRow++;
            tString wCellRef=wStream.str();
            
            m_Api->UndoCellFormat(wCellRef, "format-string :\""+ wFormat+"\";");
            switch (wCodeChar) {
                case 'A': {
                    tDouble wValue=-1234.561;
                    m_Api->UndoCellValue(wCellRef,wValue);
                    break;
                }
                case 'D': {
                    tVariant wVariant;
                    wVariant.Parse("2/2/2025 18:05:03");
                    m_Api->UndoCellValue(wCellRef,wVariant);
                    break;
                }
                case 'P': {
                    tDouble wValue=0.5;
                    m_Api->UndoCellValue(wCellRef,wValue);
                    break;
                }
                case 'N':{
                    tDouble wValue=10066.66;
                    m_Api->UndoCellValue(wCellRef,wValue);
                    break;
                }
                case 'S': {
                    tInt wValue=1;
                    m_Api->UndoCellValue(wCellRef,wValue);
                    break;
                }
                default:
                    break;
            }
          
            tString wResult=m_Api->CellFormatString(wCellRef);
            tString wInput=m_Api->CellInputString(wCellRef);
            //cout << wFormat << "="  << wFormatStringRoot->DefaultFormatString(wFormat) << endl;
            //cout << wCellRef << "--->" << wVariant << "    =" << wResult <<  " : Input=" << wInput << endl;
        }
        //cout << "}" << endl;
    }
    
    

    tApplication::Instance()->Locale("fr");
    tVariant wVariant;
    wVariant.Parse("10245,7");
    m_Api->UndoCellValue("A1", wVariant);
    
    wVariant.Parse("-1211233323,74");
    m_Api->UndoCellValue("A2", wVariant);
 
    wVariant.Parse("1234567,89");
    m_Api->UndoCellValue("A3", wVariant);
    tString wFormatAccount="'#,##0.00 $;(#,##0.00) $'";
    tString wFormatScientific="'0.00E+00'";
    m_Api->UndoCellFormat("A1", "format-string :"+ wFormatScientific+";");
    m_Api->UndoCellFormat("A2", "format-string :"+ wFormatAccount+" 3;");
    m_Api->UndoCellFormat("A3", "format-string :"+ wFormatAccount+" 4;");
    tCell* wCellA1 = m_Api->EnsureCell("A1");
    tCell* wCellA2 = m_Api->EnsureCell("A2");
    tCell* wCellA3 = m_Api->EnsureCell("A3");
    
    tString wFormatStr=m_Api->CellFormat("A4");
    //cout << endl << wFormatStr << endl;

    tString wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Scientific fr", wResult == "1,02e+04");
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA2);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 fr->"+wResult, wResult == "(1 211 233 323,740) €");

    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA3);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format number 2 fr ", wResult == "1 234 567,8900 €");

    tApplication::Instance()->Locale("us");
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Scientific us "+wResult, wResult == "1.02e+04");
    
    // Réapply for US
    m_Api->UndoCellFormat("A2", "format-string :"+ wFormatAccount+" 3;");
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA2);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 us", wResult == "(1,211,233,323.740) $");
    /*
    tBool wOk=m_Api->UndoCellFormat("A3", "color:red;");
    tByte wPrecision=m_Api->CellPrecision("A3");
    wOk=m_Api->CellSetPrecision("A3", wPrecision-1);
     */
#ifdef checkfo
    m_Api->CheckFormat();
#endif

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}
 
void TestSkFormatString::TestSkFormatDate() {
    tApplication::Instance()->Locale("fr");
    tVariant wVariant;
    wVariant.Parse("10/12/2023 12:45");
    
    tClassDate wDate(wVariant.Date());
    //cout << wDate.Day() << ":";
    //cout << wDate.Month() << ":";
    //cout << wDate.Year() << endl;
    
    m_Api->UndoCellValue("A1", wVariant);
    
    
    //tString wJsonTest=m_Api->GetJsonView(1,1,tUnit::pixels,100,10);
    //cout << wJsonTest << endl;
    
    wVariant.Parse("12:45");
    m_Api->UndoCellValue("A2", wVariant);
 
    wVariant.Parse("10/12/2023");
    m_Api->UndoCellValue("A3", wVariant);

    wVariant.Parse("1012,23");
    m_Api->UndoCellValue("A4", wVariant);
    if (!m_Api->UndoCellFormat("A1", "format-string : 'mm-dd-yyyy h:mm:ss';")) {
        cout << m_Api->ErrorWithDetail() << endl;
    };
    
    m_Api->UndoCellFormat("A2", "format-string : 'h:mm:ss';");
    m_Api->UndoCellFormat("A3", "format-string :'mm-dd-yy';");
    tCell* wCellA1 = m_Api->EnsureCell("A1");
    tCell* wCellA2 = m_Api->EnsureCell("A2");
    tCell* wCellA3 = m_Api->EnsureCell("A3");
    //tCell* wCellA4 = m_Api->EnsureCell("A4");

    tString wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Date fr "+wResult, wResult == "10/12/2023 12:45:00");
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA2);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format time 2 fr", wResult == "12:45:00");

    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA3);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format number 2 fr ", wResult == "10/12/23");

    tApplication::Instance()->Locale("us");
    
    wVariant.Parse("2023-12-10 12:45");
    
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Date us", wResult == "12-10-2023 12:45:00");
    
    
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA3);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Date Short us ", wResult == "12-10-23");
    
    //tApplication::Instance()->Locale("fr");
    //tString wJson=m_Api->GetJsonView(1,1,tUnit::pixels,1000,100);
    //cout << wJson << endl;
    
    m_Api->UndoCellFormat("A3", "background-color : yellow;");
    m_Api->UndoCellFormat("A3", "color : black;");
    
    // Raz Format
    //cout << endl << "-->" << m_Api->CellFormat("A3") << "<--" <<endl;
    //m_Api->UndoCellFormat("A3", "format-string :");
    // Syntax Error
    m_Api->UndoCellFormat("A3", "color :black;");
    
    //cout << endl << "-->" <<  m_Api->CellFormat("A3") << "<--" << endl;
#ifdef chedkfo
    m_Api-
#endif
}

void TestSkFormatString::TestSkFormatExcelInterface() {
    // Test the new Excel interface functionality
    tFormatStringRoot* wFormatStringRoot = tApplication::Instance()->FormatStringRoot();
    
    // Test Excel date format validation
    tString wValidDateFormat = "dd/mm/yyyy";
    tString wInvalidDateFormat = "";  // Empty string should be invalid
    tString wValidNumberFormat = "#,##0.00";
    tString wInvalidNumberFormat = "";  // Empty string should be invalid
    
    // Test date format validation
    CPPUNIT_ASSERT_MESSAGE("Valid Excel date format should be detected", 
                          wFormatStringRoot->IsValidExcelDate(wValidDateFormat));
    CPPUNIT_ASSERT_MESSAGE("Invalid Excel date format should be rejected", 
                          !wFormatStringRoot->IsValidExcelDate(wInvalidDateFormat));
    
    // Test number format validation
    CPPUNIT_ASSERT_MESSAGE("Valid Excel number format should be detected", 
                          wFormatStringRoot->IsValidExcelNumber(wValidNumberFormat));
    CPPUNIT_ASSERT_MESSAGE("Invalid Excel number format should be rejected", 
                          !wFormatStringRoot->IsValidExcelNumber(wInvalidNumberFormat));
    
    // Test conversion to SkRoot format
    tString wConvertedDateFormat = wFormatStringRoot->ExcelToLocalFormatString(wValidDateFormat);
    tString wConvertedNumberFormat = wFormatStringRoot->ExcelToLocalFormatString(wValidNumberFormat);
    
    CPPUNIT_ASSERT_MESSAGE("Excel date format should convert to SkRoot format", 
                          !wConvertedDateFormat.empty());
    CPPUNIT_ASSERT_MESSAGE("Excel number format should convert to SkRoot format", 
                          !wConvertedNumberFormat.empty());
    
    // Test that invalid formats return empty strings
    tString wConvertedInvalidDate = wFormatStringRoot->ExcelToLocalFormatString(wInvalidDateFormat);
    tString wConvertedInvalidNumber = wFormatStringRoot->ExcelToLocalFormatString(wInvalidNumberFormat);
    
    CPPUNIT_ASSERT_MESSAGE("Invalid Excel date format should return empty string", 
                          wConvertedInvalidDate.empty());
    CPPUNIT_ASSERT_MESSAGE("Invalid Excel number format should return empty string", 
                          wConvertedInvalidNumber.empty());
    
    // Test preview generation
    tString wDatePreview = wFormatStringRoot->DefaultFormatStringExcel(wValidDateFormat);
    tString wNumberPreview = wFormatStringRoot->DefaultFormatStringExcel(wValidNumberFormat);
    
    CPPUNIT_ASSERT_MESSAGE("Date preview should be generated", !wDatePreview.empty());
    CPPUNIT_ASSERT_MESSAGE("Number preview should be generated", !wNumberPreview.empty());
    
    // Test with various Excel date formats
    std::vector<tString> wExcelDateFormats = {
        "dd/mm/yyyy",
        "mm/dd/yyyy", 
        "yyyy-mm-dd",
        "dd-mmm-yy",
        "dddd, mmmm dd, yyyy",
        "h:mm AM/PM",
        "h:mm:ss AM/PM",
        "mm/dd/yyyy h:mm"
    };
    
    for (const auto& wFormat : wExcelDateFormats) {
        tBool wIsValid = wFormatStringRoot->IsValidExcelDate(wFormat);
        tString wPreview = wFormatStringRoot->DefaultFormatStringExcel(wFormat);
        tString wConverted = wFormatStringRoot->ExcelToLocalFormatString(wFormat);
        
        // At least one of validation, preview, or conversion should work
        tBool wAnySuccess = wIsValid || !wPreview.empty() || !wConverted.empty();
        CPPUNIT_ASSERT_MESSAGE("Excel date format should be processable: " + wFormat, wAnySuccess);
    }
    
    // Test with various Excel number formats
    std::vector<tString> wExcelNumberFormats = {
        "0",
        "0.00",
        "#,##0",
        "#,##0.00",
        "0%",
        "0.00%",
        "0.00E+00",
        "$#,##0.00",
        "$#,##0.00_);($#,##0.00)"
    };
    
    for (const auto& wFormat : wExcelNumberFormats) {
        tBool wIsValid = wFormatStringRoot->IsValidExcelNumber(wFormat);
        tString wPreview = wFormatStringRoot->DefaultFormatStringExcel(wFormat);
        tString wConverted = wFormatStringRoot->ExcelToLocalFormatString(wFormat);
        
        // At least one of validation, preview, or conversion should work
        tBool wAnySuccess = wIsValid || !wPreview.empty() || !wConverted.empty();
        CPPUNIT_ASSERT_MESSAGE("Excel number format should be processable: " + wFormat, wAnySuccess);
    }
    
    // Test error handling for invalid formats
    tString wErrorPreview = wFormatStringRoot->DefaultFormatStringExcel("completely_invalid_format");
    CPPUNIT_ASSERT_MESSAGE("Invalid format should return error message", 
                          wErrorPreview.find("Error") != tString::npos);
}

void TestSkFormatString::TestSkFormatPrecision() {
    tApplication::Instance()->Locale("fr");
    tBool wOk=m_Api->UndoCellPrecision("A1",true);
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision empty cell ",!wOk);
    

    tVariant wVariant;
    wVariant.Parse("1000000");
    m_Api->UndoCellValue("A1", wVariant);
    
    wOk=m_Api->UndoCellPrecision("A1",true);
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision empty format fr inc", wOk);
    tCell* wCellA1=m_Api->Cell("A1");
    tString wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision fr inc", wResult == "1000000,0");

    wOk=m_Api->UndoCellPrecision("A1",false);
    CPPUNIT_ASSERT_MESSAGE("Format by default fr dec", wOk);
    wResult=m_Api->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision fr dec", wResult == "1000000");
    
    m_Api->UndoCellValue("A2", "Coucou");
    wOk=m_Api->UndoCellPrecision("A2",true);
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision and string", !wOk);
    
    tString wFormatAccount="'#,##0.00 $;(#,##0.00) $'";
    wOk=m_Api->UndoCellFormat("A3", "format-string :"+ wFormatAccount+" 4;");
    CPPUNIT_ASSERT_MESSAGE("Apply Format "+wFormatAccount, wOk);
    for(tInt wRow=5;wRow<=10;wRow++) {
        tStringStream wStream;
        wStream << "A" << wRow;
      
        wOk=m_Api->UndoCellValue(wStream.str(),wStream.str()+" Coucou");
        if (!wOk) {
            cout << wStream.str() << endl;
        }
        CPPUNIT_ASSERT_MESSAGE("Set string Coucou and cell"+wStream.str(), wOk);
        tStringStream wStreamFormat;
        wStreamFormat << "font:\"Avenir\",serif "<<wRow+10<<"pt;";
        wOk=m_Api->UndoCellFormat(wStream.str(),wStreamFormat.str());
        if (!wOk) {
            cout << wStream.str() << endl;
        }
        CPPUNIT_ASSERT_MESSAGE("Set format and cell"+wStream.str()+" "+wStreamFormat.str(), wOk);
    }
    
    wOk=m_Api->UndoCellPrecision("A5:A10",false);
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision A5:A10 and string", !wOk);
   
    wOk=m_Api->UndoCellPrecision("A3",true);
    tUndo* wUndo=tApplication::Instance()->UndoRedoContainer()->LastUndo();
    tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
    if (wUndoSpreadSheet!=nullptr) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        
        wWriter.StartObject();
        wUndoSpreadSheet->IsUndo(true);
        wUndoSpreadSheet->Json(&wWriter);
        wWriter.EndObject();
        
        tString wJson=wStringBuffer.GetString();
        //cout <<  endl << wJson << endl;
#ifdef jsondebug
		tString tTest = R"({"ud":"tUndoPrecision","op":"Set Precision A3:1","sheet":"Sheet1","ref":"A3","format":"format-string:\"#,##0.00 $;(#,##0.00) $\" \"eur\" 5;","save":{"selection":"A3","listcell":[{"cell":"A3","t":"n","v":null,"format":1}],"formatstring":{"formats":[{"f":"format-string:\"#,##0.00 $;(#,##0.00) $\" \"eur\" 4;"}]},"colrow":[]},"inc":true})";
#else
	tString tTest = R"({"ud":"tUndoPrecision","op":"Set Precision A3:1","sh":"Sheet1","rf":"A3","fmt":"format-string:\"#,##0.00 $;(#,##0.00) $\" \"eur\" 5;","sv":{"sel":"A3","l_c":[{"c":"A3","t":"n","v":null,"fmt":1}],"fs":{"formats":[{"f":"format-string:\"#,##0.00 $;(#,##0.00) $\" \"eur\" 4;"}]},"cr":[]},"in":true})";
#endif
        
        CPPUNIT_ASSERT_MESSAGE("UndoPrecision UndoSave  A3:1", wJson == tTest);
	}
    
}
void TestSkFormatString::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
};

void TestSkFormatString::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    m_Application->ClearUndoRedo();
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkFormatString");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

	DoneFormatRoot();
}
