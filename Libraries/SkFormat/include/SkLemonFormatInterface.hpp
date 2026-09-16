//=============================================================================
// SkLemonInterface // CSS interface width Lemon
//=============================================================================
#ifndef SkLemonFormatInterface_hpp
#define SkLemonFormatInterface_hpp
#include <SkTypes.hpp>
#include <SkClass.hpp>
#include <SkVariant.hpp>

#include "SkFormatRoot.hpp"
#include "SkLexerFormat.hpp"
#include "SkLemonFormatReserved.hpp"

#define _DEBUGSKLemon

using namespace SkRoot;

namespace SkFormat {
	
	//=========================================================================
	//! Class interface with Lemon. Used to compile a cell
	class tLemonFormatInterface : tClass {
	private:
        //! Format Root
        tFormatRoot*            m_FormatRoot;
        
        //! Format Css
        tFormatCss*             m_FormatCss;
        
        tStackKind              m_StackKind;
		//! Reserved word.
		tLemonFormatReserved    m_LemonReserved;
		
		//! For init done.
		void*				    m_LemonParser;

        //! String Compil
        tString                 m_Code;
        
		//! True if error.
		tBool                   m_CompilError;
     
		//! Explanation of the error.
		tString                 m_Error;
        tInt                    m_ErrorLine;
        tInt                    m_ErrorColumn;
        
        // Element ============================================================
        tUnitCss            m_Width;
        tUnitCss            m_Height;
        
        tUnitRectCss        m_Padding;
        tBool               m_IsPadding;
        
        tUnitRectCss        m_Margin;
        tBool               m_IsMargin;

        tFontCss            m_FontCss;
        tBool               m_IsFont;
        
        tBorderRectCss      m_BorderRectCss;
        tBool               m_IsBorder;
        
        tTextCss            m_TextCss;
        tBool               m_IsText;
        
        tVectorUnitCss      m_VectorUnitCss;
        
        // Return Format for cell ========================================================
        tBool               m_IsCellAlloc;
        //! Cell Allocator
        tFormatRef          m_CellAlloc;
        
	public:
		/// @brief		Constructor SkLemonFunction.
		tLemonFormatInterface();

		/// @brief		Clear m_Current formula and m_StackFunction.
		void Clear();

		/// @brief		Call function with arguments.
		/// @param[in]  sCode const tChar* Code
		/// @return		tBool false if error
		tBool Compil(const tChar* sCode);
        
        /// @brief      Return Cell Alloc.
        /// @return     tFormatRef
        tFormatRef CellAlloc();

		// Reference Identifier ================================================
		/// @brief		Add Identifier (Reserved word).
		/// @param[in]  sName tString 
		/// @param[in]  sId tInt Id of reserved word
		/// @param[in]	sClass SkVirtualClass* Class for variant
		/// @param[in]	sKind tKind Kind of IdReference
		/// @return		tBool false if name already exist
		tBool AddIdRef(tString sName, tInt sId, tVirtualClass* sClass, tKind sKind);


		/// @brief		Return id of reserved word by name.
		/// @param[in]  sName tString
		/// @return		tInt Id
		tInt Id(tString sName);

        /// @brief        Return SkLemonIdRef.
        /// @param[in]  sName tString name of Id
        /// @return        SkLemonIdRef&
        tLemonFormatIdRef& IdRef(tString sName);

        /// / @brief        Return Color.
        /// @param[in]  sName tString name of Id
        /// @return        tRecColor*
        const tRecColor* Color(tString sName);
        
		// Interface formula stack ==================================================
        /// @brief        Begin Css.
        /// @param[in]  sName  tLexerToken*
        /// @param[in]  sSpeudo  tLexerToken*
        void BeginCss(tLexerToken* sName,tLexerToken* sSpeudo);
        
        /// @brief        End Css.
        void EndCss();
    

        /// @brief       Begin Kind  (Font Border..)
        /// /// @param[in]  sToken tLexerToken*
        void BeginKind(tLexerToken* sToken);
        
        /// @brief        End Kind.  Pop()
        void EndKind();

    
        /// @brief        ColorString balck white. red....
        /// @param[in]  sToken tLexerToken*
        void ColorStr(tLexerToken* sToken);

        /// @brief        ColorHash #FFEEFF..
        /// @param[in]  sToken tLexerToken*
        void ColorHash(tLexerToken* sToken);

