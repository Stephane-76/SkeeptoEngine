//=============================================================================
// SkLemonFormula Css  interface width Lemon
//=============================================================================
#include <SkApplication.hpp>
#include "../include/SkLemonFormatInterface.hpp"
#include "../include/SkLemonFormat.hpp"
#include "../include/SkLemonFormat.h"

#include <algorithm>
#include <cctype>

#define _DEBUGSKcompil

namespace SkFormat {

namespace {

// Lowercase ASCII lexeme for resolving CSS keywords when tKind was not set on the token.
tString LexemeLowerAscii(tLexerToken* sToken) {
    tString wLower = sToken->Lexeme();
    std::transform(wLower.begin(), wLower.end(), wLower.begin(),
                   [](unsigned char c) { return static_cast<tChar>(std::tolower(c)); });
    return wLower;
}

} // namespace

    // Class interface with Lemon ================================================
    tLemonFormatInterface::tLemonFormatInterface() : tClass(),
                m_FormatCss(nullptr),
                m_LemonReserved(),
                m_LemonParser(nullptr),
                m_Code(""),
                m_CompilError(false),
                m_Error(""),
                m_ErrorLine(0),
                m_ErrorColumn(0),
                m_Width(),
                m_Height(),
                m_Padding(),
                m_IsPadding(false),
                m_Margin(),
                m_IsMargin(false),
                m_FontCss(),
                m_IsFont(false),
                m_BorderRectCss(),
                m_IsBorder(false),
                m_TextCss(),
                m_IsText(),
                m_IsCellAlloc(false),
                m_CellAlloc(0) {
                m_FormatRoot = SkFormat::tFormatRoot::Instance();
    }

    void tLemonFormatInterface::Clear() {
        m_CompilError=false;
        m_Error="";
        m_ErrorLine=0;
        m_ErrorColumn=0;
        m_CellAlloc=0;
        m_Width.Unit(tUnitMetrics::none);
        m_Height.Unit(tUnitMetrics::none);
        m_Padding.Clear();
        m_IsPadding=false;
        m_Margin.Clear();
        m_IsMargin=false;
        m_FontCss.Clear();
        m_IsFont=false;
        m_BorderRectCss.Clear();
        m_IsBorder=false;
        m_TextCss.Clear();
        m_IsText=false;
        m_VectorUnitCss.clear();
        m_IsCellAlloc=false;
        m_CellAlloc = 0;
    }

    tBool tLemonFormatInterface::Compil(const tChar* sCode) {
#ifdef debugcompil
        cout << "Compil "  << sCode << endl;
#endif
        // Init each time for reset system (problem with synatx error)
        ParserInit(this);
        m_Code=tString(sCode);
        // Compilation with Lemon =====================================
        ParserRun(this, sCode);
        // Free parser
        ParserDone(this);
        
        if (CompilError()) {
           // not Ok
            cerr << ErrorWithDetail() << endl;
        }
         return(!CompilError());
    }

    tFormatRef tLemonFormatInterface::CellAlloc() { return (m_CellAlloc); }

    // Reference =================================================================
    tBool tLemonFormatInterface::AddIdRef(tString sName, tInt sId, tVirtualClass* sClass,tKind sKind) {
        return(m_LemonReserved.AddIdRef(sName, sId, sClass,sKind));
    }

    tInt tLemonFormatInterface::Id(tString sName) {
        return(m_LemonReserved.Id(sName));
    }

    tLemonFormatIdRef& tLemonFormatInterface::IdRef(tString sName) {
        return(m_LemonReserved.IdRef(sName));
    }
    
   
    // Interface formula stack ================================================
    void tLemonFormatInterface::BeginCss(tLexerToken* sName,tLexerToken* sSpeudo) {
        tString wName="";
        switch (sName->Kind()) {
            case tKind::Css:  break;
            case tKind::CssClass: wName+= "."; break;
            case tKind::CssId: wName+="#"; break;
            default:
                break;
        }
        wName+=sName->Lexeme();
        if (sSpeudo!=nullptr) {
            switch (sSpeudo->Kind()) {
                case tKind::Hover : wName+= ":hover"; break;
                case tKind::Focus : wName+= ":focus"; break;
                default:
                    break;
            }
        }
        // Is Cell
        if (wName=="ApplyCell") {
            m_IsCellAlloc=true;
            m_FormatRoot->AllocFormatRef();
        } else {
            m_IsCellAlloc=false;
            // Alloc Format
            m_FormatCss=m_FormatRoot->AllocFormat(wName);
        }
    }


