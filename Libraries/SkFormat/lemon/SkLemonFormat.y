//=============================================================================
// SkLemnonFormat.y Parser Lemon Format 
//=============================================================================
%include {   

#include "../include/SkLemonFormat.hpp"

#define _debugparser

namespace SkFormat {

// Callback for allocator
void* ParseAlloc(void * (tSize));
void  ParseFree(void *,void  (void *));

// Prédefinition lemon ========================================================
void Parse(
  void *yyp,      /* The parser */
  int yymajor,    /* The major token code number */
  tToken yyminor,   /* The value for the token  see %token_type {tToken} */
  tLemonFormatInterface* sFormula /* see %extra_argument { tLemonFormatInterface *wInterface } */
);

void ParseTrace(FILE* TraceFILE, char* zTracePrompt);

// Share Value ================================================================
// Error 
tInt wGlobalLine=0;
tInt wGlobalColumn=0;

// Management Color
tBool wGlobalInCss = false;

using namespace SkRoot;
  void token_destructor(tToken sToken) {
  }
}
} // end of namespace

%token_type {tToken}
%default_type {tToken}
%token_destructor { token_destructor($$); }

%extra_argument { tLemonFormatInterface *wInterface }

%type expr {tToken}

%type F_ID {tToken}

%type F_LABELDQ {tToken}
%type F_LABELSQ {tToken}

%type F_DOT {tToken}
%type F_COMMA {tToken}
%type F_COLON {tToken}
%type F_EXCLAMATION {tToken}
%type F_NEWLINE {tToken}
%type F_COMMENT {tToken}

// Width & height
%type   F_WIDTH {tToken}
%type   F_HEIGHT {tToken}
// Color
%type   F_BACKGROUNDCOLOR {tToken}
%type   F_COLOR {tToken}
%type   F_OPACITY {tToken}
%type   F_COLORSTR {tToken}
%type   F_COLORHASH {tToken}
%type   F_RGB {tToken} 
%type   F_RGBA {tToken}

// Text
%type   F_TEXT_ALIGN {tToken}
%type   F_LEFT {tToken}
%type   F_RIGHT {tToken}
%type   F_CENTER {tToken}
%type   F_JUSTIFY {tToken}
        
%type   F_VERTICAL_ALIGN {tToken}
%type   F_BASELINE {tToken}
%type   F_SUB {tToken}
%type   F_SUPER {tToken}
%type   F_TEXT_TOP {tToken}
%type   F_TEXT_BOTTOM {tToken}
%type   F_TOP {tToken}
%type   F_BOTTOM {tToken}
%type   F_MIDDLE {tToken}
        
%type   F_DECORATION_LINE { tToken }
%type   F_UNDERLINE { tToken }
%type   F_OVERLINE { tToken }
%type   F_LINE_THROUGH { tToken }

%type   F_TEXT_WRAP { tToken }
%type   F_WRAP { tToken }
%type   F_NO_WRAP { tToken }

%type   F_TEXT_ROTATE { tToken }

// FONT
%type   F_FONT {tToken}
%type   F_FONT_NAME {tToken}

%type   F_FONT_FAMILY {tToken}
%type   F_SERIF {tToken}
%type   F_SANS_SERIF {tToken}
%type   F_MONOSPACE {tToken}
%type   F_CURSIVE {tToken}
%type   F_FANTASY {tToken}
%type   F_SYSTEM_UI {tToken}
%type   F_EMOJI {tToken}
%type   F_MATH {tToken}
%type   F_FANGSONG {tToken}

%type   F_FONT_SIZE {tToken}

%type   F_FONT_STYLE {tToken}
%type   F_NORMAL {tToken}
%type   F_ITALIC {tToken}
%type   F_OBLIQUE {tToken}
%type   F_PERCENT {tToken}

%type   F_FONT_VARIANT {tToken}
%type   F_SMALL_CAPS {tToken}

%type   F_FONT_STRETCH {tToken}
%type   F_ULTRA_CONDENSED {tToken}
%type   F_CONDENSED {tToken}
%type   F_SEMI_CONDENSED {tToken}
%type   F_SEMI_EXPANDED {tToken}
%type   F_EXPANDED {tToken}
%type   F_EXTRA_EXPANDED {tToken}
%type   F_ULTRA_EXPANDED {tToken}

