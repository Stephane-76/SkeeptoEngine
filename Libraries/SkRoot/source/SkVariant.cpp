//=============================================================================
// SkRoot Variant
//   Manage variants for all applications (like and for spreadsheet) 
//=============================================================================

#include "../include/SkVariant.hpp"
#include "../include/SkModelClass.hpp"
// for Locale
#include "../include/SkApplication.hpp"
#include "../include/SkLexer.hpp"

#define _DEBUGSKFormatString

namespace SkRoot {

    namespace {
        // General/excelnumber masks are not in FormatStringRoot — local lookup returns "None".
        static void ResolveDateInputFormat(tVariant* sVariant, tFormatString& oFmt) {
            if (sVariant == nullptr || sVariant->Type() != tVariantType::t_date) {
                return;
            }
            tFormatStringRoot* wRoot = tApplication::Instance()->FormatStringRoot();
            const tFormatStringType wType = oFmt.FormatType();
            if (wType == tFormatStringType::exceldate) {
                oFmt.SetDefaultFormat(sVariant);
                return;
            }
            if (wRoot->FormatString2Family(wType) != tFormatStringFamily::date) {
                oFmt.FormatType(tFormatStringType::none);
            }
            oFmt.SetDefaultFormat(sVariant);
        }
    }

    tString VariantType2Str(tVariantType sVariantType) {
        switch (sVariantType) {
                
            case tVariantType::t_null :
                return("null");
                break;
            case tVariantType::t_int :
                return("int");
                break;
            case tVariantType::t_bool :
                return("bool");
                break;
            case tVariantType::t_double :
                return("float");
                break;
            case tVariantType::t_string :
                return("string");
                break;
            case tVariantType::t_error :
                return("error");
                break;
            case tVariantType::t_date :
                return("date");
                break;
            case tVariantType::t_class :
                return("class");
                break;
        }
        return("");
    }

    tVariantType Str2VariantType(tString sVariantType) {
        if (sVariantType=="int") return(tVariantType::t_int);
        if (sVariantType=="bool") return(tVariantType::t_bool);
        if (sVariantType=="double") return(tVariantType::t_double);
        if (sVariantType=="string") return(tVariantType::t_string);
        if (sVariantType=="error") return(tVariantType::t_error);
        if (sVariantType=="date") return(tVariantType::t_date);
        if (sVariantType=="class") return(tVariantType::t_class);
        return(tVariantType::t_null);
    }


	// SKVariant pour la gestion des variants =================================
	tVariant::tVariant() : tClass(), m_Type(tVariantType::t_null),m_Extra(0),m_Union() {}

	tVariant::tVariant(const tVariant& sVariant) : tClass(sVariant) {
		m_Type = sVariant.m_Type;
		m_Extra = sVariant.m_Extra;
		switch (sVariant.m_Type) {
		case tVariantType::t_null: m_Type = tVariantType::t_null; break;
		case tVariantType::t_int: m_Union.m_Int = sVariant.m_Union.m_Int; break;
		case tVariantType::t_bool: m_Union.m_Bool = sVariant.m_Union.m_Bool; break;
		case tVariantType::t_double: m_Union.m_Double = sVariant.m_Union.m_Double; break;
		case tVariantType::t_string: m_Union.m_String = new tSharedString(*sVariant.m_Union.m_String); break;
		case tVariantType::t_error: m_Union.m_Error = new tClassError(*sVariant.m_Union.m_Error); break;
		case tVariantType::t_date: m_Union.m_Date = sVariant.m_Union.m_Date; break;
		case tVariantType::t_class: {
            if (sVariant.m_Union.m_Class!=nullptr) {
                if (sVariant.m_Union.m_Class->IsCopy()) {
                    m_Union.m_Class = sVariant.m_Union.m_Class->Clone();
                } else {
                    m_Union.m_Class = sVariant.m_Union.m_Class;
                }
			}
			else {
				m_Union.m_Class = sVariant.m_Union.m_Class;
			}
		}
		}
	}

	tVariant::tVariant(tInt sValue) : tClass(), m_Extra(-1) { m_Type = tVariantType::t_int;  m_Union.m_Int = sValue; }
	tVariant::tVariant(tBool sValue) : tClass(), m_Extra(-1)  { m_Type = tVariantType::t_bool;  m_Union.m_Bool = sValue; }
	tVariant::tVariant(tDouble sValue) : tClass(), m_Extra(-1)  { m_Type = tVariantType::t_double;  m_Union.m_Double = sValue; }
	tVariant::tVariant(tString sValue) : tClass(), m_Extra(-1) { m_Type = tVariantType::t_string;  m_Union.m_String = new tSharedString(sValue); }
	tVariant::tVariant(tClassError sValue) : tClass(), m_Extra(-1) {m_Type = tVariantType::t_error; m_Union.m_Error = new tClassError(sValue); }
	tVariant::tVariant(const tChar* sValue) : tClass(), m_Extra(-1) { m_Type = tVariantType::t_string;  m_Union.m_String = new tSharedString(tString(sValue)); }
	
#ifndef __EMSCRIPTEN__32__
	tVariant::tVariant(tDate sValue) : tClass(), m_Extra(-1) { m_Type = tVariantType::t_date;  m_Union.m_Date = sValue; }
#endif
	//#endif
	tVariant::tVariant(tVirtualClass* sValue) : tClass(), m_Extra(-1) { m_Type = tVariantType::t_class;  m_Union.m_Class = sValue->Clone(); }

	tVariant::~tVariant() {
		Clear();
	}

	void tVariant::Clear() {
		switch (m_Type) {
		case tVariantType::t_string:
			delete(m_Union.m_String);
			break;
		case tVariantType::t_error: delete(m_Union.m_Error); break;
		case tVariantType::t_class: 
            if(m_Union.m_Class!=nullptr) {
                if (m_Union.m_Class->IsCopy()) {
                    delete(m_Union.m_Class);
                }
            }
            m_Union.m_Class = nullptr;
            break;
		default: break;
		}
		m_Type = tVariantType::t_null;
	}


	tString tVariant::Str() const {
		tStringStream wStream;
		switch (m_Type) {
		case tVariantType::t_null: break; // os << "null"; break;
		case tVariantType::t_int:  wStream << Int(); break;
		case tVariantType::t_bool: if (Bool()) { wStream << "true"; }
									else { wStream << "false"; }	break;
		case tVariantType::t_double: wStream << Double(); break;
		case tVariantType::t_string: wStream << String(); break;
		case tVariantType::t_error: wStream << Error().Error(); break;
		case tVariantType::t_date: {
			tClassDate wDate(Date());
			wStream << wDate.UsDate();
			break;
		}
        case tVariantType::t_class: {
            tVirtualClass* wClass=m_Union.m_Class;
            wStream <<  wClass->ClassName(); break;
        }
		}
		return(wStream.str());
	}

	void tVariant::Assign(const tVariant& sVariant) {
		Clear();
		m_Type = sVariant.m_Type;
		m_Extra = sVariant.m_Extra;
		switch (sVariant.m_Type) {
		case tVariantType::t_null: m_Type = tVariantType::t_null; break;
		case tVariantType::t_int: m_Union.m_Int = sVariant.m_Union.m_Int; break;
		case tVariantType::t_bool: m_Union.m_Bool = sVariant.m_Union.m_Bool; break;
		case tVariantType::t_double: m_Union.m_Double = sVariant.m_Union.m_Double; break;
		case tVariantType::t_string: m_Union.m_String = new tSharedString(*sVariant.m_Union.m_String); break;
		case tVariantType::t_error: m_Union.m_Error = new tClassError(*sVariant.m_Union.m_Error); break;
		case tVariantType::t_date: m_Union.m_Date = sVariant.m_Union.m_Date; break;
		case tVariantType::t_class: {
			if (sVariant.m_Union.m_Class != nullptr) {
				if (sVariant.m_Union.m_Class->IsCopy()) {
					m_Union.m_Class = sVariant.m_Union.m_Class->Clone();
				} else {
					m_Union.m_Class = sVariant.m_Union.m_Class;
				}
			} else {
                m_Type=tVariantType::t_null;
			}
		}
		}
	}
    
    
	tVariantType tVariant::Type() const { return(m_Type); }

	void tVariant::SetInt(tInt sValue) { Clear(); m_Type = tVariantType::t_int;  m_Union.m_Int = sValue; }
	tInt tVariant::Int() const { 
		if (m_Type == tVariantType::t_int) { 
			return(m_Union.m_Int); 
		}  else {
			throw(tExceptionBadType("tVariant not int"));
		}
	}

	void tVariant::SetBool(tBool sValue) { Clear(); m_Type = tVariantType::t_bool;  m_Union.m_Bool = sValue; }
	tBool tVariant::Bool() const {
		if (m_Type == tVariantType::t_bool) {
			return(m_Union.m_Bool);
		}
		else {
            if (m_Type== tVariantType::t_int) {
                if (m_Union.m_Int==1) return(true);
            }
			return(false);
		}
	}

	void tVariant::SetDouble(tDouble sValue) { Clear(); 
	m_Type = tVariantType::t_double;  m_Union.m_Double = sValue; }
	tDouble tVariant::Double() const {
		if (m_Type == tVariantType::t_double) {
			return(m_Union.m_Double);
		}
		else {
			throw(tExceptionBadType("tVariant not double"));
		}
	}

