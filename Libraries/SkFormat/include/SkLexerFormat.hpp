//=============================================================================
// SkLexer  Lexer for Lemon or direct Interface 
//=============================================================================
#ifndef SkLexerFormat_hpp
#define SkLexerFormat_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkLemonFormat.h"
using namespace SkRoot;
using namespace std;

namespace SkFormat {

    //! Kind of lexer
    enum class tKind :  tUInt {
        Integer,
        Float,
        Identifier,
        
        // Css
        Css,
        CssClass,
        CssId,
        
        // Width & Height
        Width,
        Height,
        
        // Color
        BackgroundColor,
        Color,
        Opacity,
        Rgb,
        Rgba,
        
        // Margin
        Margin,
        Margin_left,
        Margin_top,
        Margin_right,
        Margin_bottom,
        
        // Padding
        Padding,
        Padding_left,
        Padding_top,
        Padding_right,
        Padding_bottom,
        
        // Text align
        Text_align,
         Left,
         Right,
         Center,
         Justify,
        
        // Vertical align
        Vertical_align,
         Baseline,
         Sub,
         Super,
         Text_top,
         Text_bottom,
         Middle,
         Top,
         Bottom,

        // Decoration line
        Decoration_line,

         Underline,
         Overline,
         Line_through,

        // Text wrap
        Text_wrap,
          Wrap,
          No_wrap,
        
        Text_rotate,
        // Font
        Font,
        Font_name,
        Font_family,
         //None
         Serif,
         Sans_serif,
         Monospace,
         Cursive,
         Fantasy,
         System_ui,
         Emoji,
         Math,
         Fangsong,
            
        Font_size,
        Font_Style,
         Normal,
         Italic,
         Oblique,
        
        Weight,
          // normal
          Bold,
          Lighter,
          Bolder,
        
        Font_variant,
         // normal
        Small_caps,
        
        Font_stretch,
         // none
         Ultra_condensed,
         Extra_condensed,
         Condensed,
         Semi_condensed,
         //Normal, use normal
         Semi_expanded,
         Expanded,
         Extra_expanded,
        Ultra_expanded,
        
        Line_height,
    
        // Border
        Border,
        Border_left,
        Border_top,
        Border_right,
        Border_bottom,
        
        Border_radius,
        Border_top_left_radius,
        Border_top_right_radius,
        Border_bottom_left_radius,
        Border_bottom_right_radius,

        None,
        Hidden,
        Dotted,
        Dashed,
        Solid,
        Double,
        Groove,
        Ridge,
        Inset,
        Outset,
    
        // Format string
        Formatstring,
        
        // Unit
        Px,
        Percent,
        In,
        Pt,
        Pc,
        Cm,
        Mm,
        Em,
        Deg,

        ColorHash,
        
        Hover,
        Focus,
        
        LeftParen,
        RightParen,
        LeftSquare,
        RightSquare,
        LeftCurly,
        RightCurly,
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
        Equal,
        NotEqual,
        Plus,
        Minus,
        Times,
        Divide,
        Exclamation,
        Ampersand,
        At,
        Hash,
        Dot,
        Comma,
        Colon,
        Semicolon,
        LabelSimple,
        LabelDouble,
        Comment,
        Pipe,
        Dollar,
        Cell,
        Attribute,
        Range,
        Sheet,
        Function,
        And,
        Or,
        Not,
        NewLine,
        End,
        Unexpected
    };

    typedef stack<tKind> tStackKind;
    
    struct tRecCss {
        public:
        tKind    m_Kind;
        tString  m_Name;
        tInt     m_LemonCst;
    };

