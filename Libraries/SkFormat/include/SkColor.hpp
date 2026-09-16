//=============================================================================
// SkColor (Color for Css )
//=============================================================================
#ifndef SkColor_hpp
#define SkColor_hpp

#include<cmath>

#include "SkFormatShare.hpp"
#include <SkTypesClass.hpp>
namespace SkFormat {

	using namespace SkRoot;
	using namespace rapidjson;
	// Couleur ====================================
	typedef enum t_Color { // ARGB Alpha red green blue
		Black = 0xFF000000,
		Navy = 0xFF000080,
		DarkBlue = 0xFF00008B,
		MediumBlue = 0xFF0000CD,
		Blue = 0xFF0000FF,
		DarkGreen = 0xFF006400,
		Green = 0xFF008000,
		Teal = 0xFF008080,
		DarkCyan = 0xFF008B8B,
		DeepSkyBlue = 0xFF00BFFF,
		DarkTurquoise = 0xFF00CED1,
		MediumSpringGreen = 0xFF00FA9A,
		Lime = 0xFF00FF00,
		SpringGreen = 0xFF00FF7F,
		Aqua = 0xFF00FFFF,
		Cyan = 0xFF00FFFF,
		MidnightBlue = 0xFF191970,
		DodgerBlue = 0xFF1E90FF,
		LightSeaGreen = 0xFF20B2AA,
		ForestGreen = 0xFF228B22,
		SeaGreen = 0xFF2E8B57,
		DarkSlateGray = 0xFF2F4F4F, // Idem
		DarkSlateGrey = 0xFF2F4F4F,
		LimeGreen = 0xFF32CD32,
		MediumSeaGreen = 0xFF3CB371,
		Turquoise = 0xFF40E0D0,
		RoyalBlue = 0xFF4169E1,
		SteelBlue = 0xFF4682B4,
		DarkSlateBlue = 0xFF483D8B,
		MediumTurquoise = 0xFF48D1CC,
		Indigo = 0xFF4B0082,
		DarkOliveGreen = 0xFF556B2F,
		CadetBlue = 0xFF5F9EA0,
		CornflowerBlue = 0xFF6495ED,
		MediumAquaMarine = 0xFF66CDAA,
		DimGray = 0xFF696969, // Idem
		DimGrey = 0xFF696969,
		SlateBlue = 0xFF6A5ACD,
		OliveDrab = 0xFF6B8E23,
		SlateGray = 0xFF708090, // Idem
		SlateGrey = 0xFF708090,
		LightSlateGray = 0xFF778899, // Idem
		LightSlateGrey = 0xFF778899,
		MediumSlateBlue = 0xFF7B68EE,
		LawnGreen = 0xFF7CFC00,
		Chartreuse = 0xFF7FFF00,
		Aquamarine = 0xFF7FFFD4,
		Maroon = 0xFF800000,
		Purple = 0xFF800080,
		Olive = 0xFF808000,
		Gray = 0xFF808080, // Idem
		Grey = 0xFF808080,
		SkyBlue = 0xFF87CEEB,
		LightSkyBlue = 0xFF87CEFA,
		BlueViolet = 0xFF8A2BE2,
		DarkRed = 0xFF8B0000,
		DarkMagenta = 0xFF8B008B,
		SaddleBrown = 0xFF8B4513,
		DarkSeaGreen = 0xFF8FBC8F,
		LightGreen = 0xFF90EE90,
		MediumPurple = 0xFF9370D8,
		DarkViolet = 0xFF9400D3,
		PaleGreen = 0xFF98FB98,
		DarkOrchid = 0xFF9932CC,
		YellowGreen = 0xFF9ACD32,
		Sienna = 0xFFA0522D,
		Brown = 0xFFA52A2A,
		DarkGray = 0xFFA9A9A9, // Idem
		DarkGrey = 0xFFA9A9A9,
		LightBlue = 0xFFADD8E6,
		GreenYellow = 0xFFADFF2F,
		PaleTurquoise = 0xFFAFEEEE,
		LightSteelBlue = 0xFFB0C4DE,
		PowderBlue = 0xFFB0E0E6,
		FireBrick = 0xFFB22222,
		DarkGoldenRod = 0xFFB8860B,
		MediumOrchid = 0xFFBA55D3,
		RosyBrown = 0xFFBC8F8F,
		DarkKhaki = 0xFFBDB76B,
		Silver = 0xFFC0C0C0,
		MediumVioletRed = 0xFFC71585,
		IndianRed = 0xFFCD5C5C,
		Peru = 0xFFCD853F,
		Chocolate = 0xFFD2691E,
		Tan = 0xFFD2B48C,
		LightGray = 0xFFD3D3D3, // Idem
		LightGrey = 0xFFD3D3D3,
		PaleVioletRed = 0xFFD87093,
		Thistle = 0xFFD8BFD8,
		Orchid = 0xFFDA70D6,
		GoldenRod = 0xFFDAA520,
		Crimson = 0xFFDC143C,
		Gainsboro = 0xFFDCDCDC,
		Plum = 0xFFDDA0DD,
		BurlyWood = 0xFFDEB887,
		LightCyan = 0xFFE0FFFF,
		Lavender = 0xFFE6E6FA,
		DarkSalmon = 0xFFE9967A,
		Violet = 0xFFEE82EE,
		PaleGoldenRod = 0xFFEEE8AA,
		LightCoral = 0xFFF08080,
		Khaki = 0xFFF0E68C,
		AliceBlue = 0xFFF0F8FF,
		HoneyDew = 0xFFF0FFF0,
		Azure = 0xFFF0FFFF,
		SandyBrown = 0xFFF4A460,
		Wheat = 0xFFF5DEB3,
		Beige = 0xFFF5F5DC,
		WhiteSmoke = 0xFFF5F5F5,
		MintCream = 0xFFF5FFFA,
		GhostWhite = 0xFFF8F8FF,
		Salmon = 0xFFFA8072,
		AntiqueWhite = 0xFFFAEBD7,
		Linen = 0xFFFAF0E6,
		LightGoldenRodYellow = 0xFFFAFAD2,
		OldLace = 0xFFFDF5E6,
		Red = 0xFFFF0000,
		Fuchsia = 0xFFFF00FF,
		Magenta = 0xFFFF00FF,
		DeepPink = 0xFFFF1493,
		OrangeRed = 0xFFFF4500,
		Tomato = 0xFFFF6347,
		HotPink = 0xFFFF69B4,
		Coral = 0xFFFF7F50,
		DarkOrange = 0xFFFF8C00,
		LightSalmon = 0xFFFFA07A,
		Orange = 0xFFFFA500,
		LightPink = 0xFFFFB6C1,
		Pink = 0xFFFFC0CB,
		Gold = 0xFFFFD700,
		PeachPuff = 0xFFFFDAB9,
		NavajoWhite = 0xFFFFDEAD,
		Moccasin = 0xFFFFE4B5,
		Bisque = 0xFFFFE4C4,
		MistyRose = 0xFFFFE4E1,
		BlanchedAlmond = 0xFFFFEBCD,
		PapayaWhip = 0xFFFFEFD5,
		LavenderBlush = 0xFFFFF0F5,
		SeaShell = 0xFFFFF5EE,
		Cornsilk = 0xFFFFF8DC,
		LemonChiffon = 0xFFFFFACD,
		FloralWhite = 0xFFFFFAF0,
		Snow = 0xFFFFFAFA,
		Yellow = 0xFFFFFF00,
		LightYellow = 0xFFFFFFE0,
		Ivory = 0xFFFFFFF0,
		White = 0xFFFFFFFF,
		Transparent = 0x00000000,
	} t_Color;