	void tVariant::SetString(tString sValue) { Clear(); m_Type = tVariantType::t_string;  m_Union.m_String = new tSharedString(sValue); }
	tString tVariant::String() const {
		if (m_Type == tVariantType::t_string) {
			return(m_Union.m_String->Str());
		}
		else {
			throw(tExceptionBadType("tVariant not string"));
		}
	}

	void tVariant::SetChar(tChar* sValue) { Clear(); m_Type = tVariantType::t_string;  m_Union.m_String = new tSharedString(tString(sValue)); }
	
	void tVariant::SetError(tClassError sValue) { Clear(); m_Type = tVariantType::t_error; m_Union.m_Error = new tClassError(sValue); }
	tClassError tVariant::Error() const {
		if (m_Type == tVariantType::t_error) {
			return(*m_Union.m_Error);
		}
		else {
			throw(tExceptionBadType("tVariant not error"));
		}
	}

	void tVariant::SetDate(tDate sValue) { Clear(); m_Type = tVariantType::t_date;  m_Union.m_Date = sValue; }
	tDate tVariant::Date() const {
		if (m_Type == tVariantType::t_date) {
			return(m_Union.m_Date);
		}
		else {
			throw(tExceptionBadType("tVariant not date"));
		}
	}

	void tVariant::SetClass(tVirtualClass* sValue) { 
		Clear(); 
		if (sValue != nullptr) {
			m_Type = tVariantType::t_class;  
			m_Union.m_Class = sValue;
		} else {
			m_Type = tVariantType::t_null;
		}
	}
	tVirtualClass* tVariant::Class() const {
		if (m_Type == tVariantType::t_class) {
			return(m_Union.m_Class);
		}
		else {
			throw(tExceptionBadType("tVariant not class"));
		}
	}

	tShort tVariant::Extra() const { return(m_Extra); }
	void tVariant::Extra(tShort sValue) { m_Extra = sValue; }

    tBool tVariant::IsNull() const { return(m_Type==tVariantType::t_null); }
    tBool tVariant::IsInt() const { return(m_Type==tVariantType::t_int); }
    tBool tVariant::IsBool() const { return(m_Type==tVariantType::t_bool); }
    tBool tVariant::IsDouble() const { return(m_Type==tVariantType::t_double); }
    tBool tVariant::IsString() const { return(m_Type==tVariantType::t_string); }
    tBool tVariant::IsError() const { return(m_Type==tVariantType::t_error); }
    tBool tVariant::IsDate() const { return(m_Type==tVariantType::t_date); }

    tBool tVariant::IsClass() const { return(m_Type==tVariantType::t_class); }

    tBool tVariant::IsNumeric() const { return((m_Type==tVariantType::t_int) || (m_Type==tVariantType::t_double)); }
    tBool tVariant::IsNumericOrNull() const {
        return((m_Type==tVariantType::t_null) || (m_Type==tVariantType::t_int) || (m_Type==tVariantType::t_double));
    }
    tDouble tVariant::Numeric() const {
        switch (m_Type) {
            case tVariantType::t_int: return(m_Union.m_Int); break;
            case tVariantType::t_double: return(m_Union.m_Double); break;
            default: {
                throw(tExceptionBadType("tVariant not numeric"));
            }
        }
    }
    tDouble tVariant::NumericOrNull() const {
		switch (m_Type) {
            case tVariantType::t_null: return(0.0); break;
            case tVariantType::t_int: return(m_Union.m_Int); break;
            case tVariantType::t_double: return(m_Union.m_Double); break;
            default: {
                throw(tExceptionBadType("tVariant not numeric or null"));
            }
        }
    }
        
    tBool tVariant::IsExcelNull() const {
        switch (m_Type) {
            case tVariantType::t_null: return(true);
            case tVariantType::t_string: return(m_Union.m_String->Str()=="");
            default: {
                return(false);
            }
        }
    }
    tBool tVariant::HasFormula() const {
        if (m_Type == tVariantType::t_string) {
            if (String().length() > 0) {
                if (String()[0] == '=') {
                    return(true);
                }
            }
        }
        return(false);
    }

	tVariantType tVariant::Parse(tString sValue) {
		tVariantType wResult = tVariantType::t_null;
        
        tLocale* wLocale=tApplication::Instance()->Locale();
        const tChar wDecimalSeparator=wLocale->Decimal();
        const tChar wDateSeparator=wLocale->Date();
        const tChar wHourSeparator=wLocale->Time();
        
        tByte wIsDate=0;
        tByte wIsHour=0;
        
        tLexer wLex(sValue.c_str());
        wLex.SeparatorDecimal(wLocale->Decimal());
        
        tLexerToken wLexerToken;
        tByte wNbItem=0;
        // Loop until End or Unexpected caracter ===============================
        for (wLexerToken = wLex.next();!wLexerToken.is_one_of(tKind::End, tKind::Unexpected); wLexerToken = wLex.next()) {
            tBool wExit=false;
            switch (wLexerToken.Kind()) {
                case tKind::Integer:
                    wResult = tVariantType::t_int;
                    break;
                case tKind::Float: {
                    wResult = tVariantType::t_double;
                    break;
                }
                case tKind::Bool: {
                    wResult = tVariantType::t_bool;
                    break;
                }
                case tKind::End:
                    wExit=true;
                    break;
                default:
                    // minus
                    if ((wNbItem==0) && (wLexerToken.Kind()==tKind::Minus)) {
                        break;
                    }
                    // Search Separator ===========================
                    if (*wLexerToken.Begin()==wDateSeparator) {
                        wIsDate++;
                    } else {
                        if (*wLexerToken.Begin()==wHourSeparator) {
                            wIsHour++;
                        } else {
                            wResult = tVariantType::t_string;
                            wExit=true;
                        }
                    }
                    
                    break;
            }
            wNbItem++;
            if (wExit) break;
        }
        if ((wIsDate==0) && (wIsHour>0) && (wIsHour<3)) {
            if (wIsHour==1) sValue+=":00";
            tString wFormat= wLocale->FormatTimeLong();
            tClassDate wClassDate;
            if (wClassDate.ParseDateTime(sValue.c_str(),wFormat.c_str())) {
                SetDate(wClassDate.Value());
                return(tVariantType::t_date);
            }
        }
        // Date ====================================================
        if (wIsDate==2) {
            tClassDate wClassDate;
            // Hour + Date
            if (wIsHour>0) {
                if (wIsHour==1) sValue+=":00";
                // Date Long + Hour
                tString wFormat= wLocale->FormatDateLong()+" "+wLocale->FormatTimeLong();
                if (wClassDate.ParseDateTime(sValue.c_str(),wFormat.c_str())) {
                    SetDate(wClassDate.Value());
                    return(tVariantType::t_date);
                }
                // Date Short + Hour
                wFormat= wLocale->FormatDateShort()+" "+wLocale->FormatTimeLong();
                if (wClassDate.ParseDateTime(sValue.c_str(),wFormat.c_str())) {
                    SetDate(wClassDate.Value());
                    return(tVariantType::t_date);
                }
            } else {
                // Date Long
                if (wClassDate.ParseDateTime(sValue.c_str(),wLocale->FormatDateLong().c_str())) {
                    SetDate(wClassDate.Value());
                    return(tVariantType::t_date);
                }
                // Date Short
                if (wClassDate.ParseDateTime(sValue.c_str(),wLocale->FormatDateShort().c_str())) {
                    SetDate(wClassDate.Value());
                    return(tVariantType::t_date);
                }
            }
            wResult = tVariantType::t_string;
        }
    
		switch (wResult) {
		case tVariantType::t_int: {
			// MAXINT "2 147 483 647"
            try {
                SetInt(stoi(sValue.c_str()));
            }
            catch(std::invalid_argument& ){
              // if no conversion could be performed
            }
            catch(std::out_of_range& ){
                SetDouble(stod(sValue.c_str()));
           }
            catch(...) {
              // everything else
            }
			break;
		}
		case tVariantType::t_double: {
            // Change Decimal if not US
            if (wDecimalSeparator!='.')
                std::replace( sValue.begin(), sValue.end(), wDecimalSeparator, '.'); // replace all 'x' to 'y'
			SetDouble(stod(sValue.c_str()));
			break;
		}
        case tVariantType::t_bool: {
            if (sValue=="true") SetBool(true);
            if (sValue=="false") SetBool(false);
            break;
        }
        default:
            SetString(sValue);
            break;
        }

		return(wResult);
	};
        
    tString tVariant::Formula() const {
        if (m_Type == tVariantType::t_string) {
            tString wValue = String();
            if (wValue.length() > 0) {
                if (wValue[0] == '=') {
                    wValue.erase(0, 1);
                    return(wValue);
                }
            }
        }
        return("");
    }

