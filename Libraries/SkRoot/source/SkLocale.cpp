//
//  SkLocale.cpp
//  SkRoot
//
//  Created by stephane allez on 07/02/2024.
//

#include "../include/SkLocale.hpp"
#include "../include/SkApplication.hpp"

namespace SkRoot {

    namespace {
        void UpdateAccountingFormatLabels(t_UnitMoney sUnitMoney) {
            tApplication* wApp = tApplication::Instance();
            if (wApp == nullptr) {
                return;
            }
            tFormatStringRoot* wFormatStringRoot = wApp->FormatStringRoot();
            if (wFormatStringRoot == nullptr) {
                return;
            }
            tString wSym = UnitMoneySymbol(sUnitMoney);
            if (wSym.empty()) {
                wSym = "$";
            }
            auto wWithSymbol = [&](const tString& sPattern) {
                tString wResult = sPattern;
                for (tSize wPos = 0; (wPos = wResult.find('$', wPos)) != tString::npos; ) {
                    wResult.replace(wPos, 1, wSym);
                    wPos += wSym.length();
                }
                return wResult;
            };
            wFormatStringRoot->SetLocalFormatString(
                tFormatStringType::accounting, wWithSymbol("#,##0 $;-#,##0 $"));
            wFormatStringRoot->SetLocalFormatString(
                tFormatStringType::accountingP, wWithSymbol("#,##0 $;(#,##0) $"));
            wFormatStringRoot->SetLocalFormatString(
                tFormatStringType::accounting0, wWithSymbol("#,##0.00 $;-#,##0.00 $"));
            wFormatStringRoot->SetLocalFormatString(
                tFormatStringType::accounting0P, wWithSymbol("#,##0.00 $;(#,##0.00) $"));
        }
    }

    // Locale-independent Excel token normalization (French j/a, etc. -> English d/y). Used by SkFormatDate and m_ExcelFormatDate* defaults.
    tString tLocale::NormalizeExcelDateFormatToEnglish(const tString& sFormatString) {
        tString out;
        out.reserve(sFormatString.size());
        for (tSize i = 0; i < sFormatString.size(); ) {
            if (i + 4 < sFormatString.size()) {
                const tString wToken5 = sFormatString.substr(i, 5);
                if (wToken5 == "AM/PM" || wToken5 == "am/pm") {
                    out += wToken5;
                    i += 5;
                    continue;
                }
            }
            if (i + 2 < sFormatString.size()) {
                const tString wToken3 = sFormatString.substr(i, 3);
                if (wToken3 == "A/P" || wToken3 == "a/p") {
                    out += wToken3;
                    i += 3;
                    continue;
                }
            }
            const tChar c = sFormatString[i];
            if (c == 'j' || c == 'J') {
                tSize wEnd = i;
                while (wEnd < sFormatString.size() && sFormatString[wEnd] == c) {
                    ++wEnd;
                }
                const tSize wLen = wEnd - i;
                out.append(wLen, static_cast<tChar>(c == 'j' ? 'd' : 'D'));
                i = wEnd;
            } else if (c == 'a' || c == 'A') {
                tSize wEnd = i;
                while (wEnd < sFormatString.size() && sFormatString[wEnd] == c) {
                    ++wEnd;
                }
                const tSize wLen = wEnd - i;
                out.append(wLen, static_cast<tChar>(c == 'a' ? 'y' : 'Y'));
                i = wEnd;
            } else {
                out += c;
                ++i;
            }
        }
        return out;
    }

    
    tLocale::tLocale() : tClass() {
        Lang("fr");
    }

    void tLocale::Lang(tString sLang) {
        if (sLang=="us") m_Lang=tLang::US;
        if (sLang=="fr") m_Lang=tLang::FR;
        if (sLang=="en") m_Lang=tLang::EN;
        if (sLang=="de") m_Lang=tLang::DE;
        if (sLang=="sp") m_Lang=tLang::SP;
        if (sLang=="it") m_Lang=tLang::IT;
        Set();
    }
    tString tLocale::Lang() {
        switch (m_Lang) {
            case tLang::US : return("us");
            case tLang::FR : return("fr");
            case tLang::EN : return("en");
            case tLang::DE : return("de");
            case tLang::SP : return("sp");
            case tLang::IT : return("it");
            default:
                break;
        }
        return("");
    }

