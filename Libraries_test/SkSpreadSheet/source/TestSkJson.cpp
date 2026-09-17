//==============================================================================
// TestSkJon
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkJon.hpp"

// We can send it to the API of a feature 
TestSkJson::TestSkJson() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkJson::DrawCell(tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << sTitle << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}


void TestSkJson::Fill() {
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
					//wStream << "1";
				}
				else {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
					//wStream << "1";
				}
			}
			else {
				if (wCol == m_NbCol) {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
					//wStream << "1";
				}
				else {
					wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
				}
			}
			m_Api->CompilCell(wCell, wStream.str().c_str());

		}
	}
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("Fill",1, 1, m_NbRow, m_NbCol);
}

void TestSkJson::Write() {
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/json");
	Fill();
 
    m_Api->UndoCellValue("Z1", "Str1");
    m_Api->UndoCellValue("Z2", "Str1");
    m_Api->UndoCellValue("Z3", "Str1");
    m_Api->UndoInsertRangeNamed("COUCOU", "A1:A2");
    m_Api->UndoApplyMerge("B3:Z5");
    tString wJson =m_Api->WriteJson("wwww.skeema.fr/json");
    //cout << wJson << endl;
	tFile wFile("Test.json");
	wFile.SaveString(wJson);
#ifdef checksp	
	m_Api->Check();
#endif
   
     
    DrawCell("Write", 1, 1, m_NbRow, m_NbCol);
}

void TestSkJson::Read() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Api = new tApi;

	tFile wFile("Test.json");
	tString wJson = wFile.LoadString();
    //cout << wJson;
	m_Api->ReadJson(wJson);
    
	//m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("Read", 1, 1, m_NbRow, m_NbCol);

	tVariant wResult = m_Api->CellValue(m_NbRow, m_NbCol);
	CPPUNIT_ASSERT_MESSAGE("CellCalculate ", wResult.Int() == 630);

#ifdef checksp	
	m_Api->Check();
#endif
   
}

void TestSkJson::ReadMultiSheet() {
    m_Api = new tApi;
    m_Api->AddWorkBook("Test1");
    m_Api->AddSheet("Sheet1");
    Fill();

    // Just xcode
    tString wFileName="/Users/stephaneallez/Projects/Excel/Budget.json";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        m_Api->ReadJson(wJson);
        m_Api->ActiveSheet("TestIndirection");
        
        m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
        DrawCell("Read MultiSheet", 1, 1, 5, 1);
        
        
        m_Api->DeleteRow(1, 3);
        
        m_Api->Undo();
#ifdef checksp
        m_Api->Check();
#endif
    } else {
        //cout << wFileName << "Dont't exist" << endl;
    }
}

void TestSkJson::Excel() {
    m_Api = new tApi;
#ifdef SKER_FILE_DIR
    tString wFileName = tString(SKER_FILE_DIR) + "/Budget.sker";
#else
    tString wFileName="/Users/stephaneallez/Projects/Excel/Budget.sker";
#endif
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        m_Api->ReadJson(wJson);
    }
}

void TestSkJson::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Application = tApplication::Instance();
	m_NbRow = 10;
	m_NbCol = 5;
    //m_NbRow=2;
    //m_NbCol=2;
};

void TestSkJson::tearDown() {
	delete(m_Api);
}
