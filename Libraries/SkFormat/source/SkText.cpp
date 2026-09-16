//=============================================================================
// SkText (Text for Css )
//=============================================================================
#include "../include/SkText.hpp"
#include "SkApplication.hpp"
#include "SkFormatString.hpp"
#include <cmath>
#include <limits>

using namespace SkRoot;

namespace SkFormat {

    // NaN sentinel used to distinguish "rotation not specified" from
    // "rotation explicitly set to 0". Enables resetting rotation from UI
    // with "text-rotate:0;" while leaving partial formats (e.g. color-only)
    // untouched during Merge().
    static const tFloat kRotateUnset = std::numeric_limits<tFloat>::quiet_NaN();

	// SkText =================================================================
	tTextCss::tTextCss() : tFormatShare(), m_TextAlign(tTextAlign::notuse), m_VerticalTextAlign(tVerticalTextAlign::notuse),m_UnderLine(tTextDecorationLine::notuse),
        m_OverLine(tTextDecorationLine::notuse), m_LineThrough(tTextDecorationLine::notuse),m_TextWrap(tTextWrap::notuse), m_Rotate(kRotateUnset), m_FormatString() {};
	tTextCss::tTextCss(const tTextCss& sText) : tFormatShare(sText), 
                                                m_TextAlign(sText.m_TextAlign), 
                                                m_VerticalTextAlign(sText.m_VerticalTextAlign),
                                                m_UnderLine(sText.m_UnderLine),
                                                m_LineThrough(sText.m_LineThrough),
                                                m_TextWrap(sText.m_TextWrap),
                                                m_Rotate(sText.m_Rotate),
                                                m_FormatString(sText.m_FormatString) {};
	
	/// @brief		Clear SkAllocator
	void tTextCss::Clear() {
		tFormatShare::Clear();
		m_TextAlign = tTextAlign::notuse;
		m_VerticalTextAlign = tVerticalTextAlign::notuse;
        m_UnderLine = tTextDecorationLine::notuse;
        m_OverLine = tTextDecorationLine::notuse;
        m_LineThrough = tTextDecorationLine::notuse;
        m_TextWrap = tTextWrap::notuse;
        m_Rotate = kRotateUnset;
		m_FormatString.Clear();
	}

	tString tTextCss::Key() {
		tStringStream wStream;
        wStream << "[";
        if (m_TextAlign!=tTextAlign::notuse) {
            wStream << "h" << tInt(m_TextAlign);
        }
        if (m_VerticalTextAlign!=tVerticalTextAlign::notuse) {
            wStream << "v" << tInt(m_VerticalTextAlign);
        }
        if (m_UnderLine!=tTextDecorationLine::notuse) {
            wStream << "u" << tInt(m_UnderLine);
        }
        if (m_OverLine!=tTextDecorationLine::notuse) {
            wStream << "o" << tInt(m_OverLine);
        }
        if (m_LineThrough!=tTextDecorationLine::notuse) {
            wStream << "l" << tInt(m_LineThrough);
        }
        if (m_TextWrap!=tTextWrap::notuse) {
            wStream << "tw" << tInt(m_TextWrap);
        }
        if (!std::isnan(m_Rotate) && m_Rotate!=0) {
            wStream << "r" << m_Rotate;
        }
        if (!m_FormatString.Empty()) {
            wStream << "fs" << int(m_FormatString.FormatType());
            if (m_FormatString.Money()!=t_UnitMoney::None) {
                wStream << "fm" << int(m_FormatString.Money());
            }
            wStream << "fsp" << m_FormatString.Decimal();
            if ((m_FormatString.FormatType()==tFormatStringType::excelnumber) || ((m_FormatString.FormatType()==tFormatStringType::exceldate)))
                wStream << ":" <<  m_FormatString.FormatExcel();
        }
        wStream << "]";
		return(wStream.str());
	}