%type   F_LINE_HEIGHT {tToken}

%type   F_WEIGHT {tToken}
%type   F_BOLD {tToken}
%type   F_LIGHTER {tToken}
%type   F_BOLDER {tToken}
        
        // BORDER
%type   F_BORDER {tToken}
%type   F_BORDER_LEFT {tToken}
%type   F_BORDER_TOP {tToken}
%type   F_BORDER_RIGHT {tToken}
%type   F_BORDER_BOTTOM {tToken}
        
%type   F_BORDER_RADIUS {tToken}
%type   F_BORDER_TOP_LEFT_RADIUS {tToken}
%type   F_BORDER_TOP_RIGHT_RADIUS {tToken}
%type   F_BORDER_BOTTOM_LEFT_RADIUS {tToken}
%type   F_BORDER_BOTTOM_RIGHT_RADIUS {tToken}

%type   F_NONE {tToken}
%type   F_HIDDEN {tToken}
%type   F_DOTTED {tToken}
%type   F_DASHED {tToken}
%type   F_SOLID {tToken}
%type   F_DOUBLE {tToken}
%type   F_GROOVE {tToken}
%type   F_RIDGE {tToken}
%type   F_INSET {tToken}
%type   F_OUTSET {tToken}
    
%type  F_FORMATSTRING  {tToken}  
        // MARGIN
%type   F_MARGIN {tToken}
%type   F_MARGIN_LEFT {tToken}
%type   F_MARGIN_TOP {tToken}
%type   F_MARGIN_RIGHT {tToken}
%type   F_MARGIN_BOTTOM {tToken}
        
        // PADDING
%type   F_PADDING {tToken}
%type   F_PADDING_LEFT {tToken}
%type   F_PADDING_TOP {tToken}
%type   F_PADDING_RIGHT {tToken}
%type   F_PADDING_BOTTOM {tToken}

        // UNIT
%type   F_PX {tToken}
%type   F_IN {tToken} 
%type   F_PT {tToken}
%type   F_PC {tToken}
%type   F_CM {tToken}
%type   F_MM {tToken}

%type   F_EM {tToken}
%type   F_DEG {tToken}

%type   F_HOVER {tToken}
%type   F_FOCUS {tToken}

// Left (for priority of operator)
%left F_SEMICOLON.
%left F_RIGHTCURLY.
%left F_LEFTCURLY.

%left F_RIGHTPARENT.
%left F_RIGHTSQUARE.

%left F_AMPERSAND.

%left F_LEFTPARENT.
%left F_LEFTSQUARE.

// Right (for priority of operator)
%right F_EXP F_NOT.

%parse_accept {
  // code for accept
}
   
%syntax_error {  
  wInterface->CompilError(true);
  tStringStream wStream;
  wStream << "Syntax error"; 
  wInterface->Error(wStream.str(),wGlobalLine,wGlobalColumn);
}
   
/*  Begin of CSS  ============================================================*/
main ::= . 
main ::= classes F_END. {}

class ::= beginClass endClass.

classes ::= class.
classes ::= class classes.

beginClass ::= className(B) F_LEFTCURLY. { wInterface->BeginCss(B.m_Token,nullptr); }
beginClass ::= className(B) pseudoClasses(C) F_LEFTCURLY. { wInterface->BeginCss(B.m_Token,C.m_Token); }

endClass ::= cssList F_RIGHTCURLY. { wInterface->EndCss(); wGlobalInCss=false; }

className(A) ::= F_ID(B).  { A=B; A.m_Token->Kind(tKind::Css); wGlobalInCss=true; }
className(A) ::= F_DOT F_ID(B). { A=B; A.m_Token->Kind(tKind::CssClass); wGlobalInCss=true; }
className(A) ::= F_HASH F_ID(B). { A=B; A.m_Token->Kind(tKind::CssId); wGlobalInCss=true; }

pseudoClasses(A) ::= F_COLON F_HOVER(B).  { A=B; }
pseudoClasses(A) ::= F_COLON F_FOCUS(B). { A=B; }

cssList ::= .
cssList ::= css F_SEMICOLON cssList.

// Integer or float (F_DOUBLE is the border-style keyword "double", not a number).
number(A) ::= F_INTEGER(B). { A=B; }
number(A) ::= F_FLOAT(B). { A=B; }