        /// @brief        Color Rgb.(R,G,B).
        /// @param[in]  sR tSkLexerToken*
        /// @param[in]  sG tSkLexerToken*
        /// @param[in]  sB  tSkLexerToken*
        void ColorRgb(tLexerToken* sR,tLexerToken* sG,tLexerToken* sB);

        /// @brief      Color Rgb.(R,G,B),A.
        /// @param[in]  sR tSkLexerToken*
        /// @param[in]  sG tSkLexerToken*
        /// @param[in]  sB  tSkLexerToken*
        /// @param[in]  sA  tSkLexerToken*
        void ColorRgba(tLexerToken* sR,tLexerToken* sG,tLexerToken* sB,tLexerToken* sA);

        /// @brief      Opacity.
        /// @param[in]  sToken tLexerToken*
        void Opacity(tLexerToken* sToken);
        

        /// @brief      FontFamily..
        /// @param[in]  sName tLexerToken*
        void FontName(tLexerToken* sName);

        /// @brief      FontFamily..
        /// @param[in]  sFamily tLexerToken*
        void FontFamily(tLexerToken* sFamily);
   
        /// @brief      FontStyle..
        /// @param[in]  sToken tLexerToken*
        void FontStyle(tLexerToken* sToken);

        /// @brief      FontWeight..
        /// @param[in]  sToken tLexerToken*
        void FontWeight(tLexerToken* sToken);

        /// @brief      FontVariant..
        /// @param[in]  sToken tLexerToken*
        void FontVariant(tLexerToken* sToken);

        /// @brief      FontStretch.
        /// @param[in]  sType  tLexerToken*`
        /// @param[in]  sPercent  tLexerToken*`
        void FontStretch(tLexerToken* sToken);

        /// @brief      FontLineHeight.
        /// @param[in]  sToken tLexerToken*
        void FontLineHeight(tLexerToken* sToken);

        /// @brief        TextAlign..
        /// @param[in]  sToken tLexerToken*
        void TextAlign(tLexerToken* sToken);
        
        /// @brief      VerticaltAlign..
        /// @param[in]  sToken tLexerToken*
        void VerticalAlign(tLexerToken* sToken);
        
        /// @brief      TextDecoration
        /// @param[in]  sToken tLexerToken*
        void TextDecoration(tLexerToken* sToken);

        /// @brief      TextWrap..
        /// @param[in]  sToken tLexerToken*
        void TextWrap(tLexerToken* sToken);
        
        ///@brief Text Rotatio, degrees
        ///param[in] sToken tLexerToken*
        void TextRotate(tLexerToken* sToken);
        
        /// @brief      Border Style
        /// @param[in]  sToken tLexerToken*
        void BorderStyle(tLexerToken* sToken);
        
        /// @brief        Border Style
        /// @param[in]  sUnit tKind
        /// @param[in]  sToken tLexerToken*
        void PushUnit(tKind sUnit,tLexerToken* sToken);

        /// @brief      Format String
        /// @param[in]  sToken tLexerToken*
        /// @param[in]  sPrecision tLexerToken* 
        void FormatString(tLexerToken* sToken,tLexerToken*  sPrecision=nullptr);
        
        void FormatStringMoney(tLexerToken* sToken,tLexerToken* sMoney,tLexerToken* sPrecision=nullptr);
        
		/// @brief		Return m_LemonParser for init done. 
		/// @return		SkLemonFunction
		void* LemonParser();

		/// @brief		Set m_LemonParser for init done. 
		/// @param[in]	sLemonParser void*
		void LemonParser(void* sLemonParser);

		/// @brief		Set compil error true or false.
		/// @param[in]	sValue tBool
		void CompilError(tBool sValue);

		/// @brief		return compil error. 
		/// @param[in]	tBool
		tBool CompilError();

		/// @brief		Set explanation of the error.
        /// @param[in]  sRow tInt
        /// @param[in]  sCol  tInt
		/// @param[in]	sLexerError tString
		void Error(tString sLexerError,tInt sRow,tInt sCol);

        
		/// @brief		Returnf  error.
		/// @return		tString
		tString Error();
        
        /// @brief      Return error Column
        /// @return     tInt
        tInt ErrorColumn();

        /// @brief      Return error Line
        /// @return     tInt
        tInt ErrorLine();

        /// @brief        Return explanation of the error.
        /// @return       tString
        tString ErrorWithDetail();

		/// @brief		For debug on console.
		void Debug();
	};
}
#endif
