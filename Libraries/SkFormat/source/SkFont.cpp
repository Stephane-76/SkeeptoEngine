//=============================================================================
// SkFont (Font for Css )
//=============================================================================
#include "../include/SkFont.hpp"

using namespace SkRoot;

namespace SkFormat {

	// SkFont =================================================================
	tFontCss::tFontCss() : tFormatShare(), 
		m_Name(""),
		m_Family(tFontFamily::notuse),
		m_Size(),
		m_Style(tFontStyle::notuse),
		m_ObliqueDegrees(0),
		m_Variant(tFontVariant::notuse),
		m_Weight(tFontWeight::notuse),
        m_WeightInt(0),
		m_Stretch(tFontStretch::notuse),
		m_StretchPercent(0),
		m_LineHeight()  {};
	
	tFontCss::tFontCss(const tFontCss& sFont) : tFormatShare(sFont),
		m_Name(sFont.m_Name),
		m_Family(sFont.m_Family),
		m_Size(sFont.m_Size),
		m_Style(sFont.m_Style),
		m_ObliqueDegrees(sFont.m_ObliqueDegrees),
		m_Variant(sFont.m_Variant),
		m_Weight(sFont.m_Weight),
        m_WeightInt(sFont.m_WeightInt),
		m_Stretch(sFont.m_Stretch),
		m_StretchPercent(sFont.m_StretchPercent),
		m_LineHeight(sFont.m_LineHeight) {};
										  

	void tFontCss::Clear() {
		tFormatShare::Clear();
		m_Name = "";
		m_Family = tFontFamily::notuse;
		m_Size = tUnitCss(0, tUnitMetrics::none);
		m_Style = tFontStyle::notuse;
		m_ObliqueDegrees = 0;
		m_Variant = tFontVariant::notuse;
        m_Weight = tFontWeight::notuse;
        m_WeightInt = 0;
		m_Stretch = tFontStretch::notuse;
		m_StretchPercent = 0;
		m_LineHeight = tUnitCss(0, tUnitMetrics::none);
	}

	tString tFontCss::Key() {
		tStringStream wStream;
		if (m_Name() != "") wStream << "n" << m_Name();
		if (!m_Size.Empty()) wStream << "s" << m_Size;
		if (m_Family != tFontFamily::notuse) wStream << "f" << tInt(m_Family);
		if (m_Style != tFontStyle::notuse) {
			wStream << "st" << tInt(m_Style);
			if (m_Style == tFontStyle::deg) wStream << "." << m_ObliqueDegrees;
		}
		if (m_Variant != tFontVariant::notuse) wStream << "v" << tInt(m_Variant);
		
        if (m_Weight!=tFontWeight::notuse) {
            wStream << tInt(m_Weight);
            if (m_Weight==tFontWeight::integer) {
                if (m_WeightInt != 0) wStream << ":" << m_WeightInt;
            }
        }
		if (m_Stretch != tFontStretch::notuse) {
			wStream << "str" << tInt(m_Stretch);
			if (m_Stretch==tFontStretch::percent) wStream << "." << m_StretchPercent;
		}
		if (!m_LineHeight.Empty()) wStream << "l" << m_LineHeight;

		return(wStream.str());
	}

	void CompleteStream(tStringStream& wStream) {
		if (!wStream.eof()) wStream << " ";
	}
	

