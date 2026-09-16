//==============================================================================
// TestSkRangeData
// Test for UndoInsertRangeData function
//==============================================================================

#include "../include/TestSkRangeData.hpp"
#include "SkCsvImport.hpp"
#include "SkRangeData.hpp"
#include "SkRangeSort.hpp"

TestSkRangeData::TestSkRangeData() : CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkRangeData::DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << sTitle << endl;
        
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        tSheet* wSheet=m_Api->ActiveSheet();
        tColRow* wColRow=wSheet->Row(wRow);
        if (wColRow!=nullptr) {
            // Always display row at its original position, but indicate if it's hidden
            for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
                tVariant wVariant = m_Api->CellValue(wRow, wCol);
                tString wFormula = m_Api->Formula(wRow, wCol);
                tString wVisible = wColRow->DataVisible() ? "" : "[HIDDEN]";
                cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << wVisible << "\t";
            }
            cout << endl;
        }
	}
}

void TestSkRangeData::Fill() {
	tCell* wCell;
	for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
		if (wCol > 1) {
			wCell = m_Api->EnsureCell(1, wCol);
			tStringStream wStream;
			wStream << Base10ToAlpha(wCol - 1) << m_NbRow - 1 << "+1";
			m_Api->CompilCell(wCell, wStream.str().c_str());
		}
	}

	for (tInt wRow = 2; wRow <= m_NbRow; wRow++) {
		for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
			wCell = m_Api->EnsureCell(wRow, wCol);
			tStringStream wStream;
			if (wRow == m_NbRow) {
				if (wCol != m_NbCol) {
					wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
				}
				else {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
				}
			}
			else {
				if (wCol == m_NbCol) {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
				}
				else {
					wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
				}
			}
			m_Api->CompilCell(wCell, wStream.str().c_str());
		}
	}
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("", 1, 1, m_NbRow, m_NbCol);
}

void TestSkRangeData::TestTable() {
	// Create a table with different data types and formulas using #Headers and #This Row
	
	// Define table structure with multiple columns of different types
	tString wJsonData = "{\"columns\":["
		"{\"index\":1,\"name\":\"Product\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Quantity\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":3,\"name\":\"Unit Price\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":4,\"name\":\"Total\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":5,\"name\":\"Discount\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	
	// Insert RangeData table at A1:E6 (header + 5 data rows)
	tBool wResult = m_Api->UndoInsertRangeData("PRODUCTS", "A1:E6", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestTable: Insert should succeed", wResult == true);
	
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("PRODUCTS");
	CPPUNIT_ASSERT_MESSAGE("TestTable: Range should exist", wRange != nullptr);
	
	// Fill header row (row 1)
	m_Api->UndoCellValue("A1", "Product");
	m_Api->UndoCellValue("B1", "Quantity");
	m_Api->UndoCellValue("C1", "Unit Price");
	m_Api->UndoCellValue("D1", "Total");
	m_Api->UndoCellValue("E1", "Discount");
	
	// Fill data rows with different types
	// Row 2: Product A
	m_Api->UndoCellValue("A2", "Product A");
	m_Api->UndoCellValue("B2", 10);  // int
	m_Api->UndoCellValue("C2", 25.50);  // double
	m_Api->UndoCellValue("D2", "=PRODUCTS[[#This Row];[Quantity]]*PRODUCTS[[#This Row];[Unit Price]]");  // Formula with #This Row
	m_Api->UndoCellValue("E2", 0.05);  // double (5% discount)
 /*
    tCell* wCellD2=m_Api->Cell("D2");
    cout << wCellD2->FormulaStr() << endl;
*/
	// Row 3: Product B
	m_Api->UndoCellValue("A3", "Product B");
	m_Api->UndoCellValue("B3", 5);  // int
	m_Api->UndoCellValue("C3", 15.75);  // double
	m_Api->UndoCellValue("D3", "=PRODUCTS[[#This Row];[Quantity]]*PRODUCTS[[#This Row];[Unit Price]]");  // Formula with #This Row
	m_Api->UndoCellValue("E3", 0.10);  // double (10% discount)
	
	// Row 4: Product C
	m_Api->UndoCellValue("A4", "Product C");
	m_Api->UndoCellValue("B4", 20);  // int
	m_Api->UndoCellValue("C4", 8.99);  // double
	m_Api->UndoCellValue("D4", "=PRODUCTS[[#This Row];[Quantity]]*PRODUCTS[[#This Row];[Unit Price]]");  // Formula with #This Row
	m_Api->UndoCellValue("E4", 0.0);  // double (no discount)
	
	// Row 5: Product D
	m_Api->UndoCellValue("A5", "Product D");
	m_Api->UndoCellValue("B5", 8);  // int
	m_Api->UndoCellValue("C5", 12.25);  // double
	m_Api->UndoCellValue("D5", "=PRODUCTS[[#This Row];[Quantity]]*PRODUCTS[[#This Row];[Unit Price]]");  // Formula with #This Row
	m_Api->UndoCellValue("E5", 0.15);  // double (15% discount)
	
	// Row 6: Product E
	m_Api->UndoCellValue("A6", "Product E");
	m_Api->UndoCellValue("B6", 15);  // int
	m_Api->UndoCellValue("C6", 30.00);  // double
	m_Api->UndoCellValue("D6", "=PRODUCTS[[#This Row];[Quantity]]*PRODUCTS[[#This Row];[Unit Price]]");  // Formula with #This Row
	m_Api->UndoCellValue("E6", 0.20);  // double (20% discount)
	
	// Test formulas with #Headers
	// Reference to header row
	tString wFormulaHeaders = "=PRODUCTS[[#Headers]]";
	tBool wCompilResult = m_Api->UndoCellValue("A10", wFormulaHeaders);
	if (!wCompilResult) {
		cout << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula with #Headers should compile", wCompilResult == true);
	
	// Reference to specific header column
	tString wFormulaHeaderColumn = "=PRODUCTS[[#Headers];[Product]]";
	wCompilResult = m_Api->UndoCellValue("A11", wFormulaHeaderColumn);
	if (!wCompilResult) {
		cout << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula with #Headers and column should compile", wCompilResult == true);
	
	// Reference to header with column containing space
	tString wFormulaHeaderUnitPrice = "='PRODUCTS'[[#Headers];[Unit Price]]";
	wCompilResult = m_Api->UndoCellValue("A12", wFormulaHeaderUnitPrice);
	if (!wCompilResult) {
		cout << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula with #Headers and Unit Price column should compile", wCompilResult == true);
	
	// Test formula combining #This Row with calculation
	tString wFormulaThisRow = "='PRODUCTS'[[#This Row];[Total]]*(1-'PRODUCTS'[[#This Row];[Discount]])";
	wCompilResult = m_Api->UndoCellValue("F2", wFormulaThisRow);
	if (!wCompilResult) {
		cout << m_Api->ErrorWithDetail() << endl;
	}
 /*
    tCell* wCellF2=m_Api->Cell("F2");
    cout << wCellF2->FormulaStr() << endl;
*/
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula with #This Row and calculation should compile", wCompilResult == true);
	
	// Copy the same formula to other rows
	m_Api->UndoCellValue("F3", wFormulaThisRow);
	m_Api->UndoCellValue("F4", wFormulaThisRow);
	m_Api->UndoCellValue("F5", wFormulaThisRow);
	m_Api->UndoCellValue("F6", wFormulaThisRow);
	
	// Verify formulas are stored correctly
	tString wStoredFormula = m_Api->Formula(2, 4); // D2
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula should be stored correctly", 
		wStoredFormula.find("PRODUCTS") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("TestTable: Formula should contain #This Row", 
		wStoredFormula.find("#This Row") != tString::npos);
	
	DrawCell("After TestTable", 1, 1, 6, 6);
}

void TestSkRangeData::TestInsertRangeData() {
	// Test basic insertion of range data
	// filtervalue must be a tVariant JSON object with "t" (type) and "v" (value) keys
	tString wJsonData = "{\"columns\":[{\"index\":1,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Dupont\"},\"order\":\"Ascending\"},{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Jean\"},\"order\":\"Ascending\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("TEST_DATA", "A1:B2", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeData: Insert should succeed", wResult == true);
	
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("TEST_DATA");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeData: Range should exist", wRange != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeData: Range should be named", wRange->IsNamed());
	
	// Verify range reference
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeData: Range reference", wRange->StrRef(true) == "Sheet1!A1:B2");
 
    tRangeData* wRangeData=m_Api->RangeData("TEST_DATA");
    CPPUNIT_ASSERT_MESSAGE("TestInsertRangeData(): RangeData should exist before", wRangeData != nullptr);
    
#ifdef _DEBUG
    //cout << wRange->Debug() << endl;
    //cout << wRangeData->Debug() << endl;
#endif
}

void TestSkRangeData::TestInsertRangeDataUndoRedo() {
	// Test undo/redo functionality
	// filtervalue must be a tVariant JSON object with "t" (type) and "v" (value) keys
	tString wJsonData = "{\"columns\":[{\"index\":1,\"name\":\"Column1\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("TEST_UNDO", "C1:C3", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataUndoRedo: Insert should succeed", wResult == true);
	
    
 
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("TEST_UNDO");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataUndoRedo: Range should exist before undo", wRange != nullptr);

    tRangeData* wRangeData=m_Api->RangeData("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataUndoRedo: RangeData should exist before undo", wRangeData != nullptr);
	// Undo the operation
	m_Api->Undo();
	
	// Verify the named range no longer exists
	wRange = m_Api->FindRangeNamed("TEST_UNDO");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataUndoRedo: Range should not exist after undo", wRange == nullptr);
	
	// Redo the operation
	m_Api->Redo();
	
	// Verify the named range exists again
	wRange = m_Api->FindRangeNamed("TEST_UNDO");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataUndoRedo: Range should exist after redo", wRange != nullptr);
	
}

void TestSkRangeData::TestApiUndoAddRangeNamedUndoRedo() {
	// Mirrors tUndoAddRangeNamed via SkApi::UndoAddRangeNamed (not UndoInsertRangeNamed).
	tBool wOk = m_Api->UndoAddRangeNamed("API_ADD_NAMED", "M90:N91");
	CPPUNIT_ASSERT_MESSAGE("UndoAddRangeNamed should succeed", wOk);
	tRange* wRange = m_Api->FindRangeNamed("API_ADD_NAMED");
	CPPUNIT_ASSERT_MESSAGE("named range exists after Do", wRange != nullptr);

	m_Api->Undo();
	wRange = m_Api->FindRangeNamed("API_ADD_NAMED");
	CPPUNIT_ASSERT_MESSAGE("named range removed after Undo", wRange == nullptr);

	m_Api->Redo();
	wRange = m_Api->FindRangeNamed("API_ADD_NAMED");
	CPPUNIT_ASSERT_MESSAGE("named range restored after Redo", wRange != nullptr);
}

void TestSkRangeData::TestApiUndoAddRangeDataUndoRedo() {
	// tUndoAddRangeData: geometry needs Height() >= 2 for FillByRect when JSON drives metadata.
	tString wJson =
		"{\"columns\":[{\"index\":12,\"name\":\"ColApi\",\"type\":\"string\",\"filterop\":\"None\","
		"\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	tBool wOk = m_Api->UndoAddRangeData("API_ADD_RD", "L92:L93", wJson);
	CPPUNIT_ASSERT_MESSAGE("UndoAddRangeData should succeed", wOk);
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_ADD_RD") != nullptr);

	m_Api->Undo();
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_ADD_RD") == nullptr);

	m_Api->Redo();
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_ADD_RD") != nullptr);
}

void TestSkRangeData::TestUndoAddRangeDataEmptyJsonFromTopRowSingleColumn() {
	// Empty RangeData JSON: tUndoAddRangeData fills descriptors via FillByRect from the top row of
	// the ref (needs Height() >= 2). Single-column ref reads only that column's header cell.
	const tString wHeader = "SKU_FromSheet";
	CPPUNIT_ASSERT_MESSAGE("seed header cell",
						   m_Api->UndoCellValue("Z94", wHeader));
	// Row 95 is body row (may stay blank); geometry satisfies FillByRect.
	tBool wOk =
		m_Api->UndoAddRangeData("EMPTY_JSON_COL", "Z94:Z95", tString());
	CPPUNIT_ASSERT_MESSAGE("UndoAddRangeData with empty JSON should succeed", wOk);
	tRange* wRange = m_Api->FindRangeNamed("EMPTY_JSON_COL");
	CPPUNIT_ASSERT_MESSAGE("named range exists", wRange != nullptr);
	tRangeData* wRd = m_Api->RangeData("EMPTY_JSON_COL");
	CPPUNIT_ASSERT_MESSAGE("RangeData attached", wRd != nullptr);
	tColumnData* wCol = wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, wHeader);
	CPPUNIT_ASSERT_MESSAGE("column label from top row of Z94", wCol != nullptr);
	// Z is column 26 when A == 1.
	CPPUNIT_ASSERT_EQUAL(static_cast<tSize>(26), static_cast<tSize>(wCol->SheetCol()));
}

