//=============================================================================
// Skeema TypesClass
//  Class for types ...
//   We use the concept of Boxing and Unboxing from C #
//   This makes it possible to have the methods specific to the types
//=============================================================================
#if defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "../include/SkTypesClass.hpp"
#include "../include/SkApplication.hpp"
#include "../include/SkLexer.hpp"
#include "../include/SkVariant.hpp"
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <math.h>       /* pow */

#define _DEBUGSKformatstring
#define _DEBUGSKdate
namespace SkRoot {
   
    // Format String ==========================================================
    template <typename T>
    tClassFormatString<T>::tClassFormatString(T sValue,tFormatString* const sFormatString) : tClass(), m_Value(sValue),m_FormatString(sFormatString) {
    }
    template <typename T>
    T tClassFormatString<T>::Round() {
        const tDouble wScale = std::pow(10.0, static_cast<tDouble>(m_FormatString->Decimal()));
        return static_cast<T>(std::round(static_cast<tDouble>(m_Value) * wScale) / wScale);
    }

    template <typename T>
    tString tClassFormatString<T>::Format() {
        tStringStream wStream;
        tFormatStringType wFormatType = m_FormatString->FormatType();
        
        switch (wFormatType) {
            case tFormatStringType::numeric :
            case tFormatStringType::numeric0 :
            case tFormatStringType::numeric_ :
            case tFormatStringType::numeric0_ :
            case tFormatStringType::numeric0_P : {
                // Thousand separator ========================================
                tByte wSep=-1;
                // Not Separator
                if ((wFormatType==tFormatStringType::numeric) ||
                    (wFormatType==tFormatStringType::numeric0)) {
                    wSep=0;
                }
                tBool wNeg=false;
                tString wValue;
                if (m_Value<0) {
                    wNeg=true;
                    wValue=to_string(Round()*-1);
                } else {
                    wValue=to_string(Round());
                }
                // Thousand separator ========================================
                tString wResult=FormatStr(wValue,wSep);
                // Parenthesis ===============================================
                if ((wFormatType==tFormatStringType::accountingP)  ||
                    (wFormatType==tFormatStringType::accounting0P) ||
                    (wFormatType==tFormatStringType::numeric0_P)) {
                    if (wNeg) wResult='('+wResult+')';
                } else {
                    if (wNeg) wResult='-'+wResult;
                }
                
                return(wResult);
                break;
            }
        
            case tFormatStringType::percent :
            case tFormatStringType::percent0 : {
                // Excel rounds the displayed percentage to the format's
                // declared precision (round-half-away-from-zero), so 16.73 %
                // shown with "0%" must read "17%" — not "16%".
                // The previous implementation produced "16%" because
                // std::to_string always emits 6 decimals (e.g. "16.730000")
                // and FormatStr() truncates the trailing digits instead of
                // rounding them. We now round explicitly with std::round
                // (matching Excel's rounding mode) and emit exactly Decimal()
                // decimals via std::fixed/setprecision before handing the
                // string off to FormatStr() for grouping/locale handling.
                const tInt wDecimal = static_cast<tInt>(m_FormatString->Decimal());
                const tDouble wScale = std::pow(10.0, wDecimal);
                const tDouble wValue = std::round(tDouble(m_Value) * 100.0 * wScale) / wScale;
                tStringStream wSs;
                wSs << std::fixed << std::setprecision(wDecimal) << wValue;
                return(FormatStr(wSs.str(), 0) + "%");
                break;
            }
           
            case tFormatStringType::accounting :  //  Separator
            case tFormatStringType::accountingP :
            case tFormatStringType::accounting0 : // Decimal & Separator
            case tFormatStringType::accounting0P :  // Decimal, Separator &Parenthesis
            {
                tBool wNeg=false;
                tString wValue;
                if (m_Value<0) {
                    wNeg=true;
                    wValue=to_string(Round()*-1);
                } else {
                    wValue=to_string(Round());
                }
                // Thousand separator ========================================
                tString wResult=FormatStr(wValue,-1);
                
                // Parenthesis ===============================================
                if ((wFormatType==tFormatStringType::accountingP)  ||
                    (wFormatType==tFormatStringType::accounting0P)) {
                    if (wNeg) wResult='('+wResult+')';
                } else {
                    if (wNeg) wResult='-'+wResult;
                }
                return (wResult);
                break;
            }
            case tFormatStringType::scientific:
            case tFormatStringType::scientific1:
            case tFormatStringType::scientific2: {
                // Pre-round the mantissa using round-half-away-from-zero to
                // match Excel's behavior. std::setprecision on its own would
                // rely on libstdc++/libc++ banker's rounding which mismatches
                // Excel for ".5" boundary values (e.g., 2.5e0 -> "3E+00").
                const tInt wDecimal = static_cast<tInt>(m_FormatString->Decimal());
                tDouble wValue = tDouble(m_Value);
                if (wValue != 0.0) {
                    const tInt wOrder = static_cast<tInt>(std::floor(std::log10(std::fabs(wValue))));
                    const tDouble wScale = std::pow(10.0, wDecimal - wOrder);
                    wValue = std::round(wValue * wScale) / wScale;
                }
                wStream << std::scientific << std::setprecision(wDecimal) << wValue;
                tLocale* wLocale=tApplication::Instance()->Locale();
                const tChar wDecimalSeparator=wLocale->Decimal();
                if (wDecimalSeparator!='.') {
                    tString wResult=wStream.str();
                    std::replace(wResult.begin(), wResult.end(), '.', wDecimalSeparator); // replace all '.' to locale decimal
                    return(wResult);
                }
                break;
            }
            case tFormatStringType::excelnumber: {
                tNumberFormatter* wNumberFormatter=tApplication::Instance()->NumberFormatter();
                tString wExcelFormat = m_FormatString->FormatExcel();
                return(wNumberFormatter->FormatNumber(m_Value, wExcelFormat));
                break;
            }
            default: {
                tString wTmp = to_string(m_Value);
                return(FormatStr(wTmp,-1));
                break;
            }
        }
        
        return(wStream.str());
        
    };

    // FormatStr method with tFormatString parameter
    template <typename T>
    tString tClassFormatString<T>::FormatStr() {
        tFormatStringType wFormatType = m_FormatString->FormatType();
        
        switch (wFormatType) {
            case tFormatStringType::excelnumber: {
                tNumberFormatter* wNumberFormatter=tApplication::Instance()->NumberFormatter();
                tString wExcelFormat = m_FormatString->FormatExcel();
                return(wNumberFormatter->FormatNumber(m_Value, wExcelFormat));
                break;
            }
            default: {
                return(Format(m_FormatString));
                break;
            }
        }
    };

    template <typename T>
    tString tClassFormatString<T>::FormatStr(tString sValue,tByte sGrouping) {
        // For excelnumber type, we need the Excel format string
        tLocale* wLocale=tApplication::Instance()->Locale();
        const tShort wNbDecimal = m_FormatString->Decimal();
    
        const tChar wThousandSeparator=wLocale->Thousand();
        const tChar wDecimalSeparator=wLocale->Decimal();
     
        // Normalize grouping to a signed int to avoid unsigned wrap/underflow
        tInt wGrouping = (sGrouping == tByte(-1)) ? static_cast<tInt>(wLocale->Grouping()[0])
                                                  : static_cast<tInt>(sGrouping);
        tString wResult;
        if (wNbDecimal!=0) wResult=wDecimalSeparator;
        
        tSize  wPosPoint = sValue.find(".");
        if (wPosPoint!=std::string::npos) {
            if (wNbDecimal!=0) {
                while(sValue.length()<wPosPoint+1+wNbDecimal) sValue+='0';
                wResult+=sValue.substr(wPosPoint+1,wNbDecimal);
            }
            wPosPoint--;
        } else {
            if (wNbDecimal!=0){
                while(wResult.length()<=wNbDecimal)
                    wResult.push_back('0');
            }
            wPosPoint=sValue.length()-1;
        }

        tInt wPos = static_cast<tInt>(wPosPoint);
        tInt wNbGroup = 0;
        while(wPos>=0) {
            const tChar wChar = sValue[static_cast<size_t>(wPos)];
            wResult.insert(0,1,wChar);
            wNbGroup++;
            if ((wPos!=0) && (wGrouping>0)) {
                if (wNbGroup==wGrouping) {
                    wResult.insert(0,1,wThousandSeparator);
                    wNbGroup=0;
                }
            }
            wPos--;
        }
#ifdef debugformatstring
        
        cout << " tClassFormatString<T>::FormatStr()" << "=" << wResult << endl;
#endif
        return(wResult);
    }

