//=============================================================================
// SkText (Text for Css )
//=============================================================================
#include "../include/SkShadow.hpp"

using namespace SkRoot;

namespace SkFormat {

	// SkText =================================================================
	tShadowCss::tShadowCss() : tFormatShare(),
		m_LightPosX(0),
		m_LightPosY(0),
		m_LightPosZ(0),
		m_ZPlaneParam(0),
		m_LightWidth(0),
		m_SpotAlpha(0),
		m_AmbiantAlpha(0),
		m_Color() {}


/*
 m_LightWidth(600),
 m_SpotAlpha(0.25f),
 m_AmbiantAlpha(0.03f),
 */

	tShadowCss::tShadowCss(const tShadowCss& sShadow) : tFormatShare(sShadow),
		m_LightPosX(sShadow.m_LightPosX),
		m_LightPosY(sShadow.m_LightPosY),
		m_LightPosZ(sShadow.m_LightPosZ),
		m_ZPlaneParam(sShadow.m_ZPlaneParam),
		m_LightWidth(sShadow.m_LightWidth),
		m_SpotAlpha(sShadow.m_SpotAlpha),
		m_AmbiantAlpha(sShadow.m_AmbiantAlpha),
		m_Color(sShadow.m_Color) {}

	/// @brief		Clear SkAllocator
	void tShadowCss::Clear() {
		tFormatShare::Clear();
		m_LightPosX = 0;
		m_LightPosY = 0;
		m_LightPosZ = 0;
		m_ZPlaneParam = 0;
		m_LightWidth = 0; 
		m_SpotAlpha = 0;
		m_AmbiantAlpha = 0;
		m_Color.NotUse(true);
	}

	tString tShadowCss::Key() {
		tStringStream wStream;
		wStream << "[" << m_LightPosX;
		wStream << ":" << m_LightPosY;
		wStream << ":" << m_LightPosZ;
		wStream << ":" << m_ZPlaneParam;
		wStream << ":" << m_LightWidth;
		wStream << ":" << m_SpotAlpha;
		wStream << ":" << m_AmbiantAlpha;
		wStream << ":" << m_Color.Key();
		wStream << "]";
		return(wStream.str());
	}

	tString tShadowCss::Str() {
		// Not for Html Css
		return("");
	}


	void tShadowCss::PosLight(tFloat sX, tFloat sY, tFloat sZ) {
		m_LightPosX = sX;
		m_LightPosY = sY;
		m_LightPosZ = sZ;
	}

	tFloat tShadowCss::PosLightX() { return(m_LightPosX); }
	tFloat tShadowCss::PosLightY() { return(m_LightPosY); }
	tFloat tShadowCss::PosLightZ() { return(m_LightPosZ); }

	void tShadowCss::ZPlaneParam(tFloat sZPlaneParam) { m_ZPlaneParam = sZPlaneParam; }
	tFloat tShadowCss::ZPlaneParam() { return(m_ZPlaneParam); }

	void tShadowCss::LightWidth(tFloat sLightWidth) { m_LightWidth = sLightWidth; }
	tFloat tShadowCss::LightWidth() { return(m_LightWidth); }

	void tShadowCss::SpotAlpha(tFloat sSpotAlpha) { m_SpotAlpha = sSpotAlpha; }
	tFloat tShadowCss::SpotAlpha() { return(m_SpotAlpha); }

	void tShadowCss::AmbiantAlpha(tFloat sAmbiantAlpha) { m_AmbiantAlpha = sAmbiantAlpha; }
	tFloat tShadowCss::AmbiantAlpha() { return(m_AmbiantAlpha); }


	void tShadowCss::Color(tColorCss sColor) { m_Color = sColor; }
	tColorCss& tShadowCss::Color() { return(m_Color); }


	void tShadowCss::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("plx");
		sWriter->Double(m_LightPosX);
		sWriter->Key("ply");
		sWriter->Double(m_LightPosY);
		sWriter->Key("plz");
		sWriter->Double(m_LightPosZ);
		sWriter->Key("zpl");
		sWriter->Double(m_ZPlaneParam);
		sWriter->Key("liw");
		sWriter->Double(m_LightWidth);
		sWriter->Key("spa");
		sWriter->Double(m_SpotAlpha);
		sWriter->Key("ama");
		sWriter->Double(m_AmbiantAlpha);
		sWriter->Key("col");
		m_Color.Json(sWriter);

		sWriter->EndObject();
	}

	/// @brief		Reader Json. 
	/// @param[in]	sValue Value&
	void tShadowCss::Json(const rapidjson::Value& sValue) {
		m_LightPosX = tFloat(sValue["plx"].GetDouble());
		m_LightPosY = tFloat(sValue["ply"].GetDouble());
		m_LightPosZ = tFloat(sValue["plz"].GetDouble());
		m_ZPlaneParam = tFloat(sValue["zpl"].GetDouble());
		m_LightWidth = tFloat(sValue["liw"].GetDouble());
		m_SpotAlpha = tFloat(sValue["spa"].GetDouble());
		m_AmbiantAlpha = tFloat(sValue["ama"].GetDouble());
		m_Color.Json(sValue["col"]);
	}


    tBool tShadowCss::Empty() {
        return((m_LightPosX == 0) &&
               (m_LightPosY == 0) &&
               (m_LightPosZ == 0) &&
               (m_ZPlaneParam == 0) &&
               (m_LightWidth ==0 ) &&
               (m_SpotAlpha == 0) &&
               (m_AmbiantAlpha == 0) &&
               (m_Color.NotUse()));
    }

	tBool tShadowCss::operator == (tShadowCss& sShadow) {
		return((m_LightPosX == sShadow.m_LightPosX) &&
			(m_LightPosY == sShadow.m_LightPosY) &&
			(m_LightPosZ == sShadow.m_LightPosZ) &&
			(m_ZPlaneParam == sShadow.m_ZPlaneParam) &&
			(m_LightWidth == sShadow.m_LightWidth) &&
			(m_SpotAlpha == sShadow.m_SpotAlpha) &&
			(m_AmbiantAlpha == sShadow.m_AmbiantAlpha) &&
			(m_Color == sShadow.m_Color));
	}

} // end of namespace 

