//==============================================================================
// TestSkTypesClass
// Test library SkTypesClass
//==============================================================================

#include "../include/TestSkTypesClass.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

#define _debugdate

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestTypesClass);

#define _debugdate
// We can send it to the API of a feature
TestTypesClass::TestTypesClass() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};


void TestTypesClass::TestClassString() {
	tClassString wTestString;

	tString wTitle = "TesttClassString";
	tString wSubTitle = "IsNumber";
	wTestString = "1234";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsNumber());
	wTestString = "1234.34";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsNumber());
	wTestString = "8.8999999999999996E-2";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsNumber());
	wTestString = "1234.34A";
    
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsNumber());

    wTestString = "-";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " - " + wSubTitle, !wTestString.IsNumber());

	wSubTitle = "IsUnsigned";
	wTestString = "1234"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsUnsigned());
	wTestString = "-1234"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsUnsigned());

	wSubTitle = "IsInteger";
	wTestString = "1234"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsInteger());
	wTestString = "-1234"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsInteger());
	wTestString = "1234.78"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsInteger());
	wTestString = "-1234A"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsInteger());

	wSubTitle = "IsKeyCode";
	wTestString = "_--"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsKeyCode());
	wTestString = "@ALLEZ"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsKeyCode());
	wTestString = "A1_23_"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsKeyCode());

	wSubTitle = "IsEmailAdress";
	wTestString = "sallez@skeema.fr"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsEmailAdress());
	wTestString = "@allez"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsEmailAdress());
	wTestString = "sallez@skeema?fr"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsEmailAdress());

	wSubTitle = "IsUrl";
	wTestString = "https://news.google.com/"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.IsUrl());
	wTestString = "https:x//news.google.com/"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsUrl());
	wTestString = "sallez@skeema.fr"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.IsUrl());

	wSubTitle = "StartsWidth";
	wTestString = "Allez Stephane"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.StartsWith("Allez"));
	wTestString = " Allez Stephane"; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.StartsWith("Allez"));
	wTestString = ""; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, !wTestString.StartsWith("Allez"));
	wTestString = ""; CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.StartsWith(""));

	wSubTitle = "Trim";
	wTestString = "  abc  ";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestString.Trim() == "abc");

	wSubTitle = "Split";
	wTestString = "a/b/c";
	tVectorString wParts = wTestString.Split("/");
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wParts.size() == 3);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wParts[1] == "b");

	wSubTitle = "Regex number";
	wTestString = "123,45";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle,
	                      wTestString.Regex_Match("^-?\\d+([.,]\\d+)?$"));
	wTestString = "12A";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle,
	                      !wTestString.Regex_Match("^-?\\d+([.,]\\d+)?$"));
}

void convertSecondsToHMS(double seconds, int &hours, int &minutes, int &secs) {
    hours = static_cast<int>(seconds / 3600);
    seconds = static_cast<int>(seconds) % 3600;
    minutes = static_cast<int>(seconds / 60);
    secs = static_cast<int>(seconds) % 60;
}