    tString tVariant::FormatString(tFormatString* sFormatString) const {
        #ifdef DebugFormatString
            cout << "tVariant::FormatString(" << sFormatString  << "," << Str()  << ")" << endl;
        #endif
        tStringStream wStream;
        switch (m_Type) {
        case tVariantType::t_null: break; // os << "null"; break;
        case tVariantType::t_int:  {
            tClassInt wClassInt(Int());
            wStream << wClassInt.FormatString(sFormatString);
            break;
        }
        case tVariantType::t_bool: if (Bool()) { wStream << "true"; }
                                    else { wStream << "false"; }    break;
        case tVariantType::t_double: {
            tClassDouble wClassDouble(Double());
            wStream << wClassDouble.FormatString(sFormatString);
            break;
        }
        case tVariantType::t_string: wStream << String(); break;
        case tVariantType::t_error: wStream << Error().Error(); break;
        case tVariantType::t_date: {
            tClassDate wDate(Date());
            wStream << wDate.FormatString(sFormatString);
            break;
        }
        case tVariantType::t_class: wStream << "Class"; break;
        }
        return(wStream.str());
    }

    tString tVariant::InputString(tFormatString* sFormatString) const {
        #ifdef DebugFormatString
            cout << "tVariant::InputString(" << sFormatString  << "," << Str()  << ")" << endl;
        #endif
        tStringStream wStream;
        switch (m_Type) {
        case tVariantType::t_null: break; // os << "null"; break;
        case tVariantType::t_int:  {
            tStringStream wStreamDouble;
            wStreamDouble << std::fixed << std::setprecision(sFormatString->Decimal()) << tDouble(m_Union.m_Int);
            tString wResult= wStreamDouble.str();
            std::replace(wResult.begin(), wResult.end(), '.', tApplication::Instance()->Locale()->Decimal());
            wStream << wResult;
            break;
        }
        case tVariantType::t_bool: if (Bool()) { wStream << "true"; }
                                    else { wStream << "false"; }    break;
        case tVariantType::t_double: {
            tStringStream wStreamDouble;
            wStreamDouble << std::fixed << std::setprecision(GetNbDecimal(Double())) << m_Union.m_Double;
            tString wResult= wStreamDouble.str();
            std::replace(wResult.begin(), wResult.end(), '.', tApplication::Instance()->Locale()->Decimal());
            wStream << wResult;
            break;
        }
        case tVariantType::t_string: wStream << String(); break;
        case tVariantType::t_error: wStream << Error().Error(); break;
        case tVariantType::t_date: {
            tClassDate wDate(Date());
            tFormatString wResolved;
            if (sFormatString != nullptr) {
                wResolved = *sFormatString;
            }
            ResolveDateInputFormat(const_cast<tVariant*>(this), wResolved);
            tString wOut = wDate.FormatString(&wResolved);
            if (wOut.empty() || wOut == "None") {
                wResolved.FormatType(tFormatStringType::none);
                wResolved.SetDefaultFormat(const_cast<tVariant*>(this));
                wOut = wDate.FormatString(&wResolved);
                if (wOut.empty() || wOut == "None") {
                    wOut = wDate.UsDate();
                }
            }
            wStream << wOut;
            break;
        }
        case tVariantType::t_class: wStream << "Error Class"; break;
        }
        return(wStream.str());
    }

	// Json ===============================================================
	void tVariant::Json(Writer<StringBuffer>* sWriter) const {
		sWriter->Key("t"); 
		switch (Type()) {
		case tVariantType::t_null: sWriter->String("n"); break;
		case tVariantType::t_int:  sWriter->String("i"); break; 
		case tVariantType::t_bool: sWriter->String("b"); break; 
		case tVariantType::t_double: sWriter->String("d"); break; 
		case tVariantType::t_string: sWriter->String("s"); break;
		case tVariantType::t_error: sWriter->String("e"); break;
		case tVariantType::t_date: sWriter->String("da"); break;
		case tVariantType::t_class: sWriter->String("c"); break; 
		}
		sWriter->Key("v");
		switch (Type()) {
		case tVariantType::t_null: sWriter->Null();  break; // os << "null"; break;
		case tVariantType::t_int:  sWriter->Int(Int()); break;
		case tVariantType::t_bool: sWriter->Bool(Bool()); break;
		case tVariantType::t_double: sWriter->Double(Double()); break;
		case tVariantType::t_string: sWriter->String(String().c_str()); break;
		case tVariantType::t_error: Error().Json(sWriter);  break;
		case tVariantType::t_date: {
			tClassDate wDate(Date());
			sWriter->String(wDate.UsDate().c_str());
			break;
		}
		case tVariantType::t_class: {
			tVirtualClass* wVirtualClass = m_Union.m_Class;
			if (wVirtualClass != nullptr) {
				sWriter->StartObject();
				tString wName = wVirtualClass->ClassName();
				sWriter->Key("n");
				sWriter->String(wName.c_str());
                // Do we save the model with the instance data?
                tModelClass* wModelClass = tClassFactory::Instance()->Get(wName);
                if (wModelClass != nullptr) {
                    if (wModelClass->SaveModel()) {
                        sWriter->Key("o");
                        wModelClass->JsonAssociated(sWriter, wVirtualClass);
                    } else {
                        // Save Instance Data ?
                        if (wModelClass->SaveData()) {
                            sWriter->Key("v");
                            wVirtualClass->Json(sWriter);
                        };
                    }
                }
                sWriter->EndObject();
			}
			break;
		}
		}
        // Formula bytecode: function name is t_string; Extra holds argument count (must persist 0 for TODAY(), etc.).
        // Other types keep legacy rule — default m_Extra can be 0 after Clear/SetDate and must not emit "e" (see tests).
        if (m_Type == tVariantType::t_string) {
            if (m_Extra != -1) {
                sWriter->Key("e");
                sWriter->Int(m_Extra);
            }
        } else if (m_Extra != -1 && m_Extra != 0) {
            sWriter->Key("e");
            sWriter->Int(m_Extra);
        }
	}

	void tVariant::JsonClass(const rapidjson::Value& sValue) {
		const Value& wClassNameValue = sValue["n"];
		assert(wClassNameValue.IsString());
		tString wClassName = wClassNameValue.GetString();
		//cout << wClassName << endl;
		tVirtualClass* wClass = tClassFactory::Instance()->Create(wClassName);
        if (wClass==nullptr) {
            tStringStream wStream;
            wStream << "tVariant::JsonClass Class " <<wClassName << " not in factory class !";
            cout << wStream.str();
            throw(tExceptionInternalError(wStream.str()));
        }
        if (sValue.HasMember("o")) {
            tModelClass* wModelClass = tClassFactory::Instance()->Get(wClassName);
            if (wModelClass!=nullptr)
                wModelClass->JsonAssociated(sValue["o"], wClass);
        }
        if (sValue.HasMember("v")) {
            wClass->Json(sValue["v"]);
        }
     
        Clear();
        
		m_Type = tVariantType::t_class;
		m_Union.m_Class = wClass;
	}

	void tVariant::Json(const rapidjson::Value& sValue) {
		const Value& wTypeValue = sValue["t"];
		assert(wTypeValue.IsString());
		tString wTypeStr = wTypeValue.GetString();

		tVariantType wType = tVariantType::t_null;

		//if (wTypeStr == "n");
		if (wTypeStr == "i") wType = tVariantType::t_int;
		if (wTypeStr == "b") wType = tVariantType::t_bool;
		if (wTypeStr == "d") wType = tVariantType::t_double;
		if (wTypeStr == "s") wType = tVariantType::t_string;
		if (wTypeStr == "e") wType = tVariantType::t_error;
		if (wTypeStr == "da") wType = tVariantType::t_date;
		if (wTypeStr == "c") wType = tVariantType::t_class;

		const Value& wValue = sValue["v"];

		tClassError wError;
		tClassDate wClassDate;

		switch (wType) {
		case SkRoot::tVariantType::t_null:
			break;
		case SkRoot::tVariantType::t_int:
			SetInt(wValue.GetInt());
			break;
		case SkRoot::tVariantType::t_bool:
			SetBool(wValue.GetBool());
			break;
		case SkRoot::tVariantType::t_double:
			SetDouble(wValue.GetDouble());
			break;
		case SkRoot::tVariantType::t_string:
			SetString(wValue.GetString());
			break;
		case SkRoot::tVariantType::t_error:
			Clear();
			wError.Json(wValue);
			SetError(wError);
			break;
		case SkRoot::tVariantType::t_date:
			wClassDate.UsDate(wValue.GetString());
			SetDate(wClassDate.Value());
			break;
		case SkRoot::tVariantType::t_class:
			JsonClass(wValue);
			break;
		default:
			break;
		}
        if (sValue.HasMember("e")) {
            m_Extra=sValue["e"].GetInt();
        }

	}
    