void TestSkRangeData::TestApiUndoApplyRangeDataUndoRedo() {
	tString wJsonOld = "{\"columns\":["
					   "{\"index\":20,\"name\":\"HdrOldA\",\"type\":\"string\",\"filterop\":\"None\","
					   "\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
					   "{\"index\":21,\"name\":\"HdrOldB\",\"type\":\"string\",\"filterop\":\"None\","
					   "\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
					   "]}";
	tString wJsonNew = "{\"columns\":["
					   "{\"index\":20,\"name\":\"HdrNewA\",\"type\":\"string\",\"filterop\":\"None\","
					   "\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
					   "{\"index\":21,\"name\":\"HdrNewB\",\"type\":\"string\",\"filterop\":\"None\","
					   "\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
					   "]}";
	CPPUNIT_ASSERT_MESSAGE("UndoAddRangeData seed",
						   m_Api->UndoCellValue("T92", "HdrOldA")
						   && m_Api->UndoCellValue("U92", "HdrOldB")
						   && m_Api->UndoAddRangeData("API_APPLY_RD", "T92:U93", wJsonOld));
	tRange* wRange = m_Api->FindRangeNamed("API_APPLY_RD");
	tRangeData* wRd = m_Api->RangeData("API_APPLY_RD");
	CPPUNIT_ASSERT(wRd != nullptr && wRange != nullptr);
	CPPUNIT_ASSERT_MESSAGE("initial column name",
						   wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrOldA") != nullptr);

	CPPUNIT_ASSERT(m_Api->UndoCellValue("T92", "HdrNewA"));
	CPPUNIT_ASSERT(m_Api->UndoCellValue("U92", "HdrNewB"));
	CPPUNIT_ASSERT_MESSAGE("UndoApplyRangeData",
						   m_Api->UndoApplyRangeData("API_APPLY_RD", wJsonNew));
	wRd = m_Api->RangeData("API_APPLY_RD");
	wRange = m_Api->FindRangeNamed("API_APPLY_RD");
	CPPUNIT_ASSERT(wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrNewA") != nullptr);
	CPPUNIT_ASSERT(wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrOldA") == nullptr);

	m_Api->Undo();
	m_Api->UndoCellValue("T92", "HdrOldA");
	m_Api->UndoCellValue("U92", "HdrOldB");
	wRd = m_Api->RangeData("API_APPLY_RD");
	wRange = m_Api->FindRangeNamed("API_APPLY_RD");
	CPPUNIT_ASSERT(wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrOldA") != nullptr);
	CPPUNIT_ASSERT(wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrNewA") == nullptr);

	m_Api->Redo();
	m_Api->UndoCellValue("T92", "HdrNewA");
	m_Api->UndoCellValue("U92", "HdrNewB");
	wRd = m_Api->RangeData("API_APPLY_RD");
	wRange = m_Api->FindRangeNamed("API_APPLY_RD");
	CPPUNIT_ASSERT(wRd->FindColumnByName(m_Api->ActiveSheet(), wRange, "HdrNewA") != nullptr);
}

void TestSkRangeData::TestApiUndoDeleteRangeNamedUndoRedo() {
	CPPUNIT_ASSERT(m_Api->UndoAddRangeNamed("API_DEL_NAMED", "P190:Q191"));
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_DEL_NAMED") != nullptr);
	CPPUNIT_ASSERT_MESSAGE("UndoDeleteRangeNamed", m_Api->UndoDeleteRangeNamed("API_DEL_NAMED"));
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_DEL_NAMED") == nullptr);

	m_Api->Undo();
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_DEL_NAMED") != nullptr);

	m_Api->Redo();
	CPPUNIT_ASSERT(m_Api->FindRangeNamed("API_DEL_NAMED") == nullptr);
}

void TestSkRangeData::TestInsertRangeDataWithColumns() {
	// Test insertion with multiple columns and different types
	// filtervalue must be a tVariant JSON object with "t" (type) and "v" (value) keys
	tString wJsonData = "{\"columns\":["
		"{\"index\":1,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Descending\"},"
		"{\"index\":3,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"GreaterThan\",\"filtervalue\":{\"t\":\"d\",\"v\":1000},\"order\":\"Ascending\"}"
		"]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("TEST_COLUMNS", "D1:F5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataWithColumns: Insert should succeed", wResult == true);
	
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("TEST_COLUMNS");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataWithColumns: Range should exist", wRange != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataWithColumns: Range should be named", wRange->IsNamed());
	
	// Verify range reference
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataWithColumns: Range reference", wRange->StrRef(true) == "Sheet1!D1:F5");
	
	// Test deletion
	m_Api->UndoDeleteRangeNamed("TEST_COLUMNS");
	wRange = m_Api->FindRangeNamed("TEST_COLUMNS");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRangeDataWithColumns: Range should not exist after delete", wRange == nullptr);
	
	DrawCell("After InsertRangeDataWithColumns", 1, 1, 5, 10);
}

void TestSkRangeData::TestDeleteColRowCovered() {
	// Create two RangeData at different positions
	// After deleting rows, they should end up at the same coordinates (same Top and Bottom)
	// Then test Undo
	
	// RangeData1 at A1:B2 (rows 1-2) - positioned so it won't be deleted in first deletion
	tString wJsonData1 = "{\"columns\":[{\"index\":1,\"name\":\"Name1\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},{\"index\":2,\"name\":\"Value1\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	tBool wResult1 = m_Api->UndoInsertRangeData("RANGE_DATA_1", "A1:B2", wJsonData1);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Insert RangeData1 should succeed", wResult1 == true);
	
	// RangeData2 at A1:B5 (rows 1-5) - positioned so that after deleting rows 3-4, it will be at A3:B4
	tString wJsonData2 = "{\"columns\":[{\"index\":1,\"name\":\"Name2\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},{\"index\":2,\"name\":\"Value2\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	tBool wResult2 = m_Api->UndoInsertRangeData("RANGE_DATA_2", "A1:B3", wJsonData2);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Insert RangeData2 should succeed", wResult2 == true);
	
	// Verify both ranges exist at their initial positions
	tRange* wRange1 = m_Api->FindRangeNamed("RANGE_DATA_1");
	tRange* wRange2 = m_Api->FindRangeNamed("RANGE_DATA_2");
    //cout << wRange1->StrRef() << endl;
    //cout << wRange2->StrRef() << endl;
 
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range1 should exist", wRange1 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range2 should exist", wRange2 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range1 initial position", wRange1->StrRef(true) == "Sheet1!A1:B2");
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range2 initial position", wRange2->StrRef(true) == "Sheet1!A1:B3");
	
     tBool wOk=m_Api->UndoCellValue("A10","=RANGE_DATA_1");
 
    CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: A10=RANGE_DATA_1", wOk);
     
    tCell* wCell=m_Api->Cell("A10");
    CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: A10=RANGE_DATA_1",
                           wCell->FormulaStr()=="RANGE_DATA_1");
	m_Api->UndoDeleteRow(2, 4);
 
    wCell=m_Api->Cell("A6");
    //cout << wCell->FormulaStr() << endl;
    // ??? Different order after undo/delete row
	  CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: A10=RANGE_DATA_2",
                           wCell->FormulaStr()=="RANGE_DATA_2");
    
	wRange1 = m_Api->FindRangeNamed("RANGE_DATA_1");
	wRange2 = m_Api->FindRangeNamed("RANGE_DATA_2");
#ifdef checksp
    m_Api->Check();
#endif

   
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range1 should exist after deleting rows 3-4", wRange1 == nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range2 should exist after deleting rows 3-4", wRange2 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: Range2 should be at A3:B4", wRange2->StrRef(true) == "Sheet1!A1:B1");
    m_Api->Undo();
	wRange1 = m_Api->FindRangeNamed("RANGE_DATA_1");
	wRange2 = m_Api->FindRangeNamed("RANGE_DATA_2");
 
    wCell=m_Api->Cell("A10");
    //cout << wCell->FormulaStr() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestDeleteColRowCovered: A10=RANGE_DATA_1",
                           wCell->FormulaStr()=="RANGE_DATA_1");
}

