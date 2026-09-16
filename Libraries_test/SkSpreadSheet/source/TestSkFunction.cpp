//=============================================================================
// TestSkFunction.cpp
//=============================================================================
#include "../include/TestSkFunction.hpp"
#include <SkRangeRefTransform.hpp>
#include <SkException.hpp>
#include <cmath>
#include <exception>
#include <typeinfo>
#include <rapidjson/document.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//#define _printdebug

#define  _SK_DEBUG_TEST_CALENDAR

TestSkFunction::TestSkFunction() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

TestSkFunction::~TestSkFunction() {

}


void TestSkFunction::DrawCell(tInt sRowBegin,tInt sColBegin,tInt sRowEnd,tInt sColEnd) {
#ifdef printdebug
	cout << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";";
            switch (wVariant.Type()) {
                case tVariantType::t_date: {
                    // tClassDate::FormatString expects tFormatString* (not tFormatStringType).
                    tClassDate wDate(wVariant.Date());
                    tFormatString wFormat;
                    wFormat.FormatType(tFormatStringType::dateddmmyyyyhmm);
                    cout << wDate.FormatString(&wFormat);
                    break;
                }
                default:
                    cout << wVariant;
                    break;
            }
            cout << "\t";
		}
		cout << endl;
	}
#endif
}

void TestSkFunction::DebugRow(tString sTitle) {
    cout << sTitle << "----------------------------------" << endl;
    tInt wMaxRow=m_Api->BottomRight().Row();
    for(tInt wInd=1;wInd<=wMaxRow;wInd++) {
        tCell* wCell=m_Api->Cell(wInd,1);
        if (wCell!=nullptr) {
            cout << wCell->StrRef() << ":" << m_Api->Formula(wInd, 1) << "->" << wCell->Value()<< endl;
        } else {
            tStringStream wStream;
            wStream << "A" << wInd << "=nullptr";
            cout << wStream.str() << endl;
        }
    }
}