	// tClassInt ======================================================================
	tClassInt::tClassInt() : tClass() { m_Value = 0; }
	tClassInt::tClassInt(tInt sValue) : tClass() { m_Value = sValue; }
	tClassInt::tClassInt(const tClassInt& sClassInt) : tClass(sClassInt)  { m_Value = sClassInt.m_Value; }

    tString tClassInt::FormatString(tFormatString* const sFormatString) {
        tClassFormatString<tInt> wFormatString(m_Value,sFormatString);
        return(wFormatString.Format());
    }

	tBool tClassInt::operator==(const tClassInt& sClassInt) { return(m_Value == sClassInt.m_Value); }

	tBool tClassInt::operator!=(const tClassInt& sClassInt) { return(m_Value != sClassInt.m_Value); }

	tBool tClassInt::operator < (const tClassInt& sClassInt) { return(m_Value < sClassInt.m_Value); }
	tBool tClassInt::operator > (const tClassInt& sClassInt) { return(m_Value > sClassInt.m_Value); }
	tBool tClassInt::operator <= (const tClassInt& sClassInt) { return(m_Value <= sClassInt.m_Value); }
	tBool tClassInt::operator >= (const tClassInt& sClassInt) { return(m_Value >= sClassInt.m_Value); }

	tClassInt tClassInt::operator --(tInt sInt) { m_Value-=sInt; return(*this); }
	tClassInt tClassInt::operator ++(tInt sInt) { m_Value+=sInt; return(*this); }

	tClassInt& tClassInt::operator --() { m_Value--; return(*this); }
	tClassInt& tClassInt::operator ++() { m_Value++; return(*this); }

	tClassInt operator+(const tClassInt& sClassInt1, const tClassInt& sClassInt2) {
		return(tClassInt(sClassInt1.m_Value + sClassInt2.m_Value));
	}
	tClassInt operator-(const tClassInt& sClassInt1, const tClassInt& sClassInt2) {
		return(tClassInt(sClassInt1.m_Value - sClassInt2.m_Value));
	}
	tClassInt operator*(const tClassInt& sClassInt1, const tClassInt& sClassInt2) {
		return(tClassInt(sClassInt1.m_Value * sClassInt2.m_Value));
	}
	tClassInt operator/(const tClassInt& sClassInt1, const tClassInt& sClassInt2) {
		return(tClassInt(sClassInt1.m_Value / sClassInt2.m_Value));
	}


	tInt tClassInt::operator()() { return(m_Value); }

	tString tClassInt::Str() {
		tStringStream wStream;
		wStream << m_Value;
		return(wStream.str());
	}

	ostream& operator<<(ostream& os, const tClassInt&  sClassInt)
	{
		os << sClassInt.m_Value;
		return os;
	}

	// tClassFloat ======================================================================
	tClassFloat::tClassFloat() : tClass() { m_Value = 0; }
	tClassFloat::tClassFloat(tFloat sValue) : tClass() { m_Value = sValue; }
	tClassFloat::tClassFloat(const tClassFloat& sClassFloat) : tClass(sClassFloat)  { m_Value = sClassFloat.m_Value; }

    tString tClassFloat::FormatString(tFormatString* const sFormatString) {
        tClassFormatString<tFloat> wFormatString(m_Value,sFormatString);
        return(wFormatString.Format());
    }
 
	tClassFloat tClassFloat::operator = (const tString& sString) { return(tClassFloat(static_cast<tFloat>(std::atof(sString.c_str())))); }

	tBool tClassFloat::operator!=(const tClassFloat& sClassFloat) { return(m_Value != sClassFloat.m_Value); }

	tBool tClassFloat::operator < (const tClassFloat& sClassFloat) { return(m_Value < sClassFloat.m_Value); }
	tBool tClassFloat::operator > (const tClassFloat& sClassFloat) { return(m_Value > sClassFloat.m_Value); }
	tBool tClassFloat::operator <= (const tClassFloat& sClassFloat) { return(m_Value <= sClassFloat.m_Value); }
	tBool tClassFloat::operator >= (const tClassFloat& sClassFloat) { return(m_Value >= sClassFloat.m_Value); }

	tClassFloat operator+(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2) {
		return(tClassFloat(sClassFloat1.m_Value + sClassFloat2.m_Value));
	}
	tClassFloat operator-(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2) {
		return(tClassFloat(sClassFloat1.m_Value - sClassFloat2.m_Value));
	}
	tClassFloat operator*(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2) {
		return(tClassFloat(sClassFloat1.m_Value * sClassFloat2.m_Value));
	}
	tClassFloat operator/(const tClassFloat& sClassFloat1, const tClassFloat& sClassFloat2) {
		return(tClassFloat(sClassFloat1.m_Value / sClassFloat2.m_Value));
	}


	tFloat tClassFloat::operator()() { return(m_Value); }

	tString tClassFloat::Str() {
		tStringStream wStream;
		wStream << m_Value;
		return(wStream.str());
	}

	ostream& operator<<(ostream& os, const tClassFloat& sClassFloat)
	{
		os << sClassFloat.m_Value;
		return os;
	}

	// tClassDouble ======================================================================
	tClassDouble::tClassDouble() : tClass() { m_Value = 0; }
	tClassDouble::tClassDouble(tDouble sValue) : tClass() { m_Value = sValue; }
	tClassDouble::tClassDouble(const tClassDouble& sClassDouble) : tClass(sClassDouble)  { m_Value = sClassDouble.m_Value; }

    tString tClassDouble::FormatString(tFormatString* const sFormatString) {
        tClassFormatString<tDouble> wFormatString(m_Value,sFormatString);
        return(wFormatString.Format());
    }

	tClassDouble tClassDouble::operator = (const tString& sString) { return(tClassDouble(std::stod(sString.c_str())));  }

	tBool tClassDouble::operator!=(const tClassDouble& sClassDouble) { return(m_Value != sClassDouble.m_Value); }

	tBool tClassDouble::operator < (const tClassDouble& sClassDouble) { return(m_Value < sClassDouble.m_Value); }
	tBool tClassDouble::operator > (const tClassDouble& sClassDouble) { return(m_Value > sClassDouble.m_Value); }
	tBool tClassDouble::operator <= (const tClassDouble& sClassDouble) { return(m_Value <= sClassDouble.m_Value); }
	tBool tClassDouble::operator >= (const tClassDouble& sClassDouble) { return(m_Value >= sClassDouble.m_Value); }

	tClassDouble operator+(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2) {
		return(tClassDouble(sClassDouble1.m_Value + sClassDouble2.m_Value));
	}
	tClassDouble operator-(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2) {
		return(tClassDouble(sClassDouble1.m_Value - sClassDouble2.m_Value));
	}
	tClassDouble operator*(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2) {
		return(tClassDouble(sClassDouble1.m_Value * sClassDouble2.m_Value));
	}
	tClassDouble operator/(const tClassDouble& sClassDouble1, const tClassDouble& sClassDouble2) {
		return(tClassDouble(sClassDouble1.m_Value / sClassDouble2.m_Value));
	}


	tDouble tClassDouble::operator()() { return(m_Value); }

	tString tClassDouble::Str() {
		tStringStream wStream;
		wStream << m_Value;
		return(wStream.str());
	}

	ostream& operator<<(ostream& os, const tClassDouble& sClassDouble)
	{
		os << sClassDouble.m_Value;
		return os;
	}


	// tClassString ======================================================================
	tClassString::tClassString() : tClass() { m_Value = ""; }

    tClassString::tClassString(const tClassString& sClassString) : tClass(sClassString) { m_Value = sClassString.m_Value; }