void TestTypesClass::TestClassDate() {
    // Pin TZ: date formatting tests below expect Europe/Paris.
    std::string wSavedTz;
    if (const char* wPrevTz = std::getenv("TZ")) {
        wSavedTz = wPrevTz;
    }
#if defined(_WIN32)
    // MSVC does not honor IANA names such as Europe/Paris (POSIX TZ). CET-1 is UTC+1,
    // matching Europe/Paris in winter — the serial-sum cases below land in January/December.
    _putenv_s("TZ", "CET-1");
    _tzset();
#else
    setenv("TZ", "Europe/Paris", 1);
    ::tzset();
#endif
#ifdef debugdate
    cout << endl;
#endif
	tString wTitle = "TesttClassDate";

	tString wSubTitle = "IsDate";
    tClassDate wTestDate;
    wTestDate.Set(2023, 01, 02);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Year()==2023);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Month() == 01);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Day() == 02);

	wSubTitle = "Day of Week";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.DayWeek() == 1);
	
	wTestDate=wTestDate+1;
	//cout << wTestDate.UsDate() << endl;

	wSubTitle = "Date++";
	
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Year() == 2023);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Month() == 01);
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.Day() == 03);

	wSubTitle = "Date++ Day of Week";
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.DayWeek() == 2);

	wSubTitle = "Date--";
	wTestDate=wTestDate-1;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.DayWeek() == 1);

	wSubTitle = "Date + 30";
	wTestDate= wTestDate + 30;
    
	//cout << wTestDate.UsDate() << endl;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.UsDate() == "02-01-2023");

	wSubTitle = "Date - 32";
	wTestDate = wTestDate - 32;
	//cout << wTestDate.UsDate() << endl;
	CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wTestDate.UsDate() == "12-31-2022");

	wSubTitle = "Date + time fraction (Excel: calendar + TIME as day fraction)";
	{
		tClassDate wBase(2023, 6, 15);
		// Same as TIME(0,30,0): fractional day only, not a second t_date serial (which would add ~1 day).
		const tDouble wTimeFrac = (30.0 * 60.0) / 86400.0;
		tClassDate wSum = wBase + wTimeFrac;
		tInt wH = 0, wM = 0, wS = 0;
		wSum.HourMinuteSecond(wH, wM, wS);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wSum.Year() == 2023 && wSum.Month() == 6 && wSum.Day() == 15);
		CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wH == 0 && wM == 30 && wS == 0);
	}

    wTestDate.Set(0,0,0);
    
    
    tVariant wVariantDate;
    tApplication::Instance()->Locale("fr");
    
    wSubTitle = "Variant Date Parse";
    wVariantDate.Parse("31/12/2023");
    tClassDate wClassDate(wVariantDate.Date());
    tString wString=wClassDate.FormatDateTime("%d-%m-%Y");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "31-12-2023");

    wSubTitle = "tClassDate from variant t_date round-trip";
    {
        tClassDate wCal(2023, 6, 15);
        // No tVariant(tDate) ctor on __EMSCRIPTEN__32__; SetDate keeps t_date semantics on all targets.
        tVariant wV;
        wV.SetDate(wCal.Value());
        tClassDate wFromV(wV);
        CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wFromV.Value() == wCal.Value());
    }

    wSubTitle = "tClassDate from variant int Excel serial";
    {
        // A serial-derived date must share the EXACT same time_t as the same date built
        // by DATE()/tClassDate(y,m,d): equality (=A1=B1), VLOOKUP/MATCH exact match and
        // date arithmetic compare the raw time_t (local midnight), not the rendered day.
        // Excel serial 45000 = 2023-03-15 (serial 44927 = 2023-01-01, +73 days).
        const tInt wSerial = 45000;
        tClassDate wExpected(2023, 3, 15);
        tVariant wVi(wSerial);
        tClassDate wFromI(wVi);
        CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wFromI.Value() == wExpected.Value());
    }

    wSubTitle = "tClassDate from variant double Excel serial with fraction";
    {
        // Same anchor as DATE() (local midnight), plus the fractional time-of-day on top.
        const tDouble wSerial = 45000.25; // 2023-03-15 06:00 (0.25 day = 6h)
        tClassDate wExpected(2023, 3, 15);
        const tDate wExpectedValue =
            static_cast<tDate>(wExpected.Value() + static_cast<tDate>(std::llround(0.25 * 86400.0)));
        tVariant wVd(wSerial);
        tClassDate wFromD(wVd);
        CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wFromD.Value() == wExpectedValue);
    }

    wSubTitle = "tClassDate from variant string uses default date";
    {
        tVariant wVs("hello");
        tClassDate wFromS(wVs);
        CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wFromS.Year() == 1900 && wFromS.Month() == 1 && wFromS.Day() == 1);
    }
    
    wSubTitle = "Variant Date Parse with hour";
    wVariantDate.Parse("31/12/2023 12:23");
    tClassDate wClassDateHour(wVariantDate.Date());
    wString=wClassDateHour.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "31-12-2023 12:23:00");
    wSubTitle = "Variant Parse hour";
    wVariantDate.Parse("10:13");
    tClassDate wClassHour(wVariantDate.Date());
    wString=wClassHour.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "01-01-1900 10:13:00");
    
    tBool wIsHour=wClassHour.IsHours();
    wSubTitle = "Variant Date Parse is hour";
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wIsHour);
    
    // Calculate Date
    wSubTitle = "Calculate Date  -double 0.5";
    tClassDate wClassDateCalcul=wClassDate-0.5;
    wString=wClassDateCalcul.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "30-12-2023 12:00:00");
    
    wSubTitle = "Calculate Date  +double 0.5";
    wClassDateCalcul=wClassDateCalcul+0.5;
    wString=wClassDateCalcul.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "31-12-2023 00:00:00");
    
    wSubTitle = "Date Null";
    tClassDate wClassDateNull;
    wString=wClassDateNull.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wSubTitle << ":" << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "01-01-1900 00:00:00");
    
    
    
    
    wSubTitle="Calcul Date + 0  + 2 seconde ";
    wVariantDate.Parse("01/01/1970 00:00:01");
    tClassDate wClassDateHourCalcul1(wVariantDate.Date());
    wVariantDate.Parse("01/01/1970 00:10:02");
    tClassDate wClassDateHourCalcul2(wVariantDate.Date());