    void tLemonFormatInterface::EndCss() {
        if (m_Width.Unit()!=tUnitMetrics::none) {
            m_FormatRoot->Width(m_Width);
        }
        if (m_Height.Unit()!=tUnitMetrics::none) {
            m_FormatRoot->Height(m_Height);
        }
        if (m_IsPadding) {
            m_FormatRoot->Padding(m_Padding);
        }
        if (m_IsMargin) {
            m_FormatRoot->Margin(m_Margin);
        }
        if (m_IsBorder) {
            m_FormatRoot->BorderRect(m_BorderRectCss);
        }
        if (m_IsFont) {
            m_FormatRoot->Font(m_FontCss);
        }
        if (m_IsText) {
            m_FormatRoot->Text(m_TextCss);
        }
        if (m_IsCellAlloc) {
            m_CellAlloc=m_FormatRoot->ApplyCell();
        } else {
            m_FormatRoot->Validate();
        }
    }

    void tLemonFormatInterface::BeginKind(tLexerToken* sToken) {
    
        /*
        switch (sToken->Kind()) {
            case tKind::Width: cout << "Width" << endl; break;
            case tKind::Height: cout << "Height" << endl; break;
            case tKind::Color: cout << "Color" << endl; break;
            case tKind::BackgroundColor: cout << "Backgroundcolor" << endl; break;
            
            case tKind::Padding: cout << "Padding" << endl; break;
            case tKind::Padding_left: cout << "Padding-left" << endl; break;
            case tKind::Padding_top: cout << "Padding-top" << endl; break;
            case tKind::Padding_right: cout << "Padding-right" << endl; break;
            case tKind::Padding_bottom: cout << "Padding-bottom" << endl; break;
            
            case tKind::Margin: cout << "Margin" << endl; break;
            case tKind::Margin_left: cout << "Margin-left" << endl; break;
            case tKind::Margin_top: cout << "Margin-top" << endl; break;
            case tKind::Margin_right: cout << "Margin-right" << endl; break;
            case tKind::Margin_bottom: cout << "Margin-bottom" << endl; break;
            
            case tKind::Font: cout << "Font" << endl; break;
            
            case tKind::Border: cout << "Border" << endl; break;
            case tKind::Border_left: cout << "Border-left" << endl; break;
            case tKind::Border_top: cout << "Border-top" << endl; break;
            case tKind::Border_right: cout << "Border-right" << endl; break;
            case tKind::Border_bottom: cout << "Border-bottom" << endl; break;
            case tKind::Border_radius: cout << "Border-radius" << endl; break;
            case tKind::Border_top_left_radius: cout << "Border-top-left-radius" << endl; break;
            case tKind::Border_top_right_radius: cout << "Border-top-right-radius" << endl; break;
            case tKind::Border_bottom_left_radius: cout << "Border-bottom-left-radius" << endl; break;
            case tKind::Border_bottom_right_radius: cout << "Border-bottom-right-radius" << endl; break;
            default:
                break;
        }
        */
        // Push Last Token
        m_StackKind.push(sToken->Kind());
    }

    
    void tLemonFormatInterface::EndKind() {
        if (m_VectorUnitCss.size()>0) {
            // Treat multiple size like paddin 10px 20px 20px 40px
            tKind wLastElem=m_StackKind.top();
            switch (wLastElem) {
                case tKind::Padding: {
                    m_Padding.ApplyUnit(&m_VectorUnitCss);
                    break;
                }
                case tKind::Margin:  {
                    m_Margin.ApplyUnit(&m_VectorUnitCss);
                    break;
                }
                case tKind::Border: {
                    m_BorderRectCss.ApplyUnit(&m_VectorUnitCss);
                    break;
                }
                case tKind::Border_radius:  {
                    m_BorderRectCss.ApplyRadiusUnit(&m_VectorUnitCss);
                    break;
                }
                default:
                    break;
            }
        }
        m_VectorUnitCss.clear();
        m_StackKind.pop();
    }

    const tRecColor* tLemonFormatInterface::Color(tString sName) {
        return(m_LemonReserved.Color(sName));
    }

    void tLemonFormatInterface::ColorStr(tLexerToken* sToken) {
        tKind wLastElem=m_StackKind.top();
        switch (wLastElem) {
            case tKind::Color: m_FormatRoot->Color().ColorName(sToken->Lexeme());  break;
            case tKind::BackgroundColor: m_FormatRoot->BackgroundColor().ColorName(sToken->Lexeme());  break;
            case tKind::Border:  m_BorderRectCss.All().Color().ColorName(sToken->Lexeme()); break;
            case tKind::Border_left: m_BorderRectCss.Left().Color().ColorName(sToken->Lexeme()); break;
            case tKind::Border_top: m_BorderRectCss.Top().Color().ColorName(sToken->Lexeme()); break;
            case tKind::Border_right: m_BorderRectCss.Right().Color().ColorName(sToken->Lexeme()); break;
            case tKind::Border_bottom: m_BorderRectCss.Bottom().Color().ColorName(sToken->Lexeme()); break;
            default:
                break;
        }
    }

