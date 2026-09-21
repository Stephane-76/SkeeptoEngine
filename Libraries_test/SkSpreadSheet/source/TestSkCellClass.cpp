//==============================================================================
// TestSkCellClass
//==============================================================================
#include "../include/TestSkCellClass.hpp"


// We can send it to the API of a feature =====================================
TestSkCellClass::TestSkCellClass() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkCellClass::DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkCellClass::UndoOperation() {
	//return; // Drop
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}

void TestSkCellClass::SetValueMoney(tString sRef,t_UnitMoney sUnitMoney) {
    tClassUnit wClassUnit(sUnitMoney);
    tVariant wValue; // Empty for replace
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}

void TestSkCellClass::SetValueMoney(tString sRef,t_UnitMoney sUnitMoney,tDouble sValue) {
    tClassUnit wClassUnit(sUnitMoney);
    tVariant wValue=sValue;
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}
void TestSkCellClass::SetValueLength(tString sRef,t_UnitLength sUnitLength,tDouble sValue) {
    tClassUnit wClassUnit(sUnitLength);
    tVariant wValue=sValue;
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}

void  TestSkCellClass::DebugUnit(tString sRef) {
    return; //Drop
    cout << Str(sRef);
    cout << endl;
}

tString TestSkCellClass::Str(tString sRef) {
    tStringStream wStream;
    tCell* wCell=m_Api->Cell(sRef);
    wStream <<  sRef;
    if (wCell!=nullptr) {
        if (wCell->FormulaStr()!="") wStream << ":(" << wCell->FormulaStr() << ")";

        if (wCell->Value().IsClass()) {
            tCellClassUnit* wCellClassUnit=dynamic_cast<tCellClassUnit*>(wCell->Value().Class());
            if (wCellClassUnit!=nullptr) {
                wStream << "=" <<wCellClassUnit->Str();
            } else {
#ifdef _DEBUG
                wStream <<wCell->Value().Class()->Debug();
#endif
            }
        } else {
            wStream << "=" << wCell->Value();
        }
    }
    return(wStream.str());
}

void TestSkCellClass::Fill() {
	tInt wNbRow = 8;
	tInt wNbCol = 10;
	tCell* wCell;
	for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
		if (wCol > 1) {
			wCell = m_Api->EnsureCell(1, wCol);
			tStringStream wStream;
			wStream << Base10ToAlpha(wCol - 1) << wNbRow - 1 << "+1";
            tString wCompilString=wStream.str();
            if (!m_Api->CompilCell(wCell,wCompilString.c_str())) {
                tStringStream wStreamError;
                wStreamError << "Test Calculate Compil error " << wStream.str() << " " << wCompilString;
                CPPUNIT_ASSERT_MESSAGE(wStreamError.str(), false);
            }
		}
	}

	for (tInt wRow = 2; wRow <= wNbRow; wRow++) {
		for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
			wCell = m_Api->EnsureCell(wRow, wCol);
			tStringStream wStream;
			if (wRow == wNbRow) {
				if (wCol != wNbCol) {
					wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
					//wStream << "1";
				}
				else {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(wNbCol - 1) << wRow << ")";
					//wStream << "1";
				}
			}
			else {
				if (wCol == wNbCol) {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(wNbCol - 1) << wRow << ")";
					//wStream << "1";
				}
				else {
					wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
				}
			}
            tString wCompilString=wStream.str();
            if (!m_Api->CompilCell(wCell,wCompilString.c_str())) {
                tStringStream wStreamError;
                wStreamError << "Test Calculate Compil error " << wStream.str() << " " << wCompilString;
                CPPUNIT_ASSERT_MESSAGE(wStreamError.str(), false);
            }
#ifdef checksp
			wCell->Check();
#endif
		}
	}

#ifdef checksp	
	m_Api->Check();
#endif
	wCell= m_Api->EnsureCell(1, 1);
#ifdef checksp
	wCell->Check();
#endif // checksp
	wCell->Calculation();

	DrawCell(1, 1, wNbRow, wNbCol);
	tVariant wResult = m_Api->CellValue(8, 10);
	CPPUNIT_ASSERT_MESSAGE("CellCalculate ", wResult.Int() == 1953);

	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, wNbRow, wNbCol));
	DrawCell(1, 1, wNbRow, wNbCol);
	wResult=m_Api->CellValue(8, 10);
	CPPUNIT_ASSERT_MESSAGE("CellCalculate Rect ", wResult.Int() == 1953);
	//J8 = 1953
}

void TestSkCellClass::TestInstance() {
    const tClassUnit wClassUnit(t_UnitMoney::gpb);
    const tVariant wValue=12;
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
   
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass("A1", &wUnitValue);
    DebugUnit("A1");
    
    m_Api->UndoCellClass("A2:Z2", &wUnitValue);
    
    //cout << endl << m_Api->Cell("A1")->Value() << endl;
}

