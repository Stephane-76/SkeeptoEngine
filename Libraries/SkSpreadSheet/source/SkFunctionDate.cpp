//=============================================================================
// SkSpreadSheet Function Date
//=============================================================================
#include "../include/SkFunctionDate.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkColRowCellRange.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>
#include <vector>

namespace SkSpreadSheet {

namespace {

// Fraction of a 24h day (0 <= f < 1) from clock h,m,s after NormalizeExcelTimeToClock.
static tDouble ExcelTimeFractionFromClock(tInt sHour, tInt sMinute, tInt sSecond) {
    return (static_cast<tDouble>(sHour) * 3600.0
            + static_cast<tDouble>(sMinute) * 60.0
            + static_cast<tDouble>(sSecond)) / 86400.0;
}

// HOUR/MINUTE/SECOND: support t_date, or Excel serial as int/double (fractional day from serial - floor(serial)).
static tBool ClockFromExcelStyleSerial(const tVariant& wArg, tInt& oH, tInt& oM, tInt& oS) {
    if (wArg.IsDate()) {
        tClassDate wDate(wArg.Date());
        wDate.HourMinuteSecond(oH, oM, oS);
        return true;
    }
    if (wArg.IsInt() || wArg.IsDouble()) {
        const tDouble serial = wArg.Numeric();
        tDouble frac = serial - std::floor(serial);
        if (frac < 0) {
            frac += 1.0;
        }
        long long sec = static_cast<long long>(std::llround(frac * 86400.0));
        sec %= 86400;
        if (sec < 0) {
            sec += 86400;
        }
        oH = static_cast<tInt>(sec / 3600);
        sec %= 3600;
        oM = static_cast<tInt>(sec / 60);
        oS = static_cast<tInt>(sec % 60);
        return true;
    }
    return false;
}

static void NormalizeExcelTimeToClock(tInt& hour, tInt& minute, tInt& second) {
    long long total = static_cast<long long>(hour) * 3600LL
                    + static_cast<long long>(minute) * 60LL
                    + static_cast<long long>(second);
    long long norm = total % 86400LL;
    if (norm < 0) {
        norm += 86400LL;
    }
    hour = static_cast<tInt>(norm / 3600LL);
    norm %= 3600LL;
    minute = static_cast<tInt>(norm / 60LL);
    second = static_cast<tInt>(norm % 60LL);
}

// Excel-like month offset: int/double (trunc), bool, numeric string, null/blank -> 0, date -> serial trunc.
tBool MonthsOffsetFromMonthArg(const tVariant& v, tInt& sOut) {
	if (v.IsNull()) {
		sOut = 0;
		return true;
	}
	if (v.IsInt()) {
		sOut = v.Int();
		return true;
	}
	if (v.IsDouble()) {
		sOut = static_cast<tInt>(std::trunc(v.Double()));
		return true;
	}
	if (v.IsBool()) {
		sOut = v.Bool() ? 1 : 0;
		return true;
	}
	if (v.IsString()) {
		const tString& s = v.String();
		tSize start = 0;
		while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
			++start;
		}
		if (start >= s.size()) {
			sOut = 0;
			return true;
		}
		try {
			size_t idx = 0;
			const tString sub = s.substr(start);
			const tDouble d = std::stod(sub, &idx);
			while (idx < sub.size() && std::isspace(static_cast<unsigned char>(sub[idx]))) {
				++idx;
			}
			if (idx != sub.size()) {
				return false;
			}
			sOut = static_cast<tInt>(std::trunc(d));
			return true;
		} catch (...) {
			return false;
		}
	}
	if (v.IsDate()) {
		const tDouble wSerial = static_cast<tDouble>(v.Date()) / 86400.0 + 25569.0;
		sOut = static_cast<tInt>(std::trunc(wSerial));
		return true;
	}
	if (v.IsError()) {
		return false;
	}
	return false;
}

} // namespace

	tFunctionSpillKind tFunctionDate::SpillKind() const {
		return(tFunctionSpillKind::ElementWise);
	}

	// Function =============================================================
	tFunctionToday::tFunctionToday() : tFunction() {}

	tStackElem tFunctionToday::Call(tStackElems* sStackElems, tShort sNbArg) {
		std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        // Clean up (even though TODAY doesn't use arguments)
        tClassDate wClassDate(tClassDate::Now());
        tVariant wReturn;
        wReturn.SetDate(wClassDate.Value());
        return(tStackElem(wReturn));
	};

    // Function Now =============================================================
    tFunctionNow::tFunctionNow() : tFunction() {}

    tStackElem tFunctionNow::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NOW requires no arguments"))));
        }
        // Clean up (even though NOW doesn't use arguments)
        // NOW returns current date and time (same as TODAY but with time)
        tClassDate wClassDate(tClassDate::Now());
        tVariant wReturn;
        wReturn.SetDate(wClassDate.Value());
        return(tStackElem(wReturn));
    };

    
    // Function Date =============================================================
    tFunctionDateCreate::tFunctionDateCreate() : tFunction() {}

    tStackElem tFunctionDateCreate::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DATE requires 3 arguments"))));
        }
        
        // In RPN: wArgs[0]=day, wArgs[1]=month, wArgs[2]=year
        tInt wYearInt = 0, wMonthInt = 0, wDayInt = 0;
        if (!StackElemToInt(wArgs[2], wYearInt) || !StackElemToInt(wArgs[1], wMonthInt) || !StackElemToInt(wArgs[0], wDayInt)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DATE requires numeric year, month, day"))));
        }
        // Excel year rule: 0..1899 is offset from 1900; a negative year is #NUM!.
        if (wYearInt < 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, "#NUM!"))));
        }
        if (wYearInt < 1900) {
            wYearInt += 1900;
        }
        // Excel normalizes out-of-range month/day (e.g. DATE(y,13,0) -> last day of December y).
        // tClassDate::Set() relies on mktime, which rolls tm_mon/tm_mday over, so pass the raw values.
        tClassDate wClassDate(wYearInt, wMonthInt, wDayInt);
        tVariant wReturn;
        wReturn.SetDate(wClassDate.Value());
        return(tStackElem(wReturn));
    };

    //======================================================================
    tFunctionYear::tFunctionYear() : tFunctionDate() {}

    tStackElem tFunctionYear::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        if (wArg.IsDate()) {
            tClassDate wDate(wArg.Date());
            return(tStackElem(tVariant(wDate.Year())));
        }
        return(tStackElem(wArg));
    }
    //======================================================================
    tFunctionMonth::tFunctionMonth() : tFunctionDate() {}

    tStackElem tFunctionMonth::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        if (wArg.IsDate()) {
            tClassDate wDate(wArg.Date());
            return(tStackElem(tVariant(wDate.Month())));
        }
        return(tStackElem(wArg));
    }

    //======================================================================
    tFunctionDay::tFunctionDay() : tFunctionDate() {}

    tStackElem tFunctionDay::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
      
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        if (wArg.IsDate()) {
            tClassDate wDate(wArg.Date());
            return(tStackElem(tVariant(wDate.Day())));
        }
        return(tStackElem(wArg));
    }
    //======================================================================
    tFunctionWeekDay::tFunctionWeekDay() : tFunctionDate() {}

    tStackElem tFunctionWeekDay::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 1 || wArgs.size() > 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        // RPN: serial_number return_type WEEKDAY -> wArgs[0]=return_type, wArgs[1]=date (same order as TEXT)
        tVariant wArg;
        tInt wReturnType = 1;
        if (wArgs.size() == 1) {
            if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        } else {
            if (!StackElemToInt(wArgs[0], wReturnType))
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "WEEKDAY: return_type must be numeric"))));
            const tBool wValid =
                (wReturnType >= 1 && wReturnType <= 3) ||
                (wReturnType >= 11 && wReturnType <= 17);
            if (!wValid)
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, "#NUM!"))));
            if (!StackElemToVariant(wArgs[1], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (!wArg.IsDate()) return(tStackElem(wArg));
        // Excel WEEKDAY return_type (see Microsoft docs):
        // 1 or omitted: 1=Sun..7=Sat; 2: 1=Mon..7=Sun; 3: 0=Mon..6=Sun
        // 11: same as 2; 12..16: 1 = Tue..Sat respectively through 7th day; 17: same as 1
        // Invalid return_type (e.g. 4–10) → #NUM!
        tClassDate wDate(wArg.Date());
        tInt wTmWday = wDate.DayWeek(); /* 0=Sunday, 1=Monday, ..., 6=Saturday */
        tInt wResult = 0;
        if (wReturnType == 1 || wReturnType == 17) {
            wResult = wTmWday + 1;
        } else if (wReturnType == 2 || wReturnType == 11) {
            wResult = (wTmWday == 0) ? 7 : wTmWday;
        } else if (wReturnType == 3) {
            wResult = (wTmWday + 6) % 7;
        } else if (wReturnType >= 12 && wReturnType <= 16) {
            const tInt wStart = wReturnType - 10; /* 2=Tuesday .. 6=Saturday as first day of week numbering */
            wResult = ((wTmWday - wStart + 7) % 7) + 1;
        } else {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, "#NUM!"))));
        }
        return(tStackElem(tVariant(wResult)));
    }

    //======================================================================
    tFunctionWeekNum::tFunctionWeekNum() : tFunctionDate() {}
    
    tStackElem tFunctionWeekNum::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        if (wArg.IsDate()) {
            tClassDate wDate(wArg.Date());
            return(tStackElem(tVariant(wDate.WeekNum())));
        }
        return(tStackElem(wArg));
    }

    //======================================================================
    tFunctionTime::tFunctionTime() : tFunctionDate() {}
    
    tStackElem tFunctionTime::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }   
        // In RPN: wArgs[0]=second, wArgs[1]=minute, wArgs[2]=hour
        tInt wHour = 0, wMinute = 0, wSecond = 0;
        if (!StackElemToInt(wArgs[2], wHour) || !StackElemToInt(wArgs[1], wMinute) || !StackElemToInt(wArgs[0], wSecond)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TIME requires numeric hour, minute, second"))));
        }

        NormalizeExcelTimeToClock(wHour, wMinute, wSecond);

        // Excel TIME returns a decimal in [0,1): fraction of a day only (not a full t_date on 1900-01-01),
        // so date + TIME uses date + double → AddDouble, matching Excel/OOXML.
        tVariant wResult;
        wResult.SetDouble(ExcelTimeFractionFromClock(wHour, wMinute, wSecond));
        return(tStackElem(wResult));
    }

    //======================================================================
    tFunctionHour::tFunctionHour() : tFunctionDate() {}

    tStackElem tFunctionHour::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        tInt wH = 0, wM = 0, wS = 0;
        if (ClockFromExcelStyleSerial(wArg, wH, wM, wS)) {
            return(tStackElem(tVariant(wH)));
        }
        return(tStackElem(wArg));
    }

    //======================================================================
    tFunctionMinute::tFunctionMinute() : tFunctionDate() {}

    tStackElem tFunctionMinute::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        tInt wH = 0, wM = 0, wS = 0;
        if (ClockFromExcelStyleSerial(wArg, wH, wM, wS)) {
            return(tStackElem(tVariant(wM)));
        }
        return(tStackElem(wArg));
    }

    //======================================================================
    tFunctionSecond::tFunctionSecond() : tFunctionDate() {}

    tStackElem tFunctionSecond::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        tInt wH = 0, wM = 0, wS = 0;
        if (ClockFromExcelStyleSerial(wArg, wH, wM, wS)) {
            return(tStackElem(tVariant(wS)));
        }
        return(tStackElem(wArg));
    }

    //======================================================================
    //! Function EDATE - Add months to a date
    tFunctionEdate::tFunctionEdate() : tFunctionDate() {}

    tStackElem tFunctionEdate::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EDATE requires 2 arguments"))));
        }

        // EDATE(start_date, months) - In RPN: wArgs[0]=months, wArgs[1]=start_date
        tVariant wStartDate;
        if (!StackElemToVariant(wArgs[1], wStartDate)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EDATE: start_date must be a date"))));
        }
        if (!wStartDate.IsDate()) wStartDate.SetDate(tClassDate::Now());

        tVariant wMonthsV;
        if (!StackElemToVariant(wArgs[0], wMonthsV)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EDATE: months must be numeric"))));
        }
        if (wMonthsV.IsError()) {
            return(tStackElem(wMonthsV));
        }
        tInt wMonths = 0;
        if (!MonthsOffsetFromMonthArg(wMonthsV, wMonths)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EDATE: months must be numeric"))));
        }

        // Get year, month, day from start date
        tClassDate wStartDateObj(wStartDate.Date());
        tInt wYear = 0;
        tInt wMonth = 0;
        tInt wDay = 0;
        wStartDateObj.YearMonthDay(wYear, wMonth, wDay);

        // Add months
        wMonth += wMonths;
        
        // Handle year overflow/underflow
        while (wMonth > 12) {
            wMonth -= 12;
            wYear++;
        }
        while (wMonth < 1) {
            wMonth += 12;
            wYear--;
        }

        // Handle day overflow (e.g., Jan 31 + 1 month = Feb 28/29)
        // Excel behavior: if day doesn't exist in target month, use last day of that month
        tInt wMaxDay = 31; // Default max
        if (wMonth == 2) {
            // February: check for leap year
            bool wIsLeapYear = (wYear % 4 == 0 && wYear % 100 != 0) || (wYear % 400 == 0);
            wMaxDay = wIsLeapYear ? 29 : 28;
        } else if (wMonth == 4 || wMonth == 6 || wMonth == 9 || wMonth == 11) {
            wMaxDay = 30;
        }

        // Adjust day if it exceeds max days in target month
        if (wDay > wMaxDay) {
            wDay = wMaxDay;
        }

        // Create new date
        tClassDate wResultDate(wYear, wMonth, wDay);
        tVariant wResult;
        wResult.SetDate(wResultDate.Value());
        // Clean up
        return(tStackElem(wResult));
    }

    //======================================================================
    //! Function EOMONTH - Last day of the month, offset by months
    tFunctionEomonth::tFunctionEomonth() : tFunctionDate() {}

    tStackElem tFunctionEomonth::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EOMONTH requires 2 arguments"))));
        }

        // EOMONTH(start_date, months) - In RPN: wArgs[0]=months, wArgs[1]=start_date
        tVariant wStartDate;
        if (!StackElemToVariant(wArgs[1], wStartDate)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EOMONTH: start_date must be a date"))));
        }
        if (!wStartDate.IsDate()) wStartDate.SetDate(tClassDate::Now());

        tVariant wMonthsV;
        if (!StackElemToVariant(wArgs[0], wMonthsV)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EOMONTH: months must be numeric"))));
        }
        if (wMonthsV.IsError()) {
            return(tStackElem(wMonthsV));
        }
        tInt wMonths = 0;
        if (!MonthsOffsetFromMonthArg(wMonthsV, wMonths)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EOMONTH: months must be numeric"))));
        }

        tClassDate wStartDateObj(wStartDate.Date());
        tInt wYear = 0, wMonth = 0, wDay = 0;
        wStartDateObj.YearMonthDay(wYear, wMonth, wDay);

        wMonth += wMonths;
        while (wMonth > 12) { wMonth -= 12; wYear++; }
        while (wMonth < 1) { wMonth += 12; wYear--; }

        // Last day of target month
        tInt wLastDay = 31;
        if (wMonth == 2) {
            bool wIsLeapYear = (wYear % 4 == 0 && wYear % 100 != 0) || (wYear % 400 == 0);
            wLastDay = wIsLeapYear ? 29 : 28;
        } else if (wMonth == 4 || wMonth == 6 || wMonth == 9 || wMonth == 11) {
            wLastDay = 30;
        }

        tClassDate wResultDate(wYear, wMonth, wLastDay);
        tVariant wResult;
        wResult.SetDate(wResultDate.Value());
        return(tStackElem(wResult));
    }

    //======================================================================
    // Fallback numeric-date parser for DATEVALUE. Excel's DATEVALUE accepts ISO 8601 (year-first,
    // e.g. "2017-05-31") regardless of locale, plus the locale's day/month order for the
    // slash/dash/dot forms ("31/05/2017" in FR, "05/31/2017" in US). tVariant::Parse only matches
    // the single locale separator+order, so it rejects ISO text and cross-separator dates; this
    // covers the gap. Accepts "-", "/" and "." separators. Returns true and fills y/m/d on success.
    static tBool DateValueParseNumeric(const tString& sStr, tInt& sYear, tInt& sMonth, tInt& sDay) {
        tInt wParts[3] = {0, 0, 0};
        tInt wPartLen[3] = {0, 0, 0};
        tInt wNb = 0;
        tInt wCur = 0;
        tBool wInNum = false;
        for (tSize wI = 0; wI <= sStr.size(); ++wI) {
            const tChar wC = (wI < sStr.size()) ? sStr[wI] : '\0';
            if (wC >= '0' && wC <= '9') {
                if (wNb >= 3) {
                    return false;
                }
                wCur = wCur * 10 + (wC - '0');
                wInNum = true;
                if (++wPartLen[wNb] > 4) {
                    return false;
                }
            } else if (wC == '-' || wC == '/' || wC == '.' || wC == '\0') {
                if (wInNum) {
                    if (wNb >= 3) {
                        return false;
                    }
                    wParts[wNb++] = wCur;
                    wCur = 0;
                    wInNum = false;
                } else if (wC != '\0') {
                    // Separator with no preceding number (leading/double separator): not a date.
                    return false;
                }
                if (wC == '\0') {
                    break;
                }
            } else {
                // Any other character means this is not a plain numeric date string.
                return false;
            }
        }
        if (wNb != 3) {
            return false;
        }

        tInt wY = 0;
        tInt wM = 0;
        tInt wD = 0;
        if (wPartLen[0] == 4) {
            // ISO 8601: yyyy-mm-dd.
            wY = wParts[0];
            wM = wParts[1];
            wD = wParts[2];
        } else {
            // Year is the last component; day/month order follows the locale, with value-based
            // disambiguation (a component > 12 can only be the day).
            wY = wParts[2];
            const tInt wA = wParts[0];
            const tInt wB = wParts[1];
            const tBool wUsOrder = (tApplication::Instance()->Locale()->Lang() == tString("us"));
            if (wA > 12 && wB <= 12) {
                wD = wA;
                wM = wB;
            } else if (wB > 12 && wA <= 12) {
                wM = wA;
                wD = wB;
            } else if (wUsOrder) {
                wM = wA;
                wD = wB;
            } else {
                wD = wA;
                wM = wB;
            }
            // Excel two-digit year rule: 0-29 -> 2000-2029, 30-99 -> 1930-1999.
            if (wY < 100) {
                wY += (wY < 30) ? 2000 : 1900;
            }
        }

        if (wM < 1 || wM > 12 || wD < 1 || wD > 31) {
            return false;
        }
        sYear = wY;
        sMonth = wM;
        sDay = wD;
        return true;
    }

    //! Function DATEVALUE - Excel: text date -> serial (Sker stores t_date for date ops)
    static tVariant DateValueFromText(const tString& sText) {
        tString wStr = sText;
        // Trim leading/trailing spaces (Excel DATEVALUE ignores outer whitespace).
        tSize wBegin = 0;
        while (wBegin < wStr.size() && std::isspace(static_cast<unsigned char>(wStr[wBegin]))) {
            ++wBegin;
        }
        tSize wEnd = wStr.size();
        while (wEnd > wBegin && std::isspace(static_cast<unsigned char>(wStr[wEnd - 1]))) {
            --wEnd;
        }
        wStr = wStr.substr(wBegin, wEnd - wBegin);
        if (wStr.empty()) {
            return tVariant(tClassError(tTypeError::t_value, "DATEVALUE: empty string"));
        }

        tVariant wParsed;
        wParsed.Parse(wStr);
        if (wParsed.IsDate()) {
            return wParsed;
        }
        if (wParsed.IsInt() || wParsed.IsDouble()) {
            tClassDate wDate(wParsed);
            tVariant wOut;
            wOut.SetDate(wDate.Value());
            return wOut;
        }
        // tVariant::Parse only matches the locale separator/order. Fall back to the tolerant
        // numeric parser so ISO ("2017-05-31") and cross-separator dates still resolve, matching
        // Excel DATEVALUE (workbooks build ISO text via TEXT(date,"aaaa-mm-jj")).
        tInt wY = 0;
        tInt wM = 0;
        tInt wD = 0;
        if (DateValueParseNumeric(wStr, wY, wM, wD)) {
            tClassDate wDate(wY, wM, wD);
            tVariant wOut;
            wOut.SetDate(wDate.Value());
            return wOut;
        }
        return tVariant(tClassError(tTypeError::t_value, "DATEVALUE: cannot parse date"));
    }

    tFunctionDateValue::tFunctionDateValue() : tFunctionDate() {}

    tStackElem tFunctionDateValue::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DATEVALUE requires 1 argument"))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DATEVALUE: invalid argument"))));
        }
        if (wArg.IsError()) {
            return(tStackElem(wArg));
        }
        if (wArg.IsDate()) {
            return(tStackElem(wArg));
        }
        if (wArg.IsInt() || wArg.IsDouble()) {
            tClassDate wDate(wArg);
            tVariant wOut;
            wOut.SetDate(wDate.Value());
            return(tStackElem(wOut));
        }
        if (wArg.IsString()) {
            tVariant wResult = DateValueFromText(wArg.String());
            return(tStackElem(wResult));
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "DATEVALUE: argument must be text or date"))));
    }

    namespace {
        static tBool NetworkDaysVariantToDate(const tVariant& sValue, tClassDate& oDate) {
            if (sValue.IsError()) {
                return false;
            }
            if (sValue.IsDate() || sValue.IsInt() || sValue.IsDouble()) {
                oDate = tClassDate(sValue);
                return true;
            }
            return false;
        }

        static tInt NetworkDaysDayKey(tClassDate& sDate) {
            tInt wY = 0;
            tInt wM = 0;
            tInt wD = 0;
            sDate.YearMonthDay(wY, wM, wD);
            return wY * 10000 + wM * 100 + wD;
        }

        static tDouble ExcelSerialDay(tClassDate& sDate) {
            return std::floor(static_cast<tDouble>(sDate.Value()) / 86400.0 + 25569.0);
        }

        static tBool NetworkDaysAddHoliday(const tVariant& sVal, std::set<tInt>& oHolidays, tVariant* oError) {
            if (sVal.IsNull()) {
                return true;
            }
            if (sVal.IsError()) {
                if (oError != nullptr) {
                    *oError = sVal;
                }
                return false;
            }
            tClassDate wDate;
            if (!NetworkDaysVariantToDate(sVal, wDate)) {
                return true; // ignore non-dates
            }
            oHolidays.insert(NetworkDaysDayKey(wDate));
            return true;
        }

        static tBool CollectHolidays(const tStackElem& sHolArg, std::set<tInt>& oHolidays, tVariant& oError) {
            tBool wOk = true;
            if (sHolArg.Type() == tStackType::t_Range) {
                tRange* wRange = sHolArg.Range();
                if (wRange != nullptr) {
                    tColRowCellRange* wCr = wRange->ColRowCellRange();
                    if (wCr != nullptr) {
                        for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom() && wOk; ++wRow) {
                            for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight() && wOk; ++wCol) {
                                tCell* wCell = wCr->Cell(wRow, wCol);
                                tVariant wVal = (wCell != nullptr) ? wCell->CalculableValue() : tVariant();
                                wOk = NetworkDaysAddHoliday(wVal, oHolidays, &oError);
                            }
                        }
                    }
                }
            } else if (sHolArg.Type() == tStackType::t_Array) {
                tArrayValue* wArray = sHolArg.Array();
                if (wArray != nullptr) {
                    for (tIndex r = 0; r < wArray->m_Rows && wOk; ++r) {
                        for (tIndex c = 0; c < wArray->m_Cols && wOk; ++c) {
                            wOk = NetworkDaysAddHoliday(wArray->At(r, c), oHolidays, &oError);
                        }
                    }
                }
            } else {
                // Resolve scalar without calling protected tFunction::StackElemToVariant
                // (this helper lives in an anonymous namespace, not a tFunction member).
                tVariant wVal;
                tBool wResolved = false;
                switch (sHolArg.Type()) {
                    case tStackType::t_Variant:
                        wVal = sHolArg.Variant();
                        wResolved = true;
                        break;
                    case tStackType::t_Cell: {
                        tCell* wCell = sHolArg.Cell();
                        if (wCell != nullptr) {
                            wVal = wCell->CalculableValue();
                            wResolved = true;
                        }
                        break;
                    }
                    default:
                        break;
                }
                if (wResolved) {
                    wOk = NetworkDaysAddHoliday(wVal, oHolidays, &oError);
                }
            }
            return wOk;
        }

        // oWeekend[DayWeek()] where DayWeek 0=Sun .. 6=Sat. true = weekend (non-working).
        static void WeekendMaskSatSun(tBool oWeekend[7]) {
            for (int i = 0; i < 7; ++i) oWeekend[i] = false;
            oWeekend[0] = true;
            oWeekend[6] = true;
        }

        static tBool ParseWeekendMask(const tVariant& sArg, tBool oWeekend[7]) {
            for (int i = 0; i < 7; ++i) oWeekend[i] = false;
            if (sArg.IsString()) {
                const tString& wS = sArg.String();
                if (wS.size() != 7) return false;
                // Excel string: position 0=Mon .. 6=Sun; map to DayWeek 1..6,0.
                for (tSize i = 0; i < 7; ++i) {
                    const char wCh = wS[i];
                    if (wCh != '0' && wCh != '1') return false;
                    const tInt wDow = static_cast<tInt>((i + 1) % 7); // Mon->1 ... Sun->0
                    oWeekend[wDow] = (wCh == '1');
                }
                return true;
            }
            tInt wCode = 0;
            if (sArg.IsInt()) wCode = sArg.Int();
            else if (sArg.IsDouble()) wCode = static_cast<tInt>(std::trunc(sArg.Double()));
            else return false;
            // Pair weekends (1-7) and single-day weekends (11-17).
            static const tInt kPairs[8][2] = {
                {0, 0}, {6, 0}, {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}
            };
            if (wCode >= 1 && wCode <= 7) {
                oWeekend[kPairs[wCode][0]] = true;
                oWeekend[kPairs[wCode][1]] = true;
                return true;
            }
            if (wCode >= 11 && wCode <= 17) {
                // 11=Sun, 12=Mon, ..., 17=Sat
                const tInt wDow = (wCode == 11) ? 0 : (wCode - 11);
                oWeekend[wDow] = true;
                return true;
            }
            return false;
        }

        static tInt CountNetworkDays(tClassDate sStart, tClassDate sEnd,
                                     const tBool sWeekend[7], const std::set<tInt>& sHolidays) {
            tInt wSign = 1;
            if (NetworkDaysDayKey(sStart) > NetworkDaysDayKey(sEnd)) {
                tClassDate wTmp = sStart;
                sStart = sEnd;
                sEnd = wTmp;
                wSign = -1;
            }
            tInt wCount = 0;
            tClassDate wCursor = sStart;
            const tInt wEndKey = NetworkDaysDayKey(sEnd);
            for (tInt wGuard = 0; wGuard < 1000000; ++wGuard) {
                const tInt wKey = NetworkDaysDayKey(wCursor);
                const tInt wDow = wCursor.DayWeek();
                if (!sWeekend[wDow] && sHolidays.find(wKey) == sHolidays.end()) {
                    ++wCount;
                }
                if (wKey >= wEndKey) break;
                wCursor = wCursor + 1;
            }
            return wSign * wCount;
        }

        static tInt DaysInMonth(tInt sY, tInt sM) {
            static const tInt kDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (sM == 2) {
                const tBool wLeap = ((sY % 4 == 0) && (sY % 100 != 0)) || (sY % 400 == 0);
                return wLeap ? 29 : 28;
            }
            if (sM < 1 || sM > 12) return 30;
            return kDays[sM];
        }

        // Excel DAYS360 US (NASD) / European day counts (not divided by 360).
        static tInt Days360Count(tClassDate& sStart, tClassDate& sEnd, tBool sEuropean) {
            tInt y1, m1, d1, y2, m2, d2;
            sStart.YearMonthDay(y1, m1, d1);
            sEnd.YearMonthDay(y2, m2, d2);
            if (sEuropean) {
                if (d1 == 31) d1 = 30;
                if (d2 == 31) d2 = 30;
            } else {
                // US (NASD): last day of month → 30 for start; end has special next-month rule.
                if (d1 == DaysInMonth(y1, m1)) d1 = 30;
                if (d2 == DaysInMonth(y2, m2)) {
                    if (d1 < 30) {
                        d2 = 1;
                        ++m2;
                        if (m2 > 12) {
                            m2 = 1;
                            ++y2;
                        }
                    } else {
                        d2 = 30;
                    }
                }
            }
            return (y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1);
        }

        // Advance by N working days using weekend mask + holidays (shared by WORKDAY / WORKDAY_INTL).
        static tBool AdvanceWorkDays(tClassDate sStart, tInt sDays,
                                     const tBool sWeekend[7], const std::set<tInt>& sHolidays,
                                     tClassDate& oOut) {
            auto wIsWorkDay = [&](tClassDate& sDate) -> tBool {
                const tInt wDow = sDate.DayWeek();
                if (sWeekend[wDow]) return false;
                return sHolidays.find(NetworkDaysDayKey(sDate)) == sHolidays.end();
            };
            if (sDays == 0) {
                oOut = sStart;
                return true;
            }
            tClassDate wCursor = sStart;
            const tInt wStep = (sDays > 0) ? 1 : -1;
            tInt wRemaining = (sDays > 0) ? sDays : -sDays;
            for (tInt wGuard = 0; wGuard < 1000000 && wRemaining > 0; ++wGuard) {
                wCursor = wCursor + wStep;
                if (wIsWorkDay(wCursor)) {
                    --wRemaining;
                }
            }
            if (wRemaining > 0) return false;
            oOut = wCursor;
            return true;
        }

        static tBool TimeValueFromText(const tString& sText, tDouble& oFrac) {
            tString wStr = sText;
            tSize wBegin = 0;
            while (wBegin < wStr.size() && std::isspace(static_cast<unsigned char>(wStr[wBegin]))) ++wBegin;
            tSize wEnd = wStr.size();
            while (wEnd > wBegin && std::isspace(static_cast<unsigned char>(wStr[wEnd - 1]))) --wEnd;
            wStr = wStr.substr(wBegin, wEnd - wBegin);
            if (wStr.empty()) return false;
            tInt wH = 0, wM = 0, wS = 0;
            const tChar* wP = wStr.c_str();
            tChar* wStop = nullptr;
            wH = static_cast<tInt>(std::strtol(wP, &wStop, 10));
            if (wStop == wP || *wStop != ':') return false;
            wP = wStop + 1;
            wM = static_cast<tInt>(std::strtol(wP, &wStop, 10));
            if (wStop == wP) return false;
            if (*wStop == ':') {
                wP = wStop + 1;
                wS = static_cast<tInt>(std::strtol(wP, &wStop, 10));
                if (wStop == wP) return false;
            }
            NormalizeExcelTimeToClock(wH, wM, wS);
            oFrac = ExcelTimeFractionFromClock(wH, wM, wS);
            return true;
        }

        // YEARFRAC basis helpers (Excel day-count conventions).
        static tDouble YearFracBasis0(tClassDate& sStart, tClassDate& sEnd) {
            // US (NASD) 30/360
            tInt y1, m1, d1, y2, m2, d2;
            sStart.YearMonthDay(y1, m1, d1);
            sEnd.YearMonthDay(y2, m2, d2);
            if (d1 == 31) d1 = 30;
            if (d2 == 31 && d1 == 30) d2 = 30;
            return ((y2 - y1) * 360.0 + (m2 - m1) * 30.0 + (d2 - d1)) / 360.0;
        }

        static tDouble YearFracBasis4(tClassDate& sStart, tClassDate& sEnd) {
            // European 30/360
            tInt y1, m1, d1, y2, m2, d2;
            sStart.YearMonthDay(y1, m1, d1);
            sEnd.YearMonthDay(y2, m2, d2);
            if (d1 == 31) d1 = 30;
            if (d2 == 31) d2 = 30;
            return ((y2 - y1) * 360.0 + (m2 - m1) * 30.0 + (d2 - d1)) / 360.0;
        }

        static tBool IsLeapYear(tInt sY) {
            return ((sY % 4 == 0) && (sY % 100 != 0)) || (sY % 400 == 0);
        }

        static tDouble YearFracBasis1(tClassDate& sStart, tClassDate& sEnd) {
            // Actual/actual
            const tDouble wDays = ExcelSerialDay(sEnd) - ExcelSerialDay(sStart);
            tInt y1, m1, d1, y2, m2, d2;
            sStart.YearMonthDay(y1, m1, d1);
            sEnd.YearMonthDay(y2, m2, d2);
            if (y1 == y2) {
                const tDouble wYearLen = IsLeapYear(y1) ? 366.0 : 365.0;
                return wDays / wYearLen;
            }
            // Average year length weighted across spanned years (Excel-compatible approximation).
            tDouble wSum = 0.0;
            tInt wYears = 0;
            for (tInt y = y1; y <= y2; ++y) {
                wSum += IsLeapYear(y) ? 366.0 : 365.0;
                ++wYears;
            }
            return wDays / (wSum / static_cast<tDouble>(wYears));
        }
    } // namespace

    // DAYS ===================================================================
    tFunctionDays::tFunctionDays() : tFunction() {}

    tStackElem tFunctionDays::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DAYS requires 2 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());
        tVariant wEndVar, wStartVar;
        if (!StackElemToVariant(wArgs[0], wEndVar) || !StackElemToVariant(wArgs[1], wStartVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wEndVar.IsError()) return(tStackElem(wEndVar));
        if (wStartVar.IsError()) return(tStackElem(wStartVar));
        tClassDate wEnd, wStart;
        if (!NetworkDaysVariantToDate(wEndVar, wEnd) || !NetworkDaysVariantToDate(wStartVar, wStart)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        const tInt wDays = static_cast<tInt>(ExcelSerialDay(wEnd) - ExcelSerialDay(wStart));
        return(tStackElem(tVariant(wDays)));
    }

    tFunctionSpillKind tFunctionDays::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    // DAYS360 ================================================================
    tFunctionDays360::tFunctionDays360() : tFunction() {}

    tStackElem tFunctionDays360::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DAYS360 requires 2 or 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());
        tVariant wStartVar, wEndVar;
        if (!StackElemToVariant(wArgs[0], wStartVar) || !StackElemToVariant(wArgs[1], wEndVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wStartVar.IsError()) return(tStackElem(wStartVar));
        if (wEndVar.IsError()) return(tStackElem(wEndVar));
        tClassDate wStart, wEnd;
        if (!NetworkDaysVariantToDate(wStartVar, wStart) || !NetworkDaysVariantToDate(wEndVar, wEnd)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tBool wEuropean = false;
        if (wArgs.size() == 3) {
            tVariant wMethodVar;
            if (!StackElemToVariant(wArgs[2], wMethodVar)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (wMethodVar.IsError()) return(tStackElem(wMethodVar));
            // Blank method keeps US (NASD) default.
            if (!(wMethodVar.IsNull() || (wMethodVar.IsString() && wMethodVar.String().empty()))) {
                if (!StackElemToBool(wArgs[2], wEuropean)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                }
            }
        }
        return(tStackElem(tVariant(Days360Count(wStart, wEnd, wEuropean))));
    }

    tFunctionSpillKind tFunctionDays360::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    // ISOWEEKNUM =============================================================
    tFunctionIsoWeekNum::tFunctionIsoWeekNum() : tFunctionDate() {}

    tStackElem tFunctionIsoWeekNum::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wArg.IsError()) return(tStackElem(wArg));
        tClassDate wDate;
        if (!NetworkDaysVariantToDate(wArg, wDate)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        return(tStackElem(tVariant(wDate.WeekNum())));
    }

    // TIMEVALUE ==============================================================
    tFunctionTimeValue::tFunctionTimeValue() : tFunctionDate() {}

    tStackElem tFunctionTimeValue::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TIMEVALUE requires 1 argument"))));
        }
        tVariant wArg;
        if (!StackElemToVariant(wArgs[0], wArg)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
        }
        if (wArg.IsError()) return(tStackElem(wArg));
        tInt wH = 0, wM = 0, wS = 0;
        if (ClockFromExcelStyleSerial(wArg, wH, wM, wS)) {
            return(tStackElem(tVariant(ExcelTimeFractionFromClock(wH, wM, wS))));
        }
        if (wArg.IsString()) {
            tDouble wFrac = 0.0;
            if (TimeValueFromText(wArg.String(), wFrac)) {
                return(tStackElem(tVariant(wFrac)));
            }
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "TIMEVALUE: cannot parse time"))));
    }

    // DATEDIF ================================================================
    tFunctionDateDif::tFunctionDateDif() : tFunction() {}

    tStackElem tFunctionDateDif::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DATEDIF requires 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());
        tVariant wStartVar, wEndVar, wUnitVar;
        if (!StackElemToVariant(wArgs[0], wStartVar) || !StackElemToVariant(wArgs[1], wEndVar) ||
            !StackElemToVariant(wArgs[2], wUnitVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wStartVar.IsError()) return(tStackElem(wStartVar));
        if (wEndVar.IsError()) return(tStackElem(wEndVar));
        if (wUnitVar.IsError()) return(tStackElem(wUnitVar));
        tClassDate wStart, wEnd;
        if (!NetworkDaysVariantToDate(wStartVar, wStart) || !NetworkDaysVariantToDate(wEndVar, wEnd)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (ExcelSerialDay(wEnd) < ExcelSerialDay(wStart)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tString wUnit;
        if (wUnitVar.IsString()) wUnit = wUnitVar.String();
        else return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        for (char& ch : wUnit) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

        tInt y1, m1, d1, y2, m2, d2;
        wStart.YearMonthDay(y1, m1, d1);
        wEnd.YearMonthDay(y2, m2, d2);

        if (wUnit == "Y") {
            tInt wYears = y2 - y1;
            if (m2 < m1 || (m2 == m1 && d2 < d1)) --wYears;
            return(tStackElem(tVariant(wYears < 0 ? 0 : wYears)));
        }
        if (wUnit == "M") {
            tInt wMonths = (y2 - y1) * 12 + (m2 - m1);
            if (d2 < d1) --wMonths;
            return(tStackElem(tVariant(wMonths < 0 ? 0 : wMonths)));
        }
        if (wUnit == "D") {
            return(tStackElem(tVariant(static_cast<tInt>(ExcelSerialDay(wEnd) - ExcelSerialDay(wStart)))));
        }
        if (wUnit == "MD") {
            // Days remaining after complete months (Excel quirk with month-end).
            tInt wDay = d2 - d1;
            if (wDay < 0) {
                tInt wPrevM = m2 - 1;
                tInt wPrevY = y2;
                if (wPrevM < 1) { wPrevM = 12; --wPrevY; }
                wDay += DaysInMonth(wPrevY, wPrevM);
            }
            return(tStackElem(tVariant(wDay)));
        }
        if (wUnit == "YM") {
            tInt wMonths = m2 - m1;
            if (d2 < d1) --wMonths;
            if (wMonths < 0) wMonths += 12;
            return(tStackElem(tVariant(wMonths)));
        }
        if (wUnit == "YD") {
            // Days ignoring years: compare same-year anniversary.
            tClassDate wAdjStart(y2, m1, d1);
            if (ExcelSerialDay(wAdjStart) > ExcelSerialDay(wEnd)) {
                wAdjStart = tClassDate(y2 - 1, m1, d1);
            }
            return(tStackElem(tVariant(static_cast<tInt>(ExcelSerialDay(wEnd) - ExcelSerialDay(wAdjStart)))));
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "DATEDIF: invalid unit"))));
    }

    tFunctionSpillKind tFunctionDateDif::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    // YEARFRAC ===============================================================
    tFunctionYearFrac::tFunctionYearFrac() : tFunction() {}

    tStackElem tFunctionYearFrac::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "YEARFRAC requires 2 or 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());
        tVariant wStartVar, wEndVar;
        if (!StackElemToVariant(wArgs[0], wStartVar) || !StackElemToVariant(wArgs[1], wEndVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wStartVar.IsError()) return(tStackElem(wStartVar));
        if (wEndVar.IsError()) return(tStackElem(wEndVar));
        tClassDate wStart, wEnd;
        if (!NetworkDaysVariantToDate(wStartVar, wStart) || !NetworkDaysVariantToDate(wEndVar, wEnd)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tInt wBasis = 0;
        if (wArgs.size() == 3) {
            if (!StackElemToInt(wArgs[2], wBasis) || wBasis < 0 || wBasis > 4) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
            }
        }
        if (ExcelSerialDay(wEnd) < ExcelSerialDay(wStart)) {
            tClassDate wTmp = wStart;
            wStart = wEnd;
            wEnd = wTmp;
        }
        tDouble wFrac = 0.0;
        switch (wBasis) {
            case 0: wFrac = YearFracBasis0(wStart, wEnd); break;
            case 1: wFrac = YearFracBasis1(wStart, wEnd); break;
            case 2: // actual/360
                wFrac = (ExcelSerialDay(wEnd) - ExcelSerialDay(wStart)) / 360.0;
                break;
            case 3: // actual/365
                wFrac = (ExcelSerialDay(wEnd) - ExcelSerialDay(wStart)) / 365.0;
                break;
            case 4: wFrac = YearFracBasis4(wStart, wEnd); break;
            default: return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        return(tStackElem(tVariant(wFrac)));
    }

    tFunctionSpillKind tFunctionYearFrac::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    // NETWORKDAYS / NETWORKDAYS.INTL =========================================
    tFunctionNetworkDays::tFunctionNetworkDays() : tFunction() {}

    tStackElem tFunctionNetworkDays::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // NETWORKDAYS(start_date, end_date, [holidays])
        if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NETWORKDAYS requires 2 or 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wStartVar;
        tVariant wEndVar;
        if (!StackElemToVariant(wArgs[0], wStartVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NETWORKDAYS: invalid start_date"))));
        }
        if (wStartVar.IsError()) {
            return(tStackElem(wStartVar));
        }
        if (!StackElemToVariant(wArgs[1], wEndVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NETWORKDAYS: invalid end_date"))));
        }
        if (wEndVar.IsError()) {
            return(tStackElem(wEndVar));
        }

        tClassDate wStart;
        tClassDate wEnd;
        if (!NetworkDaysVariantToDate(wStartVar, wStart) || !NetworkDaysVariantToDate(wEndVar, wEnd)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NETWORKDAYS: dates must be date or serial"))));
        }

        std::set<tInt> wHolidays;
        if (wArgs.size() == 3) {
            tVariant wHolidayErr;
            if (!CollectHolidays(wArgs[2], wHolidays, wHolidayErr)) {
                return(tStackElem(wHolidayErr));
            }
        }

        tBool wWeekend[7];
        WeekendMaskSatSun(wWeekend);
        return(tStackElem(tVariant(CountNetworkDays(wStart, wEnd, wWeekend, wHolidays))));
    }

    tFunctionSpillKind tFunctionNetworkDays::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    tFunctionNetworkDaysIntl::tFunctionNetworkDaysIntl() : tFunction() {}

    tStackElem tFunctionNetworkDaysIntl::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // NETWORKDAYS_INTL(start, end, [weekend], [holidays])
        if (wArgs.size() < 2 || wArgs.size() > 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NETWORKDAYS_INTL requires 2 to 4 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wStartVar, wEndVar;
        if (!StackElemToVariant(wArgs[0], wStartVar) || !StackElemToVariant(wArgs[1], wEndVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wStartVar.IsError()) return(tStackElem(wStartVar));
        if (wEndVar.IsError()) return(tStackElem(wEndVar));
        tClassDate wStart, wEnd;
        if (!NetworkDaysVariantToDate(wStartVar, wStart) || !NetworkDaysVariantToDate(wEndVar, wEnd)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }

        tBool wWeekend[7];
        WeekendMaskSatSun(wWeekend);
        if (wArgs.size() >= 3) {
            tVariant wWeekVar;
            if (!StackElemToVariant(wArgs[2], wWeekVar)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            if (wWeekVar.IsError()) return(tStackElem(wWeekVar));
            // Blank weekend arg keeps default Sat/Sun.
            if (!(wWeekVar.IsNull() || (wWeekVar.IsString() && wWeekVar.String().empty()))) {
                if (!ParseWeekendMask(wWeekVar, wWeekend)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
                }
            }
        }

        std::set<tInt> wHolidays;
        if (wArgs.size() >= 4) {
            tVariant wHolidayErr;
            if (!CollectHolidays(wArgs[3], wHolidays, wHolidayErr)) {
                return(tStackElem(wHolidayErr));
            }
        }
        return(tStackElem(tVariant(CountNetworkDays(wStart, wEnd, wWeekend, wHolidays))));
    }

    tFunctionSpillKind tFunctionNetworkDaysIntl::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

    tFunctionWorkDay::tFunctionWorkDay(tBool sIntl) : tFunction(), m_Intl(sIntl) {}

    tStackElem tFunctionWorkDay::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        // WORKDAY(start, days, [holidays])
        // WORKDAY_INTL(start, days, [weekend], [holidays])
        if (m_Intl) {
            if (wArgs.size() < 2 || wArgs.size() > 4) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "WORKDAY_INTL requires 2 to 4 arguments"))));
            }
        } else if (wArgs.size() != 2 && wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "WORKDAY requires 2 or 3 arguments"))));
        }
        std::reverse(wArgs.begin(), wArgs.end());

        tVariant wStartVar;
        if (!StackElemToVariant(wArgs[0], wStartVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "WORKDAY: invalid start_date"))));
        }
        if (wStartVar.IsError()) {
            return(tStackElem(wStartVar));
        }
        tInt wDays = 0;
        if (!StackElemToInt(wArgs[1], wDays)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "WORKDAY: days must be numeric"))));
        }

        tClassDate wStart;
        if (!NetworkDaysVariantToDate(wStartVar, wStart)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "WORKDAY: start_date must be date or serial"))));
        }

        tBool wWeekend[7];
        WeekendMaskSatSun(wWeekend);
        std::set<tInt> wHolidays;

        if (m_Intl) {
            if (wArgs.size() >= 3) {
                tVariant wWeekVar;
                if (!StackElemToVariant(wArgs[2], wWeekVar)) {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
                }
                if (wWeekVar.IsError()) return(tStackElem(wWeekVar));
                // Blank weekend arg keeps default Sat/Sun.
                if (!(wWeekVar.IsNull() || (wWeekVar.IsString() && wWeekVar.String().empty()))) {
                    if (!ParseWeekendMask(wWeekVar, wWeekend)) {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
                    }
                }
            }
            if (wArgs.size() >= 4) {
                tVariant wHolidayErr;
                if (!CollectHolidays(wArgs[3], wHolidays, wHolidayErr)) {
                    return(tStackElem(wHolidayErr));
                }
            }
        } else if (wArgs.size() == 3) {
            tVariant wHolidayErr;
            if (!CollectHolidays(wArgs[2], wHolidays, wHolidayErr)) {
                return(tStackElem(wHolidayErr));
            }
        }

        tClassDate wResult;
        if (!AdvanceWorkDays(wStart, wDays, wWeekend, wHolidays, wResult)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_num, ""))));
        }
        tVariant wOut;
        wOut.SetDate(wResult.Value());
        return(tStackElem(wOut));
    }

    tFunctionSpillKind tFunctionWorkDay::SpillKind() const { return(tFunctionSpillKind::Aggregate); }

}; // end of namespace
