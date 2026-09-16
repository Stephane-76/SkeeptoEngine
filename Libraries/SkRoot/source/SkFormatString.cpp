//=============================================================================
// SkFormatString tFormatStringClass
/*
 * @page tFormatStringClass 
 * @par 
 * @par Allows you to format numbers or dates as a string
 */ 
//=============================================================================
#include "../include/SkVariant.hpp"
#include "../include/SkFormatString.hpp"
#include "../include/SkApplication.hpp"

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <regex>
#include <cmath>
#include <algorithm>
#include <cctype>

// Bridges to Excel-like date/number formatting
#include "../include/SkFormatDate.hpp"
#include "../include/SkFormatNumber.hpp"
#include "../include/SkLocale.hpp"

namespace SkRoot {


    tShort GetNbDecimal(tDouble sNumber) {
        tShort wReturn = 0;
        tInt wPow = 1;
        tDouble wTmp;
        
        while ( wReturn < 12) {
            wTmp = trunc(sNumber * wPow)/wPow;
            if ( wTmp == sNumber ) return wReturn;
            ++wReturn;
            wPow *= 10;
        }
        return(0);
    }

    tRecFormatString::tRecFormatString(tFormatStringType  sType,
                                     tString              sKey,
                                     tString              sLocalFormat,
                                     tShort               sPrecision,
                                     tFormatStringFamily  sFormatFamily) :  tClass(),
                                                                            m_Type(sType),
                                                                            m_Key(sKey),
                                                                            m_LocalFormat(sLocalFormat),
                                                                            m_Decimal(sPrecision),
                                                                            m_FormatFamily(sFormatFamily){
                                                        
                                                                                }

    tRecFormatString::tRecFormatString(const tRecFormatString& sRecFormatString) {
        m_Type=sRecFormatString.m_Type;
        m_Key=sRecFormatString.m_Key;
        m_LocalFormat=sRecFormatString.m_LocalFormat;
        m_Decimal=sRecFormatString.m_Decimal;
        m_FormatFamily=sRecFormatString.m_FormatFamily;
    }


    // tFormatString ===========================================================
    tFormatString::tFormatString() : tClass(),m_FormatStringType(tFormatStringType::none), m_UnitMoney(t_UnitMoney::None),m_Decimal(0),m_ExcelFormat("")  {}


    void tFormatString::Clear() {
        m_FormatStringType=tFormatStringType::none;
        m_UnitMoney=t_UnitMoney::None;
        m_Decimal=0;
        m_ExcelFormat="";
    }

    tFormatStringRoot* tFormatString::FormatStringRoot() {
        return(tApplication::Instance()->FormatStringRoot());
    }

    void tFormatString::Format(tString sFormat) { m_FormatStringType=FormatStringRoot()->StringKey2FormatStringType(sFormat); };
    void tFormatString::FormatType(tFormatStringType sFormat) { m_FormatStringType=sFormat; };

    tFormatStringType tFormatString::FormatType() const { return(m_FormatStringType); }
    tString tFormatString::FormatLocal() { return(FormatStringRoot()->FormatStringType2LocalFormatString(m_FormatStringType)); };

    void tFormatString::Money(t_UnitMoney sUnitMoney) { m_UnitMoney=sUnitMoney; }
    t_UnitMoney tFormatString::Money() const { return(m_UnitMoney); }

    void tFormatString::Decimal(tShort sDecimal) { m_Decimal=sDecimal; }
    tShort tFormatString::Decimal() { return(m_Decimal); }

    tBool tFormatString::ExcelFormat(tString sExcelFormat) {
       
        if (IsValidExcelNumberFormat(sExcelFormat)) {
            m_ExcelFormat = NormalizeExcelNumberFormatToUs(sExcelFormat);
            m_FormatStringType=tFormatStringType::excelnumber;
            return(true);
        }
        if (IsValidExcelDateFormat(sExcelFormat)) {
            m_ExcelFormat=sExcelFormat;
            m_FormatStringType=tFormatStringType::exceldate;
            return(true);
        }
        return(false);
    }
    tString  tFormatString::FormatExcel() const { return(m_ExcelFormat); }