// WIDTH & HEIGHT ==============================================================
css ::= widthheight F_COLON unit. { wInterface->EndKind(); }

widthheight ::= F_WIDTH(B).  { wInterface->BeginKind(B.m_Token); }
widthheight ::= F_HEIGHT(B). { wInterface->BeginKind(B.m_Token); }


// MARGIN ======================================================================
css ::= marginAll F_COLON unitValues. { wInterface->EndKind(); }
css ::= marginElem F_COLON unit. { wInterface->EndKind(); }

marginAll ::= F_MARGIN(B). { wInterface->BeginKind(B.m_Token); }
marginElem  ::= F_MARGIN_LEFT(B). { wInterface->BeginKind(B.m_Token); }
marginElem  ::= F_MARGIN_TOP(B). { wInterface->BeginKind(B.m_Token); }
marginElem  ::= F_MARGIN_RIGHT(B). { wInterface->BeginKind(B.m_Token); }
marginElem  ::= F_MARGIN_BOTTOM(B). { wInterface->BeginKind(B.m_Token); }

// PADDING =====================================================================
css ::= paddingAll F_COLON unitValues. { wInterface->EndKind(); }
css ::= paddingElem F_COLON unit. { wInterface->EndKind(); }

paddingAll ::= F_PADDING(B). { wInterface->BeginKind(B.m_Token); }
paddingElem ::= F_PADDING_LEFT(B). { wInterface->BeginKind(B.m_Token); }
paddingElem ::= F_PADDING_TOP(B). { wInterface->BeginKind(B.m_Token); }
paddingElem ::= F_PADDING_RIGHT(B). { wInterface->BeginKind(B.m_Token); }
paddingElem ::= F_PADDING_BOTTOM(B). { wInterface->BeginKind(B.m_Token); }

css ::= colorElem F_COLON color. { wInterface->EndKind(); }

colorElem ::= F_COLOR(B). { wInterface->BeginKind(B.m_Token); }
colorElem ::= F_BACKGROUNDCOLOR(B). { wInterface->BeginKind(B.m_Token); }

css ::= F_OPACITY F_COLON F_FLOAT(B). { wInterface->Opacity(B.m_Token); }
css ::= F_OPACITY F_COLON F_INTEGER(B). { wInterface->Opacity(B.m_Token); }

color::= F_COLORSTR(B). { wInterface->ColorStr(B.m_Token); }
color::= F_COLORHASH(B). { wInterface->ColorHash(B.m_Token); }
color::= F_RGB F_LEFTPARENT F_INTEGER(R) F_COMMA F_INTEGER(G) F_COMMA F_INTEGER(B) F_RIGHTPARENT. { wInterface->ColorRgb(R.m_Token,G.m_Token,B.m_Token); }
color::= F_RGBA F_LEFTPARENT F_INTEGER(R) F_COMMA F_INTEGER(G) F_COMMA F_INTEGER(B) F_COMMA F_INTEGER(A) F_RIGHTPARENT. { wInterface->ColorRgba(R.m_Token,G.m_Token,B.m_Token,A.m_Token); }
color::= F_RGBA F_LEFTPARENT F_INTEGER(R) F_COMMA F_INTEGER(G) F_COMMA F_INTEGER(B) F_COMMA F_FLOAT(A) F_RIGHTPARENT. { wInterface->ColorRgba(R.m_Token,G.m_Token,B.m_Token,A.m_Token); }

css ::= F_FONT_NAME F_COLON fontName(B). { wInterface->FontName(B.m_Token); }
css ::= F_FONT_FAMILY F_COLON fontName(B) family. { wInterface->FontName(B.m_Token); }

css ::= fontBegin decFont. { wInterface->EndKind(); }
fontBegin ::= F_FONT(B).  { wInterface->BeginKind(B.m_Token); }

decFontHead ::= F_COLON fontName(B). { wInterface->FontName(B.m_Token); }

// Split tail list so we never reduce fonts ::= . when the lookahead begins a font token (italic, bold, 12pt, ...).
decFont ::= decFontHead family .
decFont ::= decFontHead family font fonts_opt .

fonts_opt ::= .
fonts_opt ::= font fonts_opt .

fontName(A)    ::= F_LABELDQ(B). { A=B; }
fontName(A)    ::= F_LABELSQ(B). { A=B; }