    tClassString::tClassString(tString sValue) : tClass() { m_Value = sValue; }

    tClassString::tClassString(tChar* sValue) {
        tSize wLength = strlen(sValue);
        m_Value.assign(sValue, sValue + wLength);
    }

    tClassString::tClassString(tInt sValue) {
		tStringStream wStream;
		wStream << sValue;
		m_Value = wStream.str();
	}

	tClassString::tClassString(tDouble sValue) {
		tStringStream wStream;
		wStream << sValue;
		m_Value = wStream.str();
	}

    tString tClassString::Left(tSize sLength) {
        return(m_Value.substr(0, sLength));
    }

    tString tClassString::Right(tSize sLength) {
        return(m_Value.substr(m_Value.length() - sLength));
    }

    tString tClassString::Mid(tSize sStart, tSize sLength) {
        return(m_Value.substr(sStart, sLength));
    }

    tSize tClassString::Length() {
        return(m_Value.length());
    }

    tSize tClassString::CountChar(tChar sChar) const {
        tSize wCount = 0;
        for (tChar c : m_Value) {
            if (c == sChar) ++wCount;
        }
        return wCount;
    }

	tString tClassString::Upper() {
		tString wResult=m_Value;
		std::transform(m_Value.begin(), m_Value.end(), wResult.begin(),
			[](unsigned char c) { return static_cast<tChar>(std::toupper(c)); });
		return(wResult);
	};

	tString tClassString::Lower() {
		tString wResult = m_Value;
		wResult.reserve(m_Value.length() + 1);
		std::transform(m_Value.begin(), m_Value.end(), wResult.begin(),
			[](unsigned char c) { return static_cast<tChar>(std::tolower(c)); });
		return(wResult);
	};

	tString tClassString::Ltrim() {
		tString wResult = m_Value;
		wResult.erase(wResult.begin(), std::find_if(wResult.begin(), wResult.end(),
			[](unsigned char ch) { return !std::isspace(ch); }));
		return(wResult);
	}

	tString tClassString::Rtrim() {
		tString wResult = m_Value;
		wResult.erase(std::find_if(wResult.rbegin(), wResult.rend(),
			[](unsigned char ch) { return !std::isspace(ch); }).base(), wResult.end());
		return(wResult);
	}
	tString tClassString::Trim() {
		tString wResult = m_Value;
		wResult.erase(wResult.begin(), std::find_if(wResult.begin(), wResult.end(),
			[](unsigned char ch) { return !std::isspace(ch); }));
		wResult.erase(std::find_if(wResult.rbegin(), wResult.rend(),
			[](unsigned char ch) { return !std::isspace(ch); }).base(), wResult.end());
		return(wResult);
	}

	tString tClassString::Unquote() {
		tString wResult = m_Value;
		if (wResult.empty()) return(wResult);
		
		// Remove quotes from the beginning
		while (!wResult.empty() && (wResult.front() == '\'' || wResult.front() == '"')) {
			wResult.erase(wResult.begin());
		}
		
		// Remove quotes from the end
		while (!wResult.empty() && (wResult.back() == '\'' || wResult.back() == '"')) {
			wResult.pop_back();
		}
		
		return(wResult);
	}

    tString tClassString::Proper() {
        tString wResult = m_Value;
        bool wCapitalizeNext = true;
        
        for (size_t i = 0; i < wResult.length(); i++) {
            const unsigned char wCh = static_cast<unsigned char>(wResult[i]);
            if (wCapitalizeNext && std::isalpha(wCh)) {
                wResult[i] = static_cast<tChar>(std::toupper(wCh));
                wCapitalizeNext = false;
            } else if (std::isalpha(wCh)) {
                wResult[i] = static_cast<tChar>(std::tolower(wCh));
            } else if (std::isspace(wCh) || wResult[i] == '-' || wResult[i] == '_') {
                wCapitalizeNext = true;
            }
        }
        
        return(wResult);
    }

	tVectorString tClassString::Split(tString sSeparator) {
		tVectorString wResult;
		size_t wPos = 0;
		tString wToken;
		tString wValue = m_Value;
		while ((wPos = wValue.find(sSeparator)) != std::string::npos) {
			wToken = wValue.substr(0, wPos);
			wResult.push_back(wToken);
			wValue.erase(0, wPos + sSeparator.length());
		}
		wResult.push_back(wValue);
		return(wResult);
	}

    size_t tClassString::Find(tString sValue) {
        return(m_Value.find(sValue,0));
    }


	void tClassString::Replace(const tString sFrom, const tString sTo) {
		if (sFrom.empty())
			return;
		size_t wStart_pos = 0;
		while ((wStart_pos = m_Value.find(sFrom, wStart_pos)) != std::string::npos) {
			m_Value.replace(wStart_pos, sFrom.length(), sTo);
			wStart_pos += sTo.length(); // In case 'sTo' contains 'sFrom', like replacing 'x' with 'yx'
		}
	}

	tBool tClassString::Regex_Match(tString sRegex, tBool sIgnoreCase) {
		try {
			const auto wFlags = sIgnoreCase
				? (std::regex::ECMAScript | std::regex::icase)
				: std::regex::ECMAScript;
			regex wRx(sRegex, wFlags);
			return(regex_match(m_Value, wRx));
		} catch (const std::regex_error&) {
			return(false);
		}
	}

	tBool tClassString::Regex_Search(tString sRegex, tBool sIgnoreCase) {
		try {
			const auto wFlags = sIgnoreCase
				? (std::regex::ECMAScript | std::regex::icase)
				: std::regex::ECMAScript;
			regex wRx(sRegex, wFlags);
			return(regex_search(m_Value, wRx));
		} catch (const std::regex_error&) {
			return(false);
		}
	}

	tBool tClassString::Regex_Replace(tString sRegex, tString sReplace, tBool sIgnoreCase) {
		try {
			const auto wFlags = sIgnoreCase
				? (std::regex::ECMAScript | std::regex::icase)
				: std::regex::ECMAScript;
			regex wRx(sRegex, wFlags);
			m_Value = regex_replace(m_Value, wRx, sReplace);
			return(true);
		} catch (const std::regex_error&) {
			return(false);
		}
	}

	tBool tClassString::Regex_ExtractFirst(tString sRegex, tString& oOut, tBool sIgnoreCase) {
		oOut.clear();
		try {
			const auto wFlags = sIgnoreCase
				? (std::regex::ECMAScript | std::regex::icase)
				: std::regex::ECMAScript;
			regex wRx(sRegex, wFlags);
			std::smatch wMatch;
			if (!regex_search(m_Value, wMatch, wRx) || wMatch.empty()) {
				return(false);
			}
			oOut = wMatch.str(0);
			return(true);
		} catch (const std::regex_error&) {
			return(false);
		}
	}

	tBool tClassString::IsUnsigned() {
		if (m_Value.empty()) return(false);
		for (tSize i = 0; i < m_Value.length(); i++) {
			if (m_Value[i] < '0' || m_Value[i] > '9') return(false);
		}
		return(true);
	};

	tBool tClassString::IsInteger() {
		if (m_Value.empty()) return(false);
		tBool wHasDigit = false;
		tBool wHasPlus = false;
		tBool wHasMinus = false;
		for (tSize i = 0; i < m_Value.length(); i++) {
			tChar wChar = m_Value[i];
			if (wChar >= '0' && wChar <= '9') {
				wHasDigit = true;
			} else if (wChar == '+') {
				if (i != 0 || wHasPlus || wHasMinus) return(false);
				wHasPlus = true;
			} else if (wChar == '-') {
				if (i != 0 || wHasPlus || wHasMinus) return(false);
				wHasMinus = true;
			} else {
				return(false);
			}
		}
		return(wHasDigit);
	};

