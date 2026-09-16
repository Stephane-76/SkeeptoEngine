//=============================================================================
// SkSpreadSheet Selection Range and Cell
//=============================================================================
#include "../include/SkSelect.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkCopyPaste.hpp"

namespace SkSpreadSheet {
	//=========================================================================
	tSelect::tSelect()  {};
	tSelect::~tSelect() {
		Clear();
	};

	void tSelect::Clear() {
		m_VectorSelect.clear();
	}

    const tString tSelect::StrRef() const {
        tString wResult="";
        tBool wIsFirst=true;
        for (auto wElem : m_VectorSelect) {
            if (!wIsFirst) wResult+=";";
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
            if (wRect != nullptr) {
                wResult+=wRect->StrRef();
            } else {
                wResult+=wElem->StrRef();
            }
            wIsFirst=false;
        }
        return(wResult);
    }

    tBool tSelect::Parse(tString sSelection) {
		Clear();
		// Token Lemon
		tToken wToken;
		tStackLexerToken wStackCell;

		tLexer wLex(sSelection.c_str());
		tLexerToken wLexerToken;
        
		// Loop until End or Unexpected caracter
		for (wLexerToken = wLex.next(); !wLexerToken.is_one_of(tKind::End, tKind::Unexpected); wLexerToken = wLex.next()) {
			//std::cout << "Lex -->" << wLexerToken << endl;

			wToken.m_Token = new tLexerToken(wLexerToken);
			wToken.m_Column = wLex.Column();
			wToken.m_Line = wLex.Line();

			switch (wToken.m_Token->Kind()) {
            case tKind::Identifier: wStackCell.push(wToken.m_Token); break;
			case tKind::Cell: wStackCell.push(wToken.m_Token); break;
			case tKind::Colon: break;
			case tKind::Semicolon: { // ;
				if (wStackCell.size() == 1) {
					tLexerToken* wLexerToken = wStackCell.top(); wStackCell.pop();
                    if (wLexerToken->Kind()==tKind::Cell) {
                        m_VectorSelect.push_back(new tTempoPoint(wLexerToken->RowInt(), wLexerToken->ColInt()));
                    } else {
                        m_VectorSelect.push_back(new tTempoPoint(wLexerToken->Lexeme()));
                    }
				} else
				if (wStackCell.size() == 2) {
					tLexerToken* wLexerTokenBottom = wStackCell.top(); wStackCell.pop();
					tLexerToken* wLexerTokenTop = wStackCell.top(); wStackCell.pop();
					m_VectorSelect.push_back(new tTempoRect(wLexerTokenTop->RowInt(), wLexerTokenTop->ColInt(), wLexerTokenBottom->RowInt(), wLexerTokenBottom->ColInt()));
				}
				else
					return(false);
				break;
			}
			default: {
				return(false);
			}
			}
		}

		if (wStackCell.size() > 0) {
			if (wStackCell.size() == 1) {
				tLexerToken* wLexerToken = wStackCell.top(); wStackCell.pop();
				m_VectorSelect.push_back(new tTempoPoint(wLexerToken->RowInt(), wLexerToken->ColInt()));
			}
			else
				if (wStackCell.size() == 2) {
					tLexerToken* wLexerTokenBottom = wStackCell.top(); wStackCell.pop();
					tLexerToken* wLexerTokenTop = wStackCell.top(); wStackCell.pop();
					m_VectorSelect.push_back(new tTempoRect(wLexerTokenTop->RowInt(), wLexerTokenTop->ColInt(), wLexerTokenBottom->RowInt(), wLexerTokenBottom->ColInt()));
				}
				else
					return(false);
		}
		//Debug();
		return(true);
	}

	tBool tSelect::CallBack(tUndoSpreadSheetCallBack* sUndoSpreadSheet) {
		tBool wResult = true;
        for (auto wElem : m_VectorSelect) {
            tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
            if (wRect != nullptr) {
                wResult=sUndoSpreadSheet->CallBackRange(wRect);
                if (!wResult) return(false);
            } else {
                wResult=sUndoSpreadSheet->CallBackCell(wElem);
                if (!wResult) return(false);
            }
        }
		return(true);
	}