	tString tFontCss::Str(tBool sReturn) {
		tStringStream wStream;
		tStringStream wSep;
		wSep << ";";
		if (sReturn) wSep << endl;

		tBool wOk = false;
		// The "font" shorthand grammar requires a quoted font name as its
		// first token (see SkLemonFormat.y::decFont). When no name is set,
		// emit each property as a standalone CSS declaration (font-size,
		// font-style, ...) to produce a parseable string.
		if (m_Name() != "") {
			wStream << "font:\"" << m_Name() << "\"";
			if ((m_Family != tFontFamily::notuse) && (m_Family != tFontFamily::none)) {
				wStream << "," << FamilyStr();
			}
			if (!m_Size.Empty()) {
				wStream << " " << m_Size;
			}
			if ((m_Variant != tFontVariant::notuse) && (m_Variant != tFontVariant::none)) {
				wStream << " " << VariantStr();
			}
			wOk = true;
			// Keep weight/style out of the Sk "font:" shorthand so mutualized CSS (.sker "f")
			// matches the Excel import path: explicit font-weight and font-style declarations.
			if ((m_Weight != tFontWeight::notuse) && (m_Weight != tFontWeight::none)) {
				wStream << wSep.str() << "font-weight:" << WeightStr();
			}
			if ((m_Style != tFontStyle::notuse) && (m_Style != tFontStyle::none)) {
				wStream << wSep.str() << "font-style:" << StyleStr();
			}
		} else {
			if (!m_Size.Empty()) {
				wStream << "font-size:" << m_Size;
				wOk = true;
			}
			if ((m_Style != tFontStyle::notuse) && (m_Style != tFontStyle::none)) {
				if (wOk) wStream << wSep.str();
				wStream << "font-style:" << StyleStr();
				wOk = true;
			}
			if ((m_Variant != tFontVariant::notuse) && (m_Variant != tFontVariant::none)) {
				if (wOk) wStream << wSep.str();
				wStream << "font-variant:" << VariantStr();
				wOk = true;
			}
			if ((m_Weight != tFontWeight::notuse) && (m_Weight != tFontWeight::none)) {
				if (wOk) wStream << wSep.str();
				wStream << "font-weight:" << WeightStr();
				wOk = true;
			}
		}

		if ((m_Stretch != tFontStretch::notuse) && (m_Stretch != tFontStretch::none)) {
			if (wOk) wStream << wSep.str();
			wStream << "font-stretch:" << StretchStr();
			if (m_Stretch == tFontStretch::percent) wStream << " " << m_StretchPercent << "%";
			wOk = true;
		}
		if (!m_LineHeight.Empty()) {
			if (wOk) wStream << wSep.str();
			wStream << "line-height:" << m_LineHeight;
		}
		return(wStream.str());
	}

    void  tFontCss::Merge(tFontCss*  sFont) {
        if (sFont->Name()!="") {
            m_Name=sFont->Name();
        }
        if (sFont->Family() != tFontFamily::notuse) {
            m_Family = sFont->Family();
        }
        
        if (!sFont->Size().Empty()) {
            m_Size=sFont->Size();
        }
        if (sFont->Style() != tFontStyle::notuse) {
            m_Style=sFont->Style();
        }
        if (sFont->Variant() != tFontVariant::notuse) {
            m_Variant=sFont->Variant();
        }
        if (sFont->Weight() != tFontWeight::notuse) {
            m_Weight=sFont->Weight();
            m_WeightInt=sFont->WeightInt();
        }
        if (sFont->Stretch() != tFontStretch::notuse) {
            m_Stretch=sFont->Stretch();
        }
        if (!sFont->LineHeight().Empty()) {
            m_LineHeight=sFont->LineHeight();
        }
    }


	void tFontCss::Name(tString sName) { m_Name = sName; };
	tString  tFontCss::Name() { return(m_Name()); }

	void tFontCss::Family(tFontFamily sFamily) { m_Family = sFamily; };
	tFontFamily  tFontCss::Family() { return(m_Family); }

	tString tFontCss::FamilyStr() {
		switch (m_Family) {
		case  tFontFamily::none: return("");
		case  tFontFamily::serif: return("serif");
		case  tFontFamily::sans_serif: return("sans-serif");
		case  tFontFamily::monospace: return("monospace");
		case  tFontFamily::cursive: return("cursive");
		case  tFontFamily::fantasy: return("fantasy");
		case  tFontFamily::system_ui: return("system-ui");
		case  tFontFamily::emoji: return("emoji");
		case  tFontFamily::math: return("math");
		case  tFontFamily::fangsong: return("fangsong");
        case  tFontFamily::Count: return("");
        default : {}
		}
		return("");
	}