    void tLocale::SetDayMonth(tLang sLang) {
        switch (sLang) {
            case tLang::FR: {
                // Full day names (0=Sunday, 1=Monday, ..., 6=Saturday)
                m_DayStr[0]="Dimanche";
                m_DayStr[1]="Lundi";
                m_DayStr[2]="Mardi";
                m_DayStr[3]="Mercredi";
                m_DayStr[4]="Jeudi";
                m_DayStr[5]="Vendredi";
                m_DayStr[6]="Samedi";
                
                // Abbreviated day names
                m_DayStrAbbr[0]="Dim.";
                m_DayStrAbbr[1]="Lun.";
                m_DayStrAbbr[2]="Mar.";
                m_DayStrAbbr[3]="Mer.";
                m_DayStrAbbr[4]="Jeu.";
                m_DayStrAbbr[5]="Ven.";
                m_DayStrAbbr[6]="Sam.";
                
                // Full month names (1=January, 2=February, ..., 12=December)
                m_MonthStr[1]="Janvier";
                m_MonthStr[2]="Février";
                m_MonthStr[3]="Mars";
                m_MonthStr[4]="Avril";
                m_MonthStr[5]="Mai";
                m_MonthStr[6]="Juin";
                m_MonthStr[7]="Juillet";
                m_MonthStr[8]="Août";
                m_MonthStr[9]="Septembre";
                m_MonthStr[10]="Octobre";
                m_MonthStr[11]="Novembre";
                m_MonthStr[12]="Décembre";
                
                // Abbreviated month names
                m_MonthStrAbbr[1]="Janv.";
                m_MonthStrAbbr[2]="Févr.";
                m_MonthStrAbbr[3]="Mars";
                m_MonthStrAbbr[4]="Avr.";
                m_MonthStrAbbr[5]="Mai";
                m_MonthStrAbbr[6]="Juin";
                m_MonthStrAbbr[7]="Juil.";
                m_MonthStrAbbr[8]="Août";
                m_MonthStrAbbr[9]="Sept.";
                m_MonthStrAbbr[10]="Oct.";
                m_MonthStrAbbr[11]="Nov.";
                m_MonthStrAbbr[12]="Déc.";
                break;
            }
            case tLang::DE: {
                // German
                m_DayStr[0]="Sonntag";
                m_DayStr[1]="Montag";
                m_DayStr[2]="Dienstag";
                m_DayStr[3]="Mittwoch";
                m_DayStr[4]="Donnerstag";
                m_DayStr[5]="Freitag";
                m_DayStr[6]="Samstag";
                
                m_DayStrAbbr[0]="So";
                m_DayStrAbbr[1]="Mo";
                m_DayStrAbbr[2]="Di";
                m_DayStrAbbr[3]="Mi";
                m_DayStrAbbr[4]="Do";
                m_DayStrAbbr[5]="Fr";
                m_DayStrAbbr[6]="Sa";
                
                m_MonthStr[1]="Januar";
                m_MonthStr[2]="Februar";
                m_MonthStr[3]="März";
                m_MonthStr[4]="April";
                m_MonthStr[5]="Mai";
                m_MonthStr[6]="Juni";
                m_MonthStr[7]="Juli";
                m_MonthStr[8]="August";
                m_MonthStr[9]="September";
                m_MonthStr[10]="Oktober";
                m_MonthStr[11]="November";
                m_MonthStr[12]="Dezember";
                
                m_MonthStrAbbr[1]="Jan";
                m_MonthStrAbbr[2]="Feb";
                m_MonthStrAbbr[3]="Mär";
                m_MonthStrAbbr[4]="Apr";
                m_MonthStrAbbr[5]="Mai";
                m_MonthStrAbbr[6]="Jun";
                m_MonthStrAbbr[7]="Jul";
                m_MonthStrAbbr[8]="Aug";
                m_MonthStrAbbr[9]="Sep";
                m_MonthStrAbbr[10]="Okt";
                m_MonthStrAbbr[11]="Nov";
                m_MonthStrAbbr[12]="Dez";
                break;
            }
            case tLang::SP: {
                // Spanish
                m_DayStr[0]="Domingo";
                m_DayStr[1]="Lunes";
                m_DayStr[2]="Martes";
                m_DayStr[3]="Miércoles";
                m_DayStr[4]="Jueves";
                m_DayStr[5]="Viernes";
                m_DayStr[6]="Sábado";
                
                m_DayStrAbbr[0]="Dom";
                m_DayStrAbbr[1]="Lun";
                m_DayStrAbbr[2]="Mar";
                m_DayStrAbbr[3]="Mié";
                m_DayStrAbbr[4]="Jue";
                m_DayStrAbbr[5]="Vie";
                m_DayStrAbbr[6]="Sáb";
                
                m_MonthStr[1]="Enero";
                m_MonthStr[2]="Febrero";
                m_MonthStr[3]="Marzo";
                m_MonthStr[4]="Abril";
                m_MonthStr[5]="Mayo";
                m_MonthStr[6]="Junio";
                m_MonthStr[7]="Julio";
                m_MonthStr[8]="Agosto";
                m_MonthStr[9]="Septiembre";
                m_MonthStr[10]="Octubre";
                m_MonthStr[11]="Noviembre";
                m_MonthStr[12]="Diciembre";
                
                m_MonthStrAbbr[1]="Ene";
                m_MonthStrAbbr[2]="Feb";
                m_MonthStrAbbr[3]="Mar";
                m_MonthStrAbbr[4]="Abr";
                m_MonthStrAbbr[5]="May";
                m_MonthStrAbbr[6]="Jun";
                m_MonthStrAbbr[7]="Jul";
                m_MonthStrAbbr[8]="Ago";
                m_MonthStrAbbr[9]="Sep";
                m_MonthStrAbbr[10]="Oct";
                m_MonthStrAbbr[11]="Nov";
                m_MonthStrAbbr[12]="Dic";
                break;
            }
            case tLang::IT: {
                // Italian
                m_DayStr[0]="Domenica";
                m_DayStr[1]="Lunedì";
                m_DayStr[2]="Martedì";
                m_DayStr[3]="Mercoledì";
                m_DayStr[4]="Giovedì";
                m_DayStr[5]="Venerdì";
                m_DayStr[6]="Sabato";
                
                m_DayStrAbbr[0]="Dom";
                m_DayStrAbbr[1]="Lun";
                m_DayStrAbbr[2]="Mar";
                m_DayStrAbbr[3]="Mer";
                m_DayStrAbbr[4]="Gio";
                m_DayStrAbbr[5]="Ven";
                m_DayStrAbbr[6]="Sab";
                
                m_MonthStr[1]="Gennaio";
                m_MonthStr[2]="Febbraio";
                m_MonthStr[3]="Marzo";
                m_MonthStr[4]="Aprile";
                m_MonthStr[5]="Maggio";
                m_MonthStr[6]="Giugno";
                m_MonthStr[7]="Luglio";
                m_MonthStr[8]="Agosto";
                m_MonthStr[9]="Settembre";
                m_MonthStr[10]="Ottobre";
                m_MonthStr[11]="Novembre";
                m_MonthStr[12]="Dicembre";
                
                m_MonthStrAbbr[1]="Gen";
                m_MonthStrAbbr[2]="Feb";
                m_MonthStrAbbr[3]="Mar";
                m_MonthStrAbbr[4]="Apr";
                m_MonthStrAbbr[5]="Mag";
                m_MonthStrAbbr[6]="Giu";
                m_MonthStrAbbr[7]="Lug";
                m_MonthStrAbbr[8]="Ago";
                m_MonthStrAbbr[9]="Set";
                m_MonthStrAbbr[10]="Ott";
                m_MonthStrAbbr[11]="Nov";
                m_MonthStrAbbr[12]="Dic";
                break;
            }
            default: {
                // English/US
                m_DayStr[0]="Sunday";
                m_DayStr[1]="Monday";
                m_DayStr[2]="Tuesday";
                m_DayStr[3]="Wednesday";
                m_DayStr[4]="Thursday";
                m_DayStr[5]="Friday";
                m_DayStr[6]="Saturday";
                
                m_DayStrAbbr[0]="Sun";
                m_DayStrAbbr[1]="Mon";
                m_DayStrAbbr[2]="Tue";
                m_DayStrAbbr[3]="Wed";
                m_DayStrAbbr[4]="Thu";
                m_DayStrAbbr[5]="Fri";
                m_DayStrAbbr[6]="Sat";
                
                m_MonthStr[1]="January";
                m_MonthStr[2]="February";
                m_MonthStr[3]="March";
                m_MonthStr[4]="April";
                m_MonthStr[5]="May";
                m_MonthStr[6]="June";
                m_MonthStr[7]="July";
                m_MonthStr[8]="August";
                m_MonthStr[9]="September";
                m_MonthStr[10]="October";
                m_MonthStr[11]="November";
                m_MonthStr[12]="December";
                
                m_MonthStrAbbr[1]="Jan";
                m_MonthStrAbbr[2]="Feb";
                m_MonthStrAbbr[3]="Mar";
                m_MonthStrAbbr[4]="Apr";
                m_MonthStrAbbr[5]="May";
                m_MonthStrAbbr[6]="Jun";
                m_MonthStrAbbr[7]="Jul";
                m_MonthStrAbbr[8]="Aug";
                m_MonthStrAbbr[9]="Sep";
                m_MonthStrAbbr[10]="Oct";
                m_MonthStrAbbr[11]="Nov";
                m_MonthStrAbbr[12]="Dec";
                break;
            }
        }
    }

