//=============================================================================
// SkLemnonSpreadSheet.y Parser Lemon SpreadSheet 
//=============================================================================
%include {   

#include "../include/SkLemonSpreadSheet.hpp"

#define _debugparser
#define _debuglex
using namespace SkSpreadSheet;

// Callback for allocator
void* ParseAlloc(void * (tSize));
void  ParseFree(void *,void  (void *));
// Cell & Range, position Lexer and position LemonParser ======================
tInt wIndLex=0;

// For Error ==================================================================
tInt wLexLine=0;
tInt wLexColumn =0;

// Prédefinition lemon ========================================================
void Parse(
  void *yyp,      /* The parser */
  int yymajor,    /* The major token code number */
  tToken yyminor,   /* The value for the token  see %token_type {tToken} */
  tLemonInterface* sFormula /* see %extra_argument { tLemonInterface *wInterface } */
);
// columnid
tLexerToken* wColumnIdToken=nullptr;

void ParseTrace(FILE* TraceFILE, char* zTracePrompt);

using namespace SkRoot;
  void token_destructor(tToken sToken) { (void)sToken; }
}  
// Define Token =======================
%token_type {tToken}
%default_type {tToken}
%token_destructor { token_destructor($$); }

%extra_argument { tLemonInterface *wInterface }

%type expr {tToken}

%type ID {tToken}
%type LABELDQ {tToken}
%type LABELSQ {tToken}
%type LABELSQUARE {tToken}

%type CELL {tToken}
%type ATTRIBUTE {tToken} 
%type ERRORREF {tToken}
%type ERRORNAME {tToken}

%type PERCENT { tToken }

// Attribute
%type attribute {tToken}
// Table
%type tableid {tToken}
%type columnid {tToken}
%type thisrowcol {tToken}
%token AT.
%token HASH.

// Left (for priority of operator)
// Lower precedence at top, higher precedence at bottom
%nonassoc EQUAL NOTEQUAL GREATER GREATEREQUAL LESS LESSEQUAL.
%left  AMPERSAND.
%left  PLUS MINUS.
%left  TIMES DIVIDE MOD.
// Excel postfix percent: 50% == 0.5. Tighter than * / so AJ3<50% is AJ3<(50%).
%left  PERCENT.

%left COMMA SEMICOLON.
%left PIPE.
%left LEFTSQUARE.
%left LEFTPARENT.
%left RIGHTSQUARE.
%left RIGHTPARENT.

%left LEFTCURLY.
%left RIGHTCURLY.

// Right (for priority of operator)
// Not by example

%parse_accept {
  // code for accept
}

// Callback Error =============================================================
%syntax_error {  
  wInterface->CompilError(tErrorFormula::t_SyntaxError);
  wInterface->Error("Syntax error !",wLexLine,wLexColumn);
}   
   
//  This is to terminate with a new line ======================================
main ::= in.
in ::= .
in ::= in state END. {}

state ::= expr.   {}  
//  Contitional format ========================================================
state ::= percent_prefix conditionalformat. 
percent_prefix ::= PERCENT. { wInterface->PushOp(tKind::Percent); }
conditionalformat ::= EQUAL        expr. { wInterface->PushOp(tKind::Equal);  }  
conditionalformat ::= NOTEQUAL     expr. { wInterface->PushOp(tKind::NotEqual); }  
conditionalformat ::= GREATER      expr. { wInterface->PushOp(tKind::GreaterThan); }  
conditionalformat ::= GREATEREQUAL expr. { wInterface->PushOp(tKind::GreaterThanOrEqual); }  
conditionalformat ::= LESS         expr. { wInterface->PushOp(tKind::LessThan); }  
conditionalformat ::= LESSEQUAL    expr. { wInterface->PushOp(tKind::LessThanOrEqual); }  

// Attribute 
attribute ::= ATTRIBUTE.  
// Empty attribute (Lemon does not define $$ for empty RHS; use named LHS (A))
attribute(A) ::= .  { A.m_Token = nullptr; }

/* Method (futur) =============================================================
methodBegin ::= ATTRIBUTE(B) LEFTPARENT. { wInterface->PushFunctionMethod(B.m_Token->Lexeme(),true);  }
methodEnd ::= exprList RIGHTPARENT. {
  tLemonFunctionMethod wLemonFunctionMethod(wInterface->PopFunctionMethod());
  wInterface->PushFunctionMethod(&wLemonFunctionMethod); 
}

attribute ::= methodBegin methodEnd.
*/