	tBool tSelect::CallBack(tCopy* sCopy) {
		tBool wResult = true;
		for (auto wElem : m_VectorSelect) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
			if (wRect != nullptr) {
				wResult = sCopy->CallBackRange(wRect);
				if (!wResult) return(false);
			}
			else {
				wResult = sCopy->CallBackCell(wElem);
				if (!wResult) return(false);
			}
		}
		return(true);
	};

	tBool tSelect::JustOneCell() const {
		if (m_VectorSelect.size() == 1) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(m_VectorSelect[0]);
			if (wRect == nullptr) return(true); // if not rect is cell
		}
		return(false);
	}

	tBool tSelect::JustOneRect() const {
		if (m_VectorSelect.size() == 1) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(m_VectorSelect[0]);
			if (wRect != nullptr) return(true); // is rect 
		}
		return(false);
	}

	tTempoPoint* tSelect::FirstPoint() const {
		if (m_VectorSelect.size() == 1) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(m_VectorSelect[0]);
			if (wRect == nullptr) {
				return(m_VectorSelect[0]);
			}
		}
		return(nullptr);
	}

	tTempoRect* tSelect::FirstRect() const {
		if (m_VectorSelect.size() == 1) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(m_VectorSelect[0]);
			return(wRect); 
		}
		return(nullptr);
	}

	tVectorTempoPoint* tSelect::VectorSelect() const { 
		// Return non-const pointer to allow iteration (vector itself is not modified)
		return const_cast<tVectorTempoPoint*>(&m_VectorSelect); 
	}

	tTempoPoint* tSelect::ReturnTopLeftPoint() {
		tTempoPoint* wTempoPoint = nullptr;
		for (auto wElem : m_VectorSelect) {
            if (wElem!=nullptr) {
                tInt wRow = 0;
                tInt wCol = 0;
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
                if (wRect != nullptr) {
                    wRow = wRect->Top();
                    wCol = wRect->Left();
                } else {
                    tTempoPoint* wPoint= dynamic_cast<tTempoPoint*>(wElem);
                    wRow = wPoint->Row();
                    wCol = wPoint->Col();
                }
                if (wTempoPoint == nullptr) {
                    wTempoPoint = new tTempoPoint(wRow, wCol);
                }
                else {
                    if (wRow < wTempoPoint->Row()) wTempoPoint->Row(wRow);
                    if (wCol < wTempoPoint->Col()) wTempoPoint->Col(wCol);
                }
            }
		}
		return(wTempoPoint);
	}
	// Rebase ===============================================================
	tBool tSelect::Rebase(const tRebasePlan& sRebasePlan) {
		// Rebase all points and ranges in the selection
		for (auto wElem : m_VectorSelect) {
			if (wElem == nullptr) {
				continue;
			}
			
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
			if (wRect != nullptr) {
				// Rebase rectangle
				tRect wTempRect(*wRect);
				auto wRebasedRect = sRebasePlan.RebaseRect(wTempRect);
				if (!wRebasedRect.has_value()) {
					// Rectangle was deleted or became invalid
					return false;
				}
				
				// Update rectangle coordinates
				wRect->Top(wRebasedRect.value().Top());
				wRect->Left(wRebasedRect.value().Left());
				wRect->Bottom(wRebasedRect.value().Bottom());
				wRect->Right(wRebasedRect.value().Right());
			} else {
				// Rebase point
				tTempoPoint* wPoint = dynamic_cast<tTempoPoint*>(wElem);
				if (wPoint != nullptr) {
					tPoint wTempPoint(*wPoint);
					auto wRebasedPoint = sRebasePlan.RebasePoint(wTempPoint);
					if (!wRebasedPoint.has_value()) {
						// Point was deleted
						return false;
					}
					
					// Update point coordinates
					wPoint->Row(wRebasedPoint.value().Row());
					wPoint->Col(wRebasedPoint.value().Col());
				}
			}
		}
		
		return true;
	}

	// Json ===============================================================
	void tSelect::Json(Writer<StringBuffer>* sWriter) {
		for (auto wElem : m_VectorSelect) {
            if (wElem!=nullptr) {
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
                if (wRect != nullptr) {
                   wRect->Json(sWriter);
                }
                else {
                   wElem->Json(sWriter);
                }
            }
		}
	}

	void tSelect::Json(const Value& sValue) {
		m_VectorSelect.clear();
		assert(sValue.IsArray());
		for (SizeType wIndex = 0; wIndex < sValue.Size(); wIndex++) {
			const Value& wObj = sValue[wIndex];
			if (wObj.HasMember("r")) {
				tTempoRect* wRect = new tTempoRect;
				wRect->Json(const_cast<Value&>(wObj));
				m_VectorSelect.push_back(wRect);
			}
			else {
				tTempoPoint* wTempoPoint = new tTempoPoint;
				wTempoPoint->Json(const_cast<Value&>(wObj));
				m_VectorSelect.push_back(wTempoPoint);
			}
		}
	}

#ifdef _DEBUGSK
	tString tSelect::Debug() {
        tStringStream wStream;
		wStream << "tTempoSelect : ";
		for (auto wElem : m_VectorSelect) {
			tTempoRect* wRect = dynamic_cast<tTempoRect*>(wElem);
			if (wRect != nullptr) {
				wStream << Base10ToAlpha(wRect->Col()) << wRect->Row() << ":" << Base10ToAlpha(wRect->Right()) << wRect->Bottom() << ";";
			} else {
				wStream << Base10ToAlpha(wElem->Col()) << wElem->Row() << ";";
			}
		}
		wStream << endl;
        return(wStream.str());
	}
#endif

}