fontFamily(A)  ::= F_SERIF(B).  { A=B; }
fontFamily(A)  ::= F_SANS_SERIF(B). { A=B; }
fontFamily(A)  ::= F_MONOSPACE(B). { A=B; }
fontFamily(A)  ::= F_CURSIVE(B). { A=B; }
fontFamily(A)  ::= F_FANTASY(B). { A=B; }
fontFamily(A)  ::= F_SYSTEM_UI(B). { A=B; }
fontFamily(A)  ::= F_EMOJI(B). { A=B; }
fontFamily(A)  ::= F_MATH(B). { A=B; }
fontFamily(A)  ::= F_FANGSONG(B). { A=B; }

family ::= .
family ::= F_COMMA fontFamily(B). { wInterface->FontFamily(B.m_Token); }

font ::= unit.
font ::= unitEm.
// font shorthand tokens after family must apply to m_FontCss (longhand rules call Font* directly).
font ::= fontWeight(B). { wInterface->FontWeight(B.m_Token); }
font ::= fontStyle(B). { wInterface->FontStyle(B.m_Token); }
font ::= fontVariant.
font ::= fontStretch.

font ::= F_OBLIQUE F_INTEGER(B) F_DEG. { wInterface->FontStyle(B.m_Token); }

// Font Size ==================================================================
css ::= fontsize F_COLON unit. { wInterface->EndKind(); }
fontsize ::= F_FONT_SIZE(B).  { wInterface->BeginKind(B.m_Token); }


// Style ======================================================================
css ::= F_FONT_STYLE F_COLON fontStyle(B). { wInterface->FontStyle(B.m_Token); }
css ::= F_FONT_STYLE F_COLON F_NONE(B). { wInterface->FontStyle(B.m_Token); }

css ::= F_FONT_STYLE F_COLON F_OBLIQUE F_INTEGER(B) F_DEG. { wInterface->FontStyle(B.m_Token); }

fontStyle(A) ::= F_NORMAL(B). { A=B; }
fontStyle(A) ::= F_ITALIC(B). { A=B; }

css ::= F_WEIGHT F_COLON fontWeight(B). { wInterface->FontWeight(B.m_Token); }
// CONFLIC IF NOT USE THIS RULE
css ::= F_WEIGHT F_COLON F_NORMAL(B). { wInterface->FontWeight(B.m_Token); }
css ::= F_WEIGHT F_COLON F_NONE(B). { wInterface->FontWeight(B.m_Token); }

fontWeight(A) ::= F_BOLD(B). { A=B; }
fontWeight(A) ::= F_LIGHTER(B). { A=B; }
fontWeight(A) ::= F_BOLDER(B). { A=B; }
fontWeight(A) ::= F_INTEGER(B). { A=B; }
fontWeight(A) ::= F_FLOAT(B). { A=B; }

css ::= F_FONT_VARIANT F_COLON fontVariant.
css ::= F_FONT_VARIANT F_COLON F_NORMAL(B). {  wInterface->FontVariant(B.m_Token); }

fontVariant ::= F_SMALL_CAPS(B). {  wInterface->FontVariant(B.m_Token); }

css ::= F_FONT_STRETCH F_COLON fontStretch.
css ::= F_FONT_STRETCH F_COLON F_NORMAL(B). { wInterface->FontStretch(B.m_Token); }
css ::= F_FONT_STRETCH F_COLON F_INTEGER(B) F_PERCENT. { wInterface->FontStretch(B.m_Token); }
css ::= F_FONT_STRETCH F_COLON F_FLOAT(B) F_PERCENT. { wInterface->FontStretch(B.m_Token); }

fontStretch ::= F_ULTRA_CONDENSED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_CONDENSED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_SEMI_CONDENSED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_SEMI_EXPANDED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_EXPANDED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_EXTRA_EXPANDED(B). { wInterface->FontStretch(B.m_Token); }
fontStretch ::= F_ULTRA_EXPANDED(B). { wInterface->FontStretch(B.m_Token); }

css ::= F_LINE_HEIGHT F_COLON number(B). { wInterface->FontLineHeight(B.m_Token); }


