//=============================================================================
// SkFormat (Sub system of Css )
//=============================================================================
#ifndef SkFormatCss_hpp
#define SkFormatCss_hpp

#include <SkAllocator.hpp>
#include "SkItemCss.hpp"
#include "SkFormatShare.hpp"
#include "SkLexerFormat.hpp"

#include "SkColor.hpp"
#include "SkBorder.hpp"
#include "SkFont.hpp"
#include "SkText.hpp"
#include "SkShadow.hpp"

using namespace SkRoot;

namespace SkFormat {

class tFormatRoot;

// SkFormat =================================================================
class  alignas(SkAlign) tFormatCss : public tFormatShare {
private:
    tUnitCss    m_Width;
    tUnitCss	m_Height;
    
    tFormatRef	m_Margin;
    tFormatRef	m_Padding;
    tFormatRef  m_Shadow;
    
    tColorCss	m_Color;
    tColorCss	m_BackgroundColor;
    
    tFormatRef	m_BorderRect;
    tFormatRef	m_Font;
    tFormatRef	m_Text;
    
    tInt        m_CountCell;
#ifdef checkfo
    tInt        m_CountCellCheck;
    tBool       m_FormatCell;
#endif
public:
    /// @brief		Constructor
    tFormatCss();
    
    /// @brief		Constructor of copy
    /// @param[in]	sFormatCss SkFormatCss
    tFormatCss(tFormatCss& sFormatCss);
    
    /// @brief        Clear (callback SkAllocator)
    void Clear();
    
    /// @brief      Clear Component
    void ClearComponent();
    
    /// @brief      Test  is empty
    tBool IsEmpty();
    
    /// @brief      Get key of element
    tString RootKey(tFormatRoot* sFormatRoot);
    
    /// @brief      Get String Css
    /// @param[in]	sFormatRoot SkFormatRoot
    /// @param[in]  sKind tKind
    /// @param[in]  sReturn tBool
    /// @return		tString;
    tString Str(tFormatRoot* sFormatRoot, tBool sReturn=false);
    
    /// @brief      Inc cell counter  / Spreadsheet
    void IncCell();
    
    /// @brief      Dec cell counter  / Spreadsheet
    void DecCell();
    
    /// @brief      Inc cell counter  / Spreadsheet
    /// @return     tIntt
    tInt Cell();
    
    /// @brief		Set width
    /// @param[in]	sWidth SkUnit
    void Width(tUnitCss sWidth);
    
    /// @brief		Get width
    /// @return		SkUnit
    tUnitCss Width();
    
    /// @brief		Set height
    /// @param[in]	sWidth SkUnit
    void Height(tUnitCss sHeight);
    
    /// @brief		Get height
    /// @return		SkUnit
    tUnitCss Height();
    
    /// @brief		Get color
    /// @return		SkColor
    tColorCss& Color();
    
    /// @brief		Get background color
    /// @return		SkColor
    tColorCss& BackgroundColor();
    
    /// @brief		Set ref on Margin
    /// @param[in]	sMarginRef tFormatRef
    void Margin(tFormatRef sMarginRef);
    
    /// @brief		Get ref on Margin
    /// @return		tFormatRef
    tFormatRef Margin();
    
    /// @brief		Get Margin
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkUnitRectCss*
    tUnitRectCss* Margin(tFormatRoot* sFormatRoot);
    
    /// @brief		Set ref on Padding
    /// @param[in]	sPaddingRef tFormatRef
    void Padding(tFormatRef sPaddingRef);
    
    /// @brief		Get ref on Padding
    /// @return		tFormatRef
    tFormatRef Padding();
    
    /// @brief		Get Padding
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkUnitRectCss*
    tUnitRectCss* Padding(tFormatRoot* sFormatRoot);
    
    /// @brief		Set ref on Shadow
    /// @param[in]	sShadowRef tFormatRef
    void Shadow(tFormatRef sShadowRef);
    
    /// @brief		Get ref on Shadow
    /// @return		tFormatRef
    tFormatRef Shadow();
    
    /// @brief		Get Shadow
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkShadowCss*
    tShadowCss* Shadow(tFormatRoot* sFormatRoot);
    
    /// @brief		Set ref on font
    /// @param[in]	sFontRef tFormatRef
    void Font(tFormatRef sFontRef);
    
    /// @brief		Get ref on font
    /// @return		tFormatRef
    tFormatRef Font();
    
    /// @brief		Get Font
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkFontCss*
    tFontCss* Font(tFormatRoot* sFormatRoot);
    
    /// @brief		Set ref on text
    /// @param[in]	sTextRef tFormatRef
    void Text(tFormatRef sTextRef);
    