    void tLemonFormatInterface::ColorHash(tLexerToken* sToken) {
        tKind wLastElem=m_StackKind.top();
        tString wColorHex=sToken->Lexeme();
        wColorHex.erase(0,1);
        
        switch (wLastElem) {
            case tKind::Color: m_FormatRoot->Color().ColorHex(wColorHex);  break;
            case tKind::BackgroundColor: m_FormatRoot->BackgroundColor().ColorHex(wColorHex);  break;
            case tKind::Border:  m_BorderRectCss.All().Color().ColorHex(wColorHex); break;
            case tKind::Border_left: m_BorderRectCss.Left().Color().ColorHex(wColorHex); break;
            case tKind::Border_top: m_BorderRectCss.Top().Color().ColorHex(wColorHex); break;
            case tKind::Border_right: m_BorderRectCss.Right().Color().ColorHex(wColorHex); break;
            case tKind::Border_bottom: m_BorderRectCss.Bottom().Color().ColorHex(wColorHex); break;
            default:
                break;
        }
    }

    void tLemonFormatInterface::ColorRgb(tLexerToken* sR,tLexerToken* sG,tLexerToken* sB) {
        tKind wLastElem=m_StackKind.top();
        tUByte wRed=tUByte(atoi(sR->Lexeme().c_str()));
        tUByte wGreen=tUByte(atoi(sG->Lexeme().c_str()));
        tUByte wBlue=tUByte(atoi(sB->Lexeme().c_str()));
        switch (wLastElem) {
            case tKind::Color: m_FormatRoot->Color().ColorRgb(wRed, wGreen, wBlue);  break;
            case tKind::BackgroundColor: m_FormatRoot->BackgroundColor().ColorRgb(wRed, wGreen, wBlue); break;
            case tKind::Border:  m_BorderRectCss.All().Color().ColorRgb(wRed, wGreen, wBlue); break;
            case tKind::Border_left: m_BorderRectCss.Left().Color().ColorRgb(wRed, wGreen, wBlue);   break;
            case tKind::Border_top: m_BorderRectCss.Top().Color().ColorRgb(wRed, wGreen, wBlue);  break;
            case tKind::Border_right: m_BorderRectCss.Right().Color().ColorRgb(wRed, wGreen, wBlue);  break;
            case tKind::Border_bottom: m_BorderRectCss.Bottom().Color().ColorRgb(wRed, wGreen, wBlue);  break;
            default:
                break;
        }
    }

    void tLemonFormatInterface::ColorRgba(tLexerToken* sR,tLexerToken* sG,tLexerToken* sB,tLexerToken* sA) {
        tKind wLastElem=m_StackKind.top();
        tUByte wRed=tUByte(atoi(sR->Lexeme().c_str()));
        tUByte wGreen=tUByte(atoi(sG->Lexeme().c_str()));
        tUByte wBlue=tUByte(atoi(sB->Lexeme().c_str()));
        tUByte wAlpha=tUByte(atoi(sA->Lexeme().c_str()));
        tFloat wOpacity=tFloat(255 / wAlpha);
   
        switch (wLastElem) {
            case tKind::Color:  {
                m_FormatRoot->Color().ColorRgb(wRed, wGreen, wBlue);
                m_FormatRoot->Color().Opacity(wOpacity);
                break;
            }
            case tKind::BackgroundColor: {
                m_FormatRoot->BackgroundColor().ColorRgb(wRed, wGreen, wBlue);
                m_FormatRoot->BackgroundColor().Opacity(wOpacity);
                break;
            }
            default:
                break;
        }
    }

    void tLemonFormatInterface::Opacity(tLexerToken* sToken) {
        tFloat wOpacity=tFloat(atof(sToken->Lexeme().c_str()));
        m_FormatRoot->BackgroundColor().Opacity(wOpacity);
    }

    void tLemonFormatInterface::FontName(tLexerToken* sName) {
        m_FontCss.Name(sName->Lexeme());
        m_IsFont=true;
    }

    void tLemonFormatInterface::FontFamily(tLexerToken* sFamily) {
        tFontFamily wFontFamily=tFontFamily::none;
        switch (sFamily->Kind()) {
            case tKind::Serif: wFontFamily=tFontFamily::serif; break;
            case tKind::Sans_serif: wFontFamily=tFontFamily::sans_serif; break;
            case tKind::Monospace: wFontFamily=tFontFamily::monospace; break;
            case tKind::Cursive: wFontFamily=tFontFamily::cursive; break;
            case tKind::Fantasy: wFontFamily=tFontFamily::fantasy; break;
            case tKind::System_ui: wFontFamily=tFontFamily::system_ui; break;
            case tKind::Emoji: wFontFamily=tFontFamily::emoji; break;
            case tKind::Math: wFontFamily=tFontFamily::math; break;
            case tKind::Fangsong: wFontFamily=tFontFamily::fangsong; break;
            default:
                break;
        }
        m_FontCss.Family(wFontFamily);
    }