	tBool tClassString::IsNumber() {
		// Accept scientific notation (e.g. 8.89E-2) like strtod/Excel OOXML; require full-string parse.
		// Size-bounded manual scan (no strtod): under Wasm, musl's strtod/__floatscan scans a raw
		// C-string and can read one byte past the buffer (AddressSanitizer flags a stack-buffer-overflow
		// on SSO strings, and it is undefined behavior everywhere). Iterating by index up to size()
		// guarantees we never read past m_Value's own bytes, even if the buffer is not NUL-terminated.
		const std::string& wStr = m_Value;
		const std::size_t wLen = wStr.size();
		std::size_t i = 0;
		while (i < wLen && std::isspace(static_cast<unsigned char>(wStr[i]))) i++;
		if (i >= wLen) return(false);
		if (wStr[i] == '+' || wStr[i] == '-') i++;
		bool wHasDigits = false;
		while (i < wLen && wStr[i] >= '0' && wStr[i] <= '9') { i++; wHasDigits = true; }
		if (i < wLen && wStr[i] == '.') {
			i++;
			while (i < wLen && wStr[i] >= '0' && wStr[i] <= '9') { i++; wHasDigits = true; }
		}
		if (!wHasDigits) return(false);
		if (i < wLen && (wStr[i] == 'e' || wStr[i] == 'E')) {
			i++;
			if (i < wLen && (wStr[i] == '+' || wStr[i] == '-')) i++;
			bool wExpDigits = false;
			while (i < wLen && wStr[i] >= '0' && wStr[i] <= '9') { i++; wExpDigits = true; }
			if (!wExpDigits) return(false);
		}
		while (i < wLen && std::isspace(static_cast<unsigned char>(wStr[i]))) i++;
		return(i == wLen);
	};


	tBool tClassString::StartsWith(tString sValue) {
		if (sValue == "") return(true);
		if (m_Value.length() < sValue.length()) return(false);
		return(m_Value.substr(0, sValue.length()) == sValue);
	}


	tBool tClassString::IsKeyCode() {
		return(Regex_Match("[A-Za-z_][A-Za-z_0-9]*"));
	}

	tBool tClassString::IsEmailAdress() {
		tString wRegexEmail = "[a-z0-9\\._%+!$&*=^|~#%'`?{}/\\-]+@([a-z0-9\\-]+\\.){1,}([a-z]{2,16})";
		return(Regex_Match(wRegexEmail));
	}
	tBool tClassString::IsUrl() {
		tString wRegexUrl = "((http\\://|https\\://|ftp\\://)|(www.))+(([a-zA-Z0-9\\.-]+\\.[a-zA-Z]{2,4})|([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}))(/[a-zA-Z0-9%:/-_\\?\\.'~]*)?(:[0-9]*)?";
		return(Regex_Match(wRegexUrl));
	}

	tInt tClassString::ToInt() {
		if (m_Value == "") return(0);
		// Avoid std::stoi throw under -fno-exceptions (libc++ forwards to std::stoll).
		char* end = nullptr;
		const char* start = m_Value.c_str();
		const long v = std::strtol(start, &end, 10);
		if (end == start) return 0;
		return static_cast<tInt>(v);
	}

	tDouble tClassString::ToDouble() {
		if (m_Value == "") return(0);
		// DataBar CF params / OOXML cfvo can leave non-numeric tokens (e.g. formula refs); stod aborts on Wasm.
		char* end = nullptr;
		const char* start = m_Value.c_str();
		const tDouble v = std::strtod(start, &end);
		if (end == start) return 0;
		return v;
	}

    tClassString& tClassString::operator = (const tChar* sValue) { m_Value=tString(sValue); return(*this); }

	tBool tClassString::operator==(const tClassString& sClassString) { return(m_Value == sClassString.m_Value); }

	tBool tClassString::operator!=(const tClassString& sClassString) { return(m_Value != sClassString.m_Value); }

	tBool tClassString::operator < (const tClassString& sClassString) { return(m_Value < sClassString.m_Value); }
	tBool tClassString::operator > (const tClassString& sClassString) { return(m_Value > sClassString.m_Value); }
	tBool tClassString::operator <= (const tClassString& sClassString) { return(m_Value <= sClassString.m_Value); }
	tBool tClassString::operator >= (const tClassString& sClassString) { return(m_Value >= sClassString.m_Value); }

	tClassString tClassString::operator +(const tClassString& sClassString) { return(tClassString(m_Value += sClassString.m_Value));  }

	tString tClassString::operator()() { return(m_Value); }

	ostream& operator<<(ostream& os, const tClassString& sClassString)
	{
		os << sClassString.m_Value;
		return os;
	}

   
    const tInt Cst_isdst=0;

    // Civil calendar helpers: Windows CRT localtime/mktime do not support dates
    // before 1970, but Excel's default date is 1900-01-01.
    namespace {
        long long DaysFromCivil(int sYear, unsigned sMonth, unsigned sDay) {
            sYear -= (sMonth <= 2) ? 1 : 0;
            const int wEra = (sYear >= 0 ? sYear : sYear - 399) / 400;
            const unsigned wYoe = static_cast<unsigned>(sYear - wEra * 400);
            const unsigned wDoy = (153U * (sMonth + (sMonth > 2 ? -3 : 9)) + 2) / 5 + sDay - 1;
            const unsigned wDoe = wYoe * 365 + wYoe / 4 - wYoe / 100 + wDoy;
            return wEra * 146097LL + static_cast<long long>(wDoe) - 719468;
        }

        void CivilFromDays(long long sDays, int& sYear, unsigned& sMonth, unsigned& sDay) {
            sDays += 719468;
            const long long wEra = (sDays >= 0 ? sDays : sDays - 146096) / 146097;
            const unsigned wDoe = static_cast<unsigned>(sDays - wEra * 146097);
            const unsigned wYoe = (wDoe - wDoe / 1460 + wDoe / 36524 - wDoe / 146097) / 365;
            sYear = static_cast<int>(wYoe) + static_cast<int>(wEra) * 400;
            const unsigned wDoy = wDoe - (365 * wYoe + wYoe / 4 - wYoe / 100);
            const unsigned wMp = (5 * wDoy + 2) / 153;
            sDay = wDoy - (153 * wMp + 2) / 5 + 1;
            sMonth = wMp < 10 ? wMp + 3 : wMp - 9;
            sYear += (sMonth <= 2) ? 1 : 0;
        }

        // Seconds to add to a UTC time_t to obtain local civil seconds (Cst_isdst=0).
        long long UtcToLocalShiftSeconds() {
#if defined(WIN32)
            // MSVC mktime() rejects local times that fall before 1970 UTC (e.g. CET
            // 1970-01-01 00:00), so the gmtime+mktime trick cannot measure the offset.
            long wTz = 0;
            if (_get_timezone(&wTz) == 0) {
                return static_cast<long long>(-wTz);
            }
            return 0;
#else
            const time_t wUtc = 0;
            struct tm wGmt = {};
            if (gmtime_r(&wUtc, &wGmt) == nullptr) {
                return 0;
            }
            wGmt.tm_isdst = Cst_isdst;
            const time_t wAsIfLocal = mktime(&wGmt);
            if (wAsIfLocal == static_cast<time_t>(-1)) {
                return 0;
            }
            return static_cast<long long>(difftime(wUtc, wAsIfLocal));
#endif
        }

        void UnixToTmLocal(tDate sUnix, struct tm& sTMStruct) {
            const long long wLocal = static_cast<long long>(sUnix) + UtcToLocalShiftSeconds();
            long long wDays = wLocal / 86400;
            long long wSod = wLocal % 86400;
            if (wSod < 0) {
                wSod += 86400;
                wDays -= 1;
            }
            int wYear = 1970;
            unsigned wMonth = 1;
            unsigned wDay = 1;
            CivilFromDays(wDays, wYear, wMonth, wDay);
            sTMStruct.tm_sec = static_cast<int>(wSod % 60);
            sTMStruct.tm_min = static_cast<int>((wSod / 60) % 60);
            sTMStruct.tm_hour = static_cast<int>(wSod / 3600);
            sTMStruct.tm_mday = static_cast<int>(wDay);
            sTMStruct.tm_mon = static_cast<int>(wMonth) - 1;
            sTMStruct.tm_year = wYear - 1900;
            sTMStruct.tm_wday = static_cast<int>(((wDays + 4) % 7 + 7) % 7);
            sTMStruct.tm_yday = 0;
            sTMStruct.tm_isdst = Cst_isdst;
        }