void TestSkRangeData::TestRangeDataSortSimple() {
	// Simple test: create 3 rows with names, sort by first column
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("A2", "Charlie");
	m_Api->UndoCellValue("A3", "Alice");
	m_Api->UndoCellValue("A4", "Bob");
	
	// Create RangeData with sort by column 1 (Name) ascending
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("SIMPLE_SORT", "A1:A4", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("SIMPLE_SORT");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: Range should exist", wRange != nullptr);
	
	tRangeData* wRangeData = m_Api->RangeData("SIMPLE_SORT");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: RangeData should exist", wRangeData != nullptr);
	
	// Perform sort
	wRangeData->Sort(wRange);
	
	// Verify sort: should be Alice, Bob, Charlie (header stays first)
	tVariant wA2 = m_Api->CellValue(2, 1);
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wA4 = m_Api->CellValue(4, 1);
	
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: A2 should be Alice", wA2.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: A3 should be Bob", wA3.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortSimple: A4 should be Charlie", wA4.String() == "Charlie");
}

void TestSkRangeData::TestRangeDataSortAscending() {
	// Test ascending sort with numbers
	m_Api->UndoCellValue("A1", "Value");
	m_Api->UndoCellValue("A2", 30);
	m_Api->UndoCellValue("A3", 10);
	m_Api->UndoCellValue("A4", 20);
	m_Api->UndoCellValue("A5", 5);
	
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Value\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("ASC_SORT", "A1:A5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortAscending: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("ASC_SORT");
	tRangeData* wRangeData = m_Api->RangeData("ASC_SORT");
	
	// Perform sort
	wRangeData->Sort(wRange);
	
	// Verify ascending order: 5, 10, 20, 30
	tVariant wA2 = m_Api->CellValue(2, 1);
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wA4 = m_Api->CellValue(4, 1);
	tVariant wA5 = m_Api->CellValue(5, 1);
	
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortAscending: A2 should be 5", wA2.Int() == 5);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortAscending: A3 should be 10", wA3.Int() == 10);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortAscending: A4 should be 20", wA4.Int() == 20);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortAscending: A5 should be 30", wA5.Int() == 30);
}

void TestSkRangeData::TestRangeDataSortDescending() {
	// Test descending sort with strings
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("A3", "Charlie");
	m_Api->UndoCellValue("A4", "Bob");
	
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Descending\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("DESC_SORT", "A1:A4", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortDescending: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("DESC_SORT");
	tRangeData* wRangeData = m_Api->RangeData("DESC_SORT");
	
	// Perform sort
	wRangeData->Sort(wRange);
	
	// Verify descending order: Charlie, Bob, Alice
	tVariant wA2 = m_Api->CellValue(2, 1);
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wA4 = m_Api->CellValue(4, 1);
	
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortDescending: A2 should be Charlie", wA2.String() == "Charlie");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortDescending: A3 should be Bob", wA3.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortDescending: A4 should be Alice", wA4.String() == "Alice");
}

void TestSkRangeData::TestRangeDataSortMultiColumn() {
	// Test multi-column sort: sort by Name first, then by Age
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("B2", 30);
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("B3", 25);
	m_Api->UndoCellValue("A4", "Alice");
	m_Api->UndoCellValue("B4", 20);
	m_Api->UndoCellValue("A5", "Bob");
	m_Api->UndoCellValue("B5", 30);
	
	// Sort by Name (ascending), then by Age (ascending)
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	DrawCell("Before Sort multi Column", 1, 1, 10, 4);
    
	tBool wResult = m_Api->UndoInsertRangeData("MULTI_SORT", "A1:B5", wJsonData);
 
    DrawCell("After Sort multi Column", 1, 1, 10, 4);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("MULTI_SORT");
	tRangeData* wRangeData = m_Api->RangeData("MULTI_SORT");
	
	// Perform sort
	wRangeData->Sort(wRange);
	
	// Verify multi-column sort:
	// Alice (20), Alice (30), Bob (25), Bob (30)
	tVariant wA2 = m_Api->CellValue(2, 1);
	tVariant wB2 = m_Api->CellValue(2, 2);
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wB3 = m_Api->CellValue(3, 2);
	tVariant wA4 = m_Api->CellValue(4, 1);
	tVariant wB4 = m_Api->CellValue(4, 2);
	tVariant wA5 = m_Api->CellValue(5, 1);
	tVariant wB5 = m_Api->CellValue(5, 2);
	
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: A2 should be Alice", wA2.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: B2 should be 20", wB2.Int() == 20);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: A3 should be Alice", wA3.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: B3 should be 30", wB3.Int() == 30);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: A4 should be Bob", wA4.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: B4 should be 25", wB4.Int() == 25);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: A5 should be Bob", wA5.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumn: B5 should be 30", wB5.Int() == 30);
}

void TestSkRangeData::TestRangeDataSortMultiColumnReorganizeCells() {
	// Test multi-column sort using ReorganizeCells method: sort by Name first, then by Age
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("B2", 30);
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("B3", 25);
	m_Api->UndoCellValue("A4", "Alice");
	m_Api->UndoCellValue("B4", 20);
	m_Api->UndoCellValue("A5", "Bob");
	m_Api->UndoCellValue("B5", 30);
	
	// Sort by Name (ascending), then by Age (ascending)
	tString wJsonData = "{\"columns\":["
		"{\"index\":1,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	DrawCell("Before Sort multi Column (ReorganizeCells)", 1, 1, 10, 4);
    
	tBool wResult = m_Api->UndoInsertRangeData("MULTI_SORT_CELLS", "A1:B5", wJsonData);
 
    DrawCell("After Sort multi Column (ReorganizeCells)", 1, 1, 10, 4);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("MULTI_SORT_CELLS");
	
	// Create sort options with ReorganizeCells method
	// We'll manually create the sort options similar to what tRangeData::Sort() does
	tSortOptions wSortOptions;
	wSortOptions.HasHeader(true); // Use first row as header
	wSortOptions.AddSortColumn(0, tSortOrder::Ascending);  // Column 0 (Name)
	wSortOptions.AddSortColumn(1, tSortOrder::Ascending);  // Column 1 (Age)
	// Explicitly set ReorganizeCells method
	wSortOptions.ReorganizeMethod(tSortReorganizeMethod::ReorganizeCells);
	
	// Perform sort with explicit options
	tRangeSort wRangeSort(wRange->ColRowCellRange());
	wRangeSort.Sort(wRange, wSortOptions);
	
	// Verify multi-column sort:
	// Alice (20), Alice (30), Bob (25), Bob (30)
	tVariant wA2 = m_Api->CellValue(2, 1);
	tVariant wB2 = m_Api->CellValue(2, 2);
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wB3 = m_Api->CellValue(3, 2);
	tVariant wA4 = m_Api->CellValue(4, 1);
	tVariant wB4 = m_Api->CellValue(4, 2);
	tVariant wA5 = m_Api->CellValue(5, 1);
	tVariant wB5 = m_Api->CellValue(5, 2);
	
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: A2 should be Alice", wA2.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: B2 should be 20", wB2.Int() == 20);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: A3 should be Alice", wA3.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: B3 should be 30", wB3.Int() == 30);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: A4 should be Bob", wA4.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: B4 should be 25", wB4.Int() == 25);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: A5 should be Bob", wA5.String() == "Bob");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataSortMultiColumnReorganizeCells: B5 should be 30", wB5.Int() == 30);
}


