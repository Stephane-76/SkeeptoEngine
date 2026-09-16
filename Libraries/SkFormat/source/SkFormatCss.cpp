//=============================================================================
// SkFormat (Sub system of Css )
//=============================================================================

#include "../include/SkFormatCss.hpp"
#include "../include/SkFormatRoot.hpp"
#include <SkVariant.hpp>

namespace SkFormat {


    // SkAllocatorFormatCss ===================================================
    tFormatCss::tFormatCss() : tFormatShare(),
        m_Width(tUnitCss(0, tUnitMetrics::none)),
        m_Height(tUnitCss(0, tUnitMetrics::none)),
        m_Margin(0),
        m_Padding(0),
        m_Shadow(0),
        m_Color(),
        m_BackgroundColor(),
        m_BorderRect(0),
        m_Font(0),
        m_Text(0),
        m_CountCell(0)
    #ifdef checkfo
        ,m_CountCellCheck(0),
        m_FormatCell(false)
    #endif
    {}


    tFormatCss::tFormatCss(tFormatCss& sFormatCss) : tFormatShare(),
        m_Width(sFormatCss.m_Width),
        m_Height(sFormatCss.m_Height),
        m_Margin(sFormatCss.m_Margin),
        m_Padding(sFormatCss.m_Padding),
        m_Shadow(sFormatCss.m_Shadow),
        m_Color(sFormatCss.m_Color),
        m_BackgroundColor(sFormatCss.m_BackgroundColor),
        m_BorderRect(sFormatCss.m_BorderRect),
        m_Font(sFormatCss.m_Font),
        m_Text(sFormatCss.m_Text),
        m_CountCell(0)
    #ifdef checkfo
        ,m_CountCellCheck(0),
        m_FormatCell(false)
    #endif
    {}

    void tFormatCss::Clear() {
        tFormatShare::Clear();
        ClearComponent();
    #ifdef checkfo
        m_CountCellCheck=0,
        m_FormatCell=false;
    #endif
    }

    void tFormatCss::ClearComponent() {
        m_Color.NotUse(true);
        m_BackgroundColor.NotUse(true);
        
        m_Width = tUnitCss(0, tUnitMetrics::none);
        m_Height = tUnitCss(0, tUnitMetrics::none);
        m_Padding = 0;
        m_Margin = 0;
        m_BorderRect = 0;
        m_Shadow = 0;
        m_Font = 0;
        m_Text = 0;
        //m_CountCell = 0;
    }

    tBool tFormatCss::IsEmpty() {
        return ((m_Color.NotUse()==true) &&
                (m_BackgroundColor.NotUse()==true) &&
                (m_Width.Empty()) &&
                (m_Height.Empty()) &&
                (m_Padding== 0) &&
                (m_Margin == 0) &&
                (m_BorderRect == 0) &&
                (m_Shadow == 0) &&
                (m_Font == 0) &&
                (m_Text == 0));
    }

    void tFormatCss::IncCell() { m_CountCell++; }

    void tFormatCss::DecCell() {
        assert(m_CountCell>0);
        m_CountCell--;
    }

    tInt tFormatCss::Cell() { return(m_CountCell); }
    
	void tFormatCss::Width(tUnitCss sWidth) { m_Width = sWidth; }
	tUnitCss tFormatCss::Width() { return(m_Width); }

	void tFormatCss::Height(tUnitCss sHeight) { m_Height = sHeight; }
	tUnitCss tFormatCss::Height() { return(m_Height); }

