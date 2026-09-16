//=============================================================================
// SkBorder (Border for Css )
//=============================================================================
#include "../include/SkBorder.hpp"


namespace SkFormat {
	// SkBorder ===============================================================
	tBorderCss::tBorderCss() : tClass(),m_Color(), m_Width(),m_BorderStyle(tBorderStyle::none) {}
	tBorderCss::tBorderCss(const tBorderCss& sBorder) : tClass(sBorder), m_Color(sBorder.m_Color), m_Width(sBorder.m_Width), m_BorderStyle(sBorder.m_BorderStyle) {}
	
	void tBorderCss::Clear() {
		m_Color.NotUse(true);
		m_Width = tUnitCss(0, tUnitMetrics::none);
		m_BorderStyle = tBorderStyle::none;
	}

    tString tBorderCss::Str(tString sName) {
        tStringStream wStream;
        if (sName!="") wStream << sName << ":";
        
        tBool wOk=false;
        if (m_BorderStyle != tBorderStyle::none) {
            wStream << BorderStyleStr();
            wOk=true;
        }
        if (!m_Width.Empty()) {
            if (wOk)  wStream << " ";
            wStream << Width().Str();
            wOk=true;
        }
        
        if (!(m_Color.NotUse())) {
            if (wOk)  wStream << " ";
            wStream << m_Color.StrKey();
        }
        
        return(wStream.str());
    }

	void tBorderCss::Color(tColorCss sColor) { m_Color = sColor; }
	tColorCss& tBorderCss::Color() { return(m_Color); }

	void tBorderCss::Width(tUnitCss sWidth) { m_Width = sWidth; }
	tUnitCss tBorderCss::Width() { return(m_Width); }

	void tBorderCss::BorderStyle(tBorderStyle sBorderStyle) { m_BorderStyle  = sBorderStyle; }
	tBorderStyle tBorderCss::BorderStyle() { return(m_BorderStyle); }

	tString tBorderCss::BorderStyleStr() {
		switch (m_BorderStyle)	{
			case tBorderStyle::none: return("none");
			case tBorderStyle::hidden: return("hidden");
			case tBorderStyle::dotted: return("dotted");
			case tBorderStyle::dashed: return("dashed");
			case tBorderStyle::solid: return("solid");
			case tBorderStyle::_double: return("double");
			case tBorderStyle::groove: return("groove");
			case tBorderStyle::ridge: return("ridge");
			case tBorderStyle::inset: return("inset");
			case tBorderStyle::outset: return("outset");
            case tBorderStyle::Count:break;
		}
		return("");
	}

	tBool tBorderCss::Empty() {
		return((m_BorderStyle == tBorderStyle::none) &&
		   (m_Color.NotUse()) && 
		   (m_Width.Empty()));
	}

	void  tBorderCss::Merge(const tBorderCss sBorderCss) {
		tColorCss wColor = sBorderCss.m_Color;
		if (!wColor.NotUse()) m_Color = wColor;
		
		tUnitCss wWidth = sBorderCss.m_Width;
		if (!wWidth.Empty()) m_Width = wWidth;
	
		if (sBorderCss.m_BorderStyle != tBorderStyle::none)  m_BorderStyle = sBorderCss.m_BorderStyle;
	}