// tableid can be either LABELSQ, LABELDQ, LABELSQUARE or ID (for table and column names)
// Lemon automatically copies the value when no action is specified
tableid ::= ID.
tableid ::= LABELSQ.
tableid ::= LABELDQ.

table  ::= tableid(B) LABELSQUARE(C). { wInterface->PushTable(B.m_Token,C.m_Token); }

// Items
expr ::= INTEGER(B). { wInterface->PushInteger(B.m_Token); }
expr ::= FLOAT(B).   { wInterface->PushFloat(B.m_Token); }
expr ::= BOOL(B).   { wInterface->PushBool(B.m_Token); }

// Attribute Error #NAME? and #REEF!
// Error #Name?
expr ::= ERRORNAME(B) attribute. { wInterface->PushErrorName(B.m_Token); }
// Error #REF!
expr ::= ERRORREF(B) attribute. { wInterface->PushErrorRef(B.m_Token); } 

// Sheet!#REF! (invalid / deleted ref; Excel still parses e.g. OFFSET('Sheet'!#REF!,...))
expr ::= SHEET(B) ERRORREF(C) attribute(D). { wInterface->PushSheetErrorRef(B.m_Token,C.m_Token,D.m_Token); }


expr ::= ID(B) attribute(C). { wInterface->PushID(B.m_Token,C.m_Token); }

// Excel R1C1: MyNameR[-1]C[2] (named range + relative offset suffix).
expr ::= ID(B) CELL(C) attribute(D). {
  wInterface->PushNamedRangeR1C1OffsetParse(B.m_Token, C.m_Token, D.m_Token);
}

expr ::= SHEET(B) ID(C) attribute(D). { wInterface->PushSheetID(B.m_Token,C.m_Token,D.m_Token); }

// Sheet!MyNameR[-1]C[2]
expr ::= SHEET(B) ID(C) CELL(D) attribute(E). {
  wInterface->PushSheetNamedRangeR1C1OffsetParse(B.m_Token, C.m_Token, D.m_Token, E.m_Token);
}

// Label rules after table rules to avoid conflicts
expr ::= LABELSQ(B). { wInterface->PushLabel(B.m_Token); }
expr ::= LABELDQ(B). { wInterface->PushLabel(B.m_Token); }

// Table Data =========================================================
// example Table[[@This Row][Col1]:Col2]]
expr ::= table.

// [@[Col]] — full bracket form
expr ::= LABELSQUARE(B). { wInterface->PushTable(nullptr,B.m_Token); }

// Excel shorthand: @[Col] and [@Col] (#This Row), same as [@[Col]]
thisrowcol ::= AT LABELSQUARE(B). { wInterface->PushThisRowColumn(B.m_Token); }
thisrowcol ::= AT ID(B). { wInterface->PushThisRowColumn(B.m_Token); }
expr ::= thisrowcol.

// Cell
expr ::= CELL(B) attribute(C). {  wInterface->PushCell(B.m_Token,C.m_Token); }
// Sheet!Cell
expr ::= SHEET(B) CELL(C) attribute(D). {  wInterface->PushSheetCell(B.m_Token,C.m_Token,D.m_Token); }

// Excel spilled-range operator: D14# / Sheet!D14# (full dynamic-array footprint, else #REF!).
expr ::= CELL(B) HASH. { wInterface->PushSpillRef(B.m_Token); }
expr ::= SHEET(B) CELL(C) HASH. { wInterface->PushSheetSpillRef(B.m_Token, C.m_Token); }