    void tLocale::Set() {
        switch (m_Lang) {
            case tLang::DE:
            case tLang::EN:
            case tLang::US : {
                m_Decimal= '.';
                m_Thousand = ',';
                m_Grouping ="\03";
                m_Date='-';
                m_Time=':';
                m_Arg=',';
                tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datelong, "%m-%d-%Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateshort, "%m-%d-%y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestr, "%A %B %d");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryy, "%A %B %d %y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryyyy, "%A %B %d %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datedmm, "%B %d");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datemmmyy, "%B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmssap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmm, "%H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmss, "%H:%M:%S");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmm, "%m-%d-%Y %H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmmss, "%m-%d-%Y %H:%M:%S");
                // Restore US Excel-style numeric labels after FR session (keys stay #,##0).
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0, "0.00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric_, "#,##0");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_, "#,##0.00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_P, "#,##0.00;(#,##0.00)");
                switch (m_Lang) {
                    case tLang::US: m_UnitMoney=t_UnitMoney::usd; break;
                    case tLang::EN: m_UnitMoney=t_UnitMoney::gpb; break;
                    default: m_UnitMoney=t_UnitMoney::eur; break;
                }
                // Excel-style patterns (English letters: d/m/y; French Excel: j/a for day/year)
                switch (m_Lang) {
                    case tLang::US:
                        m_ExcelFormatDateShort = "mm/dd/yyyy";
                        m_ExcelFormatDateLong = "dddd, mmmm dd, yyyy";
                        m_ExcelFormatDateTime = "mm/dd/yyyy hh:mm";
                        break;
                    case tLang::EN:
                        m_ExcelFormatDateShort = "dd/mm/yyyy";
                        m_ExcelFormatDateLong = "dddd dd mmmm yyyy";
                        m_ExcelFormatDateTime = "dd/mm/yyyy hh:mm";
                        break;
                    case tLang::DE:
                        m_ExcelFormatDateShort = "dd.mm.yyyy";
                        m_ExcelFormatDateLong = "dddd, dd. mmmm yyyy";
                        m_ExcelFormatDateTime = "dd.mm.yyyy hh:mm";
                        break;
                    default:
                        m_ExcelFormatDateShort = "mm/dd/yyyy";
                        m_ExcelFormatDateLong = "dddd, mmmm dd, yyyy";
                        m_ExcelFormatDateTime = "mm/dd/yyyy hh:mm";
                        break;
                }
                break;
            }
            case tLang::SP:
            case tLang::IT: {
                m_Decimal= ',';
                m_Thousand = '.';
                m_Grouping ="\03";
                m_Date='/';
                m_Time=':';
                m_Arg=';';
                tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datelong, "%d/%m/%Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateshort, "%d/%m/%y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestr, "%A %d %B");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryy, "%A %d %B %y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryyyy, "%A %d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datedmm, "%d %B");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datemmmyy, "%d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmssap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmm, "%H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmss, "%H:%M:%S");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmm, "%d/%m/%Y %H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmmss, "%d/%m/%Y %H:%M:%S");
                m_UnitMoney=t_UnitMoney::eur;
                m_ExcelFormatDateShort = "dd/mm/yyyy";
                m_ExcelFormatDateLong = "dddd dd mmmm yyyy";
                m_ExcelFormatDateTime = "dd/mm/yyyy hh:mm";
                break;
            }
            case tLang::FR : {
                // fr-FR: narrow space as thousands sep (shown as ' '), comma as decimal.
                // Do NOT use '.' — "1.000" looks like three decimals, not one thousand.
                m_Decimal= ',';
                m_Thousand = ' ';
                m_Grouping ="\03";
                m_Date='/';
                m_Time=':';
                m_Arg=';';
                tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datelong, "%d/%m/%Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateshort, "%d/%m/%y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestr, "%A %d %B");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryy, "%A %d %B %y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryyyy, "%A %d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datedmm, "%d %B");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datemmmyy, "%d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmssap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmm, "%H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmss, "%H:%M:%S");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmm, "%d/%m/%Y %H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmmss, "%d/%m/%Y %H:%M:%S");
                // Menu labels: space grouping + comma decimals (keys stay US Excel #,##0).
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0, "0,00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric_, "# ##0");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_, "# ##0,00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_P, "# ##0,00;(# ##0,00)");
                m_UnitMoney=t_UnitMoney::eur;
                // French Excel tokens (j/a); parser uses tLocale::NormalizeExcelDateFormatToEnglish.
                m_ExcelFormatDateShort = "jj/mm/aaaa";
                m_ExcelFormatDateLong = "jjjj jj mmmm aaaa";
                m_ExcelFormatDateTime = "jj/mm/aaaa hh:mm";
                break;
            }
            default:
                // French fallback (same as tLang::FR)
                m_Decimal= ',';
                m_Thousand = ' ';
                m_Grouping ="\03";
                m_Date='/';
                m_Time=':';
                m_Arg=';';
                tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datelong, "%d/%m/%Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateshort, "%d/%m/%y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestr, "%A %d %B ");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryy, "%A %d %B %y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datestryyyy, "%A %d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datedmm, "%d %B");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datemmmyy, "%d %B %Y");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmssap, "%p");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmm, "%H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::datehmmss, "%H:%M:%S");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmm, "%d/%m/%Y %H:%M");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::dateddmmyyyyhmmss, "%d/%m/%Y %H:%M:%S");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0, "0,00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric_, "# ##0");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_, "# ##0,00");
                wFormatStringRoot->SetLocalFormatString(tFormatStringType::numeric0_P, "# ##0,00;(# ##0,00)");
                m_UnitMoney=t_UnitMoney::eur;
                // Same French Excel defaults as tLang::FR branch above
                m_ExcelFormatDateShort = "jj/mm/aaaa";
                m_ExcelFormatDateLong = "jjjj jj mmmm aaaa";
                m_ExcelFormatDateTime = "jj/mm/aaaa hh:mm";
                break;
        }
        UpdateAccountingFormatLabels(m_UnitMoney);
        SetDayMonth(m_Lang);
    }

    tChar tLocale::Decimal() { return(m_Decimal);}
    
    tChar tLocale::Thousand() { return(m_Thousand);}

    tString tLocale::Grouping() { return(m_Grouping);}

    tChar tLocale::Date() { return(m_Date);}

    tChar tLocale::Time() { return(m_Time);}

    tChar tLocale::Arg() { return(m_Arg);}
    