    tString tFormatString::ExcelEquivalent(const tString& locale) {
        // Custom Excel format: return as-is
        if (m_FormatStringType == tFormatStringType::excelnumber || m_FormatStringType == tFormatStringType::exceldate) {
            return m_ExcelFormat;
        }
        if (m_FormatStringType == tFormatStringType::none) {
            return tString("");
        }
        tFormatStringRoot* wRoot = FormatStringRoot();
        // Resolve locale: empty means use application current locale (same as tLocale)
        tString wLocaleKey = locale;
        if (wLocaleKey.empty() && tApplication::Instance() != nullptr && tApplication::Instance()->Locale() != nullptr) {
            wLocaleKey = tApplication::Instance()->Locale()->Lang();
        }
        tString wExcel = wRoot->FormatStringType2LocalFormatString(m_FormatStringType);
        if (wExcel.empty() || wExcel == "None") {
            return wExcel;
        }
        // For date formats, use same order as tLocale::Set(): FR/SP/IT = day/month (dd/mm), US/EN/DE = month/day (mm-dd)
        if (wRoot->FormatString2Family(m_FormatStringType) == tFormatStringFamily::date) {
            tString wLocaleLower = wLocaleKey;
            std::transform(wLocaleLower.begin(), wLocaleLower.end(), wLocaleLower.begin(), [](tChar c) { return static_cast<tChar>(std::tolower(static_cast<unsigned char>(c))); });
            tBool wEuropeanOrder = false;
            if (wLocaleLower.size() >= 2) {
                tString wLang = wLocaleLower.substr(0, 2);
                if (wLang == "fr" || wLang == "sp" || wLang == "it") {
                    wEuropeanOrder = true;  // same as tLocale: FR, SP, IT use %d/%m/%Y
                }
                // US, EN, DE keep default mm-dd (month-day)
            }
            if (wEuropeanOrder) {
                size_t pos = 0;
                while ((pos = wExcel.find("mm-dd", pos)) != tString::npos) {
                    wExcel.replace(pos, 5, "dd/mm");
                    pos += 5;
                }
            }
        }
        return wExcel;
    }

    tBool tFormatString::SetDefaultFormat(tVariant* sVariant) {
        // Format by default
        if ((sVariant!=nullptr) &&(m_FormatStringType==tFormatStringType::none)) {
            
            if (sVariant->Type()==tVariantType::t_date) {
                tClassDate wClassDate(sVariant->Date());
                if (wClassDate.IsHours()) {
                    m_FormatStringType=tFormatStringType::datehmm;
                } else {
                    m_FormatStringType=tFormatStringType::dateshort;
                }
            }
            switch (sVariant->Type()) {
                case tVariantType::t_null: break;
                case tVariantType::t_int:
                    m_FormatStringType=tFormatStringType::numeric;
                    m_Decimal=0;
                    break;
                case tVariantType::t_bool:  break;
                case tVariantType::t_double:
                    m_FormatStringType=tFormatStringType::numeric;
                    m_Decimal=GetNbDecimal(sVariant->Double());
                    break;
                case tVariantType::t_string:  break;
                case tVariantType::t_error:  break;
                case tVariantType::t_date: {
                    m_FormatStringType=tFormatStringType::datelong;
                    break;
                }
                case tVariantType::t_class:  break;
                default:
                    break;
            }
            return(true);
        }
        return(false);
    }

    void tFormatString::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("f");
        sWriter->Int(tInt(m_FormatStringType));
        if (m_UnitMoney!=t_UnitMoney::None) {
            sWriter->Key("m");
            sWriter->Int(tInt(m_UnitMoney));
        }
        // Always emit "p" so undo precision and round-trip see zero decimals (excelnumber + c).
        sWriter->Key("p");
        sWriter->Int(m_Decimal);
        if ((m_FormatStringType==tFormatStringType::excelnumber) || (m_FormatStringType==tFormatStringType::exceldate)) {
            sWriter->Key("c");
            sWriter->String(m_ExcelFormat.c_str());
        }

