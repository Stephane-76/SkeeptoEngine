//=============================================================================
// SkText (Text for Css )
//=============================================================================
#ifndef SkText_hpp
#define SkText_hpp

#include "SkFormatShare.hpp"
#include "SkColor.hpp"
#include "SkUnit.hpp"

using namespace SkRoot;

namespace SkFormat {
    // Not use for merge none raz element
	enum class tTextAlign : tByte {
		notuse,
        none,
		left,
		right,
		center,
		justify,
		Count
	};
 

	enum class tVerticalTextAlign : tByte {
        notuse =0,
		none,
		baseline,
		sub,
		super,
		text_top,
		text_bottom,
		middle,
		top,
		bottom,
		Count
	};

    enum class tTextDecorationLine : tByte {
        notuse=0,
        none,
        use,
        Count
    };

	enum class tTextWrap : tByte {
        notuse=0,
        none,
		wrap,
		nowrap,
		Count
	};

	// SkText =================================================================
	class  alignas(SkAlign) tTextCss : public tFormatShare {
	private:
		// Text align
		tTextAlign		    m_TextAlign;
		tVerticalTextAlign  m_VerticalTextAlign;
        
		// Decoration line
        tTextDecorationLine m_UnderLine;
        tTextDecorationLine m_OverLine;
        tTextDecorationLine m_LineThrough;

		// Text wrap
		tTextWrap           m_TextWrap;

		// Rotation
        tFloat             m_Rotate;

        // Format string
        tFormatString       m_FormatString;
    public:
		/// @brief		Constructor
		tTextCss();

		/// @brief		Copy constructor 
		/// @param[in]	sText SkText&
		tTextCss(const tTextCss& sText);

		/// @brief		Clear SkAllocator
		void Clear();

		/// @brief      Get key of element
		tString Key() override;

		/// @brief      Get String Css
        /// @param[in]  sReturn tBool
		/// @return		tString;
		tString Str(tBool sReturn=false);
        
        /// @brief      Merge
        /// @param[in]  sText  tTexttCss*
        void  Merge(tTextCss*  sText);

		/// @brief		Set Text alignment 
		/// @param[in]	sTextAlign  tTextAlign 
		void TextAlign(tTextAlign sTextAlign);

		/// @brief		Get Text alignment 
		/// @return		tTextAlign 
		tTextAlign TextAlign();

		/// @brief		Get Text alignment str
		/// @return		tString
		tString TextAlignStr();

		/// @brief		Set Vertical text alignment 
		/// @param[in]	sVerticalTextAlign  tVerticalTextAlign 
		void VerticalTextAlign(tVerticalTextAlign sVerticalTextAlign);

		/// @brief		Get Vertical text alignment 
		/// @return		tVerticalTextAlign 
		tVerticalTextAlign VerticalTextAlign();

		/// @brief		Get Text alignment str
		/// @return		tString
		tString VerticalTextAlignStr();

        /// @brief        Set UnderLine
        /// @param[in]    sDecorationLine tTextDecorationLine
        void UnderLine(tTextDecorationLine sUnderLine);

        /// @brief        Get UnderLine
        /// @return       tTextDecorationLine
        tTextDecorationLine UnderLine();

        /// @brief        Set Overline
        /// @param[in]    sDecorationLine tTextDecorationLine
        void OverLine(tTextDecorationLine sOverLine);

        /// @brief        Get OverLine
        /// @return       tTextDecorationLine
        tTextDecorationLine OverLine();

        /// @brief        Set LineThrough
        /// @param[in]    sDecorationLine tTextDecorationLine
        void LineThrough(tTextDecorationLine sLineThrough);

        /// @brief        Get  LineThrough
        /// @return       tTextDecorationLine
        tTextDecorationLine LineThrough();

        /// @brief        Get TextWrapStr
        /// @return       tString
        tString TextWrapStr();

        // Text wrap
        /// @brief        Set TextWrap
        /// @param[in]    sTextWrap  tTextWrap
        void TextWrap(tTextWrap sTextWrap);

		/// @brief        Get TextWrap
		/// @return       tTextWrap
		tTextWrap TextWrap();

		/// @brief        Set Rotation
		/// @param[in]    sRotation  tDouble
		void Rotate(tDouble sRotation);

		/// @brief        Get Rotation
		/// @return       tDouble
        tFloat Rotate();

        /// @brief        Set FormatString
        /// @param[in]    sFormatString  tString
        void FormatString(tString sFormatString);

		/// @brief		Get format string
		/// @return		tString
		tString FormatStringStr();
        
        /// @brief        Get Class  format string
        /// @return       tFormatString
        tFormatString* FormatString();
        
        /// @brief        Get format string type
        /// @return       tFormatString
        tFormatStringType _FormatString();

        /// @brief      Set Money
        /// @param[in] sMoney  t_UnitMoney
        void Money(t_UnitMoney sUnitMoney);
        
        /// @brief      Get Money
        /// @return      t_UnitMoney
        t_UnitMoney Money();
        
        /// @brief        Set format string
        /// @param[in]    sFormatString  tString
        void Precision(tByte sPrecision);

        /// @brief        Get format string
        /// @return        tTextAlign
        tByte Precision();

        /// @brief        Set CustomFormatString
        /// @param[in]    sFormatString  tString
        void ExcelFormatString(tString sFormatString);

        /// @brief        Get format string
        /// @return        tTextAlign
        tString ExcelFormatString();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);

        /// @brief      get if text is  empty
        /// @return     tBool return true
        tBool Empty();
        
		/// @brief		operator ==
		/// @param[in]	sText SkText&
		/// @return		tBool
		tBool operator == (tTextCss& sText);
	};

}

#endif