#ifdef debugdate
    cout << "1, " << wClassDateHourCalcul1.FormatDateTime("%d-%m-%Y %H:%M:%S") << endl;
    cout << "2, " << wClassDateHourCalcul2.FormatDateTime("%d-%m-%Y %H:%M:%S") << endl;
#endif
    wClassDateCalcul=wClassDateHourCalcul1+wClassDateHourCalcul2;
    wString=wClassDateCalcul.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << "1, " << wClassDateHourCalcul1.FormatDateTime("%d-%m-%Y %H:%M:%S") << endl;
    cout << "2, " << wClassDateHourCalcul2.FormatDateTime("%d-%m-%Y %H:%M:%S") << endl;
    cout << wSubTitle << ":" << wString << endl;
#endif
    // Excel-style serial addition (tClassDate + tClassDate): sum of day+fraction serials, not wall-clock merge.
    // Serial→date now anchors at LOCAL midnight (so imported dates == DATE()); with TZ=Europe/Paris,
    // 1970-01-01 00:00:01 + 1970-01-01 00:10:02 → 02-01-2040 22:10:03.
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "02-01-2040 22:10:03");
    
#ifdef debugdate
    cout << "wClassDate =" << wClassDate.FormatDateTime("%d-%m-%Y %H:%M:%S") << endl;
#endif
    
    wSubTitle="Calcul Date + Duration  ";
    wClassDateCalcul=wClassDateHourCalcul1+wClassDate;
    wString=wClassDateCalcul.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wString << endl;
#endif
    // Excel serial sum: 1970-01-01 00:00:01 + 31-12-2023 00:00:00 → 31-12-2093 22:00:01 (TZ=Europe/Paris, local-midnight anchor).
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "31-12-2093 22:00:01");

    wSubTitle="Calcul Date - Duration  ";
    wClassDateCalcul=wClassDateCalcul-wClassDateHourCalcul2;
    wString=wClassDateCalcul.FormatDateTime("%d-%m-%Y %H:%M:%S");
#ifdef debugdate
    cout << wString << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "31-12-2093 21:49:59");
    
    // Test Read Write Json Date US
    tString wPushLang=m_Application->Locale()->Lang();
    if (wPushLang!="us") m_Application->Locale("us");
    // Test Read and Write Json
    wVariantDate.Parse("12-30-2023 11:49:59");
    
    wSubTitle="Test write Variant Date ";
    StringBuffer wStringBuffer;
    Writer<StringBuffer> wWriter(wStringBuffer);
    wWriter.StartObject();
    wVariantDate.Json(&wWriter);
    wWriter.EndObject();
    
    //cout << wStringBuffer.GetString() << endl;
    tString wJsonResult="{\"t\":\"da\",\"v\":\"12-30-2023 11:49:59\"}";
    wString = wStringBuffer.GetString();
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle,wString ==wJsonResult);
    
    Document wDocument;
    wDocument.Parse(wString.c_str());
    tString wResult = wDocument["v"].GetString();
    //cout << wResult << endl;
    
    wSubTitle="Test Read Variant Date ";
    wVariantDate.Parse(wResult);
    wClassDateCalcul.Value(wVariantDate.Date());
    
    wString=wClassDateCalcul.FormatDateTime("%m-%d-%Y %H:%M:%S");
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "12-30-2023 11:49:59");
    
    if (wPushLang!="us") m_Application->Locale(wPushLang);
    
    //cout << wClassDateCalcul.FormatDateTime("Today is %A, %b %d. Time:  %r") << endl;