	tString tFormatCss::RootKey(tFormatRoot* sFormatRoot) {
		tStringStream wStream;

		// Size
		if (!m_Width.Empty()) wStream << "w" << m_Width;
		if (!m_Height.Empty()) wStream << "h" << m_Height;

		// Color
		if (!m_Color.NotUse())	wStream << "c" << m_Color.Key();
		
		// BackgroundColor
		if (!m_BackgroundColor.NotUse()) wStream << "bc" << m_BackgroundColor.Key();
		
		// Margin
		if (m_Margin != 0) wStream << "m" << m_Margin;
		// Padding
		if (m_Padding != 0) wStream << "p" << m_Padding;

		// Font
		if (m_Font != 0) wStream << "f" << m_Font;
		// Text
		if (m_Text != 0) wStream << "t" << m_Text;

		// BorderRect
		if (m_BorderRect != 0) wStream << "b" << m_BorderRect;

		// Shadow
		if (m_Shadow != 0) wStream << "bs" << m_Shadow;

		return(wStream.str());
	};

	
	tString tFormatCss::Str(tFormatRoot* sFormatRoot,tBool sReturn) {
		tStringStream  wStream;
        tStringStream wStreamReturn;
        wStreamReturn << ";";
        if (sReturn) wStreamReturn << endl;
        
		tBool wOk = false;
		// Width
		if (!m_Width.Empty()) {
			wOk = true;
			wStream << "width:" << m_Width;
		}
		// Height
		if (!m_Height.Empty()) {
			if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "height:" << m_Height;
		}

		// Color
		if (!m_Color.NotUse()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "color:" << m_Color.StrKey();
		}
		// BackgroundColor
		if (!m_BackgroundColor.NotUse()) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << "background-color:" << m_BackgroundColor.StrKey();
		}

		// Margin
		tUnitRectCss* wMargin = sFormatRoot->Margin(m_Margin);
		if (wMargin != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << wMargin->Str("margin",sReturn);
		}

		// Padding
		tUnitRectCss* wPadding = sFormatRoot->Padding(m_Padding);
		if (wPadding != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << wPadding->Str("padding",sReturn);
		}

		// Shadow
		tShadowCss* wShadow = sFormatRoot->Shadow(m_Shadow);
		if (wShadow != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << wShadow->Str();
		}

		// Font
		SkFormat::tFontCss* wFont = sFormatRoot->Font(m_Font);
		if (wFont != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
			wOk = true;
			wStream << wFont->Str(sReturn);
		}

		// Text
		SkFormat::tTextCss* wText = sFormatRoot->Text(m_Text);
		if (wText != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << wText->Str(sReturn);
		}

		// BorderRect
		tBorderRectCss* wBorderRect = sFormatRoot->BorderRect(m_BorderRect);
		if (wBorderRect != nullptr) {
            if (wOk) wStream << wStreamReturn.str();
			wStream << wBorderRect->Str(sReturn);
            wOk=true;
        }
        if (wOk) wStream << wStreamReturn.str();