// Range Cell:Cell
expr ::= CELL(B) COLON CELL(C). {  wInterface->PushRange(B.m_Token,C.m_Token); }
// Partial #REF! or named-range bounds (Excel: SUM(#REF!:MyRange), A1:MyRange, MyRange:A1)
expr ::= ERRORREF(B) COLON ID(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= ERRORREF(B) COLON CELL(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= ERRORREF(B) COLON ERRORREF(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= CELL(B) COLON ERRORREF(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= CELL(B) COLON ID(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= ID(B) COLON CELL(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= ID(B) COLON ID(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
expr ::= ID(B) COLON ERRORREF(C). { wInterface->PushRange(B.m_Token, C.m_Token); }
// Sheet!Cell:Cell
expr ::= SHEET(B) CELL(C) COLON CELL(D). { wInterface->PushSheetRange(B.m_Token,C.m_Token,D.m_Token); }

// Dynamic range: one bound from function result (no PushRange). Do not call SetCell here: the lexer already added the static cell ref when flushing (e.g. A1 before "(" in A1:INDEX(...)); calling SetCell again would duplicate the ref.
expr ::= CELL(B) COLON funcBegin funcEnd. { wInterface->PushDynamicRangeRight(B.m_Token); }
expr ::= funcBegin funcEnd COLON CELL(C). { wInterface->PushDynamicRangeLeft(C.m_Token); }


// Dynamic range: both bounds from function results (no PushRange, no SetCell)
expr ::= funcBegin funcEnd COLON funcBegin funcEnd. { wInterface->PushDynamicRangeBoth(); }

expr ::= table COLON funcBegin funcEnd. { wInterface->PushDynamicRangeBoth(); }
expr ::= funcBegin funcEnd COLON table. { wInterface->PushDynamicRangeBoth(); }
expr ::= table COLON table. { wInterface->PushDynamicRangeBoth(); }


expr ::= MINUS expr. {  wInterface->PushOp(tKind::UnaryMinus); }
expr ::= PLUS expr. { /* Excel unary plus: identity, no bytecode */ }
// Excel postfix percent (50% -> 0.5). Distinct from CF prefix `%>=10`.
expr ::= expr PERCENT. { wInterface->PushPercentLiteral(); }

expr ::= expr TIMES  expr. { wInterface->PushOp(tKind::Times); }  
expr ::= expr DIVIDE expr. { wInterface->PushOp(tKind::Divide); }  

expr ::= expr MINUS  expr. { wInterface->PushOp(tKind::Minus); }  
expr ::= expr PLUS   expr. { wInterface->PushOp(tKind::Plus); }  

expr ::= expr AMPERSAND expr. { wInterface->PushOp(tKind::Ampersand); }

expr ::= expr EQUAL        expr. { wInterface->PushOp(tKind::Equal); }  
expr ::= expr NOTEQUAL     expr. { wInterface->PushOp(tKind::NotEqual); }  
expr ::= expr GREATER      expr. { wInterface->PushOp(tKind::GreaterThan); }  
expr ::= expr GREATEREQUAL expr. { wInterface->PushOp(tKind::GreaterThanOrEqual); }  
expr ::= expr LESS         expr. { wInterface->PushOp(tKind::LessThan); }  
expr ::= expr LESSEQUAL    expr. { wInterface->PushOp(tKind::LessThanOrEqual); } 

expr ::= LEFTPARENT expr RIGHTPARENT. {} 

// Literal lists (Excel-style): comma = next column (same row); semicolon = next row.
// Separator is pushed before the following cell so bytecode order is value, sep, value, ...
// listcomma / listsemi: intermediate rules so we do not duplicate five cell types × two separators.
listcell ::= INTEGER(B). { wInterface->PushInteger(B.m_Token); }
listcell ::= FLOAT(B). { wInterface->PushFloat(B.m_Token); }
listcell ::= BOOL(B). { wInterface->PushBool(B.m_Token); }
listcell ::= LABELSQ(B). { wInterface->PushLabel(B.m_Token); }
listcell ::= LABELDQ(B). { wInterface->PushLabel(B.m_Token); }
// Signed numeric constants inside array literals (Excel: {-1,-1,-1,1}, {9,8,6,1}).
// A leading '+' is identity. Only literals are signed here (no expressions inside {}).
listcell ::= MINUS INTEGER(B). { wInterface->PushInteger(B.m_Token, true); }
listcell ::= MINUS FLOAT(B).   { wInterface->PushFloat(B.m_Token, true); }
listcell ::= PLUS INTEGER(B).  { wInterface->PushInteger(B.m_Token); }
listcell ::= PLUS FLOAT(B).    { wInterface->PushFloat(B.m_Token); }

listcomma ::= listliteral COMMA. { wInterface->PushOp(tKind::Comma); }
listsemi  ::= listliteral SEMICOLON. { wInterface->PushOp(tKind::Semicolon); }

listliteral ::= .
listliteral ::= listcell.
listliteral ::= listcomma listcell.
listliteral ::= listsemi listcell.

// Delimiters for literal arrays: push markers so InternalCalculation can build tRange (matrix spill).
openbrace ::= LEFTCURLY. { wInterface->PushOp(tKind::LeftCurly); }
openpipe  ::= PIPE.    { wInterface->PushOp(tKind::LeftCurly); }

expr ::= openbrace listliteral RIGHTCURLY. { wInterface->PushOp(tKind::RightCurly); }
expr ::= openpipe listliteral PIPE.        { wInterface->PushOp(tKind::RightCurly); }

// Function arguments: comma or semicolon (both accepted; matches Excel FR/EN habits).
argsep ::= COMMA.
argsep ::= SEMICOLON.
exprList ::= .
exprList ::= expr. {wInterface->IncArg(); }
exprList ::= exprList argsep expr. { wInterface->IncArg(); }
// Omitted argument (e.g. OFFSET(ref,,,h,w) — Excel defaults rows/cols to 0).
// PushEmpty BEFORE IncArg: IF short-circuit records ArgEndPos in IncArg, so the blank/0
// literal must already be in the bytecode or it lands in the next branch (Stack not empty).
exprList ::= exprList argsep. { wInterface->PushEmptyFunctionArg(); wInterface->IncArg(); }

funcBegin  ::= ID(B) LEFTPARENT. { wInterface->PushFunctionMethod(B.m_Token->Lexeme(),false); }  
funcEnd    ::= exprList RIGHTPARENT. { 
  tLemonFunctionMethod wLemonFunctionMethod(wInterface->PopFunctionMethod());
  wInterface->PushFunctionMethod(&wLemonFunctionMethod); 
}
 
// Function
expr ::= funcBegin funcEnd. {}

// LET(name1, value1, [name2, value2, ...], body) — Excel local named variables.
// Postfix emission (see tCell::InternalCalculation): LetBeginScope, <value1>, LetBind name1,
// <value2>, LetBind name2, ..., <body>, LetEndScope. Each value is evaluated once, in order, and a
// later value/body may reference an earlier name (PushID emits LetVarRef for a known LET local).
// The binding-name IDs are consumed directly by letBinding (never as expr), so they emit no ref and
// never reach PushID. Names were pre-collected by CollectLetNames so the key pass matches this pass.
letBegin ::= LET LEFTPARENT. { wInterface->LetBeginScope(); }
letBinding ::= ID(N) argsep expr. { wInterface->LetBind(N.m_Token->Lexeme()); }
letBindings ::= letBinding.
letBindings ::= letBindings argsep letBinding.
expr ::= letBegin letBindings argsep expr RIGHTPARENT. { wInterface->LetEndScope(); }

%include {  
namespace SkSpreadSheet {

    void ParserInit(tLemonInterface* sInterface) {
      sInterface->LemonParser(ParseAlloc (malloc));
#ifdef debugparser
      // Debug config
      ParseTrace(stdout, "Parser->");
#endif
    }

    void ParserDone(tLemonInterface* sInterface) {
      ParseFree(sInterface->LemonParser(), free );
      sInterface->LemonParser(nullptr);
    }

    tBool ParserRun(tLemonInterface* sInterface,const tChar* sCode) {
      // Debug Lemon
      //ParseTrace(stdout, "parser >>");

      // Token Lemon
      tToken wToken;

      wIndLex=0;
      
      sInterface->ClearCompil();
      // Create Lexer
      tLexer wLex(sCode);
      // Add Separator Decimal & Arg
      tLocale* wLocale=tApplication::Instance()->Locale();
      wLex.SeparatorArg(wLocale->Arg());
      wLex.SeparatorDecimal(wLocale->Decimal());
      tLexerToken wLexerToken;

      // Loop until End or Unexpected caracter 
      for (wLexerToken = wLex.next();!wLexerToken.is_one_of(tKind::End, tKind::Unexpected); wLexerToken = wLex.next()) {
            //std::cout << "Lex -->" << wLexerToken << endl;
            // Appel la fin
            wToken.m_Token=new tLexerToken(wLexerToken);
            wToken.m_Column = wLex.Column();
            wToken.m_Line = wLex.Line();
            wToken.m_Token->IndLex(wIndLex++);
            wLexColumn=wToken.m_Column;
            wLexLine=wToken.m_Line;

            tInt wCstLemon = 0;
            switch (wToken.m_Token->Kind()) {
            case tKind::Integer: wCstLemon = INTEGER; break;
            case tKind::Float: wCstLemon = FLOAT;  break;
            case tKind::Bool: wCstLemon = BOOL; break;
            case tKind::Plus: wCstLemon = PLUS; break;
            case tKind::Minus: wCstLemon = MINUS; break;
            case tKind::Times: wCstLemon = TIMES; break;
            case tKind::Divide: wCstLemon = DIVIDE; break;
            case tKind::Equal: wCstLemon = EQUAL; break;
            case tKind::NotEqual: wCstLemon = NOTEQUAL; break;
            case tKind::GreaterThan: wCstLemon = GREATER; break;
            case tKind::GreaterThanOrEqual: wCstLemon = GREATEREQUAL; break;
            case tKind::LessThan: wCstLemon = LESS; break;
            case tKind::LessThanOrEqual: wCstLemon = LESSEQUAL; break;
            case tKind::Sheet: {
              wCstLemon = SHEET;
              sInterface->SheetByToken(&wLexerToken);
              break;
            }
            case tKind::Cell: wCstLemon = CELL; break;
            case tKind::Hash: wCstLemon = HASH; break;
            
            case tKind::LeftParen : wCstLemon = LEFTPARENT; break;
            case tKind::RightParen : wCstLemon = RIGHTPARENT; break;
            case tKind::LeftSquare  : {
                  wCstLemon = LEFTSQUARE;  
                  break;
            }
            case tKind::RightSquare  : {
                  wCstLemon = RIGHTSQUARE; 
                  break;
            }
            case tKind::Identifier: {
                tString wResult = wToken.m_Token->Lexeme();
                if (wResult == "@") {
                  wCstLemon = AT;
                  break;
                }
                // Is reserved word and or not etc...
                tString wUppercase=wResult;
                std::transform(wResult.begin(), wResult.end(),wUppercase.begin(), ::toupper);
                tInt wRes = sInterface->Id(wUppercase);
                if (wRes != -1) {
                  // Function SpreadSheet
                  wCstLemon = wRes;
                } else if (wUppercase == "LET") {
                  // LET is parsed by a dedicated grammar rule (local scope), not the generic ID '(' path.
                  wCstLemon = LET;
                } else {
                  wCstLemon = ID;
                }
                break;
            }
            case tKind::LabelDouble: wCstLemon = LABELDQ; break; 
            case tKind::LabelSimple: wCstLemon = LABELSQ; break;
            case tKind::LabelSquare: wCstLemon = LABELSQUARE; break; 

            case tKind::LeftCurly: wCstLemon = LEFTCURLY; break;
            case tKind::RightCurly: wCstLemon = RIGHTCURLY; break;

            case tKind::ErrorRef: {
                wCstLemon = ERRORREF;
                break;
            }
            case tKind::ErrorName: {
                wCstLemon = ERRORNAME;
                break;
            } 

  
            case tKind::Ampersand  : wCstLemon = AMPERSAND; break;
            case tKind::Percent    : wCstLemon = PERCENT; break;
            //case tKind::Exclamation: wCstLemon = EXCLAMATION; break;
            case tKind::Attribute : wCstLemon = ATTRIBUTE; break;
            //case tKind::NewLine: wCstLemon = NEWLINE; break;
            case tKind::Colon: wCstLemon = COLON; break;
            case tKind::Comma: wCstLemon = COMMA; break;
            case tKind::Semicolon: wCstLemon = SEMICOLON; break;
            case tKind::Pipe: wCstLemon = PIPE; break;
            default: {
                sInterface->CompilError(tErrorFormula::t_SyntaxError);
                sInterface->Error("Syntax error !",wLexLine,wLexColumn);
                break;
            }
            }
          
#ifdef debuglex
           tLexerToken* wDebugLexerToken=wToken.m_Token;
           cout << wLexColumn <<  " Lex -->" << wDebugLexerToken->Kind() << ":" <<   wDebugLexerToken->Lexeme();
           cout << endl;
#endif
            Parse(sInterface->LemonParser(), wCstLemon, wToken,sInterface);
            if (sInterface->CompilError()!=tErrorFormula::t_None) { break; }
      }
      if (sInterface->CompilError() == tErrorFormula::t_None) {
        if (wLex.ArrayLiteralDepth() != 0 || wLex.InPipeLiteral()) {
          sInterface->CompilError(tErrorFormula::t_SyntaxError);
          sInterface->Error("Unbalanced array literal { } or | |", wLexLine, wLexColumn);
        }
      }
      Parse (sInterface->LemonParser(), END, wToken,sInterface);

      if (wLexerToken.Kind()==tKind::Unexpected) {
        sInterface->CompilError(tErrorFormula::t_SyntaxError);
        sInterface->Error("Caracter Invalid !",wLexLine,wLexColumn);
      }
      Parse(sInterface->LemonParser(), 0, wToken,sInterface);
    
      return(sInterface->CompilError()==tErrorFormula::t_None);
    }
  } // End of namespace
  
}