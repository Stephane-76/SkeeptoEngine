#ifndef __EMSCRIPTEN__

#include "../include/CallJson.hpp"

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

using namespace SkSpreadSheet;

// Helper function to print JSON results
void printResult(const tString& functionName, const tString& result) {
    std::cout << "=== " << functionName << " ===" << std::endl;
    std::cout << "Result: " << result << std::endl;
    std::cout << "===================" << std::endl << std::endl;
}

/// <#Description#>
int CallWithJson() {
    tUISpreadSheet spreadsheet;

    // Example 1: Create a new workbook
    tString params1 = R"({"uri": "example.xlsx"})";
    printResult("NewWorkBook", spreadsheet._Call("NewWorkBook", params1));

    // Example 2: Add a sheet
    tString params2 = R"({"name": "Sheet1"})";
    printResult("AddSheet", spreadsheet._Call("AddSheet", params2));

    // Example 3: Set cell value
    tString params3 = R"({
        "ref": "A1",
        "value": "Hello World",
        "sheet": "Sheet1"
    })";
    printResult("Value", spreadsheet._Call("Value", params3));

    // Example 4: Set cell format
    tString params4 = R"({
        "ref": "A1",
        "value": "background-color: yellow; color: red;",
        "sheet": "Sheet1"
    })";
    printResult("Format", spreadsheet._Call("Format", params4));

    // Example 5: Merge cells
    tString params5 = R"({
        "ref": "A1:B2",
        "sheet": "Sheet1"
    })";
    printResult("Merge", spreadsheet._Call("Merge", params5));

    // Example 6: Set row size
    tString params6 = R"({
        "begin": 1,
        "end": 1,
        "size": 30.0,
        "sheet": "Sheet1"
    })";
    printResult("SizeRow", spreadsheet._Call("SizeRow", params6));
    
    tString paramsBottomRight = R"({
        "sheet": "Sheet1"
    })";
    tString wResult=spreadsheet._Call("JsonBottomRight", paramsBottomRight);
    cout << "Result :" << wResult << endl;
    
    Document doc;
    doc.Parse(wResult.c_str()); 
    
    if (doc.HasParseError()) {
        ShowParseErrorJson(&doc, wResult);
    };
    // Example 7: Insert row
    tString params7 = R"({
        "begin": 2,
        "end": 2,
        "sheet": "Sheet1"
    })";
    printResult("InsertRow", spreadsheet._Call("InsertRow", params7));

    
    // Example 8: Get cell value
    tString params8 = R"({
        "ref": "A1",
        "sheet": "Sheet1"
    })";
    printResult("GetValue", spreadsheet._Call("GetValue", params8));

    // Example 9: Set cell class
    tString params9 = R"({
        "ref": "A1",
        "className": "MyClass",
        "sheet": "Sheet1"
    })";
    printResult("CellClass", spreadsheet._Call("CellClass", params9));

    // Example 10: Get JSON view
    tString params10 = R"({
        "row": 1,
        "col": 1,
        "viewHeight": 500,
        "viewWidth": 800,
        "diffY": 0,
        "diffX": 0,
        "sheet": "Sheet1"
    })";
    printResult("JsonView", spreadsheet._Call("JsonView", params10));
    
    // Example 11: Convert between base-10 and alpha
    tString params11 = R"({"value": 27})";
    printResult("B10toAlpha", spreadsheet._Call("B10toAlpha", params11));

    tString params12 = R"({"value": "AA"})";
    printResult("AlphaToB10", spreadsheet._Call("AlphaToB10", params12));

    // Example 12: Tree operations
    tString params13 = R"({
        "row": 1,
        "sheet": "Sheet1"
    })";
    printResult("OpenCloseTreeRow", spreadsheet._Call("OpenCloseTreeRow", params13));

    // Example 13: Get maximum dimensions
    printResult("MaxCol", spreadsheet._Call("MaxCol", "{}"));
    printResult("MaxRow", spreadsheet._Call("MaxRow", "{}"));

    // Example 14: Error handling (missing parameters)
    tString params14 = R"({"ref": "A1","value":"123"})"; // Missing value parameter
    printResult("Value (Error)", spreadsheet._Call("Value", params14));

    // Example 15: Unknown function
    printResult("UnknownFunction", spreadsheet._Call("UnknownFunction", "{}"));

    
    
#ifdef SKER_FILE_DIR
    tString wFileName = tString(SKER_FILE_DIR) + "/Horaires.json";
#else
    tString wFileName;
#endif
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
        
    
        // Appel de NewWorkBook avec Call
        tString wNewWorkBookParams = "{\"uri\":\"www.skeema.fr/WorkBook\"}";
        tString wNewWorkBookResult = spreadsheet._Call("NewWorkBook", wNewWorkBookParams);
        
        // Appel de ReadJson avec Call
        tString wReadJsonParams = "{\"json\":\"" + JsonStringResult(wJson) + "\"}";
        //cout << wReadJsonParams << endl;
        Document docJson;
        docJson.Parse(wReadJsonParams.c_str());
        
        if (docJson.HasParseError()) {
            ShowParseErrorJson(&docJson, wReadJsonParams);
        };
        tString wReadJsonResult = spreadsheet._Call("ReadJson", wReadJsonParams);

        // Appel de JsonView avec Call
        tString wJsonViewParams = R"({
            "row": 1,
            "col": 1,
            "viewHeight": 800,
            "viewWidth": 800,
            "diffY": 0,
            "diffX": 0,
            "sheet": "Sheet1"
        })";
        tString wJsonViewResult = spreadsheet._Call("JsonView", wJsonViewParams);

        Document docExcel;
        docExcel.Parse(wJsonViewResult.c_str()); 
        
        // Parcours du document JSON
        if (docExcel.HasParseError()) {
            cout << "Erreur de parsing JSON" << endl;
        } else {
            cout << "=== Contenu du document JSON ===" << endl;
            
            // Parcours des cellules
            if (docExcel.HasMember("cells") && docExcel["cells"].IsArray()) {
                const Value& cells = docExcel["cells"];
                cout << "Nombre de cellules: " << cells.Size() << endl;
                
                for (SizeType i = 0; i < cells.Size(); i++) {
                    const Value& cell = cells[i];
                    cout << "Cellule " << i << ":" << endl;
                    
                    if (cell.HasMember("ref")) cout << "  Ref: " << cell["ref"].GetString() << endl;
                    if (cell.HasMember("value")) cout << "  Value: " << cell["value"].GetString() << endl;
                    if (cell.HasMember("format")) cout << "  Format: " << cell["format"].GetString() << endl;
                }
            }
            
            // Parcours des colonnes
            if (docExcel.HasMember("cols") && docExcel["cols"].IsArray()) {
                const Value& cols = docExcel["cols"];
                cout << "\nColonnes:" << endl;
                for (SizeType i = 0; i < cols.Size(); i++) {
                    const Value& col = cols[i];
                    cout << "  Col " << i << ": " << col.GetDouble() << "px" << endl;
                }
            }
            
            // Parcours des lignes
            if (docExcel.HasMember("rows") && docExcel["rows"].IsArray()) {
                const Value& rows = docExcel["rows"];
                cout << "\nLignes:" << endl;
                for (SizeType i = 0; i < rows.Size(); i++) {
                    const Value& row = rows[i];
                    cout << "  Row " << i << ": " << row.GetDouble() << "px" << endl;
                }
            }
            
            cout << "============================" << endl;
        }
    }

    
    return 0;
} 
#endif
