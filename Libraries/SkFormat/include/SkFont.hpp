//=============================================================================
// SkFont (Font for Css )
//=============================================================================
#ifndef SkFont_hpp
#define SkFont_hpp

#include "SkFormatShare.hpp"
#include "SkColor.hpp"
#include "SkUnitMetrics.hpp"

using namespace SkRoot;

namespace SkFormat {
	// Font family ============================================================
	enum class tFontFamily : tByte {
        notuse = 0,
		none,
		serif,
		sans_serif,
		monospace,
		cursive,
		fantasy,
		system_ui,
		emoji,
		math,
		fangsong,
		Count
	};
  
	// Font Style =============================================================
	enum class tFontStyle : tByte {
        notuse = 0,
        none,
		normal,
		italic,
		oblique,
		deg, // Oblique Percentage
		Count
	};

    // Font Weight ============================================================
    enum class tFontWeight : tByte {
        notuse = 0,
        none,
        normal,
        bold,
        lighter,
        bolder,
        integer, /* 1..1000 */
        Count
    };

	// Font Variant ===========================================================
	enum class tFontVariant : tByte {
        notuse = 0,
        none,
		normal,
		small_caps,
		Count
	};
   
// font-stretch ===========================================================
	enum class tFontStretch : tByte {
        notuse = 0,
        none,
		ultra_condensed,
		extra_condensed,
		condensed,
		semi_condensed,
		normal,
		semi_expanded,
		expanded,
		extra_expanded,
		ultra_expanded,
		percent,
		Count
	};

    // Font ===================================================================
	class  alignas(SkAlign) tFontCss : public tFormatShare {
	private:
		tSharedString  m_Name;
		tFontFamily	   m_Family;

		tUnitCss	   m_Size;

		tFontStyle	   m_Style;
		tFloat		   m_ObliqueDegrees;
		tFontVariant   m_Variant;

		tFontWeight    m_Weight;
        tInt           m_WeightInt;
		tFontStretch   m_Stretch;
		tFloat 		   m_StretchPercent;

		tUnitCss	   m_LineHeight;
	public:
		/// @brief		Constructor
		tFontCss();

		/// @brief		Copy constructor 
		/// @param[in]	sFont SkFont&
		tFontCss(const tFontCss& sFont);

		/// @brief		Clear (callback SkAllocator)
		void Clear();

		/// @brief      Get key of element
		tString Key() override;

		/// @brief      Get String Css
        /// @param[in]  sReturn tBool
		/// @return		tString;
		tString Str(tBool sReturn=false);
        
        /// @brief      Merge
        /// @param[in]  sFont  FontCss*
        void  Merge(tFontCss*  sFont);

		/// @brief		Set Name
		/// @param[in]	sName tString
		void Name(tString sName);

		/// @brief		Get Name
		/// @return		tString
		tString Name();

		/// @brief		Set family
		/// @param[in]	sFontFamily tFontFamily
		void Family(tFontFamily sFontFamily);

		/// @brief		Get family
		/// @return		tFontFamily
		tFontFamily Family();

		/// @brief		Get family string
		/// @return		tString
		tString FamilyStr();

		/// @brief		Set Size 
		/// @param[in]	sSize SkUnit
		void Size(tUnitCss sSize);

		/// @brief		Get size
		/// @return		SkUnit
		tUnitCss Size();

		/// @brief		Set Style
		/// @param[in]	sFontStyle tFontStyle
		void Style(tFontStyle sFontStyle);

		/// @brief		Get Style
		/// @return		tFontStyle
		tFontStyle Style();

		/// @brief		Get style string
		/// @return		tString
		tString StyleStr();

		/// @brief		Set Oblique degrees
		/// @param[in]	sValue tInt
		void ObliqueDegrees(tFloat sValue);

		/// @brief		Get Oblique pourcentage
		/// @return		tFloat
		tFloat  ObliqueDegrees();

		/// @brief		Set Variant
		/// @param[in]	sVariant tFontVariant
		void Variant(tFontVariant sVariant);

		/// @brief		Get Variant
		/// @return		tFontVariant
		tFontVariant Variant();

		/// @brief		Get Variant string
		/// @return		tString
		tString VariantStr();

		/// @brief		Set Weight
        /// @param[in]  sWeight tFontWeight
		/// @param[in]	sWeightInt  tInt
		void Weight(tFontWeight sWeight, tInt sWeightInt);

        /// @brief        Get Weight t
        /// @return        tInt
        tFontWeight Weight();

        /// @brief		Get Weight Value
		/// @return		tInt
		tInt  WeightInt();

        /// @brief        Get Weight
        /// @return        tString
        tString WeightStr();

		/// @brief		Set Stretch
		/// @param[in]	sStretch tFontStretch
		void Stretch(tFontStretch sStretch);

		/// @brief		Get Stretch
		/// @return		tFontStretch
		tFontStretch Stretch();

		/// @brief		Get style Stretch
		/// @return		tString
		tString StretchStr();

		/// @brief		Set Stretch percent
		/// @param[in]	sValue tFloat
		void StretchPercent(tFloat sValue);

		/// @brief		Get Stretch pourcentage
		/// @return		tFloat
		tFloat  StretchPercent();

		/// @brief		Set LineHeight 
		/// @param[in]	sLineHeight SkUnit
		void LineHeight(tUnitCss sLineHeight);

		/// @brief		Get LineHeight
		/// @return		SkUnit
		tUnitCss LineHeight();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);
        
        /// @brief      get if font  is  empty
        /// @return     tBool return true
        tBool Empty();

		/// @brief		operator == 
		/// @param[in]	sFont SkFont&
		/// @return		tBool
		tBool operator == (tFontCss& sFont);
	};

} // end of namespace 

#endif // end of SkFont_hpp
