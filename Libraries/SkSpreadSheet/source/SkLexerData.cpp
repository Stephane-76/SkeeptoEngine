//=============================================================================
// SkLexerData.cpp  Lexer Excel data 
// Author Stéphane ALLEZ
// 11/02/2026
//=============================================================================

#include "../include/SkLexerData.hpp"

#define _debugparser

namespace SkSpreadSheet {

    tLexerData::tLexerData(tLexerToken* sTable,tChar sArg) : tClass(),
                            m_Table(sTable),
                            m_Arg(sArg),
                            m_SpecialKey(tTypeData::t_Column),
                            m_Col1(), m_Col2(),
                            m_NumCol1(-1),m_NumCol2(-1),
                            m_IndexCol1(-1),m_IndexCol2(-1) {
    }

    tLexerData::~tLexerData() {}

    tString tLexerData::Table() const {
        if (m_Table==nullptr) return("");
        tString wTable=m_Table->Lexeme();
        if ((m_Table->Kind()==tKind::LabelSimple) || (m_Table->Kind()==tKind::LabelDouble)) {
           tChar wSep='\'';
            if (m_Table->Kind()==tKind::LabelDouble) wSep='"';
            wTable = tString(1, wSep) + wTable + tString(1, wSep);
        }
        return(wTable);
    }
    tTypeData tLexerData::SpecialKey() const { return m_SpecialKey; }
    
    tString tLexerData::Col1() const { return m_Col1; }
    tString tLexerData::Col2() const { return m_Col2; }

    tIndex tLexerData::NumCol1() { return m_NumCol1; }
    void tLexerData::NumCol1(tIndex sIndex) { m_NumCol1 = sIndex; }

    tIndex tLexerData::NumCol2() { return m_NumCol2; }
    void tLexerData::NumCol2(tIndex sIndex) { m_NumCol2 = sIndex; }

    tString tLexerData::TokenFormula() {
        m_FormulaKey.push_back("$");
        tString wResult="";
        tIndex wIndex=0;
        for(auto wOp : m_FormulaKey) {
            if ((wIndex==m_IndexCol1) || (wIndex==m_IndexCol2)) {
                tStringStream wStream;
                if (wIndex==m_IndexCol1) { wStream << m_NumCol1; }
                else { wStream << m_NumCol2; }
                tClassString wOpStr(wOp);
                const tBool wBracketCol = wOpStr.StartsWith("[")
                    || ((wOp == "c1" || wOp == "c2") && m_SpecialKey == tTypeData::t_ThisRow);
                if (wBracketCol) {
                    wOp="["+wStream.str()+"]";
                } else {
                    wOp=wStream.str();
                }
            }
            wIndex++;
            wResult+=wOp;
        }
        return(wResult);
    }