	void tBorderCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("c");
		m_Color.Json(sWriter);
		sWriter->Key("u");
		m_Width.Json(sWriter);
		sWriter->Key("b");
		sWriter->Int(tInt(m_BorderStyle));
		sWriter->EndObject();
	}

	/// @brief		Reader Json. 
	/// @param[in]	sValue Value&
	void tBorderCss::Json(const rapidjson::Value& sValue) {
		m_Color.Json(sValue["c"]);
		m_Width.Json(sValue["u"]);
		m_BorderStyle = tBorderStyle(sValue["b"].GetInt());
	}

    void tBorderCss::JsonJavaScriptCanvas(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("c");
        sWriter->String(m_Color.StrKey().c_str());
        sWriter->Key("w");
        sWriter->Int(tInt(m_Width.Pixels()));
        sWriter->Key("s");
        sWriter->Int(tInt(m_BorderStyle));
        sWriter->EndObject();
    }


	tBool tBorderCss::operator == (tBorderCss& sBorder) {
		return((m_Color == sBorder.m_Color) &&
			   (m_Width == sBorder.m_Width) &&
			   (m_BorderStyle == sBorder.m_BorderStyle));
	}
    /*
    tBorderCss tBorderCss::operator=(const tBorderCss& sBorderCss) {
        m_Color=sBorderCss.m_Color;
        m_Width=sBorderCss.m_Width;
        m_BorderStyle=sBorderCss.m_BorderStyle;
        return(*this);
    }
    */
	// SkBorderRect ===========================================================
	tBorderRectCss::tBorderRectCss() : tFormatShare(),m_All(), m_Left(), m_Top(), m_Right(), m_Bottom(), m_Radius(), m_TopLeftRadius(), m_TopRightRadius(), m_BottomLeftRadius(), m_BottomRightRadius(){}
	tBorderRectCss::tBorderRectCss(const tBorderRectCss& sBorderRect) : tFormatShare(sBorderRect), 
							m_All(sBorderRect.m_All),
							m_Left(sBorderRect.m_Left), 
							m_Top(sBorderRect.m_Top), 
							m_Right(sBorderRect.m_Right), 
							m_Bottom(sBorderRect.m_Bottom),
							m_Radius(sBorderRect.m_Radius),
							m_TopLeftRadius(sBorderRect.m_TopLeftRadius), 
							m_TopRightRadius(sBorderRect.m_TopRightRadius),
							m_BottomLeftRadius(sBorderRect.m_BottomLeftRadius),
							m_BottomRightRadius(sBorderRect.m_BottomRightRadius) {}

	void tBorderRectCss::Clear() {
		tFormatShare::Clear();
		m_All.Clear();
		m_Left.Clear();
		m_Top.Clear();
		m_Right.Clear();
		m_Bottom.Clear();
		m_Radius=tUnitCss(0, tUnitMetrics::none);
		m_TopLeftRadius = tUnitCss(0, tUnitMetrics::none);
		m_TopRightRadius = tUnitCss(0, tUnitMetrics::none);
		m_BottomLeftRadius = tUnitCss(0, tUnitMetrics::none);
		m_BottomRightRadius = tUnitCss(0, tUnitMetrics::none);
	}

	tString tBorderRectCss::Key() {
		tStringStream wStream;
		if (!m_All.Empty()) {
			wStream << "a" << m_All.Color().Key() << ":" << m_All.Width() << ":" << tInt(m_All.BorderStyle());
		}
		if (!m_Left.Empty()) {
			wStream << "l" << m_Left.Color().Key() << ":" << m_Left.Width() << ":" << tInt(m_Left.BorderStyle());
		}
		if (!m_Top.Empty()) {
			wStream << "t" << m_Top.Color().Key() << ":" << m_Top.Width() << ":" << tInt(m_Top.BorderStyle());
		}
		if (!m_Right.Empty()) {
			wStream << "r" << m_Right.Color().Key() << ":" << m_Right.Width() << ":" << tInt(m_Right.BorderStyle());
		}
		if (!m_Bottom.Empty()) {
			wStream << "b" << m_Bottom.Color().Key() << ":" << m_Bottom.Width() << ":" << tInt(m_Bottom.BorderStyle());
		}

		if (!m_Radius.Empty()) {
			wStream << "ra" << m_Radius.Str();
		}

		if (!m_TopLeftRadius.Empty()) {
			wStream << "tl" << m_TopLeftRadius.Str();
		}
		if (!m_TopRightRadius.Empty()) {
			wStream << "tr" << m_TopRightRadius.Str();
		}
		if (!m_BottomLeftRadius.Empty()) {
			wStream << "bl" << m_BottomLeftRadius.Str();
		}
		if (!m_BottomRightRadius.Empty()) {
			wStream << "br" << m_BottomRightRadius.Str();
		}

		return(wStream.str());
	}

	tString tBorderRectCss::Str(tBool sReturn) {
		tStringStream wStream;
        tStringStream wStreamReturn;
        wStreamReturn << ";";
        if (sReturn) wStreamReturn << endl;
        
		tBool wOk = false;
		if (!m_All.Empty()) {
			wStream << m_All.Str("border");
			wOk = true;
		}
		if (!m_Left.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << m_Left.Str("border-left");
            wOk = true;
        }
		if (!m_Top.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << m_Top.Str("border-top");
            wOk = true;
		}
		if (!m_Right.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << m_Right.Str("border-right");
            wOk = true;
		}
        
		if (!m_Bottom.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << m_Bottom.Str("border-bottom");
            wOk = true;
		}

		if (!m_Radius.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "border-radius:" << m_Radius.Str();
		}

		if (!m_TopLeftRadius.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "border-top-left-radius:" << m_TopLeftRadius.Str();
		}
		if (!m_TopRightRadius.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "border-top-right-radius:" << m_TopRightRadius.Str();
		}
		if (!m_BottomLeftRadius.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "border-bottom-left-radius:" << m_BottomLeftRadius.Str();
		}
		if (!m_BottomRightRadius.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
			wStream << "border-bottom-right-radius:" << m_BottomRightRadius.Str();
		}
       
		return(wStream.str());
	}

    void  tBorderRectCss::Merge(tBorderRectCss*  sBorder) {
        //= Border ============================================================
        if (!sBorder->All().Empty()) {
            m_All = sBorder->All();
        }
        if (!sBorder->Left().Empty()) {
            m_Left = sBorder->Left();
        }
        if (!sBorder->Top().Empty()) {
            m_Top = sBorder->Top();
        }
        if (!sBorder->Right().Empty()) {
            m_Right = sBorder->Right();
        }
        if (!sBorder->Bottom().Empty()) {
            m_Bottom = sBorder->Bottom();
        }
        // Radius =============================================================
        if (!sBorder->Radius().Empty()) {
            m_Radius = sBorder->Radius();
        }
        if (!sBorder->TopLeftRadius().Empty()) {
            m_TopLeftRadius = sBorder->TopLeftRadius();
        }
        if (!sBorder->TopRightRadius().Empty()) {
            m_TopRightRadius = sBorder->TopRightRadius();
        }
        if (!sBorder->BottomRightRadius().Empty()) {
            m_BottomRightRadius = sBorder->BottomRightRadius();
        }
        if (!sBorder->BottomLeftRadius().Empty()) {
            m_BottomLeftRadius = sBorder->BottomLeftRadius();
        }
    }


	tBorderCss& tBorderRectCss::All() { return(m_All); }
	tBorderCss& tBorderRectCss::Left() { return(m_Left); }
	tBorderCss& tBorderRectCss::Top() { return(m_Top); }
	tBorderCss& tBorderRectCss::Right() { return(m_Right); }
	tBorderCss& tBorderRectCss::Bottom() { return(m_Bottom); }

	tUnitCss& tBorderRectCss::Radius() { return(m_Radius); }
	tUnitCss& tBorderRectCss::TopLeftRadius() { return(m_TopLeftRadius); }
	tUnitCss& tBorderRectCss::TopRightRadius() { return(m_TopRightRadius); }
	tUnitCss& tBorderRectCss::BottomRightRadius() { return(m_BottomRightRadius); }
	tUnitCss& tBorderRectCss::BottomLeftRadius() { return(m_BottomLeftRadius); }

	tBool tBorderRectCss::Empty() {
		return(m_All.Empty() &&
			m_Left.Empty() &&
			m_Top.Empty() &&
			m_Right.Empty() &&
			m_Bottom.Empty() &&
			m_Radius.Empty() &&
			m_TopLeftRadius.Empty() &&
			m_TopRightRadius.Empty() &&
			m_BottomLeftRadius.Empty() &&
			m_BottomRightRadius.Empty());
	}

    void tBorderRectCss::ApplyUnit(tVectorUnitCss* sVector) {
        switch (sVector->size()) {
            case 0: break;
            case 1: {
                m_All.Width((*sVector)[0]);
                break;
            }
            case 2: {
                /* top and bottom | left and right */
                m_Top=m_All;
                m_Bottom=m_All;
                m_Top.Width((*sVector)[0]);
                m_Bottom.Width((*sVector)[1]);
                
                m_Left=m_All;
                m_Right=m_All;
                m_Left.Width((*sVector)[0]);
                m_Right.Width((*sVector)[1]);
                m_All.Clear();
                break;
            }
            case 3: {
                /* top | left and right | bottom */
                m_Top=m_All;
                m_Top.Width((*sVector)[0]);
         
                m_Left=m_All;
                m_Right=m_All;
                m_Left.Width((*sVector)[1]);
                m_Right.Width((*sVector)[1]);
         
                m_Bottom=m_All;
                m_Bottom.Width((*sVector)[2]);
                break;
            }
            case 4:
                
            default:
                /* top | right | bottom | left */
                m_Top=m_All;
                m_Top.Width((*sVector)[0]);
                m_Right=m_All;
                m_Right.Width((*sVector)[1]);
                m_Bottom=m_All;
                m_Bottom.Width((*sVector)[2]);
                m_Left=m_All;
                m_Left.Width((*sVector)[3]);
                m_All.Clear();
   
                break;
        }
    }

    void tBorderRectCss::ApplyRadiusUnit(tVectorUnitCss* sVector) {
        switch (sVector->size()) {
            case 0: break;
            case 1: {
                m_Radius=(*sVector)[0];
                break;
            }
            case 2: {
                /* top-left and bottom-right | top-right and bottom-left  */
                m_TopLeftRadius=(*sVector)[0];
                m_BottomRightRadius=(*sVector)[0];
  
                m_TopRightRadius=(*sVector)[1];
                m_BottomLeftRadius=(*sVector)[1];
                break;
            }
            case 3: {
                /* top-left | top-right  and bottom-left  | bottom-right */
                m_TopLeftRadius=(*sVector)[0];
                m_TopRightRadius=(*sVector)[1];
                m_BottomLeftRadius=(*sVector)[1];
                m_BottomRightRadius=(*sVector)[2];
                break;
            }
            case 4:
            default:
                /* top-left | top-right | bottom-right | bottom-left */
                m_TopLeftRadius=(*sVector)[0];
                m_TopRightRadius=(*sVector)[1];
                m_BottomRightRadius=(*sVector)[2];
                m_BottomLeftRadius=(*sVector)[3];
                break;
        }
    }

    tBool tBorderRectCss::DeleteBorder(tShort sBorderMask) {
        tBool wResult=false;
        if ((sBorderMask & tBorderAll) == tBorderAll) {
            if (!m_All.Empty()) { m_All.Clear(); wResult=true; }
            if (!m_Left.Empty()) { m_Left.Clear(); wResult=true; }
            if (!m_Top.Empty()) { m_Top.Clear(); wResult=true;  }
            if (!m_Right.Empty()) { m_Right.Clear(); wResult=true;  }
            if (!m_Bottom.Empty()) { m_Bottom.Clear(); wResult=true;  }
            return(wResult);
        }
        
        if ((sBorderMask & tBorderLeft) == tBorderLeft) {
            if (!m_All.Empty()) {
                if (!m_Left.Empty()) { m_Left.Clear();  }
                if (m_Top.Empty()) { m_Top=m_All;  }
                if (m_Right.Empty()) { m_Right=m_All;  }
                if (m_Bottom.Empty()) { m_Bottom=m_All;  }
                m_All.Clear();
                return(true);
            }
            if (!m_Left.Empty()) { m_Left.Clear(); wResult=true; }
        }
        
        if ((sBorderMask & tBorderTop) == tBorderTop) {
            if (!m_All.Empty()) {
                if (m_Left.Empty()) { m_Left=m_All;  }
                if (!m_Top.Empty()) { m_Top.Clear();  }
                if (m_Right.Empty()) { m_Right=m_All;  }
                if (m_Bottom.Empty()) { m_Bottom=m_All;  }
                m_All.Clear();
                return(true);
            }
            if (!m_Top.Empty()) { m_Top.Clear(); wResult=true; }
        }
        
        if ((sBorderMask & tBorderRight) == tBorderRight) {
            if (!m_All.Empty()) {
                if (m_Left.Empty()) { m_Left=m_All;  }
                if (m_Top.Empty()) { m_Top = m_All;  }
                if (!m_Right.Empty()) { m_Right.Clear();  }
                if (m_Bottom.Empty()) { m_Bottom=m_All;  }
                m_All.Clear();
                return(true);
            }
            if (!m_Right.Empty()) { m_Right.Clear(); wResult=true; }
        }
        
        
        if ((sBorderMask & tBorderBottom) == tBorderBottom) {
            if (!m_All.Empty()) {
                if (m_Left.Empty()) { m_Left=m_All;  }
                if (m_Top.Empty()) { m_Top = m_All;  }
                if (m_Right.Empty()) { m_Right=m_All;  }
                if (!m_Bottom.Empty()) { m_Bottom.Clear();  }
                m_All.Clear();
                return(true);
            }
            if (!m_Bottom.Empty()) { m_Bottom.Clear(); wResult=true; }
        }
        
        return(wResult);
    }


    tShort tBorderRectCss::BorderMask() {
        tShort wResult=0;
        if (!m_All.Empty()) {   wResult+=tBorderAll; }
        if (!m_Left.Empty()) {   wResult+=tBorderLeft; }
        if (!m_Top.Empty()) {   wResult+=tBorderTop; }
        if (!m_Right.Empty()) {   wResult+=tBorderRight; }
        if (!m_Bottom.Empty()) {   wResult+=tBorderBottom; }
        return(wResult);
    }


	void tBorderRectCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		if (!m_All.Empty()) {	sWriter->Key("a"); m_All.Json(sWriter); }
		if (!m_Left.Empty()) { sWriter->Key("l"); m_Left.Json(sWriter); }
		if (!m_Top.Empty()) { sWriter->Key("t"); m_Top.Json(sWriter); }
		if (!m_Right.Empty()) { sWriter->Key("r"); m_Right.Json(sWriter); }
		if (!m_Bottom.Empty()) { sWriter->Key("b"); m_Bottom.Json(sWriter); }
		if (!m_Radius.Empty()) { sWriter->Key("ra"); m_Radius.Json(sWriter); }
		if (!m_TopLeftRadius.Empty()) { sWriter->Key("tl"); m_TopLeftRadius.Json(sWriter); }
		if (!m_TopRightRadius.Empty()) { sWriter->Key("tr"); m_TopRightRadius.Json(sWriter); }
		if (!m_BottomLeftRadius.Empty()) { sWriter->Key("bl"); m_BottomLeftRadius.Json(sWriter); }
		if (!m_BottomRightRadius.Empty()) { sWriter->Key("br"); m_BottomRightRadius.Json(sWriter); }
		sWriter->EndObject();
	}

	void tBorderRectCss::Json(const rapidjson::Value& sValue) {
		if (sValue.HasMember("a")) m_All.Json(sValue["a"]);
		if (sValue.HasMember("l")) m_Left.Json(sValue["l"]);
		if (sValue.HasMember("t")) m_Top.Json(sValue["t"]);
		if (sValue.HasMember("r")) m_Right.Json(sValue["r"]);
		if (sValue.HasMember("b")) m_Bottom.Json(sValue["b"]);
		if (sValue.HasMember("ra")) m_Radius.Json(sValue["ra"]);
		if (sValue.HasMember("tl")) m_TopLeftRadius.Json(sValue["tl"]);
		if (sValue.HasMember("tr")) m_TopRightRadius.Json(sValue["tr"]);
		if (sValue.HasMember("bl")) m_BottomLeftRadius.Json(sValue["bl"]);
		if (sValue.HasMember("br")) m_BottomRightRadius.Json(sValue["br"]);
	}

    void tBorderRectCss::JsonJavaScript(Writer<StringBuffer>* sWriter) {
        if (!m_All.Empty()) {    sWriter->Key("f_bo"); sWriter->String(m_All.Str().c_str()); }
        if (!m_Left.Empty()) { sWriter->Key("f_bol"); sWriter->String(m_Left.Str().c_str()); }
        if (!m_Top.Empty()) { sWriter->Key("f_bot"); sWriter->String(m_Top.Str().c_str()); }
        if (!m_Right.Empty()) { sWriter->Key("f_bor");sWriter->String(m_Right.Str().c_str());  }
        if (!m_Bottom.Empty()) { sWriter->Key("f_bob"); sWriter->String(m_Bottom.Str().c_str()); }

        if (!m_Radius.Empty()) { sWriter->Key("f_ra"); sWriter->String(m_Radius.Str().c_str()); }
        if (!m_TopLeftRadius.Empty()) { sWriter->Key("f_ratl"); sWriter->String(m_TopLeftRadius.Str().c_str()); }
        if (!m_TopRightRadius.Empty()) { sWriter->Key("f_ratr"); sWriter->String(m_TopRightRadius.Str().c_str()); }
        if (!m_BottomLeftRadius.Empty()) { sWriter->Key("f_rabl"); sWriter->String(m_BottomLeftRadius.Str().c_str()); }
        if (!m_BottomRightRadius.Empty()) { sWriter->Key("f_rabr"); sWriter->String(m_BottomRightRadius.Str().c_str()); }
    }

    void tBorderRectCss::JsonJavaScriptCanvas(Writer<StringBuffer>* sWriter) {
        if (!m_All.Empty()) { sWriter->Key("f_bo"); m_All.JsonJavaScriptCanvas(sWriter); }
        if (!m_Left.Empty()) { sWriter->Key("f_bol"); m_Left.JsonJavaScriptCanvas(sWriter); }
        if (!m_Top.Empty()) { sWriter->Key("f_bot"); m_Top.JsonJavaScriptCanvas(sWriter); }
        if (!m_Right.Empty()) { sWriter->Key("f_bor"); m_Right.JsonJavaScriptCanvas(sWriter);  }
        if (!m_Bottom.Empty()) { sWriter->Key("f_bob"); m_Bottom.JsonJavaScriptCanvas(sWriter); }

        if (!m_Radius.Empty()) { sWriter->Key("f_ra"); sWriter->Int(tInt(m_Radius.Pixels())); }
        if (!m_TopLeftRadius.Empty()) { sWriter->Key("f_ratl"); sWriter->Int(tInt(m_TopLeftRadius.Pixels())); }
        if (!m_TopRightRadius.Empty()) { sWriter->Key("f_ratr"); sWriter->Int(tInt(m_TopRightRadius.Pixels())); }
        if (!m_BottomLeftRadius.Empty()) { sWriter->Key("f_rabl"); sWriter->Int(tInt(m_BottomLeftRadius.Pixels())); }
        if (!m_BottomRightRadius.Empty()) { sWriter->Key("f_rabr"); sWriter->Int(tInt(m_BottomRightRadius.Pixels())); }
    }


}