        sWriter->EndObject();
    }

    void tFormatString::Json(const rapidjson::Value& sValue) {
        m_FormatStringType = tFormatStringType(sValue["f"].GetInt());
        if (sValue.HasMember("m"))
            m_UnitMoney = t_UnitMoney(sValue["m"].GetInt());
        if (sValue.HasMember("p") && sValue["p"].IsInt())
            m_Decimal = sValue["p"].GetInt();
        else
            m_Decimal = 0;
        if (sValue.HasMember("c")) {
            m_ExcelFormat = sValue["c"].GetString();
        }
    }

    tBool tFormatString::Empty() {
        return(m_FormatStringType==tFormatStringType::none);
    }

    tBool tFormatString::operator == (tFormatString& sFormatString) {
        return((m_FormatStringType == sFormatString.m_FormatStringType) &&
               (m_UnitMoney == sFormatString.m_UnitMoney) &&
               (m_Decimal == sFormatString.m_Decimal) &&
               (m_ExcelFormat == sFormatString.m_ExcelFormat)
               );
    }

    tFormatString tFormatString::operator = (tFormatString& sFormatString) {
        m_FormatStringType = sFormatString.m_FormatStringType;
        m_UnitMoney = sFormatString.m_UnitMoney;
        m_Decimal = sFormatString.m_Decimal;
        m_ExcelFormat = sFormatString.m_ExcelFormat;
        return(*this);
    }


    // Root Container =========================================================
    tFormatStringRoot::tFormatStringRoot()  : tClass() {
        Fill();
    }

    tFormatStringRoot::~tFormatStringRoot() {
        Clear();
    }

    void tFormatStringRoot::Clear() {
        m_VectorFormatString.clear();
    }

