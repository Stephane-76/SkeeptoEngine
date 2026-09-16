//=============================================================================
// SkUnitMetrics  (UnitMetrics for  Css )
//=============================================================================
#ifndef SkUnitMetrics_hpp
#define SkUnitMetrics_hpp

#include <SkMetrics.hpp>
#include "SkFormatShare.hpp"

using namespace rapidjson; 
using namespace SkRoot;

namespace SkFormat {

	// Measure with unit ======================================================
	class tUnitCss : public tClass {
	private:
		tUnitMetrics	m_Unit;
		tFloat		    m_Value;

		/// @brief		Return convert unit 
		/// @param[in]	sValue  tFloat
		/// @param[in]	sFrom	tUnit
		/// @param[in]	sTo		tUnit
		/// @return				tFloat
		tFloat Convert(const tFloat sValue, const tUnitMetrics sFrom, const tUnitMetrics sTot);
		
	public:
		/// @brief		Constructor
		tUnitCss();

		/// @brief		Constructor with value and unit
		/// @param[in]	sValue tFloat
		/// @param[in]	sUnit SkUnit
		tUnitCss(tFloat sValue,tUnitMetrics sUnit);

		/// @brief		Copy constructor 
		/// @param[in]	sUnit SkUnit&
		tUnitCss(const tUnitCss& sUnit);

		/// @brief		set Skunit
		/// @param[in]	sSkUnit SkUnit
		void Unit(tUnitCss sSkUnit);

		/// @brief		set unit
		/// @param[in]	sUnit SkUnit
		void Unit(tUnitMetrics sUnit);

		/// @brief		get unit 
		/// @return		SkUnit
		tUnitMetrics Unit();

		/// @brief		set value 
		/// @param[in]	sValue tFloat
		void Value(tFloat sValue);

		/// @brief		get value 
		/// @return		tFloat
		tFloat Value();

		/// @brief		Return pixels
		/// @return		tFloat
		tFloat Pixels();

		/// @brief		Return inches
		/// @return		tFloat
		tFloat Inches();

		/// @brief		Return points
		/// @return		tFloat
		tFloat Points();

		/// @brief		Return picas
		/// @return		tFloat
		tFloat Picas();

		/// @brief		Return centimeters
		/// @return		tFloat
		tFloat Centimeters();

		/// @brief		Return millimeters
		/// @return		tFloat
		tFloat Millimeters();

		/// @brief		Return % (m_Value)
		/// @return		tFloat
		tFloat Percent();


		/// @brief		Return Pixel with % (if use percent) 
		/// @param[in]	sWidthHeight tFloat
		/// @return		tFloat
		tFloat SizePixels(tFloat sWidthHeight);


		/// @brief		get str value
		/// @return		tString
		tString Str();

		/// @brief      get is none unit
		/// @return     tBool return true if unit = none
		tBool Empty();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);

		/// @brief		operator == 
		/// @param[in]	sUnit SkUnit&
		/// @return		tBool
		tBool operator == (const tUnitCss& sUnit);

		/// @brief		operator != 
		/// @param[in]	sUnit SkUnit&
		/// @return		tBool
		bool operator != (const tUnitCss& sUnit);
	};

	ostream& operator<<(ostream& sStream, tUnitCss sValue);

    typedef vector<tUnitCss> tVectorUnitCss;

	// Measure Rect for padding, margin, border ================================
	class tUnitRectCss : public tFormatShare {
	private:
		tUnitCss    m_All;
		tUnitCss    m_Left;
		tUnitCss    m_Top;
		tUnitCss    m_Right;
		tUnitCss    m_Bottom;
	public:
		/// @brief		Constructor
		tUnitRectCss();

		/// @brief		Copy constructor 
		/// @param[in]	sUnitRect SkUnitRect&
		tUnitRectCss(const tUnitRectCss& sUnitRect);

		/// @brief		Constructor with Unit for all
		/// @param[in]	sAll SkUnit
		tUnitRectCss(const tUnitCss& sAll);

		/// @brief		Constructor with Unit 
		/// @param[in]	sLeft SkUnit
		/// @param[in]	sTop SkUnit
		/// @param[in]	sRight SkUnit
		/// @param[in]	sBottom SkUnitRect&
		tUnitRectCss(const tUnitCss& sLeft, const tUnitCss& sTop, const tUnitCss& sRight, const tUnitCss& sBottom );

		/// @brief		Clear
		void Clear();

		/// @brief      Get key of element
		tString Key() override;

		/// @brief      Get String Css sRoot (padding or margin)
		/// @param[in]	sRoot tString 
		/// @return		tString;
		tString Str(tString sRoot,tBool sReturn=false);
        
        /// @brief      Merge
        /// @param[in]  sMarge tUnitRecCss*
        void  Merge(tUnitRectCss*  sMerge);

		/// @brief		Set unit for all rect 
		/// @param[in]	sUnit SkUnit
		void Rect(tUnitCss sUnit);

		/// @brief		set unit for All 
		/// @param[in]	sUnit SkUnit
		void All(tUnitCss sUnit);

		/// @brief		get unit for All 
		/// @return		SkUnit
		tUnitCss& All();

		/// @brief		set unit for left 
		/// @param[in]	sUnit SkUnit
		void Left(tUnitCss sUnit);
		/// @brief		get unit for left 
		/// @return		SkUnit
		tUnitCss& Left();

		/// @brief		set unit for top 
		/// @param[in]	sUnit SkUnit
		void Top(tUnitCss sUnit);

		/// @brief		get unit for top 
		/// @return		SkUnit
		tUnitCss& Top();

		/// @brief		set unit for right 
		/// @param[in]	sUnit SkUnit
		void Right(tUnitCss sUnit);

		/// @brief		get unit for right 
		/// @return		SkUnit
		tUnitCss& Right();

		/// @brief		set unit for bottom 
		/// @param[in]	sUnit SkUnit
		void Bottom(tUnitCss sUnit);

		/// @brief		get unit for bottom 
		/// @return		SkUnit
		tUnitCss& Bottom();
        
        // Multiple ===========================================================
         /* 1 Apply to all four sides */
         /* 2 top and bottom | left and right */
         /* 3 top | left and right | bottom */
         /* 4 top | right | bottom | left */
        
        /// @brief        Apply unit and border.
        /// @param[in]    sVector tVectorUnitCss*
        void ApplyUnit(tVectorUnitCss* sVector);
        
		// Json ===============================================================
		/// @brief		Writer Json
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);

		/// @brief      get is all unit is none 
		/// @return     tBool return true if all unit = none
		tBool Empty();

	};

}; // End of namespace SkFormat


#endif