	struct tRecColor {
		t_Color		 m_Color;
		tString      m_Key;
        tString      m_Name;
	};


    const tRecColor  CstRecColor[] = {
        { t_Color::Black, "black", "Black" },
        { t_Color::Navy, "navy", "Navy" },
        { t_Color::DarkBlue, "darkblue", "DarkBlue" },
        { t_Color::MediumBlue, "mediumblue", "MediumBlue" },
        { t_Color::Blue, "blue", "Blue" },
        { t_Color::DarkGreen, "darkgreen", "DarkGreen" },
        { t_Color::Green, "green", "Green" },
        { t_Color::Teal, "teal", "Teal" },
        { t_Color::DarkCyan, "darkcyan", "DarkCyan" },
        { t_Color::DeepSkyBlue, "deepskyblue", "DeepSkyBlue" },
        { t_Color::DarkTurquoise, "darkturquoise", "DarkTurquoise" },
        { t_Color::MediumSpringGreen, "mediumspringgreen", "MediumSpringGreen" },
        { t_Color::Lime, "lime", "Lime" },
        { t_Color::SpringGreen, "springgreen", "SpringGreen" },
        { t_Color::Aqua, "aqua", "Aqua" },
        { t_Color::Cyan, "cyan", "Cyan" },
        { t_Color::MidnightBlue, "midnightblue", "MidnightBlue" },
        { t_Color::DodgerBlue, "dodgerblue", "DodgerBlue" },
        { t_Color::LightSeaGreen, "lightseagreen", "LightSeaGreen" },
        { t_Color::ForestGreen, "forestgreen", "ForestGreen" },
        { t_Color::SeaGreen, "seagreen", "SeaGreen" },
        { t_Color::DarkSlateGray, "darkslategray", "DarkSlateGray" },
        { t_Color::DarkSlateGrey, "darkslategrey", "DarkSlateGrey" },
        { t_Color::LimeGreen, "limegreen", "LimeGreen" },
        { t_Color::MediumSeaGreen, "mediumseagreen", "MediumSeaGreen" },
        { t_Color::Turquoise, "turquoise", "Turquoise" },
        { t_Color::RoyalBlue, "royalblue", "RoyalBlue" },
        { t_Color::SteelBlue, "steelblue", "SteelBlue" },
        { t_Color::DarkSlateBlue, "darkslateblue", "DarkSlateBlue" },
        { t_Color::MediumTurquoise, "mediumturquoise", "MediumTurquoise" },
        { t_Color::Indigo, "indigo", "Indigo" },
        { t_Color::DarkOliveGreen, "darkolivegreen", "DarkOliveGreen" },
        { t_Color::CadetBlue, "cadetblue", "CadetBlue" },
        { t_Color::CornflowerBlue, "cornflowerblue", "CornflowerBlue" },
        { t_Color::MediumAquaMarine, "mediumaquamarine", "MediumAquaMarine" },
        { t_Color::DimGray, "dimgray", "DimGray" },
        { t_Color::DimGrey, "dimgrey", "DimGrey" },
        { t_Color::SlateBlue, "slateblue", "SlateBlue" },
        { t_Color::OliveDrab, "olivedrab", "OliveDrab" },
        { t_Color::SlateGray, "slategray", "SlateGray" },
        { t_Color::SlateGrey, "slategrey", "SlateGrey" },
        { t_Color::LightSlateGray, "lightslategray", "LightSlateGray" },
        { t_Color::LightSlateGrey, "lightslategrey", "LightSlateGrey" },
        { t_Color::MediumSlateBlue, "mediumslateblue", "MediumSlateBlue" },
        { t_Color::LawnGreen, "lawngreen", "LawnGreen" },
        { t_Color::Chartreuse, "chartreuse", "Chartreuse" },
        { t_Color::Aquamarine, "aquamarine", "Aquamarine" },
        { t_Color::Maroon, "maroon", "Maroon" },
        { t_Color::Purple, "purple", "Purple" },
        { t_Color::Olive, "olive", "Olive" },
        { t_Color::Gray, "gray", "Gray" },
        { t_Color::Grey, "grey", "Grey" },
        { t_Color::SkyBlue, "skyblue", "SkyBlue" },
        { t_Color::LightSkyBlue, "lightskyblue", "LightSkyBlue" },
        { t_Color::BlueViolet, "blueviolet", "BlueViolet" },
        { t_Color::DarkRed, "darkred", "DarkRed" },
        { t_Color::DarkMagenta, "darkmagenta", "DarkMagenta" },
        { t_Color::SaddleBrown, "saddlebrown", "SaddleBrown" },
        { t_Color::DarkSeaGreen, "darkseagreen", "DarkSeaGreen" },
        { t_Color::LightGreen, "lightgreen", "LightGreen" },
        { t_Color::MediumPurple, "mediumpurple", "MediumPurple" },
        { t_Color::DarkViolet, "darkviolet", "DarkViolet" },
        { t_Color::PaleGreen, "palegreen", "PaleGreen" },
        { t_Color::DarkOrchid, "darkorchid", "DarkOrchid" },
        { t_Color::YellowGreen, "yellowgreen", "YellowGreen" },
        { t_Color::Sienna, "sienna", "Sienna" },
        { t_Color::Brown, "brown", "Brown" },
        { t_Color::DarkGray, "darkgray", "DarkGray" },
        { t_Color::DarkGrey, "darkgrey", "DarkGrey" },
        { t_Color::LightBlue, "lightblue", "LightBlue" },
        { t_Color::GreenYellow, "greenyellow", "GreenYellow" },
        { t_Color::PaleTurquoise, "paleturquoise", "PaleTurquoise" },
        { t_Color::LightSteelBlue, "lightsteelblue", "LightSteelBlue" },
        { t_Color::PowderBlue, "powderblue", "PowderBlue" },
        { t_Color::FireBrick, "firebrick", "FireBrick" },
        { t_Color::DarkGoldenRod, "darkgoldenrod", "DarkGoldenRod" },
        { t_Color::MediumOrchid, "mediumorchid", "MediumOrchid" },
        { t_Color::RosyBrown, "rosybrown", "RosyBrown" },
        { t_Color::DarkKhaki, "darkkhaki", "DarkKhaki" },
        { t_Color::Silver, "silver", "Silver" },
        { t_Color::MediumVioletRed, "mediumvioletred", "MediumVioletRed" },
        { t_Color::IndianRed, "indianred", "IndianRed" },
        { t_Color::Peru, "peru", "Peru" },
        { t_Color::Chocolate, "chocolate", "Chocolate" },
        { t_Color::Tan, "tan", "Tan" },
        { t_Color::LightGray, "lightgray", "LightGray" },
        { t_Color::LightGrey, "lightgrey", "LightGrey" },
        { t_Color::PaleVioletRed, "palevioletred", "PaleVioletRed" },
        { t_Color::Thistle, "thistle", "Thistle" },
        { t_Color::Orchid, "orchid", "Orchid" },
        { t_Color::GoldenRod, "goldenrod", "GoldenRod" },
        { t_Color::Crimson, "crimson", "Crimson" },
        { t_Color::Gainsboro, "gainsboro", "Gainsboro" },
        { t_Color::Plum, "plum", "Plum" },
        { t_Color::BurlyWood, "burlywood", "BurlyWood" },
        { t_Color::LightCyan, "lightcyan", "LightCyan" },
        { t_Color::Lavender, "lavender", "Lavender" },
        { t_Color::DarkSalmon, "darksalmon", "DarkSalmon" },
        { t_Color::Violet, "violet", "Violet" },
        { t_Color::PaleGoldenRod, "palegoldenrod", "PaleGoldenRod" },
        { t_Color::LightCoral, "lightcoral", "LightCoral" },
        { t_Color::Khaki, "khaki", "Khaki" },
        { t_Color::AliceBlue, "aliceblue", "AliceBlue" },
        { t_Color::HoneyDew, "honeydew", "HoneyDew" },
        { t_Color::Azure, "azure", "Azure" },
        { t_Color::SandyBrown, "sandybrown", "SandyBrown" },
        { t_Color::Wheat, "wheat", "Wheat" },
        { t_Color::Beige, "beige", "Beige" },
        { t_Color::WhiteSmoke, "whitesmoke", "WhiteSmoke" },
        { t_Color::MintCream, "mintcream", "MintCream" },
        { t_Color::GhostWhite, "ghostwhite", "GhostWhite" },
        { t_Color::Salmon, "salmon", "Salmon" },
        { t_Color::AntiqueWhite, "antiquewhite", "AntiqueWhite" },
        { t_Color::Linen, "linen", "Linen" },
        { t_Color::LightGoldenRodYellow, "lightgoldenrodyellow", "LightGoldenRodYellow" },
        { t_Color::OldLace, "oldlace", "OldLace" },
        { t_Color::Red, "red", "Red" },
        { t_Color::Fuchsia, "fuchsia", "Fuchsia" },
        { t_Color::Magenta, "magenta", "Magenta" },
        { t_Color::DeepPink, "deeppink", "DeepPink" },
        { t_Color::OrangeRed, "orangered", "OrangeRed" },
        { t_Color::Tomato, "tomato", "Tomato" },
        { t_Color::HotPink, "hotpink", "HotPink" },
        { t_Color::Coral, "coral", "Coral" },
        { t_Color::DarkOrange, "darkorange", "DarkOrange" },
        { t_Color::LightSalmon, "lightsalmon", "LightSalmon" },
        { t_Color::Orange, "orange", "Orange" },
        { t_Color::LightPink, "lightpink", "LightPink" },
        { t_Color::Pink, "pink", "Pink" },
        { t_Color::Gold, "gold", "Gold" },
        { t_Color::PeachPuff, "peachpuff", "PeachPuff" },
        { t_Color::NavajoWhite, "navajowhite", "NavajoWhite" },
        { t_Color::Moccasin, "moccasin", "Moccasin" },
        { t_Color::Bisque, "bisque", "Bisque" },
        { t_Color::MistyRose, "mistyrose", "MistyRose" },
        { t_Color::BlanchedAlmond, "blanchedalmond", "BlanchedAlmond" },
        { t_Color::PapayaWhip, "papayawhip", "PapayaWhip" },
        { t_Color::LavenderBlush, "lavenderblush", "LavenderBlush" },
        { t_Color::SeaShell, "seashell", "SeaShell" },
        { t_Color::Cornsilk, "cornsilk", "Cornsilk" },
        { t_Color::LemonChiffon, "lemonchiffon", "LemonChiffon" },
        { t_Color::FloralWhite, "floralwhite", "FloralWhite" },
        { t_Color::Snow, "snow", "Snow" },
        { t_Color::Yellow, "yellow", "Yellow" },
        { t_Color::LightYellow, "lightyellow", "LightYellow" },
        { t_Color::Ivory, "ivory", "Ivory" },
        { t_Color::White, "white", "White" },
        { t_Color::Transparent, "transparent", "Transparent" }
	};
	enum class tTypeColor { CstColor, HexaColor, RGBColor };


	
	// Color ====================================================
	class tColorCss : public tClass {
	private:
		tFloat  m_Opacity;
		tColor  m_Color;
		tBool   m_NotUse;
	public:
		/// @brief		Constructor
		tColorCss();