void  tFormatStringRoot::Fill() {
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::numeric,"0","0",0,tFormatStringFamily::numeric));
    
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::numeric0,"0.00","0.00",2,tFormatStringFamily::numeric));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::numeric_,"#,##0","#,##0",0,tFormatStringFamily::numeric ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::numeric0_,"#,##0.00", "#,##0.00",2,tFormatStringFamily::numeric));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::numeric0_P,"#,##0.00;(#,##0.00)","#,##0.00;(#,##0.00)",2,tFormatStringFamily::numeric));
    
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::percent, "0%","0%",0, tFormatStringFamily::percent));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::percent0,"0.00%","0.00%",2, tFormatStringFamily::percent));
    
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::scientific,"0.00E+00","0.00E+00",2, tFormatStringFamily::scientific ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::scientific1,"0.000E+00","0.000E+00",3, tFormatStringFamily::scientific ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::scientific2,"0.0000E+00","0.0000E+00",4, tFormatStringFamily::scientific ));
    
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::accounting,"#,##0 $;-#,##0 $","#,##0 $;-#,##0 $",0, tFormatStringFamily::accounting ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::accountingP,"#,##0 $;(#,##0) $","#,##0 $;(#,##0) $",0, tFormatStringFamily::accounting ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::accounting0, "#,##0.00 $;-#,##0.00 $","#,##0.00 $;-#,##0.00 $",2,  tFormatStringFamily::accounting ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::accounting0P,"#,##0.00 $;(#,##0.00) $", "#,##0.00 $;(#,##0.00) $",2,  tFormatStringFamily::accounting ));
    
    
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datelong, "mm-dd-yyyy","mm-dd-yyyy",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::dateshort, "mm-dd-yy","mm-dd-yy",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datestr,"dddd mmmm dd","dddd mmmm dd",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datestryy,"dddd mmmm dd yy","dddd mmmm dd yy",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datestryyyy,"dddd mmmm dd yyyy","dddd mmmm dd yyyy",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datedmm,"mmmm dd","mmmm dd",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datemmmyy,"mmmm yyyy","mmmm yyyy",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datehmmap,"h:mm AM/PM","h:mm AM/PM",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datehmmssap,"h:mm:ss AM/PM","h:mm:ss AM/PM",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datehmm, "h:mm","h:mm",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::datehmmss,"h:mm:ss", "h:mm:ss",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::dateddmmyyyyhmm,"mm-dd-yyyy h:mm","mm-dd-yyyy h:mm",0, tFormatStringFamily::date ));
    m_VectorFormatString.push_back(tRecFormatString(tFormatStringType::dateddmmyyyyhmmss,"mm-dd-yyyy h:mm:ss","mm-dd-yyyy h:mm:ss",0, tFormatStringFamily::date ));    
    }
    // Interface FormatString
    tFormatStringType tFormatStringRoot::StringKey2FormatStringType(tString sFormatStringKey) {
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Key ==sFormatStringKey) return(wRecFormatString.m_Type);
        }
        return(tFormatStringType::none);
    }
    
    tFormatStringFamily tFormatStringRoot::StringKey2FormatStringFamily(tString sFormatStringKey) {
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Key ==sFormatStringKey) return(wRecFormatString.m_FormatFamily);
        }
        return(tFormatStringFamily::none);
    }
    
    void tFormatStringRoot::SetLocalFormatString(tFormatStringType sFormatStringType,tString sFormatString) {
        int wPos=0;
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Type==sFormatStringType)  {
                m_VectorFormatString[wPos].m_LocalFormat=sFormatString;
                return;
            }
            wPos++;
        }
    }
    
    tString tFormatStringRoot::FormatStringType2LocalFormatString(tFormatStringType sFormatStringType) {
        //cout << "Search " << tInt(sFormatStringType) << endl;
        for(auto wRecFormatString : m_VectorFormatString) {
            //cout <<  tInt(wRecFormatString.m_Type) << ": " << wRecFormatString.m_Key << ":" << wRecFormatString.m_LocalFormat << endl;
            if (wRecFormatString.m_Type==sFormatStringType) return(wRecFormatString.m_LocalFormat);
        }
        return("None");
    }

    tString tFormatStringRoot::FormatStringType2Key(tFormatStringType sFormatStringType) {
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Type==sFormatStringType) return(wRecFormatString.m_Key);
        }
        return("");
    }
    
    tFormatStringFamily tFormatStringRoot::FormatString2Family(tFormatStringType sFormatStringType) {
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Type==sFormatStringType) return(wRecFormatString.m_FormatFamily);
        }
        return(tFormatStringFamily::none);
    }
    
    
    tShort tFormatStringRoot::DefaultPrecision(tFormatStringType sFormatString) {
        for(auto wRecFormatString : m_VectorFormatString) {
            if (wRecFormatString.m_Type==sFormatString) return(wRecFormatString.m_Decimal);
        }
        return(0);
    }
    
    tString tFormatStringRoot::JsonFormatString() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        
        wWriter.StartObject();
        wWriter.Key("formatstring");
        wWriter.StartArray();
        for(auto wRecFormatStringFamily : CstRecFormatStringFamily) {
            wWriter.StartObject();
            wWriter.Key("code");
            wWriter.String(wRecFormatStringFamily.m_Label.c_str());
            wWriter.Key("fs");
            wWriter.StartArray();
            for(auto wRecFormatString :m_VectorFormatString) {
                if (wRecFormatString.m_FormatFamily==wRecFormatStringFamily.m_Family) {
                    wWriter.StartObject();
                    wWriter.Key("f");
                    wWriter.String(wRecFormatString.m_Key.c_str());
                    wWriter.Key("l");
                    wWriter.String(wRecFormatString.m_LocalFormat.c_str());
                    wWriter.EndObject();
                }
            }
            wWriter.EndArray();
            wWriter.EndObject();
        }
        wWriter.EndArray();
        wWriter.EndObject();
        
        return(wStringBuffer.GetString());
    }
    
    tString tFormatStringRoot::DefaultFormatString(tString sFormatString) {
        tVariant wVariant;
        tString wMoneySymbol="";
        tFormatString wFormatString;
        wFormatString.Decimal(DefaultPrecision(StringKey2FormatStringType(sFormatString)));
        wFormatString.Format(sFormatString);
        
        tFormatStringFamily wFamily=StringKey2FormatStringFamily(sFormatString);
        switch (wFamily) {
            case tFormatStringFamily::numeric: wVariant=-1000000; break;
            case tFormatStringFamily::percent: wVariant=0.50; break;
            case tFormatStringFamily::scientific: wVariant=1234567890.12; break;
            case tFormatStringFamily::accounting: {
                wVariant=-1000000;
                // Get Default locale
                wMoneySymbol=  tApplication::Instance()->Locale()->MoneySymbol();
                break;
            }
            case tFormatStringFamily::date: {
                tClassDate wClassDate;
                wClassDate.Now();
                wVariant.SetDate(wClassDate.Value());
                break;
            }
            default: return("Error unknow format"); break;
        }
        tStringStream wResult;
        wResult << wVariant.FormatString(&wFormatString);
        if (wMoneySymbol!="") wResult << " " << wMoneySymbol;
        return(wResult.str());
    }
    
    // Excel bridge ============================================================
    tBool tFormatStringRoot::IsValidExcelDate(tString sFormatString) {
        return SkRoot::IsValidExcelDateFormat(sFormatString);
    }
    
    tBool tFormatStringRoot::IsValidExcelNumber(tString sFormatString) {
        return SkRoot::IsValidExcelNumberFormat(sFormatString);
    }
    
    tString tFormatStringRoot::ExcelToLocalFormatString(tString sExcelFormat) {
        // Try date first
        if (SkRoot::IsValidExcelDateFormat(sExcelFormat)) {
            return SkRoot::ConvertToSkRootDateFormat(sExcelFormat);
        }
        // Then number
        if (SkRoot::IsValidExcelNumberFormat(sExcelFormat)) {
            return SkRoot::ConvertToSkRootFormat(sExcelFormat);
        }
        return "";
    }
    
    tString tFormatStringRoot::DefaultFormatStringExcel(tString sExcelFormat) {
        // If date-like
        if (SkRoot::IsValidExcelDateFormat(sExcelFormat)) {
            // Use current date for preview
            tClassDate wNow;
            wNow.Now();
            tInt y = wNow.Year();
            tInt m = wNow.Month();
            tInt d = wNow.Day();
            return SkRoot::FormatWithExcelDateFormat(y, m, d, sExcelFormat);
        }
        // If number-like
        if (SkRoot::IsValidExcelNumberFormat(sExcelFormat)) {
            // Sample value chosen to exercise grouping/decimals/percent
            tDouble wSample = 1234567.89;
            return SkRoot::FormatWithExcelNumberFormat(wSample, sExcelFormat);
        }
        return tString("Error unknow excel format");
    }
    
    tString tFormatString::AddDecimals(const tString& format, tInt decimalChange) {
        if (format.empty() || decimalChange == 0) {
            return format;
        }

        // Split into up to 4 sections by ';' while respecting quotes "..." and brackets [...]
        auto splitSections = [](const tString& s) {
            vector<tString> sections;
            tString current;
            bool inQuotes = false;
            bool inBracket = false;
            for (size_t i = 0; i < s.size(); ++i) {
                char c = s[i];
                if (c == '"') {
                    inQuotes = !inQuotes;
                    current.push_back(c);
                    continue;
                }
                if (!inQuotes) {
                    if (c == '[') inBracket = true;
                    if (c == ']') inBracket = false;
                    if (c == ';' && !inBracket) {
                        sections.push_back(current);
                        current.clear();
                        continue;
                    }
                }
                current.push_back(c);
            }
            sections.push_back(current);
            return sections;
        };

        auto joinSections = [](const vector<tString>& v) {
            tString out;
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) out.push_back(';');
                out += v[i];
            }
            return out;
        };

        auto adjustOneSection = [&](const tString& sec) -> tString {
            if (sec.empty()) return sec;

            // Find the main numeric run (last run of [#0.,,]) not inside quotes or brackets
            size_t runStart = tString::npos;
            size_t runEnd = tString::npos;
            bool inQuotes = false;
            bool inBracket = false;

            for (size_t i = 0; i < sec.size(); ++i) {
                char c = sec[i];
                if (c == '"') { inQuotes = !inQuotes; continue; }
                if (inQuotes) continue;
                if (c == '[') { inBracket = true; continue; }
                if (c == ']') { inBracket = false; continue; }
                if (inBracket) continue;

                auto isNumChar = [](char ch){ return ch=='#' || ch=='0' || ch==',' || ch=='.'; };
                if (isNumChar(c)) {
                    if (runStart == tString::npos) runStart = i;
                    runEnd = i; // inclusive
                } else {
                    if (runStart != tString::npos) {
                        // end of a run
                        runStart = tString::npos;
                    }
                }
            }

            // If we ended inside a run, we already have runEnd set to last numeric char
            // But we might have multiple runs; we want the last one, so scan backwards to refine
            if (runEnd != tString::npos) {
                // rewind runStart to include contiguous numeric chars ending at runEnd
                size_t s = runEnd;
                while (s>0) {
                    char ch = sec[s-1];
                    if (ch=='#' || ch=='0' || ch==',' || ch=='.') { --s; }
                    else break;
                }
                runStart = s;
            }

            if (runStart == tString::npos || runEnd == tString::npos) {
                return sec; // nothing to adjust
            }

            tString prefix = sec.substr(0, runStart);
            tString numeric = sec.substr(runStart, runEnd - runStart + 1);
            tString suffix = sec.substr(runEnd + 1);

            auto buildZeros = [](tInt n){ tString z; for(tInt i=0;i<n;++i) z += '0'; return z; };

            // When both '.' and ',' appear (e.g. #.##0,00 FR or #,##0.00 US), tExcelFormatParser uses
            // NormalizeExcelNumberFormatToUs internally so GetDecimalSeparator() may not match the
            // original string — rfind(decSep) can hit the wrong '.' or ','. Use legacy heuristics.
            const bool wHasDotAndComma =
                (numeric.find('.') != tString::npos) && (numeric.find(',') != tString::npos);

            // Prefer tExcelFormatParser: distinguishes thousands comma in #,##00 from decimal in #,##0.00.
            if (!wHasDotAndComma && IsValidExcelNumberFormat(numeric)) {
                tExcelFormatParser parser(numeric);
                if (parser.IsValid()) {
                    const tString decSep = parser.GetDecimalSeparator();
                    const size_t sepLen = decSep.length();
                    if (decSep.empty()) {
                        if (decimalChange > 0) {
                            // Excel format *code* uses '.' for a new fractional part here; do not use
                            // Locale()->Decimal() (e.g. fr => ',') or patterns like ### break tests and #,##00 gets ',0'.
                            numeric.push_back('.');
                            numeric += buildZeros(decimalChange);
                        }
                        return prefix + numeric + suffix;
                    }
                    size_t sepPos = (sepLen == 1)
                        ? numeric.rfind(decSep[0])
                        : numeric.rfind(decSep);
                    // If normalized parser dec sep is absent from original (should not happen here), legacy
                    if (sepPos != tString::npos) {
                        size_t decCount = 0;
                        for (size_t i = sepPos + sepLen; i < numeric.size(); ++i) {
                            char c = numeric[i];
                            if (c == '0' || c == '#') decCount++; else break;
                        }
                        if (decimalChange > 0) {
                            size_t insertPos = sepPos + sepLen + decCount;
                            numeric.insert(insertPos, buildZeros(decimalChange));
                        } else {
                            tInt toRemove = -decimalChange;
                            const size_t afterDecStart = sepPos + sepLen;
                            if (toRemove >= (tInt)decCount) {
                                numeric = numeric.substr(0, sepPos) + numeric.substr(afterDecStart + decCount);
                            } else if (toRemove > 0) {
                                size_t keepUntil = afterDecStart + (decCount - (size_t)toRemove);
                                numeric = numeric.substr(0, keepUntil) + numeric.substr(afterDecStart + decCount);
                            }
                        }
                        return prefix + numeric + suffix;
                    }
                }
            }

            // Fallback: legacy heuristics when format is not a valid Excel number pattern
            size_t dotPos = numeric.rfind('.');
            size_t commaPos = numeric.rfind(',');
            size_t sepPos = tString::npos;
            if (dotPos != tString::npos && commaPos != tString::npos) {
                // Prefer the separator whose immediate tail is strictly decimal placeholders (only 0/#)
                auto isStrictDecimalTail = [&](size_t pos) {
                    if (pos == tString::npos || pos + 1 >= numeric.size()) return false;
                    size_t i = pos + 1;
                    bool hasAtLeastOne = false;
                    for (; i < numeric.size(); ++i) {
                        char c = numeric[i];
                        if (c == '0' || c == '#') { hasAtLeastOne = true; continue; }
                        break;
                    }
                    return hasAtLeastOne; // must have at least one placeholder immediately after
                };
                bool dotDecimal = isStrictDecimalTail(dotPos);
                bool commaDecimal = isStrictDecimalTail(commaPos);
                if (commaDecimal && !dotDecimal) sepPos = commaPos;
                else if (dotDecimal && !commaDecimal) sepPos = dotPos;
                else sepPos = std::max(dotPos, commaPos); // fallback to rightmost
            } else if (dotPos != tString::npos) {
                sepPos = dotPos;
            } else if (commaPos != tString::npos) {
                // Treat comma as potential decimal only if there are trailing 0/# after it, else it is grouping
                size_t tmp = commaPos;
                size_t k = tmp + 1;
                bool hasTrailDigits = false;
                while (k < numeric.size() && (numeric[k]=='0' || numeric[k]=='#')) { hasTrailDigits = true; ++k; }
                if (hasTrailDigits) {
                    sepPos = tmp;
                }
            }

            if (sepPos == tString::npos) {
                // No explicit decimal part in numeric run
                if (decimalChange > 0) {
                    // Insert decimal separator + zeros at end of numeric run (before suffix)
                    char insSep = (numeric.find('.') != tString::npos) ? '.' : ((numeric.find(',') != tString::npos) ? ',' : '.');
                    numeric += insSep;
                    numeric += buildZeros(decimalChange);
                }
                // If negative change and no decimals, nothing to remove
                return prefix + numeric + suffix;
            }

            // Count current decimals length (only '0' or '#')
            size_t decCount = 0;
            for (size_t i = sepPos + 1; i < numeric.size(); ++i) {
                char c = numeric[i];
                if (c == '0' || c == '#') decCount++; else break;
            }

            if (decimalChange > 0) {
                size_t insertPos = sepPos + 1 + decCount;
                numeric.insert(insertPos, buildZeros(decimalChange));
            } else {
                tInt toRemove = -decimalChange;
                if (toRemove >= (tInt)decCount) {
                    // Remove sep and all decimals
                    numeric = numeric.substr(0, sepPos) + numeric.substr(sepPos + 1 + decCount);
                } else if (toRemove > 0) {
                    size_t keepUntil = sepPos + 1 + (decCount - (size_t)toRemove);
                    numeric = numeric.substr(0, keepUntil) + numeric.substr(sepPos + 1 + decCount);
                }
            }

            return prefix + numeric + suffix;
        };

        auto sections = splitSections(format);
        for (auto& sec : sections) {
            sec = adjustOneSection(sec);
        }
        return joinSections(sections);
    }
        
    } // End of namespace