    const tRecCss CstRecCss[] = {
        { tKind::Width, "width",F_WIDTH },
        { tKind::Height, "height",F_HEIGHT },
  
        
        { tKind::BackgroundColor, "background-color",F_BACKGROUNDCOLOR },
        { tKind::Color, "color",F_COLOR },
        { tKind::Opacity, "opacity",F_OPACITY },
        { tKind::Rgb, "rgb",F_RGB },
        { tKind::Rgba, "rgba",F_RGBA },
        
        { tKind::None, "none", F_NONE },
        // Margin
        { tKind::Margin,"margin",F_MARGIN },
        { tKind::Margin_left,"margin-left",F_MARGIN_LEFT },
        { tKind::Margin_top,"margin-top",F_MARGIN_TOP },
        { tKind::Margin_right,"margin-right",F_MARGIN_RIGHT },
        { tKind::Margin_bottom,"margin-bottom",F_MARGIN_BOTTOM },

        // Padding
        { tKind::Padding,"padding",F_PADDING },
        { tKind::Padding_left,"padding-left",F_PADDING_LEFT },
        { tKind::Padding_top,"padding-top",F_PADDING_TOP },
        { tKind::Padding_right,"padding-right",F_PADDING_RIGHT},
        { tKind::Padding_bottom,"padding-bottom",F_PADDING_BOTTOM },
        
        { tKind::Text_align, "text-align",F_TEXT_ALIGN },
        { tKind::Left, "left",F_LEFT },
        { tKind::Right, "right",F_RIGHT },
        { tKind::Center, "center",F_CENTER },
        { tKind::Justify, "justify",F_JUSTIFY },
 
        { tKind::Vertical_align, "vertical-align",F_VERTICAL_ALIGN },
        { tKind::Baseline, "baseline",F_BASELINE},
        { tKind::Sub, "sub",F_SUB},
        { tKind::Super, "super",F_SUPER},
        { tKind::Text_top, "text-top",F_TEXT_TOP},
        { tKind::Text_bottom, "text-bottom",F_TEXT_BOTTOM},
        { tKind::Middle, "middle",F_MIDDLE},
        { tKind::Top, "top",F_TOP },
        { tKind::Bottom, "bottom",F_BOTTOM },
     
        { tKind::Decoration_line, "text-decoration-line",F_DECORATION_LINE },
        { tKind::Underline, "underline",F_UNDERLINE },
        { tKind::Overline, "overline",F_OVERLINE },
        { tKind::Line_through, "line-through",F_LINE_THROUGH },
        
        { tKind::Text_wrap, "text-wrap",F_TEXT_WRAP },
        { tKind::Wrap, "wrap",F_WRAP },
        { tKind::No_wrap, "nowrap", F_NO_WRAP },

        { tKind::Text_rotate, "text-rotate",F_TEXT_ROTATE },
        // Font
        { tKind::Font,"font",F_FONT },
        { tKind::Font_name,"font-name",F_FONT_NAME },
        
        { tKind::Font_family,"font-family",F_FONT_FAMILY },
        { tKind::Serif , "serif", F_SERIF },
        { tKind::Sans_serif, "sans-serif", F_SANS_SERIF },
        { tKind::Monospace , "monospace", F_MONOSPACE },
        { tKind::Cursive , "cursive", F_CURSIVE },
        { tKind::Fantasy, "fantasy" , F_FANTASY },
        { tKind::System_ui, "system-ui", F_SYSTEM_UI },
        { tKind::Emoji, "emoji", F_EMOJI },
        { tKind::Math, "math" , F_MATH},
        { tKind::Fangsong, "fangsong", F_FANGSONG },
        
        { tKind::Font_size,"font-size",F_FONT_SIZE },
        
        { tKind::Font_Style,"font-style",F_FONT_STYLE },
        { tKind::Normal,"normal",F_NORMAL },
        { tKind::Italic,"italic",F_ITALIC },
        { tKind::Oblique,"oblique",F_OBLIQUE },
        
        { tKind::Weight,"font-weight",F_WEIGHT },
        { tKind::Bold,"bold", F_BOLD },
        { tKind::Lighter,"lighter", F_LIGHTER },
        { tKind::Bolder,"bolder", F_BOLDER},
        
        { tKind::Font_variant, "font-variant", F_FONT_VARIANT},
        { tKind::Small_caps, "small-caps", F_SMALL_CAPS },
      
        { tKind::Font_stretch, "font-stretch", F_FONT_STRETCH},
        { tKind::Ultra_condensed, "ultra-condensed",F_ULTRA_CONDENSED },
        { tKind::Condensed, "condensed" , F_CONDENSED },
        { tKind::Semi_condensed, "semi-condensed", F_SEMI_CONDENSED },
        //{ tKind::Stretch_normal, "normal", F_STRETCH_NORMAL},
        { tKind::Semi_expanded, "semi-expanded", F_SEMI_EXPANDED },
        { tKind::Expanded, "expanded", F_EXPANDED },
        { tKind::Extra_expanded, "extra-expanded", F_EXTRA_EXPANDED },
        { tKind::Ultra_expanded, "ultra-expanded", F_ULTRA_EXPANDED },

        { tKind::Line_height,"line-height",F_LINE_HEIGHT },
      
        // Border
        { tKind::Border,"border",F_BORDER },
        { tKind::Border_left,"border-left",F_BORDER_LEFT },
        { tKind::Border_top,"border-top",F_BORDER_TOP },
        { tKind::Border_right,"border-right",F_BORDER_RIGHT },
        { tKind::Border_bottom,"border-bottom",F_BORDER_BOTTOM },
        
        { tKind::Border_radius,"border-radius",F_BORDER_RADIUS },
        { tKind::Border_top_left_radius,"border-top-left-radius",F_BORDER_TOP_LEFT_RADIUS },
        { tKind::Border_top_right_radius,"border-top-right-radius",F_BORDER_TOP_RIGHT_RADIUS},
        { tKind::Border_bottom_left_radius,"border-bottom-left-radius",F_BORDER_BOTTOM_LEFT_RADIUS },
        { tKind::Border_bottom_right_radius,"border-bottom-right-radius",F_BORDER_BOTTOM_RIGHT_RADIUS },

        { tKind::Hidden,"hidden",F_HIDDEN },
        { tKind::Dotted,"dotted",F_DOTTED },
        { tKind::Dashed,"dashed",F_DASHED },
        { tKind::Solid,"solid",F_SOLID },
        { tKind::Double,"double",F_DOUBLE },
        { tKind::Groove,"groove",F_GROOVE},
        { tKind::Ridge,"ridge",F_RIDGE },
        { tKind::Inset,"inset",F_INSET },
        { tKind::Outset,"outset",F_OUTSET },

        { tKind::Formatstring,"format-string", F_FORMATSTRING },
        // Unit
        { tKind::Px,"px",F_PX},
        { tKind::Percent,"%",F_PERCENT },
        { tKind::In,"in",F_IN},
        { tKind::Pt,"pt",F_PT},
        { tKind::Pc,"pc",F_PC},
        { tKind::Cm,"cm",F_CM},
        { tKind::Mm,"mm",F_MM},
 
        { tKind::Em,"em",F_EM},
        { tKind::Deg,"deg",F_DEG},
 
        { tKind::Hover,"hover",F_HOVER},
        { tKind::Hover,"focus",F_FOCUS},

 
    };
    
