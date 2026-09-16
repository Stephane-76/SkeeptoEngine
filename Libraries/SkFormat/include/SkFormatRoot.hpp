//=============================================================================
// SkFormat (Sub system of Css )
//=============================================================================
#ifndef SkFormatRoot_hpp
#define SkFormatRoot_hpp

#include "SkFormatCss.hpp"

using namespace SkRoot;

namespace SkFormat {

    //
	// FormatRoot =============================================================
	class tFormatRoot : public tClass {
	private:
		///! Type of allocator
		typedef tAllocator<tFormatCss, tFormatRef, 512> tAllocatorFormatPool;

		///! Type of map for name & poolr
		typedef map<tString, tFormatRef> tMapKey;

        //! Type of map for color
        typedef map<tString,const tRecColor*> tMapNameColor;
        typedef map<tUInt,const tRecColor*> tMapColor;

		// Allocator ==========================================================
		tItemCss<tUnitRectCss, 32> m_AllocatorPadding;
		tItemCss<tUnitRectCss, 32> m_AllocatorMargin;

		tItemCss<tFontCss, 32> m_AllocatorFont;
		tItemCss<tTextCss, 32> m_AllocatorText;
		tItemCss<tBorderRectCss, 32> m_AllocatorBorderRect;
		tItemCss<tShadowCss, 32> m_AllocatorShadow;

		tAllocatorFormatPool m_AllocatorFormat;

        ///! Type of map for pool
		tMapKey	    m_MapFormatPool;
        //! Name of format...
        //! In the case of the spreadsheet the map contains the allocatorRef of Format
        //! in this cas we are sure that key is unique (see m_CSS in cell)
		tMapKey		m_MapFormat;
        
        //! Map For Json
        typedef std::map<tAllocatorRef,tSize> tMapJsonCell;
        tMapJsonCell m_MapJsonCell;
        tVectorString m_VectorFormatCell;
        //! Type of map for color
        tMapNameColor m_MapNameColor;
        tMapColor   m_MapColor;

		tString		m_CurrentKey;
		tString		m_OldKeyPool;

		tFormatCss  m_CurrentFormat;
        
        tFormatCss  m_ModifyFormat;
		tFormatRef  m_ModifyRef;

        // For Merge ==========================================================
        tFormatMergeCss m_FormatMerge;
		

		void IncComponent(tFormatCss* sFormat);
		void DeleteComponent(tFormatCss* sFormat);
	public:
		static tFormatRoot* m_StaticFormatRoot;
	public:
		/// @brief		Constructor
		tFormatRoot();

		/// @brief		Clear
		void Clear();

		/// @brief		Reset (Restart);
		void Reset();


		/// @brief      Return Instance of SkFormatRoot
		/// @return		SkFormatRoot*
		static tFormatRoot* Instance();

        /// @brief    Return Instance of SkFormatRoot
        /// @return   tVectorString
        tVectorString ListOfCssName();
        
		// Interface By Key =======================================================
		/// @brief      Alloc format return new format
		/// @return		tFormatCss* 
		tFormatCss* AllocFormatRef();

		/// @brief      return format
		///	@param[in]  sFormatRef tFormatRef
		/// @return		SkFormatCss*
		tFormatCss* FormatRef(tFormatRef sFormatRef);

		/// @brief		Delete format
		/// @param[in]  sFormatRef tFormatRef
		void DeleteFormatRef(tFormatRef sFormatRef);


		// Interface By Key ===================================================
        /// @brief      Alloc format return new format
		/// @param[in]  sKey tString
		/// @return		SkFormatCss*
		tFormatCss* AllocFormat(tString sKey);

		/// @brief      return format
		///	@param[in]  sKey tString
		/// @return		SkFormatCss*
		tFormatCss* Format(tString sKey);

		/// @brief		Delete format
		/// @param[in]  sKey tString
		void DeleteFormat(tString sKey);

		/// @brief		Modify format
		/// @param[in]  sKey tString;
		tFormatCss* ModifyFormat(tString sKey);

		/// @brief		End format construction
		/// @return		tFormatRef
		tFormatRef Validate();
        