#ifndef __EMSCRIPTEN__
void TestSkRangeData::TestCsvLoadAndSort() {
	// Load CSV file
	tString wCsvPath = "/Users/stephaneallez/Projects/library/libraries_test/SkFileTest/test_data.csv";
	tCsvImport wCsvImport(',', '"', true);

	// Import CSV starting at A1
	tBool wResult = wCsvImport.Import(wCsvPath, "A1", nullptr, nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: CSV import should succeed", wResult == true);
	
    DrawCell("Before Sort", 1, 1, 10, 4);
 
	// CSV has 4 columns (Nom, Prenom, Age, Nationalite) and 54 rows (header + 53 data rows)
	// Range reference: A1:D54
	tString wRangeRef = "A1:D53";
	
	// Create RangeData with column definitions from CSV headers using UndoInsertRangeData
	// Column indices in JSON are 0-based relative to the range: 0=Nom, 1=Prenom, 2=Age, 3=Nationalite
	// For "tri sans sélection" (sort without selection), we create a RangeData with all columns
	// but without user selection - we'll use default ascending order for all columns
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Nom\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Prenom\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":3,\"name\":\"Nationalite\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	//cout << "m_Api->UndoInsertRangeData" << endl;
	// Insert RangeData using API
	tBool wInsertResult = m_Api->UndoInsertRangeData("CSV_DATA", wRangeRef, wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: UndoInsertRangeData should succeed", wInsertResult == true);
	//cout << "end m_Api->UndoInsertRangeData" << endl;
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("CSV_DATA");
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: Range should exist", wRange != nullptr);
	
	// Get RangeData from API
	tRangeData* wRangeData = m_Api->RangeData("CSV_DATA");
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: RangeData should exist", wRangeData != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: RangeData should not be empty", wRangeData->IsEmpty() == false);
	
	// Perform sort (will sort by all columns in order: Nom, Prenom, Age, Nationalite, all ascending)
	wRangeData->Sort(wRange);
 
    DrawCell("After Sort", 1, 1, 10, 4);
	
	CPPUNIT_ASSERT_MESSAGE("TestCsvLoadAndSort: Sort should complete", true);
}
#endif

void TestSkRangeData::TestRangeDataFilterEquals() {
	// Test filter with Equals operator
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("A4", "Alice");
	m_Api->UndoCellValue("A5", "Charlie");
 
    DrawCell("Before Filter Alice",1, 1, 5, 1);
	
	// Create RangeData with filter: Name equals "Alice"
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"Equals\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Alice\"},\"order\":\"None\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("FILTER_EQUALS", "A1:A5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: UndoInsertRangeData should succeed", wResult == true);
 
    DrawCell("After Filter Alice",1, 1, 5, 1);
	
	tRange* wRange = m_Api->FindRangeNamed("FILTER_EQUALS");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Range should exist", wRange != nullptr);
	
	tRangeData* wRangeData = m_Api->RangeData("FILTER_EQUALS");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: RangeData should exist", wRangeData != nullptr);
	
	// Perform filter
	wRangeData->Filter(wRange);
	
	// Verify filter: only rows with "Alice" should be visible (rows 2 and 4)
	// Header row (row 1) should always be visible
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 1 (header) should be visible", wColRowCellRange->Row(1)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 2 (Alice) should be visible", wColRowCellRange->Row(2)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 3 (Bob) should not be visible", wColRowCellRange->Row(3)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 4 (Alice) should be visible", wColRowCellRange->Row(4)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 5 (Charlie) should not be visible", wColRowCellRange->Row(5)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 3 hidden from grid view",
	                       wColRowCellRange->IsRowVisible(3) == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 3 has zero height in JsonView",
	                       wColRowCellRange->SizeRow(3, tUnitMetrics::millimeters) == 0.0);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterEquals: Row 2 stays visible in grid view",
	                       wColRowCellRange->IsRowVisible(2) == true);
}

void TestSkRangeData::TestRangeDataFilterGreaterThan() {
	// Test filter with GreaterThan operator
	m_Api->UndoCellValue("A1", "Value");
	m_Api->UndoCellValue("A2", 10);
	m_Api->UndoCellValue("A3", 30);
	m_Api->UndoCellValue("A4", 20);
	m_Api->UndoCellValue("A5", 5);
	
	// Create RangeData with filter: Value > 15
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Value\",\"type\":\"int\",\"filterop\":\"GreaterThan\",\"filtervalue\":{\"t\":\"i\",\"v\":15},\"order\":\"None\"}]}";
	
    DrawCell("Before Filter >15",1, 1, 5, 1);
 
	tBool wResult = m_Api->UndoInsertRangeData("FILTER_GT", "A1:A5", wJsonData);
 
    DrawCell("After Filter >15",1, 1, 5, 1);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("FILTER_GT");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Range should exist", wRange != nullptr);
	
	tRangeData* wRangeData = m_Api->RangeData("FILTER_GT");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: RangeData should exist", wRangeData != nullptr);
	
	// Perform filter
	wRangeData->Filter(wRange);
	
	// Verify filter: only rows with value > 15 should be visible (rows 3 and 4)
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Row 1 (header) should be visible", wColRowCellRange->Row(1)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Row 2 (10) should not be visible", wColRowCellRange->Row(2)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Row 3 (30) should be visible", wColRowCellRange->Row(3)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Row 4 (20) should be visible", wColRowCellRange->Row(4)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterGreaterThan: Row 5 (5) should not be visible", wColRowCellRange->Row(5)->DataVisible() == false);
}

void TestSkRangeData::TestRangeDataFilterContains() {
	// Test filter with Contains operator
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("A4", "Charlie");
	m_Api->UndoCellValue("A5", "Alice Smith");
	
	// Create RangeData with filter: Name contains "li"
	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"Contains\",\"filtervalue\":{\"t\":\"s\",\"v\":\"li\"},\"order\":\"None\"}]}";
	
	tBool wResult = m_Api->UndoInsertRangeData("FILTER_CONTAINS", "A1:A5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("FILTER_CONTAINS");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Range should exist", wRange != nullptr);
	
	tRangeData* wRangeData = m_Api->RangeData("FILTER_CONTAINS");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: RangeData should exist", wRangeData != nullptr);
	
	// Perform filter
	wRangeData->Filter(wRange);
	
	// Verify filter: only rows containing "li" should be visible (rows 2, 4, 5)
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Row 1 (header) should be visible", wColRowCellRange->Row(1)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Row 2 (Alice) should be visible", wColRowCellRange->Row(2)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Row 3 (Bob) should not be visible", wColRowCellRange->Row(3)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Row 4 (Charlie) should be visible", wColRowCellRange->Row(4)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterContains: Row 5 (Alice Smith) should be visible", wColRowCellRange->Row(5)->DataVisible() == true);
}

void TestSkRangeData::TestRangeDataFilterMultiColumn() {
	// Test filter with multiple columns (AND logic)
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("B2", 25);
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("B3", 30);
	m_Api->UndoCellValue("A4", "Alice");
	m_Api->UndoCellValue("B4", 30);
	m_Api->UndoCellValue("A5", "Charlie");
	m_Api->UndoCellValue("B5", 25);
	
	// Create RangeData with filters: Name equals "Alice" AND Age > 25
	// Also apply sort ascending on column 1 (Name)
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"Equals\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Alice\"},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"GreaterThan\",\"filtervalue\":{\"t\":\"i\",\"v\":25},\"order\":\"None\"}"
		"]}";
	
    DrawCell("Before Filter MultiColumn",1, 1, 5, 2);
  
	tBool wResult = m_Api->UndoInsertRangeData("FILTER_MULTI", "A1:B5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: UndoInsertRangeData should succeed", wResult == true);
	
	tRange* wRange = m_Api->FindRangeNamed("FILTER_MULTI");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Range should exist", wRange != nullptr);
	
	tRangeData* wRangeData = m_Api->RangeData("FILTER_MULTI");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: RangeData should exist", wRangeData != nullptr);
	/*
	// Apply filter first, then sort (as done in ApplyRangeData)
	wRangeData->Filter(wRange);
	wRangeData->Sort(wRange);
	*/
    DrawCell("After Filter MultiColumn",1, 1, 5, 2);
	
	// Verify filter: only row 4 matches both criteria (Alice AND Age > 25)
	// After filtering and sorting ascending on Name, the visible row (Alice, 30) is moved to row 3
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Row 1 (header) should be visible", wColRowCellRange->Row(1)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Row 2 (Alice, 25) should not be visible", wColRowCellRange->Row(2)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Row 3 (Alice, 30) should be visible after sort", wColRowCellRange->Row(3)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Row 4 (Bob, 30) should not be visible", wColRowCellRange->Row(4)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: Row 5 (Charlie, 25) should not be visible", wColRowCellRange->Row(5)->DataVisible() == false);
	
	// Verify that the visible row contains Alice with Age 30 (after sort and filter)
	// After sorting ascending on Name, Alice(30) is at row 3
	tVariant wA3 = m_Api->CellValue(3, 1);
	tVariant wB3 = m_Api->CellValue(3, 2);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: A3 should be Alice after sort and filter", wA3.String() == "Alice");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiColumn: B3 should be 30 after sort and filter", wB3.Int() == 30);
}