    void tVariant::JsonJavaScript(Writer<StringBuffer>* sWriter) const {
        sWriter->Key("c_t");
        switch (Type()) {
        case tVariantType::t_null: sWriter->String("n"); break;
        case tVariantType::t_int:  sWriter->String("i"); break;
        case tVariantType::t_bool: sWriter->String("b"); break;
        case tVariantType::t_double: sWriter->String("d"); break;
        case tVariantType::t_string: sWriter->String("s"); break;
        case tVariantType::t_error: sWriter->String("e"); break;
        case tVariantType::t_date: sWriter->String("da"); break;
        case tVariantType::t_class: sWriter->String("c"); break;
        }
        sWriter->Key("c_v");
        switch (Type()) {
        case tVariantType::t_null: sWriter->Null();  break; // os << "null"; break;
        case tVariantType::t_int:  sWriter->Int(Int()); break;
        case tVariantType::t_bool: sWriter->Bool(Bool()); break;
        case tVariantType::t_double: sWriter->Double(Double()); break;
        case tVariantType::t_string: sWriter->String(String().c_str()); break;
        case tVariantType::t_error: sWriter->String(Error().Error().c_str());  break;
        case tVariantType::t_date: {
            tClassDate wDate(Date());
            sWriter->String(wDate.UsDate().c_str());
            break;
        }
        case tVariantType::t_class: {
            tVirtualClass* wVirtualClass = m_Union.m_Class;
            if (wVirtualClass != nullptr) {
                sWriter->StartObject();
                tString wName = wVirtualClass->ClassName();
                sWriter->Key("n");
                sWriter->String(wName.c_str());
                if (wVirtualClass->IsReactComponent()) {
                    sWriter->Key("co");
                    sWriter->Bool(true);
                }
                sWriter->Key("c");
                if (wVirtualClass->IsJsonJavaScript()) {
                    wVirtualClass->JsonJavaScript(sWriter);
                } else {
                    wVirtualClass->Json(sWriter);
                }
                sWriter->EndObject();
            }
            else {
                sWriter->Null(); break;
            }
            break;
        }
        }
    }

	tVariant tVariant::operator = (const tVariant& sVariant) { Assign(sVariant);  return(*this);  }
	
	tBool tVariant::operator == (const tVariant& sVariant) const {
		// Cas Particulier int double
		if ((m_Type == tVariantType::t_int) && (sVariant.m_Type == tVariantType::t_double)) {
			return(m_Union.m_Int == sVariant.m_Union.m_Double);
		}
		if ((m_Type == tVariantType::t_double) && (sVariant.m_Type == tVariantType::t_int)) {
			return(m_Union.m_Double == sVariant.m_Union.m_Int);
		}
        // Null ================================================================
        // Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
        if (m_Type==tVariantType::t_null) {
            if (sVariant.m_Type==tVariantType::t_int) {
                return(0 == sVariant.m_Union.m_Int);
            }
            if (sVariant.m_Type==tVariantType::t_double) {
                return(0.0 == sVariant.m_Union.m_Double);
            }
            if (sVariant.m_Type==tVariantType::t_string) {
                return("" == sVariant.m_Union.m_String->Str());
            }
            if (sVariant.m_Type==tVariantType::t_bool) {
                return(false == sVariant.m_Union.m_Bool);
            }
        }
        if (sVariant.m_Type==tVariantType::t_null) {
            if (m_Type==tVariantType::t_int) {
                return(m_Union.m_Int == 0);
            }
            if (m_Type==tVariantType::t_double) {
                return(m_Union.m_Double == 0.0);
            }
            if (m_Type==tVariantType::t_string) {
                return(m_Union.m_String->Str() == "");
            }
            if (m_Type==tVariantType::t_bool) {
                return(m_Union.m_Bool == false);
            }
        }

		if (m_Type != sVariant.Type()) return(false);
		switch (m_Type) {
		case tVariantType::t_null: return(true);
		case tVariantType::t_int: return(m_Union.m_Int == sVariant.m_Union.m_Int);
		case tVariantType::t_bool: return(m_Union.m_Bool == sVariant.m_Union.m_Bool);
		case tVariantType::t_double: return(m_Union.m_Double == sVariant.m_Union.m_Double);
		case tVariantType::t_string: return((*m_Union.m_String)() == (*sVariant.m_Union.m_String)());
		case tVariantType::t_date: return(m_Union.m_Date == sVariant.m_Union.m_Date);
		case tVariantType::t_error: return(*m_Union.m_Error == *sVariant.m_Union.m_Error);
		case tVariantType::t_class: return((*m_Union.m_Class) == *(sVariant.m_Union.m_Class));
        default: break;
		}
		return(false);
	}
	// operator!= delegates to == so the two stay in sync (notably the int<->double
	// and null<->scalar coercions).  Defined here rather than inline in the header
	// so a future refactor of operator== can't accidentally desync the two.
	tBool tVariant::operator != (const tVariant& sVariant) const {
		return(!(*this == sVariant));
	}
	tBool tVariant::operator < (const tVariant& sVariant) const {
		// Cas Particulier int double
		if ((m_Type == tVariantType::t_int) && (sVariant.m_Type == tVariantType::t_double)) {
			return(tDouble(m_Union.m_Int) < sVariant.m_Union.m_Double);
		}
		if ((m_Type == tVariantType::t_double) && (sVariant.m_Type == tVariantType::t_int)) {
			return(m_Union.m_Double < tDouble(sVariant.m_Union.m_Int));
		}

        // Null ================================================================
        // Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
        if (m_Type==tVariantType::t_null) {
            if (sVariant.m_Type==tVariantType::t_int) {
                return(0 < sVariant.m_Union.m_Int);
            }
            if (sVariant.m_Type==tVariantType::t_double) {
                return(0.0 < sVariant.m_Union.m_Double);
            }
            if (sVariant.m_Type==tVariantType::t_string) {
                return("" < sVariant.m_Union.m_String->Str());
            }
            if (sVariant.m_Type==tVariantType::t_bool) {
                return(0 < (sVariant.m_Union.m_Bool ? 1 : 0)); // Convert bool to int for comparison
            }
        }
        if (sVariant.m_Type==tVariantType::t_null) {
            if (m_Type==tVariantType::t_int) {
                return(m_Union.m_Int < 0);
            }
            if (m_Type==tVariantType::t_double) {
                return(m_Union.m_Double < 0.0);
            }
            if (m_Type==tVariantType::t_string) {
                return(m_Union.m_String->Str() < "");
            }
            if (m_Type==tVariantType::t_bool) {
                return((m_Union.m_Bool ? 1 : 0) < 0); // Convert bool to int for comparison
            }
        }
		if (m_Type != sVariant.Type()) return(m_Type < sVariant.Type());
		switch (m_Type) {
		// null < null is false : strict ordering is irreflexive.
		case tVariantType::t_null: return(false);
		case tVariantType::t_int: return(m_Union.m_Int < sVariant.m_Union.m_Int);
		case tVariantType::t_bool: return(m_Union.m_Bool < sVariant.m_Union.m_Bool);
		case tVariantType::t_double: return(m_Union.m_Double < sVariant.m_Union.m_Double);
		case tVariantType::t_string: return((*m_Union.m_String)() < (*sVariant.m_Union.m_String)());
		case tVariantType::t_date: return(m_Union.m_Date < sVariant.m_Union.m_Date);
		case tVariantType::t_error: return(m_Union.m_Error < sVariant.m_Union.m_Error);
		case tVariantType::t_class: return(false);
        default: break;
		}
		return(false);
	}
	tBool tVariant::operator <= (const tVariant& sVariant) const {
        // Cas Particulier int double
        if ((m_Type == tVariantType::t_int) && (sVariant.m_Type == tVariantType::t_double)) {
            return(tDouble(m_Union.m_Int) <= sVariant.m_Union.m_Double);
        }
        if ((m_Type == tVariantType::t_double) && (sVariant.m_Type == tVariantType::t_int)) {
            return(m_Union.m_Double <= tDouble(sVariant.m_Union.m_Int));
        }
        // Null ================================================================
        // Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
        if (m_Type==tVariantType::t_null) {
            if (sVariant.m_Type==tVariantType::t_int) {
                return(0 <= sVariant.m_Union.m_Int);
            }
            if (sVariant.m_Type==tVariantType::t_double) {
                return(0.0 <= sVariant.m_Union.m_Double);
            }
            if (sVariant.m_Type==tVariantType::t_string) {
                return("" <= sVariant.m_Union.m_String->Str());
            }
            if (sVariant.m_Type==tVariantType::t_bool) {
                return(0 <= (sVariant.m_Union.m_Bool ? 1 : 0)); // Convert bool to int for comparison
            }
        }
        if (sVariant.m_Type==tVariantType::t_null) {
            if (m_Type==tVariantType::t_int) {
                return(m_Union.m_Int <= 0);
            }
            if (m_Type==tVariantType::t_double) {
                return(m_Union.m_Double <= 0.0);
            }
            if (m_Type==tVariantType::t_string) {
                return(m_Union.m_String->Str() <= "");
            }
            if (m_Type==tVariantType::t_bool) {
                return((m_Union.m_Bool ? 1 : 0) <= 0); // Convert bool to int for comparison
            }
        }
        if (m_Type != sVariant.Type()) return(m_Type < sVariant.Type());
        switch (m_Type) {
        case tVariantType::t_null: return(true);
        case tVariantType::t_int: return(m_Union.m_Int <= sVariant.m_Union.m_Int);
        case tVariantType::t_bool: return(m_Union.m_Bool <= sVariant.m_Union.m_Bool);
        case tVariantType::t_double: return(m_Union.m_Double <= sVariant.m_Union.m_Double);
        case tVariantType::t_string: return((*m_Union.m_String)() <= (*sVariant.m_Union.m_String)());
        case tVariantType::t_date: return(m_Union.m_Date <= sVariant.m_Union.m_Date);
        case tVariantType::t_error: return(m_Union.m_Error <= sVariant.m_Union.m_Error);
        case tVariantType::t_class: return(false);
        default: break;
        }
        return(false);
	}

