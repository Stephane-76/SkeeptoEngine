//=============================================================================
// SkText (Text for Css )
//=============================================================================
#ifndef SkShadow_hpp
#define SkShadow_hpp

#include "SkFormatShare.hpp"
#include "SkColor.hpp"
#include "SkUnit.hpp"

using namespace SkRoot;

namespace SkFormat {

	// SkShadow =================================================================
	class  alignas(SkAlign) tShadowCss : public tFormatShare {
	private:
		/// Light Pos  X Y Z
		tFloat	m_LightPosX;
		tFloat	m_LightPosY;
		tFloat	m_LightPosZ;

		tFloat  m_ZPlaneParam;
		// Light Width 
		tFloat	m_LightWidth;

		// Spot Alpha
		tFloat	m_SpotAlpha;

		// Ambient Alpha
		tFloat	m_AmbiantAlpha;

		// Color Shadow
		tColorCss m_Color;
	public:
		/// @brief		Constructor
		tShadowCss();

		/// @brief		Copy constructor 
		/// @param[in]	sShadow SkShadow&
		tShadowCss(const tShadowCss& sShadow);

		/// @brief		Clear SkAllocator
		void Clear();

		/// @brief      Get key of element
		virtual tString Key();

		/// @brief      Get String Css
		/// @return		tString;
		tString Str();

		/// @brief		Set Pos light
		/// @param[in]	sX  tFloat 
		/// @param[in]	sY  tFloat 
		/// @param[in]	sZ  tFloat 
		void PosLight(tFloat sX, tFloat sY, tFloat sZ);

		/// @brief      Get Pos light X
		/// @return		tFloat;
		tFloat PosLightX();

		/// @brief      Get Pos light Y
		/// @return		tFloat;
		tFloat PosLightY();

		/// @brief      Get Pos light Z
		/// @return		tFloat;
		tFloat PosLightZ();

		/// @brief		Set ZPlaneParam
		/// @param[in]	sZPlaneParam tFloat 
		void ZPlaneParam(tFloat sZPlaneParam);

		/// @brief      Get Zplane Param
		/// @return		tFloat;
		tFloat ZPlaneParam();

		/// @brief		Set LightWidth
		/// @param[in]	sLightWidth tFloat 
		void LightWidth(tFloat sLightWidth);

		/// @brief      Get LightWidth
		/// @return		tFloat;
		tFloat LightWidth();

		/// @brief		Set SpotAlpha
		/// @param[in]	sSpotAlpha tFloat 
		void SpotAlpha(tFloat sSpotAlpha);

		/// @brief      Get SpotAlpha
		/// @return		tFloat;
		tFloat SpotAlpha();

		/// @brief		Set AmbiantAlpha
		/// @param[in]	sAmbiantAlpha tFloat 
		void AmbiantAlpha(tFloat sAmbiantAlpha);

		/// @brief      Get AmbiantAlpha
		/// @return		tFloat;
		tFloat AmbiantAlpha();


		/// @brief		Set Color
		/// @param[in]	sColor SkColorCss
		void Color(tColorCss sColor);

		/// @brief      Get Color
		/// @return		SkColorCss;
		tColorCss& Color();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);
        
        /// @brief      get if shadow is  empty
        /// @return     tBool return true
        tBool Empty();

		/// @brief		operator == 
		/// @param[in]	sText SkText&
		/// @return		tBool
		tBool operator == (tShadowCss& sShadow);
	};

}

#endif