    tBool tLexerData::Parse(tLexerToken* sLexerToken) {
        // Mark Begin
        if (m_Table==nullptr) {
            m_FormulaKey.push_back("name");
        }
         m_FormulaKey.push_back("$");
        // Parse label square data from current lexer position (e.g. after LabelSquare token)
        tString wCode=sLexerToken->Lexeme();
        tLexer wLexer(wCode.c_str());
    #ifdef debugparser
         cout << "Parse Data  Col 1 -" <<sLexerToken->Lexeme()<< endl;
    #endif
        // Case Table[Col A] or Excel [@Col] (#This Row) without inner brackets
        if (tClassString(wCode).CountChar(']')==0) {
            tClassString wLexeme(wCode);
            if (wLexeme().length() > 1 && wLexeme()[0] == '@') {
                m_SpecialKey = tTypeData::t_ThisRow;
                m_FormulaKey.push_back("[r]");
                m_Col1 = wLexeme().substr(1);
                tClassString wCol1(m_Col1);
                m_Col1 = wCol1.Unquote();
                m_IndexCol1 = m_FormulaKey.size();
                m_FormulaKey.push_back("[c1]");
                return(true);
            }
            m_Col1=wCode;
            m_IndexCol1=m_FormulaKey.size();
            m_FormulaKey.push_back("c1");
            return(true);
        }

        tLexerToken wLexerToken=wLexer.next();
         // Loop until End or Unexpected caracter
        while (!wLexerToken.is_one_of(tKind::End, tKind::Unexpected)) {
            tString wTokenLex=wLexerToken.Lexeme();
            switch (wLexerToken.Kind()) {
    #ifdef debugparser
                    cout << "   Column ID -" << wLexerToken << endl;
    #endif
                // Identifier ==================================================
                case tKind::Identifier : {
                  tClassString wLexeme(wLexerToken.Lexeme());
                  // Table[@[Col1...  at this state @[Col1]
                  if (wLexeme().length()==1) {
                      if (wLexeme()=="@") {
                        // if Col1!="" error syntax
                        if (m_Col1!="") return(false);
                        m_SpecialKey=tTypeData::t_ThisRow;
                        m_FormulaKey.push_back("[r]");
                        break;
                      }
                  }
                 if (m_Col1=="") {
                    m_Col1=wLexeme.Unquote();
                    m_IndexCol1=m_FormulaKey.size();
                    m_FormulaKey.push_back("c1");
                    break;
                  } else  {
                    if (m_Col2!="") return(false);
                    m_Col2=wLexeme.Unquote();
                    m_IndexCol2=m_FormulaKey.size();
                    m_FormulaKey.push_back("c2");
                    break;
                  }
                  return(false);
                }
                // Label Square ===============================================
                case tKind::LabelSquare: {
    #ifdef debugparser
                    cout << "   Column LABELSQUARE -" << wLexerToken << endl;
    #endif
                    tBool wDataType=false;
                    tClassString wLexeme(wLexerToken.Lexeme());
                    if (wLexeme()=="#This Row") {
                        m_SpecialKey=tTypeData::t_ThisRow;
                        m_FormulaKey.push_back("[r]");
                        wDataType=true;
                    }
                    if (wLexeme()=="#Headers") {
                        m_SpecialKey=tTypeData::t_Headers;
                         m_FormulaKey.push_back("[h]");
                        wDataType=true;
                    }
                     if (wLexeme()=="#Totals") {
                        // Must be t_Totals: t_Headers would resolve [[#Totals],[Col]] to the header row (e.g. #VALUE! on C6-C7).
                        m_SpecialKey=tTypeData::t_Totals;
                        m_FormulaKey.push_back("[t]");
                        wDataType=true;
                    }
                    if (wLexeme()=="#All") {
                        m_SpecialKey=tTypeData::t_All;
                        m_FormulaKey.push_back("[a]");
                        wDataType=true;
                    }
                    if ((wDataType) && (m_Col1!="")) return(false);
                    if (!wDataType) {
                      if (m_Col1=="") {
                        m_Col1=wLexeme.Unquote();
                        m_IndexCol1=m_FormulaKey.size();
                        m_FormulaKey.push_back("[c1]");
                        break;
                      } else  {
                        if (m_Col2!="") return(false);
                        m_Col2=wLexeme.Unquote();
                        m_IndexCol2=m_FormulaKey.size();
                        m_FormulaKey.push_back("[c2]");
                        break;
                      }
                    } else {
                        break;
                    }  // DataType
                    
                    return(false);
                }
                case tKind::Comma :
                    m_FormulaKey.push_back(",");
                    break;
                case tKind::Semicolon: {
                    m_FormulaKey.push_back(";");
                    break;
                }
                case tKind::Colon: {
                    m_FormulaKey.push_back(":");
                    break;
                }
                
                default: {
                    // Arg
                    if (wTokenLex.length()==1) {
                        tChar wChar=wTokenLex[0];
                        if (wChar==m_Arg) {
                            // Internal US  (Important)
                            wTokenLex=',';
                        }
                    } else {
        #ifdef debugparser
                        cout << " Column error -" << wLexerToken << endl;
        #endif
                        return(false);
                     }
                     break;
                 }
            }
            wLexerToken=wLexer.next();
        }
        return(true);
    }

}