    //=========================================================================
    //! Token used by } Allocated in Circular Memory
    class tLexerToken  {
    private:
        //! Kind
        tKind            m_Kind;
        //! Lexeme content
        const tChar*     m_Begin;
        const tChar*     m_End;
     
    public:
        tLexerToken() noexcept;
        /// @brief      Constructor with kind.
        /// @param[in]  sKind tKind
        tLexerToken(tKind sKind) noexcept;

        /// @brief      Constructor with kind and selection char.
        /// @param[in]  kind tKind
        /// @param[in]  beg tChar* pointer of begin char
        /// @param[in]  len tSize length of selection
        tLexerToken(tKind kind, const tChar* beg, tSize len) noexcept;

        /// @brief      Constructor with kind and selection char
        /// @param[in]  kind tKind
        /// @param[in]  beg tChar* pointer of begin char
        /// @param[in]  end tChar* pointer of end char
        tLexerToken(tKind kind, const tChar* beg, const tChar* end) noexcept;


        /// @brief      Constructor of copy
        /// @param[in]  sSkLexerToken const SkLexerToken&
        tLexerToken(const tLexerToken& sSkLexerToken) noexcept;

        /// @brief      destructor
        ~tLexerToken();

        /// @brief      Return Kind
        /// @return     tKind
        tKind Kind() const noexcept;

        /// @brief      Set Kind
        /// @param[in]  sValue tKind
        void Kind(tKind sValue) noexcept;