        tDate TmLocalToUnix(const struct tm& sTMStruct) {
            const int wYear = sTMStruct.tm_year + 1900;
            const unsigned wMonth = static_cast<unsigned>(sTMStruct.tm_mon + 1);
            const unsigned wDay = static_cast<unsigned>(sTMStruct.tm_mday);
            const long long wLocal =
                DaysFromCivil(wYear, wMonth, wDay) * 86400LL +
                static_cast<long long>(sTMStruct.tm_hour) * 3600LL +
                static_cast<long long>(sTMStruct.tm_min) * 60LL +
                static_cast<long long>(sTMStruct.tm_sec);
            return static_cast<tDate>(wLocal - UtcToLocalShiftSeconds());
        }
    }

	// tClassDate ======================================================================
    tClassDate::tClassDate() : tClass(), m_Value() { Clear(); }
	tClassDate::tClassDate(tDate sValue) : tClass() { m_Value = sValue; }
	tClassDate::tClassDate(tInt sYear, tInt sMonth, tInt sDay) : tClass() {
		Set(sYear, sMonth, sDay);
	};

	tClassDate::tClassDate(const tClassDate& sClassDate) : tClass(sClassDate) { m_Value = sClassDate.m_Value; }

    tClassDate::tClassDate(const tVariant& sVariant) : tClass(), m_Value(0) {
        switch (sVariant.Type()) {
            case tVariantType::t_date:
                m_Value = sVariant.Date();
                break;
            case tVariantType::t_int:
            case tVariantType::t_double: {
                // Excel serial (1 = 1900-01-01). Rebuild the date through Set() so a
                // serial-derived date shares the EXACT same time_t as DATE()/EDATE()/
                // TODAY(): equality (=A1=B1), VLOOKUP/MATCH exact match and date
                // arithmetic compare the raw time_t, not the rendered calendar day, so
                // every serial->date path must agree on the time-of-day anchor.
                const double wSerial = sVariant.Numeric();
                const double wFrac = wSerial - std::floor(wSerial);
                // Pure time-of-day (OOXML serial in [0,1)): anchor on 1900-01-01 like SetHour/IsHours().
                // Do not use the full-serial gmtime+Set path: mktime(1899-12-31) can fail and leave
                // m_Value at 0, then += frac*86400 stores raw Unix seconds (7:30 → 08:30 in UTC+1).
                if (wSerial >= 0.0 && wSerial < 1.0) {
                    const tInt wTotalSec =
                        static_cast<tInt>(std::llround(wFrac * 86400.0)) % 86400;
                    SetDateHour(1900, 1, 1, wTotalSec / 3600, (wTotalSec % 3600) / 60, wTotalSec % 60);
                    break;
                }
                // The 1900 leap-year bug is honored (serials < 60 are shifted one day).
                // Decode Y/M/D with civil math: Windows gmtime_s rejects time_t < 1970
                // (Excel's default epoch is 1900-01-01), which left serial 1 != DATE(1900,1,1).
                // Then re-anchor to local midnight via Set() so import and DATE() share time_t.
                const double wOffsetDays = (wSerial < 60.0) ? 25568.0 : 25569.0;
                const long long wUnixDays =
                    static_cast<long long>(std::floor(wSerial - wOffsetDays));
                int wYear = 1970;
                unsigned wMonth = 1;
                unsigned wDay = 1;
                CivilFromDays(wUnixDays, wYear, wMonth, wDay);
                Set(wYear, static_cast<tInt>(wMonth), static_cast<tInt>(wDay));
                if (wFrac > 0.0) {
                    m_Value += static_cast<tDate>(std::llround(wFrac * 86400.0));
                }
                break;
            }
            default:
                Clear();
                break;
        }
    }

    void tClassDate::Clear() {
        Set(1900, 1, 1);
    }

