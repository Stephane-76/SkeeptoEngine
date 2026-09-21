
#include "../include/SkUISpreadSheetApi.hpp"
#include "../include/CallJson.hpp"

using namespace SkRoot;
using namespace SkFormat;
using namespace SkSpreadSheet;
#ifndef __EMSCRIPTEN__
int main() {
	std::cout << "SkReactSpreadSheet version 1.0" << endl;
	tUISpreadSheet* wUISpreadSheet = new tUISpreadSheet();
    wUISpreadSheet->IsUndoActif(true);
    tBool wOk;

    // For Debug
    tFormatApi* wFormatApi=wUISpreadSheet->FormatApi();
    // Bug Border
    wUISpreadSheet->_Border("C2:C2",5,"dashed black 3px;","Sheet1");
    
    wUISpreadSheet->_Value("A1", "12122121,31","Sheet1");
    wUISpreadSheet->_ApplyFormatString("A1:A4","\"#,##0.00 $;(#,##0.00) $\"","Sheet1");
    
    cout << wUISpreadSheet->CellFormatString("A1");
    cout << wUISpreadSheet->_GetInputValue("A1","Sheet1");
    

#ifdef _DEBUGSK
    cout << wFormatApi->Debug();
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif
#endif
  
#ifdef _DEBUGSK
    cout << wFormatApi->Debug();
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif  
#endif
    wUISpreadSheet->_Border("D5:E7;J6:O9",5,"solid yellow 3px;","Sheet1");
#ifdef _DEBUGSK
    cout << wFormatApi->Debug();
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif
#endif

    // Bug Border
    wUISpreadSheet->_Border("C2:E12;J18:O22",5,"dashed black 3px;","Sheet1");

    wUISpreadSheet->_Border("A20:G20",5,"solid red 2px;","Sheet1");
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif
    wUISpreadSheet->_Border("A12:Z22",4,"solid yellow 3px;","Sheet1");
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif

    wUISpreadSheet->_Border("A1:A1",1,"solid red 3px;","Sheet1");


#ifdef _DEBUGSK
    cout << wFormatApi->Debug();
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif
#endif
    wUISpreadSheet->_Undo();
    wUISpreadSheet->_Undo();
    wUISpreadSheet->_Undo();
    wUISpreadSheet->_Undo();
    wUISpreadSheet->_Undo();
    
    wUISpreadSheet->_Redo();
    wUISpreadSheet->_Redo();
    wUISpreadSheet->_Redo();
    wUISpreadSheet->_Redo();
    wUISpreadSheet->_Redo();
    
    wOk=wUISpreadSheet->_DeleteCol(1, 2,"Sheet1");
    if (wOk) {
        cout << "Delete Col Ok" << endl;
    }
    wUISpreadSheet->_Undo();
    
    //wUISpreadSheet->_Format("A1:A3", "background-color:blue;");
    
    wUISpreadSheet->_RegisterClassAttribute("JavaScriptObj","Javascript Object","Javascript");
    wUISpreadSheet->_AddProperty("Name", "string", "label", 1, "Name Default");
    wUISpreadSheet->_AddProperty("value", "int", "label", 2, "");
    wOk=wUISpreadSheet->_AddProperty("date", "date", "label", 3, "12/12/2024");
    cout << wOk << endl;
    wUISpreadSheet->_CellClass("A1","JavaScriptObj");
    
    //wUISpreadSheet->_ValueAttribute("A1", "Name", "Coucou !")
    
    cout << wUISpreadSheet->_JsonCellClass();
    
    wUISpreadSheet->_ValueAttribute("A1", "value", "=A2+1","Sheet1");
    std::cout << wUISpreadSheet->_GetValue("A1","Shee1") << endl;
  
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "Name","Sheet1") << endl;
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "value","Sheet1") << endl;
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "date","Sheet1") << endl;
    
	wUISpreadSheet->_SizeRow(16,1, 30,"Sheet1");
	wUISpreadSheet->_SizeCol(7,1, 20,"Sheet1");
	wUISpreadSheet->_ValueInt("A2", 2,"Sheet1");
	wUISpreadSheet->_ValueInt("A3", 20,"Sheet1");
	wUISpreadSheet->_ValueInt("A4", 200,"Sheet1");

	wUISpreadSheet->_ValueString("A6","=SUM(A1:A5)","Sheet1");
	wUISpreadSheet->_ValueString("A7", "Coucou Stephane","Sheet1");

	wUISpreadSheet->_ValueInt("B2", 2,"Sheet1");
	wUISpreadSheet->_ValueInt("C2", 20,"Sheet1");
	wUISpreadSheet->_ValueInt("D2", 200,"Sheet1");
    //wUISpreadSheet->_Format("A1:A3", "background-color:blue;");
    wUISpreadSheet->_Border("A1:A3",5,"1px solid blue;","Sheet1");

    wUISpreadSheet->_ValueInt("E2", 2,"Sheet1");
    wUISpreadSheet->_ValueInt("E3", 20,"Sheet1");
    wUISpreadSheet->_ValueInt("E4", 200,"Sheet1");
    
    wUISpreadSheet->_AddFunction("FUNCT","function test","Javascript", 2);
    wUISpreadSheet->_ValueString("A10","=FUNCT(A4;A1:A7)","Sheet1");
    
    wUISpreadSheet->_Merge("A3:Z10","Sheet1");
    
    tRect wRect(2,2,3,3);
    
    tVectorRange sVectorRange;
    wUISpreadSheet->FindRangesCovered(wRect,&sVectorRange);

    wUISpreadSheet->_Merge("E2:H3","Sheet1");
    
    tColRow::tContainerRange::tResult* wResult=wUISpreadSheet->FindRangesCovered(2, 5);
    
    tRange* wRange=wUISpreadSheet->ActiveSheet()->MergedRange(2, 5);
    cout << wRange->StrRef() << endl;

    
    tString wView  = wUISpreadSheet->_JsonView(1, 1, 396, 400,-10,-10,"Sheet1");
   
    wOk=wUISpreadSheet->_DeleteCol(1, 2,"Sheet1");
    if (wOk) {
        cout << "Delete Col Ok" << endl;
    }
    wUISpreadSheet->_Undo();
   
    wUISpreadSheet->_CellClass("A1","JavaScriptObj");
    
    tString wNewUri="www.skeema.fr/book/myBook";
    tString wOldUri=wUISpreadSheet->_GetActiveWorkBook();
    wOk=wUISpreadSheet->_RenameWorkBook(wOldUri, wNewUri);
    tString wSave=wUISpreadSheet->_WriteJson(wNewUri);
    
    wOk=wUISpreadSheet->_DeleteCol(1, 2,"Sheet1");
    if (wOk) {
        cout << "Delete Col Ok" << endl;
    }
    wUISpreadSheet->_Undo();
    
    delete(wUISpreadSheet);
    
    tApplication::Instance()->ClearUndoRedo();
    
    wUISpreadSheet = new tUISpreadSheet();
    wUISpreadSheet->IsUndoActif(true);
    wUISpreadSheet->_RegisterClassAttribute("JavaScriptObj","Javascript object","Javascript");