    void tLemonFormatInterface::FontStyle(tLexerToken* sToken) {
        // Do not default to Style(none): an unrecognized Kind would wipe a prior shorthand value.
        tFontStyle wFontStyle = tFontStyle::notuse;
        switch (sToken->Kind()) {
            case tKind::None: wFontStyle=tFontStyle::none; break;
            case tKind::Normal: wFontStyle=tFontStyle::normal;  break;
            case tKind::Oblique: wFontStyle=tFontStyle::oblique; break;
            case tKind::Italic: wFontStyle=tFontStyle::italic; break;
            case tKind::Integer:
            case tKind::Float: {
                wFontStyle=tFontStyle::deg;
                tFloat wFloat=tFloat(atof(sToken->Lexeme().c_str()));
                m_FontCss.ObliqueDegrees(wFloat);
                break;
            }
            default:
                break;
        }
        if (wFontStyle == tFontStyle::notuse) {
            const tString wLower = LexemeLowerAscii(sToken);
            if (wLower == "italic") wFontStyle = tFontStyle::italic;
            else if (wLower == "oblique") wFontStyle = tFontStyle::oblique;
            else if (wLower == "normal") wFontStyle = tFontStyle::normal;
            else if (wLower == "none") wFontStyle = tFontStyle::none;
        }
        if (wFontStyle == tFontStyle::notuse)
            return;
        m_FontCss.Style(wFontStyle);
        m_IsFont=true;

    }

    void tLemonFormatInterface::FontWeight(tLexerToken* sToken) {
        tFontWeight wFontWeight = tFontWeight::notuse;
        tInt wWeightInt=0;
        switch (sToken->Kind()) {
            case tKind::None: wFontWeight=tFontWeight::none; break;
            case tKind::Normal: wFontWeight=tFontWeight::normal; break;
            case tKind::Bold: wFontWeight=tFontWeight::bold; break;
            case tKind::Lighter: wFontWeight=tFontWeight::lighter;   break;
            case tKind::Bolder: wFontWeight=tFontWeight::bolder; break;
            case tKind::Integer: {
                wFontWeight=tFontWeight::integer;
                wWeightInt=atoi(sToken->Lexeme().c_str());
                break;
            }
            case tKind::Float: {
                wFontWeight=tFontWeight::integer;
                wWeightInt=static_cast<tInt>(atof(sToken->Lexeme().c_str()));
                break;
            }
            default:
                break;
        }
        if (wFontWeight == tFontWeight::notuse) {
            const tString wLower = LexemeLowerAscii(sToken);
            if (wLower == "bold") wFontWeight = tFontWeight::bold;
            else if (wLower == "lighter") wFontWeight = tFontWeight::lighter;
            else if (wLower == "bolder") wFontWeight = tFontWeight::bolder;
            else if (wLower == "normal") wFontWeight = tFontWeight::normal;
        }
        if (wFontWeight == tFontWeight::notuse)
            return;
        m_FontCss.Weight(wFontWeight,wWeightInt);
        m_IsFont=true;

    }
    
    void tLemonFormatInterface::FontVariant(tLexerToken* sToken) {
        tFontVariant wFontVariant=tFontVariant::none;
        switch (sToken->Kind()) {
            case tKind::Normal: wFontVariant=tFontVariant::normal;  break;
            case tKind::Small_caps: wFontVariant=tFontVariant::small_caps;  break;
            default:
                break;
        }
        m_FontCss.Variant(wFontVariant);
        m_IsFont=true;
    }

    void tLemonFormatInterface::FontStretch(tLexerToken* sToken) {
        tFontStretch wFontStretch=tFontStretch::none;
        tFloat wFloat=0;
        switch (sToken->Kind()) {
            case tKind::Normal : wFontStretch=tFontStretch::normal; break;
            case tKind::Ultra_condensed : wFontStretch=tFontStretch::ultra_condensed; break;
            case tKind::Condensed : wFontStretch=tFontStretch::condensed; break;
            case tKind::Semi_condensed : wFontStretch=tFontStretch::semi_condensed; break;
            case tKind::Semi_expanded : wFontStretch=tFontStretch::semi_expanded; break;
            case tKind::Expanded : wFontStretch=tFontStretch::expanded; break;
            case tKind::Extra_expanded : wFontStretch=tFontStretch::extra_expanded; break;
            case tKind::Ultra_expanded : wFontStretch=tFontStretch::ultra_expanded; break;
            case tKind::Integer :
            case tKind::Float : {
                wFontStretch=tFontStretch::percent;
                wFloat=tFloat(atof(sToken->Lexeme().c_str()));
                m_FontCss.StretchPercent(wFloat);
                break;
            }
            default:
                break;

        }
        m_FontCss.Stretch(wFontStretch);
        m_IsFont=true;
    }
   