	void tFontCss::Size(tUnitCss sSize) { m_Size = sSize; }
	tUnitCss tFontCss::Size() { return(m_Size); }

	void tFontCss::Style(tFontStyle sStyle) { 
		m_Style = sStyle; 
		if (m_Style != tFontStyle::deg) m_ObliqueDegrees = 0;
	};
	tFontStyle  tFontCss::Style() { return(m_Style); }

	tString tFontCss::StyleStr() {
		switch (m_Style) {
			case tFontStyle::none: return("");
			case tFontStyle::normal: return("normal");
			case tFontStyle::italic: return("italic");
            case tFontStyle::oblique: return("oblique");
            case tFontStyle::deg:  {
                tStringStream wStream;
                wStream << "oblique " << m_ObliqueDegrees << "deg";
                return(wStream.str());
            }
            case tFontStyle::Count: break;
            default : {}
		}
		return("");
	}

	void tFontCss::ObliqueDegrees(tFloat sValue) { m_Style = tFontStyle::deg; m_ObliqueDegrees = sValue; }
	tFloat  tFontCss::ObliqueDegrees() { return(m_ObliqueDegrees); }

	void tFontCss::Variant(tFontVariant sVariant) { m_Variant = sVariant; };
	tFontVariant  tFontCss::Variant() { return(m_Variant); }

	tString tFontCss::VariantStr() {
		switch(m_Variant) {
			case tFontVariant::none: return("" );
            case tFontVariant::normal: return("normal" );
			case tFontVariant::small_caps: return("small_caps" );
            case tFontVariant::Count : break;
            default : {}
		}
		return("");
	}

    void tFontCss::Weight(tFontWeight sWeight,tInt sWeightInt) {
        m_Weight=sWeight;
        m_WeightInt = sWeightInt;
    }
    
    tFontWeight  tFontCss::Weight() { return(m_Weight); }

    tInt  tFontCss::WeightInt() { return(m_WeightInt); }

    tString tFontCss::WeightStr() {
        switch(m_Weight) {
            case tFontWeight::none: return(""); break;
            case tFontWeight::normal: return("normal"); break;
            case tFontWeight::bold: return("bold"); break;
            case tFontWeight::lighter: return("lighter"); break;
            case tFontWeight::bolder: return("bolder"); break;
            case tFontWeight::integer: {
                tStringStream wStream;
                wStream << m_WeightInt;
                return(wStream.str());
            }
            default : {}
        }
        return("");
    }


	void tFontCss::Stretch(tFontStretch sStretch) {
		m_Stretch = sStretch;  
		if (m_Stretch != tFontStretch::percent) m_StretchPercent = 0;
	}
	tFontStretch tFontCss::Stretch() { return(m_Stretch); }

	tString tFontCss::StretchStr() {
		switch (m_Stretch) {
		case tFontStretch::none: return("");
		case tFontStretch::ultra_condensed: return("ultra-condensed");
		case tFontStretch::condensed: return("condensed");
		case tFontStretch::semi_condensed: return("semi-condensed");
		case tFontStretch::normal: return("normal");
		case tFontStretch::semi_expanded: return("semi-expanded");
		case tFontStretch::expanded: return("expanded");
        case tFontStretch::extra_expanded: return("extra-expanded");
		case tFontStretch::extra_condensed: return("extra-condensed");
		case tFontStretch::ultra_expanded: return("ultra-expanded");
		case tFontStretch::percent: return("");
        case tFontStretch::Count: break;
        default : {}
		}
		return("");
	};

	void tFontCss::StretchPercent(tFloat sValue) { m_Stretch = tFontStretch::percent; m_StretchPercent = sValue; }
	tFloat tFontCss::StretchPercent() { return(m_StretchPercent); }

	void tFontCss::LineHeight(tUnitCss sLineHeight) { m_LineHeight = sLineHeight;  }
	tUnitCss tFontCss::LineHeight() { return(m_LineHeight); }

