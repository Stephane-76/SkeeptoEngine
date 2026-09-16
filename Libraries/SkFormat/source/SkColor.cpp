//=============================================================================
// SkColor  (Color for css Css )
//=============================================================================

#include "../include/SkColor.hpp"
#include "../include/SkFormatRoot.hpp"
namespace SkFormat {
	// Color ====================================================
	tColorCss::tColorCss() : tClass(), m_Opacity(1), m_Color(), m_NotUse(true) {};

	tColorCss::tColorCss(tColor sColor) : tClass(), m_Opacity(1), m_Color(), m_NotUse(false) {}

	tColorCss::tColorCss(const tColorCss& sColor) : tClass(sColor), m_Opacity(sColor.m_Opacity), m_Color(sColor.m_Color), m_NotUse(sColor.m_NotUse) {}

	tString tColorCss::Key(){
		tStringStream wStream;
		wStream << m_Color;
		if (m_Opacity!=1) wStream << "," << m_Opacity;
		return(wStream.str());
	};

	tString tColorCss::Str(tBool sReturn) {
		tStringStream wStream;
		wStream << StrKey();
		if (m_Opacity != 1) {
            wStream << ";";
            if (sReturn) wStream << endl;
            wStream << "opacity:" << m_Opacity;
		}
		return(wStream.str());
	}

	void tColorCss::Opacity(tFloat sOpacity) { m_Opacity = sOpacity; }

	tFloat tColorCss::Opacity() { return(m_Opacity); }
        
	void  tColorCss::NotUse(tBool sNotUse) { m_NotUse = sNotUse; }

	tBool tColorCss::NotUse() { return(m_NotUse); }

	void tColorCss::ColorRgb(tUByte sRed, tUByte sGreen, tUByte sBlue) {
		m_NotUse = false;
		m_Color=((sRed & 0xff) << 16) + ((sGreen & 0xff) << 8) + (sBlue & 0xff);
	}

	tuple <tUByte, tUByte, tUByte> tColorCss::ColorRgb() {
		tUByte wRed, wGreen, wBlue;
		wRed = (m_Color & 0xff0000) >> 16;
		wGreen = (m_Color & 0x00ff00) >> 8;
		wBlue = (m_Color & 0x0000ff);
		return(make_tuple(wRed, wGreen, wBlue));
	}

    tUByte tColorCss::Red() { return((m_Color & 0xff0000) >> 16); }
    tUByte tColorCss::Green() { return((m_Color & 0x00ff00) >> 8); }
    tUByte tColorCss::Blue() { return(m_Color & 0x0000ff); }

	void tColorCss::Color(tColor sColor) { m_NotUse = false; m_Color = sColor; }
	tColor tColorCss::Color() { return(m_Color); }


	void tColorCss::ColorHex(tString sValue) {
		m_NotUse = false;
		if (sValue.length() == 8) {
			m_Opacity= tFloat(std::stoul(sValue.substr(0, 2), nullptr, 16)) / 255.0f;
			m_Color= t_Color(std::stoul(sValue.substr(2, 6), nullptr, 16));
		} else {
			m_Opacity= 1;
			m_Color= t_Color(std::stoul(sValue, nullptr, 16));
		}
	}

	tString tColorCss::ColorHex() {
		tStringStream wStream;
		wStream << "#";
		wStream << std::setfill('0') << std::setw(6);
		wStream << std::uppercase << std::hex << (m_Color & 0xFFFFFF);
		return(wStream.str());
	}

	void tColorCss::ColorName(tString sName) {
        tClassString wLowerName=tClassString(sName);
        tString wName=wLowerName.Lower();
        tFormatRoot* wFormatRoot=tFormatRoot::Instance();
        
        const tRecColor* wRecColor=wFormatRoot->FindColorByName(wName);
        if (wRecColor!=nullptr) {
            m_NotUse = false;
            m_Color = wRecColor->m_Color;
            return;
        }
    
        m_Color = 0; // black;
	}

	tString tColorCss::StrKey() {
        tFormatRoot* wFormatRoot=tFormatRoot::Instance();
        
        const tRecColor* wRecColor=wFormatRoot->FindColorByColor(m_Color);
        if (wRecColor!=nullptr) {
            return(wRecColor->m_Key);
        }
		return(ColorHex());
	}

