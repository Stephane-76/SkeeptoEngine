//==============================================================================
// TestSkFormatExcel
//==============================================================================
#include "../include/TestSkFormatExcel.hpp"



// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormatExcel);

// We can send it to the API of a feature
TestSkFormatExcel::TestSkFormatExcel() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {}

void TestSkFormatExcel::TestSkFormatExcelNumber() { 
    tNumberFormatter* wNumberFormatter=m_Application->NumberFormatter();
    // Default app locale is French; these Excel masks are canonical US — align locale when expecting US display.
    m_Application->Locale("us");
    tString wFormatString = "#,##0.00";
    tDouble wNumber = 1234567.89;
    
    tString wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "1[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1,234,567.89", wFormattedNumber == "1,234,567.89");

    wFormatString = "#,##0.0000";
    wNumber = 1234567.8901;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "2[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1,234,567.8901", wFormattedNumber == "1,234,567.8901");

    CPPUNIT_ASSERT_MESSAGE("Four-part numeric format (last=text) should validate", IsValidExcelNumberFormat("0;0;0;@"));
    wFormatString = "0;0;0;@";
    wFormattedNumber = wNumberFormatter->FormatNumber(42.0, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("positive picks first section -> 42", wFormattedNumber == "42");
    wFormattedNumber = wNumberFormatter->FormatNumber(-3.0, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("negative picks second section -> abs value formatted", wFormattedNumber == "3");

    // Fr format (European mask; display separators follow tLocale — space thousands, comma decimal)
    m_Application->Locale("fr");
    wFormatString = "#.##0,00000";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "3[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1 234 567,89012", wFormattedNumber == "1 234 567,89012");
    
    // currency format US
    m_Application->Locale("us");
    wFormatString = "$#,##0.00";
    wNumber = 1234567.89;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "4[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1,234,567.89", wFormattedNumber == "$1,234,567.89");

    wFormatString = "$#,##0.00;(#,##0.00) $";
    wNumber = -1234567.89;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "5[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1,234,567.89", wFormattedNumber == "(1,234,567.89) $");

    m_Application->Locale("fr");
    // currency format Euro
    wFormatString = "#.##0,00 €";
    wNumber = 1234567.89;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "6[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1 234 567,89 €", wFormattedNumber == "1 234 567,89 €");

    wFormatString = "#.##0,00;(#.##0,00) €";
    wNumber = -1234567.89;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "7[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be (1 234 567,89) €", wFormattedNumber == "(1 234 567,89) €");

    // Percent format
    m_Application->Locale("fr");
    wFormatString = "0,00%";
    wNumber = 0.123456789;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "8[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 12,35%", wFormattedNumber == "12,35%");


    // Scientific format
    wFormatString = "0,00E+00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "9[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1.23457E+06", wFormattedNumber == "1,23457E+06");
    
    // Us format
    m_Application->Locale("us");
    wFormatString = "#,##0.00000";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "10[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1,234,567.89012", wFormattedNumber == "1,234,567.89012");
    
    // Percent format
    wFormatString = "0.00%";
    wNumber = 0.123456789;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "11[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 12.35%", wFormattedNumber == "12.35%");


    // Scientific format
    wFormatString = "0.00E+00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "12[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be 1.23457E+06", wFormattedNumber == "1.23457E+06");

    // Yen format
    wFormatString = "¥#,##0.00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "13[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be ¥1,234,567.89", wFormattedNumber == "¥1,234,567.89");

    wFormatString = "[$¥-ja-JP]#,##0.00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "14[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be ¥1,234,567.89", wFormattedNumber == "¥1,234,567.89");

    wFormatString = "£#,##0.00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "15[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be £1,234,567.89", wFormattedNumber == "£1,234,567.89");

    wFormatString = "[$£-en-GB]#,##0.00";
    wNumber = 1234567.89012;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "16[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("Formatted number should be £1,234,567.89", wFormattedNumber == "£1,234,567.89");

    // OOXML euro bracket: numeric mask is normalized to US (#,##0.00); output separators follow tLocale.
    m_Application->Locale("us");
    wFormatString = "[$\xE2\x82\xAC-40C]#,##0.00";
    wNumber = 1234567.89;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("[$€-40C]#,##0.00 under us locale", wFormattedNumber == "\xE2\x82\xAC""1,234,567.89");

    m_Application->Locale("fr");
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("[$€-40C]#,##0.00 under fr locale uses Decimal/Thousand", wFormattedNumber == "\xE2\x82\xAC""1 234 567,89");

    wFormatString = "[$\xE2\x82\xAC-40C]#.##0,00";
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("[$€-40C] European mask normalizes to US then FR display", wFormattedNumber == "\xE2\x82\xAC""1 234 567,89");

    CPPUNIT_ASSERT_MESSAGE(
        "Import helper expands OOXML [$€-LCID] before bracket strip",
        NormalizeExcelIntlCurrencyBracketsInMask("[$\xE2\x82\xAC-40C]#,##0.00") == "\xE2\x82\xAC#,##0.00");

    CPPUNIT_ASSERT_MESSAGE(
        "European thousands/decimals normalize to US structural mask",
        NormalizeExcelNumberFormatToUs("#.##0,00") == "#,##0.00");

    // ==============================================================
    // Additional tests: 4-part sections, color tag neutralization,
    // and simplified accounting-style currency with parentheses
    // ==============================================================

    // 4-part sections: positive;negative;color/explicit;zero;text
    m_Application->Locale("us");

    wFormatString = "#,##0;[Red]-#,##0;0;@";
    // positive
    wNumber = 1234.0;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("4-part positive should be 1,234", wFormattedNumber == "1,234");

    wNumber = -1234.0;
    wFormatString = "#,##0;[Red](#,##0);0;@";
    auto wInfo = wNumberFormatter->FormatNumberWithInfo(wNumber,wFormatString);
    CPPUNIT_ASSERT_MESSAGE("4-part negative should be (1,234)", wInfo.text == "(1,234)");
    CPPUNIT_ASSERT_MESSAGE("4-part negative should have color", wInfo.hasColor);
    CPPUNIT_ASSERT_MESSAGE("4-part negative should have color name Red", wInfo.colorName == "Red");
    // negative (color tag ignored)
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    //cout << "16[" << wFormattedNumber << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("4-part negative should be (1,234)", wFormattedNumber == "(1,234)");
    // zero section
    wNumber = 0.0;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("4-part zero should be 0", wFormattedNumber == "0");

    // Color tag neutralization on decimal format
    wFormatString = "[Red]#,##0.00";
    wNumber = 1234.56;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("Color tag should be ignored in output", wFormattedNumber == "1,234.56");

    // Simplified accounting-style GBP: negative in parentheses
    wFormatString = "[$£-en-GB]#,##0.00;([$£-en-GB]#,##0.00)";
    wNumber = -1234.56;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("Accounting GBP negative should be (£1,234.56)", wFormattedNumber == "(£1,234.56)");

    // Simplified accounting-style JPY: no decimals, negative in parentheses
    wFormatString = "[$¥-ja-JP]#,##0;([$¥-ja-JP]#,##0)";
    wNumber = -1234.4; // rounds to -1,234
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("Accounting JPY negative should be (¥1,234)", wFormattedNumber == "(¥1,234)");

    // Accounting-style EUR with OOXML currency bracket [$€-1] across sections.
    // Regression guard for xlsx import (bilan_previsionnel.xlsx style masks): the euro glyph
    // must survive on the POSITIVE section. The importer previously flattened such masks and
    // re-appended a single trailing "€" that landed only in the zero section, so positive
    // values lost their currency. [$€-1] must expand per section, not once at the tail.
    m_Application->Locale("us");
    wFormatString = "[$\xE2\x82\xAC-1]#,##0.00;([$\xE2\x82\xAC-1]#,##0.00)";
    wNumber = 1234.56;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("Accounting EUR positive should be €1,234.56", wFormattedNumber == "\xE2\x82\xAC""1,234.56");
    wNumber = -1234.56;
    wFormattedNumber = wNumberFormatter->FormatNumber(wNumber, wFormatString);
    CPPUNIT_ASSERT_MESSAGE("Accounting EUR negative should be (€1,234.56)", wFormattedNumber == "(\xE2\x82\xAC""1,234.56)");

    CPPUNIT_ASSERT_MESSAGE(
        "[$€-1] bracket expands to bare € glyph in place (not stripped)",
        NormalizeExcelIntlCurrencyBracketsInMask("[$\xE2\x82\xAC-1]#,##0.00") == "\xE2\x82\xAC#,##0.00");
}

void TestSkFormatExcel::TestSkFormatExcelDate() {
    tDateFormatter* wDateFormatter=m_Application->DateFormatter();
    tString wFormatStr = "dd/mm/yyyy";
    tClassDate wDate(2023,12,10);
    m_Application->Locale("us");
    tString wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10/12/2023", wFormattedDate == "10/12/2023");

    wFormatStr = "mm/dd/yyyy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 12/10/2023", wFormattedDate == "12/10/2023");
    
    wFormatStr = "yyyy-mm-dd";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 2023-12-10", wFormattedDate == "2023-12-10");
    
    m_Application->Locale("us");
    wFormatStr = "dd-mmm-yy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10-Dec-23", wFormattedDate == "10-Dec-23");
    
    wFormatStr = "dddd, mmmm dd, yyyy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be Sunday, December 10, 2023", wFormattedDate == "Sunday, December 10, 2023");

    wFormatStr = "h:mm AM/PM";
    wFormattedDate = wDateFormatter->FormatTime(12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 12:00 PM", wFormattedDate == "12:00 PM");
    
    wFormatStr = "h:mm:ss AM/PM";
    wFormattedDate = wDateFormatter->FormatTime(12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 12:00:00 PM", wFormattedDate == "12:00:00 PM");

    wFormatStr = "mm/dd/yyyy h:mm";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 12/10/2023 12:00", wFormattedDate == "12/10/2023 12:00");

    wFormatStr = "dd/mm/yy h:mm";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10/12/23 12:00", wFormattedDate == "10/12/23 12:00");

    wFormatStr = "dd-mmm-yy h:mm";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10-Dec-23 12:00", wFormattedDate == "10-Dec-23 12:00");

    wFormatStr = "dd-mmm-yy h:mm AM/PM";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 0, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10-Dec-23 12:00 AM", wFormattedDate == "10-Dec-23 12:00 AM");

    wFormatStr = "dd-mmm-yy h:mm:ss AM/PM";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 0, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10-Dec-23 12:00:00 AM", wFormattedDate == "10-Dec-23 12:00:00 AM");

    wFormatStr = "dd/mm/yy h:mm:ss";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10/12/23 12:00:00", wFormattedDate == "10/12/23 12:00:00");

    // Multi-section Excel time (positive;negative-style layout): "h:mm;@" — first section applies to times.
    CPPUNIT_ASSERT_MESSAGE("h:mm;@ should be valid Excel date/time format", IsValidExcelDateFormat("h:mm;@"));
    wFormatStr = "h:mm;@";
    wFormattedDate = wDateFormatter->FormatTime(14, 30, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("h:mm;@ should render as 14:30", wFormattedDate == "14:30");

    {
        const auto wQuoted = SplitExcelFormatSections("\"a;b\";0");
        CPPUNIT_ASSERT_MESSAGE("quoted semicolon stays in section 1", wQuoted.size() == 2 && wQuoted[0] == "\"a;b\"");
        const auto wBracket = SplitExcelFormatSections("[h]:mm:ss;@");
        CPPUNIT_ASSERT_MESSAGE("semicolon inside brackets does not split", wBracket.size() == 2 && wBracket[0].find("[h]:mm:ss") != std::string::npos);
        const auto wFour = SplitExcelFormatSections("0;0;0;@");
        CPPUNIT_ASSERT_MESSAGE("fourth-section @ displays raw text", FormatExcelTextSection(wFour, "Hello") == "Hello");
    }

    // Fr =====================================================================
    m_Application->Locale("fr");
    wFormatStr = "dd/mm/yy h:mm:ss";
    wFormattedDate = wDateFormatter->FormatDateTime(2023, 12, 10, 12, 0, 0, wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10/12/23 12:00:00", wFormattedDate == "10/12/23 12:00:00");
    
    wFormatStr = "dd-mmm-yy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be 10-Déc.-23", wFormattedDate == "10-Déc.-23");
    
    wFormatStr = "dddd, mmmm dd, yyyy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("Formatted date should be Dimanche, Décembre 10, 2023", wFormattedDate == "Dimanche, Décembre 10, 2023");

    // French Excel uses j/jj/jjj/jjjj for day (same semantics as d/dd/ddd/dddd); aa/aaaa for year
    CPPUNIT_ASSERT_MESSAGE("Normalize jj/mm/aaaa to dd/mm/yyyy", NormalizeExcelDateFormatToEnglish("jj/mm/aaaa") == "dd/mm/yyyy");
    CPPUNIT_ASSERT_MESSAGE("Normalize preserves AM/PM", NormalizeExcelDateFormatToEnglish("jj/mm/aaaa h:mm AM/PM") == "dd/mm/yyyy h:mm AM/PM");
    CPPUNIT_ASSERT_MESSAGE("jjj should be valid Excel date format", IsValidExcelDateFormat("jjj"));
    wFormatStr = "jj/mm/yyyy";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("jj/mm/yyyy should match dd/mm/yyyy", wFormattedDate == wDateFormatter->FormatDate(wDate(), "dd/mm/yyyy"));
    wFormatStr = "jjj";
    wFormattedDate = wDateFormatter->FormatDate(wDate(), wFormatStr);
    CPPUNIT_ASSERT_MESSAGE("jjj should match ddd for weekday abbrev", wFormattedDate == wDateFormatter->FormatDate(wDate(), "ddd"));

    // Test AddDecimals method
    //cout << "Testing AddDecimals method..." << endl;
    
    tFormatString wFormatString;
    tString wResult;
    
    // Test adding decimals to format without decimal point
    wResult = wFormatString.AddDecimals("###", 1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###, 1) should return ###.0", wResult == "###.0");
    
    wResult = wFormatString.AddDecimals("###", 2);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###, 2) should return ###.00", wResult == "###.00");
    
    // Test adding more decimals to existing format
    wResult = wFormatString.AddDecimals("###.0", 1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0, 1) should return ###.00", wResult == "###.00");
    
    wResult = wFormatString.AddDecimals("###.00", 1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.00, 1) should return ###.000", wResult == "###.000");
    
    // Test removing decimals
    wResult = wFormatString.AddDecimals("###.00", -1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.00, -1) should return ###.0", wResult == "###.0");
    
    wResult = wFormatString.AddDecimals("###.0", -1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0, -1) should return ###", wResult == "###");
    
    // Test removing more decimals than available
    wResult = wFormatString.AddDecimals("###.0", -2);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0, -2) should return ###", wResult == "###");
    
    // Test with no change
    wResult = wFormatString.AddDecimals("###.00", 0);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.00, 0) should return ###.00", wResult == "###.00");
    
    // Test with empty string
    wResult = wFormatString.AddDecimals("", 1);
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(empty, 1) should return empty", wResult == "");
    
    // Test with currency symbols
    wResult = wFormatString.AddDecimals("### $", 1);
    //cout << "AddDecimals(### $, 1) = [" << wResult << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(### $, 1) should return ###.0 $", wResult == "###.0 $");
    
    wResult = wFormatString.AddDecimals("### $", 2);
    //cout << "AddDecimals(### $, 2) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(### $, 2) should return ###.00 $", wResult == "###.00 $");
    
    wResult = wFormatString.AddDecimals("###.0 $", 1);
    //cout << "AddDecimals(###.0 $, 1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0 $, 1) should return ###.00 $", wResult == "###.00 $");
    
    wResult = wFormatString.AddDecimals("###.00 $", -1);
    //cout << "AddDecimals(###.00 $, -1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.00 $, -1) should return ###.0 $", wResult == "###.0 $");
    
    wResult = wFormatString.AddDecimals("###.0 $", -1);
    //cout << "AddDecimals(###.0 $, -1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0 $, -1) should return ### $", wResult == "### $");
    
    // Test with percentage
    wResult = wFormatString.AddDecimals("### %", 1);
    //cout << "AddDecimals(### %, 1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(### %, 1) should return ###.0 %", wResult == "###.0 %");
    
    wResult = wFormatString.AddDecimals("###.0 %", -1);
    //cout << "AddDecimals(###.0 %, -1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals(###.0 %, -1) should return ### %", wResult == "### %");
    
    // Test with complex format
    wResult = wFormatString.AddDecimals("$###.00", 1);
    //cout << "AddDecimals($###.00, 1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals($###.00, 1) should return $###.000", wResult == "$###.000");
    
    wResult = wFormatString.AddDecimals("$###.00", -1);
    //cout << "AddDecimals($###.00, -1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE("AddDecimals($###.00, -1) should return $###.0", wResult == "$###.0");
    
    // Complex Excel-like numeric formats (tags, multi-sections, quotes)
    // FR accounting-like: add one decimal across sections
    tString frAcc = "[$€-fr-FR]* #.##0,00_-;([$€-fr-FR]* #.##0,00_-);([$€-fr-FR]* \"-\"_-);(@_)";
    wResult = wFormatString.AddDecimals(frAcc, 1);
    //cout << "AddDecimals(fr accounting +1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE(
        "AddDecimals(fr accounting, +1) should add one 0 after comma in both numeric sections",
        wResult == "[$€-fr-FR]* #.##0,000_-;([$€-fr-FR]* #.##0,000_-);([$€-fr-FR]* \"-\"_-);(@_)"
    );

    // FR accounting-like: remove two decimals -> no decimals in numeric sections
    wResult = wFormatString.AddDecimals(frAcc, -2);
    //cout << "AddDecimals(fr accounting -2) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE(
        "AddDecimals(fr accounting, -2) should remove decimals (comma and digits)",
        wResult == "[$€-fr-FR]* #.##0_-;([$€-fr-FR]* #.##0_-);([$€-fr-FR]* \"-\"_-);(@_)"
    );

    // EN GBP two-section format: add, then remove all
    tString enGbp = "[$£-en-GB]#,##0.00;([$£-en-GB]#,##0.00)";
    wResult = wFormatString.AddDecimals(enGbp, 1);
    //cout << "AddDecimals(en GBP +1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE(
        "AddDecimals(en GBP, +1) should add one 0 after dot in both sections",
        wResult == "[$£-en-GB]#,##0.000;([$£-en-GB]#,##0.000)"
    );

    wResult = wFormatString.AddDecimals(enGbp, -2);
    //cout << "AddDecimals(en GBP -2) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE(
        "AddDecimals(en GBP, -2) should remove decimals entirely",
        wResult == "[$£-en-GB]#,##0;([$£-en-GB]#,##0)"
    );

    // Mixed literals and symbols: ensure suffix preserved
    tString percWithText = "#,##0.00\" km\";(#,##0.00\" km\")";
    wResult = wFormatString.AddDecimals(percWithText, -1);
    //cout << "AddDecimals(text suffix -1) = [" << result << "]" << endl;
    CPPUNIT_ASSERT_MESSAGE(
        "AddDecimals on quoted suffix should only shrink decimals",
        wResult == "#,##0.0\" km\";(#,##0.0\" km\")"
    );
    
    //cout << "AddDecimals tests completed successfully!" << endl;
    
    // Test C++ to Excel format conversion
}
void TestSkFormatExcel::setUp() {
    m_Application = tApplication::Instance();
}

void TestSkFormatExcel::tearDown() {
}
 