    tDate tClassDate::Now() {
        return(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }

    // Function to format a time_t value into a date or time string.
    string tClassDate::FormatDateTime(tString sFormat) {
        char wBuffer[90];
        struct tm wTimeInfo;
        DateToTMStruct(wTimeInfo);
        wTimeInfo.tm_isdst =  Cst_isdst; // summer time
        strftime(wBuffer, sizeof(wBuffer), sFormat.c_str(), &wTimeInfo);
        return wBuffer;
    }

    tString tClassDate::FormatString(tFormatString* const sFormatString) {
        tLocale* wLocale=tApplication::Instance()->Locale();
        
        tFormatStringType wFormatType=sFormatString->FormatType();
        
        switch (wFormatType) {
            // Excel Date/Time via Excel format string
            case tFormatStringType::exceldate: {
                tDateFormatter* wDateFormatter=tApplication::Instance()->DateFormatter();
                tString wExcelFormat = sFormatString->FormatExcel();
                return(wDateFormatter->FormatDateTime(m_Value, wExcelFormat));
                break;
            }
            // Other
            default: {
                tFormatStringType wResolvedType = wFormatType;
                if (wResolvedType == tFormatStringType::excelnumber) {
                    wResolvedType = tFormatStringType::datelong;
                }
                tString wFormat=tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(wResolvedType);
                if (wFormat.empty() || wFormat == "None") {
                    wFormat=tApplication::Instance()->FormatStringRoot()->FormatStringType2LocalFormatString(tFormatStringType::datelong);
                }
                tClassString wClassString(wFormat);
                wClassString.Replace("%A",wLocale->DayStr(DayWeek()));
                wClassString.Replace("%B",wLocale->MonthStr(Month()));
                wClassString.Replace("%p",AMPM(wFormatType));
                // strftime not fill 0
                if (wClassString.Find("%d")!=std::string::npos)
                    wClassString.Replace("%d",Day0());
                if (wClassString.Find("%m")!=std::string::npos)
                    wClassString.Replace("%m",Month0());
                if (wClassString.Find("%y")!=std::string::npos)
                    wClassString.Replace("%y",YearShort0());
                if (wClassString.Find("%Y")!=std::string::npos)
                    wClassString.Replace("%Y",YearLong0());
                
                return(FormatDateTime(wClassString()));      }
        }
        return("");
    }

    tDate tClassDate::TimeOffset() {
        return static_cast<tDate>(UtcToLocalShiftSeconds());
    }

	void tClassDate::DateToTMStruct(struct tm& sTMStruct) {
		// Prefer CRT localtime when it works (1970+). Fall back to civil math for
		// Excel's 1900-01-01 epoch, which Windows mktime/localtime reject.
#if defined(WIN32)
		if (localtime_s(&sTMStruct, &m_Value) == 0) {
			sTMStruct.tm_isdst = Cst_isdst;
			return;
		}
#else
		if (localtime_r(&m_Value, &sTMStruct) != nullptr) {
			sTMStruct.tm_isdst = Cst_isdst;
			return;
		}
#endif
		UnixToTmLocal(m_Value, sTMStruct);
    }

	void tClassDate::TMStructToDate(struct tm& sTMStruct) {
		// mktime() uses local TZ; some platforms return -1 for far-past dates without updating the struct
		// (e.g. Jan 1 1900 on Windows/macOS), leaving m_Value stuck at the ctor default.
		struct tm wOriginalStruct = sTMStruct;
		sTMStruct.tm_isdst = -1;
		time_t wTimeValue = mktime(&sTMStruct);
		if (wTimeValue == static_cast<time_t>(-1)) {
			sTMStruct = wOriginalStruct;
			sTMStruct.tm_isdst = 0;
			wTimeValue = mktime(&sTMStruct);
		}
		if (wTimeValue != static_cast<time_t>(-1)) {
			m_Value = static_cast<tDate>(wTimeValue);
			return;
		}
		m_Value = TmLocalToUnix(wOriginalStruct);
	}

	void tClassDate::InternalSet() {
		tInt wYear; tInt wMonth; tInt wDay;
		YearMonthDay(wYear, wMonth, wDay);
		Set(wYear, wMonth, wDay);
	};

    tDate tClassDate::AddInt(tInt sInt) {
        struct tm wTMStruct;
        tClassDate wClassDate;
        DateToTMStruct(wTMStruct);
        wTMStruct.tm_mday+=sInt;
        wClassDate.TMStructToDate(wTMStruct);
        return(wClassDate.Value());
    }

    const tInt Cst_SecondByDay=60*60*24;

    tDate tClassDate::AddDouble(tDouble sDouble) {
        const tInt wSecondDay=Cst_SecondByDay;
        tDouble wNbDay = floor(sDouble);
        tDouble wNbSecond = (sDouble-wNbDay)* wSecondDay;
        
#ifdef debugdate
        tInt wHours = wNbSecond / 3600;
        tInt wTotalSeconds= tInt(wNbSecond) % 3600; // Obtenir le reste des secondes après calcul des heures
        tInt wMinutes = wTotalSeconds / 60;
        tInt wSeconds = wTotalSeconds % 60; //

        cout << "Add  D:" << wNbDay;
        cout << ":H:" << wHours;
        cout << ":M:" << wMinutes;
        cout << ":S:" << wSeconds;
#endif
        tDate wDelta = static_cast<tDate>((wNbDay*wSecondDay)+wNbSecond);
        // Do not mutate this: callers may reuse the same tClassDate or chain ops; mutation broke matrix date + offset.
        return m_Value + wDelta;
    }

    tString tClassDate::Day0() {
        tStringStream wStream;
        wStream  << std::setfill('0') << setw(2) << Day();
        return(wStream.str());
    }

    tString  tClassDate::Month0() {
        tStringStream wStream;
        wStream << std::setfill('0') << setw(2) << Month();
        return(wStream.str());
    }

    tString tClassDate::YearShort0() {
        tStringStream wStream;
        wStream << std::setfill('0') << setw(2) << (Year() % 100);
        return(wStream.str());
    }

    tString  tClassDate::YearLong0() {
        tStringStream wStream;
        wStream << std::setfill('0') << setw(4) << Year();
        return(wStream.str());
    }

    tString  tClassDate::AMPM(tFormatStringType sFormatStringType) {
        tStringStream wStream;
        tString  wPM="AM"; // am – ante meridiem –  0h  12h. pm – post meridiem –  12h  0h.
        tInt wHour=Hour();
        if (wHour > 12)  {
            wPM="PM";
            wHour=wHour-12;
        }
        tLocale* wLocale=tApplication::Instance()->Locale();
        wStream << wHour <<  wLocale->Time()  << std::setw(2) <<  std::setfill('0') << Minute();
        if (sFormatStringType==tFormatStringType::datehmmssap) {
            wStream <<  wLocale->Time()  <<  std::setw(2) <<  std::setfill('0') << Second();
        }
        
        wStream << " " << wPM;
        return(wStream.str());
    }

	tDate tClassDate::Value() { return(m_Value); }

    void tClassDate::Value(tDate sValue) { m_Value=sValue; }

    tBool tClassDate::IsHours() {
        struct tm wTMStruct;
        DateToTMStruct(wTMStruct);
        return((wTMStruct.tm_mday==1) &&
               (wTMStruct.tm_mon==0) &&
               (wTMStruct.tm_year==0));
    }

	tDate tClassDate::Set(tInt sYear, tInt sMonth, tInt sDay) {
		struct tm wTMStruct;
        wTMStruct.tm_sec=0;        /* seconds after the minute [0-60] int */
        wTMStruct.tm_min=0;        /* minutes after the hour [0-59] int */
        wTMStruct.tm_hour=0;      /* hours since midnight [0-23] int */
        wTMStruct.tm_mday=sDay;    /* day of the month [1-31]  int */
        wTMStruct.tm_mon=sMonth-1;        /* months since January [0-11] int */
        wTMStruct.tm_year= sYear - 1900;    /* years since 1900 int */
        wTMStruct.tm_wday=0;    /* days since Sunday [0-6] int */
        wTMStruct.tm_yday=0;    /* days since January 1 [0-365] int  */
        wTMStruct.tm_isdst=Cst_isdst;    /* Daylight Savings Time flag  int */
#ifndef  WIN32
		wTMStruct.tm_gmtoff = 0;    /* offset from UTC in seconds long */
#endif // ! WIN32
        //wTMStruct.tm_zone;      /* timezone abbreviation char* */
		TMStructToDate(wTMStruct);
		return(m_Value);
	}

    tDate tClassDate::SetHour(tInt sHour,tInt sMinute,tInt sSecond) {
        struct tm wTMStruct;
        wTMStruct.tm_sec=sSecond;        /* seconds after the minute [0-60] int */
        wTMStruct.tm_min=sMinute;        /* minutes after the hour [0-59] int */
        wTMStruct.tm_hour=sHour;    /* hours since midnight [0-23] int */
        wTMStruct.tm_mday=1;    /* day of the month [1-31]  int */
        wTMStruct.tm_mon=0;        /* months since January [0-11] int */
        wTMStruct.tm_year= 0;    /* years since 1900 int */
        wTMStruct.tm_wday=0;    /* days since Sunday [0-6] int */
        wTMStruct.tm_yday=0;    /* days since January 1 [0-365] int  */
        wTMStruct.tm_isdst=Cst_isdst;    /* Daylight Savings Time flag  int */
#ifndef  WIN32
        wTMStruct.tm_gmtoff=0;    /* offset from UTC in seconds long */
#endif      
		//wTMStruct.tm_zone;      /* timezone abbreviation char* */
		TMStructToDate(wTMStruct);
        return(m_Value);
    }

    tDate tClassDate::SetDateHour(tInt sYear, tInt sMonth, tInt sDay,tInt sHour,tInt sMinute,tInt sSecond) {
        struct tm wTMStruct;
        wTMStruct.tm_sec=sSecond;        /* seconds after the minute [0-60] int */
        wTMStruct.tm_min=sMinute;        /* minutes after the hour [0-59] int */
        wTMStruct.tm_hour=sHour;    /* hours since midnight [0-23] int */
        wTMStruct.tm_mday=sDay;    /* day of the month [1-31]  int */
        wTMStruct.tm_mon=sMonth-1;        /* months since January [0-11] int */
        wTMStruct.tm_year= sYear - 1900;    /* years since 1900 int */
        wTMStruct.tm_wday=0;    /* days since Sunday [0-6] int */
        wTMStruct.tm_yday=0;    /* days since January 1 [0-365] int  */
        wTMStruct.tm_isdst=Cst_isdst;    /* Daylight Savings Time flag  int */
#ifndef  WIN32
        wTMStruct.tm_gmtoff=0;    /* offset from UTC in seconds long */
#endif        
		//wTMStruct.tm_zone;      /* timezone abbreviation char* */
		TMStructToDate(wTMStruct);
        return(m_Value);
    }

	void tClassDate::YearMonthDay(tInt& sYear, tInt& sMonth, tInt& sDay) {
		struct tm wTMStruct; 
		DateToTMStruct(wTMStruct);
		sDay = wTMStruct.tm_mday;
		sMonth = wTMStruct.tm_mon + 1;
		sYear = wTMStruct.tm_year + 1900;
	};

    void tClassDate::HourMinuteSecond(tInt& sHour, tInt& sMinute, tInt& sSecond) {
        struct tm wTMStruct;
        DateToTMStruct(wTMStruct);
        sHour = wTMStruct.tm_hour;
        sMinute = wTMStruct.tm_min;
        sSecond = wTMStruct.tm_sec;
    }

    // State of parsing
    tStateParse wLocalState=tStateParse::none;
 
    // function to parse a date or time string.
    // Windows function strptime don't exist
    tBool tClassDate::ParseDateTime(const char* sValue, const char* sFormat) {
        wLocalState=tStateParse::none;
        tLocale* wLocale=tApplication::Instance()->Locale();
        
        const tChar wDecimalSeparator=wLocale->Decimal();
        //const tChar wDateSeparator=wLocale->Date();
        //const tChar wHourSeparator=wLocale->Time();
        
        // Parse format
        tLexer wLexFormat(sFormat);
        wLexFormat.SeparatorDecimal(wDecimalSeparator);
        tLexerToken wLexerTokenFormat;

        // parse value
        tLexer wLexValue(sValue);
        wLexValue.SeparatorDecimal(wDecimalSeparator);
        tLexerToken wLexerTokenValue;

        tBool wIsHour=false;
        tBool wIsDate=false;
        tInt wLongYear=-1;
        tInt wYear=-1;
        tInt wDay=-1;
        tInt wMonth=-1;
        tInt wHour=0;
        tInt wMinute=0;
        tInt wSecond=0;
        
        // Token Value
        wLexerTokenValue = wLexValue.next();
        if (wLexerTokenValue.is_one_of(tKind::End, tKind::Unexpected)) return(false);

        // Loop until End or Unexpected caracter ===============================
        for (wLexerTokenFormat = wLexFormat.next();!wLexerTokenFormat.is_one_of(tKind::End, tKind::Unexpected); wLexerTokenFormat = wLexFormat.next()) {
            //cout << "wLexerTokenFormat = " << wLexerTokenFormat.Lexeme() << endl;
            tBool wExit=false;
            switch (wLexerTokenFormat.Kind()) {
                case tKind::End:
                    wExit=true;
                    break;
                default:
                    tChar wChar=wLexerTokenFormat.Lexeme()[0];
                    switch (wChar) {
                        case '%':
                            wLocalState=tStateParse::_percent;
                            break;
                        case 'Y' :
                        case 'y' :
                        case 'm' :
                        case 'd' :
                        case 'H' :
                        case 'M' :
                        case 'S' : {
                            if (wLocalState==tStateParse::_percent) {
                                if (wChar=='Y') wLocalState=tStateParse::YEAR; else
                                if (wChar=='y') wLocalState=tStateParse::year; else
                                if (wChar=='m') wLocalState=tStateParse::month; else
                                if (wChar=='d') wLocalState=tStateParse::day; else
                                if (wChar=='H') wLocalState=tStateParse::hour; else
                                if (wChar=='M') wLocalState=tStateParse::minute; else
                                if (wChar=='S') wLocalState=tStateParse::second;
                                break;
                            }
                            // Contine on default
                        }
                        default:
                            if ( wLexerTokenValue.Lexeme()!=wLexerTokenFormat.Lexeme()) {
                                return(false);
                            }
                            wLexerTokenValue = wLexValue.next();
                    }
                    break;
            }
            
            tString wStringValue;
            // Is element date hour
            if (wLocalState>tStateParse::_percent) {
                wStringValue=wLexerTokenValue.Lexeme().c_str();
                if (wLexerTokenValue.Kind()!=tKind::Integer) return(false);
            }
            
            switch (wLocalState) {
                case tStateParse::YEAR:
                    wLongYear=stoi(wStringValue);
                    //if (wLongYear<1900) return(false);
                    break;
                case tStateParse::year:
                    wYear=stoi(wStringValue);
                    break;
                case tStateParse::month:
                    wMonth=stoi(wStringValue);
                    if (wMonth==0) return(false);
                    if (wMonth>12) return(false);
                    break;
                case tStateParse::day:
                    wDay=stoi(wStringValue);
                    if (wDay==0) return(false);
                    if (wDay>31) return(false);
                    break;
                case tStateParse::hour:
                    wIsHour=true;
                    wHour=stoi(wStringValue);
                    //if (wHour>23) return(false);
                    break;
                case tStateParse::minute:
                    wMinute=stoi(wStringValue);
                    if (wMinute>59) return(false);
                    break;
                case tStateParse::second:
                    wSecond=stoi(wStringValue);
                    if (wSecond>59) return(false);
                    break;
                default:
                    break;
            }
            if (wLocalState>tStateParse::_percent) {
                wLexerTokenValue = wLexValue.next();
                wLocalState=tStateParse::none;
            }
     
            if (wExit) break;
        }
        struct tm wTMStruct;
        wTMStruct.tm_sec=0; /* seconds after the minute [0-60] int */
        wTMStruct.tm_min=0; /* minutes after the hour [0-59] int */
        wTMStruct.tm_hour=0; /* hours since midnight [0-23] int */
        wTMStruct.tm_mday=1; /* day of the month [1-31]  int */
        wTMStruct.tm_mon=0;  /* months since January [0-11] int */
        wTMStruct.tm_year=0; /* years since 1900 int */
        wTMStruct.tm_wday=0; /* days since Sunday [0-6] int */
        wTMStruct.tm_yday=0; /* days since January 1 [0-365] int  */
#ifndef WIN32
        wTMStruct.tm_gmtoff=0; /* offset from UTC in seconds long */
#endif
        // Set date
        if (((wYear!=-1) || (wLongYear!=-1)) && ((wMonth!=-1) && (wDay!=-1))) {
            if (wYear!=-1) {
                wTMStruct.tm_year= (wYear+2000) - 1900;
            }
            if (wLongYear!=-1) {
                wTMStruct.tm_year= wLongYear - 1900;    /* years since 1900 int */
            }
            wTMStruct.tm_mon=wMonth-1;  /* months since January [0-11] int */
            wTMStruct.tm_mday=wDay; /* day of the month [1-31]  int */
            wIsDate=true;
        }
        // Set hour
        if (wIsHour) {
            wTMStruct.tm_hour=wHour;
            wTMStruct.tm_min=wMinute;
            wTMStruct.tm_sec=wSecond; /* seconds after the minute [0-60] int */
        }
        if (wIsDate || wIsHour) {
            wTMStruct.tm_isdst =  Cst_isdst; // summer time
            TMStructToDate(wTMStruct);
            return(true);
        }
        return(false);
    }
 

	tInt  tClassDate::Year() {
		tInt wYear; tInt wMonth; tInt wDay;
		YearMonthDay(wYear, wMonth, wDay);
		return(wYear);
	}

	tInt  tClassDate::Month() {
		tInt wYear; tInt wMonth; tInt wDay;
		YearMonthDay(wYear, wMonth, wDay);
		return(wMonth);
	}

	tInt  tClassDate::Day() {
		tInt wYear; tInt wMonth; tInt wDay;
		YearMonthDay(wYear, wMonth, wDay);
		return(wDay);
	}

    tInt tClassDate::Hour() {
        tInt wHour;  tInt wMinute; tInt wSecond;
        HourMinuteSecond(wHour, wMinute, wSecond);
        return(wHour);
    }

    tInt tClassDate::Minute() {
        tInt wHour; tInt wMinute; tInt wSecond;
        HourMinuteSecond(wHour, wMinute, wSecond);
        return(wMinute);
    }

    tInt  tClassDate::Second() {
        tInt wHour; tInt wMinute; tInt wSecond;
        HourMinuteSecond(wHour, wMinute, wSecond);
        return(wSecond);
    }

	tInt  tClassDate::DayWeek() {
        struct tm wTMStruct;
        DateToTMStruct(wTMStruct);
        return(wTMStruct.tm_wday);
	}


    tInt  tClassDate::WeekNum() {
        struct tm wTMStruct;
        DateToTMStruct(wTMStruct);
        char wBuffer[3];
        // Utiliser strftime avec "%V" pour obtenir le numéro de la semaine ISO
        strftime(wBuffer, sizeof(wBuffer), "%V", &wTMStruct);
        return atoi(wBuffer);
    }

	tString tClassDate::UsDate(tBool sTime) {
		tInt wYear;
		tInt wMonth;
		tInt wDay;
		tStringStream wStream;
		YearMonthDay(wYear, wMonth, wDay);
		wStream << std::setfill('0') << setw(2) << wMonth << "-" << setw(2) << wDay << "-" << setw(4) << wYear;
        if (sTime) {
            tInt wHour;
            tInt wMinute;
            tInt wSecond;
            HourMinuteSecond(wHour, wMinute, wSecond);
            if ((wHour!=0) || (wMinute!=0) || (wSecond!=0)) {
                wStream << " " << std::setfill('0') << setw(2) << wHour << ":" << setw(2) << wMinute << ":" << setw(2) << wSecond;
            }
        }
		return(wStream.str());
	};

	void tClassDate::UsDate(tString sDate) {
		tInt wYear = 0;
		tInt wMonth = 0;
		tInt wDay =0;
		tInt wHour = 0;
		tInt wMinute = 0;
		tInt wSecond = 0;

		tClassString wClassString(sDate);
        tVectorString wVectorRoot = wClassString.Split(" ");
        if (wVectorRoot.size()>0) {
            tVectorString wVectorDate = tClassString(wVectorRoot[0]).Split("-");
            // Verify that we have exactly 3 parts (MM-DD-YYYY)
            if (wVectorDate.size() == 3) {
                tInt wIndex = 0;
                for (auto wItem : wVectorDate) {
                    switch (wIndex) {
                        case 0:
                            wMonth = atoi(wItem.c_str());
                            break;
                        case 2:
                            wYear = atoi(wItem.c_str());
                            break;
                        case 1:
                            wDay = atoi(wItem.c_str());
                            break;
                    }
                    wIndex++;
                }
            }
        }
        // Parse time if present
        if (wVectorRoot.size()>1) {
            tVectorString wVectorHour = tClassString(wVectorRoot[1]).Split(":");
            tInt wIndex = 0;
            for (auto wItem : wVectorHour) {
                switch (wIndex) {
                    case 0:
                        wHour = atoi(wItem.c_str());
                        break;
                    case 1:
                        wMinute = atoi(wItem.c_str());
                        break;
                    case 2:
                        wSecond = atoi(wItem.c_str());
                        break;
                }
                wIndex++;
            }
        }
        
        // Set date and time together to avoid SetHour() resetting the date
        if (wYear > 0 && wYear < 10000 && wMonth > 0 && wMonth <= 12 && wDay > 0 && wDay <= 31) {
            SetDateHour(wYear, wMonth, wDay, wHour, wMinute, wSecond);
        }
	}
   
	tClassDate tClassDate::operator = (const tDate sValue) { return(tClassDate(sValue)); }

	// pour �galit� ou pas on se mets sur les jours ==============================
	tBool tClassDate::operator==(tClassDate& sClassDate) { 
		return(UsDate() == sClassDate.UsDate());
	}

	tBool tClassDate::operator!=(tClassDate& sClassDate) { 
		return(UsDate() != sClassDate.UsDate());
	}

	tBool tClassDate::operator < (tClassDate& sClassDate) { 
		return(UsDate() < sClassDate.UsDate());
	}
	tBool tClassDate::operator > (tClassDate& sClassDate) { 
		return(UsDate() > sClassDate.UsDate());
	}
	tBool tClassDate::operator <= (tClassDate& sClassDate) { 
		return(UsDate() <= sClassDate.UsDate());
	}
	tBool tClassDate::operator >= (tClassDate& sClassDate) { 
		return(UsDate() >= sClassDate.UsDate());
	}
	

	tClassDate tClassDate::operator +(tClassInt sClassInt) {
        return(tClassDate(AddInt(sClassInt())));
	}
	
	tClassDate tClassDate::operator +(tInt sInt) {
        return(tClassDate(AddInt(sInt)));
	}

	tClassDate tClassDate::operator -(tClassInt sClassInt) {
        return(tClassDate(AddInt(-sClassInt())));
	}

	tClassDate tClassDate::operator -(tInt sInt) {
        return(tClassDate(AddInt(-sInt)));
	}

  
    tClassDate tClassDate::operator +(tDouble sDouble) {
        return(tClassDate(AddDouble(sDouble)));
    }

    tClassDate tClassDate::operator +(tClassDouble sClassDouble)  {
        return(tClassDate(AddDouble(sClassDouble())));
    }

    tClassDate tClassDate::operator -(tDouble sDouble) {
        return(tClassDate(AddDouble(-sDouble)));
    }

    tClassDate tClassDate::operator -(tClassDouble sClassDouble) {
        return(tClassDate(AddDouble(-sClassDouble())));
    }

    tClassDate tClassDate::operator +(tClassDate sClassDate) {
        // Excel: date operands are serial numbers (day + fraction). Summing Unix timestamps was wrong
        // (e.g. B4+TIME(0,30,0) showed years jumping backward). Match tVariant double→date Excel path.
        const tDouble wLeftSerial = static_cast<tDouble>(m_Value) / static_cast<tDouble>(Cst_SecondByDay) + 25569.0;
        const tDouble wRightSerial = static_cast<tDouble>(sClassDate.m_Value) / static_cast<tDouble>(Cst_SecondByDay) + 25569.0;
        tVariant wSum;
        wSum.SetDouble(wLeftSerial + wRightSerial);
        return tClassDate(wSum);
    }

    tClassDate tClassDate::operator -(tClassDate sClassDate) {
        tDate wLeft=m_Value+TimeOffset();
        tDate wRight=sClassDate.m_Value+sClassDate.TimeOffset();
        
        tClassDate wClassReturn(wLeft-wRight);
        wClassReturn.Value(wClassReturn.Value()- wClassReturn.TimeOffset());
 
        return(wClassReturn);
    }

	tDate tClassDate::operator()() { return(m_Value); }

	ostream& operator<<(ostream& os, tClassDate& sClassDate) {
		tInt wYear;
		tInt wMonth;
		tInt wDay;
		sClassDate.YearMonthDay(wYear, wMonth, wDay);
		os << std::setfill('0')  << setw(2) << wDay << "/" << setw(2) << wMonth << "/" << setw(4) << wYear;
		return os;
	}
	
	// tClassError ==============================================, =============
	tClassError::tClassError() : tClass(), m_Code(tTypeError::t_none),m_String("") {};

	tClassError::tClassError(tTypeError sCode, tString sString) : tClass(), m_Code(sCode), m_String(sString) {};
	
	tClassError::tClassError(const tClassError& sClassError) : tClass(sClassError) {
		m_Code = sClassError.m_Code;
		m_String = sClassError.m_String;
	}

	tClassError::~tClassError() {
	};

	void tClassError::SetError(tTypeError sCode, tString sString) { m_Code = sCode; m_String = sString; }
	tTypeError tClassError::Code() { return(m_Code); };
	tInt tClassError::CodeInt() { return(static_cast<tInt>(m_Code)); }
	void tClassError::CodeInt(tInt sCode) { m_Code = static_cast<tTypeError>(sCode); }
	tString tClassError::String() { return(m_String); }
	tString tClassError::Error() {
        tStringStream wStream;
		switch (m_Code)	{
            case tTypeError::t_none: wStream << "NONE"; break;
            case tTypeError::t_value: wStream << "#VALUE!"; break;
            case tTypeError::t_div0: wStream << "#DIV/0!"; break;
            case tTypeError::t_ref: wStream << "#REF!"; break;
            case tTypeError::t_num: wStream << "#NUM!"; break;
            case tTypeError::t_name: wStream << "#NAME?"; break;
            case tTypeError::t_recursive: wStream << "#RECURSIVE"; break;
            case tTypeError::t_arg: wStream << "#ARG!"; break;
            case tTypeError::t_class: wStream << "#CLASS!"; break;
            case tTypeError::t_na: wStream << "#NA"; break;
            case tTypeError::t_matrix: wStream << "#MATRIX!"; break;
            case tTypeError::t_spill: wStream << "#SPILL!"; break;
            case tTypeError::t_calc: wStream << "#CALC!"; break;
            // A used-but-omitted optional LAMBDA parameter surfaces like Excel's #VALUE!.
            case tTypeError::t_omitted: wStream << "#VALUE!"; break;
            default:
                return("");
                break;
		}
        if (m_String!="") {
            wStream << " " << m_String;
        }
        return(wStream.str());
	}
	tBool tClassError::operator==(tClassError& sClassError) {
		return(m_Code == sClassError.m_Code);
	}

	void tClassError::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("c");
		sWriter->Int(CodeInt());
		sWriter->Key("m");
		sWriter->String(m_String.c_str());
		sWriter->EndObject();
	}

	void tClassError::Json(const rapidjson::Value& sValue) {
		const Value& wCodeValue = sValue["c"];
		assert(wCodeValue.IsInt());
		m_Code = tTypeError(wCodeValue.GetInt());
		const Value& wStringValue = sValue["m"];
		assert(wStringValue.IsString());
		m_String = wStringValue.GetString();
	}


}; // end of namespace ========================================================