	void tColorCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("o");
		sWriter->Double(m_Opacity);
		sWriter->Key("c");
		sWriter->Uint(m_Color);
		sWriter->EndObject();
	}

	/// @brief		Reader Json. 
	/// @param[in]	sValue Value&
	void tColorCss::Json(const rapidjson::Value& sValue) {
		m_Opacity= static_cast<float>(sValue["o"].GetDouble());
		m_Color = sValue["c"].GetUint();
		m_NotUse = false;
	}

	tBool tColorCss::operator == (tColorCss& sColor) {
		return((m_Color == sColor.m_Color) && (m_Opacity== sColor.m_Opacity) && (m_NotUse==sColor.m_NotUse));
	}

	// Color Utis =============================================================
	tUByte GetR(tColor color) {	return (tUByte)((color & 0xFF000000) >> 8); }

	tUByte GetG(tColor color) { return (tUByte)((color & 0xFF000000) >> 16); }

	tUByte GetB(tColor color) { return (tUByte)((color & 0xFF000000) >> 24); }

	void RGB2HSL(tColor sColor, tFloat& sHue, tFloat& sSaturation, tFloat& sLuminance) {
		tFloat R; tFloat G; tFloat B; tFloat D; tFloat Cmax; tFloat Cmin;

		R = static_cast<tFloat>(GetR(sColor) * 1.0 / 255);
		G = static_cast<tFloat>(GetG(sColor) * 1.0 / 255);
		B = static_cast<tFloat>(GetB(sColor) * 1.0 / 255);
		Cmax = max(R, max(G, B));
		Cmin = min(R, min(G, B));
		sLuminance = tFloat((Cmax + Cmin) * 1.0 / 2);

		if (Cmax == Cmin)
		{
			sHue = 0;
			sSaturation = 0;
		}
		else
		{
			D = Cmax - Cmin;
			if (sLuminance < 0.5) { sSaturation = tFloat(D * 1.0 / (Cmax + Cmin)); }
			else { sSaturation = tFloat(D * 1.0 / tFloat(2 - Cmax - Cmin)); }

			if (R == Cmax) { sHue = tFloat((G - B) * 1.0 / D); }
			else
				if (G == Cmax) { sHue = tFloat(2 + (B - R) * 1.0 / D); }
				else { sHue = tFloat(4 + (R - G) * 1.0 / D); }
			sHue = tFloat(sHue * 1.0 / 6);
			if (sHue < 0) sHue = sHue + 1;
		}
	}

	tUByte HueToColourValue(tFloat sHue, tFloat s1, tFloat s2) {
		tUByte wResult;
		tFloat wByreRGB;

		sHue = sHue - std::floor(sHue);

		if (6 * sHue < 1) { wByreRGB = s1 + (s2 - s1) * sHue * 6; }
		else
			if (2 * sHue < 1) { wByreRGB = s2; }
			else
				if (3 * sHue < 2) { wByreRGB = s1 + (s2 - s1) * ((tFloat)(2 * 1.0 / 3 - sHue) * 6); }
				else { wByreRGB = s1; }
		wResult = tUByte(255 * wByreRGB);
		return wResult;
	}

#ifdef __EMSCRIPTEN__
#define RGB(r,g,b)          ((tColor)(((tByte)(r)|((tInt)((tByte)(g))<<8))|(((tLong)(tByte)(b))<<16)))
#endif


	tColor HSL2RGB(const tFloat& sHue, const tFloat& sSaturation, const tFloat& sLuminance) {
		tColor wResult;
		tFloat w1; tFloat w2;
		tUByte wRed; tUByte wGreen; tUByte wBlue;

		// Level of Gray if sturation =0 
		if (sSaturation == 0) {
			wRed = tUByte(255 * sLuminance);
			wGreen = wRed;
			wBlue = wRed;
		} else {
			// Normal Method (Historic Nat System)
			if (sLuminance <= 0.5) {
				w2 = sLuminance * (1 + sSaturation);
			} else {
				w2 = sLuminance + sSaturation - sLuminance * sSaturation;
			}
			w1 = 2 * sLuminance - w2;
			wBlue = HueToColourValue(sHue - (tFloat)(1 * 1.0 / 3), w1, w2);
			wGreen = HueToColourValue(sHue, w1, w2);
			wRed = HueToColourValue(sHue + (tFloat)(1 * 1.0 / 3), w1, w2);
		}
		// Result with Alpha
		wResult = 0xFF | (wRed << 8) | (wGreen << 16) | (wBlue << 24);
		return(wResult);
	}

	tColor BrightenColor(tColor sColor, tFloat sAmount) {
		tFloat wHue;
		tFloat wStaturation;
		tFloat wLuminance;

		RGB2HSL(sColor, wHue, wStaturation, wLuminance);
		wLuminance += sAmount;
		if (wLuminance > 100) {	wLuminance = 100; }
		return HSL2RGB(wHue, wStaturation, wLuminance);
	}


	tColor DarkenColor(tColor sColor, tFloat sAmount) {
		tFloat wHue;
		tFloat wStaturation;
		tFloat wLuminance;

		RGB2HSL(sColor, wHue, wStaturation, wLuminance);
		if (sAmount >= wLuminance)	{	wLuminance = 0;	} else { wLuminance -= sAmount;	}
		return HSL2RGB(wHue, wStaturation, wLuminance);
	}


}// End of namespace SkFormat