		return(wStream.str());
	}

	tColorCss& tFormatCss::Color() { return(m_Color); }

	tColorCss& tFormatCss::BackgroundColor() { return(m_BackgroundColor); }

	void tFormatCss::Margin(tFormatRef sMarginRef) { m_Margin = sMarginRef; }
	tFormatRef tFormatCss::Margin() { return(m_Margin); }
	tUnitRectCss* tFormatCss::Margin(tFormatRoot* sFormatRoot) { 
		if (m_Margin != 0) return(sFormatRoot->Margin(m_Margin));
		return(nullptr);
	}


	void tFormatCss::Padding(tFormatRef sPaddingRef) { m_Padding = sPaddingRef; }
	tFormatRef tFormatCss::Padding() { return(m_Padding); }
	tUnitRectCss* tFormatCss::Padding(tFormatRoot* sFormatRoot) {
		if (m_Padding != 0) return(sFormatRoot->Padding(m_Padding));
		return(nullptr);
	}

	void tFormatCss::Shadow(tFormatRef sShadowRef) { m_Shadow = sShadowRef; }
	tFormatRef tFormatCss::Shadow() { return(m_Shadow); }
	tShadowCss* tFormatCss::Shadow(tFormatRoot* sFormatRoot) {
		if (m_Shadow != 0) return(sFormatRoot->Shadow(m_Shadow));
		return(nullptr);
	}

	void tFormatCss::Font(tFormatRef sFontRef) { m_Font = sFontRef; }
	tFormatRef tFormatCss::Font() { return(m_Font); }
	tFontCss* tFormatCss::Font(tFormatRoot* sFormatRoot) {
		if (m_Font != 0) return(sFormatRoot->Font(m_Font));
		return(nullptr);
	}


	void tFormatCss::Text(tFormatRef sTextRef) { m_Text = sTextRef; }
	tFormatRef tFormatCss::Text() { return(m_Text); }
	tTextCss* tFormatCss::Text(tFormatRoot* sFormatRoot) {
		if (m_Text != 0) return(sFormatRoot->Text(m_Text));
		return(nullptr);
	}

	void tFormatCss::BorderRect(tFormatRef sBorderRectRef) { m_BorderRect = sBorderRectRef; }
	tFormatRef tFormatCss::BorderRect() { return(m_BorderRect); }
	tBorderRectCss* tFormatCss::BorderRect(tFormatRoot* sFormatRoot) {
		if (m_BorderRect != 0) return(sFormatRoot->BorderRect(m_BorderRect));
		return(nullptr);
	}

	void tFormatCss::Json(Writer<StringBuffer>* sWriter, tFormatRoot* sFormatRoot) {
		sWriter->StartObject();
		if (!m_Width.Empty()) { sWriter->Key("w"); m_Width.Json(sWriter); }
		if (!m_Height.Empty()) { sWriter->Key("h"); m_Height.Json(sWriter); }

		if (!m_Color.NotUse()) { sWriter->Key("c"); m_Color.Json(sWriter); }
		if (!m_BackgroundColor.NotUse()) { sWriter->Key("bc"); m_BackgroundColor.Json(sWriter); }
		if (m_Margin != 0) { sWriter->Key("m"); sFormatRoot->Margin(m_Margin)->Json(sWriter); }
		if (m_Padding != 0) { sWriter->Key("p"); sFormatRoot->Padding(m_Padding)->Json(sWriter); }

		if (m_Font != 0) { sWriter->Key("f"); sFormatRoot->Font(m_Font)->Json(sWriter); };
		if (m_Text != 0) { sWriter->Key("t"); sFormatRoot->Text(m_Text)->Json(sWriter); };

		if (m_BorderRect != 0) { sWriter->Key("br"); sFormatRoot->BorderRect(m_BorderRect)->Json(sWriter); }
		if (m_Shadow != 0) { sWriter->Key("sha"); sFormatRoot->Shadow(m_Shadow)->Json(sWriter); }
		sWriter->EndObject();
	}

	void tFormatCss::Json(const rapidjson::Value& sValue, tFormatRoot* sFormatRoot) {
		if (sValue.HasMember("w")) { m_Width.Json(sValue["w"]); }
		if (sValue.HasMember("h")) { m_Height.Json(sValue["h"]); }

		if (sValue.HasMember("c")) { m_Color.Json(sValue["c"]); }
		if (sValue.HasMember("bc")) { m_BackgroundColor.Json(sValue["bc"]); }
		if (sValue.HasMember("m")) { tUnitRectCss wMargin; wMargin.Json(sValue["m"]);	sFormatRoot->Margin(wMargin); }
		if (sValue.HasMember("p")) { tUnitRectCss wPadding; wPadding.Json(sValue["p"]);	sFormatRoot->Padding(wPadding); }

		if (sValue.HasMember("f")) { tFontCss wFont; wFont.Json(sValue["f"]);	sFormatRoot->Font(wFont); }
		if (sValue.HasMember("t")) { tTextCss wText; wText.Json(sValue["t"]);	sFormatRoot->Text(wText); }

		if (sValue.HasMember("br")) { tBorderRectCss wBorderRect; wBorderRect.Json(sValue["br"]);	sFormatRoot->BorderRect(wBorderRect); }
		if (sValue.HasMember("sha")) { tShadowCss wShadow; wShadow.Json(sValue["sha"]); 	sFormatRoot->Shadow(wShadow); }
	}

	tFormatCss& tFormatCss::operator=(const tFormatCss& sFormatCss) {
		assert(this != &sFormatCss);
		m_Width = sFormatCss.m_Width;
		m_Height = sFormatCss.m_Height;

		m_Color = sFormatCss.m_Color;
		m_BackgroundColor = sFormatCss.m_BackgroundColor;

		m_Padding = sFormatCss.m_Padding;
		m_Margin = sFormatCss.m_Margin;

		m_Font = sFormatCss.m_Font;
		m_Text = sFormatCss.m_Text;

		m_BorderRect= sFormatCss.m_BorderRect;
		m_Shadow = sFormatCss.m_Shadow;
		return(*this);
	}

    tFormatRef tFormatCss::CountCell() {
        return(m_CountCell);
    }

	tString tFormatCss::Debug(tFormatRoot* sFormatRoot) {
		tStringStream wStream;
		wStream << "[";
		// Width
		if (!m_Width.Empty()) {
			wStream << "w:" << m_Width << " ";;
		}

		//Height
		if (!m_Height.Empty()) {
			wStream << "h:" << m_Height << " ";;
		}

		// Color
		if (!m_Color.NotUse()) {
			wStream << "c:" << m_Color.StrKey() << " o:" << m_Color.Opacity() << " ";
		}
		if (!m_BackgroundColor.NotUse()) {
			wStream << "bc:" << m_BackgroundColor.StrKey() << " o:" << m_Color.Opacity() << " ";
		}
		// Margin
		tUnitRectCss* wMargin = sFormatRoot->Margin(m_Margin);
		if (wMargin != nullptr) {
			wStream << "m:" << wMargin->Key() <<  " Nb: "<< wMargin->Count() << " Ref:" << m_Margin << " ";
		}

		// Padding
		tUnitRectCss* wPadding = sFormatRoot->Padding(m_Padding);
		if (wPadding != nullptr) {
			wStream << "p:" << wPadding->Key() << " Nb: " << wPadding->Count() << " Ref:" << m_Padding << " ";
		}

		// Shadow
		tShadowCss* wShadow = sFormatRoot->Shadow(m_Shadow);
		if (wShadow != nullptr) {
			wStream << "bs:" << wShadow->Key() << " Nb: " << wShadow->Count() << " Ref:" << m_Shadow << " ";
		}

		// Font
		SkFormat::tFontCss* wFont = sFormatRoot->Font(m_Font);
		if (wFont != nullptr) {
			wStream <<  "f:" << wFont->Key() <<  " Nb: " << wFont->Count() << " Ref:" << m_Font << " ";
		}

		// Text
		SkFormat::tTextCss* wText = sFormatRoot->Text(m_Text);
		if (wText != nullptr) {
			wStream << "t:" << wText->Key() << " Nb: " << wText->Count() << " Ref:" << m_Text << " ";
		}

		// BorderRect
		tBorderRectCss* wBorderRect = sFormatRoot->BorderRect(m_BorderRect);
		if (wBorderRect != nullptr) {
			wStream << "b:" << wBorderRect->Key() << " Nb: " << wBorderRect->Count() << " Ref:" << m_BorderRect << " ";
		}

		wStream << "]";
        wStream << " Real " << m_CountCell;
#ifdef checkfo
        wStream << " Check " << m_CountCellCheck;
#endif
        
		return(wStream.str());
	};