        // Interface By Cell ===================================================
        /// This is to manage the formats attached to the cells of the spreadsheet
        ///=====================================================================
        /// @brief      Applycell
        /// @return     tFormatRef
        tFormatRef ApplyCell();


        /// @brief      Delete cell
        /// @param[in]  sCellInstance tFormatRef
        void DeleteCell(tFormatRef sCellInstance);
  
        /// @brief      Inc  cell
        /// @param[in]  sCellInstance tFormatRef
        void IncCell(tFormatRef sCellInstance);
  
        // Interface with Elemn ================================================
        /// @brief      Merge Format Css
        /// @param[in]  sPivotKey tString 
        /// @param[in]  sKey tString  
        /// @return     SkFormatCss*
		tFormatCss* MergeInPlace(tString sPivotKey, tString sKey);


        /// @brief      Begin Merge operation
        void BeginMerge();
        
        /// @brief      Merge with Format in allocator ref
        /// @param[in]  sCellInstance tFormatRef
        void Merge(tFormatRef sPivotInstance);
        
        /// @brief      Begin Merge operation
        /// @return     tFormatRef
        tFormatRef ApplyMerge();
        
        
        /// @brief      Return formatMerge
        /// @Return FormatMerge
        tFormatMergeCss* FormatMerge();
        
        // Border =============================================================
        /// @brief     Delete Border
        /// @param[in] sInstance  tFormatRef
        /// @param[in] sBorderMask  tShort
        /// @return tFormatRef
        tFormatRef DeleteBorder(tFormatRef sInstance,tShort sBorderMask);
        
        /// @brief     Return Border mask
        /// @param[in] sInstance  tFormatRef
        /// @return  tShort
        tShort BorderMask(tFormatRef sInstance);
        
		/// @brief      Get String Css
		/// @param[in]	sKey tString
		/// @return		tString;
		tString Str(tString sKey);

		/// @brief      Get all Css
		/// @return		tString;
		tString Str();

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
		
		// Modify current format ==============================================
		/// @brief		Get color 
		/// @return		SkColor
		tColorCss& Color();

		/// @brief		Get background color 
		/// @return		SkColor
		tColorCss& BackgroundColor();

		/// Margin ============================================================
		/// @brief		Set Margin 
		/// @param[in]  sMargin SkUnitRect
		void Margin(tUnitRectCss& sMargin);

		/// @brief		Get ref on Margin 
		/// @param[in]  sMarginRef tFormatRef
		/// @return		SkUnitRect&
		tUnitRectCss* Margin(tFormatRef sMarginRef);

		/// @brief		Get ref on Margin 
		/// @return		SkUnitRect&
		tUnitRectCss* CurrentMargin();

		/// @brief		Delete Margin 
		/// @param[in]  sMarginRef SkRoot;
		/// @return		tBool
		tBool DeleteMargin(tFormatRef sMarginRef);

		/// Padding ===========================================================
		/// @brief		Set Padding 
		/// @param[in]  sPadding SkUnitRect
		void Padding(tUnitRectCss& sPadding);

		/// @brief		Get ref on Padding 
		/// @param[in]  sPaddingRef tFormatRef
		/// @return		SkUnitRect&
		tUnitRectCss* Padding(tFormatRef sPaddingRef);

		/// @brief		Get ref on Padding 
		/// @return		SkUnitRect&
		tUnitRectCss* CurrentPadding();

		/// @brief		Delete Padding 
		/// @param[in]  sPaddingRef SkRoot;
		/// @return		tBool
		tBool DeletePadding(tFormatRef sPaddingRef);

		/// Shadow ===========================================================
		/// @brief		Set Shadow 
		/// @param[in]  sShadow SkUnitRect
		void Shadow(tShadowCss& sShadow);

		/// @brief		Get ref on Shadow 
		/// @param[in]  sShadowRef tFormatRef
		/// @return		SkUnitRect&
		tShadowCss* Shadow(tFormatRef sShadowRef);

		/// @brief		Get ref on Shadow 
		/// @return		SkUnitRect&
		tShadowCss* CurrentShadow();