    void tLemonFormatInterface::FontLineHeight(tLexerToken* sToken) {
        switch (sToken->Kind()) {
            case tKind::Integer: {
                tInt wInteger=atoi(sToken->Lexeme().c_str());
                m_FontCss.LineHeight(tUnitCss(tFloat(wInteger),tUnitMetrics::number));
                break;
            }
        default:
            break;
        }
    }

    void tLemonFormatInterface::TextAlign(tLexerToken* sToken) {
        tTextAlign wTextAlign=tTextAlign::none;
        switch (sToken->Kind()) {
            case tKind::None: wTextAlign=tTextAlign::none; break;
            case tKind::Left: wTextAlign=tTextAlign::left; break;
            case tKind::Right: wTextAlign=tTextAlign::right; break;
            case tKind::Center:wTextAlign=tTextAlign::center; break;
            case tKind::Justify: wTextAlign=tTextAlign::justify; break;
            default:
                break;
        }
        m_TextCss.TextAlign(wTextAlign);
        m_IsText=true;
    }

    void tLemonFormatInterface::VerticalAlign(tLexerToken* sToken) {
        tVerticalTextAlign wVerticalTextAlign=tVerticalTextAlign::none;
        switch (sToken->Kind()) {
            case tKind::None: wVerticalTextAlign=tVerticalTextAlign::none; break;
            case tKind::Baseline: wVerticalTextAlign=tVerticalTextAlign::baseline; break;
            case tKind::Sub: wVerticalTextAlign=tVerticalTextAlign::sub; break;
            case tKind::Super: wVerticalTextAlign=tVerticalTextAlign::super; break;
            case tKind::Text_top: wVerticalTextAlign=tVerticalTextAlign::text_top; break;
            case tKind::Text_bottom: wVerticalTextAlign=tVerticalTextAlign::text_bottom; break;
            case tKind::Middle: wVerticalTextAlign=tVerticalTextAlign::middle; break;
            case tKind::Top: wVerticalTextAlign=tVerticalTextAlign::top; break;
            case tKind::Bottom: wVerticalTextAlign=tVerticalTextAlign::bottom; break;
            default:
                break;
        }
        m_TextCss.VerticalTextAlign(wVerticalTextAlign);
        m_IsText=true;
    }

    void tLemonFormatInterface::TextWrap(tLexerToken* sToken) {
        tTextWrap wTextWrap=tTextWrap::notuse;
        switch (sToken->Kind()) {
            case tKind::Wrap: wTextWrap=tTextWrap::wrap; break;
            case tKind::No_wrap: wTextWrap=tTextWrap::nowrap; break;
            default:
                break;
        }
        m_TextCss.TextWrap(wTextWrap);
        m_IsText=true;
    }
    void tLemonFormatInterface::TextRotate(tLexerToken* sToken) {
        tFloat wFloat=0;
        switch (sToken->Kind()) {
            case tKind::Float :
            case tKind::Integer : wFloat=tFloat(atof(sToken->Lexeme().c_str())); break;
            default:
                break;
        }
        m_TextCss.Rotate(wFloat);
        m_IsText=true;
    }
 
    void tLemonFormatInterface::TextDecoration(tLexerToken* sToken) {
        switch (sToken->Kind()) {
            case tKind::None : {
                m_TextCss.UnderLine(tTextDecorationLine::none);
                m_TextCss.OverLine(tTextDecorationLine::none);
                m_TextCss.LineThrough(tTextDecorationLine::none);
                break;
            }
            case tKind::Underline : m_TextCss.UnderLine(tTextDecorationLine::use); break;
            case tKind::Overline : m_TextCss.OverLine(tTextDecorationLine::use); break;
            case tKind::Line_through : m_TextCss.LineThrough(tTextDecorationLine::use); break;
            default:
                break;
        }
        m_IsText=true;
    };