#ifdef checkfo
    void tFormatCss::FormatCell(tBool sFormatCell) {
        m_FormatCell=sFormatCell;
    }

    tBool tFormatCss::FormatCell() { return(m_FormatCell); }

    void tFormatCss::ResetCellCheck() {
        m_CountCellCheck=0;
    }

    void tFormatCss::IncCountCellCheck() {
        m_CountCellCheck++;
    }

	void tFormatCss::ResetCheck(tFormatRoot* sFormatRoot) {
		// Margin
		tUnitRectCss* wMargin = sFormatRoot->Margin(m_Margin);
		if (wMargin != nullptr) {
			wMargin->ResetCheck();
		}

		// Padding
		tUnitRectCss* wPadding = sFormatRoot->Padding(m_Padding);
		if (wPadding != nullptr) {
			wPadding->ResetCheck();
		}

		// Shadow
		tShadowCss* wShadow = sFormatRoot->Shadow(m_Shadow);
		if (wShadow != nullptr) {
			wShadow->ResetCheck();
		}

		// Font
		SkFormat::tFontCss* wFont = sFormatRoot->Font(m_Font);
		if (wFont != nullptr) {
			wFont->ResetCheck();
		}

		// Text
		SkFormat::tTextCss* wText = sFormatRoot->Text(m_Text);
		if (wText != nullptr) {
			wText->ResetCheck();
		}

		// BorderRect
		tBorderRectCss* wBorderRect = sFormatRoot->BorderRect(m_BorderRect);
		if (wBorderRect != nullptr) {
			wBorderRect->ResetCheck();
		}
	}

	void tFormatCss::IncCheck(tFormatRoot* sFormatRoot) {
		// Margin
		tUnitRectCss* wMargin = sFormatRoot->Margin(m_Margin);
		if (wMargin != nullptr) {
			wMargin->IncCheck();
		}

		// Padding
		tUnitRectCss* wPadding = sFormatRoot->Padding(m_Padding);
		if (wPadding != nullptr) {
			wPadding->IncCheck();
		}

		// Shadow
		tShadowCss* wShadow = sFormatRoot->Shadow(m_Shadow);
		if (wShadow != nullptr) {
			wShadow->IncCheck();
		}

		// Font
		SkFormat::tFontCss* wFont = sFormatRoot->Font(m_Font);
		if (wFont != nullptr) {
			wFont->IncCheck();
		}

		// Text
		SkFormat::tTextCss* wText = sFormatRoot->Text(m_Text);
		if (wText != nullptr) {
			wText->IncCheck();
		}

		// BorderRect
		tBorderRectCss* wBorderRect = sFormatRoot->BorderRect(m_BorderRect);
		if (wBorderRect != nullptr) {
			wBorderRect->IncCheck();
		}
	}

    void tFormatCss::CheckCell(tFormatRoot* sFormatRoot) {
        if (m_CountCell!=m_CountCellCheck ) {
            cerr << "Error " << Str(sFormatRoot)  << "Real:" <<  m_CountCell << "== Check:" << m_CountCellCheck << endl;
        }
        //assert(m_CountCell==m_CountCellCheck);
    }

	void tFormatCss::Check(tFormatRoot* sFormatRoot) {
		// Margin
		tUnitRectCss* wMargin = sFormatRoot->Margin(m_Margin);
		if (wMargin != nullptr) {
			wMargin->Check();
		}

		// Padding
		tUnitRectCss* wPadding = sFormatRoot->Padding(m_Padding);
		if (wPadding != nullptr) {
			wPadding->Check();
		}

		// Shadow
		tShadowCss* wShadow = sFormatRoot->Shadow(m_Shadow);
		if (wShadow != nullptr) {
			wShadow->Check();
		}

		// Font
		SkFormat::tFontCss* wFont = sFormatRoot->Font(m_Font);
		if (wFont != nullptr) {
			wFont->Check();
		}

		// Text
		SkFormat::tTextCss* wText = sFormatRoot->Text(m_Text);
		if (wText != nullptr) {
			wText->Check();
		}

		// BorderRect
		tBorderRectCss* wBorderRect = sFormatRoot->BorderRect(m_BorderRect);
		if (wBorderRect != nullptr) {
			wBorderRect->Check();
		}
	}