void TestSkRangeData::TestRangeDataFilterMultiValueSameColumn() {
	m_Api->UndoCellValue("A1", "Year");
	m_Api->UndoCellValue("A2", "1961");
	m_Api->UndoCellValue("A3", "1968");
	m_Api->UndoCellValue("A4", "1975");
	m_Api->UndoCellValue("A5", "1961");

	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Year\",\"type\":\"string\",\"filterop\":\"Equals\","
		"\"filtervalue\":{\"t\":\"s\",\"v\":\"1961\"},"
		"\"filtervalues\":[{\"t\":\"s\",\"v\":\"1961\"},{\"t\":\"s\",\"v\":\"1968\"}],"
		"\"order\":\"None\"}"
		"]}";

	tBool wResult = m_Api->UndoInsertRangeData("FILTER_MULTI_VALUE", "A1:A5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: UndoInsertRangeData should succeed", wResult == true);

	tRange* wRange = m_Api->FindRangeNamed("FILTER_MULTI_VALUE");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Range should exist", wRange != nullptr);

	tRangeData* wRangeData = m_Api->RangeData("FILTER_MULTI_VALUE");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: RangeData should exist", wRangeData != nullptr);
	tColumnData* wColumn = wRangeData->FindColumnByIndex(0);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Column should exist", wColumn != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: filtervalues count",
	                       wColumn->FilterValues().size() == 2);

	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Row 1 (header) should be visible", wColRowCellRange->Row(1)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Row 2 (1961) should be visible", wColRowCellRange->Row(2)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Row 3 (1968) should be visible", wColRowCellRange->Row(3)->DataVisible() == true);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Row 4 (1975) should not be visible", wColRowCellRange->Row(4)->DataVisible() == false);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterMultiValueSameColumn: Row 5 (1961) should be visible", wColRowCellRange->Row(5)->DataVisible() == true);
}

void TestSkRangeData::TestRangeDataFilterPersistsAfterSort() {
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("A2", "Bob");
	m_Api->UndoCellValue("A3", "Alice");
	m_Api->UndoCellValue("A4", "Charlie");

	tString wJsonData = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\","
		"\"filterop\":\"Equals\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Alice\"},\"order\":\"Ascending\"}]}";

	tBool wResult = m_Api->UndoInsertRangeData("FILTER_SORT", "A1:A4", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: insert", wResult == true);

	tRange* wRange = m_Api->FindRangeNamed("FILTER_SORT");
	CPPUNIT_ASSERT(wRange != nullptr);
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();

	// Sort descending while keeping the same filter metadata (popup sort + active filter).
	tString wSortJson = "{\"columns\":[{\"index\":0,\"name\":\"Name\",\"type\":\"string\","
		"\"filterop\":\"Equals\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Alice\"},\"order\":\"Descending\"}]}";
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: apply sort",
		m_Api->UndoApplyRangeData("FILTER_SORT", wSortJson) == true);

	tRangeData* wRangeData = m_Api->RangeData("FILTER_SORT");
	CPPUNIT_ASSERT(wRangeData != nullptr);
	tColumnData* wColumn = wRangeData->FindColumnByIndex(0);
	CPPUNIT_ASSERT(wColumn != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: filter operator kept",
		wColumn->FilterOperator() == tFilterOperator::Equals);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: filter value kept",
		wColumn->FilterValue().String() == "Alice");

	for (tIndex wRow = 2; wRow <= 4; wRow++) {
		const tString wName = m_Api->CellValue(wRow, 1).String();
		const tBool wVisible = wColRowCellRange->Row(wRow)->DataVisible();
		if (wName == "Alice") {
			CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: Alice visible",
				wVisible == true);
		} else {
			CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterPersistsAfterSort: non-Alice hidden",
				wVisible == false);
		}
	}
}

void TestSkRangeData::TestRangeDataFilterRecalculatesSubtotal() {
	// Table with totals row: SUBTOTAL(109,[Ecart]) must refresh after ApplyRangeData filter.
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Ecart");
	m_Api->UndoCellValue("A2", "Alice");
	m_Api->UndoCellValue("B2", 10);
	m_Api->UndoCellValue("A3", "Bob");
	m_Api->UndoCellValue("B3", 20);
	m_Api->UndoCellValue("A4", "Alice");
	m_Api->UndoCellValue("B4", 30);

	tString wJsonData = "{\"lastrow\":true,\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\","
		"\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"None\"},"
		"{\"index\":1,\"name\":\"Ecart\",\"type\":\"double\",\"filterop\":\"None\","
		"\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"None\"}"
		"]}";

	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: insert",
		m_Api->UndoInsertRangeData("TBL_SUBTOTAL", "A1:B4", wJsonData) == true);

	tRangeData* wRangeData = m_Api->RangeData("TBL_SUBTOTAL");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: RangeData",
		wRangeData != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: HasTotals",
		wRangeData->HasTotals() == true);

	// Totals row is stored range bottom + 1 (row 5).
	tString wSubtotalFormula = "=SUBTOTAL(109;TBL_SUBTOTAL[[Ecart]])";
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: SUBTOTAL formula",
		m_Api->UndoCellValue("B5", wSubtotalFormula) == true);

	tVariant wVariant = m_Api->CellValue("B5");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: all rows visible",
		wVariant.IsDouble());
	CPPUNIT_ASSERT_DOUBLES_EQUAL(60.0, wVariant.Double(), 0.0001);

	tString wFilterJson = "{\"lastrow\":true,\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"Equals\","
		"\"filtervalue\":{\"t\":\"s\",\"v\":\"Alice\"},\"order\":\"None\"},"
		"{\"index\":1,\"name\":\"Ecart\",\"type\":\"double\",\"filterop\":\"None\","
		"\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"None\"}"
		"]}";
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: apply filter",
		m_Api->UndoApplyRangeData("TBL_SUBTOTAL", wFilterJson) == true);

	tRange* wRange = m_Api->FindRangeNamed("TBL_SUBTOTAL");
	CPPUNIT_ASSERT(wRange != nullptr);
	tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: Bob hidden",
		wColRowCellRange->Row(3)->DataVisible() == false);

	wVariant = m_Api->CellValue("B5");
	CPPUNIT_ASSERT_MESSAGE("TestRangeDataFilterRecalculatesSubtotal: filtered SUBTOTAL",
		wVariant.IsDouble());
	CPPUNIT_ASSERT_DOUBLES_EQUAL(40.0, wVariant.Double(), 0.0001);
}