	tBool tVariant::operator > (const tVariant& sVariant) const {
        // Cas Particulier int double
        if ((m_Type == tVariantType::t_int) && (sVariant.m_Type == tVariantType::t_double)) {
            return(tDouble(m_Union.m_Int) > sVariant.m_Union.m_Double);
        }
        if ((m_Type == tVariantType::t_double) && (sVariant.m_Type == tVariantType::t_int)) {
            return(m_Union.m_Double > tDouble(sVariant.m_Union.m_Int));
        }
        // Null ================================================================
        // Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
        if (m_Type==tVariantType::t_null) {
            if (sVariant.m_Type==tVariantType::t_int) {
                return(0 > sVariant.m_Union.m_Int);
            }
            if (sVariant.m_Type==tVariantType::t_double) {
                return(0.0 > sVariant.m_Union.m_Double);
            }
            if (sVariant.m_Type==tVariantType::t_string) {
                return("" > sVariant.m_Union.m_String->Str());
            }
            if (sVariant.m_Type==tVariantType::t_bool) {
                return(0 > (sVariant.m_Union.m_Bool ? 1 : 0)); // Convert bool to int for comparison
            }
        }
        if (sVariant.m_Type==tVariantType::t_null) {
            if (m_Type==tVariantType::t_int) {
                return(m_Union.m_Int > 0);
            }
            if (m_Type==tVariantType::t_double) {
                return(m_Union.m_Double > 0.0);
            }
            if (m_Type==tVariantType::t_string) {
                return(m_Union.m_String->Str() > "");
            }
            if (m_Type==tVariantType::t_bool) {
                return((m_Union.m_Bool ? 1 : 0) > 0); // Convert bool to int for comparison
            }
        }
        if (m_Type != sVariant.Type()) return(m_Type > sVariant.Type());
        switch (m_Type) {
        case tVariantType::t_null: return(false);
        case tVariantType::t_int: return(m_Union.m_Int > sVariant.m_Union.m_Int);
        case tVariantType::t_bool: return(m_Union.m_Bool > sVariant.m_Union.m_Bool);
        case tVariantType::t_double: return(m_Union.m_Double > sVariant.m_Union.m_Double);
        case tVariantType::t_string: return((*m_Union.m_String)() > (*sVariant.m_Union.m_String)());
        case tVariantType::t_date: return(m_Union.m_Date > sVariant.m_Union.m_Date);
        case tVariantType::t_error: return(m_Union.m_Error > sVariant.m_Union.m_Error);
        case tVariantType::t_class: return(false);
        default: break;
        }
        return(false);
    }
	tBool tVariant::operator >= (const tVariant& sVariant) const {
        // Cas Particulier int double
        if ((m_Type == tVariantType::t_int) && (sVariant.m_Type == tVariantType::t_double)) {
            return(tDouble(m_Union.m_Int) >= sVariant.m_Union.m_Double);
        }
        if ((m_Type == tVariantType::t_double) && (sVariant.m_Type == tVariantType::t_int)) {
            return(m_Union.m_Double >= tDouble(sVariant.m_Union.m_Int));
        }
        // Null ================================================================
        // Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
        if (m_Type==tVariantType::t_null) {
            if (sVariant.m_Type==tVariantType::t_int) {
                return(0 >= sVariant.m_Union.m_Int);
            }
            if (sVariant.m_Type==tVariantType::t_double) {
                return(0.0 >= sVariant.m_Union.m_Double);
            }
            if (sVariant.m_Type==tVariantType::t_string) {
                return("" >= sVariant.m_Union.m_String->Str());
            }
            if (sVariant.m_Type==tVariantType::t_bool) {
                return(0 >= (sVariant.m_Union.m_Bool ? 1 : 0)); // Convert bool to int for comparison
            }
        }
        if (sVariant.m_Type==tVariantType::t_null) {
            if (m_Type==tVariantType::t_int) {
                return(m_Union.m_Int >= 0);
            }
            if (m_Type==tVariantType::t_double) {
                return(m_Union.m_Double >= 0.0);
            }
            if (m_Type==tVariantType::t_string) {
                return(m_Union.m_String->Str() >= "");
            }
            if (m_Type==tVariantType::t_bool) {
                return((m_Union.m_Bool ? 1 : 0) >= 0); // Convert bool to int for comparison
            }
        }

        if (m_Type != sVariant.Type()) return(m_Type > sVariant.Type());
        switch (m_Type) {
        // null >= null is true : a value is always >= itself (reflexivity).
        case tVariantType::t_null: return(true);
        case tVariantType::t_int: return(m_Union.m_Int >= sVariant.m_Union.m_Int);
        case tVariantType::t_bool: return(m_Union.m_Bool >= sVariant.m_Union.m_Bool);
        case tVariantType::t_double: return(m_Union.m_Double >= sVariant.m_Union.m_Double);
        case tVariantType::t_string: return((*m_Union.m_String)() >= (*sVariant.m_Union.m_String)());
        case tVariantType::t_date: return(m_Union.m_Date >= sVariant.m_Union.m_Date);
        case tVariantType::t_error: return(m_Union.m_Error >= sVariant.m_Union.m_Error);
        case tVariantType::t_class: return(false);
        default: break;
        }
        return(false);
	}

	tBool tVariant::operator || (const tVariant& sVariant) const {
		// Treat Null as false
		tBool wLeft = false;
		tBool wRight = false;
		
		if (m_Type == tVariantType::t_bool) {
			wLeft = Bool();
		} else if (m_Type == tVariantType::t_null) {
			wLeft = false;
		}
		
		if (sVariant.m_Type == tVariantType::t_bool) {
			wRight = sVariant.Bool();
		} else if (sVariant.m_Type == tVariantType::t_null) {
			wRight = false;
		}
		
		return(wLeft || wRight);
	}
	tBool tVariant::operator && (const tVariant& sVariant) const {
		// Treat Null as false
		tBool wLeft = false;
		tBool wRight = false;
		
		if (m_Type == tVariantType::t_bool) {
			wLeft = Bool();
		} else if (m_Type == tVariantType::t_null) {
			wLeft = false;
		}
		
		if (sVariant.m_Type == tVariantType::t_bool) {
			wRight = sVariant.Bool();
		} else if (sVariant.m_Type == tVariantType::t_null) {
			wRight = false;
		}
		
		return(wLeft && wRight);
	}
	tBool tVariant::operator ! () const {
		// Truthiness rules consistent with Excel / JavaScript :
		//   null            -> falsy   -> !v == true
		//   bool false      -> falsy   -> !v == true
		//   int 0           -> falsy   -> !v == true
		//   double 0.0/NaN  -> falsy   -> !v == true (NaN is conventionally falsy)
		//   string ""       -> falsy   -> !v == true
		//   anything else   -> truthy  -> !v == false
		switch (m_Type) {
			case tVariantType::t_null:   return(true);
			case tVariantType::t_bool:   return(!Bool());
			case tVariantType::t_int:    return(m_Union.m_Int == 0);
			case tVariantType::t_double: return(m_Union.m_Double == 0.0 || m_Union.m_Double != m_Union.m_Double);
			case tVariantType::t_string: return(m_Union.m_String == nullptr || m_Union.m_String->Str() == "");
			default: break;
		}
		// date / error / class : non-empty container, considered truthy.
		return(false);
	}