void TestSkCellClass::TestCopy() {
    SetValueMoney("A2:Z2", t_UnitMoney::eur, 12);
    
    
    m_Api->Copy("A2:Z2");
    
    m_Api->UndoPaste("B2");
    
    //cout << endl << m_Api->Cell("J2")->Value().Class()->Debug() << endl;
}

void TestSkCellClass::TestCalculate() {
    
    SetValueMoney("B1", t_UnitMoney::usd, 12);
    DebugUnit("B1");
    //cout << endl << Str("B1")  << endl;
    CPPUNIT_ASSERT_MESSAGE("CellCalculate B1", Str("B1")  == "B1=12.00 $");
    
    m_Api->UndoCellValue("A2", "=A1/B1");
    DebugUnit("A2");
    //cout << endl << Str("A2")  << endl;
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A2", Str("A2")  == "A2:(A1/B1)=0.00 $");

    SetValueMoney("A1", t_UnitMoney::eur, 6);
    DebugUnit("A1");
    DebugUnit("A2");
    
    m_Api->Undo();
    m_Api->Redo();

    m_Api->UndoCellValue("A2", "=A1/B1");
    DebugUnit("A2");
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A2", Str("A2")  == "A2:(A1/B1)=0.50 €/$");
    
 
    // Divide Same Unit Return variant without CellClassUnit
    SetValueMoney("B1", t_UnitMoney::eur, 6);
    DebugUnit("A2");
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A2", Str("A2")  == "A2:(A1/B1)=1.00");
    
    SetValueMoney("B1", t_UnitMoney::usd, 12);
    // Test
    m_Api->UndoCellValue("B1", 12);
    DebugUnit("B1");
    
    tString wResult=m_Api->JsonView(1, 3, tUnitMetrics::pixels, 12, 300,-10,-20, true);
    //cout << wResult << endl;
 
    SetValueLength("A3",t_UnitLength::meter,30);
    DebugUnit("A3");
    
    DebugUnit("A1");
    
    m_Api->UndoCellValue("A4", "=A1/A3");
    DebugUnit("A4");
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A4", Str("A4")  == "A4:(A1/A3)=0.20 €/m");
    
    m_Api->UndoCellValue("A5", "=A3*A3");
    DebugUnit("A5");
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A5", Str("A5")  == "A5:(A3*A3)=900.00 m2");
    
    DebugUnit("A1");
    m_Api->UndoCellValue("A6", "=A5/A1");
    DebugUnit("A6");
  
    m_Api->UndoCellValue("A7", "=A6/A3");
    DebugUnit("A7");
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A7", Str("A7")  == "A7:(A6/A3)=5.00 m/€");
    
 
    m_Api->UndoCellValue("A8", "=(3+A6)/12");
    DebugUnit("A8");
    
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A8", Str("A8")  == "A8:((3+A6)/12)=12.75 m2/€");
    
    m_Api->UndoCellValue("A9", "=A8/A3/A3");
    DebugUnit("A9");
    //cout << Str("A9") << endl;
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A8", Str("A9") == "A9:(A8/A3/A3)=0.01 /€");
    m_Api->UndoCellValue("A10", "=A7*A4");
    DebugUnit("A10");
    //cout << Str("A10") << endl;
    CPPUNIT_ASSERT_MESSAGE("CellCalculate A10", Str("A10")  == "A10:(A7*A4)=1.00");
    
    m_Api->UndoCellValue("A11", "=A4*A7");
    DebugUnit("A11");
    
    m_Api->UndoCellValue("A12", "=A7/A1");
    DebugUnit("A12");
    
    m_Api->UndoCellValue("A13", "=A7*A7");
    DebugUnit("A13");
    
    m_Api->UndoCellValue("C4", 2);
    m_Api->UndoCellValue("C5", 3);
    m_Api->UndoCellValue("C6", 4);
    
    m_Api->UndoCellValue("C3", 1);
    
    SetValueMoney("C3:C6",t_UnitMoney::usd);
    
    m_Api->UndoCellValue("D4","=SUM(C4:C6)");
    DebugUnit("D4");
    
    DebugUnit("C3");
    m_Api->UndoCellValue("C3", 12);
    DebugUnit("C3");
    
    m_Api->UndoApplyUnit("C3:C6;C3:C20","Monetary", "eur");
    DebugUnit("C3");
    
    m_Api->Undo();
    DebugUnit("C3");
    m_Api->Undo();
    DebugUnit("C3");
    m_Api->Undo();
    DebugUnit("C3");
    
    SetValueMoney("C3", t_UnitMoney::eur, 6);
    SetValueMoney("C4", t_UnitMoney::usd, 8);
    
    m_Api->UndoCellValue("A1", "=C3+C4");
    DebugUnit("A1");
    wResult=m_Api->JsonView(1, 1, tUnitMetrics::pixels, 100, 100,-10,-20, true);
    //cout << wResult << endl;
}

void TestSkCellClass::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkCellClass::tearDown() {
	delete(m_Api);
}