	void tFontCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		if (m_Name() != "") {	sWriter->Key("n"); sWriter->String(m_Name().c_str()); }
		if ((m_Family != tFontFamily::notuse) && (m_Family != tFontFamily::none)) { sWriter->Key("f"); sWriter->Int(tInt(m_Family)); }
		if (!m_Size.Empty()) {	sWriter->Key("s");	m_Size.Json(sWriter); }
		if ((m_Style != tFontStyle::notuse) && (m_Style != tFontStyle::none)) {
			sWriter->Key("st"); sWriter->Int(tInt(m_Style));
			if (m_Style == tFontStyle::deg) { sWriter->Key("oblderp"); sWriter->Double(m_ObliqueDegrees); }
		}
		if ((m_Variant != tFontVariant::notuse) && (m_Variant != tFontVariant::none)) { sWriter->Key("v"); sWriter->Int(tInt(m_Variant)); }
        if ((m_Weight != tFontWeight::notuse) && (m_Weight != tFontWeight::none)) {
            sWriter->Key("w"); sWriter->Int(tInt(m_Weight));
			if (m_Weight == tFontWeight::integer) {
				sWriter->Key("wi"); sWriter->Double(m_WeightInt);
			}
        }
		if ((m_Stretch != tFontStretch::notuse) && (m_Stretch != tFontStretch::none)) {
			sWriter->Key("str"); sWriter->Int(tInt(m_Stretch));
			sWriter->Key("strper"); sWriter->Double(m_StretchPercent);
		}
		if (!m_LineHeight.Empty()) { sWriter->Key("l");	m_LineHeight.Json(sWriter); }
		sWriter->EndObject();
	}

	void tFontCss::Json(const rapidjson::Value& sValue) {
		if (sValue.HasMember("n")) m_Name=sValue["n"].GetString();
		if (sValue.HasMember("f")) m_Family = tFontFamily(sValue["f"].GetInt());
		if (sValue.HasMember("s")) m_Size.Json(sValue["s"]);
		if (sValue.HasMember("st")) m_Style=tFontStyle(sValue["st"].GetInt());
		if (sValue.HasMember("oblper")) m_ObliqueDegrees  = tFloat(sValue["oblper"].GetDouble());
		if (sValue.HasMember("v")) m_Variant = tFontVariant(sValue["v"].GetInt());
		if (sValue.HasMember("w")) m_Weight = tFontWeight(sValue["w"].GetInt());
        if (sValue.HasMember("wi")) m_WeightInt = sValue["wi"].GetInt();
		if (sValue.HasMember("str")) m_Stretch = tFontStretch(sValue["str"].GetInt());
		if (sValue.HasMember("strper")) m_StretchPercent = tFloat(sValue["strper"].GetDouble());
		if (sValue.HasMember("l")) m_LineHeight.Json(sValue["s"]);
	}

    tBool tFontCss::Empty() {
        return((m_Name() == "") &&
               (m_Family == tFontFamily::notuse) &&
               (m_Size.Empty()) &&
               (m_Style == tFontStyle::notuse) &&
               (m_Variant == tFontVariant::notuse) &&
               (m_Weight  == tFontWeight::notuse) &&
               (m_Stretch == tFontStretch::notuse) &&
               (m_LineHeight.Empty()));
    }

	tBool tFontCss::operator == (tFontCss& sFont) {
		return((m_Name() == sFont.m_Name()) && 
			   (m_Family == sFont.m_Family) && 
			   (m_Size == sFont.m_Size) && 
			   (m_Style == sFont.m_Style) &&
			   (m_ObliqueDegrees == sFont.m_ObliqueDegrees) &&
			   (m_Variant == sFont.m_Variant) &&
			   (m_Weight == sFont.m_Weight) &&
               (m_WeightInt == sFont.m_WeightInt) &&
			   (m_Stretch == sFont.m_Stretch) &&
			   (m_ObliqueDegrees == sFont.m_ObliqueDegrees) &&
			   (m_LineHeight == sFont.m_LineHeight));
	}

}