void TestSkRangeData::TestTableFormula() {
	// Test formulas with table column references: tablename[columnName]
	
	// Create a RangeData table with named columns
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
        "{\"index\":3,\"name\":\"Solde d'ouverture\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	
	// Insert RangeData at A1:D5
	tBool wResult = m_Api->UndoInsertRangeData("EMPLOYEES", "A1:D5", wJsonData);
   
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Insert should succeed", wResult == true);
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("C1", "Salary");
	m_Api->UndoCellValue("D1", "Solde d'ouverture");
	
	// Verify the named range exists
	tRange* wRange = m_Api->FindRangeNamed("EMPLOYEES");
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Range should exist", wRange != nullptr);
	
	// Fill some test data
	m_Api->UndoCellValue("A2", "John");
	m_Api->UndoCellValue("B2", 30);
	m_Api->UndoCellValue("C2", 5000);
	
	m_Api->UndoCellValue("A3", "Jane");
	m_Api->UndoCellValue("B3", "=B2+1");
	m_Api->UndoCellValue("C3", 6000);
    
    tString wFormula11="=EMPLOYEES[[Name]]";
    //cout << "Compil " << wFormula0 << endl;
    tBool wCompilResult = m_Api->UndoCellValue("A11", wFormula11);
    if (!wCompilResult) {
      cout << m_Api->ErrorWithDetail() << endl;
    }
/*
    tCell* wCellA11=m_Api->Cell("A11");
    cout << wCellA11->Formula()->FormulaKey() << endl;
    cout << wCellA11->FormulaStr() << endl;
*/
    
    tString wFormula12="=EMPLOYEES[[#Headers]]";
    wCompilResult = m_Api->UndoCellValue("A12",wFormula12);
    if (!wCompilResult) {
      cout << m_Api->ErrorWithDetail() << endl;
    }
 
    
    tString wFormula13="=EMPLOYEES[[#Headers];[Name]]";
    wCompilResult = m_Api->UndoCellValue("A13",wFormula13);
    if (!wCompilResult) {
      cout << m_Api->ErrorWithDetail() << endl;
    }
    tString wFormula14="=EMPLOYEES[[#Headers];[Solde d'ouverture]]";
    wCompilResult = m_Api->UndoCellValue("A14",wFormula14);
    if (!wCompilResult) {
      cout << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Headers and apostrophe column", wCompilResult == true);
 /*
    cout << m_Api->Cell("A11")->FormulaStr() << endl;
    cout << m_Api->Cell("A12")->FormulaStr() << endl;
    cout << m_Api->Cell("A13")->FormulaStr() << endl;
    cout << m_Api->Cell("A14")->FormulaStr() << endl;


    tCell* wCellA14=m_Api->Cell("A14");
    cout << wCellA14->Formula()->FormulaKey() << endl;
    cout << wCellA14->FormulaStr() << endl;
    tWorkBook* wWorkBook=m_Api->ActiveWorkBook();
    #ifdef _DEBUG
        cout << wWorkBook->Debug() << endl;
    #endif
    
*/
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Column Name", "="+m_Api->Cell("A11")->FormulaStr() == wFormula11);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Headers",  "="+m_Api->Cell("A12")->FormulaStr() == wFormula12);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Headers and Column",  "="+m_Api->Cell("A13")->FormulaStr() == wFormula13);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Headers and apostrophe column formula",
                           "="+m_Api->Cell("A14")->FormulaStr() == wFormula14);
    
    
 	// tCell* wCellA10=m_Api->Cell("A10");
    // cout << endl;
    // cout << "D2=" << wCellA10->FormulaStr() << "=" << wCellA10->Value() << endl;
   
	// Test formula with single bracket: EMPLOYEES[Salary]
	// Use G2 (not D2): D1 gets =EMPLOYEES[[Salary]]*123 below and spills down column D, which replaces D2 as spill extend.
	// G2 avoids E1:F3 where SALARIES is inserted later.
	tString wFormula1 = "='EMPLOYEES'[[#This Row];[Salary]]+2";
	tBool wCompilResult1 = m_Api->UndoCellValue("G2", wFormula1);
    if (!wCompilResult1) {
        cout << m_Api->ErrorWithDetail() << endl;
    }
    /*
    tCell* wCellG2=m_Api->Cell("G2");
    cout << wCellG2->Formula()->FormulaKey() << endl;
    cout << wCellG2->FormulaStr() << endl;
    */
    if ( wFormula1 != "="+m_Api->Cell("G2")->FormulaStr()) {
        cout << endl << wFormula1 << endl;
        cout << "="<< m_Api->Cell("G2")->FormulaStr() << endl;
    }
    //cout << endl << m_Api->Cell("D2")->FormulaStr() << endl;
    
    wFormula11="='EMPLOYEES'[[#Totals];[Age]]";
    wCompilResult1 = m_Api->UndoCellValue("E2", wFormula11);
    if (!wCompilResult1) {
        cout << m_Api->ErrorWithDetail() << endl;
    }
/*
    tCell* wCellE2=m_Api->Cell("E2");
    cout << wCellE2->Formula()->FormulaKey() << endl;
    cout << wCellE2->FormulaStr() << endl;
*/
    wFormula12="=SUM(A1:C1)";
    
    
    wCompilResult1 = m_Api->UndoCellValue("E3", wFormula12);
    if (!wCompilResult1) {
        cout << m_Api->ErrorWithDetail() << endl;
    }
  
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula with single bracket should compile", wCompilResult1 == true);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula equality 2",
                           wFormula1 == "="+m_Api->Cell("G2")->FormulaStr());
	
	// Test formula with double bracket: EMPLOYEES[[Salary]]
	tString wFormula2 = "=EMPLOYEES[[Salary]]*123";
	tBool wCompilResult2 = m_Api->UndoCellValue("D1", wFormula2);
    if (!wCompilResult2) {
        cout << m_Api->ErrorWithDetail() << endl;
    }   
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula with double bracket should compile", wCompilResult2 == true);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula equality  1",
                           wFormula2 == "="+ m_Api->Cell("D1")->FormulaStr());
  
    
       
	// Test formula with quoted table name: 'EMPLOYEES'['Salary']
	tString wFormula3 = "='EMPLOYEES'[Salary]";
	tBool wCompilResult3 = m_Api->UndoCellValue("D3", wFormula3);
    if (!wCompilResult3) {
        cout << m_Api->ErrorWithDetail() << endl;
    }
/*
    tCell* wCellD3=m_Api->Cell("D3");
    cout << wCellD3->Formula()->FormulaKey() << endl;
    cout << m_Api->Cell("D3")->FormulaStr() << endl;
*/
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula with quoted table and column should compile", wCompilResult3 == true);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula equality  3",
                           wFormula3 == "="+m_Api->Cell("D3")->FormulaStr());
    //tCell* wCellD3=m_Api->Cell("D3");
    //cout << "D3 <"   << wCellD3->FormulaStr() << ">=" << wCellD3->Value() << endl;
    
	// Test formula with column name containing spaces: EMPLOYEES['Salary Amount']
	// First, we need to create a column with spaces in the name
	tString wJsonData2 = "{\"columns\":["
		"{\"index\":0,\"name\":\"Full Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Salary Amount\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	
	tBool wResult2 = m_Api->UndoInsertRangeData("SALARIES", "E1:F3", wJsonData2);
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Insert SALARIES should succeed", wResult2 == true);
	m_Api->UndoCellValue("E1", "Full Name");
	m_Api->UndoCellValue("F1", "Salary Amount");
	
	// Fill test data
	m_Api->UndoCellValue("E2", "John Doe");
	m_Api->UndoCellValue("F2", 5000);
	
	// Test formula with column name containing spaces
	tString wFormula4 = "=SALARIES['Salary Amount']";
    //tString wFormula4 = "=SALARIES['Salary Amunt']";
	tBool wCompilResult4 = m_Api->UndoCellValue("G1", wFormula4);
    if (!wCompilResult4) {
        //cerr << m_Api->Error() << m_Api->ErrorWithDetail() << endl;
    }
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula with column name containing label should not compile", wCompilResult4 == false);
	
	// Verify formulas are stored correctly
	tString wStoredFormula1 = m_Api->Formula(1, 4); // D1
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Formula should be stored correctly", 
		wStoredFormula1.find("EMPLOYEES") != tString::npos);
	
	tString wStoredFormula2 = m_Api->Formula(2, 7); // G2 (double-bracket formula; D2 may be D1 spill extend)
	CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Double bracket formula should be stored correctly", 
		wStoredFormula2.find("EMPLOYEES") != tString::npos);
	
	DrawCell("After TestTableFormula", 1, 1, 5, 7);
 
    m_Api->EnsureRangeNamed("MyRange", "B10");
    // Match WriteJson/ReadJson (SKApi forces "us" for serialization) so FormulaStr() before/after round-trip is identical.
    tApplication::Instance()->Locale("us");
    // H2 not D2: D1 =EMPLOYEES[[Salary]]*123 spills down column D; after ReadJson recalc, spill SetValue clears extend cells and D2 loses its formula.
    tString wFormula5 = "=SUM('EMPLOYEES'[@[Age]:[Salary]];MyRange)";
	tBool wCompilResult5 = m_Api->UndoCellValue("H2", wFormula5);
    if (!wCompilResult5) {
        cout << m_Api->ErrorWithDetail() << endl;
    }
   
    tCell* wCellH2=m_Api->Cell("H2");
    tString wFormula5After = wCellH2->FormulaStr();
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: Double bracket formula with range should be stored correctly",
                           wFormula5After.find("EMPLOYEES") != tString::npos && wFormula5After.find("MyRange") != tString::npos);
 
    tString wJson=m_Api->WriteJson("www.skeema.fr/test");
    
    tearDown();
    setUp();
    tApplication::Instance()->Locale("us");
    m_Api->ReadJson(wJson);
   
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: After read  ",
                           m_Api->Cell("H2")->FormulaStr() == wFormula5After);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: After read A14 apostrophe header",
                           m_Api->Cell("A14")->FormulaStr() == wFormula14.substr(1));
    CPPUNIT_ASSERT_MESSAGE("TestTableFormula: After read A14 compiles",
                           m_Api->Cell("A14")->Formula() != nullptr);

    tApplication::Instance()->Locale("fr");
}