	tString tTextCss::Str(tBool sReturn) {
		tStringStream wStream;
        
        tStringStream wStreamReturn;
        wStreamReturn << ";";
        if (sReturn) wStreamReturn << endl;
        
		tBool wOk = false;
		if (m_TextAlign != tTextAlign::notuse) {
			wOk = true;
			wStream << "text-align:" << TextAlignStr();
		}
		if ((m_VerticalTextAlign != tVerticalTextAlign::notuse) && ((m_VerticalTextAlign != tVerticalTextAlign::none))) {
			if (wOk) wStream << wStreamReturn.str();
			wStream << "vertical-align:" << VerticalTextAlignStr();
            wOk = true;
		}
        if ((m_UnderLine!= tTextDecorationLine::notuse) && (m_UnderLine != tTextDecorationLine::none)) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << "text-decoration-line:" << "underline";
            wOk = true;
        }
        if ((m_OverLine!= tTextDecorationLine::notuse) && (m_OverLine != tTextDecorationLine::none)) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << "text-decoration-line:" << "overline";
            wOk = true;
        }
        if ((m_LineThrough!= tTextDecorationLine::notuse) && (m_LineThrough != tTextDecorationLine::none)) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << "text-decoration-line:" << "line-through";
            wOk = true;
        }
        if ((m_TextWrap!= tTextWrap::notuse) && (m_TextWrap != tTextWrap::none)) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << "text-wrap:" << TextWrapStr();
            wOk = true;
        }
        if (!std::isnan(m_Rotate) && m_Rotate!=0) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << "text-rotate:" << m_Rotate;
            wOk = true;
        }
        if (m_FormatString.FormatType() != tFormatStringType::none) {
            if (wOk) wStream << wStreamReturn.str();
            if ((m_FormatString.FormatType()!=tFormatStringType::excelnumber) && (m_FormatString.FormatType()!=tFormatStringType::exceldate)) {
                // Emit the predefined KEY (locale-independent, e.g. "mm-dd-yyyy") and not the
                // resolved local format (e.g. "%m-%d-%Y" in en-US). The reverse parser path
                // (SkLemonInterface::FormatString) matches on m_Key via StringKey2FormatStringType;
                // serializing the local format breaks round-trip and disables locale-aware
                // re-resolution when the file is loaded under a different locale.
                tString wKey = tApplication::Instance()->FormatStringRoot()->FormatStringType2Key(m_FormatString.FormatType());
                if (wKey.empty()) wKey = m_FormatString.FormatLocal(); // fallback for unregistered types
                wStream << "format-string:\"" << wKey << "\"";
                if (m_FormatString.Money()!=t_UnitMoney::None) {
                    wStream << " \"" << UnitMoneyStr(m_FormatString.Money()) << "\"";
                }
                wStream << " " << m_FormatString.Decimal();;
            } else {
                wStream << "format-string:\"" << m_FormatString.FormatExcel() << "\"";
                if (m_FormatString.Money()!=t_UnitMoney::None) {
                    wStream << " \"" << UnitMoneyStr(m_FormatString.Money()) << "\"";
                }
                wStream << " " << m_FormatString.Decimal();
            }
       }
		return(wStream.str());
	}

    void  tTextCss::Merge(tTextCss*  sText) {
        if (sText->TextAlign()!=tTextAlign::notuse) {
            m_TextAlign=sText->TextAlign();
        }
        if (sText->VerticalTextAlign()!=tVerticalTextAlign::notuse) {
            m_VerticalTextAlign=sText->VerticalTextAlign();
        }
        if (sText->m_UnderLine != tTextDecorationLine::notuse) {
            m_UnderLine=sText->UnderLine();
        }
        if (sText->m_OverLine != tTextDecorationLine::notuse) {
            m_OverLine=sText->OverLine();
        }
        if (sText->m_LineThrough != tTextDecorationLine::notuse) {
            m_LineThrough=sText->LineThrough();
        }
        if (sText->m_TextWrap != tTextWrap::notuse) {
            m_TextWrap=sText->TextWrap();
        }
        // Merge when the incoming rotation was explicitly set (including 0,
        // which means "reset rotation"). Only leave existing value untouched
        // when the incoming field is NaN (== not specified).
        if (!std::isnan(sText->m_Rotate)) {
            m_Rotate=sText->m_Rotate;
        }
        if (!sText->m_FormatString.Empty()) {
            m_FormatString=sText->m_FormatString;
        }
    }

	void tTextCss::TextAlign(tTextAlign sTextAlign) { m_TextAlign = sTextAlign; }

	tTextAlign tTextCss::TextAlign() { return(m_TextAlign); }

	tString tTextCss::TextAlignStr() {
		switch (m_TextAlign) {
		case tTextAlign::none: return("none");
		case tTextAlign::left: return("left");
		case tTextAlign::right: return("right");
		case tTextAlign::center: return("center");
		case tTextAlign::justify: return("justify");
        default :break;
		}
		return("");
	}

	void tTextCss::VerticalTextAlign(tVerticalTextAlign sVerticalTextAlign) { m_VerticalTextAlign = sVerticalTextAlign; }

	tVerticalTextAlign tTextCss::VerticalTextAlign() { return(m_VerticalTextAlign); }

	tString tTextCss::VerticalTextAlignStr() {
		switch(m_VerticalTextAlign) {
		case tVerticalTextAlign::none : return("none");
		case tVerticalTextAlign::baseline: return("baseline");
		case tVerticalTextAlign::sub: return("sub");
		case tVerticalTextAlign::super: return("super"); 
		case tVerticalTextAlign::text_top: return("text-top"); 
		case tVerticalTextAlign::text_bottom: return("text-bottom");
		case tVerticalTextAlign::top: return("top");
		case tVerticalTextAlign::bottom: return("bottom");
        case tVerticalTextAlign::middle: return("middle");
        default :break;
		}
		return("");
	}

    void tTextCss::UnderLine(tTextDecorationLine sUnderLine) { m_UnderLine=sUnderLine; }
    tTextDecorationLine tTextCss::UnderLine() {return (m_UnderLine); }

    void tTextCss::OverLine(tTextDecorationLine sOverLine) {m_OverLine=sOverLine; }
    tTextDecorationLine tTextCss::OverLine() { return(m_OverLine); }

    void tTextCss::LineThrough(tTextDecorationLine sLineThrough) { m_LineThrough=sLineThrough; }
    tTextDecorationLine tTextCss::LineThrough() { return(m_LineThrough); }

    void tTextCss::Rotate(tDouble sRotation) { m_Rotate=tFloat(sRotation); }
    // Return 0 when rotation is "unset" (NaN sentinel) so external callers
    // keep seeing the same public contract.
    tFloat tTextCss::Rotate() { return std::isnan(m_Rotate) ? tFloat(0) : m_Rotate; }

    tString tTextCss::TextWrapStr() {
        switch(m_TextWrap) {
        case tTextWrap::wrap: return("wrap");
        case tTextWrap::nowrap: return("nowrap");
        default :break;
        }
        return("");
    }

    void tTextCss::TextWrap(tTextWrap sTextWrap) { m_TextWrap=sTextWrap; }
    tTextWrap tTextCss::TextWrap() { return(m_TextWrap); }

    void tTextCss::FormatString(tString sFormatString) {
        m_FormatString.Format(sFormatString);
    }
    
    tString tTextCss::FormatStringStr() { return(m_FormatString.FormatLocal()); };

    tFormatString* tTextCss::FormatString() { return(&m_FormatString);}
    tFormatStringType tTextCss::_FormatString() { return(m_FormatString.FormatType()); }

    void tTextCss::Money(t_UnitMoney sUnitMoney) { m_FormatString.Money(sUnitMoney); }
    t_UnitMoney tTextCss::Money() { return(m_FormatString.Money()); };
    
    void tTextCss::Precision(tByte sDecimal) { m_FormatString.Decimal(sDecimal); }
    tByte tTextCss::Precision() { return(tByte(m_FormatString.Decimal())); }

    void tTextCss::ExcelFormatString(tString sFormatString) {
        m_FormatString.ExcelFormat(sFormatString);
    };

    tString tTextCss::ExcelFormatString() { return(m_FormatString.FormatExcel()); };

	void tTextCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
        if (m_TextAlign!=tTextAlign::notuse) {
            sWriter->Key("h");
            sWriter->Int(tInt(m_TextAlign));
        }
        if (m_VerticalTextAlign!=tVerticalTextAlign::notuse) {
            sWriter->Key("v");
            sWriter->Int(tInt(m_VerticalTextAlign));
        }
        if (m_UnderLine!=tTextDecorationLine::notuse) {
            sWriter->Key("u");
            sWriter->Int(tInt(m_UnderLine));
        }
        if (m_OverLine!=tTextDecorationLine::notuse) {
            sWriter->Key("o");
            sWriter->Int(tInt(m_OverLine));
        }
        if (m_LineThrough!=tTextDecorationLine::notuse) {
            sWriter->Key("l");
            sWriter->Int(tInt(m_LineThrough));
        }
      
        if (m_TextWrap!=tTextWrap::notuse) {
            sWriter->Key("tw");
            sWriter->Int(tInt(m_TextWrap));
        }
        if (!std::isnan(m_Rotate) && m_Rotate!=0) {
            sWriter->Key("r");
            sWriter->Double(m_Rotate);
        }
        if (!m_FormatString.Empty()) {
            sWriter->Key("f");
            m_FormatString.Json(sWriter);
        }
        sWriter->EndObject();
	}

	/// @brief		Reader Json. 
	/// @param[in]	sValue Value&
	void tTextCss::Json(const rapidjson::Value& sValue) {
        if (sValue.HasMember("h")) m_TextAlign= tTextAlign( sValue["h"].GetInt());
        if (sValue.HasMember("v")) m_VerticalTextAlign = tVerticalTextAlign(sValue["v"].GetInt());
        if (sValue.HasMember("u")) m_UnderLine = tTextDecorationLine(sValue["u"].GetInt());
        if (sValue.HasMember("o")) m_OverLine = tTextDecorationLine(sValue["u"].GetInt());
        if (sValue.HasMember("l")) m_LineThrough = tTextDecorationLine(sValue["u"].GetInt());
        if (sValue.HasMember("tw")) m_TextWrap = tTextWrap(sValue["tw"].GetInt());
        if (sValue.HasMember("r")) m_Rotate = tFloat(sValue["r"].GetDouble());
        if (sValue.HasMember("f")) m_FormatString.Json(sValue["f"]);
    }

    tBool tTextCss::Empty() {
        return((m_TextAlign==tTextAlign::notuse) &&
               (m_VerticalTextAlign==tVerticalTextAlign::notuse) &&
               (m_UnderLine==tTextDecorationLine::notuse) &&
               (m_OverLine==tTextDecorationLine::notuse) &&
               (m_LineThrough==tTextDecorationLine::notuse) &&
               (m_TextWrap==tTextWrap::notuse) &&
               (std::isnan(m_Rotate) || m_Rotate==0) &&
               (m_FormatString.Empty()));
    }
	

    tBool tTextCss::operator == (tTextCss& sText) {
        // NaN rotation values count as equal when both are unset; otherwise
        // compare by numeric value.
        const tBool wRotateEq = (std::isnan(m_Rotate) && std::isnan(sText.m_Rotate))
                             || (m_Rotate == sText.m_Rotate);
  	return((m_TextAlign == sText.m_TextAlign) &&
           (m_VerticalTextAlign == sText.m_VerticalTextAlign) &&
           (m_UnderLine == sText.m_UnderLine) &&
           (m_OverLine == sText.m_OverLine) &&
           (m_LineThrough == sText.m_LineThrough) &&
           (m_TextWrap == sText.m_TextWrap) &&
           wRotateEq &&
           (m_FormatString == sText.m_FormatString));
	}

} // end of namespace 