// Text Align ==================================================================
css ::= F_TEXT_ALIGN F_COLON F_NONE(B). { wInterface->TextAlign(B.m_Token); }
css ::= F_TEXT_ALIGN F_COLON F_LEFT(B). { wInterface->TextAlign(B.m_Token); }
css ::= F_TEXT_ALIGN F_COLON F_RIGHT(B). { wInterface->TextAlign(B.m_Token); }
css ::= F_TEXT_ALIGN F_COLON F_CENTER(B). { wInterface->TextAlign(B.m_Token); }
css ::= F_TEXT_ALIGN F_COLON F_JUSTIFY(B). { wInterface->TextAlign(B.m_Token); }

// Vertical Align ==============================================================
css ::= F_VERTICAL_ALIGN F_COLON F_NONE(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_BASELINE(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_SUB(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_SUPER(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_TEXT_TOP(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_TEXT_BOTTOM(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_MIDDLE(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_TOP(B). { wInterface->VerticalAlign(B.m_Token); }
css ::= F_VERTICAL_ALIGN F_COLON F_BOTTOM(B). { wInterface->VerticalAlign(B.m_Token); }

// Text Decoration =============================================================
css ::= F_DECORATION_LINE F_COLON F_NONE(B). { wInterface->TextDecoration(B.m_Token); }
css ::= F_DECORATION_LINE F_COLON F_UNDERLINE(B). { wInterface->TextDecoration(B.m_Token); }
css ::= F_DECORATION_LINE F_COLON F_OVERLINE(B). { wInterface->TextDecoration(B.m_Token); }
css ::= F_DECORATION_LINE F_COLON F_LINE_THROUGH(B). { wInterface->TextDecoration(B.m_Token); }

// wrap 
css ::= F_TEXT_WRAP F_COLON F_WRAP(B). { wInterface->TextWrap(B.m_Token); } 
css ::= F_TEXT_WRAP F_COLON F_NO_WRAP(B). { wInterface->TextWrap(B.m_Token); } 

// rotate
css ::= F_TEXT_ROTATE F_COLON F_FLOAT(B). { wInterface->TextRotate(B.m_Token); }
css ::= F_TEXT_ROTATE F_COLON F_INTEGER(B). { wInterface->TextRotate(B.m_Token); }
// Border ======================================================================
css ::= border F_COLON borderComponents. { wInterface->EndKind(); }

borderComponent ::= unit.
borderComponent ::= color.
borderComponent ::= borderStyle.

borderComponents ::= .
borderComponents ::= borderComponent borderComponents.

border ::= F_BORDER(B). { wInterface->BeginKind(B.m_Token); }
border ::= F_BORDER_LEFT(B). { wInterface->BeginKind(B.m_Token); }
border ::= F_BORDER_TOP(B). { wInterface->BeginKind(B.m_Token); }
border ::= F_BORDER_RIGHT(B). { wInterface->BeginKind(B.m_Token); }
border ::= F_BORDER_BOTTOM(B). { wInterface->BeginKind(B.m_Token); }

borderStyle ::=  F_NONE(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_HIDDEN(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_DOTTED(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_DASHED(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_SOLID(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_DOUBLE(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_GROOVE(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_RIDGE(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_INSET(B). { wInterface->BorderStyle(B.m_Token); }
borderStyle ::=  F_OUTSET(B). { wInterface->BorderStyle(B.m_Token); }

css ::= borderRadius.

borderRadius ::= borderRadiusAll F_COLON unitValues. { wInterface->EndKind(); }
borderRadius ::= borderRadiusElem F_COLON unit. { wInterface->EndKind(); }

borderRadiusAll ::= F_BORDER_RADIUS(B). { wInterface->BeginKind(B.m_Token); }
borderRadiusElem ::= F_BORDER_TOP_LEFT_RADIUS(B). { wInterface->BeginKind(B.m_Token); }
borderRadiusElem ::= F_BORDER_TOP_RIGHT_RADIUS(B). { wInterface->BeginKind(B.m_Token); }
borderRadiusElem ::= F_BORDER_BOTTOM_LEFT_RADIUS(B). { wInterface->BeginKind(B.m_Token); }
borderRadiusElem ::= F_BORDER_BOTTOM_RIGHT_RADIUS(B). { wInterface->BeginKind(B.m_Token); }

// FORMAT STRING ===============================================================
css ::=  F_FORMATSTRING F_COLON F_LABELDQ(B).  {  wInterface->FormatString(B.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELDQ(B) F_INTEGER(C).  { wInterface->FormatString(B.m_Token,C.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELSQ(B).  {  wInterface->FormatString(B.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELSQ(B) F_INTEGER(C).  { wInterface->FormatString(B.m_Token,C.m_Token); }

css ::=  F_FORMATSTRING F_COLON F_LABELDQ(B) F_LABELDQ(C).  {  wInterface->FormatStringMoney(B.m_Token,C.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELDQ(B) F_LABELDQ(C) F_INTEGER(D).  { wInterface->FormatStringMoney(B.m_Token,C.m_Token,D.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELSQ(B) F_LABELSQ(C).  {  wInterface->FormatStringMoney(B.m_Token,C.m_Token); }
css ::=  F_FORMATSTRING F_COLON F_LABELSQ(B) F_LABELSQ(C) F_INTEGER(D).  { wInterface->FormatStringMoney(B.m_Token,C.m_Token,D.m_Token); }


// UNIT ========================================================================
unitValue ::= unit.

unitValues ::= .
unitValues ::= unitValue unitValues.

unit ::= F_INTEGER(B) F_PX. { wInterface->PushUnit(tKind::Px,B.m_Token); }
unit ::= F_INTEGER(B) F_IN. { wInterface->PushUnit(tKind::In,B.m_Token); }
unit ::= F_INTEGER(B) F_PT. { wInterface->PushUnit(tKind::Pt,B.m_Token); }
unit ::= F_INTEGER(B) F_PC. { wInterface->PushUnit(tKind::Pc,B.m_Token); }
unit ::= F_INTEGER(B) F_CM. { wInterface->PushUnit(tKind::Cm,B.m_Token); }
unit ::= F_INTEGER(B) F_MM. { wInterface->PushUnit(tKind::Mm,B.m_Token); }
unit ::= F_INTEGER(B) F_PERCENT. { wInterface->PushUnit(tKind::Percent,B.m_Token); }

unit ::= F_FLOAT(B) F_PX. { wInterface->PushUnit(tKind::Px,B.m_Token); }
unit ::= F_FLOAT(B) F_IN. { wInterface->PushUnit(tKind::In,B.m_Token); }
unit ::= F_FLOAT(B) F_PT. { wInterface->PushUnit(tKind::Pt,B.m_Token); }
unit ::= F_FLOAT(B) F_PC. { wInterface->PushUnit(tKind::Pc,B.m_Token); }
unit ::= F_FLOAT(B) F_CM. { wInterface->PushUnit(tKind::Cm,B.m_Token); }
unit ::= F_FLOAT(B) F_MM. { wInterface->PushUnit(tKind::Mm,B.m_Token); }
unit ::= F_FLOAT(B) F_PERCENT. { wInterface->PushUnit(tKind::Percent,B.m_Token); }

unitEm ::= F_INTEGER(B) F_EM. { wInterface->PushUnit(tKind::Em,B.m_Token); }
unitEm ::= F_FLOAT(B) F_EM. { wInterface->PushUnit(tKind::Em,B.m_Token); }

%include {  

// Cette déclaration externe est pour vous assurer que vous utilisez le parseur correctement.
typedef struct yyParser yyParser;



namespace SkFormat {
    void ParserInit(tLemonFormatInterface* sInterface) {
      sInterface->LemonParser(ParseAlloc (malloc));
#ifdef debugparser
      // Debug config
      ParseTrace(stdout, "Parser->");
#endif
    }

    void ParserDone(tLemonFormatInterface* sInterface) {
      ParseFree(sInterface->LemonParser(), free );
      sInterface->LemonParser(nullptr);
    }

    tBool ParserRun(tLemonFormatInterface* sInterface,const tChar* sCode) {
      // Token Lemon
      tToken wToken;

      sInterface->Clear();

      // Lexer
      tLexer wLex(sCode);

      // For parse Reserved Word & Color
      wGlobalInCss=false;
      wLex.InCss(wGlobalInCss);

      tLexerToken wLexerToken;
      // Loop until End or Unexpected caracter 
      for (;;) {
            wLex.InCss(wGlobalInCss);
            wLexerToken = wLex.next();
            if (wLexerToken.is_one_of(tKind::End, tKind::Unexpected))
                break;
            // Go to LEMON
            wToken.m_Token=new tLexerToken(wLexerToken);
            wToken.m_Column = wLex.Column();
            wToken.m_Line = wLex.Line();
            
            wGlobalLine=wLex.Line();
            wGlobalColumn=wLex.Column();

            tInt wCstLemon = 0;
            switch (wToken.m_Token->Kind()) {
            case tKind::Integer: wCstLemon = F_INTEGER; break;
            case tKind::Float: wCstLemon = F_FLOAT;  break;
            
            case tKind::LabelDouble: wCstLemon = F_LABELDQ; break; 
            case tKind::LabelSimple: wCstLemon = F_LABELSQ; break; 

            case tKind::Ampersand  : wCstLemon = F_AMPERSAND; break;
            case tKind::Exclamation: wCstLemon = F_EXCLAMATION; break;
           
            case tKind::Dot: wCstLemon = F_DOT; break;
            case tKind::Hash: wCstLemon = F_HASH; break;
            case tKind::Comma: wCstLemon = F_COMMA; break;
            case tKind::Colon: wCstLemon = F_COLON; break;
            case tKind::Semicolon: wCstLemon = F_SEMICOLON; break;
            case tKind::NewLine: wCstLemon = F_NEWLINE; break;
            case tKind::Percent : wCstLemon = F_PERCENT; break;
            case tKind::Comment: wCstLemon = F_COMMENT; break;
          
            case tKind::LeftCurly : wCstLemon = F_LEFTCURLY; break;
            case tKind::RightCurly : wCstLemon = F_RIGHTCURLY; break;
            case tKind::LeftParen : wCstLemon = F_LEFTPARENT; break;
            case tKind::RightParen : wCstLemon = F_RIGHTPARENT; break;
            case tKind::LeftSquare  : wCstLemon = F_LEFTSQUARE; break;
            case tKind::RightSquare  : wCstLemon = F_RIGHTSQUARE; break;

            case tKind::ColorHash    : wCstLemon = F_COLORHASH; break;
            
            case tKind::Identifier: {
                // Is reserved word and or not etc...
                if (wGlobalInCss) {
                  tString wResult = wToken.m_Token->Lexeme();
                  // std::transform(wResult.begin(), wResult.end(), wResult.begin(), ::toupper);
                  tInt wRes = sInterface->Id(wResult);
                  if (wRes != -1) {
                    // Reserved word
                    wCstLemon = wRes;
                    // Set Kind an token
                    tLemonFormatIdRef wLemonRef= sInterface->IdRef(wResult);
                    wToken.m_Token->Kind(wLemonRef.Kind());
                  } else {
                    /// Color
                    std::transform(wResult.begin(), wResult.end(), wResult.begin(), ::tolower);
                    const tRecColor* wRecColor=sInterface->Color(wResult);
                    if (wRecColor!=nullptr) {
                      wCstLemon=F_COLORSTR;
                    } else {
                      // Identifier
                      wCstLemon = F_ID;
                    } 
                } 
                } else {
                    // Identifier
                    wCstLemon = F_ID;
                }
                break;
            }
            default: {
                sInterface->CompilError(true);
                tStringStream wStream;
                wStream << "Bad character";
                sInterface->Error(wStream.str(),wGlobalLine,wGlobalColumn);
                break;
            }
            }
            
#ifdef debugparser
            std::cout << wGlobalColumn <<  " Lex -->" << wLexerToken  << " -> " << wCstLemon << endl;
#endif
            if ((wCstLemon!=F_NEWLINE) && (wCstLemon!=F_COMMENT)) {
                Parse(sInterface->LemonParser(), wCstLemon, wToken,sInterface);
                if (sInterface->CompilError()) { return(false); }
            }
      }
      // Force the end ===============================================================
      Parse (sInterface->LemonParser(), F_END, wToken,sInterface);

      // Is Unexpected caracter ?
      if (wLexerToken.Kind()==tKind::Unexpected) {
        sInterface->CompilError(true);
        tStringStream wStream;
        wStream << "Bad character";
        sInterface->Error(wStream.str(),wGlobalLine,wGlobalColumn);
      }
      Parse (sInterface->LemonParser(), 0, wToken,sInterface);
        
      return(!sInterface->CompilError());
    }
  } // End of namespace
  
}