#endif // checkfo

    ///!  FormatMergeCss for merge FormatCss i n memory before apply ==============================
    tFormatMergeCss::tFormatMergeCss() :  tClass(),
    m_Width(tUnitCss(0, tUnitMetrics::none)),
    m_Height(tUnitCss(0, tUnitMetrics::none)),
    m_Margin(),
    m_Padding(),
    m_Shadow(),
    m_Color(),
    m_BackgroundColor(),
    m_BorderRect(),
    m_Font(),
    m_Text()
    {}

    void tFormatMergeCss::Clear() {
        m_Width=tUnitCss(0,tUnitMetrics::none);
        m_Height=tUnitCss(0,tUnitMetrics::none);
        m_Margin.Clear();
        m_Padding.Clear();
        m_Shadow.Clear();
        m_Color.NotUse(true);
        m_BackgroundColor.NotUse(true);
        m_BorderRect.Clear();
        m_Font.Clear();
        m_Text.Clear();
    }

    tString tFormatMergeCss::tFormatMergeCss::Str(tBool sReturn) {
        tStringStream  wStream;
        tStringStream wStreamReturn;
        wStreamReturn << ";";
        if (sReturn) wStreamReturn << endl;
        
        tBool wOk = false;
        // Width
        if (!m_Width.Empty()) {
            wOk = true;
            wStream << "width:" << m_Width;
        }
        // Height
        if (!m_Height.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << "height:" << m_Height;
        }

        // Color
        if (!m_Color.NotUse()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << "color:" << m_Color.StrKey();
        }
        // BackgroundColor
        if (!m_BackgroundColor.NotUse()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << "background-color:" << m_BackgroundColor.StrKey();
        }

        // Margin
        if (!m_Margin.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << m_Margin.Str("margin",sReturn);
        }

        // Padding
        if (!m_Padding.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << m_Padding.Str("padding",sReturn);
        }

        // Shadow
        if (!m_Shadow.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << m_Shadow.Str();
        }

        // Font
        if (!m_Font.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << m_Font.Str(sReturn);
        }

        // Text
        if (!m_Text.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << m_Text.Str(sReturn);
        }

        // BorderRect
        if (!m_BorderRect.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << m_BorderRect.Str(sReturn);
            wOk=true;
        }
        if (wOk) wStream << wStreamReturn.str();

        return(wStream.str());
        
    }

    tFormatString* tFormatMergeCss::FormatString() { return(m_Text.FormatString()); }
    
    tBorderRectCss*  tFormatMergeCss::BorderRect() { return(&m_BorderRect); }

    void tFormatMergeCss::JsonJavaScriptVariantDisplay(Writer<StringBuffer>* sWriter,tVariant* sVariant,tBool sCss) {
        if (sVariant==nullptr || sVariant->Type()==tVariantType::t_null) {
            return;
        }
        tFormatString wFormatString=*m_Text.FormatString();
        wFormatString.SetDefaultFormat(sVariant);
        tString wF = sVariant->FormatString(&wFormatString);
        sWriter->Key("f_value");
        sWriter->String(wF.c_str());
        if (wFormatString.Money()!=t_UnitMoney::None) {
            sWriter->Key("f_mo");
            sWriter->String(UnitMoneySymbol(wFormatString.Money()).c_str());
        }
    }

    void tFormatMergeCss::JsonJavaScriptStyle(Writer<StringBuffer>* sWriter,tBool sCss) {
        // Width
        if (!m_Width.Empty()) {
            sWriter->Key("f_w");
            if (sCss) {
                sWriter->String(m_Width.Str().c_str());
            } else {
                sWriter->Int(tInt(m_Width.Pixels()));
            }
        }
        // Height
        if (!m_Height.Empty()) {
            sWriter->Key("f_h");
            if (sCss) {
                sWriter->String(m_Height.Str().c_str());
            } else {
                sWriter->Int(tInt(m_Height.Pixels()));
            }
        }
        //Color
        if (!m_Color.NotUse()) {
            sWriter->Key("f_c");
            sWriter->String(m_Color.StrKey().c_str());
        }
        if (!m_BackgroundColor.NotUse()) {
            sWriter->Key("f_bc");
            sWriter->String(m_BackgroundColor.StrKey().c_str());
        }
        // Css ====================================================================
        if (sCss) {
            if (!m_Margin.Empty()) {
                if (!m_Margin.All().Empty()) { sWriter->Key("f_m"); sWriter->String(m_Margin.All().Str().c_str());  }
                if (!m_Margin.Top().Empty()) { sWriter->Key("f_mt"); sWriter->String(m_Margin.Top().Str().c_str());  }
                if (!m_Margin.Left().Empty()) { sWriter->Key("f_ml"); sWriter->String(m_Margin.Left().Str().c_str());  }
                if (!m_Margin.Bottom().Empty()) { sWriter->Key("f_mb"); sWriter->String(m_Margin.Bottom().Str().c_str());  }
                if (!m_Margin.Right().Empty()) { sWriter->Key("f_mr"); sWriter->String(m_Margin.Right().Str().c_str());  }
            }
            if (!m_Padding.Empty()) {
                if (!m_Padding.All().Empty()) { sWriter->Key("f_p"); sWriter->String(m_Padding.All().Str().c_str());  }
                if (!m_Padding.Top().Empty()) { sWriter->Key("f_pt"); sWriter->String(m_Padding.Top().Str().c_str());  }
                if (!m_Padding.Left().Empty()) { sWriter->Key("f_pl"); sWriter->String(m_Padding.Left().Str().c_str());  }
                if (!m_Padding.Bottom().Empty()) { sWriter->Key("f_pb"); sWriter->String(m_Padding.Bottom().Str().c_str());  }
                if (!m_Padding.Right().Empty()) { sWriter->Key("f_pr"); sWriter->String(m_Padding.Right().Str().c_str());  }
            }
        } else {
        // format for canvas =====================================================
            if (!m_Margin.Empty()) {
                if (!m_Margin.All().Empty()) { sWriter->Key("f_m"); sWriter->Int(tInt(m_Margin.All().Pixels()));  }
                if (!m_Margin.Top().Empty()) { sWriter->Key("f_mt"); sWriter->Int(tInt(m_Margin.Top().Pixels()));  }
                if (!m_Margin.Left().Empty()) { sWriter->Key("f_ml"); sWriter->Int(tInt(m_Margin.Left().Pixels())); }
                if (!m_Margin.Bottom().Empty()) { sWriter->Key("f_mb"); sWriter->Int(tInt(m_Margin.Bottom().Pixels()));  }
                if (!m_Margin.Right().Empty()) { sWriter->Key("f_mr"); sWriter->Int(tInt(m_Margin.Right().Pixels()));  }
            }
            if (!m_Padding.Empty()) {
                if (!m_Padding.All().Empty()) { sWriter->Key("f_p"); sWriter->Int(tInt(m_Padding.All().Pixels()));  }
                if (!m_Padding.Top().Empty()) { sWriter->Key("f_pt"); sWriter->Int(tInt(m_Padding.Top().Pixels()));  }
                if (!m_Padding.Left().Empty()) { sWriter->Key("f_pl"); sWriter->Int(tInt(m_Padding.Left().Pixels()));  }
                if (!m_Padding.Bottom().Empty()) { sWriter->Key("f_pb"); sWriter->Int(tInt(m_Padding.Bottom().Pixels()));  }
                if (!m_Padding.Right().Empty()) { sWriter->Key("f_pr"); sWriter->Int(tInt(m_Padding.Right().Pixels()));  }
            }
        }
        
        // Shadow
        if (!m_Shadow.Empty()) {
                // futur
        }
        // Font
        if (!m_Font.Empty()) {
            if (m_Font.Name()!="") {
                sWriter->Key("f_f_n"); sWriter->String(m_Font.Name().c_str());
            }
            if ((m_Font.Family()!=tFontFamily::notuse) && ((m_Font.Family()!=tFontFamily::none))) {
                sWriter->Key("f_f_f"); sWriter->String(m_Font.FamilyStr().c_str());
            }
            if (!m_Font.Size().Empty()) {
                sWriter->Key("f_f_s"); sWriter->Int(tInt(m_Font.Size().Value()));
            }
            if ((m_Font.Style()!=tFontStyle::notuse) && (m_Font.Style()!=tFontStyle::none)) {
                sWriter->Key("f_st"); sWriter->Int(tInt(m_Font.Style()));
            }
            if ((m_Font.Weight()!=tFontWeight::notuse) && (m_Font.Weight()!=tFontWeight::none)) {
                sWriter->Key("f_we"); sWriter->Int(tInt(m_Font.Weight()));
            }
        }

        // Text ==================================================================
        if (!m_Text.Empty()) {
            if ((m_Text.TextAlign()!= tTextAlign::notuse) && ((m_Text.TextAlign()!= tTextAlign::none))) {
                sWriter->Key("f_ah"); sWriter->Int(tInt(m_Text.TextAlign()));
            }
            if ((m_Text.VerticalTextAlign()!= tVerticalTextAlign::notuse) && (m_Text.VerticalTextAlign()!= tVerticalTextAlign::none)) {
                sWriter->Key("f_av"); sWriter->Int(tInt(m_Text.VerticalTextAlign()));
            }
            if ((m_Text.UnderLine()!=tTextDecorationLine::notuse) && (m_Text.UnderLine()!=tTextDecorationLine::none)) {
                sWriter->Key("f_d_u"); sWriter->Int(1);
            }
            if ((m_Text.OverLine()!=tTextDecorationLine::notuse) && (m_Text.OverLine()!=tTextDecorationLine::none)) {
                sWriter->Key("f_d_o"); sWriter->Int(1);
            }
            if ((m_Text.LineThrough()!=tTextDecorationLine::notuse) && (m_Text.LineThrough()!=tTextDecorationLine::none)) {
                sWriter->Key("f_d_l"); sWriter->Int(1);
            }

            if (m_Text.TextWrap()!=tTextWrap::notuse) {
                sWriter->Key("f_tw"); sWriter->Int(tInt(m_Text.TextWrap()));
            }
            if (m_Text.Rotate()!=0) {
                sWriter->Key("f_tr"); sWriter->Double(m_Text.Rotate());
            }
     
            if (!m_Text.Str().empty()) {
                sWriter->Key("f_t"); sWriter->String(m_Text.Str().c_str());
            }
            
            
    #ifdef _DEBUGSK
            if (!m_Text.FormatString()->Empty()) {
                sWriter->Key("f_for");
                m_Text.FormatString()->Json(sWriter);
            }
    #endif
        }
        // BorderRect
        if (!m_BorderRect.Empty()) {
            if (sCss) {
                m_BorderRect.JsonJavaScript(sWriter);
            } else {
                m_BorderRect.JsonJavaScriptCanvas(sWriter);
            }
        }
    }

    void tFormatMergeCss::JsonJavaScript(Writer<StringBuffer>* sWriter,tVariant* sVariant,tBool sCss) {
        JsonJavaScriptVariantDisplay(sWriter, sVariant, sCss);
        tBool wIsTextAlign=false;
        if (!m_Text.Empty()) {
            if ((m_Text.TextAlign()!= tTextAlign::notuse) && ((m_Text.TextAlign()!= tTextAlign::none))) {
                wIsTextAlign=true;
            }
        }
        JsonJavaScriptStyle(sWriter, sCss);
        // Default left align for plain strings when no explicit align was serialized.
        if (!wIsTextAlign) {
            if (sVariant!=nullptr) {
                if (sVariant->Type()==tVariantType::t_string) {
                    sWriter->Key("f_ah"); sWriter->Int(tInt(tTextAlign::left));
                }
            }
        }
    }

} // end of namespace