#ifdef checkfo
    wUISpreadSheet->CheckFormat();
#endif
    tBool wLoad=wUISpreadSheet->_ReadJson(wSave);

    tCell* wCell=wUISpreadSheet->Cell("A1");
    tCellClass* wCellClass=wCell->Class();
    
    wOk=wUISpreadSheet->_EnsureCell("Z34","Sheet1");
    
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "Name","Sheet1") << endl;
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "value","Sheet1") << endl;
    std::cout << wUISpreadSheet->_GetValueAttribute("A1", "date","Sheet1") << endl;
    
    wOk=wUISpreadSheet->_DeleteRow(1, 3,"Sheet1");
    if (wOk) {
        cout << "Delete Col Ok" << endl;
    }
    wUISpreadSheet->_Undo();
      
    wOk=wUISpreadSheet->_DeleteCol(1, 2,"Sheet1");
    if (wOk) {
        cout << "Delete Col Ok" << endl;
    }
    wUISpreadSheet->_Undo();
    
    wOk=wUISpreadSheet->_DeleteRow(1, 2,"Sheet1");
    if (wOk) {
        cout << "Delete Row Ok" << endl;
    }
    wUISpreadSheet->_Undo();
    
    wUISpreadSheet->_InsertNamedRange("TEST","A1:Z256","Sheet1");
    
    cout << wUISpreadSheet->JsonRangeNamed() << endl;
    // Test format
    tString wFormatString=wUISpreadSheet->_DefaultFormatString("#,##0.00 $;(#,##0.00) $");
    cout << wFormatString;
    
    
    
	delete(wUISpreadSheet);


    DoneFormatRoot();
	return(0);
}
#endif