void TestSkRangeData::TestTableJsonSaveAndLoad() {
	// Test saving and loading a table to/from JSON
	
	// Create a RangeData table with named columns
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":3,\"name\":\"Solde d'ouverture\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	
	// Insert RangeData at A1:D5 (UndoInsertRangeData attaches metadata; it does not fill header cells)
	tBool wResult = m_Api->UndoInsertRangeData("EMPLOYEES", "A1:D5", wJsonData);
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Insert should succeed", wResult == true);
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("C1", "Salary");
	m_Api->UndoCellValue("D1", "Solde d'ouverture");
	
	// Save the table to JSON
	tString wJson = m_Api->WriteJson("www.skeema.fr/test");
	
    tearDown();
    setUp();
	
	m_Api->ReadJson(wJson);

	// Round-trip preserves named range and RangeData column metadata, not cell formulas unless set explicitly
	tRange* wRange = m_Api->FindRangeNamed("EMPLOYEES");
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Named range after read", wRange != nullptr);
	tSheet* wSheet = m_Api->ActiveSheet();
	const tIndex wHeaderRow = wRange->TopIndex();
	const tIndex wLeft = wRange->LeftIndex();

	tRangeData* wRangeData = m_Api->RangeData("EMPLOYEES");
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: RangeData after read",
	                       wRangeData != nullptr && !wRangeData->IsEmpty());

	tColumnData* wCol1 = wRangeData->FindColumnByIndex(0);
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Column 1",
	                       wCol1 != nullptr && wCol1->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Name");
	tColumnData* wCol2 = wRangeData->FindColumnByIndex(1);
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Column 2",
	                       wCol2 != nullptr && wCol2->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Age");
	tColumnData* wCol3 = wRangeData->FindColumnByIndex(2);
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Column 3",
	                       wCol3 != nullptr && wCol3->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Salary");
	tColumnData* wCol4 = wRangeData->FindColumnByIndex(3);
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Column 4",
	                       wCol4 != nullptr && wCol4->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Solde d'ouverture");
	CPPUNIT_ASSERT_MESSAGE("TestTableJsonSaveAndLoad: Column with apostrophe by name",
	                       wRangeData->FindColumnByName(wSheet, wRange, "Solde d'ouverture") != nullptr);
	
}

void TestSkRangeData::TestTableHeaderDuplicateNameRejected() {
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";

	CPPUNIT_ASSERT(m_Api->UndoInsertRangeData("EMPLOYEES", "A1:C5", wJsonData));
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("C1", "Salary");

	tRange* wRange = m_Api->FindRangeNamed("EMPLOYEES");
	tSheet* wSheet = m_Api->ActiveSheet();
	const tIndex wHeaderRow = wRange->TopIndex();
	const tIndex wLeft = wRange->LeftIndex();
	tRangeData* wRangeData = m_Api->RangeData("EMPLOYEES");

	CPPUNIT_ASSERT(!m_Api->UndoCellValue("B1", "Name"));
	CPPUNIT_ASSERT(wRangeData->FindColumnByName(wSheet, wRange, "Age") != nullptr);
	CPPUNIT_ASSERT(wRangeData->FindColumnByName(wSheet, wRange, "Name") != nullptr);
	CPPUNIT_ASSERT(wRangeData->FindColumnByOrdinal(wLeft, 1)->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Age");

	CPPUNIT_ASSERT(m_Api->UndoCellValue("B1", "Years"));
	CPPUNIT_ASSERT(wRangeData->FindColumnByName(wSheet, wRange, "Years") != nullptr);
	CPPUNIT_ASSERT(wRangeData->FindColumnByOrdinal(wLeft, 1)->HeaderLabel(wSheet, wHeaderRow, wLeft) == "Years");
}

void TestSkRangeData::TestInsertRowAppliesCalculatedColumnFormula() {
	// calculatedColumnFormula on Total + existing row formulas; insert must materialize C3 with deps.
	tString wJsonData = "{\"firstrow\":true,\"columns\":["
		"{\"index\":1,\"name\":\"Qty\",\"type\":\"int\"},"
		"{\"index\":2,\"name\":\"Price\",\"type\":\"double\"},"
		"{\"index\":3,\"name\":\"Total\",\"type\":\"double\","
		"\"calculatedColumnFormula\":\"Tcalc[[#This Row],[Qty]]*Tcalc[[#This Row],[Price]]\"}"
		"]}";
	CPPUNIT_ASSERT_MESSAGE("TestInsertRowAppliesCalculatedColumnFormula: create table",
						   m_Api->UndoInsertRangeData("Tcalc", "A1:C3", wJsonData));
	m_Api->UndoCellValue("A1", "Qty");
	m_Api->UndoCellValue("B1", "Price");
	m_Api->UndoCellValue("C1", "Total");
	m_Api->UndoCellValue("A2", 2);
	m_Api->UndoCellValue("B2", 10);
	CPPUNIT_ASSERT(m_Api->UndoCellValue(
		"C2", "=Tcalc[[#This Row];[Qty]]*Tcalc[[#This Row];[Price]]"));
	m_Api->UndoCellValue("A3", 3);
	m_Api->UndoCellValue("B3", 20);
	CPPUNIT_ASSERT(m_Api->UndoCellValue(
		"C3", "=Tcalc[[#This Row];[Qty]]*Tcalc[[#This Row];[Price]]"));

	tRangeData* wData = m_Api->RangeData("Tcalc");
	CPPUNIT_ASSERT(wData != nullptr);
	tColumnData* wTotalCol = wData->FindColumnByOrdinal(1, 2);
	CPPUNIT_ASSERT(wTotalCol != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRowAppliesCalculatedColumnFormula: template stored",
						   !wTotalCol->CalculatedColumnFormula().empty());

	CPPUNIT_ASSERT(m_Api->UndoInsertRow(3, 1));
	tCell* wC3 = m_Api->Cell("C3");
	CPPUNIT_ASSERT_MESSAGE("TestInsertRowAppliesCalculatedColumnFormula: formula on new row",
						   wC3 != nullptr && wC3->Formula() != nullptr);
	CPPUNIT_ASSERT_MESSAGE("TestInsertRowAppliesCalculatedColumnFormula: VectorRef deps",
						   wC3->VectorRef() != nullptr && !wC3->VectorRef()->empty());
}

void TestSkRangeData::TestTableFormulaInPlace() {
	// Test formulas with table column references: tablename[@[columnName]]
	
	// Create a RangeData table with named columns
	tString wJsonData = "{\"columns\":["
		"{\"index\":0,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":1,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
		"{\"index\":2,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
        "{\"index\":3,\"name\":\"Solde d'ouverture\",\"type\":\"double\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}"
		"]}";
	
	// Insert RangeData at A1:D5
	tBool wResult = m_Api->UndoInsertRangeData("EMPLOYEES", "A1:D5", wJsonData);
	m_Api->UndoCellValue("A1", "Name");
	m_Api->UndoCellValue("B1", "Age");
	m_Api->UndoCellValue("C1", "Salary");
	m_Api->UndoCellValue("D1", "Solde d'ouverture");
 
    m_Api->UndoCellValue("B2", 1);
    m_Api->UndoCellValue("B3", 2);
    m_Api->UndoCellValue("B4", 3);
    m_Api->UndoCellValue("B5", 4);
    
    m_Api->UndoCellValue("C2", 10);
    m_Api->UndoCellValue("C3", 20);
    m_Api->UndoCellValue("C4", 30);
    m_Api->UndoCellValue("C5", 40);
    
	CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace: Insert should succeed", wResult == true);
    // Cell
    tString wFormula1 = "=[@[Salary]]";
	tBool wCompilResult1 = m_Api->UndoCellValue("D5", wFormula1);
 
    if (!wCompilResult1) {
        CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary]", wResult == true);
    }
    
    m_Api->UndoCellValue("A1","=ROW()+1");
    
    //cout << m_Api->Cell("B2")->FormulaStr() << endl;
    //cout << m_Api->Cell("B2")->FormulaStr(false,true) << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary]",
                           "EMPLOYEES[[#This Row][Salary]]"==m_Api->Cell("D5")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary] sUser",
                           "[@[Salary]]"==m_Api->Cell("D5")->FormulaStr(false,true));
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : sUser no semicolon after @",
                           m_Api->Cell("D5")->FormulaStr(false,true).find("[@;[") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : Api Formula user",
                           "[@[Salary]]"==m_Api->Formula("D5", nullptr, true));

    tString wFormulaAtShorthand = "=@[Salary]";
    tBool wCompilAtShorthand = m_Api->UndoCellValue("D4", wFormulaAtShorthand);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =@[Salary] shorthand", wCompilAtShorthand == true);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =@[Salary] canonical",
                           "[@[Salary]]"==m_Api->Cell("D4")->FormulaStr(false, true));

    tString wFormulaAtNoBrackets = "=[@Salary]";
    tBool wCompilAtNoBrackets = m_Api->UndoCellValue("C5", wFormulaAtNoBrackets);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary]", wCompilAtNoBrackets == true);
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary] canonical",
                           "[@[Salary]]"==m_Api->Cell("C5")->FormulaStr(false, true));

    // Range
    tString wFormula2 = "=SUM([[Age]])";
	tBool wCompilResult2 = m_Api->UndoCellValue("D2", wFormula2);
 
    if (!wCompilResult2) {
        CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =SUM([[Age]])", wResult == true);
    }
    //cout << m_Api->Cell("B3")->FormulaStr() << endl;
    //cout << m_Api->Cell("B3")->FormulaStr(false,true) << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[Age]]",
                           "SUM(EMPLOYEES[[Age]])"==m_Api->Cell("D2")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary] sUser",
                           "SUM([[Age]])"==m_Api->Cell("D2")->FormulaStr(false,true));
    
    tString wFormula3 = "=SUM([@[Age]:[Salary]])";
	tBool wCompilResult3 = m_Api->UndoCellValue("D3", wFormula3);
 
    if (!wCompilResult3) {
        CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : ==SUM([@[Age]:[Salary]])", wResult == true);
    }
    //cout << m_Api->Cell("B4")->FormulaStr() << endl;
    //cout << m_Api->Cell("B4")->FormulaStr(false,true) << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM([@[Age]:[Salary]])",
                           "SUM(EMPLOYEES[[#This Row][Age]:[Salary]])"==m_Api->Cell("D3")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM([@[Age]:[Salary]]) sUser",
                           "SUM([@[Age]:[Salary]])"==m_Api->Cell("D3")->FormulaStr(false,true));
      
    // Another Sheete
    m_Api->AddSheet("Sheet2");
    m_Api->ActiveSheet("Sheet2");
    
    tString wFormula4="=SUM(EMPLOYEES[[Age]])";
    
    tBool wCompilResult4=m_Api->UndoCellValue("A1", wFormula4);
    if (!wCompilResult4) {
        CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =Sheet1!EMPLOYEES[[Age]", wResult == true);
    }
    //cout << m_Api->Cell("A1")->Debug() << endl;
    //cout << m_Api->Cell("A1")->Value() << endl;
    //cout << m_Api->Cell("A1")->FormulaStr() << endl;
    //cout << m_Api->Cell("A1")->FormulaStr(false,true) << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM(Sheet1!EMPLOYEES[[Age]])",
                           "SUM(EMPLOYEES[[Age]])"==m_Api->Cell("A1")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM(Sheet1!EMPLOYEES[[Age]])",
                           "SUM(EMPLOYEES[[Age]])"==m_Api->Cell("A1")->FormulaStr(false,true));
      
    tApplication::Instance()->Locale("fr");
    	
	// Save the table to JSON
	tString wJson = m_Api->WriteJson("www.skeema.fr/test");
	
    tearDown();
    setUp();
	
    tApplication::Instance()->Locale("fr");
	m_Api->ReadJson(wJson);
     m_Api->ActiveSheet("Sheet1");
       
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary]",
                           "EMPLOYEES[[#This Row][Salary]]"==m_Api->Cell("D5")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : =[@Salary] sUser",
                           "[@[Salary]]"==m_Api->Cell("D5")->FormulaStr(false,true));

    // Formula-pick CellRef: same-row / full-column structured refs for the UI.
    CPPUNIT_ASSERT_MESSAGE("CellRef same-row single",
                           "@[Salary]"==m_Api->CellRef("D5", "C5"));
    CPPUNIT_ASSERT_MESSAGE("CellRef same-row columns",
                           "[@[Age]:[Salary]]"==m_Api->CellRef("D5", "B5:C5"));
    CPPUNIT_ASSERT_MESSAGE("CellRef full column inside table",
                           "[[Salary]]"==m_Api->CellRef("D5", "C2:C5"));
    CPPUNIT_ASSERT_MESSAGE("CellRef outside table stays A1",
                           "E5"==m_Api->CellRef("D5", "E5"));
    // Edit cell outside the table, pick a full data column → Table[[Col]].
    m_Api->UndoCellValue("F10", 0);
    CPPUNIT_ASSERT_MESSAGE("CellRef from outside table full column",
                           "EMPLOYEES[[Salary]]"==m_Api->CellRef("F10", "C2:C5"));
                           
    m_Api->ActiveSheet("Sheet2");

    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM(Sheet1!EMPLOYEES[[Age]])",
                           "SUM(EMPLOYEES[[Age]])"==m_Api->Cell("A1")->FormulaStr());
                           
    CPPUNIT_ASSERT_MESSAGE("TestTableFormulaInPlace : SUM(Sheet1!EMPLOYEES[[Age]])",
                           "SUM(EMPLOYEES[[Age]])"==m_Api->Cell("A1")->FormulaStr(false,true));
}

