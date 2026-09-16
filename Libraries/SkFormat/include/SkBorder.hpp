//=============================================================================
// SkBorder (Border for Css )
//=============================================================================
#ifndef SkBorder_hpp
#define SkBorder_hpp

#include "SkFormatShare.hpp"
#include "SkColor.hpp"
#include "SkUnitMetrics.hpp"

using namespace SkRoot;

namespace SkFormat {

	enum class tBorderStyle : tByte {
		none = 0,      
		hidden,        
		dotted,        
		dashed,         
		solid,   
		_double,   
		groove,
		ridge,
		inset,
		outset,
		Count
	};

	class  alignas(SkAlign) tBorderCss : public tClass {
	protected:
		tColorCss		 m_Color;
		tUnitCss		 m_Width;
		tBorderStyle	 m_BorderStyle;
	public:
		/// @brief		Constructor
		tBorderCss();

		/// @brief		Copy constructor 
		/// @param[in]	sBorder SkBorder&
		tBorderCss(const tBorderCss& sBorder);

		/// @brief		Clear (callback SkAllocator)
		void Clear();

        /// @brief      Get String Css
        /// @brief      sName tString
        /// @return     tString;
        tString Str(tString sName="");


		/// @brief		Set color
		/// @param[in]	sColor SkColor
		void Color(tColorCss sColor);

		/// @brief		Get ref of color 
		/// @return		&SkColor
		tColorCss& Color();

		/// @brief		Set width
		/// @param[in]	sWidth SkUnit
		void Width(tUnitCss sWidth);

		/// @brief		Get width 
		/// @return		SkUnit
		tUnitCss Width();

		/// @brief		Set border style
		/// @param[in]	sBorderStyle SkBorderStyle
		void BorderStyle(tBorderStyle sBorderStyle);
		
		/// @brief		Get border style 
		/// @return		tBorderStyle
		tBorderStyle BorderStyle();

		/// @brief		Get border style string
		/// @return		tBorderStyle
		tString BorderStyleStr();

		/// @brief      get is none border 
		/// @return     tBool return true if BorderStyle = none
		tBool Empty();

		/// @brief      Merge
		/// @param[in]	sBorderCss SkBorderCss
		void  Merge(const tBorderCss sBorderCss);

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);
        
        /// @brief        Writer Json for react canvas.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonJavaScriptCanvas(Writer<StringBuffer>* sWriter);
        
        
		/// @brief		operator ==
		/// @param[in]	sBordert SkBorder&
		/// @return		tBool
		tBool operator == (tBorderCss& sBorder);
        
        /// brief        copy assignment
        /// @param[in]   sBorderCss tBorderCss
        //tBorderCss operator=(const tBorderCss& sBorderCss);
        
	};

	class  alignas(SkAlign) tBorderRectCss : public tFormatShare {
	private:
		tBorderCss  m_All;
		tBorderCss  m_Left;
		tBorderCss  m_Top;
		tBorderCss  m_Right;
		tBorderCss  m_Bottom;

		tUnitCss    m_Radius;
		tUnitCss    m_TopLeftRadius;
		tUnitCss	m_TopRightRadius;
		tUnitCss	m_BottomLeftRadius;
		tUnitCss	m_BottomRightRadius;
	public:
		/// @brief		Constructor
		tBorderRectCss();
		
		/// @brief		Copy constructor 
		/// @param[in]	sBorder SkBorder&
		tBorderRectCss(const tBorderRectCss& sBorderRect);

		/// @brief      Clear for SkAllocator
		void Clear();

		/// @brief      Get key of element
		tString Key() override;

        
		/// @brief      Get String Css
        /// @param[in]  sReturntBool
        /// @return		tString;
		tString Str(tBool sReturn=false);
        
        /// @brief      Merge
        /// @param[in]  sBorder  tBorderRecCss*
        void  Merge(tBorderRectCss*  sBorder);

		/// @brief		get ref of border 
		/// @return		SkBorder&
		tBorderCss& All();

		/// @brief		get ref of border left
		/// @return		SkBorder&
		tBorderCss& Left();

		/// @brief		get ref of border top
		/// @return		SkBorder&
		tBorderCss& Top();

		/// @brief		get ref of border right
		/// @return		SkBorder&
		tBorderCss& Right();

		/// @brief		get ref of border bottom
		/// @return		SkBorder&
		tBorderCss& Bottom();

		/// @brief		get ref of Radius
		/// @return		SkUnit&
		tUnitCss& Radius();

		/// @brief		get ref of top left radius
		/// @return		SkUnit&
		tUnitCss& TopLeftRadius();

		/// @brief		get ref of top right radius
		/// @return		SkUnit&
		tUnitCss& TopRightRadius();

		/// @brief		get ref of bottom right radius
		/// @return		SkUnit&
		tUnitCss& BottomRightRadius();

		/// @brief		get ref of bottom left radius
		/// @return		SkUnit&
		tUnitCss& BottomLeftRadius();

		/// @brief      get is BorderRect empty
		/// @return     tBool return true if all element is empty
		tBool Empty();
        
        // Multiple ===========================================================
         /* 1 Apply to all four sides */
         /* 2 top and bottom | left and right */
         /* 3 top | left and right | bottom */
         /* 4 top | right | bottom | left */
        
        /// @brief        Apply unit and border.
        /// @param[in]    sVector tVectorUnitCss*
        void ApplyUnit(tVectorUnitCss* sVector);

        /// @brief        Apply unit and Radius
        /// @param[in]    sVector tVectorUnitCss*
        void ApplyRadiusUnit(tVectorUnitCss* sVector);

        // Spreadsheet interface ==============================================
        
        /// @brief     delete Border
        /// @param[in] sBorderMask  tShort
        /// @return tBool
        tBool DeleteBorder(tShort sBorderMask);
        
        /// @brief     Return Border mask
        /// @return  tByte
        tShort BorderMask();
        
		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);
        
        /// @brief        Writer Json for react.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonJavaScript(Writer<StringBuffer>* sWriter);

        /// @brief        Writer Json for react canvas.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonJavaScriptCanvas(Writer<StringBuffer>* sWriter);
      
	};


}

#endif