		/// @brief		Constructor with value 
		/// @param[in]	sColor SkColor
		tColorCss(tColor sColor);
	
		/// @brief		Copy constructor
		/// @param[in]	sColor SkColor&
		tColorCss(const tColorCss& sColor);

		/// @brief      Get key of element
		/// @return		tString;
		tString Key();

		/// @brief      Get String Css
        /// @param[in]  sReturn tBool 
		/// @return		tString;
		tString Str(tBool sReturn=false);

		/// @brief		Set Opacity
		/// @param[in]	sOpacity tFloat
		void Opacity(tFloat sOpacity);

		/// @brief		Set Opacity
		/// @return		tFloat
		tFloat Opacity();


		/// @brief		Set Not Use
		/// @param[in]	sNotUse tBool
		void NotUse(tBool sNotUse);

		/// @brief		Return true if not use 
		/// @return		tBool
		tBool NotUse();

		/// @brief		Set RGB Color 
		/// @param[in]	sRed tUByte
		/// @param[in]	sGreen tUByte
		/// @param[in]	sBlue tUByte
		void ColorRgb(tUByte sRed, tUByte sGreen, tUByte sBlue);

        /// @brief       Return Red
        /// @return tUByte
        tUByte Red();

        /// @brief       Return Green
        /// @return tUByte
        tUByte Green();
        