void TestSkFunction::TestFunctionDate() {
    m_Api->UndoCellValue("A1", "=YEAR(TODAY())");
    tVariant wVariant = m_Api->CellValue("A1");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), tClassDate(tClassDate::Now()).Year());

    m_Api->UndoCellValue("A1", "=MONTH(TODAY())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), tClassDate(tClassDate::Now()).Month());

    m_Api->UndoCellValue("A1", "=DAY(TODAY())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), tClassDate(tClassDate::Now()).Day());

    m_Api->UndoCellValue("A1", "=WEEKDAY(TODAY())");
    wVariant = m_Api->CellValue("A1");
    // WEEKDAY returns 1 (Sun)..7 (Sat) by default; DayWeek() returns 0 (Sun)..6 (Sat)
    CPPUNIT_ASSERT_EQUAL(tClassDate(tClassDate::Now()).DayWeek() + 1, wVariant.Int());
    
    m_Api->UndoCellValue("A1", "=WEEKNUM(TODAY())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), tClassDate(tClassDate::Now()).WeekNum());

    m_Api->UndoCellValue("A1", "=TIME(12,30,59)");
    wVariant = m_Api->CellValue("A1");
    // Excel TIME returns fraction of day in [0,1), not a t_date on 1900-01-01.
    const tDouble wExpectedTimeFrac =
        (12.0 * 3600.0 + 30.0 * 60.0 + 59.0) / 86400.0;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wExpectedTimeFrac, wVariant.Double(), 1e-12);

    m_Api->UndoCellValue("A1", "=HOUR(TIME(12,30,59))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 12);
    
    m_Api->UndoCellValue("A1", "=MINUTE(TIME(12,30,59))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 30);

    m_Api->UndoCellValue("A1", "=SECOND(TIME(12,30,59))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 59);

    // Excel TIME wraps into a single 24h clock (no extra calendar day from hour overflow).
    m_Api->UndoCellValue("A1", "=HOUR(TIME(27,0,0))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);
    m_Api->UndoCellValue("A1", "=MINUTE(TIME(0,90,0))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 30);
    m_Api->UndoCellValue("A1", "=HOUR(TIME(0,90,0))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 1);

    // Test NOW function
    m_Api->UndoCellValue("A2", "=NOW()");
    wVariant = m_Api->CellValue("A2");
    CPPUNIT_ASSERT(wVariant.Type() == tVariantType::t_date); // Should return a date
    
    // Test DATE function
    m_Api->UndoCellValue("A3", "=DATE(2024,3,15)");
    wVariant = m_Api->CellValue("A3");
    tClassDate wDate(2024, 3, 15);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wDate.Value());
    
    m_Api->UndoCellValue("A4", "=DATE(2023,12,31)");
    wVariant = m_Api->CellValue("A4");
    tClassDate wDate2(2023, 12, 31);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wDate2.Value());

    // Excel normalization: DATE(y, m, 0) -> last day of month m-1 (regression: =DATE(YEAR(NOW());MONTH(NOW())+1;0)).
    m_Api->UndoCellValue("A50", "=DATE(2026,8,0)");
    wVariant = m_Api->CellValue("A50");
    CPPUNIT_ASSERT(wVariant.IsDate());
    tClassDate wDateEndJul(2026, 7, 31);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wDateEndJul.Value());

    // Month overflow: DATE(y, 13, 0) -> last day of December y.
    m_Api->UndoCellValue("A51", "=DATE(2026,13,0)");
    wVariant = m_Api->CellValue("A51");
    CPPUNIT_ASSERT(wVariant.IsDate());
    tClassDate wDateEndDec(2026, 12, 31);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wDateEndDec.Value());

    // Day overflow: DATE(2024, 2, 30) -> 2024-03-01 (leap Feb has 29 days).
    m_Api->UndoCellValue("A52", "=DATE(2024,2,30)");
    wVariant = m_Api->CellValue("A52");
    CPPUNIT_ASSERT(wVariant.IsDate());
    tClassDate wDateMar1(2024, 3, 1);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wDateMar1.Value());

    // Regression (SkExcel2SpreadSheet date import): a date built from an Excel serial
    // must share the EXACT same time_t as the same date built by DATE(). Equality
    // (=A1=B1), VLOOKUP/MATCH exact match and date arithmetic compare the raw time_t,
    // not the rendered calendar day, so an imported serial date and a formula date for
    // the same day must be identical (previously import used noon UTC while DATE() used
    // local midnight, so they never matched).
    CPPUNIT_ASSERT_EQUAL(tClassDate(tVariant(static_cast<tDouble>(45306.0))).Value(),
                         tClassDate(2024, 1, 15).Value()); // serial 45306 = 2024-01-15
    // 1900 leap-year bug boundary: serial 1 = 1900-01-01, serial 61 = 1900-03-01.
    CPPUNIT_ASSERT_EQUAL(tClassDate(tVariant(static_cast<tDouble>(1.0))).Value(),
                         tClassDate(1900, 1, 1).Value());
    CPPUNIT_ASSERT_EQUAL(tClassDate(tVariant(static_cast<tDouble>(61.0))).Value(),
                         tClassDate(1900, 3, 1).Value());

    // FR locale — exact formula: =DATE(YEAR(NOW());MONTH(NOW())+1;0) (last day of current month).
    tApplication::Instance()->Locale("fr");
    CPPUNIT_ASSERT_MESSAGE("compile DATE(YEAR(NOW());MONTH(NOW())+1;0)",
        m_Api->UndoCellValue("A53", "=DATE(YEAR(NOW());MONTH(NOW())+1;0)"));
    wVariant = m_Api->CellValue("A53");
    CPPUNIT_ASSERT(wVariant.IsDate());
    {
        tClassDate wNow(tClassDate::Now());
        tInt wYear = 0, wMonth = 0, wDay = 0;
        wNow.YearMonthDay(wYear, wMonth, wDay);
        tClassDate wExpectedEom(wYear, wMonth + 1, 0);
        CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wExpectedEom.Value());
    }
    tApplication::Instance()->Locale("us");

    m_Api->UndoCellValue("A5", "=YEAR(DATE(2024,3,15))");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2024);
    
    m_Api->UndoCellValue("A6", "=MONTH(DATE(2024,3,15))");
    wVariant = m_Api->CellValue("A6");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);
    
    m_Api->UndoCellValue("A7", "=DAY(DATE(2024,3,15))");
    wVariant = m_Api->CellValue("A7");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 15);

    // Test EDATE function - Add months to a date
    // EDATE("2024-01-15", 1) should return 2024-02-15
    m_Api->UndoCellValue("A8", "=EDATE(DATE(2024,1,15), 1)");
    wVariant = m_Api->CellValue("A8");
    tClassDate wEdate1(2024, 2, 15);
    //cout  << endl << "=EDATE(DATE(2024,1,15), 1)" << endl << wVariant << endl;
    //cout  << wEdate1.Value() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate1.Value());
    
    // EDATE("2024-01-15", -1) should return 2023-12-15
    m_Api->UndoCellValue("A9", "=EDATE(DATE(2024,1,15), -1)");
    wVariant = m_Api->CellValue("A9");
    tClassDate wEdate2(2023, 12, 15);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate2.Value());
    
    // EDATE("2024-01-31", 1) should return 2024-02-29 (leap year)
    m_Api->UndoCellValue("A10", "=EDATE(DATE(2024,1,31), 1)");
    wVariant = m_Api->CellValue("A10");
    tClassDate wEdate3(2024, 2, 29);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate3.Value());
    
    // EDATE("2023-01-31", 1) should return 2023-02-28 (non-leap year)
    m_Api->UndoCellValue("A11", "=EDATE(DATE(2023,1,31), 1)");
    wVariant = m_Api->CellValue("A11");
    tClassDate wEdate4(2023, 2, 28);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate4.Value());
    
    // EDATE("2024-03-31", -1) should return 2024-02-29 (leap year)
    m_Api->UndoCellValue("A12", "=EDATE(DATE(2024,3,31), -1)");
    wVariant = m_Api->CellValue("A12");
    tClassDate wEdate5(2024, 2, 29);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate5.Value());
    
    // EDATE("2024-12-15", 2) should return 2025-02-15 (year overflow)
    m_Api->UndoCellValue("A13", "=EDATE(DATE(2024,12,15), 2)");
    wVariant = m_Api->CellValue("A13");
    tClassDate wEdate6(2025, 2, 15);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate6.Value());
    
    // EDATE("2024-01-15", 12) should return 2025-01-15 (exactly one year)
    m_Api->UndoCellValue("A14", "=EDATE(DATE(2024,1,15), 12)");
    wVariant = m_Api->CellValue("A14");
    tClassDate wEdate7(2025, 1, 15);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate7.Value());
    
    // EDATE("2024-04-30", 1) should return 2024-05-30 (30 days month)
    m_Api->UndoCellValue("A15", "=EDATE(DATE(2024,4,30), 1)");
    wVariant = m_Api->CellValue("A15");
    tClassDate wEdate8(2024, 5, 30);
    CPPUNIT_ASSERT_EQUAL(wVariant.Date(), wEdate8.Value());
    
      // Test Excel Calendar
     // Fixed calendar date so YEAR/DATE/WEEKDAY assertions do not depend on machine clock or timezone.
    const tInt wCalYear = 2026;
    tBool wOk = m_Api->UndoCellValue("A1", "=DATE(2026,3,20)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT(m_Api->CellValue("A1").Type() == tVariantType::t_date);

    wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(2026, m_Api->CellValue("A2").Int());

    wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
    CPPUNIT_ASSERT(wOk);

    wOk = m_Api->UndoCellValue("A3", "=DATE(A2,1,1)");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wJan1(wCalYear, 1, 1);
        CPPUNIT_ASSERT_EQUAL(wJan1.Value(), m_Api->CellValue("A3").Date());
    }

    wOk = m_Api->UndoCellValue("A4", "=WEEKDAY(DATE(A2,1,1))");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wJan1(wCalYear, 1, 1);
        // WEEKDAY default (return_type 1): 1=Sunday .. 7=Saturday (SkFunctionWeekDay)
        const tInt wExpectedWd = wJan1.DayWeek() + 1;
        CPPUNIT_ASSERT_EQUAL(wExpectedWd, m_Api->CellValue("A4").Int());
    }

    wOk = m_Api->UndoCellValue("A6", "=DATE(AnnéeCalendrier,12,1)-WEEKDAY(DATE(AnnéeCalendrier,12,1))");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wDec1(wCalYear, 12, 1);
        const tInt wWdDefault = wDec1.DayWeek() + 1;
        tClassDate wExpectedA6 = wDec1 - wWdDefault;
        CPPUNIT_ASSERT_EQUAL(wExpectedA6.Value(), m_Api->CellValue("A6").Date());
    }

    wOk = m_Api->UndoCellValue("A7", "l");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "A7");
    CPPUNIT_ASSERT(wOk);

    // =1+DATE(...)-WEEKDAY(...,(DébutSemaine="l")+1)+8  → WEEKDAY(...,2): Mon=1..Sun=7
    wOk = m_Api->UndoCellValue("A8",
        "=1+DATE(AnnéeCalendrier,12,1)-WEEKDAY(DATE(AnnéeCalendrier,12,1),(DébutSemaine=\"l\")+1)+8");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wDec1(wCalYear, 12, 1);
        const tInt wTmWday = wDec1.DayWeek();
        const tInt wWdType2 = (wTmWday == 0) ? 7 : wTmWday;
        tClassDate wExpectedA8 = (wDec1 + 1) - wWdType2 + 8;
        CPPUNIT_ASSERT_EQUAL(wExpectedA8.Value(), m_Api->CellValue("A8").Date());
    }

    // WEEKDAY extended return_type (Excel parity): 11≡2, 17≡1; 12 = week starting Tuesday; invalid 4 → #NUM!
    wOk = m_Api->UndoCellValue("Z1", "=DATE(2026,3,23)");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoCellValue("Z2", "=WEEKDAY(Z1,11)");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoCellValue("Z3", "=WEEKDAY(Z1,2)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("Z2").Int(), m_Api->CellValue("Z3").Int());
    wOk = m_Api->UndoCellValue("Z4", "=WEEKDAY(Z1,17)");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoCellValue("Z5", "=WEEKDAY(Z1,1)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("Z4").Int(), m_Api->CellValue("Z5").Int());
    // Monday: type 2/11 → 1
    CPPUNIT_ASSERT_EQUAL(1, m_Api->CellValue("Z2").Int());
    // Type 12 (Tuesday = 1): Monday is 7
    wOk = m_Api->UndoCellValue("Z6", "=WEEKDAY(Z1,12)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(7, m_Api->CellValue("Z6").Int());
    wOk = m_Api->UndoCellValue("Z7", "=WEEKDAY(Z1,4)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT(m_Api->CellValue("Z7").IsError());

    // NETWORKDAYS: Mon 2024-01-01 .. Fri 2024-01-05 = 5 workdays
    wOk = m_Api->UndoCellValue("N1", "=NETWORKDAYS(DATE(2024,1,1), DATE(2024,1,5))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(5, m_Api->CellValue("N1").Int());

    // Same week with New Year's Day as holiday -> 4
    wOk = m_Api->UndoCellValue("N2", "=NETWORKDAYS(DATE(2024,1,1), DATE(2024,1,5), DATE(2024,1,1))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(4, m_Api->CellValue("N2").Int());

    // Reverse order -> negative
    wOk = m_Api->UndoCellValue("N3", "=NETWORKDAYS(DATE(2024,1,5), DATE(2024,1,1))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(-5, m_Api->CellValue("N3").Int());

    // WORKDAY: Fri 2024-01-05 + 1 workday = Mon 2024-01-08
    wOk = m_Api->UndoCellValue("N4", "=WORKDAY(DATE(2024,1,5), 1)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT(m_Api->CellValue("N4").IsDate());
    {
        tClassDate wGot(m_Api->CellValue("N4").Date());
        tClassDate wExp(2024, 1, 8);
        CPPUNIT_ASSERT_EQUAL(wExp.Value(), wGot.Value());
    }
    // WORKDAY with holiday on Monday -> Tuesday
    wOk = m_Api->UndoCellValue("N5", "=WORKDAY(DATE(2024,1,5), 1, DATE(2024,1,8))");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wGot(m_Api->CellValue("N5").Date());
        tClassDate wExp(2024, 1, 9);
        CPPUNIT_ASSERT_EQUAL(wExp.Value(), wGot.Value());
    }

    // DAYS / ISOWEEKNUM / TIMEVALUE / DATEDIF / YEARFRAC / NETWORKDAYS_INTL
    wOk = m_Api->UndoCellValue("N6", "=DAYS(DATE(2024,1,10), DATE(2024,1,1))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(9, m_Api->CellValue("N6").Int());

    wOk = m_Api->UndoCellValue("N7", "=ISOWEEKNUM(DATE(2024,1,1))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(tClassDate(2024, 1, 1).WeekNum(), m_Api->CellValue("N7").Int());

    wOk = m_Api->UndoCellValue("N8", "=TIMEVALUE(\"12:30:00\")");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5 + 30.0 / 1440.0, m_Api->CellValue("N8").Double(), 1e-12);

    wOk = m_Api->UndoCellValue("N9", "=DATEDIF(DATE(2020,1,15), DATE(2024,3,20), \"Y\")");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(4, m_Api->CellValue("N9").Int());

    wOk = m_Api->UndoCellValue("N10", "=YEARFRAC(DATE(2024,1,1), DATE(2024,7,1), 0)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, m_Api->CellValue("N10").Double(), 1e-12);

    // weekend code 11 = Sunday only -> Mon-Sat workdays; 2024-01-01 Mon .. 2024-01-07 Sun = 6
    wOk = m_Api->UndoCellValue("N11", "=NETWORKDAYS_INTL(DATE(2024,1,1), DATE(2024,1,7), 11)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(6, m_Api->CellValue("N11").Int());

    // DAYS360 — US (NASD) vs European.
    wOk = m_Api->UndoCellValue("N12", "=DAYS360(DATE(2012,1,1), DATE(2012,12,31))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(360, m_Api->CellValue("N12").Int());
    wOk = m_Api->UndoCellValue("N13", "=DAYS360(DATE(2012,1,1), DATE(2012,12,31), TRUE)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(359, m_Api->CellValue("N13").Int());
    wOk = m_Api->UndoCellValue("N14", "=DAYS360(DATE(2011,1,30), DATE(2011,2,1))");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(1, m_Api->CellValue("N14").Int());

    // WORKDAY_INTL: weekend code 11 = Sunday only → Fri + 1 = Sat.
    wOk = m_Api->UndoCellValue("N15", "=WORKDAY_INTL(DATE(2024,1,5), 1, 11)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT(m_Api->CellValue("N15").IsDate());
    {
        tClassDate wGot(m_Api->CellValue("N15").Date());
        tClassDate wExp(2024, 1, 6);
        CPPUNIT_ASSERT_EQUAL(wExp.Value(), wGot.Value());
    }
    // Default weekend (Sat/Sun) matches WORKDAY.
    wOk = m_Api->UndoCellValue("N16", "=WORKDAY_INTL(DATE(2024,1,5), 1)");
    CPPUNIT_ASSERT(wOk);
    {
        tClassDate wGot(m_Api->CellValue("N16").Date());
        tClassDate wExp(2024, 1, 8);
        CPPUNIT_ASSERT_EQUAL(wExp.Value(), wGot.Value());
    }

}

void TestSkFunction::TestFunctionExcelCalendar() {
    tBool wOk = m_Api->UndoCellValue("A1", "=DATE(2026,3,20)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT(m_Api->CellValue("A1").Type() == tVariantType::t_date);

    wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(2026, m_Api->CellValue("A2").Int());

    wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoCellValue("B2", "l");
    CPPUNIT_ASSERT(wOk);
    wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "B2");
    CPPUNIT_ASSERT(wOk);

    // Excel Calendar test: named array (6x7) + dates — multi-cell spill from A10 (like Excel 365).
    tWorkBook* wWorkBook=m_Api->ActiveWorkBook();
    wOk=wWorkBook->InsertNamedFormula("JoursEtSemaines","{0,1,2,3,4,5,6} + {0;1;2;3;4;5}*7");
    CPPUNIT_ASSERT(wOk);
   
    wOk = m_Api->UndoCellValue("A10","=JoursEtSemaines+DATE(AnnéeCalendrier,1,1)-WEEKDAY(DATE(AnnéeCalendrier,1,1),(DébutSemaine=\"l\")+1)+8");
    CPPUNIT_ASSERT(wOk);
    // CellValue("A10") uses the active sheet — keep Sheet1 selected (do not switch to _$$ before these checks).
#ifdef SK_DEBUG_TEST_CALENDAR
    // Enable in preprocessor to print cell types when diagnosing spill / named-formula buffer reads.
    cerr << "[TestFunctionExcelCalendar] A10 type=" << static_cast<int>(m_Api->CellValue("A10").Type())
         << " B10 type=" << static_cast<int>(m_Api->CellValue("B10").Type()) << endl;
             // Debug dump: spill on Sheet1 (include column A), then named-formula sheet _$$; restore Sheet1 for the rest.
    DrawCell(10, 1, 20, 8);
    m_Api->ActiveSheet("_$$");
    DrawCell(1, 1, 10, 8);
    m_Api->ActiveSheet("Sheet1");
#endif
    CPPUNIT_ASSERT(m_Api->CellValue("A10").Type() == tVariantType::t_date);
    CPPUNIT_ASSERT(m_Api->CellValue("B10").Type() == tVariantType::t_date);
    CPPUNIT_ASSERT(m_Api->CellValue("G10").Type() == tVariantType::t_date);
    CPPUNIT_ASSERT(m_Api->CellValue("A11").Type() == tVariantType::t_date);
    CPPUNIT_ASSERT(m_Api->CellValue("G15").Type() == tVariantType::t_date);
  
    
    //tCell* wCell=m_Api->Cell("A10");
    //cout << wCell->FormulaStr() << endl;
    /* Test
    wOk = m_Api->UndoCellValue("A1", "=DATE(2026,3,1)");
    wOk = m_Api->UndoCellValue("B1","=WEEKDAY(A1)");
    tCell* wCell=m_Api->Cell("B1");
    
    cout << endl << wCell->StrRef() << "=" << wCell->FormulaStr()  << "=" << wCell->Value() << endl;
    */
}

void TestSkFunction::TestFunctionMath() {
    tBool wOk = false;
    m_Api->UndoCellValue("A1", "=SIN(1.2)");
    tVariant wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), sin(1.2));

    m_Api->UndoCellValue("A1", "=COS(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), cos(1.2));

    // Blank cell coerces to 0 (Excel SIN/COS/…).
    m_Api->UndoRaz("Z1");
    m_Api->UndoCellValue("A1", "=SIN(Z1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("A1", "=COS(Z1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("A1", "=ABS(Z1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);

    m_Api->UndoCellValue("A1", "=TAN(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), tan(1.2));

    m_Api->UndoCellValue("A1", "=ACOS(0.5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), acos(0.5));

    m_Api->UndoCellValue("A1", "=ASIN(0.5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), asin(0.5));

    m_Api->UndoCellValue("A1", "=ATAN(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), atan(1.2));

    m_Api->UndoCellValue("A1", "=SQRT(10)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), sqrt(10));

    m_Api->UndoCellValue("A1", "=LOG(10)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), log(10));

    // LOG10 must not be lexed as cell LOG10 (column LOG, row 10).
    wOk = m_Api->UndoCellValue("A1", "=LOG10(0.3)");
    CPPUNIT_ASSERT(wOk);
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), log10(0.3));

    // Legacy alias still resolves
    wOk = m_Api->UndoCellValue("A1", "=LOG_10(0.3)");
    CPPUNIT_ASSERT(wOk);
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), log10(0.3));

    m_Api->UndoCellValue("A1", "=EXP(10)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), exp(10));

    m_Api->UndoCellValue("A1", "=PI()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), M_PI);

    m_Api->UndoCellValue("A1", "=RADIANS(180)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), M_PI);
    
    m_Api->UndoCellValue("A1", "=DEGREES(PI())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 180.0);

    // Extended unary math (hyperbolic / reciprocal / SIGN / SQRTPI).
    m_Api->UndoCellValue("A1", "=SINH(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), sinh(1.2));
    m_Api->UndoCellValue("A1", "=COSH(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), cosh(1.2));
    m_Api->UndoCellValue("A1", "=TANH(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), tanh(1.2));
    m_Api->UndoCellValue("A1", "=ASINH(1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), asinh(1.2));
    m_Api->UndoCellValue("A1", "=ACOSH(2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), acosh(2.0));
    m_Api->UndoCellValue("A1", "=ATANH(0.5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), atanh(0.5));
    m_Api->UndoCellValue("A1", "=COT(PI()/4)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("A1", "=SEC(0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("A1", "=CSC(PI()/2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("A1", "=ACOT(0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(M_PI / 2.0, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("A1", "=SIGN(-3)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(-1.0, wVariant.Double());
    m_Api->UndoCellValue("A1", "=SIGN(0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(0.0, wVariant.Double());
    m_Api->UndoCellValue("A1", "=SIGN(5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(1.0, wVariant.Double());
    m_Api->UndoCellValue("A1", "=SQRTPI(1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(std::sqrt(M_PI), wVariant.Double(), 1e-12);
    
    m_Api->UndoCellValue("A1", "=ROUND(1.53456)");
    wVariant = m_Api->CellValue("A1");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 2.0);
    
    wOk=m_Api->UndoCellValue("A1", "=ROUND(1.53556,2)");
    wVariant = m_Api->CellValue("A1");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 1.54);
    
    m_Api->UndoCellValue("A1", "=TRUNC(1.53456,2)");
    wVariant = m_Api->CellValue("A1");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 1.53);
    
    m_Api->UndoCellValue("A1", "=TRUNC(1.53456)");
    wVariant = m_Api->CellValue("A1");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 1.0);

    // CEILING_MATH / FLOOR_MATH (Excel CEILING.MATH / FLOOR.MATH via dotted rewrite on import).
    m_Api->UndoCellValue("E1", "=CEILING_MATH(2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);
    m_Api->UndoCellValue("E1", "=CEILING_MATH(-2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -2);
    m_Api->UndoCellValue("E1", "=CEILING_MATH(-2.5,1,-1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -3);
    m_Api->UndoCellValue("E1", "=FLOOR_MATH(2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);
    m_Api->UndoCellValue("E1", "=FLOOR_MATH(-2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -2);
    m_Api->UndoCellValue("E1", "=FLOOR_MATH(-2.5,1,-1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -3);
    m_Api->UndoCellValue("E1", "=CEILING_MATH(2.1,0.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 2.5, 1e-12);

    // CEILING_PRECISE / FLOOR_PRECISE / ISO_CEILING — always toward ±∞; |significance|.
    m_Api->UndoCellValue("E1", "=CEILING_PRECISE(2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    m_Api->UndoCellValue("E1", "=CEILING_PRECISE(-2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FLOOR_PRECISE(-2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-3, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FLOOR_PRECISE(2.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=ISO_CEILING(-2.5,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=CEILING_PRECISE(2.5,0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_div0, wVariant.Error().Code());

    // Legacy CEILING / FLOOR (compatibility): significance required, same-sign rule.
    m_Api->UndoCellValue("E1", "=CEILING(2.5,1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    m_Api->UndoCellValue("E1", "=CEILING(-2.5,-2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-4, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FLOOR(2.5,1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FLOOR(-2.5,-2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=CEILING(2.5,-1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());

    // MROUND — nearest multiple; half away from zero; same-sign rule.
    m_Api->UndoCellValue("E1", "=MROUND(10,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(9, wVariant.Int());
    m_Api->UndoCellValue("E1", "=MROUND(1.3,0.2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.4, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("E1", "=MROUND(-10,-3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-9, wVariant.Int());
    m_Api->UndoCellValue("E1", "=MROUND(1.5,1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=MROUND(10,-3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());

    // GCD / LCM — truncate toward zero; use absolute values.
    m_Api->UndoCellValue("E1", "=GCD(24,36)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(12, wVariant.Int());
    m_Api->UndoCellValue("E1", "=GCD(5.9,2.1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=LCM(5,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());
    m_Api->UndoCellValue("E1", "=LCM(24,36)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(72, wVariant.Int());

    // EVEN / ODD — round away from zero to even/odd; ODD(0)=1.
    m_Api->UndoCellValue("E1", "=EVEN(1.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=EVEN(3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(4, wVariant.Int());
    m_Api->UndoCellValue("E1", "=EVEN(-1.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=ODD(1.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    m_Api->UndoCellValue("E1", "=ODD(0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=ODD(-2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-3, wVariant.Int());

    // COMBIN — binomial coefficient; truncate toward zero; #NUM! if invalid.
    m_Api->UndoCellValue("E1", "=COMBIN(5,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());
    m_Api->UndoCellValue("E1", "=COMBIN(8,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(56, wVariant.Int());
    m_Api->UndoCellValue("E1", "=COMBIN(5.9,2.1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());
    m_Api->UndoCellValue("E1", "=COMBIN(3,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_num, wVariant.Error().Code());

    // COMBINA — combinations with repetition C(n+k-1, k).
    m_Api->UndoCellValue("E1", "=COMBINA(5,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(35, wVariant.Int());
    m_Api->UndoCellValue("E1", "=COMBINA(0,0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());

    // BASE / DECIMAL / ROMAN / ARABIC / MULTINOMIAL / SERIESSUM / PERCENTOF
    m_Api->UndoCellValue("E1", "=BASE(7,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("111"), wVariant.String());
    m_Api->UndoCellValue("E1", "=BASE(100,16)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("64"), wVariant.String());
    m_Api->UndoCellValue("E1", "=BASE(15,2,10)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("0000001111"), wVariant.String());
    m_Api->UndoCellValue("E1", "=DECIMAL(\"FF\",16)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(255.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=DECIMAL(\"111\",2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, wVariant.Numeric(), 1e-12);

    // Engineering bits + radix conversions (shared BitOp / EngStep / EngBaseConvert).
    m_Api->UndoCellValue("E1", "=BITAND(13,25)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(9.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BITOR(5,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BITXOR(5,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BITLSHIFT(4,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(16.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BITRSHIFT(16,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BITLSHIFT(16,-2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=DELTA(5,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=DELTA(5,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GESTEP(5,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GESTEP(3,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    // BIN2DEC / DEC2BIN / HEX2* must not be lexed as cell BIN2 / DEC2 / HEX2 + trailing id.
    m_Api->UndoCellValue("E1", "=BIN2DEC(1100100)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(100.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BIN2DEC(\"1111111111\")");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=DEC2BIN(9,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("1001"), wVariant.String());
    m_Api->UndoCellValue("E1", "=DEC2BIN(-1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("1111111111"), wVariant.String());
    m_Api->UndoCellValue("E1", "=DEC2BIN(512)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    m_Api->UndoCellValue("E1", "=HEX2DEC(\"FF\")");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(255.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=OCT2DEC(10)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=BIN2HEX(11111111,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("00FF"), wVariant.String());
    m_Api->UndoCellValue("E1", "=HEX2BIN(\"F\",8)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("00001111"), wVariant.String());

    m_Api->UndoCellValue("E1", "=ROMAN(499)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("CDXCIX"), wVariant.String());
    m_Api->UndoCellValue("E1", "=ROMAN(999,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(tString("IM"), wVariant.String());
    m_Api->UndoCellValue("E1", "=ARABIC(\"MCMXII\")");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1912, wVariant.Int());
    m_Api->UndoCellValue("E1", "=ARABIC(\"IM\")");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(999, wVariant.Int());
    m_Api->UndoCellValue("E1", "=MULTINOMIAL(2,3,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1260.0, wVariant.Numeric(), 1e-12); // 9!/(2!3!4!)
    m_Api->UndoCellValue("G1", 1);
    m_Api->UndoCellValue("G2", 1);
    m_Api->UndoCellValue("E1", "=SERIESSUM(2,0,1,G1:G2)"); // 1*2^0 + 1*2^1 = 3
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERCENTOF(25,100)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.25, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("H1", 10);
    m_Api->UndoCellValue("H2", 20);
    m_Api->UndoCellValue("H3", 70);
    m_Api->UndoCellValue("E1", "=PERCENTOF(H1:H2,H1:H3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.3, wVariant.Numeric(), 1e-12);

    // FACTDOUBLE — double factorial; negative odd allowed.
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(15, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(6)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(48, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(-1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(-3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(-5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    m_Api->UndoCellValue("E1", "=FACTDOUBLE(-2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_num, wVariant.Error().Code());

    // SUMSQ — sum of squares.
    m_Api->UndoCellValue("E1", "=SUMSQ(3,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(25, wVariant.Int());
    m_Api->UndoCellValue("D1", 1);
    m_Api->UndoCellValue("D2", 2);
    m_Api->UndoCellValue("D3", 3);
    m_Api->UndoCellValue("E1", "=SUMSQ(D1:D3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(14, wVariant.Int());

    // SUMX2MY2 / SUMX2PY2 / SUMXMY2 — pairwise over equal-sized arrays.
    // Use G/H so A1 stays intact for the SUM(A1:A2,…) assertion below.
    m_Api->UndoCellValue("G1", 2);
    m_Api->UndoCellValue("G2", 3);
    m_Api->UndoCellValue("H1", 4);
    m_Api->UndoCellValue("H2", 5);
    m_Api->UndoCellValue("E1", "=SUMX2MY2(G1:G2,H1:H2)");
    wVariant = m_Api->CellValue("E1");
    // (4-16)+(9-25) = -12-16 = -28
    CPPUNIT_ASSERT_EQUAL(-28, wVariant.Int());
    m_Api->UndoCellValue("E1", "=SUMX2PY2(G1:G2,H1:H2)");
    wVariant = m_Api->CellValue("E1");
    // (4+16)+(9+25) = 20+34 = 54
    CPPUNIT_ASSERT_EQUAL(54, wVariant.Int());
    m_Api->UndoCellValue("E1", "=SUMXMY2(G1:G2,H1:H2)");
    wVariant = m_Api->CellValue("E1");
    // (2-4)²+(3-5)² = 4+4 = 8
    CPPUNIT_ASSERT_EQUAL(8, wVariant.Int());

    // INT rounds down toward negative infinity (scratch cell E1: keep A1 for the SUM test below).
    m_Api->UndoCellValue("E1", "=INT(8.9)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 8);

    m_Api->UndoCellValue("E1", "=INT(-8.9)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -9);

    m_Api->UndoCellValue("E1", "=INT(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 5);

    // INT preserves magnitudes beyond 32-bit int as a double.
    m_Api->UndoCellValue("E1", "=INT(1234567890123.7)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 1234567890123.0, 0.5);

    // MEDIAN
    m_Api->UndoCellValue("E1", "=MEDIAN(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());

    m_Api->UndoCellValue("E1", "=MEDIAN(1, 2, 3, 4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.5, wVariant.Double(), 1e-12);

    m_Api->UndoCellValue("D1", 10);
    m_Api->UndoCellValue("D2", 20);
    m_Api->UndoCellValue("D3", 30);
    m_Api->UndoCellValue("E1", "=MEDIAN(D1:D3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(20, wVariant.Int());

    // STDEV_S (sample, n-1; Excel STDEV.S): STDEV_S(1,2,3,4,5) = sqrt(2.5)
    m_Api->UndoCellValue("E1", "=STDEV_S(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sqrt(2.5), wVariant.Double(), 1e-12);

    // Excel compatibility alias STDEV
    m_Api->UndoCellValue("E1", "=STDEV(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sqrt(2.5), wVariant.Double(), 1e-12);

    // STDEV_P / VAR_S / VAR_P (shared tFunctionVariance): values 1..5
    // population variance = 2, sample variance = 2.5
    m_Api->UndoCellValue("E1", "=STDEV_P(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(sqrt(2.0), wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("E1", "=VAR_S(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.5, wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("E1", "=VAR_P(1, 2, 3, 4, 5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Double(), 1e-12);

    // AVEDEV / DEVSQ — use Numeric() (whole results may be boxed as int).
    m_Api->UndoCellValue("E1", "=AVEDEV(1,2,3,4,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.2, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=DEVSQ(1,2,3,4,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, wVariant.Numeric(), 1e-12);

    // AVERAGEA / MAXA / MINA / *A variance / moments / STANDARDIZE / FISHER / PHI / GAUSS / GAMMA
    m_Api->UndoCellValue("E1", "=AVERAGEA(TRUE,FALSE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=MAXA(TRUE,FALSE,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=MINA(TRUE,FALSE,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=STDEVA(1,2,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=VARPA(1,2,3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0 / 3.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=SKEW(1,2,3,4,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=SKEW_P(1,2,3,4,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=KURT(1,2,3,4,5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.2, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=STANDARDIZE(5,3,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=FISHER(0.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5 * std::log(3.0), wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=FISHERINV(0.5*LN(3))");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-10);
    m_Api->UndoCellValue("E1", "=PHI(0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / std::sqrt(2.0 * M_PI), wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GAUSS(0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GAMMA(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(24.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GAMMALN(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(std::log(24.0), wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GAMMALN_PRECISE(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(std::log(24.0), wVariant.Numeric(), 1e-12);

    // Normal distribution: NORM.S.* / NORM.* (+ Compatibility aliases).
    m_Api->UndoCellValue("E1", "=NORM_S_DIST(0,TRUE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=NORM_S_DIST(0,FALSE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / std::sqrt(2.0 * M_PI), wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=NORMSDIST(0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=NORM_S_DIST(1.333333,TRUE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.908788726, wVariant.Numeric(), 1e-8);
    m_Api->UndoCellValue("E1", "=NORM_S_DIST(1.333333,FALSE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.164010148, wVariant.Numeric(), 1e-8);
    m_Api->UndoCellValue("E1", "=NORM_DIST(42,40,1.5,TRUE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.9087888, wVariant.Numeric(), 1e-7);
    m_Api->UndoCellValue("E1", "=NORM_DIST(42,40,1.5,FALSE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.10934005, wVariant.Numeric(), 1e-7);
    m_Api->UndoCellValue("E1", "=NORMDIST(42,40,1.5,TRUE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.9087888, wVariant.Numeric(), 1e-7);
    m_Api->UndoCellValue("E1", "=NORM_S_INV(0.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=NORMSINV(0.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=NORM_S_INV(0.908788726)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.333333, wVariant.Numeric(), 1e-5);
    m_Api->UndoCellValue("E1", "=NORM_INV(0.9087888,40,1.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.0, wVariant.Numeric(), 1e-4);
    m_Api->UndoCellValue("E1", "=NORMINV(0.9087888,40,1.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.0, wVariant.Numeric(), 1e-4);
    m_Api->UndoCellValue("E1", "=NORM_DIST(1,0,0,TRUE)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    m_Api->UndoCellValue("E1", "=NORM_S_INV(0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    m_Api->UndoCellValue("E1", "=NORM_S_DIST(1,TRUE)-0.5");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5 * std::erf(1.0 / std::sqrt(2.0)), wVariant.Numeric(), 1e-12);

    // CORREL / PEARSON / COVARIANCE / SLOPE / INTERCEPT / FORECAST — use G/H scratch.
    m_Api->UndoCellValue("G1", 1);
    m_Api->UndoCellValue("G2", 2);
    m_Api->UndoCellValue("G3", 3);
    m_Api->UndoCellValue("H1", 2);
    m_Api->UndoCellValue("H2", 4);
    m_Api->UndoCellValue("H3", 6);
    m_Api->UndoCellValue("E1", "=CORREL(G1:G3,H1:H3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PEARSON(G1:G3,H1:H3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=COVARIANCE_P(G1:G3,H1:H3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0 / 3.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=COVARIANCE_S(G1:G3,H1:H3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=SLOPE(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=INTERCEPT(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=FORECAST(4,H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=FORECAST_LINEAR(4,H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-12);

    // PERCENTILE_INC / QUARTILE_INC
    m_Api->UndoCellValue("E1", "=PERCENTILE_INC(G1:G3,0.5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=QUARTILE_INC(G1:G3,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERCENTILE(G1:G3,0)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);

    // RSQ / STEYX / PERCENTRANK / PERMUT / means / FREQUENCY
    m_Api->UndoCellValue("E1", "=RSQ(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=STEYX(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    // TREND — spill fitted / extrapolated y (G={1,2,3}, H={2,4,6})
    m_Api->UndoCellValue("J1", "=TREND(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("J2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("J3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("K1", 4);
    m_Api->UndoCellValue("K2", 5);
    m_Api->UndoCellValue("J1", "=TREND(H1:H3,G1:G3,K1:K2)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("J2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("J1", "=TREND(H1:H3)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("J1", "=TREND(H1:H3,G1:G3,4,FALSE)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-12);

    // GROWTH — exponential fit (H={2,4,8} on G={1,2,3} → b=1, m=2).
    m_Api->UndoCellValue("H1", 2);
    m_Api->UndoCellValue("H2", 4);
    m_Api->UndoCellValue("H3", 8);
    m_Api->UndoCellValue("J1", "=GROWTH(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-10);
    wVariant = m_Api->CellValue("J2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, wVariant.Numeric(), 1e-10);
    wVariant = m_Api->CellValue("J3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, wVariant.Numeric(), 1e-10);
    m_Api->UndoCellValue("K1", 4);
    m_Api->UndoCellValue("J1", "=GROWTH(H1:H3,G1:G3,K1)");
    wVariant = m_Api->CellValue("J1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(16.0, wVariant.Numeric(), 1e-9);

    // LINEST — slope/intercept spill on a free N1:O5 area (K1 still holds GROWTH's new_x).
    m_Api->UndoCellValue("H1", 2);
    m_Api->UndoCellValue("H2", 4);
    m_Api->UndoCellValue("H3", 6);
    m_Api->UndoCellValue("N1", "=LINEST(H1:H3,G1:G3)");
    wVariant = m_Api->CellValue("N1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("O1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("N1", "=LINEST(H1:H3,G1:G3,TRUE,TRUE)");
    wVariant = m_Api->CellValue("N1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("O1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("N3"); // r²
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);

    m_Api->UndoCellValue("E1", "=PERCENTRANK_INC(G1:G3,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERCENTRANK_EXC(G1:G3,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERCENTRANK(G1:G3,1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERMUT(5,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(20.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=PERMUTATIONA(5,2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(25.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=GEOMEAN(1,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("E1", "=HARMEAN(1,2,4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.0 / 7.0, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("G4", 4);
    m_Api->UndoCellValue("G5", 5);
    m_Api->UndoCellValue("G6", 6);
    m_Api->UndoCellValue("G7", 7);
    m_Api->UndoCellValue("G8", 8);
    m_Api->UndoCellValue("G9", 9);
    m_Api->UndoCellValue("G10", 10);
    m_Api->UndoCellValue("E1", "=TRIMMEAN(G1:G10,0.2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.5, wVariant.Numeric(), 1e-12);
    m_Api->UndoCellValue("H1", 2);
    m_Api->UndoCellValue("H2", 4);
    m_Api->UndoCellValue("I1", "=FREQUENCY(G1:G5,H1:H2)");
    wVariant = m_Api->CellValue("I1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("I2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, wVariant.Numeric(), 1e-12);
    wVariant = m_Api->CellValue("I3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, wVariant.Numeric(), 1e-12);

    // ATAN2 / QUOTIENT / FACT
    m_Api->UndoCellValue("E1", "=ATAN2(1, 1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(atan2(1.0, 1.0), wVariant.Double(), 1e-12);
    m_Api->UndoCellValue("E1", "=QUOTIENT(10, 3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3.0, wVariant.Double());
    m_Api->UndoCellValue("E1", "=QUOTIENT(-10, 3)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(-3.0, wVariant.Double());
    m_Api->UndoCellValue("E1", "=FACT(5)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(120, wVariant.Int());

    // LARGE / SMALL (shared tFunctionNth): D1:D3 = 10,20,30
    m_Api->UndoCellValue("E1", "=LARGE(D1:D3, 1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(30, wVariant.Int());
    m_Api->UndoCellValue("E1", "=LARGE(D1:D3, 2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(20, wVariant.Int());
    m_Api->UndoCellValue("E1", "=SMALL(D1:D3, 1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());
    m_Api->UndoCellValue("E1", "=SMALL(D1:D3, 2)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(20, wVariant.Int());

    // MODE_SNGL / MODE / MODE_MULT
    CPPUNIT_ASSERT_MESSAGE("compile MODE_SNGL", m_Api->UndoCellValue("E1", "=MODE_SNGL(1, 2, 2, 3, 3, 3)"));
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    CPPUNIT_ASSERT_MESSAGE("compile MODE", m_Api->UndoCellValue("E1", "=MODE(4, 4, 5, 5, 5, 6)"));
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(5, wVariant.Int());
    CPPUNIT_ASSERT_MESSAGE("compile MODE_SNGL unique", m_Api->UndoCellValue("E1", "=MODE_SNGL(1, 2, 3)"));
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_na);
    // MODE_MULT: 2 and 3 both appear twice → vertical spill in first-appearance order.
    m_Api->UndoCellValue("L1", "=MODE_MULT(1, 2, 2, 3, 3)");
    wVariant = m_Api->CellValue("L1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    wVariant = m_Api->CellValue("L2");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());

    // RANK_EQ / RANK — descending default; ties share top rank
    m_Api->UndoCellValue("F1", 10);
    m_Api->UndoCellValue("F2", 20);
    m_Api->UndoCellValue("F3", 20);
    m_Api->UndoCellValue("F4", 30);
    CPPUNIT_ASSERT_MESSAGE("compile RANK_EQ", m_Api->UndoCellValue("E1", "=RANK_EQ(30, F1:F4)"));
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=RANK_EQ(20, F1:F4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("E1", "=RANK(10, F1:F4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(4, wVariant.Int());
    m_Api->UndoCellValue("E1", "=RANK_EQ(10, F1:F4, 1)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("E1", "=RANK_AVG(20, F1:F4)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.5, wVariant.Numeric(), 1e-12);

    m_Api->UndoCellValue("A2", 3);
    
    wOk=m_Api->UndoCellValue("A3","=SUM(A1:A2,2.5,3.2)");
    CPPUNIT_ASSERT(wOk);
    wVariant=m_Api->CellValue("A3");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 9.7);

    // Text in range is ignored (Excel SUM; INSIDE DEMO #R: labels).
    m_Api->UndoCellValue("B1", "#R:BA_AB-");
    m_Api->UndoCellValue("B2", 100);
    m_Api->UndoCellValue("B3", 50);
    m_Api->UndoCellValue("B4", "label");
    wOk = m_Api->UndoCellValue("A3", "=SUM(B1:B4)");
    CPPUNIT_ASSERT(wOk);
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 150);
    
    // Test MOD function
    m_Api->UndoCellValue("A4", "=MOD(10,3)");
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 1); // 10 mod 3 = 1
    
    m_Api->UndoCellValue("A5", "=MOD(15,4)");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // 15 mod 4 = 3
    
    m_Api->UndoCellValue("A6", "=MOD(20,5)");
    wVariant = m_Api->CellValue("A6");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0); // 20 mod 5 = 0
    
    m_Api->UndoCellValue("A7", "=MOD(10.5,3.2)");
    wVariant = m_Api->CellValue("A7");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), fmod(10.5, 3.2), 0.0001); // Test with doubles
    
    m_Api->UndoCellValue("A8", "=MOD(-10,3)");
    wVariant = m_Api->CellValue("A8");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -1); // -10 mod 3 = -1 (Excel behavior)
    
    // Test POWER function
    m_Api->UndoCellValue("A9", "=POWER(2,3)");
    wVariant = m_Api->CellValue("A9");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 8); // 2^3 = 8
    
    m_Api->UndoCellValue("A10", "=POWER(5,2)");
    wVariant = m_Api->CellValue("A10");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 25); // 5^2 = 25
    
    m_Api->UndoCellValue("A11", "=POWER(2,0.5)");
    wVariant = m_Api->CellValue("A11");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), sqrt(2.0), 0.0001); // 2^0.5 = sqrt(2)
    
    m_Api->UndoCellValue("A12", "=POWER(10,-1)");
    wVariant = m_Api->CellValue("A12");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.1, 0.0001); // 10^-1 = 0.1
    
    m_Api->UndoCellValue("A13", "=POWER(2.5,2.5)");
    wVariant = m_Api->CellValue("A13");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), pow(2.5, 2.5), 0.0001); // Test with doubles
    
    // Test PRODUCT function
    m_Api->UndoCellValue("A14", "=PRODUCT(2,3,4)");
    wVariant = m_Api->CellValue("A14");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 24); // 2*3*4 = 24
    
    m_Api->UndoCellValue("A15", "=PRODUCT(5,2)");
    wVariant = m_Api->CellValue("A15");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 10); // 5*2 = 10
    
    m_Api->UndoCellValue("A16", "=PRODUCT(2.5,4)");
    wVariant = m_Api->CellValue("A16");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 10.0, 0.0001); // 2.5*4 = 10
    
    // Setup values for PRODUCT range test
    m_Api->UndoCellValue("B1", "2");
    m_Api->UndoCellValue("B2", "3");
    m_Api->UndoCellValue("B3", "4");
    m_Api->UndoCellValue("A17", "=PRODUCT(B1:B3)");
    wVariant = m_Api->CellValue("A17");
    //cout << "A17=PRODUCT(B1:B3)" << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 24); // 2*3*4 = 24

    // Test SUMPRODUCT function
    m_Api->UndoCellValue("C1", "1");
    m_Api->UndoCellValue("C2", "2");
    m_Api->UndoCellValue("C3", "3");
    m_Api->UndoCellValue("D1", "4");
    m_Api->UndoCellValue("D2", "5");
    m_Api->UndoCellValue("D3", "6");

    m_Api->UndoCellValue("A22", "=SUMPRODUCT(2,3,4)");
    wVariant = m_Api->CellValue("A22");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 24); // 2*3*4 = 24

    m_Api->UndoCellValue("A23", "=SUMPRODUCT(C1:C3)");
    wVariant = m_Api->CellValue("A23");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 6); // 1+2+3 = 6

    m_Api->UndoCellValue("A24", "=SUMPRODUCT(C1:C3,D1:D3)");
    wVariant = m_Api->CellValue("A24");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 32); // 1*4 + 2*5 + 3*6 = 32

    m_Api->UndoCellValue("A25", "=SUMPRODUCT(C1:C3,2)");
    wVariant = m_Api->CellValue("A25");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 12); // (1+2+3)*2 = 12

    m_Api->UndoCellValue("A26", "=SUMPRODUCT(C1:C3,D1)");
    wVariant = m_Api->CellValue("A26");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 24); // broadcast D1=4 across C1:C3

    m_Api->UndoCellValue("A27", "=SUMPRODUCT(C1:C2,D1:D3)");
    wVariant = m_Api->CellValue("A27");
    CPPUNIT_ASSERT(wVariant.IsError()); // mismatched range sizes -> #VALUE!
    
    // Test RAND function (returns value between 0 and 1)
    m_Api->UndoCellValue("A18", "=RAND()");
    wVariant = m_Api->CellValue("A18");
    CPPUNIT_ASSERT(wVariant.Double() >= 0.0 && wVariant.Double() <= 1.0); // Should be between 0 and 1
    
    // Test RANDBETWEEN function
    m_Api->UndoCellValue("A19", "=RANDBETWEEN(1,10)");
    wVariant = m_Api->CellValue("A19");
    CPPUNIT_ASSERT(wVariant.Int() >= 1 && wVariant.Int() <= 10); // Should be between 1 and 10
    
    m_Api->UndoCellValue("A20", "=RANDBETWEEN(5,15)");
    wVariant = m_Api->CellValue("A20");
    CPPUNIT_ASSERT(wVariant.Int() >= 5 && wVariant.Int() <= 15); // Should be between 5 and 15
    
    m_Api->UndoCellValue("A21", "=RANDBETWEEN(-10,-5)");
    wVariant = m_Api->CellValue("A21");
    CPPUNIT_ASSERT(wVariant.Int() >= -10 && wVariant.Int() <= -5); // Should be between -10 and -5
}

void TestSkFunction::TestFunctionSpreadSheet() {
    // Test Count
    m_Api->UndoCellValue("A1", "1");
    m_Api->UndoCellValue("A2", "2");
    m_Api->UndoCellValue("A3", "3");
    
    m_Api->UndoCellValue("A10", "10");
    m_Api->UndoCellValue("A11", "11");
    m_Api->UndoCellValue("A12", "12");
    
    m_Api->UndoCellValue("A15", "=COUNT(A1:A12)");
    tVariant wVariant = m_Api->CellValue("A15");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 6);
    
    // Test COUNTA function (counts all non-empty cells)
    // Setup test data for COUNTA
    m_Api->UndoCellValue("D1", "Text1");
    m_Api->UndoCellValue("D2", "");
    m_Api->UndoCellValue("D3", "Text2");
    m_Api->UndoCellValue("D4", 0); // Zero is not empty
    m_Api->UndoCellValue("D5", "Text3");
    
    m_Api->UndoCellValue("A16", "=COUNTA(D1:D5)");
    wVariant = m_Api->CellValue("A16");
    // In Excel, COUNTA counts cells with empty string "" as non-empty
    // D1="Text1", D2="", D3="Text2", D4=0, D5="Text3" -> all 5 are counted
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 5); // All 5 cells are non-empty (including empty string)
    
    m_Api->UndoCellValue("A17", "=COUNTA(D1,D2,D3,D4,D5)");
    wVariant = m_Api->CellValue("A17");
    // In Excel, COUNTA counts cells with empty string "" as non-empty
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 5); // All 5 cells are non-empty (including empty string)
    
    m_Api->UndoCellValue("A18", "=COUNTA(A1,A2,A3)");
    wVariant = m_Api->CellValue("A18");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // All three have values
    
    // Test COUNTBLANK function
    // Note: In Excel, COUNTBLANK counts only truly empty cells, not cells with empty string ""
    // D2 contains "" which is NOT considered blank by COUNTBLANK
    m_Api->UndoCellValue("A19", "=COUNTBLANK(D1:D5)");
    wVariant = m_Api->CellValue("A19");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0); // D2 has "" which is not blank, so 0 blank cells
    
    m_Api->UndoCellValue("A22", "=COUNTBLANK(D1,D2,D3)");
    wVariant = m_Api->CellValue("A22");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0); // D2 has "" which is not blank, so 0 blank cells
    
    // Test COUNTBLANK with empty range
    // Note: UndoCellValue("E1", "") creates a cell with empty string "", which is NOT blank
    // To test truly empty cells, we need cells that don't exist or have null values
    // F1 and F2 may have been used in previous tests or may not exist, so COUNTBLANK returns 0
    m_Api->UndoCellValue("A23", "=COUNTBLANK(F1:F2)");
    wVariant = m_Api->CellValue("A23");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0); // F1 and F2 are not blank (may not exist or have values)
    
    // DataRange function for Component Floating Object
    m_Api->UndoCellValue("A24", "=DATARANGE(A1:A3;B2;C1:C2)");
    wVariant = m_Api->CellValue("A24");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("[\"A1:A3\",\"B2\",\"C1:C2\"]"));

    m_Api->UndoCellValue("A25", "=DATARANGE(A1:A3,B2,C1)");
    wVariant = m_Api->CellValue("A25");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("[\"A1:A3\",\"B2\",\"C1\"]"));

    m_Api->UndoCellValue("A26", "=DataRange(A1:A3;B2;C1)");
    wVariant = m_Api->CellValue("A26");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("[\"A1:A3\",\"B2\",\"C1\"]"));

    m_Api->UndoCellValue("B1", "Name");
    m_Api->UndoCellValue("B2", "ALLEZ");
    m_Api->UndoCellValue("B3", "DUPONT");
    m_Api->UndoCellValue("B4", "HENRI");
    
    m_Api->UndoCellValue("B5", "LEROY");
    m_Api->UndoCellValue("B6", "LACOUR");
    m_Api->UndoCellValue("B7", "BEAUCHAMP");
    
    m_Api->UndoCellValue("C1", "Surname");
    m_Api->UndoCellValue("C2", "Stéphane");
    m_Api->UndoCellValue("C3", "Pierre");
    m_Api->UndoCellValue("C4", "Martin");
    
    m_Api->UndoCellValue("C5", "Anne");
    m_Api->UndoCellValue("C6", "Sophie");
    m_Api->UndoCellValue("C7", "Marie");

    m_Api->UndoCellValue("A27", "=JSON(B2:B7)");
    wVariant = m_Api->CellValue("A27");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(
        wVariant.String(),
        tString("[\"ALLEZ\",\"DUPONT\",\"HENRI\",\"LEROY\",\"LACOUR\",\"BEAUCHAMP\"]"));

    m_Api->UndoCellValue("A28", "=JSON(B2:C7)");
    wVariant = m_Api->CellValue("A28");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(
        wVariant.String(),
        tString("[{\"value\":\"ALLEZ\",\"label\":\"Stéphane\"},{\"value\":\"DUPONT\",\"label\":\"Pierre\"},"
                "{\"value\":\"HENRI\",\"label\":\"Martin\"},{\"value\":\"LEROY\",\"label\":\"Anne\"},"
                "{\"value\":\"LACOUR\",\"label\":\"Sophie\"},{\"value\":\"BEAUCHAMP\",\"label\":\"Marie\"}]"));

    // US locale (setUp): comma separates function args — B2, B4, C6 as three single cells.
    m_Api->UndoCellValue("A29", "=JSON(B2,B4,C6)");
    wVariant = m_Api->CellValue("A29");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("[\"ALLEZ\",\"HENRI\",\"Sophie\"]"));

    m_Api->UndoCellValue("A30", "=JSON(B2:B4,C6)");
    wVariant = m_Api->CellValue("A30");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(
        wVariant.String(),
        tString("[\"ALLEZ\",\"DUPONT\",\"HENRI\",\"Sophie\"]"));
    
    m_Api->UndoCellValue("A15", "=INDEX(B2:C7,2,2)");
    tCell* wCellA15=m_Api->Cell("A15");
    wVariant=wCellA15->Value();
    //cout << wCellA15->Value();
    tString wTest="Pierre";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wTest);
    
    m_Api->UndoCellValue("B16", "=VLOOKUP(B5,B2:C7,2,FALSE)");
    tCell* wCellB16=m_Api->Cell("B16");
    wVariant=wCellB16->Value();
    //cout << wVariant << endl;
    wTest="Anne";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wTest);
    
    m_Api->UndoCellValue("B17", "=HLOOKUP(\"Surname\",B1:C7,5,FALSE)");
    tCell* wCellB17=m_Api->Cell("B17");
    wVariant=wCellB17->Value();
    //cout << wVariant << endl;
    wTest="Anne";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wTest);

    // VLOOKUP/HLOOKUP Excel-compatible: range_lookup, #N/A, #REF!
    m_Api->UndoCellValue("F1", 10);
    m_Api->UndoCellValue("F2", 20);
    m_Api->UndoCellValue("F3", 30);
    m_Api->UndoCellValue("F4", 40);
    m_Api->UndoCellValue("F5", 50);
    m_Api->UndoCellValue("G1", "a");
    m_Api->UndoCellValue("G2", "b");
    m_Api->UndoCellValue("G3", "c");
    m_Api->UndoCellValue("G4", "d");
    m_Api->UndoCellValue("G5", "e");

    m_Api->UndoCellValue("H1", "=VLOOKUP(20,F1:G5,2,FALSE)");
    wVariant = m_Api->CellValue("H1");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("b"));

    m_Api->UndoCellValue("H2", "=VLOOKUP(25,F1:G5,2)");
    wVariant = m_Api->CellValue("H2");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("b"));

    m_Api->UndoCellValue("H3", "=VLOOKUP(25,F1:G5,2,FALSE)");
    CPPUNIT_ASSERT(m_Api->CellValue("H3").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H3").Error().Code(), tTypeError::t_na);

    m_Api->UndoCellValue("H4", "=VLOOKUP(100,F1:G5,2,TRUE)");
    wVariant = m_Api->CellValue("H4");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("e"));

    m_Api->UndoCellValue("H5", "=VLOOKUP(5,F1:G5,2,TRUE)");
    CPPUNIT_ASSERT(m_Api->CellValue("H5").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H5").Error().Code(), tTypeError::t_na);

    m_Api->UndoCellValue("H6", "=VLOOKUP(20,F1:G5,3,FALSE)");
    CPPUNIT_ASSERT(m_Api->CellValue("H6").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H6").Error().Code(), tTypeError::t_ref);

    m_Api->UndoCellValue("I1", 10);
    m_Api->UndoCellValue("J1", 20);
    m_Api->UndoCellValue("K1", 30);
    m_Api->UndoCellValue("I2", "x");
    m_Api->UndoCellValue("J2", "y");
    m_Api->UndoCellValue("K2", "z");

    m_Api->UndoCellValue("H7", "=HLOOKUP(25,I1:K2,2,TRUE)");
    wVariant = m_Api->CellValue("H7");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("y"));

    m_Api->UndoCellValue("H8", "=HLOOKUP(25,I1:K2,2,FALSE)");
    CPPUNIT_ASSERT(m_Api->CellValue("H8").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H8").Error().Code(), tTypeError::t_na);

    // LOOKUP vector approx (largest key <= lookup)
    m_Api->UndoCellValue("H20", "=LOOKUP(25,F1:F5,G1:G5)");
    wVariant = m_Api->CellValue("H20");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("b"));

    m_Api->UndoCellValue("H21", "=LOOKUP(100,F1:F5,G1:G5)");
    wVariant = m_Api->CellValue("H21");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("e"));

    m_Api->UndoCellValue("H22", "=LOOKUP(5,F1:F5,G1:G5)");
    CPPUNIT_ASSERT(m_Api->CellValue("H22").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H22").Error().Code(), tTypeError::t_na);

    // XLOOKUP exact match + if_not_found
    m_Api->UndoCellValue("H9", "=XLOOKUP(20,F1:F5,G1:G5)");
    wVariant = m_Api->CellValue("H9");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("b"));

    m_Api->UndoCellValue("H10", "=XLOOKUP(99,F1:F5,G1:G5,\"missing\")");
    wVariant = m_Api->CellValue("H10");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("missing"));

    m_Api->UndoCellValue("H11", "=XLOOKUP(99,F1:F5,G1:G5)");
    CPPUNIT_ASSERT(m_Api->CellValue("H11").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H11").Error().Code(), tTypeError::t_na);

    // XMATCH exact match + search last-to-first
    m_Api->UndoCellValue("H12", "=XMATCH(20,F1:F5)");
    wVariant = m_Api->CellValue("H12");
    CPPUNIT_ASSERT(wVariant.IsInt());
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);

    m_Api->UndoCellValue("H13", "=XMATCH(99,F1:F5)");
    CPPUNIT_ASSERT(m_Api->CellValue("H13").IsError());
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("H13").Error().Code(), tTypeError::t_na);
    
    // Test MATCH function
    // Test MATCH with exact match (match_type = 0)
    //cout << endl << "=MATCH(\"DUPONT\",B2:B7,0)" << endl;
    m_Api->UndoCellValue("B18", "=MATCH(\"DUPONT\",B2:B7,0)");
    tCell* wCellB18=m_Api->Cell("B18");
    wVariant=wCellB18->Value();
    //cout << "B18=" << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2); // "DUPONT" is at position 2 in B2:B7
    
    // Test MATCH with exact match for "LEROY"
    m_Api->UndoCellValue("B19", "=MATCH(\"LEROY\",B2:B7,0)");
    tCell* wCellB19=m_Api->Cell("B19");
    wVariant=wCellB19->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 4); // "LEROY" is at position 4 in B2:B7
    
    // Test MATCH with default match_type (1 = greater or equal) - ascending order
    m_Api->UndoCellValue("A20", 10);
    m_Api->UndoCellValue("A21", 20);
    m_Api->UndoCellValue("A22", 30);
    m_Api->UndoCellValue("A23", 40);
    m_Api->UndoCellValue("A24", 50);
    
    m_Api->UndoCellValue("B20", "=MATCH(25,A20:A24)");
    tCell* wCellB20=m_Api->Cell("B20");
    wVariant=wCellB20->Value();
    //cout << "B20=" << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // 25 >= 30, so position 3
    
    // Test MATCH with exact value in ascending order
    m_Api->UndoCellValue("B21", "=MATCH(30,A20:A24)");
    tCell* wCellB21=m_Api->Cell("B21");
    wVariant=wCellB21->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // Exact match at position 3
    
    // Test MATCH with match_type = -1 (less or equal) - descending order
    m_Api->UndoCellValue("C20", 50);
    m_Api->UndoCellValue("C21", 40);
    m_Api->UndoCellValue("C22", 30);
    m_Api->UndoCellValue("C23", 20);
    m_Api->UndoCellValue("C24", 10);
    
    m_Api->UndoCellValue("D20", "=MATCH(25,C20:C24,-1)");
    tCell* wCellD20=m_Api->Cell("D20");
    wVariant=wCellD20->Value();
    //cout << "D20=" << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // 25 <= 30, so position 3
    
    // Test MATCH with value not found (should return #N/A)
    m_Api->UndoCellValue("B22", "=MATCH(\"NOTFOUND\",B2:B7,0)");
    tCell* wCellB22=m_Api->Cell("B22");
    wVariant=wCellB22->Value();
    tVariant wVariantError;
    tClassError wClassError(tTypeError::t_na, "");
    wVariantError.SetError(wClassError);
    CPPUNIT_ASSERT_EQUAL(wVariantError.Error().Code(), wVariant.Error().Code());
    
    // Test MATCH with horizontal range
    m_Api->UndoCellValue("E1", "Apple");
    m_Api->UndoCellValue("F1", "Banana");
    m_Api->UndoCellValue("G1", "Orange");
    m_Api->UndoCellValue("H1", "Grape");
    
    m_Api->UndoCellValue("E2", "=MATCH(\"Orange\",E1:H1,0)");
    tCell* wCellE2=m_Api->Cell("E2");
    wVariant=wCellE2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // "Orange" is at position 3 in E1:H1

    // Whole-column MATCH/INDEX (Excel A:A / $B:$B). Must not collapse to A1:A1.
    m_Api->UndoCellValue("J1", "P.A.");
    m_Api->UndoCellValue("K1", "XA136");
    m_Api->UndoCellValue("J2", "=INDEX(J:J,MATCH(\"XA136\",$K:$K,0))");
    tCell* wCellJ2=m_Api->Cell("J2");
    wVariant=wCellJ2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("P.A."));
    
    m_Api->UndoCellValue("A16", "=ROWS(A2:$A$16)");
    tCell* wCellA16=m_Api->Cell("A16");
    wVariant=wCellA16->Value();
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 15);
    
     m_Api->UndoCellValue("B16", "=A16+1");
    
#if defined(_DEBUG) && defined(printdebug)
    DebugRow("A16=ROWS(A2:$A$16)");
    tSheet* wSheet=m_Api->ActiveSheet();
    cout << wSheet->Debug() << endl;
#endif
    
    m_Api->UndoInsertRow(3, 4);
    
#if defined(_DEBUG) && defined(printdebug)
    DebugRow("InsertRow 3,4");
    wSheet=m_Api->ActiveSheet();
    cout << wSheet->Debug() << endl;
#endif
    
    
    tCell* wCellA20=m_Api->Cell("A20");
    wVariant=wCellA20->Value();
    //cout << endl << wVariant.Int() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 19);
    
    wCellB20=m_Api->Cell("B20");
    wVariant=wCellB20->Value();
      //cout << endl << wVariant.Int() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 20);
    
    m_Api->UndoDeleteRow(3, 4);
#if defined(_DEBUG) && defined(printdebug)
    DebugRow("DeleteRow 3,4");
    wSheet=m_Api->ActiveSheet();
    cout << wSheet->Debug() << endl;
#endif
    wVariant=wCellA16->Value();
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 15);
    
    m_Api->Undo();
    m_Api->Undo();
    
    
    
#if defined(_DEBUG) && defined(printdebug)
    DebugRow("Undo x 2");
    wSheet=m_Api->ActiveSheet();
    cout << wSheet->Debug() << endl;
#endif
    
    wCellA16=m_Api->Cell("A16");
    wVariant=wCellA16->Value();
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 15);
    
    m_Api->UndoInsertRow(2, 2);
    
    
#if defined(_DEBUG) && defined(printdebug)
    cout << wSheet->Debug() << endl;
#endif
    
    
    
    m_Api->UndoCellValue("A17", "=COLUMNS(B1:C6)");
    tCell* wCellA17=m_Api->Cell("A17");
    wVariant=wCellA17->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);
    
#if defined(_DEBUG) && defined(printdebug)
    cout << wSheet->Debug() << endl;
#endif
    
    m_Api->UndoCellValue("A18", "=ROW()");
    tCell* wCellA18=m_Api->Cell("A18");
    wVariant=wCellA18->Value();
    //cout << endl << wVariant.Int() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 18);
    
    m_Api->UndoInsertRangeNamed("TESTROW", "A12:B12");
    m_Api->UndoCellValue("A19", "=ROW(TESTROW)");
    tCell* wCellA19=m_Api->Cell("A19");
    wVariant=wCellA19->Value();
    //cout << endl << wVariant.Int() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 12);
    
    
    
#if defined(_DEBUG) && defined(printdebug)
    cout << wSheet->Debug() << endl;
#endif
    
    
    m_Api->UndoInsertRow(12, 2);
    wCellA20=m_Api->Cell("A20");
    wVariant=wCellA20->Value();
    //cout << endl << wVariant.Int() << endl;
    
    m_Api->Undo();
    
    m_Api->UndoCellValue("B3", "=COLUMN()");
    wCellA18=m_Api->Cell("B3");
    wVariant=wCellA18->Value();
    //cout << endl << wVariant.Int() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);

    // ADDRESS: PopArgs is LIFO — 5-arg form must reverse before reading row/col.
    m_Api->UndoCellValue("G1", "=ADDRESS(1,2)");
    wVariant = m_Api->CellValue("G1");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("$B$1"));

    m_Api->UndoCellValue("G2", "=ADDRESS(1,2,1,TRUE,\"Rapport financier\")");
    wVariant = m_Api->CellValue("G2");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("'Rapport financier'!$B$1"));

    m_Api->UndoCellValue("G3", "=ADDRESS(12,12,4)");
    wVariant = m_Api->CellValue("G3");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("L12"));
    
#if defined(_DEBUG) && defined(printdebug)
    cout << wSheet->Debug() << endl;
#endif
    

}

void TestSkFunction::TestFunctionSumIf() {
    // Test SUMIF function
    // Setup test data for SUMIF - Start with simple numeric test
    m_Api->UndoCellValue("D1", "Values");
    m_Api->UndoCellValue("D2", 10);
    m_Api->UndoCellValue("D3", 20);
    m_Api->UndoCellValue("D4", 10);
    m_Api->UndoCellValue("D5", 30);
    m_Api->UndoCellValue("D6", 10);
    
    m_Api->UndoCellValue("E1", "Sales");
    m_Api->UndoCellValue("E2", 100);
    m_Api->UndoCellValue("E3", 150);
    m_Api->UndoCellValue("E4", 200);
    m_Api->UndoCellValue("E5", 75);
    m_Api->UndoCellValue("E6", 120);
    // Test SUMIF with numeric criteria first
    m_Api->UndoCellValue("F1", "=SUMIF(D2:D6,10,E2:E6)");
    tCell* wCellF1 = m_Api->Cell("F1");
    tVariant wVariant = wCellF1->Value();
    //cout << "wCellF1=" << wVariant;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 420); // 100 + 200 + 120 = 420
    
    // Test SUMIF with string criteria
    m_Api->UndoCellValue("D1", "Product");
    m_Api->UndoCellValue("D2", "Apple");
    m_Api->UndoCellValue("D3", "Banana");
    m_Api->UndoCellValue("D4", "Apple");
    m_Api->UndoCellValue("D5", "Orange");
    m_Api->UndoCellValue("D6", "Apple");
    
    m_Api->UndoCellValue("F2", "=SUMIF(D2:D6,\"Apple\",E2:E6)");
    tCell* wCellF2 = m_Api->Cell("F2");
    wVariant = wCellF2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 420); // 100 + 200 + 120 = 420
    
    // Test SUMIF without sum range (sum the criteria cells themselves)
    m_Api->UndoCellValue("F2", "=SUMIF(E2:E6,\">100\")");
    wCellF2 = m_Api->Cell("F2");
    
    wVariant = wCellF2->Value();
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 470); // 150 + 200 + 120 = 470
    
    // Test SUMIF with numeric criteria
    m_Api->UndoCellValue("F3", "=SUMIF(E2:E6,\"150\")");
    tCell* wCellF3 = m_Api->Cell("F3");
    wVariant = wCellF3->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 150);
    
    // Test SUMIF with empty result
    m_Api->UndoCellValue("F4", "=SUMIF(D2:D6,\"Grape\",E2:E6)");
    tCell* wCellF4 = m_Api->Cell("F4");
    wVariant = wCellF4->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);
    
    // Test SUMIF with mixed data types
    m_Api->UndoCellValue("G1", "Type");
    m_Api->UndoCellValue("G2", "A");
    m_Api->UndoCellValue("G3", "B");
    m_Api->UndoCellValue("G4", "A");
    m_Api->UndoCellValue("G5", "A");
    
    m_Api->UndoCellValue("H1", "Value");
    m_Api->UndoCellValue("H2", 10);
    m_Api->UndoCellValue("H3", 20);
    m_Api->UndoCellValue("H4", 30);
    m_Api->UndoCellValue("H5", 40);
    
    m_Api->UndoCellValue("I1", "=SUMIF(G2:G5,\"A\",H2:H5)");
    tCell* wCellI1 = m_Api->Cell("I1");
    wVariant = wCellI1->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 80); // 10 + 30 + 40 = 80

    // Text in sum_range is ignored (Excel SUMIF; INSIDE DEMO #R: labels in column C).
    m_Api->UndoCellValue("P2", "BA_AB-");
    m_Api->UndoCellValue("P3", "BA_AB-");
    m_Api->UndoCellValue("P4", "Other");
    m_Api->UndoCellValue("Q2", "#R:BA_AB-");
    m_Api->UndoCellValue("Q3", 100);
    m_Api->UndoCellValue("Q4", 50);
    m_Api->UndoCellValue("R1", "=SUMIF(P2:P4,\"BA_AB-\",Q2:Q4)");
    wVariant = m_Api->Cell("R1")->Value();
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 100);
    
    // Test SUMIF with wildcards
    m_Api->UndoCellValue("J1", "Names");
    m_Api->UndoCellValue("J2", "John");
    m_Api->UndoCellValue("J3", "Jane");
    m_Api->UndoCellValue("J4", "Jack");
    m_Api->UndoCellValue("J5", "Jill");
    
    m_Api->UndoCellValue("K1", "Scores");
    m_Api->UndoCellValue("K2", 85);
    m_Api->UndoCellValue("K3", 92);
    m_Api->UndoCellValue("K4", 78);
    m_Api->UndoCellValue("K5", 88);
    
    // Test wildcard * (matches any characters)
    m_Api->UndoCellValue("L1", "=SUMIF(J2:J5,\"J*\",K2:K5)");
    tCell* wCellL1 = m_Api->Cell("L1");
    wVariant = wCellL1->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 343); // 85 + 92 + 78 + 88 = 343 (all start with J)
    
    // Test wildcard ? (matches single character)
    m_Api->UndoCellValue("L2", "=SUMIF(J2:J5,\"J???\",K2:K5)");
    tCell* wCellL2 = m_Api->Cell("L2");
    wVariant = wCellL2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 343); // 85 + 92 + 78 + 88 = 343 (all are 4 characters starting with J)
    
    // Test specific wildcard pattern
    m_Api->UndoCellValue("L3", "=SUMIF(J2:J5,\"Ja*\",K2:K5)");
    tCell* wCellL3 = m_Api->Cell("L3");
    wVariant = wCellL3->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 92+78); // Only "Jane" matches "Ja*"
    
    // ------------------------------
    // AVERAGEIF tests
    // Using the same D/E data prepared above
    // 1) Two-argument form: AVERAGEIF(E2:E6, ">100") -> average of 150,200,120 = 470/3 ≈ 156.666...
    m_Api->UndoCellValue("N1", "=AVERAGEIF(E2:E6,\">100\")");
    tCell* wCellN1 = m_Api->Cell("N1");
    wVariant = wCellN1->Value();
    //cout << wVariant;
    // Compare using floor as requested: 470/3 => 156.66... => 156
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 156.0);

    // 2) Three-argument form: AVERAGEIF(D2:D6, "Apple", E2:E6) -> average of 100,200,120 = 420/3 = 140
    m_Api->UndoCellValue("N2", "=AVERAGEIF(D2:D6,\"Apple\",E2:E6)");
    tCell* wCellN2 = m_Api->Cell("N2");
    wVariant = wCellN2->Value();
    //cout << wVariant;
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(std::round(wVariant.Double())), 140);

    // 3) Wildcard criteria: AVERAGEIF(D2:D6, "A*", E2:E6) -> Apple rows -> 100,200,120 -> 140
    m_Api->UndoCellValue("N3", "=AVERAGEIF(D2:D6,\"A*\",E2:E6)");
    tCell* wCellN3 = m_Api->Cell("N3");
    wVariant = wCellN3->Value();
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(std::round(wVariant.Double())), 140);

    // 4) Exact numeric match on sum range same as criteria range
    // AVERAGEIF(E2:E6, "150") -> average of {150} = 150
    m_Api->UndoCellValue("N4", "=AVERAGEIF(E2:E6,\"150\")");
    tCell* wCellN4 = m_Api->Cell("N4");
    wVariant = wCellN4->Value();
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(std::round(wVariant.Double())), 150);

    // 5) No matches -> average should be 0 per current behavior
    m_Api->UndoCellValue("N5", "=AVERAGEIF(D2:D6,\"Grape\",E2:E6)");
    tCell* wCellN5 = m_Api->Cell("N5");
    wVariant = wCellN5->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);

    // ------------------------------
    // AVERAGEIFS tests
    // Test AVERAGEIFS with multiple criteria
    // AVERAGEIFS(E2:E6, D2:D6, "Apple", E2:E6, ">100") -> average where D="Apple" AND E>100
    // Expected: E4 (200) and E6 (120) -> (200+120)/2 = 160
    m_Api->UndoCellValue("O1", "=AVERAGEIFS(E2:E6,D2:D6,\"Apple\",E2:E6,\">100\")");
    tCell* wCellO1 = m_Api->Cell("O1");
    wVariant = wCellO1->Value();
    //cout << wVariant; // Debug output
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 160.0);

    // Test AVERAGEIFS with wildcards
    // AVERAGEIFS(E2:E6, D2:D6, "Ap*", E2:E6, ">50") -> average where D starts with "Ap" AND E>50
    // Expected: E2 (100), E4 (200), E6 (120) -> (100+200+120)/3 = 140
    m_Api->UndoCellValue("O2", "=AVERAGEIFS(E2:E6,D2:D6,\"Ap*\",E2:E6,\">50\")");
    tCell* wCellO2 = m_Api->Cell("O2");
    wVariant = wCellO2->Value();
    //cout << wVariant; // Debug output
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 140.0);

    // Test AVERAGEIFS with no matches
    // AVERAGEIFS(E2:E6, D2:D6, "Grape", G2:G6, ">200") -> no matches -> 0
    m_Api->UndoCellValue("O3", "=AVERAGEIFS(E2:E6,D2:D6,\"Grape\",G2:G6,\">200\")");
    tCell* wCellO3 = m_Api->Cell("O3");
    wVariant = wCellO3->Value();
    //cout << wVariant; // Debug output
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);

    // Test AVERAGEIFS with single criteria (should work like AVERAGEIF)
    // AVERAGEIFS(E2:E6, D2:D6, "Banana") -> average of E3 (150) -> 150
    m_Api->UndoCellValue("O4", "=AVERAGEIFS(E2:E6,D2:D6,\"Banana\")");
    tCell* wCellO4 = m_Api->Cell("O4");
    wVariant = wCellO4->Value();
    // cout << wVariant; // Debug output
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 150.0);
    // Test COUNTIF with same data
    m_Api->UndoCellValue("M1", "=COUNTIF(J2:J5,\"J*\")");
    tCell* wCellM1 = m_Api->Cell("M1");
    wVariant = wCellM1->Value();
    //cout << endl <<  wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 4); // All 4 names start with J
    
    // Test COUNTIF with wildcard ?
    m_Api->UndoCellValue("M2", "=COUNTIF(J2:J5,\"J???\")");
    tCell* wCellM2 = m_Api->Cell("M2");
    wVariant = wCellM2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 4); // All 4 names are 4 characters starting with J
    
    // Test COUNTIF with specific pattern
    m_Api->UndoCellValue("M3", "=COUNTIF(J2:J5,\"Ja*\")");
    tCell* wCellM3 = m_Api->Cell("M3");
    wVariant = wCellM3->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2); // Only "Jane" and Jack matches "Ja*"
    
    // Test COUNTIF with numeric criteria
    m_Api->UndoCellValue("M4", "=COUNTIF(K2:K5,\">80\")");
    tCell* wCellM4 = m_Api->Cell("M4");
    wVariant = wCellM4->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // 85, 92, 88 are > 80
    
    // Test COUNTIF with exact numeric match
    m_Api->UndoCellValue("M5", "=COUNTIF(K2:K5,\"92\")");
    tCell* wCellM5 = m_Api->Cell("M5");
    wVariant = wCellM5->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 1); // Only 92 matches exactly

    // ------------------------------
    // "<>" (not equal) and "=" (explicit equal) operators
    // D2:D6 = Apple, Banana, Apple, Orange, Apple ; E2:E6 = 100,150,200,75,120
    // Text "<>": everything that is not "Apple" -> Banana(150) + Orange(75) = 225
    m_Api->UndoCellValue("M6", "=SUMIF(D2:D6,\"<>Apple\",E2:E6)");
    wVariant = m_Api->Cell("M6")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 225);

    // Text "<>": count of non-Apple products -> Banana + Orange = 2
    m_Api->UndoCellValue("M7", "=COUNTIF(D2:D6,\"<>Apple\")");
    wVariant = m_Api->Cell("M7")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);

    // Numeric "<>": K2:K5 = 85,92,78,88 ; not equal to 92 -> 85,78,88 = 3
    m_Api->UndoCellValue("M8", "=COUNTIF(K2:K5,\"<>92\")");
    wVariant = m_Api->Cell("M8")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);

    // Explicit "=" numeric: only E rows equal to 150 -> 150
    m_Api->UndoCellValue("M9", "=SUMIF(E2:E6,\"=150\")");
    wVariant = m_Api->Cell("M9")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 150);

    // Explicit "=" text: count of products equal to "Apple" -> 3
    m_Api->UndoCellValue("M10", "=COUNTIF(D2:D6,\"=Apple\")");
    wVariant = m_Api->Cell("M10")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);

    // ------------------------------
    // "~" escape: match a literal * or ? instead of treating it as a wildcard
    m_Api->UndoCellValue("S2", "A*B");
    m_Api->UndoCellValue("S3", "AXB");
    m_Api->UndoCellValue("S4", "A*B");
    m_Api->UndoCellValue("T2", 10);
    m_Api->UndoCellValue("T3", 20);
    m_Api->UndoCellValue("T4", 30);
    // "A~*B" matches only the literal "A*B" cells (S2, S4) -> 10 + 30 = 40
    m_Api->UndoCellValue("U1", "=SUMIF(S2:S4,\"A~*B\",T2:T4)");
    wVariant = m_Api->Cell("U1")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 40);

    // Without the escape, "A*B" is a wildcard and matches all three -> count = 3
    m_Api->UndoCellValue("U2", "=COUNTIF(S2:S4,\"A*B\")");
    wVariant = m_Api->Cell("U2")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);

    // Escaped wildcard count: only literal "A*B" -> 2
    m_Api->UndoCellValue("U3", "=COUNTIF(S2:S4,\"A~*B\")");
    wVariant = m_Api->Cell("U3")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);
}
void TestSkFunction::TestFunctionSumIfs() {
    // Prepare data
    m_Api->UndoCellValue("A1", "Product");
    m_Api->UndoCellValue("A2", "Apple");
    m_Api->UndoCellValue("A3", "Banana");
    m_Api->UndoCellValue("A4", "Apple");
    m_Api->UndoCellValue("A5", "Orange");
    m_Api->UndoCellValue("A6", "Apple");

    m_Api->UndoCellValue("B1", "Region");
    m_Api->UndoCellValue("B2", "North");
    m_Api->UndoCellValue("B3", "South");
    m_Api->UndoCellValue("B4", "North");
    m_Api->UndoCellValue("B5", "West");
    m_Api->UndoCellValue("B6", "North");

    m_Api->UndoCellValue("C1", "Sales");
    m_Api->UndoCellValue("C2", 100);
    m_Api->UndoCellValue("C3", 150);
    m_Api->UndoCellValue("C4", 200);
    m_Api->UndoCellValue("C5", 75);
    m_Api->UndoCellValue("C6", 120);

    // SUMIFS with 2 criteria: Product=Apple AND Region=North -> rows 2,4,6 => 100+200+120=420
    m_Api->UndoCellValue("D1", "=SUMIFS(C2:C6,A2:A6,\"Apple\",B2:B6,\"North\")");
    tCell* wCellD1 = m_Api->Cell("D1");
    tVariant wVariant = wCellD1->Value();
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 420);

    // SUMIFS with numeric comparator: Sales>100 AND Product=Apple -> rows 4,6 => 200+120=320
    m_Api->UndoCellValue("D2", "=SUMIFS(C2:C6,C2:C6,\">100\",A2:A6,\"Apple\")");
    tCell* wCellD2 = m_Api->Cell("D2");
    wVariant = wCellD2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 320);

    // SUMIFS with wildcard: Product starts with 'A*' AND Region='North' -> rows 2,4,6 => 420
    m_Api->UndoCellValue("D3", "=SUMIFS(C2:C6,A2:A6,\"A*\",B2:B6,\"North\")");
    tCell* wCellD3 = m_Api->Cell("D3");
    wVariant = wCellD3->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 420);

    // SUMIFS with 3 criteria: Product=Apple AND Region=North AND Sales>100 -> rows 4,6 => 320
    m_Api->UndoCellValue("D4", "=SUMIFS(C2:C6,A2:A6,\"Apple\",B2:B6,\"North\",C2:C6,\">100\")");
    tCell* wCellD4 = m_Api->Cell("D4");
    wVariant = wCellD4->Value();
    /*
    cout << endl << wCellD4->FormulaStr() << endl;
    cout << wCellD4->Debug();
    cout << "Result=" << wVariant.Int() << "<>320" << endl;
    */
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 320);

    // Text in sum_range is ignored (same semantics as SUMIF).
    m_Api->UndoCellValue("F2", "#R:label");
    m_Api->UndoCellValue("F4", 200);
    m_Api->UndoCellValue("F6", 120);
    m_Api->UndoCellValue("E1", "=SUMIFS(F2:F6,A2:A6,\"Apple\",B2:B6,\"North\")");
    wVariant = m_Api->Cell("E1")->Value();
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 320);

    // SUMIFS misaligned ranges should return #VALUE error
    // Make an intentionally misaligned criteria range
    m_Api->UndoCellValue("D5", "=SUMIFS(C2:C6,A2:A5,\"Apple\")");
    tCell* wCellD5 = m_Api->Cell("D5");
    wVariant = wCellD5->Value();
    tVariant wVariantError;
    tClassError wClassError(tTypeError::t_value, "");
    wVariantError.SetError(wClassError);
    CPPUNIT_ASSERT_EQUAL(wVariantError.Error().Code(), wVariant.Error().Code());
    
    // Use same data as SUMIFS test
    m_Api->UndoCellValue("A1", "Product");
    m_Api->UndoCellValue("A2", "Apple");
    m_Api->UndoCellValue("A3", "Banana");
    m_Api->UndoCellValue("A4", "Apple");
    m_Api->UndoCellValue("A5", "Orange");
    m_Api->UndoCellValue("A6", "Apple");

    m_Api->UndoCellValue("B1", "Region");
    m_Api->UndoCellValue("B2", "North");
    m_Api->UndoCellValue("B3", "South");
    m_Api->UndoCellValue("B4", "North");
    m_Api->UndoCellValue("B5", "West");
    m_Api->UndoCellValue("B6", "North");

    m_Api->UndoCellValue("C1", "Sales");
    m_Api->UndoCellValue("C2", 100);
    m_Api->UndoCellValue("C3", 150);
    m_Api->UndoCellValue("C4", 200);
    m_Api->UndoCellValue("C5", 75);
    m_Api->UndoCellValue("C6", 120);

    // COUNTIFS with 2 criteria: Product=Apple AND Region=North -> rows 2,4,6 => count = 3
    m_Api->UndoCellValue("E1", "=COUNTIFS(A2:A6,\"Apple\",B2:B6,\"North\")");
    tCell* wCellE1 = m_Api->Cell("E1");
    wVariant = wCellE1->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);

    // COUNTIFS with numeric comparator: Sales>100 AND Product=Apple -> rows 4,6 => count = 2
    m_Api->UndoCellValue("E2", "=COUNTIFS(C2:C6,\">100\",A2:A6,\"Apple\")");
    tCell* wCellE2 = m_Api->Cell("E2");
    wVariant = wCellE2->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);

    // COUNTIFS with wildcard: Product starts with 'A*' AND Region='North' -> rows 2,4,6 => count = 3
    m_Api->UndoCellValue("E3", "=COUNTIFS(A2:A6,\"A*\",B2:B6,\"North\")");
    tCell* wCellE3 = m_Api->Cell("E3");
    wVariant = wCellE3->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);

    // COUNTIFS with 3 criteria: Product=Apple AND Region=North AND Sales>100 -> rows 4,6 => count = 2
    m_Api->UndoCellValue("E4", "=COUNTIFS(A2:A6,\"Apple\",B2:B6,\"North\",C2:C6,\">100\")");
    tCell* wCellE4 = m_Api->Cell("E4");
    wVariant = wCellE4->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2);

    // COUNTIFS with no matches -> count = 0
    m_Api->UndoCellValue("E5", "=COUNTIFS(A2:A6,\"Grape\",B2:B6,\"North\")");
    tCell* wCellE5 = m_Api->Cell("E5");
    wVariant = wCellE5->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);

    // MAXIFS / MINIFS share SUMIFS criteria machinery (tFunctionAggIfs).
    // Apple + North -> values 100,200,120 -> max 200, min 100
    m_Api->UndoCellValue("G1", "=MAXIFS(C2:C6,A2:A6,\"Apple\",B2:B6,\"North\")");
    wVariant = m_Api->Cell("G1")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 200);

    m_Api->UndoCellValue("G2", "=MINIFS(C2:C6,A2:A6,\"Apple\",B2:B6,\"North\")");
    wVariant = m_Api->Cell("G2")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 100);

    // No match -> 0 (Excel semantics)
    m_Api->UndoCellValue("G3", "=MAXIFS(C2:C6,A2:A6,\"Grape\")");
    wVariant = m_Api->Cell("G3")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);

    m_Api->UndoCellValue("G4", "=MINIFS(C2:C6,A2:A6,\"Grape\")");
    wVariant = m_Api->Cell("G4")->Value();
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);
}


void TestSkFunction::TestFunctionLogical() {
    m_Api->UndoCellValue("A1", "=IF(1>0, \"True\", \"False\")");
    tVariant wVariant = m_Api->CellValue("A1");
    tString wValue="True";
    CPPUNIT_ASSERT_EQUAL(wValue,wVariant.String());

    m_Api->UndoCellValue("A1", "=IFERROR(1/0, \"Error\",\"OK\")");
    wVariant = m_Api->CellValue("A1");
    wValue="Error";
    //cout << endl << wVariant << endl;
    CPPUNIT_ASSERT_EQUAL(wValue,wVariant.String());

    m_Api->UndoCellValue("A1", "=NA()");
    wVariant = m_Api->CellValue("A1");
    tVariant wVariantError;
    tClassError wClassError(tTypeError::t_na, "");
    wVariantError.SetError(wClassError);
    CPPUNIT_ASSERT_EQUAL(wVariantError.Error().Code(),wVariant.Error().Code());

   

    m_Api->UndoCellValue("A1", "=ISBLANK(A2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true); // A2 is truly empty (null)

    // In Excel, ISBLANK("") returns FALSE because empty string "" is NOT blank
    // A cell is blank only if its variant IsNull()
    m_Api->UndoCellValue("A1", "=ISBLANK(\"\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false); // Empty string "" is NOT blank

    m_Api->UndoCellValue("A1", "=ISNA(NA())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    m_Api->UndoCellValue("A1", "=ISNA(5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A2", "=NA()");
    m_Api->UndoCellValue("A1", "=ISNA(A2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    m_Api->UndoCellValue("A1", "=ISNA(#REF!)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=ISNUMBER(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    m_Api->UndoCellValue("A1", "=ISNUMBER(\"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=ISNUMBER(DATE(2024,1,15))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    m_Api->UndoCellValue("A1", "=ISNUMBER(TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    // ISREF: Cell/Range → TRUE; literals / errors-as-values → FALSE
    CPPUNIT_ASSERT_MESSAGE("compile ISREF", m_Api->UndoCellValue("A1", "=ISREF(A2)"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISREF(A2:B3)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISREF(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISREF(\"A1\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISREF(#REF!)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISREF(INDIRECT(\"A2\"))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    // ISFORMULA: TRUE only when the referenced cell owns a formula; non-ref → #VALUE!
    m_Api->UndoCellValue("B1", 10);
    m_Api->UndoCellValue("B2", "=B1+1");
    m_Api->UndoCellValue("A1", "=ISFORMULA(B2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISFORMULA(B1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISFORMULA(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_value);

    // FORMULATEXT: formula with leading '='; no formula → #N/A; non-ref → #VALUE!
    m_Api->UndoCellValue("A1", "=FORMULATEXT(B2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL('=', wVariant.String()[0]);
    CPPUNIT_ASSERT(wVariant.String().find("B1") != tString::npos);
    m_Api->UndoCellValue("A1", "=FORMULATEXT(B1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_na, wVariant.Error().Code());
    m_Api->UndoCellValue("A1", "=FORMULATEXT(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_value, wVariant.Error().Code());

    // CELL: address / col / row / contents / type
    m_Api->UndoCellValue("C5", 42);
    m_Api->UndoCellValue("A1", "=CELL(\"address\", C5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("$C$5"), wVariant.String());
    m_Api->UndoCellValue("A1", "=CELL(\"col\", C5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(3, wVariant.Int());
    m_Api->UndoCellValue("A1", "=CELL(\"row\", C5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(5, wVariant.Int());
    m_Api->UndoCellValue("A1", "=CELL(\"contents\", C5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(42, wVariant.Int());
    m_Api->UndoCellValue("A1", "=CELL(\"type\", C5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("v"), wVariant.String());
    m_Api->UndoCellValue("D1", "hello");
    m_Api->UndoCellValue("A1", "=CELL(\"type\", D1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("l"), wVariant.String());

    // INFO
    m_Api->UndoCellValue("A1", "=INFO(\"system\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsString());
    m_Api->UndoCellValue("A1", "=INFO(\"recalc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("Automatic"), wVariant.String());
    m_Api->UndoCellValue("A1", "=INFO(\"numfile\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsNumeric());
    CPPUNIT_ASSERT(wVariant.Numeric() >= 1.0);

    // SHEET / SHEETS
    m_Api->UndoCellValue("A1", "=SHEET()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("A1", "=SHEET(\"Sheet1\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("A1", "=SHEETS()");
    wVariant = m_Api->CellValue("A1");
    const tInt wSheetsBefore = wVariant.Int();
    CPPUNIT_ASSERT(wSheetsBefore >= 1);
    m_Api->UndoAddSheet("Sheet2");
    m_Api->UndoCellValue("A1", "=SHEETS()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wSheetsBefore + 1, wVariant.Int());
    m_Api->UndoCellValue("A1", "=SHEET(\"Sheet2\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->ActiveSheet("Sheet2");
    m_Api->UndoCellValue("A1", "=SHEET()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoCellValue("A1", "=SHEET(Sheet2!A1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoDeleteSheet("Sheet2");

    // ISEVEN / ISODD (truncate toward zero, then parity)
    m_Api->UndoCellValue("A1", "=ISEVEN(2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISEVEN(2.9)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISODD(3)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISODD(3.9)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISEVEN(3)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    // N / TYPE
    m_Api->UndoCellValue("A1", "=N(TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(1.0, wVariant.Double());
    m_Api->UndoCellValue("A1", "=N(\"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(0.0, wVariant.Double());
    m_Api->UndoCellValue("A1", "=TYPE(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(1, wVariant.Int());
    m_Api->UndoCellValue("A1", "=TYPE(\"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("A1", "=TYPE(TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(4, wVariant.Int());
    // TYPE returns 64 for arrays / multi-cell ranges.
    m_Api->UndoCellValue("A1", "=TYPE(SEQUENCE(3))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(64, wVariant.Int());
    m_Api->UndoCellValue("B1", 1);
    m_Api->UndoCellValue("B2", 2);
    m_Api->UndoCellValue("A1", "=TYPE(B1:B2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(64, wVariant.Int());

    // ERROR_TYPE (Excel ERROR.TYPE) — numeric code for an error; non-error → #N/A.
    m_Api->UndoCellValue("A1", "=ERROR_TYPE(1/0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());
    m_Api->UndoCellValue("A1", "=ERROR_TYPE(FACT(-1))");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(6, wVariant.Int());
    m_Api->UndoCellValue("A1", "=ERROR_TYPE(NA())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(7, wVariant.Int());
    m_Api->UndoCellValue("A1", "=ERROR_TYPE(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(tTypeError::t_na, wVariant.Error().Code());

    // Shared tFunctionIsType: ISERROR / ISERR / ISLOGICAL / ISTEXT / ISNONTEXT.
    m_Api->UndoCellValue("A1", "=ISERROR(NA())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISERROR(1/0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISERROR(5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=ISERR(NA())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISERR(1/0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    m_Api->UndoCellValue("A1", "=ISLOGICAL(TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISLOGICAL(0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=ISTEXT(\"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISTEXT(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=ISNONTEXT(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=ISNONTEXT(\"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=ISNONTEXT(NA())");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    // Bare TRUE/FALSE are literals; TRUE()/FALSE() are 0-arg functions (shared impl).
    m_Api->UndoCellValue("A1", "=TRUE()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=FALSE()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=TRUE");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=FALSE");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=IFNA(NA(), \"missing\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("missing"));

    m_Api->UndoCellValue("A1", "=IFNA(5, \"missing\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(5, wVariant.Int());

    // IFNA must not trap non-#N/A errors
    m_Api->UndoCellValue("A1", "=IFNA(1/0, \"missing\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_div0);
    
    tBool wOk=m_Api->UndoCellValue("A1", "=AND(1>0, 1<0)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);

    m_Api->UndoCellValue("A1", "=OR(1>0, 1<0)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);

    // XOR: odd number of TRUE -> TRUE
    m_Api->UndoCellValue("A1", "=XOR(TRUE, FALSE, TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=XOR(TRUE, FALSE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    
    // Test IFS function
    m_Api->UndoCellValue("A2", "=IFS(1>2, \"First\", 2>1, \"Second\", 3>2, \"Third\")");
    wVariant = m_Api->CellValue("A2");
    //cout << endl << wVariant << endl;
    tString wIfsResult = "Second";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wIfsResult); // First true condition
    
    m_Api->UndoCellValue("A3", "=IFS(1>2, \"First\", 2>3, \"Second\", 3>2, \"Third\")");
    wVariant = m_Api->CellValue("A3");
    wIfsResult = "Third";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wIfsResult); // Only third condition is true
    
    m_Api->UndoCellValue("A4", "=IFS(1>2, \"First\", 2>3, \"Second\", \"Default\")");
    wVariant = m_Api->CellValue("A4");
    wIfsResult = "Default";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wIfsResult); // Else value when no condition is true
    
    m_Api->UndoCellValue("A5", "=IFS(5>10, 100, 10>5, 200)");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 200); // Numeric result
    
    m_Api->UndoCellValue("A6", "=IFS(1>2, \"A\", 2>3, \"B\")");
    wVariant = m_Api->CellValue("A6");
    // Should return #N/A error when no condition is true and no else value
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_na);
}

void TestSkFunction::TestFunctionIfShortCircuit() {
    // IF is now compiled to short-circuit opcodes (only the taken branch runs). These tests also validate that
    // the runtime ref index (wIndice) stays aligned when a branch is skipped: an IF followed by another cell
    // reference must still read the correct cell.
    m_Api->UndoCellValue("A2", "=1");
    m_Api->UndoCellValue("A3", "=100");
    m_Api->UndoCellValue("A4", "=200");
    m_Api->UndoCellValue("A5", "=5");
    m_Api->UndoCellValue("A6", "=9");

    // Simple branch selection.
    m_Api->UndoCellValue("A1", "=IF(2>1, 10, 20)");
    CPPUNIT_ASSERT_EQUAL(10, m_Api->CellValue("A1").Int());
    m_Api->UndoCellValue("A1", "=IF(2<1, 10, 20)");
    CPPUNIT_ASSERT_EQUAL(20, m_Api->CellValue("A1").Int());

    // then-branch taken, else-branch (A4) skipped, trailing ref (A5) must still resolve: A3 + A5 = 105.
    tBool wOk = m_Api->UndoCellValue("A1", "=IF(A2>0, A3, A4) + A5");
    if (!wOk) { cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl; }
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(105, m_Api->CellValue("A1").Int());

    // else-branch taken, then-branch (A3) skipped, trailing ref (A5) must still resolve: A4 + A5 = 205.
    m_Api->UndoCellValue("A1", "=IF(A2<0, A3, A4) + A5");
    CPPUNIT_ASSERT_EQUAL(205, m_Api->CellValue("A1").Int());

    // Nested IF inside the then-branch; validates wIndice alignment across nested skips.
    // A2>0 true -> inner IF(A2>5) false -> A4=200; skipped A3 (inner) and A6 (outer else); + A5 = 205.
    m_Api->UndoCellValue("A1", "=IF(A2>0, IF(A2>5, A3, A4), A6) + A5");
    CPPUNIT_ASSERT_EQUAL(205, m_Api->CellValue("A1").Int());

    // 2-argument IF: no else. TRUE -> the value; FALSE -> boolean FALSE (Excel semantics).
    m_Api->UndoCellValue("A1", "=IF(A2>0, 7)");
    CPPUNIT_ASSERT_EQUAL(7, m_Api->CellValue("A1").Int());
    m_Api->UndoCellValue("A1", "=IF(A2<0, 7)");
    CPPUNIT_ASSERT_EQUAL(false, m_Api->CellValue("A1").Bool());

    // Error condition propagates as the IF result.
    m_Api->UndoCellValue("A1", "=IF(NA(), 1, 2)");
    CPPUNIT_ASSERT(m_Api->CellValue("A1").IsError());

    // String branches still work.
    m_Api->UndoCellValue("A1", "=IF(1>0, \"True\", \"False\")");
    CPPUNIT_ASSERT_EQUAL(tString("True"), m_Api->CellValue("A1").String());
    m_Api->UndoCellValue("A1", "=IF(1<0, \"True\", \"False\")");
    CPPUNIT_ASSERT_EQUAL(tString("False"), m_Api->CellValue("A1").String());

    // Omitted then-branch: IF(ISNA(...);;value) — must not throw "Stack not empty".
    // FR locale uses ';' separators (same bytecode path as the AD93 situation template).
    wOk = m_Api->UndoCellValue("A1", "=IF(ISNA(NA());;42)");
    if (!wOk) { cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl; }
    CPPUNIT_ASSERT_MESSAGE("compile IF(ISNA(NA());;42)", wOk);
    CPPUNIT_ASSERT_MESSAGE("IF omitted then when TRUE is blank", m_Api->CellValue("A1").IsNull()
        || m_Api->CellValue("A1").IsExcelNull()
        || (m_Api->CellValue("A1").IsString() && m_Api->CellValue("A1").String().empty()));

    wOk = m_Api->UndoCellValue("A1", "=IF(ISNA(5);;42)");
    CPPUNIT_ASSERT_MESSAGE("compile IF(ISNA(5);;42)", wOk);
    CPPUNIT_ASSERT_EQUAL(42, m_Api->CellValue("A1").Int());

    // Omitted else-branch: IF(cond;then;) 
    wOk = m_Api->UndoCellValue("A1", "=IF(1>0;7;)");
    CPPUNIT_ASSERT_MESSAGE("compile IF(1>0;7;)", wOk);
    CPPUNIT_ASSERT_EQUAL(7, m_Api->CellValue("A1").Int());
    wOk = m_Api->UndoCellValue("A1", "=IF(1<0;7;)");
    CPPUNIT_ASSERT_MESSAGE("compile IF(1<0;7;)", wOk);
    CPPUNIT_ASSERT_MESSAGE("IF omitted else when FALSE is blank", m_Api->CellValue("A1").IsNull()
        || m_Api->CellValue("A1").IsExcelNull()
        || (m_Api->CellValue("A1").IsString() && m_Api->CellValue("A1").String().empty()));
}

void TestSkFunction::TestFunctionLet() {
    // Scalar binding: x = 5, body = x*2 -> 10
    tBool wOk = m_Api->UndoCellValue("A1", "=LET(x, 5, x*2)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT(wOk);
    tVariant wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());

    // Round-trip: the formula must still read as LET(...), not an expanded form.
    tString wFormula = m_Api->Formula("A1");
    CPPUNIT_ASSERT(wFormula.find("LET") != tString::npos);
    CPPUNIT_ASSERT(wFormula.find("x") != tString::npos);

    // A name used several times in the body.
    m_Api->UndoCellValue("A2", "=LET(x, 3, x + x*x)"); // 3 + 9 = 12
    wVariant = m_Api->CellValue("A2");
    CPPUNIT_ASSERT_EQUAL(12, wVariant.Int());

    // Cell reference inside a binding value. B1 must be a real number (not the text "4"),
    // so it is set via a numeric formula: a plain "4" would be stored as text and text*2 is #VALUE!.
    m_Api->UndoCellValue("B1", "=4");
    m_Api->UndoCellValue("A3", "=LET(x, B1*2, x + x)"); // (4*2)+(4*2) = 16
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT_EQUAL(16, wVariant.Int());

    // Cascading bindings: a later value references an earlier name.
    m_Api->UndoCellValue("A4", "=LET(x, 2, y, x+1, x*y)"); // x=2, y=3 -> 6
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT_EQUAL(6, wVariant.Int());

    // Nested LET.
    m_Api->UndoCellValue("A5", "=LET(x, 10, LET(y, 5, x+y))"); // 15
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(15, wVariant.Int());

    // A binding value that depends on a cell recomputes when the cell changes.
    m_Api->UndoCellValue("B1", "=10");
    wVariant = m_Api->CellValue("A3"); // (10*2)+(10*2) = 40
    CPPUNIT_ASSERT_EQUAL(40, wVariant.Int());
}

void TestSkFunction::TestFunctionSwitch() {
    // Match on a numeric expression, first matching value wins.
    tBool wOk = m_Api->UndoCellValue("A1", "=SWITCH(2, 1, 10, 2, 20, 3, 30)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT(wOk);
    tVariant wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(20, wVariant.Int());

    // Round-trip: the formula must still read as SWITCH(...).
    tString wFormula = m_Api->Formula("A1");
    CPPUNIT_ASSERT(wFormula.find("SWITCH") != tString::npos);

    // No match, with a trailing default (odd argument count after the expression).
    m_Api->UndoCellValue("A2", "=SWITCH(9, 1, 10, 2, 20, 99)"); // default 99
    wVariant = m_Api->CellValue("A2");
    CPPUNIT_ASSERT_EQUAL(99, wVariant.Int());

    // No match, no default -> #N/A.
    m_Api->UndoCellValue("A3", "=SWITCH(9, 1, 10, 2, 20)");
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_na);

    // String matching (engine equality, same as the "=" operator).
    m_Api->UndoCellValue("A4", "=SWITCH(\"b\", \"a\", 1, \"b\", 2, \"c\", 3)");
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT_EQUAL(2, wVariant.Int());

    // Expression taken from a cell reference.
    m_Api->UndoCellValue("B1", "=2");
    m_Api->UndoCellValue("A5", "=SWITCH(B1, 1, 100, 2, 200, 300)");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(200, wVariant.Int());

    // Recomputes when the referenced cell changes.
    m_Api->UndoCellValue("B1", "=1");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(100, wVariant.Int());
}

void TestSkFunction::TestFunctionSwitchArray() {
    // Element-wise SWITCH over an array expression (Excel dynamic array): each element is
    // switched independently and the result spills. C1:C3 = "a","b","c".
    m_Api->UndoCellValue("C1", "a");
    m_Api->UndoCellValue("C2", "b");
    m_Api->UndoCellValue("C3", "c");
    tBool wOk = m_Api->UndoCellValue("E1", "=SWITCH(C1:C3, \"a\", 10, \"b\", 20, \"c\", 30)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT(wOk);
    CPPUNIT_ASSERT_EQUAL(10, m_Api->CellValue("E1").Int());
    CPPUNIT_ASSERT_EQUAL(20, m_Api->CellValue("E2").Int());
    CPPUNIT_ASSERT_EQUAL(30, m_Api->CellValue("E3").Int());

    // No-match elements fall back to the default across the whole array. C4 has no matching value.
    m_Api->UndoCellValue("C4", "z");
    m_Api->UndoCellValue("G1", "=SWITCH(C1:C4, \"a\", 1, \"b\", 2, -1)"); // default -1
    CPPUNIT_ASSERT_EQUAL(1,  m_Api->CellValue("G1").Int());   // "a" -> 1
    CPPUNIT_ASSERT_EQUAL(2,  m_Api->CellValue("G2").Int());   // "b" -> 2
    CPPUNIT_ASSERT_EQUAL(-1, m_Api->CellValue("G3").Int());   // "c" -> default -1
    CPPUNIT_ASSERT_EQUAL(-1, m_Api->CellValue("G4").Int());   // "z" -> default -1

    // No default: an unmatched element yields #N/A for that cell only.
    m_Api->UndoCellValue("I1", "=SWITCH(C1:C4, \"a\", 1, \"b\", 2)");
    CPPUNIT_ASSERT_EQUAL(1, m_Api->CellValue("I1").Int());
    CPPUNIT_ASSERT_EQUAL(2, m_Api->CellValue("I2").Int());
    CPPUNIT_ASSERT(m_Api->CellValue("I3").IsError());        // "c" -> #N/A
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("I3").Error().Code(), tTypeError::t_na);

    // Broadcasting: a 1x3 header expression with 2x1 result arrays -> 2x3 output (league-table
    // pattern). Header P1:R1 = "P","W","D"; result columns are S1:S2, T1:T2, U1:U2.
    m_Api->UndoCellValue("P1", "P"); m_Api->UndoCellValue("Q1", "W"); m_Api->UndoCellValue("R1", "D");
    m_Api->UndoCellValue("S1", 5);   m_Api->UndoCellValue("S2", 6);   // -> "P" column
    m_Api->UndoCellValue("T1", 7);   m_Api->UndoCellValue("T2", 8);   // -> "W" column
    m_Api->UndoCellValue("U1", 9);   m_Api->UndoCellValue("U2", 10);  // -> "D" column
    wOk = m_Api->UndoCellValue("W1",
        "=SWITCH(P1:R1, \"P\", S1:S2, \"W\", T1:T2, \"D\", U1:U2)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT(wOk);
    // Row 1: 5,7,9 ; Row 2: 6,8,10 (col W=23, X=24, Y=25).
    CPPUNIT_ASSERT_EQUAL(5,  m_Api->CellValue("W1").Int());
    CPPUNIT_ASSERT_EQUAL(7,  m_Api->CellValue("X1").Int());
    CPPUNIT_ASSERT_EQUAL(9,  m_Api->CellValue("Y1").Int());
    CPPUNIT_ASSERT_EQUAL(6,  m_Api->CellValue("W2").Int());
    CPPUNIT_ASSERT_EQUAL(8,  m_Api->CellValue("X2").Int());
    CPPUNIT_ASSERT_EQUAL(10, m_Api->CellValue("Y2").Int());
}

void TestSkFunction::TestFunctionChoose() {
    // Positional selection: index 2 returns the second value.
    tBool wOk = m_Api->UndoCellValue("A1", "=CHOOSE(2, 10, 20, 30)");
    if (!wOk) {
        cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
    CPPUNIT_ASSERT(wOk);
    tVariant wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(20, wVariant.Int());

    // Round-trip: the formula must still read as CHOOSE(...).
    tString wFormula = m_Api->Formula("A1");
    CPPUNIT_ASSERT(wFormula.find("CHOOSE") != tString::npos);

    // String values.
    m_Api->UndoCellValue("A2", "=CHOOSE(3, \"Lun\", \"Mar\", \"Mer\")");
    wVariant = m_Api->CellValue("A2");
    tString wExpected = "Mer";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wExpected);

    // Decimal index is truncated toward zero (2.9 -> 2).
    m_Api->UndoCellValue("A3", "=CHOOSE(2.9, 100, 200, 300)");
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT_EQUAL(200, wVariant.Int());

    // Index out of range -> #VALUE!.
    m_Api->UndoCellValue("A4", "=CHOOSE(5, 1, 2, 3)");
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_value);

    // Index taken from a cell, recomputes on change.
    m_Api->UndoCellValue("B1", "=1");
    m_Api->UndoCellValue("A5", "=CHOOSE(B1, 111, 222, 333)");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(111, wVariant.Int());
    m_Api->UndoCellValue("B1", "=3");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(333, wVariant.Int());

    // A range argument selected by CHOOSE stays a range (SUM over it works).
    m_Api->UndoCellValue("C1", "=1");
    m_Api->UndoCellValue("C2", "=2");
    m_Api->UndoCellValue("C3", "=3");
    m_Api->UndoCellValue("A6", "=SUM(CHOOSE(1, C1:C3, D1:D3))"); // 1+2+3 = 6
    wVariant = m_Api->CellValue("A6");
    CPPUNIT_ASSERT_EQUAL(6, wVariant.Int());

    // Array index of scalars: CHOOSE({1,2,3}, ...) spills a 1x3 row (same shape as the index).
    CPPUNIT_ASSERT(m_Api->UndoCellValue("A10", "=CHOOSE({1,2,3}, \"a\", \"b\", \"c\")"));
    CPPUNIT_ASSERT_EQUAL(tString("a"), m_Api->CellValue("A10").String());
    CPPUNIT_ASSERT_EQUAL(tString("b"), m_Api->CellValue("B10").String());
    CPPUNIT_ASSERT_EQUAL(tString("c"), m_Api->CellValue("C10").String());

    // League-Table Part B Table B2: CHOOSE({1,2}, col, col) stitches columns; SORT by col 2 desc.
    m_Api->UndoCellValue("A12", "C");
    m_Api->UndoCellValue("B12", 10);
    m_Api->UndoCellValue("A13", "A");
    m_Api->UndoCellValue("B13", 30);
    m_Api->UndoCellValue("A14", "B");
    m_Api->UndoCellValue("B14", 20);
    CPPUNIT_ASSERT(m_Api->UndoCellValue("D12", "=SORT(CHOOSE({1,2}, A12:A14, B12:B14), 2, -1)"));
    CPPUNIT_ASSERT_EQUAL(tString("A"), m_Api->CellValue("D12").String());
    CPPUNIT_ASSERT_EQUAL(30, m_Api->CellValue("E12").Int());
    CPPUNIT_ASSERT_EQUAL(tString("B"), m_Api->CellValue("D13").String());
    CPPUNIT_ASSERT_EQUAL(20, m_Api->CellValue("E13").Int());
    CPPUNIT_ASSERT_EQUAL(tString("C"), m_Api->CellValue("D14").String());
    CPPUNIT_ASSERT_EQUAL(10, m_Api->CellValue("E14").Int());
}

void TestSkFunction::TestFunctionText() {
    m_Api->UndoCellValue("A1", "=CONCAT(\"Hello\", \"World\")");
    tVariant wVariant = m_Api->CellValue("A1");
    tString wResult="HelloWorld";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    m_Api->UndoCellValue("A1", "=LEFT(\"Hello\", 3)");
    wVariant = m_Api->CellValue("A1");
    wResult="Hel";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    m_Api->UndoCellValue("A1", "=RIGHT(\"Hello\", 3)");
    wVariant = m_Api->CellValue("A1");
    wResult="llo";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    m_Api->UndoCellValue("A1", "=MID(\"Hello\", 2, 3)");
    wVariant = m_Api->CellValue("A1");
    wResult="ell";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);
    // Test Overfow
    m_Api->UndoCellValue("A1", "=MID(\"Hello!\", 2, 30)");
    wVariant = m_Api->CellValue("A1");
    //cout << endl << wVariant << endl;
    wResult="ello!";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    // Test LEN function - Basic cases
    m_Api->UndoCellValue("A1", "=LEN(\"Hello!\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 6);
    
    // Test LEN with empty string
    m_Api->UndoCellValue("A1", "=LEN(\"\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 0);
    
    // Test LEN with single character
    m_Api->UndoCellValue("A1", "=LEN(\"A\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 1);
    
    // Test LEN with spaces
    m_Api->UndoCellValue("A1", "=LEN(\"   \")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3);
    
    // Test LEN with text containing spaces
    m_Api->UndoCellValue("A1", "=LEN(\"Hello World\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 11);
    
    // Test LEN with special characters
    m_Api->UndoCellValue("A1", "=LEN(\"Hello@World#123\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 15);
    
    // Test LEN with accented characters
    m_Api->UndoCellValue("A1", "=LEN(\"Café résumé\")");
    wVariant = m_Api->CellValue("A1");
   
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 11);
    
    // Test LEN with cell reference containing text
    m_Api->UndoCellValue("B1", "Test String");
    m_Api->UndoCellValue("A1", "=LEN(B1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 11);
    
    // Test LEN with long string
    m_Api->UndoCellValue("A1", "=LEN(\"This is a very long string to test the LEN function with multiple words\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 71);
    
    // Test LEN error cases - number instead of string
    m_Api->UndoCellValue("A1", "=LEN(123)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(),3);
    
    // Test LEN error cases - no arguments
    m_Api->UndoCellValue("A1", "=LEN()");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_arg);
    
    // Test LEN ON NAMED
    m_Api->UndoCellValue("A1", "=LEN(ERRORNAME)");
    wVariant = m_Api->CellValue("A1");
    //cout << wVariant << endl;
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_arg);
    

    m_Api->UndoCellValue("A1", "=FIND(\"World\", \"Hello World\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 7);

    m_Api->UndoCellValue("A1", "=EXACT(\"Abc\", \"Abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=EXACT(\"Abc\", \"abc\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=EXACT(1, \"1\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    
    m_Api->UndoCellValue("A1", "=SEARCH(\"World\", \"Hello world\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 7); 

    
    m_Api->UndoCellValue("A1", "=REPLACE(\"Hello World\", 7, 5, \"Everyone\")");
    wVariant = m_Api->CellValue("A1");
    wResult="Hello Everyone";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);
    
    m_Api->UndoCellValue("A1", "=SUBSTITUTE(\"Hello World\", \"World\", \"Everyone\")");
    wVariant = m_Api->CellValue("A1");
    wResult="Hello Everyone";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    // REGEX* (ECMAScript engine)
    m_Api->UndoCellValue("A1", "=REGEXTEST(\"abc123\", \"[0-9]+\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), true);
    m_Api->UndoCellValue("A1", "=REGEXTEST(\"abc\", \"[0-9]+\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.Bool(), false);
    m_Api->UndoCellValue("A1", "=REGEXREPLACE(\"a1b2\", \"[0-9]\", \"X\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("aXbX"));
    m_Api->UndoCellValue("A1", "=REGEXEXTRACT(\"id=42;ok\", \"[0-9]+\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("42"));
    
    m_Api->UndoCellValue("A1", "=UPPER(\"hello\")");
    wVariant = m_Api->CellValue("A1");
    wResult="HELLO";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);
    
    
    m_Api->UndoCellValue("A1", "=LOWER(\"HELLO\")");
    wVariant = m_Api->CellValue("A1");
    wResult="hello";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    m_Api->UndoCellValue("A1", "=PROPER(\"hello stéphane\")");
    wVariant = m_Api->CellValue("A1");
    wResult="Hello Stéphane";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);
    
    
    m_Api->UndoCellValue("A1", "=TRIM(\"  Hello World  \")");
    wVariant = m_Api->CellValue("A1");
    wResult="Hello World";
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), wResult);

    // CLEAN / CODE / CHAR / UNICODE / UNICHAR / NUMBERVALUE
    CPPUNIT_ASSERT_MESSAGE("compile CLEAN", m_Api->UndoCellValue("A1", "=CLEAN(\"A\"&CHAR(9)&\"B\"&CHAR(10)&\"C\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("ABC"), wVariant.String());

    CPPUNIT_ASSERT_MESSAGE("compile CHAR", m_Api->UndoCellValue("A1", "=CHAR(65)"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("A"), wVariant.String());
    CPPUNIT_ASSERT_MESSAGE("compile CODE", m_Api->UndoCellValue("A1", "=CODE(\"A\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(65, wVariant.Int());
    CPPUNIT_ASSERT_MESSAGE("compile CODE CHAR roundtrip", m_Api->UndoCellValue("A1", "=CODE(CHAR(10))"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(10, wVariant.Int());

    CPPUNIT_ASSERT_MESSAGE("compile UNICHAR", m_Api->UndoCellValue("A1", "=UNICHAR(66)"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("B"), wVariant.String());
    CPPUNIT_ASSERT_MESSAGE("compile UNICODE", m_Api->UndoCellValue("A1", "=UNICODE(\"B\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(66, wVariant.Int());
    CPPUNIT_ASSERT_MESSAGE("compile UNICODE accent", m_Api->UndoCellValue("A1", "=UNICODE(\"é\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(233, wVariant.Int());
    CPPUNIT_ASSERT_MESSAGE("compile UNICHAR 0", m_Api->UndoCellValue("A1", "=UNICHAR(0)"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_value);

    CPPUNIT_ASSERT_MESSAGE("compile NUMBERVALUE", m_Api->UndoCellValue("A1", "=NUMBERVALUE(\"1.234,56\", \",\", \".\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1234.56, wVariant.Double(), 1e-9);
    CPPUNIT_ASSERT_MESSAGE("compile NUMBERVALUE pct", m_Api->UndoCellValue("A1", "=NUMBERVALUE(\"3.5%\")"));
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.035, wVariant.Double(), 1e-12);
    
    // T — text unchanged; non-text → ""
    m_Api->UndoCellValue("A1", "=T(\"hello\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("hello"), wVariant.String());
    m_Api->UndoCellValue("A1", "=T(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsString());
    CPPUNIT_ASSERT_EQUAL(tString(""), wVariant.String());

    // FIXED / DOLLAR — use US locale for stable separators.
    tApplication::Instance()->Locale("us");
    m_Api->UndoCellValue("A1", "=FIXED(1234.567,2,TRUE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("1234.57"), wVariant.String());
    m_Api->UndoCellValue("A1", "=FIXED(1234.567,2,FALSE)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("1,234.57"), wVariant.String());
    m_Api->UndoCellValue("A1", "=DOLLAR(1234.5)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("$1,234.50"), wVariant.String());
    m_Api->UndoCellValue("A1", "=DOLLAR(-1.2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("($1.20)"), wVariant.String());

    // VALUETOTEXT / ARRAYTOTEXT
    m_Api->UndoCellValue("A1", "=VALUETOTEXT(42)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("42"), wVariant.String());
    m_Api->UndoCellValue("A1", "=VALUETOTEXT(\"hi\",1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("\"hi\""), wVariant.String());
    m_Api->UndoCellValue("B1", 1);
    m_Api->UndoCellValue("C1", 2);
    m_Api->UndoCellValue("A1", "=ARRAYTOTEXT(B1:C1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("1, 2"), wVariant.String());
    m_Api->UndoCellValue("A1", "=ARRAYTOTEXT(B1:C1,1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(tString("{1,2}"), wVariant.String());

    // Test TEXT function
    m_Api->UndoCellValue("A1", "=TEXT(123.456,\"0.00\")");
    wVariant = m_Api->CellValue("A1");
    // TEXT converts number to string (format is simplified for now)
    CPPUNIT_ASSERT(wVariant.Type() == tVariantType::t_string);
    
    m_Api->UndoCellValue("A2", "=TEXT(100,\"#,##0\")");
    wVariant = m_Api->CellValue("A2");
    CPPUNIT_ASSERT(wVariant.Type() == tVariantType::t_string);
    
    // Test French locale
    tApplication::Instance()->Locale("fr");
    m_Api->UndoCellValue("B2", "=TEXT(46021;\"ddd\")");
    wVariant = m_Api->CellValue("B2");
    CPPUNIT_ASSERT(wVariant.Type() == tVariantType::t_string);
     //cout << m_Api->Cell("B2")->StrRef() << "=" << m_Api->Cell("B2")->FormulaStr() << ":" << m_Api->Cell("B2")->Value() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("Mar."));
    tApplication::Instance()->Locale("us");
    m_Api->UndoCellValue("B2", "=TEXT(46021,\"ddd\")");
    wVariant = m_Api->CellValue("B2");
    CPPUNIT_ASSERT(wVariant.Type() == tVariantType::t_string);
    //cout << m_Api->Cell("B2")->StrRef() << "=" << m_Api->Cell("B2")->FormulaStr() << ":" << m_Api->Cell("B2")->Value() << endl;
    CPPUNIT_ASSERT_EQUAL(wVariant.String(),tString("Tue"));

    // TEXT(D9;"jjj") when D9 is a spill origin / E9 a spill extend (Calendar weekday header).
    tApplication::Instance()->Locale("fr");
    m_Api->UndoCellValue("D8", "=DATE(2026;1;1)");
    m_Api->UndoCellValue("D9", "=D8+{0,1,2}");
    m_Api->UndoCellValue("A20", "=TEXT(D9;\"jjj\")");
    wVariant = m_Api->CellValue("A20");
    CPPUNIT_ASSERT_MESSAGE("TEXT(D9) spill origin must be text, not #ARG", wVariant.Type() == tVariantType::t_string);
    CPPUNIT_ASSERT_EQUAL(tString("Jeu."), wVariant.String());
    m_Api->UndoCellValue("A21", "=TEXT(E9;\"jjj\")");
    wVariant = m_Api->CellValue("A21");
    CPPUNIT_ASSERT_MESSAGE("TEXT(E9) spill extend must be text, not #ARG", wVariant.Type() == tVariantType::t_string);
    CPPUNIT_ASSERT_EQUAL(tString("Ven."), wVariant.String());
    m_Api->UndoCellValue("A22", "=TEXT(Z99;\"jjj\")");
    wVariant = m_Api->CellValue("A22");
    CPPUNIT_ASSERT_MESSAGE("TEXT(blank) is empty text", wVariant.Type() == tVariantType::t_string);
    CPPUNIT_ASSERT_EQUAL(tString(""), wVariant.String());
    tApplication::Instance()->Locale("us");
    // Test VALUE function
    m_Api->UndoCellValue("A3", "=VALUE(\"123\")");
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 123);
    
    m_Api->UndoCellValue("A4", "=VALUE(\"123.45\")");
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 123.45, 0.0001);
    
    m_Api->UndoCellValue("A5", "=VALUE(\"  -42  \")");
    wVariant = m_Api->CellValue("A5");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), -42);
    
    m_Api->UndoCellValue("A6", "=VALUE(123)");
    wVariant = m_Api->CellValue("A6");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 123); // Already a number, returns as-is

    // LEFT / RIGHT with optional num_chars (defaults to 1)
    m_Api->UndoCellValue("A1", "=LEFT(\"Hello\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("H"));

    m_Api->UndoCellValue("A1", "=RIGHT(\"Hello\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("o"));

    // TEXTAFTER
    m_Api->UndoCellValue("A1", "=TEXTAFTER(\"Hello World\", \" \")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("World"));

    // TEXTAFTER on a number coerced to text (decimals part)
    m_Api->UndoCellValue("A1", "=TEXTAFTER(3.14, \".\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("14"));

    // TEXTAFTER instance_num (2nd occurrence)
    m_Api->UndoCellValue("A1", "=TEXTAFTER(\"a-b-c\", \"-\", 2)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("c"));

    // TEXTAFTER not found -> #N/A
    m_Api->UndoCellValue("A1", "=TEXTAFTER(\"abc\", \",\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT(wVariant.IsError());
    CPPUNIT_ASSERT_EQUAL(wVariant.Error().Code(), tTypeError::t_na);

    // TEXTBEFORE
    m_Api->UndoCellValue("A1", "=TEXTBEFORE(\"Hello World\", \" \")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("Hello"));

    // TEXTBEFORE with negative instance (last delimiter)
    m_Api->UndoCellValue("A1", "=TEXTBEFORE(\"a.b.c\", \".\", -1)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("a.b"));

    // TEXTJOIN with literals, ignore_empty = TRUE
    m_Api->UndoCellValue("A1", "=TEXTJOIN(\"-\", TRUE, \"a\", \"\", \"b\", \"c\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("a-b-c"));

    // TEXTJOIN with ignore_empty = FALSE keeps blanks
    m_Api->UndoCellValue("A1", "=TEXTJOIN(\"-\", FALSE, \"a\", \"\", \"b\")");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("a--b"));

    // TEXTJOIN over a range
    m_Api->UndoCellValue("C1", "x");
    m_Api->UndoCellValue("C2", "y");
    m_Api->UndoCellValue("C3", "z");
    m_Api->UndoCellValue("A1", "=TEXTJOIN(\",\", TRUE, C1:C3)");
    wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("x,y,z"));

    // TEXTSPLIT spills a row
    m_Api->UndoCellValue("A10", "=TEXTSPLIT(\"a,b,c\", \",\")");
    wVariant = m_Api->CellValue("A10");
    CPPUNIT_ASSERT_EQUAL(wVariant.String(), tString("a"));
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("B10").String(), tString("b"));
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("C10").String(), tString("c"));

    // TEXTSPLIT with row + column delimiters
    m_Api->UndoCellValue("A12", "=TEXTSPLIT(\"a,b;c,d\", \",\", \";\")");
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("A12").String(), tString("a"));
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("B12").String(), tString("b"));
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("A13").String(), tString("c"));
    CPPUNIT_ASSERT_EQUAL(m_Api->CellValue("B13").String(), tString("d"));
}

void  TestSkFunction::TestVerifiyFormulaColRow(tIndex sBeginRow,tIndex sBeginCol,tIndex sSizeRow,tIndex sSizeCol,tBool sTestCellNull) {
    for(tIndex wRow =sBeginRow ; wRow<=sBeginRow+sSizeRow ; wRow++) {
        for(tIndex wCol = sBeginCol; wCol<=sBeginCol+sSizeCol; wCol++) {
            tStringStream wCellAdr;
            wCellAdr << Base10ToAlpha(wCol) << wRow;
            tCell* wCell=m_Api->Cell(wCellAdr.str());
            if (wCell!=nullptr) {
                tVariant wVariant=wCell->Value();
                CPPUNIT_ASSERT_MESSAGE("TestFunction ColRow "+wCellAdr.str(), wVariant=wRow*100+wCol);
#ifdef printdebug
                cout << wCellAdr.str() << "=" << wVariant << ".";
#endif
            } else {
                if (sTestCellNull) {
                    CPPUNIT_ASSERT_MESSAGE("TestFunction ColRow "+wCellAdr.str()+" nullptr",wCell!=nullptr);
                } else {
#ifdef printdebug
                    cout << wCellAdr.str() << "=null.";
#endif
                }
            }
            
        }
#ifdef printdebug
        cout << endl;
#endif
    }
}

void TestSkFunction::TestFunctionRowCol() {
    try {
        tIndex wBeginRow=1;
        tIndex wBeginCol=1;
        tIndex wSizeRow=10;
        tIndex wSizeCol=10;
        for(tIndex wRow =wBeginRow ; wRow<=wBeginRow+wSizeRow ; wRow++) {
            for(tIndex wCol = wBeginCol; wCol<=wBeginCol+wSizeCol; wCol++) {
                tStringStream wCellAdr;
                wCellAdr << Base10ToAlpha(wCol) << wRow;
                tStringStream wFormula;
                wFormula << "=ROW()*100+COLUMN()";
                tBool wOk=m_Api->UndoCellValue(wCellAdr.str(), wFormula.str());
                if (!wOk) {
                    cout << m_Api->ErrorWithDetail() << endl;
                }
            }
        }
        
        TestVerifiyFormulaColRow(wBeginRow, wBeginCol, wSizeRow, wSizeRow);
        
        m_Api->UndoInsertRow(1, 3);
        m_Api->UndoInsertCol(1,2);
        wBeginRow=4;
        wBeginCol=3;
        
        TestVerifiyFormulaColRow(wBeginRow,wBeginCol, wSizeRow, wSizeRow);
        
        m_Api->UndoDeleteRow(5,2);
        TestVerifiyFormulaColRow(wBeginRow,wBeginCol, wSizeRow, wSizeRow,false);
        
        m_Api->UndoDeleteRow(6,2);
        TestVerifiyFormulaColRow(wBeginRow,wBeginCol, wSizeRow, wSizeRow,false);
        
        m_Api->Undo();
        TestVerifiyFormulaColRow(wBeginRow,wBeginCol, wSizeRow, wSizeRow,false);
        
        m_Api->Undo();
        TestVerifiyFormulaColRow(wBeginRow,wBeginCol, wSizeRow, wSizeRow);
        
        m_Api->Undo();
        TestVerifiyFormulaColRow(wBeginCol,wBeginRow, wSizeRow, wSizeRow,false);
     
        m_Api->Undo();
        wBeginRow=1;
        wBeginCol=1;
        TestVerifiyFormulaColRow(wBeginCol,wBeginRow, wSizeRow, wSizeRow);
        
        
    #if defined(_DEBUG) && defined(printdebug)
        tSheet* wSheet=m_Api->ActiveSheet();
        cout << wSheet->Debug() << endl;
    #endif
    }
    catch (SkRoot::tExceptionInternalError& e) {
        // Catch reference exceptions (now that source code uses throw(T(...)) instead of throw(new T(...)))
        std::cerr << std::endl << "EXCEPTION (tExceptionInternalError): " << e.what() << std::endl;
        throw; // Re-throw to let CPPUNIT handle it
    }
    catch (const std::exception& e) {
        // Catch all standard exceptions
        std::cerr << std::endl << "EXCEPTION (std::exception): " << e.what() << std::endl;
        throw; // Re-throw to let CPPUNIT handle it
    }
    catch (const std::string& e) {
        // Catch string exceptions (from SkCalculationPath.cpp)
        std::cerr << std::endl << "EXCEPTION (std::string): " << e << std::endl;
        throw; // Re-throw to let CPPUNIT handle it
    }
    catch (...) {
        // Catch all other exceptions - should not happen now, but keep for safety
        std::cerr << std::endl << "EXCEPTION (unknown type)" << std::endl;
        std::exception_ptr eptr = std::current_exception();
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (SkRoot::tExceptionInternalError& e) {
                std::cerr << "EXCEPTION MESSAGE (via rethrow - tExceptionInternalError): " << e.what() << std::endl;
                throw;
            }
            catch (const std::exception& e) {
                std::cerr << "EXCEPTION MESSAGE (via rethrow - std::exception): " << e.what() << std::endl;
                throw;
            }
            catch (...) {
                std::cerr << "Cannot extract exception message - type is truly unknown" << std::endl;
            }
        }
        throw; // Re-throw to let CPPUNIT handle it
    }
}

void TestSkFunction::TestFunctionVolatile() {
    m_Api->UndoCellValue("A1",1);
    m_Api->UndoCellValue("A3","=ROW()+A1");
    
    tCell* wCellA3=m_Api->Cell("A3");
    //cout << endl << "A3=" << wCellA3->Value() << endl;
    CPPUNIT_ASSERT_EQUAL(wCellA3->Value().Int(), 4);
    m_Api->UndoCellValue("A1",2);
    //cout << "A3=" << wCellA3->Value() << endl;
    CPPUNIT_ASSERT_EQUAL(wCellA3->Value().Int(), 5);
}

void TestSkFunction::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();

    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_Application->Locale()->Lang("us");
}

void TestSkFunction::TestFunctionSubtotal() {
    // Test SUBTOTAL with SUM (function_num = 9)
    m_Api->UndoCellValue("E1", 10);
    m_Api->UndoCellValue("E2", 20);
    m_Api->UndoCellValue("E3", 30);
    m_Api->UndoCellValue("E4", "=SUBTOTAL(9,E1:E3)");
    tVariant wVariant = m_Api->CellValue("E4");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 60.0); // SUM of 10+20+30 = 60
    
    // Test SUBTOTAL with AVERAGE (function_num = 1)
    m_Api->UndoCellValue("E5", "=SUBTOTAL(1,E1:E3)");
    wVariant = m_Api->CellValue("E5");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 20.0, 0.0001); // AVERAGE of 10+20+30 = 20
    
    // Test SUBTOTAL with COUNT (function_num = 2)
    m_Api->UndoCellValue("E6", "=SUBTOTAL(2,E1:E3)");
    wVariant = m_Api->CellValue("E6");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // COUNT of 3 numeric values
    
    // Test SUBTOTAL with COUNTA (function_num = 3)
    m_Api->UndoCellValue("F1", "Text1");
    m_Api->UndoCellValue("F2", 5);
    m_Api->UndoCellValue("F3", "");
    m_Api->UndoCellValue("E7", "=SUBTOTAL(3,F1:F3)");
    wVariant = m_Api->CellValue("E7");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // COUNTA counts all non-empty cells (including empty string)
    
    // Test SUBTOTAL with MAX (function_num = 4)
    m_Api->UndoCellValue("E8", "=SUBTOTAL(4,E1:E3)");
    wVariant = m_Api->CellValue("E8");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 30.0); // MAX of 10,20,30 = 30
    
    // Test SUBTOTAL with MIN (function_num = 5)
    m_Api->UndoCellValue("E9", "=SUBTOTAL(5,E1:E3)");
    wVariant = m_Api->CellValue("E9");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 10.0); // MIN of 10,20,30 = 10
    
    // Test SUBTOTAL ignoring nested SUBTOTAL formulas
    m_Api->UndoCellValue("G1", 5);
    m_Api->UndoCellValue("G2", 15);
    m_Api->UndoCellValue("G3", "=SUBTOTAL(9,G1:G2)"); // Nested SUBTOTAL
    m_Api->UndoCellValue("G4", 25);
    m_Api->UndoCellValue("E10", "=SUBTOTAL(9,G1:G4)");
    wVariant = m_Api->CellValue("E10");
    // Should sum G1, G2, G4 = 5+15+25 = 45 (G3 with SUBTOTAL is ignored)
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 45.0, 0.0001);
    
    // Test SUBTOTAL with multiple ranges
    m_Api->UndoCellValue("H1", 1);
    m_Api->UndoCellValue("H2", 2);
    m_Api->UndoCellValue("I1", 3);
    m_Api->UndoCellValue("I2", 4);
    m_Api->UndoCellValue("E11", "=SUBTOTAL(9,H1:H2,I1:I2)");
    wVariant = m_Api->CellValue("E11");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 10.0); // SUM of 1+2+3+4 = 10
    
    // Test SUBTOTAL with function_num 109 (SUM ignoring hidden rows) — all visible
    m_Api->UndoCellValue("J1", 100);
    m_Api->UndoCellValue("J2", 200);
    m_Api->UndoCellValue("J3", 300);
    m_Api->UndoCellValue("E12", "=SUBTOTAL(109,J1:J3)");
    wVariant = m_Api->CellValue("E12");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 600.0); // SUM of 100+200+300 = 600

    // SUBTOTAL(109) excludes manually hidden rows; SUBTOTAL(9) still includes them (Excel)
    m_Api->UndoSizeRow(2, 2, 0.0);
    m_Api->UndoCellValue("E12", "=SUBTOTAL(109,J1:J3)");
    wVariant = m_Api->CellValue("E12");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 400.0); // 100+300, row 2 hidden
    m_Api->UndoCellValue("E12", "=SUBTOTAL(9,J1:J3)");
    wVariant = m_Api->CellValue("E12");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 600.0); // 9 includes manually hidden rows
    m_Api->UndoSizeRow(2, 2, -1.0); // restore default row height

    // Both SUBTOTAL(9) and SUBTOTAL(109) exclude filter-hidden rows (DataVisible)
    tColRowCellRange* wSheet = m_Api->ActiveSheet()->ColRowCellRange();
    m_Api->UndoCellValue("M1", 10);
    m_Api->UndoCellValue("M2", 20);
    m_Api->UndoCellValue("M3", 30);
    wSheet->EnsureRow(2)->DataVisible(false);
    m_Api->UndoCellValue("E12", "=SUBTOTAL(109,M1:M3)");
    wVariant = m_Api->CellValue("E12");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 40.0); // 10+30
    m_Api->UndoCellValue("E12", "=SUBTOTAL(9,M1:M3)");
    wVariant = m_Api->CellValue("E12");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 40.0); // filter-hidden rows excluded for 9 too
    wSheet->EnsureRow(2)->DataVisible(true);
    
    // Test SUBTOTAL with COUNT (function_num = 102)
    m_Api->UndoCellValue("E13", "=SUBTOTAL(102,E1:E3)");
    wVariant = m_Api->CellValue("E13");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // COUNT of 3 numeric values
    
    // Test SUBTOTAL with COUNTA (function_num = 103)
    m_Api->UndoCellValue("E14", "=SUBTOTAL(103,F1:F3)");
    wVariant = m_Api->CellValue("E14");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // COUNTA counts all non-empty cells
    
    // Test SUBTOTAL with empty range
    m_Api->UndoCellValue("E15", "=SUBTOTAL(9,K1:K10)");
    wVariant = m_Api->CellValue("E15");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 0.0); // SUM of empty range = 0
    
    // Test SUBTOTAL with mixed values (numbers and text)
    m_Api->UndoCellValue("L1", 10);
    m_Api->UndoCellValue("L2", "Text");
    m_Api->UndoCellValue("L3", 20);
    m_Api->UndoCellValue("E16", "=SUBTOTAL(9,L1:L3)");
    wVariant = m_Api->CellValue("E16");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 30.0); // SUM ignores text, only sums 10+20 = 30
    
    // Test SUBTOTAL COUNT with mixed values
    m_Api->UndoCellValue("E17", "=SUBTOTAL(2,L1:L3)");
    wVariant = m_Api->CellValue("E17");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 2); // COUNT only counts numeric values (10, 20) = 2
    
    // Test SUBTOTAL COUNTA with mixed values
    m_Api->UndoCellValue("E18", "=SUBTOTAL(3,L1:L3)");
    wVariant = m_Api->CellValue("E18");
    CPPUNIT_ASSERT_EQUAL(wVariant.Int(), 3); // COUNTA counts all non-empty cells = 3
}

void TestSkFunction::TestFunctionAggregate() {
    tVariant wVariant;

    // AGGREGATE SUM (9), option 4 = ignore nothing
    m_Api->UndoCellValue("P1", 10);
    m_Api->UndoCellValue("P2", 20);
    m_Api->UndoCellValue("P3", 30);
    m_Api->UndoCellValue("E20", "=AGGREGATE(9,4,P1:P3)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 60.0);

    // Option 5: ignore hidden rows
    m_Api->UndoSizeRow(2, 2, 0.0);
    m_Api->UndoCellValue("E20", "=AGGREGATE(9,5,P1:P3)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 40.0); // 10+30
    m_Api->UndoSizeRow(2, 2, -1.0);

    // Option 6: ignore errors
    m_Api->UndoCellValue("Q1", 100);
    m_Api->UndoCellValue("Q2", "=1/0");
    m_Api->UndoCellValue("Q3", 300);
    m_Api->UndoCellValue("E20", "=AGGREGATE(9,6,Q1:Q3)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 400.0); // 100+300

    // Option 0: ignore nested AGGREGATE
    m_Api->UndoCellValue("R1", 5);
    m_Api->UndoCellValue("R2", 15);
    m_Api->UndoCellValue("R3", "=AGGREGATE(9,4,R1:R2)");
    m_Api->UndoCellValue("R4", 25);
    m_Api->UndoCellValue("E20", "=AGGREGATE(9,0,R1:R4)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 45.0, 0.0001); // 5+15+25

    // MEDIAN (12)
    m_Api->UndoCellValue("S1", 1);
    m_Api->UndoCellValue("S2", 3);
    m_Api->UndoCellValue("S3", 9);
    m_Api->UndoCellValue("E20", "=AGGREGATE(12,4,S1:S3)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 3.0);

    // LARGE (14, k=2)
    m_Api->UndoCellValue("E20", "=AGGREGATE(14,4,S1:S3,2)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 3.0);

    // Filter-hidden row with option 5
    tColRowCellRange* wSheet = m_Api->ActiveSheet()->ColRowCellRange();
    m_Api->UndoCellValue("T1", 10);
    m_Api->UndoCellValue("T2", 20);
    m_Api->UndoCellValue("T3", 30);
    wSheet->EnsureRow(2)->DataVisible(false);
    m_Api->UndoCellValue("E20", "=AGGREGATE(9,5,T1:T3)");
    wVariant = m_Api->CellValue("E20");
    CPPUNIT_ASSERT_EQUAL(wVariant.Double(), 40.0);
    wSheet->EnsureRow(2)->DataVisible(true);
}

void TestSkFunction::TestRangeRefTransform() {
    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "A1"), tString("Sheet1!A1"));
    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "A1:B10"), tString("Sheet1!A1:B10"));
    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "Autre!B1"), tString("Autre!B1"));
    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "Sheet1!C2"), tString("Sheet1!C2"));

    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", "Sheet1!A1:B10"), tString("A1:B10"));
    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", "Sheet1!C2"), tString("C2"));
    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", "Autre!B1"), tString("Autre!B1"));
    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", "A1"), tString("A1"));

    const tString wFormula = "=DATARANGE(A1:A3,B2,C1)";
    const tString wQualified = QualifyRefsForSheet("Sheet1", wFormula);
    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "A1:A3"), tString("Sheet1!A1:A3"));
    CPPUNIT_ASSERT_EQUAL(wQualified, tString("=DATARANGE(Sheet1!A1:A3,Sheet1!B2,Sheet1!C1)"));
    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", wQualified), wFormula);

    CPPUNIT_ASSERT_EQUAL(QualifyRefsForSheet("Sheet1", "MyRange"), tString("MyRange"));
    CPPUNIT_ASSERT_EQUAL(StripTargetSheetFromRefs("Sheet1", "Sheet1!MyRange"), tString("Sheet1!MyRange"));
}

namespace {

    tSize CollectRefCount(const tString& sJson) {
        rapidjson::Document wDoc;
        wDoc.Parse(sJson.c_str());
        if (!wDoc.IsObject() || !wDoc.HasMember("refs") || !wDoc["refs"].IsArray()) {
            return 0;
        }
        return wDoc["refs"].Size();
    }

    tBool CollectSyntaxError(const tString& sJson) {
        rapidjson::Document wDoc;
        wDoc.Parse(sJson.c_str());
        return wDoc.IsObject() && wDoc.HasMember("syntaxError") && wDoc["syntaxError"].GetBool();
    }

    tBool CollectHasResolvedRef(const tString& sJson, const tString& sText, tIndex sTop, tIndex sLeft,
                                tIndex sBottom, tIndex sRight) {
        rapidjson::Document wDoc;
        wDoc.Parse(sJson.c_str());
        if (!wDoc.IsObject() || !wDoc.HasMember("refs") || !wDoc["refs"].IsArray()) {
            return false;
        }
        for (rapidjson::SizeType wI = 0; wI < wDoc["refs"].Size(); ++wI) {
            const rapidjson::Value& wRef = wDoc["refs"][wI];
            if (!wRef.IsObject() || !wRef.HasMember("resolved") || !wRef["resolved"].GetBool()) {
                continue;
            }
            if (wRef.HasMember("text") && wRef["text"].GetString() == sText && wRef.HasMember("top")
                && static_cast<tIndex>(wRef["top"].GetInt()) == sTop
                && static_cast<tIndex>(wRef["left"].GetInt()) == sLeft
                && static_cast<tIndex>(wRef["bottom"].GetInt()) == sBottom
                && static_cast<tIndex>(wRef["right"].GetInt()) == sRight) {
                return true;
            }
        }
        return false;
    }

} // namespace

void TestSkFunction::TestCollectFormulaRefs() {
    tWorkBook* const wWorkBook = m_Api->ActiveWorkBook();
    CPPUNIT_ASSERT(wWorkBook != nullptr);

    const tString wCellJson =
        CollectFormulaRefsJson(wWorkBook, "Sheet1", 5, 2, "=SUM(A1:B2,C3)");
    CPPUNIT_ASSERT_EQUAL(static_cast<tSize>(2), CollectRefCount(wCellJson));
    CPPUNIT_ASSERT(CollectHasResolvedRef(wCellJson, "A1:B2", 1, 1, 2, 2));
    CPPUNIT_ASSERT(CollectHasResolvedRef(wCellJson, "C3", 3, 3, 3, 3));
    CPPUNIT_ASSERT(!CollectSyntaxError(wCellJson));

    CPPUNIT_ASSERT(m_Api->UndoAddRangeNamed("MY_REFS", "D4:E6"));
    const tString wNamedJson = CollectFormulaRefsJson(wWorkBook, "Sheet1", 1, 1, "=MY_REFS+1");
    CPPUNIT_ASSERT(CollectHasResolvedRef(wNamedJson, "MY_REFS", 4, 4, 6, 5));

    const tString wPartialJson = CollectFormulaRefsJson(wWorkBook, "Sheet1", 1, 1, "=SUM(A1:B2");
    CPPUNIT_ASSERT(!CollectSyntaxError(wPartialJson));
    CPPUNIT_ASSERT(CollectHasResolvedRef(wPartialJson, "A1:B2", 1, 1, 2, 2));

    const tString wErrorJson = CollectFormulaRefsJson(wWorkBook, "Sheet1", 1, 1, "=A1+#REF!");
    CPPUNIT_ASSERT_EQUAL(static_cast<tSize>(2), CollectRefCount(wErrorJson));
    CPPUNIT_ASSERT(CollectHasResolvedRef(wErrorJson, "A1", 1, 1, 1, 1));
}

void TestSkFunction::tearDown() {
    m_Application->Locale()->Lang("fr");
    delete(m_Api);
}