    void tLemonFormatInterface::BorderStyle(tLexerToken* sToken) {
        tBorderStyle wBorderStyle=tBorderStyle::none;
        switch (sToken->Kind()) {
            case tKind::None: wBorderStyle=tBorderStyle::none; break;
            case tKind::Hidden: wBorderStyle=tBorderStyle::hidden; break;
            case tKind::Dotted: wBorderStyle=tBorderStyle::dotted; break;
            case tKind::Dashed: wBorderStyle=tBorderStyle::dashed; break;
            case tKind::Solid: wBorderStyle=tBorderStyle::solid; break;
            case tKind::Double: wBorderStyle=tBorderStyle::_double; break;
            case tKind::Groove: wBorderStyle=tBorderStyle::groove; break;
            case tKind::Ridge: wBorderStyle=tBorderStyle::ridge; break;
            case tKind::Inset: wBorderStyle=tBorderStyle::inset; break;
            case tKind::Outset:wBorderStyle=tBorderStyle::outset; break;
            default:
                break;
        }
        tKind wLastElem=m_StackKind.top();
        switch (wLastElem) {
            case tKind::Border: m_BorderRectCss.All().BorderStyle(wBorderStyle); break;
            case tKind::Border_left: m_BorderRectCss.Left().BorderStyle(wBorderStyle); break;
            case tKind::Border_top: m_BorderRectCss.Top().BorderStyle(wBorderStyle); break;
            case tKind::Border_right: m_BorderRectCss.Right().BorderStyle(wBorderStyle); break;
            case tKind::Border_bottom: m_BorderRectCss.Bottom().BorderStyle(wBorderStyle); break;
            default:
               break;
        }
        m_IsBorder=true;
    }