	// Fonction amie ============================================================
	tVariant operator+(const tVariant& sVariant1,const tVariant& sVariant2) {
		tVariant wResult;
		
		// Propagate errors immediately - if either operand is an error, return the error
		if (sVariant1.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant1.m_Union.m_Error);
			return(wResult);
		}
		if (sVariant2.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant2.m_Union.m_Error);
			return(wResult);
		}
		
		if (sVariant1.m_Type == tVariantType::t_class) { return(sVariant1.m_Union.m_Class->Operator_plus(true,sVariant2)); }
		if (sVariant2.m_Type == tVariantType::t_class) { return(sVariant2.m_Union.m_Class->Operator_plus(false,sVariant1)); }

		switch (sVariant1.m_Type) {
		case tVariantType::t_null: {
			// Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt(0); break; // 0 + 0 = 0
			case tVariantType::t_int: wResult.SetInt(0 + sVariant2.m_Union.m_Int); break;
			case tVariantType::t_bool: wResult.SetInt(0 + (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(0.0 + sVariant2.m_Union.m_Double); break;
			case tVariantType::t_string: wResult.SetString("" + sVariant2.m_Union.m_String->Str()); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				// Boxing for calculate day+
				tClassDate wClassDate(sVariant2.m_Union.m_Date);
				wResult.SetDate(wClassDate());
                break;
			}
            default: break;
			}
			break;

		}
		case tVariantType::t_int: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt(sVariant1.m_Union.m_Int + 0); break;
			case tVariantType::t_int: {
				// This code checks for integer overflow and underflow before performing the addition.
				// If an overflow or underflow would occur, the result is stored as a double instead of an int.
				tInt w1 = sVariant1.m_Union.m_Int;
				tInt w2 = sVariant2.m_Union.m_Int;
				if (((w2 > 0) && (w1 > INT_MAX - w2)) || /* would overflow */ 
				    ((w2 < 0) && (w1 < INT_MIN - w2)))   /* would underflow */ {
					// Cast first: `w1 + w2` as tInt wraps (UB) then stores the wrapped value.
					wResult.SetDouble(static_cast<tDouble>(w1) + static_cast<tDouble>(w2));
				}
				else {
					wResult.SetInt(w1 + w2);
				}
				break;
			}
				
			// int + bool widens to int (cohérent avec bool + int plus bas) — l'ancien
			// SetBool écrasait silencieusement le résultat numérique en TRUE/FALSE.
			case tVariantType::t_bool: wResult.SetInt(sVariant1.m_Union.m_Int + (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(tDouble(sVariant1.m_Union.m_Int) + sVariant2.m_Union.m_Double); break;
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				// Boxing for calculate day+
				tClassDate wClassDate(sVariant2.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate + sVariant1.m_Union.m_Int;
				wResult.SetDate(wClassDateResult()); break;
			}
            default: break;
			}
			break;
		}
		case tVariantType::t_bool: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) + 0); break;
			case tVariantType::t_int: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1:0) + sVariant2.m_Union.m_Int); break;
			// bool + bool : somme arithmétique (TRUE + TRUE = 2). L'ancien code
			// utilisait `&&`, qui faisait un ET logique au lieu de l'addition.
			case tVariantType::t_bool: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1:0) + (sVariant2.m_Union.m_Bool ? 1:0)); break;
			case tVariantType::t_double: wResult.SetDouble((sVariant1.m_Union.m_Bool ? 1 : 0) + sVariant2.m_Union.m_Double); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_double: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetDouble(sVariant1.m_Union.m_Double + 0.0); break;
			case tVariantType::t_int:	wResult.SetDouble(sVariant1.m_Union.m_Double + tDouble(sVariant2.m_Union.m_Int)); break;
			case tVariantType::t_bool: wResult.SetDouble(sVariant1.m_Union.m_Double  + (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(sVariant1.m_Union.m_Double + sVariant2.m_Union.m_Double); break;
            case tVariantType::t_date: {
                // double + date: add the *double* operand as day offset (Excel-style).
                tClassDate wClassDate(sVariant2.m_Union.m_Date);
                tClassDate wClassDateResult = wClassDate + sVariant1.m_Union.m_Double;
                wResult.SetDate(wClassDateResult());
                break;
            }

			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_string: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetString(sVariant1.m_Union.m_String->Str() + ""); break;
			case tVariantType::t_string:	wResult.SetString(sVariant1.m_Union.m_String->Str() + sVariant2.m_Union.m_String->Str()); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_error: {
			wResult.SetError(*sVariant1.m_Union.m_Error); break;
			break;
		}
		case tVariantType::t_date: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				// Treat Null as 0 days
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate + 0;
				wResult.SetDate(wClassDateResult());
				break;
			}
			case tVariantType::t_int: {
				// Boxing for calculate day 
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate + sVariant2.m_Union.m_Int;
				wResult.SetDate(wClassDateResult());
				break;
			}
                case tVariantType::t_double: {
                // Boxing for calculate day
                tClassDate wClassDate(sVariant1.m_Union.m_Date);
                tClassDate wClassDateResult = wClassDate + sVariant2.m_Union.m_Double;
                wResult.SetDate(wClassDateResult());
                break;
            }
			case tVariantType::t_bool: {
				// Boxing for calculate day 
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate + (sVariant2.m_Union.m_Bool ? 1 : 0);
				wResult.SetDate(wClassDateResult());
				break;
			}
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			// Like Excel
            case tVariantType::t_date: {
                tClassDate wClassDate(sVariant1.m_Union.m_Date);
                tClassDate wClassDateRight(sVariant2.m_Union.m_Date);
                tClassDate wClassDateResult = wClassDate + wClassDateRight;
                wResult.SetDate(wClassDateResult());
                break;
            }
            default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		default: break;
		}
		return(wResult);
	}

	// Excel "&" concatenation =================================================
	// Convert any non-error operand to its text representation and concatenate.
	// This is what differentiates "&" from "+": "+" keeps strict numeric typing
	// (string + int -> #VALUE!), while "&" coerces every operand to a string.
	// Without this, formulas like ="YEAR ("&Year-1&")" return #VALUE! because
	// the formula engine routes Ampersand through operator+ on tVariant.
	// Strip trailing fractional zeros produced by std::fixed formatting (Excel & text).
	static tString StripFixedTrailingZeros(tString sValue) {
		const tSize wDot = sValue.find('.');
		if (wDot == tString::npos) {
			return sValue;
		}
		while (!sValue.empty() && sValue.back() == '0') {
			sValue.pop_back();
		}
		if (!sValue.empty() && sValue.back() == '.') {
			sValue.pop_back();
		}
		return sValue;
	}

	static tString VariantToText(const tVariant& sVariant) {
		switch (sVariant.Type()) {
		case tVariantType::t_null:
			return("");
		case tVariantType::t_int: {
			tStringStream wSs;
			wSs << sVariant.Int();
			return(wSs.str());
		}
		case tVariantType::t_bool:
			return(sVariant.Bool() ? tString("TRUE") : tString("FALSE"));
		case tVariantType::t_double: {
			// Excel "&" never emits scientific notation (e.g. 20701000 not 2.0701e+07).
			tStringStream wSs;
			wSs << std::fixed << std::setprecision(15) << sVariant.Double();
			tString wValue = StripFixedTrailingZeros(wSs.str());
			tLocale* wLocale = tApplication::Instance()->Locale();
			if ((wLocale != nullptr) && (wLocale->Decimal() != '.')) {
				std::replace(wValue.begin(), wValue.end(), '.', wLocale->Decimal());
			}
			return(wValue);
		}
		case tVariantType::t_string:
			return(sVariant.String());
		case tVariantType::t_date: {
			// Excel "&" on a date emits the underlying serial number, not a
			// formatted date. We mirror that by streaming the raw tDate value.
			tStringStream wSs;
			wSs << sVariant.Date();
			return(wSs.str());
		}
		case tVariantType::t_error:
			return(sVariant.Error().Error());
		case tVariantType::t_class:
			return("");
		}
		return("");
	}

	tVariant Ampersand(const tVariant& sVariant1, const tVariant& sVariant2) {
		tVariant wResult;
		// Errors propagate immediately, like the other arithmetic operators.
		if (sVariant1.Type() == tVariantType::t_error) {
			wResult.SetError(sVariant1.Error());
			return(wResult);
		}
		if (sVariant2.Type() == tVariantType::t_error) {
			wResult.SetError(sVariant2.Error());
			return(wResult);
		}
		wResult.SetString(VariantToText(sVariant1) + VariantToText(sVariant2));
		return(wResult);
	}

	tVariant operator-(const tVariant& sVariant1, const tVariant& sVariant2) {
		tVariant wResult;

		// Propagate errors immediately - if either operand is an error, return the error
		if (sVariant1.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant1.m_Union.m_Error);
			return(wResult);
		}
		if (sVariant2.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant2.m_Union.m_Error);
			return(wResult);
		}

		if (sVariant1.m_Type == tVariantType::t_class) { return(sVariant1.m_Union.m_Class->Operator_minus(true, sVariant2)); }
		if (sVariant2.m_Type == tVariantType::t_class) { return(sVariant2.m_Union.m_Class->Operator_minus(false, sVariant1)); }

		switch (sVariant1.m_Type) {
		case tVariantType::t_null: {
			// Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt(0 - 0); break; // 0 - 0 = 0
			case tVariantType::t_int: wResult.SetInt(0 - sVariant2.m_Union.m_Int); break;
			case tVariantType::t_bool: wResult.SetInt(0 - (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(0.0 - sVariant2.m_Union.m_Double); break;
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				// Boxing for calculate day 
				tClassDate wClassDate(sVariant2.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate - 0;
				wResult.SetDate(wClassDateResult()); break;
			}
            default: break;
			}
			break;
		}
		case tVariantType::t_int: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt(sVariant1.m_Union.m_Int - 0); break;
            case tVariantType::t_int: {
                // This code checks for integer overflow and underflow before performing the subtraction.
                // If an overflow or underflow would occur, the result is stored as a double instead of an int.
                tInt w1 = sVariant1.m_Union.m_Int;
                tInt w2 = sVariant2.m_Union.m_Int;
                // w1 - w2: underflow if subtracting a positive from too small,
                // overflow if subtracting a negative from too large.
                if ((w2 > 0 && w1 < INT_MIN + w2) || (w2 < 0 && w1 > INT_MAX + w2)) {
                    wResult.SetDouble(static_cast<tDouble>(w1) - static_cast<tDouble>(w2));
                }
                else {
                    wResult.SetInt(w1 - w2);
                }
                break;
            }
			case tVariantType::t_bool: wResult.SetInt(sVariant1.m_Union.m_Int - (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(tDouble(sVariant1.m_Union.m_Int) - sVariant2.m_Union.m_Double); break;
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				// Boxing for calculate day-
				tClassDate wClassDate(sVariant2.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate - sVariant1.m_Union.m_Int;
				wResult.SetDate(wClassDateResult()); break;
			}
            default: break;
			}
			break;
		}
		case tVariantType::t_bool: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) - 0); break;
			case tVariantType::t_int: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) - sVariant2.m_Union.m_Int); break;
			case tVariantType::t_bool: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) - (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble((sVariant1.m_Union.m_Bool ? 1 : 0) - sVariant2.m_Union.m_Double); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_double: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetDouble(sVariant1.m_Union.m_Double - 0.0); break;
			// double - int / double - bool : on garde la précision flottante.
			// L'ancienne version castait le double en int (tInt(1.5) == 1) et
			// perdait silencieusement la partie fractionnaire — 1.5 - 1 donnait 0.
			case tVariantType::t_int: wResult.SetDouble(sVariant1.m_Union.m_Double - tDouble(sVariant2.m_Union.m_Int)); break;
			case tVariantType::t_bool: wResult.SetDouble(sVariant1.m_Union.m_Double - tDouble(sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(sVariant1.m_Union.m_Double - sVariant2.m_Union.m_Double); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_string: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_error: {
			wResult.SetError(*sVariant1.m_Union.m_Error); break;
			break;
		}
		case tVariantType::t_date: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				// Treat Null as 0 days
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate - 0;
				wResult.SetDate(wClassDateResult());
				break;
			}
			case tVariantType::t_int: {
				// Boxing for calculate day-
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate - sVariant2.m_Union.m_Int;
				wResult.SetDate(wClassDateResult());
				break;
			}
			case tVariantType::t_bool: {
				// Boxing for calculate day 
				tClassDate wClassDate(sVariant1.m_Union.m_Date);
				tClassDate wClassDateResult = wClassDate - (sVariant2.m_Union.m_Bool ? 1 : 0);
				wResult.SetDate(wClassDateResult());
				break;
			}
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
            // Like Excel
            case tVariantType::t_date: {
                tClassDate wClassDate(sVariant1.m_Union.m_Date);
                tClassDate wClassDateRight(sVariant2.m_Union.m_Date);
                tClassDate wClassDateResult = wClassDate - wClassDateRight;
                wResult.SetDate(wClassDateResult());
                break;
            }
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
        default: break;
		}
		return(wResult);
	}

	tVariant operator*(const tVariant& sVariant1, const tVariant& sVariant2) {
		tVariant wResult;

		// Propagate errors immediately - if either operand is an error, return the error
		if (sVariant1.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant1.m_Union.m_Error);
			return(wResult);
		}
		if (sVariant2.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant2.m_Union.m_Error);
			return(wResult);
		}

		if (sVariant1.m_Type == tVariantType::t_class) { return(sVariant1.m_Union.m_Class->Operator_multiply(true, sVariant2)); }
		if (sVariant2.m_Type == tVariantType::t_class) { return(sVariant2.m_Union.m_Class->Operator_multiply(false, sVariant1)); }

		switch (sVariant1.m_Type) {
		case tVariantType::t_null: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null:wResult.SetInt(0); break;
			case tVariantType::t_int: wResult.SetInt(0); break;
			case tVariantType::t_bool: wResult.SetBool(0); break;
			case tVariantType::t_double: wResult.SetDouble(0); break;
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
            case tVariantType::t_date: wResult.Clear(); break;
            default: break;
			}
			break;
		}
		case tVariantType::t_int: {
			switch (sVariant2.m_Type) {
            case tVariantType::t_null: wResult.SetInt(sVariant1.m_Union.m_Int * 0); break; // Treat Null as 0
			case tVariantType::t_int: {
				int w1 = sVariant1.m_Union.m_Int;
				int w2 = sVariant2.m_Union.m_Int;
				
				// Check for overflow in integer multiplication
				// Handle special cases first
				if (w1 == 0 || w2 == 0) {
					wResult.SetInt(0);
				}
				else if (w1 == -1 && w2 == INT_MIN) {
					// Special case: -1 * INT_MIN would overflow
					tDouble wDouble = static_cast<tDouble>(w1) * static_cast<tDouble>(w2);
					wResult.SetDouble(wDouble);
				}
				else if (w2 == -1 && w1 == INT_MIN) {
					// Special case: INT_MIN * -1 would overflow
					tDouble wDouble = static_cast<tDouble>(w1) * static_cast<tDouble>(w2);
					wResult.SetDouble(wDouble);
				}
				else {
					// General case: check for overflow
					// For w2 == -1, we already handled w1 == INT_MIN, so w1 != INT_MIN here
					// For other negative w2, we need to check bounds carefully
					tBool wOverflow = false;
					if (w2 > 0) {
						// Positive multiplier: check if result would exceed INT_MAX or be below INT_MIN
						if (w1 > INT_MAX / w2 || w1 < INT_MIN / w2) {
							wOverflow = true;
						}
					}
					else if (w2 == -1) {
						// w2 == -1: already handled w1 == INT_MIN case, so no overflow possible
						// (any other w1 * -1 fits in int range)
						wOverflow = false;
					}
					else {
						// Negative multiplier (w2 < 0, w2 != -1)
						// INT_MAX / w2 will be negative, INT_MIN / w2 will be positive
						// For w2 < -1, INT_MIN / w2 is safe to compute
						if (w1 < INT_MAX / w2 || w1 > INT_MIN / w2) {
							wOverflow = true;
						}
					}
					if (wOverflow) {
						// Overflow would occur, use double
						tDouble wDouble = static_cast<tDouble>(w1) * static_cast<tDouble>(w2);
						wResult.SetDouble(wDouble);
					}
					else {
						wResult.SetInt(w1 * w2);
					}
				}
				break;
			}
			case tVariantType::t_bool: wResult.SetInt(sVariant1.m_Union.m_Int * (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(tDouble(sVariant1.m_Union.m_Int) * sVariant2.m_Union.m_Double); break;
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
	        default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}

		case tVariantType::t_bool: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) * 0); break; // Treat Null as 0
			case tVariantType::t_int: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) * sVariant2.m_Union.m_Int); break;
			case tVariantType::t_bool: wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) * (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble((sVariant1.m_Union.m_Bool ? 1 : 0) * sVariant2.m_Union.m_Double); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_double: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: wResult.SetDouble(sVariant1.m_Union.m_Double * 0.0); break; // Treat Null as 0.0
			case tVariantType::t_int:	wResult.SetDouble(sVariant1.m_Union.m_Double * tDouble(sVariant2.m_Union.m_Int)); break;
			case tVariantType::t_bool: wResult.SetDouble(sVariant1.m_Union.m_Double * (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDouble(sVariant1.m_Union.m_Double * sVariant2.m_Union.m_Double); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_string: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_error: {
			wResult.SetError(*sVariant1.m_Union.m_Error); break;
			break;
		}
		case tVariantType::t_date: {
			switch (sVariant2.m_Type) {
            case tVariantType::t_null: {
				// Treat Null as 0 - date * 0 = 0
				wResult.SetDate(sVariant1.m_Union.m_Date * 0);
				break;
			}
			case tVariantType::t_int: wResult.SetDate(sVariant1.m_Union.m_Date * sVariant2.m_Union.m_Int); break;
			case tVariantType::t_bool: wResult.SetDate(sVariant1.m_Union.m_Date * (sVariant2.m_Union.m_Bool ? 1 : 0)); break;
			case tVariantType::t_double: wResult.SetDate(sVariant1.m_Union.m_Date * tDate(sVariant2.m_Union.m_Double)); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			// Like Excel
			case tVariantType::t_date: wResult.SetDate(sVariant1.m_Union.m_Date * sVariant2.m_Union.m_Date); break;
        
            default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
        default: break;
		}
		return(wResult);
	}

	tVariant operator/(const tVariant& sVariant1, const tVariant& sVariant2) {
		tVariant wResult;

		// Propagate errors immediately - if either operand is an error, return the error
		if (sVariant1.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant1.m_Union.m_Error);
			return(wResult);
		}
		if (sVariant2.m_Type == tVariantType::t_error) {
			wResult.SetError(*sVariant2.m_Union.m_Error);
			return(wResult);
		}

		if (sVariant1.m_Type == tVariantType::t_class) { return(sVariant1.m_Union.m_Class->Operator_divide(true, sVariant2)); }
		if (sVariant2.m_Type == tVariantType::t_class) { return(sVariant2.m_Union.m_Class->Operator_divide(false, sVariant1)); }

		switch (sVariant1.m_Type) {
		case tVariantType::t_null: {
			// Treat Null as default values: 0 for int, 0.0 for double, "" for string, false for bool
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				wResult.SetError(tClassError(tTypeError::t_div0, "")); // 0 / 0 = division by zero
				break;
			}
			case tVariantType::t_int: {
				if (sVariant2.m_Union.m_Int == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				} else {
					wResult.SetInt(0 / sVariant2.m_Union.m_Int); // 0 / value = 0
				}
				break;
			}
			case tVariantType::t_bool: {
				if ((sVariant2.m_Union.m_Bool ? 1 : 0) == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				} else {
					wResult.SetInt(0 / (sVariant2.m_Union.m_Bool ? 1 : 0)); // 0 / value = 0
				}
				break;
			}
			case tVariantType::t_double: {
				if (sVariant2.m_Union.m_Double == 0.0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				} else {
					wResult.SetDouble(0.0 / sVariant2.m_Union.m_Double); // 0.0 / value = 0.0
				}
				break;
			}
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				if (sVariant2.m_Union.m_Date == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				} else {
					wResult.SetDouble(0.0 / tDouble(sVariant2.m_Union.m_Date)); // 0 / date = 0
				}
				break;
			}
            default: break;
			}
			break;
		}
		case tVariantType::t_int: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				wResult.SetError(tClassError(tTypeError::t_div0, "")); // division by zero (Null = 0)
				break;
			}
			case tVariantType::t_int: {
				if (sVariant2.m_Union.m_Int == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble((double)sVariant1.m_Union.m_Int / (double) sVariant2.m_Union.m_Int);
				}
				break;
			}
			case tVariantType::t_bool: {
				if ((sVariant2.m_Union.m_Bool ? 1 : 0) == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetInt(sVariant1.m_Union.m_Int / (sVariant2.m_Union.m_Bool ? 1 : 0));
				}
				break;
			}
			case tVariantType::t_double: {
				if (sVariant2.m_Union.m_Double == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble((double)sVariant1.m_Union.m_Int / sVariant2.m_Union.m_Double);
				}
				break;
			}
			case tVariantType::t_string: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			case tVariantType::t_date: {
				if (sVariant2.m_Union.m_Date == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble((double)sVariant1.m_Union.m_Int / tDouble(sVariant2.m_Union.m_Date));
				}
				break;
			}
            default: break;
			}
			break;
		}

		case tVariantType::t_bool: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				wResult.SetError(tClassError(tTypeError::t_div0, "")); // division by zero (Null = 0)
				break;
			}
			case tVariantType::t_int: {
				if (sVariant2.m_Union.m_Int == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) / sVariant2.m_Union.m_Int);
				}
				break;
			}
			case tVariantType::t_bool: {
				if ((sVariant2.m_Union.m_Bool ? 1 : 0) == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetInt((sVariant1.m_Union.m_Bool ? 1 : 0) / (sVariant2.m_Union.m_Bool ? 1 : 0));
				}
				break;
			}
			case tVariantType::t_double: {
				if (sVariant2.m_Union.m_Double == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble((sVariant1.m_Union.m_Bool ? 1 : 0) / sVariant2.m_Union.m_Double);
				}
				break;
			}
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_double: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_null: {
				wResult.SetError(tClassError(tTypeError::t_div0, "")); // division by zero (Null = 0.0)
				break;
			}
			case tVariantType::t_int: {
				if (sVariant2.m_Union.m_Int == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble(sVariant1.m_Union.m_Double  / sVariant2.m_Union.m_Int);
				}
				break;
			}
			case tVariantType::t_bool: {
				if ((sVariant2.m_Union.m_Bool ? 1 : 0) == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble(sVariant1.m_Union.m_Double / (sVariant2.m_Union.m_Bool ? 1 : 0));
				}
				break;
			}			
			case tVariantType::t_double: {
				if (sVariant2.m_Union.m_Double == 0) {
					wResult.SetError(tClassError(tTypeError::t_div0, ""));
				}
				else {
					wResult.SetDouble(sVariant1.m_Union.m_Double / sVariant2.m_Union.m_Double);
				}
				break;
			}
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_string: {
			switch (sVariant2.m_Type) {
			case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
			default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
			}
			break;
		}
		case tVariantType::t_error: {
			wResult.SetError(*sVariant1.m_Union.m_Error); break;
			break;
		}
		case tVariantType::t_date: {
            switch (sVariant2.m_Type) {
                case tVariantType::t_null: {
					wResult.SetError(tClassError(tTypeError::t_div0, "")); // division by zero (Null = 0)
					break;
				}
                case tVariantType::t_int: {
                    if (sVariant2.m_Union.m_Int == 0) {
                        wResult.SetError(tClassError(tTypeError::t_div0, ""));
                    }
                    else {
                        wResult.SetDate(sVariant1.m_Union.m_Date / sVariant2.m_Union.m_Int);
                    }
                    break;
                }
                case tVariantType::t_bool: {
                    if ((sVariant2.m_Union.m_Bool ? 1 : 0) == 0) {
                        wResult.SetError(tClassError(tTypeError::t_div0, ""));
                    }
                    else {
                        wResult.SetDate(sVariant1.m_Union.m_Date / (sVariant2.m_Union.m_Bool ? 1 : 0));
                    }
                    break;
                }
                case tVariantType::t_double: {
                    if (sVariant2.m_Union.m_Double == 0) {
                        wResult.SetError(tClassError(tTypeError::t_div0, ""));
                    }
                    else {
                        wResult.SetDouble(sVariant1.m_Union.m_Date / sVariant2.m_Union.m_Double);
                    }
                    break;
                }
                case tVariantType::t_date: {
                    // Like Excel
                    wResult.SetDate(sVariant1.m_Union.m_Date / sVariant2.m_Union.m_Date);
                    break;
                }
                case tVariantType::t_error: wResult.SetError(*sVariant2.m_Union.m_Error); break;
                default: wResult.SetError(tClassError(tTypeError::t_value, "")); break;
            }
            break; // case t_date of the outer switch — explicit to avoid fall-through.
		}
        default: break;
		}
		return(wResult);
	}

    // separators in the given integer
    tString ThousandSeparator(tInt n) {
		tString ans = "";

		// Convert the given integer
		// to equivalent string
		tString num = to_string(n);

		// Initialise count
		tInt count = 0;

		// Traverse the string in reverse
		for (tInt i = int(num.size()) - 1; i >= 0; i--) {
			count++;
			ans.push_back(num[i]);

			// If three characters
			// are traversed
			if (count == 3) {
                ans.push_back(tApplication::Instance()->Locale()->Thousand());
				count = 0;
			}
		}

		// Reverse the string to get
		// the desired output
		reverse(ans.begin(), ans.end());

		// If the given string is
		// less than 1000
		if (ans.size() % 4 == 0) {
			// Remove thousand separarator','
			ans.erase(ans.begin());
		}

		return ans;
	}

	ostream& operator<<(ostream& sStream, const tVariant& sValue) {
		switch (sValue.Type()) {
		case tVariantType::t_null: break; // os << "null"; break;
		case tVariantType::t_int:  sStream << ThousandSeparator(sValue.Int()); break;
		case tVariantType::t_bool: if (sValue.Bool()) { sStream << "true"; } else { sStream << "false"; }	break;
		case tVariantType::t_double: {
			std::ios_base::fmtflags wFlags = sStream.flags();
			std::streamsize wPrecision = sStream.precision();
			sStream << std::fixed << std::setprecision(2) << sValue.Double();
			sStream.flags(wFlags);
			sStream.precision(wPrecision);
			break;
		} 
		case tVariantType::t_string: sStream << sValue.String(); break;
		case tVariantType::t_error: sStream << sValue.Error().Error(); break;
        case tVariantType::t_date: sStream << sValue.Date(); break;
		case tVariantType::t_class: {
			tString wClassName="nullptr";
			tVirtualClass* wClass = sValue.Class();
			if (wClass != nullptr) wClassName = wClass->ClassName();
			sStream << "Class:" << wClassName;
            break;
		}
		}
		return(sStream);
	}

	// Class Ancestor for Class Variant =======================================
	tVariantClass::tVariantClass() : tVirtualClass() , m_Value() {}
	tVariantClass::tVariantClass(tVariant sValue) : tVirtualClass() , m_Value(sValue) {}

	tVariantClass::tVariantClass(const tVariantClass& sVariantClass) : tVirtualClass(sVariantClass) {
			m_Value = sVariantClass.m_Value;
	}

	tVirtualClass* tVariantClass::Clone() {
		return(new tVariantClass(*this));
	}

    void tVariantClass::Value(tVariant* sValue) {
        m_Value=*sValue;
    }

	tVariant* tVariantClass::Value() {
		return(&m_Value);
	}

    void tVariantClass::JsonJavaScript(Writer<StringBuffer>* sWriter) {}
    
    tVariant tVariantClass::Operator_plus(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tVariant* wValue=&wVariant;
        if (wVariant.Type() == tVariantType::t_class)
            wValue=wVariant.Class()->Value();
        
        m_Value = m_Value + *wValue;
        return(tVariant(this));
    }
        
    tVariant tVariantClass::Operator_minus(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tVariant* wValue=&wVariant;
        if (wVariant.Type() == tVariantType::t_class)
            wValue=wVariant.Class()->Value();
        
        if (wValue!=nullptr) {
            if (sLeft) {
                m_Value = m_Value - *wValue;
            }
            else {
                m_Value = *wValue - m_Value;
            }
        }
        return(tVariant(this));
    }

    tVariant tVariantClass::Operator_multiply(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tVariant* wValue=&wVariant;
        if (wVariant.Type() == tVariantType::t_class)
            wValue=wVariant.Class()->Value();
        
        if (wValue!=nullptr) m_Value = m_Value * *wValue;
        return(tVariant(this));
    }
        
    tVariant tVariantClass::Operator_divide(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tVariant* wValue=&wVariant;
        if (wVariant.Type() == tVariantType::t_class)
            wValue=wVariant.Class()->Value();
        
        if (wValue!=nullptr) {
            if (sLeft) {
                m_Value = m_Value / *wValue;
            }
            else {
                m_Value = *wValue / m_Value;
            }
        }
        return(tVariant(this));
    }

}; // end of namespace==========================================================