        /// @brief      Test if Kind is sKind
        /// @param[in]  sKind tKind
        /// @return     tBool
        tBool is(tKind sKind) const noexcept;

        /// @brief      Test if Kind is not sKind
        /// @param[in]  sKind tKind
        /// @return     tBool
        tBool is_not(tKind sKind) const noexcept;

        /// @brief      Test if Kind is Sk1 or Sk2
        /// @param[in]  sK1 tKind
        /// @param[in]  sK2 tKind
        /// @return     tBool
        tBool is_one_of(tKind sK1, tKind sK2) const noexcept;

        /// @brief      Test if Kind is Sk1 or Sk2 or in list
        /// @param[in]  sK1 tKind
        /// @param[in]  sK2 tKind
        /// @param[in]  sKs Ts...
        /// @return     tBool
        template <typename... Ts>
        tBool is_one_of(tKind sK1, tKind sK2, Ts... sKs) const noexcept;

        /// @brief      Return lexeme 
        /// @return     tString
        const tString Lexeme() const noexcept;

   
        /// @brief      new for use circular memory
        /// @return     sz tInt size to alloc
        void* operator new(tSize sz) noexcept;

        /// @brief      delete for use circular memory (do nothing)..
        /// @return     ptr void*
        void operator delete(void* ptr) noexcept;
    };

    typedef stack<tLexerToken*> SkStackLexerToken;

    //=========================================================================
    //! Lexer for formula SpreadSheet
    class tLexer {
    private:
        //! Current line 
        tInt m_Line;
        //! Current column
        tInt m_Column;

        //! Management Color & Reserverd Word
        tBool m_InCss;
        
        //! Pointeur of char for make string
        const tChar* m_beg = nullptr;
 
        /// @brief      Return identifier or sheet example ABCDEF or Sheet1!
        /// @return     SkLexerToken
        tLexerToken identifier() noexcept;

        /// @brief      Return number
        /// @return     SkLexerToken
        tLexerToken number() noexcept;

        
        /// @brief      Return COLOR #F00000 r
        /// @return     SkLexerToken
        tLexerToken color() noexcept;

        /// @brief      Return divide or comment
        /// @return     SkLexerToken
        tLexerToken divide_or_comment() noexcept;

        /// @brief      Return less than or equal or not eqaul < <= <> 
        /// @return     SkLexerToken
        tLexerToken LessThan_or_Equal_or_NotEqual() noexcept;

        /// @brief      Return greater than or equal > >= 
        /// @return     SkLexerToken
        tLexerToken GreaterThan_or_Equal() noexcept;

        /// @brief      Return label with " " 
        /// @return     SkLexerToken
        tLexerToken label_double_quote() noexcept;

        /// @brief      Return label with ' ' 
        /// @return     SkLexerToken
        tLexerToken label_simple_quote() noexcept;

    
        /// @brief      Alloc SkLexerToken
        /// @param[in]  sKind tKind
        /// @return     SkLexerToken
        inline tLexerToken atom(tKind sKind) noexcept;

        /// @brief      Return the last caracter
        /// @return     SkLexerToken
        inline tChar peek() const noexcept;

        /// @brief      Return the last caracter and skip
        /// @return     SkLexerToken
        inline tChar get() noexcept;

    public:
        /// @brief      Constructor.
        tLexer() noexcept;
        
        /// @brief      Constructor with pointer of char.
        /// @param[in]  beg tKind
        tLexer(const tChar* beg) noexcept;

        
        /// @brief      Set in Css for parsing color and reserveed  word
        /// @param1          sInCss tBool
        void InCss(tBool sInCss) noexcept;
        
        /// @brief      Get InCss.
        /// @return     tBool
        tBool InCss() noexcept;

        /// @brief      Search next item with notation A1.
        /// @return     SkLexerToken
        tLexerToken next() noexcept;

        //! Current column in lexer
        tInt Column();
        //! Current line in lexer
        tInt Line();
    };
    ostream& operator<<(ostream& os, const tKind& kind);

    ostream& operator<<(ostream& os, const tLexerToken& sSkLexerTokenk);

}
#endif