tString tLocale::FormatDateLong() { return(tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(tFormatStringType::datelong)); }

    tString tLocale::FormatDateShort() { return(tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(tFormatStringType::dateshort)); }

    tString tLocale::FormatTimeLong() { return(tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(tFormatStringType::datehmmss)); }

    tString tLocale::FormatTimeShort()  { return(tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(tFormatStringType::datehmm)); }

    tString tLocale::ExcelFormatDateShort() {
        return m_ExcelFormatDateShort;
    }

    tString tLocale::ExcelFormatDateLong() {
        return m_ExcelFormatDateLong;
    }

    tString tLocale::ExcelFormatDateTime() {
        return m_ExcelFormatDateTime;
    }

    tString tLocale::DayStr(tInt sDayNum) {
        assert((sDayNum>=0) && (sDayNum<7));
        return(m_DayStr[sDayNum]);
    }

    tString tLocale::MonthStr(tInt sMonthNum) {
        assert((sMonthNum>=1) && (sMonthNum<=12));
        return(m_MonthStr[sMonthNum]);
    }
    
    tString tLocale::DayStrAbbr(tInt sDayNum) {
        assert((sDayNum>=0) && (sDayNum<7));
        return(m_DayStrAbbr[sDayNum]);
    }
    
    tString tLocale::MonthStrAbbr(tInt sMonthNum) {
        assert((sMonthNum>=1) && (sMonthNum<=12));
        return(m_MonthStrAbbr[sMonthNum]);
    }

    t_UnitMoney tLocale::UnitMoney() {
        return (m_UnitMoney);
    }

    tString tLocale::MoneySymbol() {
        return(UnitMoneySymbol(m_UnitMoney));
    }

    tLocalePush::tLocalePush(const tString& sLang) : m_Prev(), m_Changed(false) {
        tApplication* wApp = tApplication::Instance();
        if (wApp == nullptr || wApp->Locale() == nullptr) {
            return;
        }
        m_Prev = wApp->Locale()->Lang();
        if (m_Prev != sLang) {
            wApp->Locale(sLang);
            m_Changed = true;
        }
    }

    tLocalePush::~tLocalePush() {
        if (!m_Changed) {
            return;
        }
        tApplication* wApp = tApplication::Instance();
        if (wApp != nullptr && wApp->Locale() != nullptr) {
            wApp->Locale(m_Prev);
        }
    }
}