    /// @brief		Get ref on text
    /// @return		tFormatRef
    tFormatRef Text();
    
    /// @brief		Get Text
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkTextCss*
    tTextCss* Text(tFormatRoot* sFormatRoot);
    
    /// @brief		Set ref on BorderRect
    /// @param[in]	sBorderRectRef tFormatRef
    void BorderRect(tFormatRef sBorderRectRef);
    
    /// @brief		Get ref on Border rect
    /// @return		tFormatRef
    tFormatRef BorderRect();
    
    /// @brief		Get BorderRect
    /// @param[in]	sFormatRoot SkFormatRoot*
    /// @return		SkBorderRectCss*
    tBorderRectCss*  BorderRect(tFormatRoot* sFormatRoot);
    

    // Json ===============================================================
    /// @brief		Writer Json.
    /// @param[in]	sWriter Writer<StringBuffer>*
    void Json(Writer<StringBuffer>* sWriter, tFormatRoot* sFormatRoot);
    
    /// @brief		Reader Json.
    /// @param[in]	sValue Value&
    void Json(const rapidjson::Value& sValue, tFormatRoot* sFormatRoot);
    
    /// brief		copy assignment
    /// @param[in]	sFormatCss SkFormatCss
    tFormatCss& operator=(const tFormatCss& sFormatCss);
    
    /// @brief      Return CountCell
    /// @return     tFormatRef
    tFormatRef CountCell();
    
    /// @brief      Get key of element
    /// @param[in]	sFormatRoot SkFormatRoot
    tString Debug(tFormatRoot* sFormatRoot);
    
#ifdef checkfo
    /// @brief      Set FormatCell
    /// @param[in]    sFormatCell  tBool
    void FormatCell(tBool sFormatCell);
    
    /// @brief      Set FormatCell
    /// @returnl
    tBool FormatCell();
    
    /// @brief Reset cell check
    void ResetCellCheck();

    /// @brief In count cell check
    void IncCountCellCheck();
    
    /// @brief      Reset check
    /// @param[in]	sFormatRoot SkFormatRoot
    void ResetCheck(tFormatRoot* sFormatRoot);
    
    /// @brief      Inc check
    /// @param[in]	sFormatRoot SkFormatRoot
    void IncCheck(tFormatRoot* sFormatRoot);
    
    /// @brief      Check cell
    /// @param[in]  sFormatRoot SkFormatRoot
    void CheckCell(tFormatRoot* sFormatRoot);
    
    /// @brief      Check
    /// @param[in]	sFormatRoot SkFormatRoot
    void Check(tFormatRoot* sFormatRoot);
#endif // checkfo
};

typedef std::map<tString, tFormatCss*> tMapFormatCss;
typedef std::map<tString, tFormatCss*>::iterator tMapFormatCssIterator;

//!  FormatMergeCss for merge FormatCss i n memory before apply ==============================
class  alignas(SkAlign) tFormatMergeCss : public tClass {
private:
    friend class tFormatRoot;
    tUnitCss        m_Width;
    tUnitCss        m_Height;
    
    tUnitRectCss    m_Margin;
    tUnitRectCss    m_Padding;
    tShadowCss      m_Shadow;
    
    tColorCss       m_Color;
    tColorCss       m_BackgroundColor;
    
    tBorderRectCss  m_BorderRect;
    tFontCss        m_Font;
    tTextCss        m_Text;
public:
    /// @brief        Constructor
    tFormatMergeCss();
  
    /// @brief        Clear
    void Clear();
    
    /// @brief      Get String Css
    /// @param[in]  sReturn tBool
    /// @return        tString;
    tString Str(tBool sReturn=false);
    
    /// @brief      Get Format String
    /// @return     tFormatString*
    tFormatString* FormatString();
    
    /// @brief      Get  borderRect
    /// @return     tBorderRect*
    tBorderRectCss* BorderRect();
  
    /// @brief        Writer Json for react.
    /// @param[in]    sWriter Writer<StringBuffer>*
    /// @param[in]    sVariant tVariant
    /// @param [in] sCss tBool (Css or Canvas)
    void JsonJavaScript(Writer<StringBuffer>* sWriter,tVariant* sVariant,tBool sCss);

    /// @brief Display value keys only (f_value, f_mo) — per-cell, not deduplicated.
    void JsonJavaScriptVariantDisplay(Writer<StringBuffer>* sWriter,tVariant* sVariant,tBool sCss);

    /// @brief Style keys only (no f_value) — candidate for JsonView format table dedup.
    void JsonJavaScriptStyle(Writer<StringBuffer>* sWriter,tBool sCss);
};

}; // end of namespace


#endif // end of SkFormatCss_hpp