void TestSkRangeData::TestTableColonColumnIsFullColumn() {
	// League-Table-Examples Part A: INDEX(TableA1[P], MATCH(POS, TableA1[[RANK]:[RANK]], 0)).
	// [[RANK]:[RANK]] must be the whole RANK column, not the RANK cell on the formula row.
	tString wJson = "{\"firstrow\":true,\"columns\":["
		"{\"index\":0,\"name\":\"TEAM\",\"type\":\"string\"},"
		"{\"index\":1,\"name\":\"RANK\",\"type\":\"int\"},"
		"{\"index\":2,\"name\":\"P\",\"type\":\"int\"}"
		"]}";
	CPPUNIT_ASSERT(m_Api->UndoInsertRangeData("TableA1", "A1:C4", wJson));
	m_Api->UndoCellValue("A1", "TEAM");
	m_Api->UndoCellValue("B1", "RANK");
	m_Api->UndoCellValue("C1", "P");
	m_Api->UndoCellValue("A2", "Newcastle");
	m_Api->UndoCellValue("B2", 8);
	m_Api->UndoCellValue("C2", 38);
	m_Api->UndoCellValue("A3", "Liverpool");
	m_Api->UndoCellValue("B3", 1);
	m_Api->UndoCellValue("C3", 38);
	m_Api->UndoCellValue("A4", "Arsenal");
	m_Api->UndoCellValue("B4", 2);
	m_Api->UndoCellValue("C4", 36);

	// POS=1 is Liverpool (row 3), not the RANK on the formula row (row 6 is empty / outside).
	CPPUNIT_ASSERT_MESSAGE("compile MATCH [[RANK]:[RANK]]",
		m_Api->UndoCellValue("E2", "=INDEX(TableA1[P];MATCH(1;TableA1[[RANK]:[RANK]];0))"));
	tVariant wE2 = m_Api->CellValue(2, 5);
	CPPUNIT_ASSERT_MESSAGE("MATCH full column finds RANK=1 -> Liverpool P=38",
		wE2.IsNumeric() && wE2.Numeric() == 38.0);

	CPPUNIT_ASSERT_MESSAGE("compile MATCH RANK=2",
		m_Api->UndoCellValue("E3", "=INDEX(TableA1[TEAM];MATCH(2;TableA1[[RANK]:[RANK]];0))"));
	tVariant wE3 = m_Api->CellValue(3, 5);
	CPPUNIT_ASSERT_MESSAGE("MATCH RANK=2 -> Arsenal",
		wE3.IsString() && wE3.String() == "Arsenal");

	// [#This Row],[Age]:[Salary] must stay a single row (existing InPlace contract).
	CPPUNIT_ASSERT_MESSAGE("compile This Row colon",
		m_Api->UndoCellValue("D2", "=SUM(TableA1[[#This Row];[RANK]:[P]])"));
	tVariant wD2 = m_Api->CellValue(2, 4);
	CPPUNIT_ASSERT_MESSAGE("This Row [RANK]:[P] is 8+38",
		wD2.IsNumeric() && wD2.Numeric() == 46.0);
}

void TestSkRangeData::TestTableIndexMatchThisRowPerRow() {
	// League-Table Part A Table A3 / OrderedTable3: every data row looks up POS in RANK.
	// First row POS=1 must not be the only one that resolves.
	tString wJsonA1 = "{\"firstrow\":true,\"columns\":["
		"{\"index\":0,\"name\":\"TEAM\",\"type\":\"string\"},"
		"{\"index\":1,\"name\":\"RANK\",\"type\":\"int\"}"
		"]}";
	CPPUNIT_ASSERT(m_Api->UndoInsertRangeData("TableA1", "A1:B4", wJsonA1));
	m_Api->UndoCellValue("A1", "TEAM");
	m_Api->UndoCellValue("B1", "RANK");
	m_Api->UndoCellValue("A2", "Newcastle");
	m_Api->UndoCellValue("B2", 8);
	m_Api->UndoCellValue("A3", "Liverpool");
	m_Api->UndoCellValue("B3", 1);
	m_Api->UndoCellValue("A4", "Arsenal");
	m_Api->UndoCellValue("B4", 2);

	tString wJsonA3 = "{\"firstrow\":true,\"columns\":["
		"{\"index\":0,\"name\":\"POS\",\"type\":\"int\"},"
		"{\"index\":1,\"name\":\"TEAM\",\"type\":\"string\"}"
		"]}";
	CPPUNIT_ASSERT(m_Api->UndoInsertRangeData("TableA3", "D1:E3", wJsonA3));
	m_Api->UndoCellValue("D1", "POS");
	m_Api->UndoCellValue("E1", "TEAM");
	m_Api->UndoCellValue("D2", 1);
	m_Api->UndoCellValue("D3", 2);

	const tString wLookup =
		"=INDEX(TableA1[TEAM];MATCH(TableA3[[#This Row];[POS]:[POS]];TableA1[[RANK]:[RANK]];0))";
	CPPUNIT_ASSERT_MESSAGE("compile TableA3 row 1", m_Api->UndoCellValue("E2", wLookup));
	CPPUNIT_ASSERT_MESSAGE("compile TableA3 row 2", m_Api->UndoCellValue("E3", wLookup));

	tVariant wE2 = m_Api->CellValue(2, 5);
	tVariant wE3 = m_Api->CellValue(3, 5);
	CPPUNIT_ASSERT_MESSAGE("POS 1 -> Liverpool", wE2.IsString() && wE2.String() == "Liverpool");
	CPPUNIT_ASSERT_MESSAGE("POS 2 -> Arsenal (not #N/A)", wE3.IsString() && wE3.String() == "Arsenal");
}

void TestSkRangeData::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Application = tApplication::Instance();
    m_Application->Locale("fr");
	m_NbRow = 10;
	m_NbCol = 5;
	m_Api = new tApi;
	m_Api->NewWorkBook("www.skeema.fr/test");
}

void TestSkRangeData::tearDown() {
	delete(m_Api);
}