    void tLemonFormatInterface::PushUnit(tKind sUnit,tLexerToken* sToken) {
        // Unit
        tUnitMetrics wUnit=tUnitMetrics::none;
        switch (sUnit) {
            case tKind::Px: wUnit=tUnitMetrics::pixels; break;
            case tKind::Percent: wUnit=tUnitMetrics::percent; break;
            case tKind::In: wUnit=tUnitMetrics::inches; break;
            case tKind::Pt: wUnit=tUnitMetrics::points; break;
            case tKind::Pc: wUnit=tUnitMetrics::picas; break;
            case tKind::Cm: wUnit=tUnitMetrics::centimeters; break;
            case tKind::Mm: wUnit=tUnitMetrics::millimeters; break;
            case tKind::Em: wUnit=tUnitMetrics::em; break;
            default:
                break;
        }
 
        // Value ==============================================================
        tFloat wFloat=0;
        switch (sToken->Kind()) {
            case tKind::Float :
            case tKind::Integer : wFloat=tFloat(atof(sToken->Lexeme().c_str())); break;
            default:
                break;
        }
        tKind wLastElem=m_StackKind.top();
        tUnitCss wUnitCss=tUnitCss(wFloat,wUnit);
        switch (wLastElem) {
            case tKind::Width: m_Width=wUnitCss; break;
            case tKind::Height: m_Height=wUnitCss; break;

            case tKind::Font_size :
            case tKind::Font: m_FontCss.Size(wUnitCss); m_IsFont=true; break;
            
            
            case tKind::Padding: m_VectorUnitCss.push_back(wUnitCss); m_IsPadding=true; break;
            case tKind::Padding_left: m_Padding.Left(wUnitCss); m_IsPadding=true; break;
            case tKind::Padding_top: m_Padding.Top(wUnitCss); m_IsPadding=true; break;
            case tKind::Padding_right: m_Padding.Right(wUnitCss); m_IsPadding=true; break;
            case tKind::Padding_bottom: m_Padding.Bottom(wUnitCss); m_IsPadding=true; break;
            
            case tKind::Margin: m_VectorUnitCss.push_back(wUnitCss); m_IsMargin=true; break;
            case tKind::Margin_left: m_Margin.Left(wUnitCss); m_IsMargin=true; break;
            case tKind::Margin_top: m_Margin.Top(wUnitCss);  m_IsMargin=true; break;
            case tKind::Margin_right: m_Margin.Right(wUnitCss);  m_IsMargin=true; break;
            case tKind::Margin_bottom: m_Margin.Bottom(wUnitCss);  m_IsMargin=true; break;
            
            case tKind::Border: m_VectorUnitCss.push_back(wUnitCss); m_IsBorder=true;  break;
            case tKind::Border_left: m_BorderRectCss.Left().Width(wUnitCss); m_IsBorder=true; break;
            case tKind::Border_top: m_BorderRectCss.Top().Width(wUnitCss); m_IsBorder=true; break;
            case tKind::Border_right: m_BorderRectCss.Right().Width(wUnitCss); m_IsBorder=true; break;
            case tKind::Border_bottom: m_BorderRectCss.Bottom().Width(wUnitCss); m_IsBorder=true; break;
                
            case tKind::Border_radius: m_VectorUnitCss.push_back(wUnitCss); m_IsBorder=true;  break;
            case tKind::Border_top_left_radius: m_BorderRectCss.TopLeftRadius().Unit(wUnitCss);  m_IsBorder=true; break;
            case tKind::Border_top_right_radius: m_BorderRectCss.TopRightRadius().Unit(wUnitCss);  m_IsBorder=true; break;
            case tKind::Border_bottom_left_radius: m_BorderRectCss.BottomLeftRadius().Unit(wUnitCss);  m_IsBorder=true; break;
            case tKind::Border_bottom_right_radius: m_BorderRectCss.BottomRightRadius().Unit(wUnitCss);  m_IsBorder=true; break;
            default:
                break;
        }
    }

    
    void tLemonFormatInterface::FormatString(tLexerToken* sToken,tLexerToken* sPrecision) {
        tString wFormatStr=sToken->Lexeme();
#ifdef debugcompil
        cout << "tLemonFormatInterface::FormatString " << sToken->Lexeme();
        if (sPrecision!=nullptr) cout<< " (" << sPrecision->Lexeme() << ")";
        cout << endl;
#endif
        m_TextCss.FormatString(wFormatStr);
      
        tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
        // Saerch
        
        tFormatStringType wFormatStringType=wFormatStringRoot->StringKey2FormatStringType(wFormatStr);
        // Format String predefined
        if (wFormatStringType!=tFormatStringType::none) {
            if (sPrecision!=nullptr) {
    #ifdef debugcompil
                cout << "Precision value " << atoi(sPrecision->Lexeme().c_str()) << endl;
    #endif
                tShort wPrecision=tShort(atoi(sPrecision->Lexeme().c_str()));
                m_TextCss.Precision(tByte(wPrecision));
            } else {
                m_TextCss.Precision(tByte(wFormatStringRoot->DefaultPrecision(wFormatStringType)));
            }
    #ifdef debugcompil
            cout << "No precision value" << endl;
    #endif
            tFormatStringFamily wFormatStringFamily=wFormatStringRoot->StringKey2FormatStringFamily(wFormatStr);
            if (wFormatStringFamily==tFormatStringFamily::accounting) {
                m_TextCss.Money(tApplication::Instance()->Locale()->UnitMoney());
            }
          
            m_IsText=true;
            return;
        } else {
            // Format String Excel
            tFormatString* wFormatString= m_TextCss.FormatString();
            if (wFormatString->ExcelFormat(wFormatStr)) {
                tApplication* wApplication=tApplication::Instance();
                if (wFormatString->FormatType()==tFormatStringType::excelnumber) {
                   tNumberFormatter* wNumberFormatter=wApplication->NumberFormatter();
                   
                   // Parse the Excel format string to extract formatting information
                   auto wFormatParser = wNumberFormatter->GetFormatParser(wFormatStr);
                   if (wFormatParser && wFormatParser->IsValid()) {
                       // Get currency symbol
                       tString wCurrencySymbol = wFormatParser->GetCurrencySymbol();
                       
                       // Get decimal separator
                       tString wDecimalSeparator = wFormatParser->GetDecimalSeparator();
                       
                       // Get thousands separator
                       tString wThousandsSeparator = wFormatParser->GetThousandsSeparator();
                       
                       // Get number of decimal places
                       tInt wDecimalPlaces = wFormatParser->GetDecimalPlaces();
                       
                       // You can now use these extracted information for your formatting logic
                       // For example, you could set CSS properties based on these values
                       if (!wCurrencySymbol.empty()) {
                           // Set currency symbol in CSS
                            //m_TextCss.CurrencySymbol(wCurrencySymbol);
                       }
                       
                       if (!wDecimalSeparator.empty()) {
                           // Set decimal separator in CSS
                           //m_TextCss.DecimalSeparator(wDecimalSeparator);
                       }
                       
                       if (wDecimalPlaces > 0) {
                           // Set precision based on decimal places
                           m_TextCss.Precision(tByte(wDecimalPlaces));
                       }
                       
                       // Debug output to see what was parsed
#ifdef debugcompil
                       tInt wIntegerDigits = wFormatParser->GetIntegerDigits();
                       tBool wHasPercent = wFormatParser->HasPercent();
                       const tVectorExcelFormatElement& wElements = wFormatParser->GetElements();
                       tString wIntegerPart = wFormatParser->GetIntegerPart();
                       tString wDecimalPart = wFormatParser->GetDecimalPart();
                       cout << "Parsed Excel format: " << wFormatStr << endl;
                       cout << "Currency Symbol: '" << wCurrencySymbol << "'" << endl;
                       cout << "Decimal Separator: '" << wDecimalSeparator << "'" << endl;
                       cout << "Thousands Separator: '" << wThousandsSeparator << "'" << endl;
                       cout << "Decimal Places: " << wDecimalPlaces << endl;
                       cout << "Integer Digits: " << wIntegerDigits << endl;
                       cout << "Has Percent: " << (wHasPercent ? "true" : "false") << endl;
                       cout << "Integer Part Pattern: '" << wIntegerPart << "'" << endl;
                       cout << "Decimal Part Pattern: '" << wDecimalPart << "'" << endl;
                       cout << "Number of Elements: " << wElements.size() << endl;
#endif
                       m_IsText=true;
                       return;
                   }
                } else {
                    if (wFormatString->FormatType()==tFormatStringType::exceldate) {
                        tDateFormatter* wDateFormatter=wApplication->DateFormatter();
                        // Parse the Excel format string to extract formatting information
                        auto wFormatParser = wDateFormatter->GetFormatParser(wFormatStr);
                        tString wExcelFormat = wFormatString->FormatExcel();
                        if (wFormatParser && wFormatParser->IsValid()) {
                            m_TextCss.ExcelFormatString(wFormatStr);
                            m_IsText=true;
                            return;
                        }
                    }
                }
            }
        }
       
#ifdef debugcompil
        cout << "tLemonFormatInterface::FormatString end error format" << endl;
#endif
        //Error
        CompilError(true);
        Error("Bad format string : " + wFormatStr, 1, 1);
    }