        /// @brief       Return Blue;
        /// @return tUByte
        tUByte Blue();
        
        /// @brief		Get RGB Color
		/// @return		tuple <tUByte, tUByte, tUByte>
		tuple <tUByte, tUByte, tUByte> ColorRgb();
		
		/// @brief		Set Color
		/// @param[in]	sColor tInt
		void Color(tColor sColor);

		/// @brief		Get Color 
		/// @return		tInt
		tColor  Color();

		/// @brief		Set Hexadecimal value 
		/// @param[in]	sValue tString
		void ColorHex(tString sValue);

		/// @brief		Get Hexadecimal value of color 
		/// @return		tString
		tString ColorHex();


		/// @brief		Set Color by name
		/// @param[in]	sName tString
        void ColorName(tString sName);

		/// @brief		Get Color string 
		/// @return		tString
		tString StrKey();

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const rapidjson::Value& sValue);


		/// @brief		operator == 
		/// @param[in]	sColor SkColor&
		/// @return		tBool
		tBool operator == (tColorCss& sColor);
	};

    
	// Color Utis =============================================================
	tUByte GetR(tColor color);
	tUByte GetG(tColor color);
	tUByte GetB(tColor color);

	void RGB2HSL(tColor sColor, tFloat& sHue, tFloat& sSaturation, tFloat& sLuminance);
	tColor HSL2RGB(const tFloat& sHue, const tFloat& sSaturation, const tFloat& sLuminance);

	tColor BrightenColor(tColor sColor, tFloat sAmount);
	tColor DarkenColor(tColor sColor, tFloat sAmount);

}; // End of NameSpace

#endif // SkColort_hpp