		/// @brief		Delete Shadow 
		/// @param[in]  sShadowRef SkRoot;
		/// @return		tBool
		tBool DeleteShadow(tFormatRef sShadowRef);

		/// Font ==============================================================
		/// @brief		Set font 
		/// @param[in]  sFont SkFont
		void Font(tFontCss& sFont);

		/// @brief		Get ref on Font 
		/// @param[in]  sFontRef tFormatRef
		/// @return		SkFont&
		tFontCss* Font(tFormatRef sFontRef);

		/// @brief		Get ref on font 
		/// @return		SkFont&
		tFontCss* CurrentFont();

		/// @brief		Delete Font 
		/// @param[in]  sFontRef SkRoot;
		/// @return		tBool
		tBool DeleteFont(tFormatRef sFontRef);

		/// Text ==============================================================
		/// @brief		Set text 
		/// @param[in]  sText SkText
		void Text(tTextCss& sText);

		/// @brief		Get ref on text 
		/// @param[in]  sTextRef tFormatRef
		/// @return		SkText&
		tTextCss* Text(tFormatRef sTextRef);

		/// @brief		Get ref on current format text 
		/// @return		SkText&
		tTextCss* CurrentText();

		/// @brief		Delete Text 
		/// @param[in]  sTextRef tFormatRef
		/// @return		tBool
		tBool DeleteText(tFormatRef sTextRef);

		/// BorderRect ========================================================
		/// @brief		Set BorderRect 
		/// @param[in]  sBorderRect SkBorderRect
		void BorderRect(tBorderRectCss& sBorderRect);

		/// @brief		Get ref on BorderRect 
		/// @param[in]  sBorderRectRef tFormatRef
		/// @return		SkBorderRect&
		tBorderRectCss* BorderRect(tFormatRef sBorderRectRef);

		/// @brief		Get ref on current format BorderRect 
		/// @return		SkBorderRect&
		tBorderRectCss* CurrentBorderRect();

		/// @brief		Delete BorderRect 
		/// @param[in]  sBorderRectRef tFormatRef
		/// @return		tBool
		tBool DeleteBorderRect(tFormatRef sBorderRectRef);

		// Json ===============================================================
        /// @brief Begin WriteJson
        void BeginWriteJson();

        /// @brief   Return Json format  m_css conversion (allocatorRef)
        /// @param[in] sFormatRef tFormatRef
        /// @return tSize
        tSize WriteJsonAddFormat(tFormatRef sFormatRef);
        
        /// @brief ReadJsonGetFormat
        /// @param[in] sIndex tSize
        /// @return tSize
        tString ReadJsonGetFormat(tSize sIndex);
        
        // Json ===============================================================
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonSpreadSheet(Writer<StringBuffer>* sWriter);

        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void JsonSpreadSheet(const rapidjson::Value& sValue);
        
        
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void Json(const rapidjson::Value& sValue);
        
		/// @brief		Writer Json in String.
		/// @return		SkSring
		tString WriteJson();

		/// @brief		Reader Json. 
		/// @param[in]  Json tString
		void ReadJson(tString sJson);

        // Color ==============================================================
        /// @brief      Return Color by Name
        /// @param[in]  sName  tString
        /// @return     const tRecColor*
        const tRecColor* FindColorByName(tString sName);

        /// @brief      Return Color byColor
        /// @param[in]  sColor  t_Color
        /// @return     const tRecColor*
        const tRecColor* FindColorByColor(tUInt sColor);    
        
        ///  @brief Return number of Format
        ///  @return fFormatRef
        tFormatRef Count();
    
#ifdef _DEBUGSK
		tString DebugFormat();
		tString DebugMargin();
		tString DebugPadding();
		tString DebugShadow();


		tString DebugFont();
		tString DebugText();
		tString DebugBorderRect();
#endif // _DEBUGSK
#ifdef checkfo
        /// @brief      Method reset check
        void ResetCheckCell();
        
        /// @brief      Check cell
        void CheckCell();
            
		/// @brief      check
		void Check();
#endif

	};

	// @brief function for terminate format
	void DoneFormatRoot();

}

#endif // end of SkFormatRoot_hpp