#if defined(_WIN32)
    if (wSavedTz.empty()) {
        _putenv("TZ=");
    } else {
        _putenv_s("TZ", wSavedTz.c_str());
    }
    _tzset();
#else
    if (wSavedTz.empty()) {
        unsetenv("TZ");
    } else {
        setenv("TZ", wSavedTz.c_str(), 1);
    }
    ::tzset();
#endif
}

void TestTypesClass::TestParseDateTimeLocale() {
    tString wTitle = "TestParseDateTimeLocale";
    tString wSavedLang = m_Application->Locale()->Lang();

    m_Application->Locale("fr");
    {
        tClassDate wDate;
        // Short pattern is %d/%m/%y: %y always adds 2000 (see tClassDate::ParseDateTime).
        tString wFmtShort = m_Application->Locale()->FormatDateShort();
        CPPUNIT_ASSERT_MESSAGE(wTitle + " FR short parse",
                               wDate.ParseDateTime("01/01/24", wFmtShort.c_str()));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " FR short year", 2024, wDate.Year());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " FR short month", 1, wDate.Month());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " FR short day", 1, wDate.Day());

        tString wFmtLong = m_Application->Locale()->FormatDateLong();
        CPPUNIT_ASSERT_MESSAGE(wTitle + " FR long parse",
                               wDate.ParseDateTime("01/01/2024", wFmtLong.c_str()));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " FR long year", 2024, wDate.Year());
    }

    m_Application->Locale("us");
    {
        tClassDate wDate;
        tString wFmtShort = m_Application->Locale()->FormatDateShort();
        CPPUNIT_ASSERT_MESSAGE(wTitle + " US short parse",
                               wDate.ParseDateTime("01-02-24", wFmtShort.c_str()));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " US short month", 1, wDate.Month());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " US short day", 2, wDate.Day());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " US short year", 2024, wDate.Year());

        tString wFmtLong = m_Application->Locale()->FormatDateLong();
        CPPUNIT_ASSERT_MESSAGE(wTitle + " US long parse",
                               wDate.ParseDateTime("01-02-2024", wFmtLong.c_str()));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(wTitle + " US long year", 2024, wDate.Year());
    }

    if (wSavedLang != "us") {
        m_Application->Locale(wSavedLang);
    }
}

void TestTypesClass::TestClassDouble() {
    tApplication::Instance()->Locale("fr");
    tString wTitle = "TesttClassDouble";

    tClassDouble wClassDouble;
    wClassDouble = 123456.78;
    tFormatString  wFormatString;
    wFormatString.FormatType(tFormatStringType::accounting0);
    wFormatString.Decimal(2);
    wFormatString.Money(t_UnitMoney::eur);
    tString wString=wClassDouble.FormatString(&wFormatString);
    tString wSubTitle = "Currency format fr";
    //cout << wString << endl;
    // fr locale: space thousands sep + comma decimal (not '.' like SP/IT).
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "123 456,78");
    
    tApplication::Instance()->Locale("us");
    wSubTitle = "Currency format us";
    wString=wClassDouble.FormatString(&wFormatString);
    //cout << wString << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "123,456.78");

    wSubTitle = "Cacul";
    tClassDouble wClassDouble1(123);
    tClassDouble wClassDouble2(0.5);
    tClassDouble wClassDouble3=wClassDouble1+wClassDouble2;
    //cout << "1: " << wClassDouble1.FormatString(tFormatStringType::accounting0,t_UnitMoney::eur,2) << endl;
    //cout << "3: " << wClassDouble2.FormatString(tFormatStringType::accounting0,t_UnitMoney::eur,2) << endl;
    
    wString=wClassDouble3.FormatString(&wFormatString);
    //cout << wString <<endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wString == "123.50");
}

void TestTypesClass::setUp() {
	m_Application = tApplication::Instance();
};

    void TestTypesClass::tearDown() {
    }