    void tLemonFormatInterface::FormatStringMoney(tLexerToken* sToken,tLexerToken* sMoney,tLexerToken* sPrecision) {
#ifdef debugcompil
        cout << "FormatStringMoney " << sToken->Lexeme() << ":" << sMoney->Lexeme() << ":" << sPrecision->Lexeme() << endl;
#endif
        m_TextCss.FormatString(sToken->Lexeme());
        
        tString wMoneyStr=sMoney->Lexeme();
        m_TextCss.Money(UnitMoney(wMoneyStr));
        
        if (sPrecision!=nullptr) {
#ifdef debugcompil
            cout << "Precision value " << sPrecision->Lexeme() << endl;
#endif
            m_TextCss.Precision(tByte(stoi(sPrecision->Lexeme())));
        } else {
#ifdef debugcompil
            cout << "No precision value" << endl;
#endif
            tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
            // Search default value in format
            tFormatStringType wFormatStringType=wFormatStringRoot->StringKey2FormatStringType(sToken->Lexeme());
            m_TextCss.Precision(tByte(wFormatStringRoot->DefaultPrecision(wFormatStringType)));
        }
        m_IsText=true;
    }

    void* tLemonFormatInterface::LemonParser() { return(m_LemonParser); }
    void tLemonFormatInterface::LemonParser(void* sLemonParser) { m_LemonParser = sLemonParser; }

    void tLemonFormatInterface::CompilError(tBool sValue) {
        m_CompilError = sValue;
    }

    tBool tLemonFormatInterface::CompilError() { return(m_CompilError);}

    void tLemonFormatInterface::Error(tString sLexerError,tInt sLine, tInt sCol) {
        m_Error=sLexerError;
        m_ErrorLine=sLine;
        m_ErrorColumn=sCol;
 #ifdef debugcompil
        cerr << "Error: " <<  ErrorWithDetail() << endl;
#endif
    }

    tString tLemonFormatInterface::ErrorWithDetail() {
        tStringStream wStreamError;
        wStreamError << m_Error << " [";
        tString wError=m_Code;
        if (wError.find('\n') != std::string::npos) {
            wStreamError << m_ErrorLine << ",";
        }
        wStreamError << m_ErrorColumn << "]";
        
        // Split the code into lines
        std::vector<tString> wLines;
        std::istringstream iss(wError);
        tString wLine;
        
        // Handle single line case
        if (wError.find('\n') == std::string::npos) {
            wLines.push_back(wError);
        } else {
            while (std::getline(iss, wLine)) {
                wLines.push_back(wLine);
            }
        }

        // Check if the line number is valid
        if (m_ErrorLine <= 0 || m_ErrorLine > static_cast<tInt>(wLines.size())) {
            wError+="empty !";
        } else {
            // Calculate the position in the original string
            tInt position = 0;
            for (tInt i = 0; i < m_ErrorLine - 1; i++) {
                position += tInt(wLines[i].length()) + 1; // +1 for the newline character
            }
            position += m_ErrorColumn;

            // Insert the "^" character at the error position
            wError.insert(position, "^....");
        }
        wStreamError << "->" << wError;
        return(wStreamError.str());
    }

    tString tLemonFormatInterface::Error() { return(m_Error); }

    tInt tLemonFormatInterface::ErrorColumn() { return(m_ErrorColumn); }

    tInt tLemonFormatInterface::ErrorLine() { return(m_ErrorLine); }    

    void tLemonFormatInterface::Debug() {
    }
} // end of namespace
