//=============================================================================
// SkSpreadSheet Tools
//=============================================================================
#include "../include/SkTools.hpp"
#include <SkApplication.hpp>
#include "../include/SkUndoRedoJsonCallBack.hpp"
#include <cctype>
namespace SkSpreadSheet {

	tString Base10ToAlpha(tIndex sValue) {
		tString wReturn = "";
		// First column of Sheet
		if (sValue == 0) { return("@"); }
		sValue--;

		// Divide by 26 and get modulo for value of caracter
		tIndex wRest = sValue / 26;
		while (wRest > 0) {
			wRest--;
			// ABCDEFGHIJKLMNOPQRSTUVWXYZ
			tIndex wChar = (wRest % 26);
			wReturn = (tChar)(65 + wChar) + wReturn;
			wRest = wRest / 26;
		}
		tIndex wChar = (sValue % 26);
		wReturn += (tChar)(65 + wChar) + wRest;
		return(wReturn);
	}

	tIndex AlphaToBase10(tString sValue) {
		tIndex wRange = 1;
		tIndex wResult = 0;
		// 0 return @ for first column of Sheet
		if (sValue == "@") { return(wResult); }
		while (!sValue.empty()) {
			tSize wLength = sValue.length();
			tChar wVal = (tChar)sValue[wLength - 1] - 64; // from (A..Z) A=65  
			sValue.erase(wLength - 1, 1);
			if (wRange > 1) {
				wResult += tIndex(wVal * pow(26, wRange - 1));
			}
			else {
				wResult += wVal;
			}
			wRange++;
		}
		return(wResult);
	}

	tString SheetNameForFormula(tString sSheetName) {
		if (sSheetName.empty()) return sSheetName;
		// Quote only when an ASCII character requires it (e.g. space); non-ASCII (UTF-8) is left unquoted to match lexer
		tBool wNeedQuote = false;
		for (unsigned char c : sSheetName) {
			if (c < 0x80) {
				// ASCII: allow letters, digits, underscore only
				if (c != '_' && !std::isalnum(c)) {
					wNeedQuote = true;
					break;
				}
			}
			// Non-ASCII (UTF-8): do not require quoting
		}
		if (!wNeedQuote) return sSheetName;
		// Wrap in single quotes; escape any single quote by doubling it
		tString wResult;
		wResult.reserve(sSheetName.size() + 4);
		wResult += '\'';
		for (tChar c : sSheetName) {
			if (c == '\'') wResult += '\'';
			wResult += c;
		}
		wResult += '\'';
		return wResult;
	}

    tBool ParseRange(tString sRef, tIndex& sTop, tIndex& sLeft, tIndex& sBottom, tIndex& sRight) {
        tSelect wSelect;
        tBool wResult = wSelect.Parse(sRef);
        if (wResult) {
            if (wSelect.VectorSelect()->size() != 1) return(false);
            tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
            for (auto wItem : *wVectorSelect) {
                tTempoRect* wRect = dynamic_cast<tTempoRect*>(wItem);
                if (wRect != nullptr) {
                    sTop = wRect->Top();
                    sLeft = wRect->Left();
                    sBottom = wRect->Bottom();
                    sRight = wRect->Right();
                    return(true);
                }
                tTempoPoint* wPoint = dynamic_cast<tTempoPoint*>(wItem);
                if (wPoint!=nullptr) {
                    sTop = wPoint->Row();
                    sLeft = wPoint->Col();
                    sBottom = wPoint->Row();
                    sRight = wPoint->Col();
                    return(true);
                }
            }
            
        }
        return(false);
    }

    tBool ParseCell(tString sRef, tIndex& sRow, tIndex& sCol) {
        tSelect wSelect;
        tBool wResult = wSelect.Parse(sRef);
        if (wResult) {
            if (wSelect.VectorSelect()->size() != 1) return(false);
            tVectorTempoPoint* wVectorSelect = wSelect.VectorSelect();
            for (auto wItem : *wVectorSelect) {
                tTempoPoint* wPoint = dynamic_cast<tTempoPoint*>(wItem);
                if (wPoint != nullptr) {
                    sRow = wPoint->Row();
                    sCol = wPoint->Col();
                    return(true);
                }
            }
        }
        return(false);
    }

	//=========================================================================
//! Sorted range by operator < on SkPoint
	class tComparatorPoint {
	public:
		/// @brief      Operator() compare with SkPoint < operator.
		/// @param[in]  sE2 SkPoint*
		/// @param[in]  sE1 SkPoint*
		SkInline bool operator()(tPoint* sE1, tPoint* sE2) {
			return(*sE1 < *sE2);
		}
	};

    
	//=========================================================================
	//! Select Point by integer
	tPoint::tPoint() : m_Row(-1), m_Col(-1) {}
	tPoint::tPoint(tIndex sRow, tIndex sCol) : m_Row(sRow), m_Col(sCol) {}

	tPoint::tPoint(const tPoint& sPoint) : m_Row(sPoint.m_Row), m_Col(sPoint.m_Col) {}
    tPoint::tPoint(tTempoPoint& sTempoPoint) : m_Row(sTempoPoint.Row()), m_Col(sTempoPoint.Col()) {}
	tPoint::~tPoint() {}

	void tPoint::Row(tIndex sRow) { m_Row = sRow; }
	tIndex tPoint::Row() const { return(m_Row); }

	void tPoint::Col(tIndex sCol) { m_Col = sCol; }
	tIndex tPoint::Col() const { return(m_Col); }

	tString tPoint::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row;
		return(wStream.str());
	}

	tBool tPoint::operator < (const tPoint& sPoint) const {
		if (m_Row == sPoint.m_Row) {
			return(m_Col < sPoint.m_Col);
		}
		return(m_Row < sPoint.m_Row);
	}

	tBool tPoint::operator == (const tPoint& sPoint) const {
		return((m_Row == sPoint.m_Row) && (m_Col == sPoint.m_Col));
	}

	//=========================================================================
	//! Sorted range by operator < on SkPoint
	class tComparatorTempPoint {
	public:
		/// @brief      Operator() compare with SkPoint < operator.
		/// @param[in]  sE2 SkPoint*
		/// @param[in]  sE1 SkPoint*
		SkInline bool operator()(tTempoPoint* sE1, tTempoPoint* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//=========================================================================
	//! Select Point by integer
	tTempoPoint::tTempoPoint() : m_Row(-1), m_Col(-1),m_Name() {}
	tTempoPoint::tTempoPoint(tIndex sRow, tIndex sCol) : m_Row(sRow), m_Col(sCol),m_Name() {}


      // Parse a cell reference like "A1", "ZE123", etc.
    tTempoPoint::tTempoPoint(tString sRef) : m_Row(-1), m_Col(-1), m_Name() {
        ParseRef(sRef);
    }


	tTempoPoint::tTempoPoint(const tTempoPoint& sPoint) : m_Row(sPoint.m_Row), m_Col(sPoint.m_Col) {}

    tTempoPoint::tTempoPoint(tPoint& sPoint) : m_Row(sPoint.Row()),m_Col(sPoint.Col()) {  }
	
    tTempoPoint::~tTempoPoint() {}

	void tTempoPoint::Row(tIndex sRow) { m_Row = sRow; };
	tIndex tTempoPoint::Row() { return(m_Row); }

	void tTempoPoint::Col(tIndex sCol) { m_Col = sCol; };
	tIndex tTempoPoint::Col() { return(m_Col); }

    void tTempoPoint::Name(tString sName) { m_Name = sName; };
    tString tTempoPoint::Name() { return(m_Name); }

	tBool tTempoPoint::ParseRef(tString sRef) {  
		tString wCol;
        tString wRow;
        bool wFoundDigit = false;
        
        // Loop through each character in the reference
        for (tChar c : sRef) {
            // Check if character is a valid column letter (A-Z)
            if (c >= '@' && c <= 'Z') {
                if (!wFoundDigit) {
                    wCol += c;
                } else {
                    // If a letter appears after digits, it's invalid
                    return(false);
                }
            } else if (std::isdigit(c)) {
                // If it's a digit, add to row part
                wFoundDigit = true;
                wRow += c;
            } else {
                // Ignore any other character (optional: handle error)
                break;
            }
        }
        
        if (!wCol.empty() && !wRow.empty()) {
            m_Col = AlphaToBase10(wCol);
            m_Row = std::stoi(wRow);
        }
		return(true);
	}

	tString tTempoPoint::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row;
		return(wStream.str());
	}

    void tTempoPoint::Translate(tTempoPoint* sTempoPoint) {
        m_Row+=sTempoPoint->m_Row;
        m_Col+=sTempoPoint->m_Col;
    }

    tBool tTempoPoint::CallBack(tUndoRedoJsonCallBack* sUndoRedoJson) {
        return(sUndoRedoJson->CallBackCell(this));
    }


	void tTempoPoint::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("p");
		sWriter->StartArray();
		sWriter->Int(m_Row);
		sWriter->Int(m_Col);
		sWriter->EndArray();
		sWriter->EndObject();
	}

	void tTempoPoint::Json(const Value& sValue) {
		// Extract row and column from the JSON array
		const Value& wArray = sValue["p"];
		if (wArray.IsArray() && wArray.Size() >= 2) {
			m_Row = wArray[0].GetInt();
			m_Col = wArray[1].GetInt();
		}
	}


	void* tTempoPoint::operator new(tSize sz) noexcept {
		return(tApplication::Instance()->AllocTemporary(sz));
	}
	void tTempoPoint::operator delete(void* ptr) noexcept {}

	tBool tTempoPoint::operator < (const tTempoPoint sPoint) {
		if (m_Row == sPoint.m_Row) {
			return(m_Col < sPoint.m_Col);
		}
		return(m_Row < sPoint.m_Row);
	}

	tBool tTempoPoint::operator == (const tTempoPoint sPoint) {
		return((m_Row == sPoint.m_Row) && (m_Col == sPoint.m_Col));
	}
	//=========================================================================
	//! Sorted range by operator < on SkRect
	class SkComparatorRect {
	public:
		/// @brief      Operator() compare with SkRect < operator.
		/// @param[in]  sE2 SkRect*
		/// @param[in]  sE1 SkRect*
		SkInline bool operator()(tRect* sE1, tRect* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//=========================================================================
	//! Select Range by integer
	tRect::tRect() : m_Bottom(-1), m_Right(-1) {}
	tRect::tRect(tIndex sTopRow, tIndex sTopCol, tIndex sBottomRow, tIndex sRightCol) : tPoint(sTopRow, sTopCol), m_Bottom(sBottomRow), m_Right(sRightCol) {}
	tRect::tRect(const tRect& sRect) : tPoint(sRect), m_Bottom(sRect.m_Bottom), m_Right(sRect.m_Right) {}

    tRect::tRect(const tString sRect) {
        ParseRange(sRect,m_Row,m_Col,m_Bottom,m_Right);
    }

    tRect::tRect(tTempoRect& sTempoRect) : tPoint(sTempoRect), m_Bottom(sTempoRect.Bottom()), m_Right(sTempoRect.Right()) {} ;

	tRect::~tRect() {	};

    void tRect::Set(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) {
        m_Row=sTop;
        m_Col=sLeft;
        m_Bottom=sBottom;
        m_Right=sRight;
    }

	tIndex tRect::Top() { return(m_Row); }
	void tRect::Top(tIndex sValue) { m_Row = sValue; }

	tIndex tRect::Left() { return(m_Col); }
	void tRect::Left(tIndex sValue) { m_Col = sValue; }

	tIndex tRect::Bottom() { return(m_Bottom); }
	void tRect::Bottom(tIndex sValue) { m_Bottom = sValue; }

	tIndex tRect::Right() { return(m_Right); }
	void tRect::Right(tIndex sValue) { m_Right = sValue; }

    tInt tRect::Width() { return(m_Right-m_Col+1); }
    tInt tRect::Height() { return(m_Bottom-m_Row+1); }

	tBool tRect::IsValid() const {
		return m_Row >= 0 && m_Col >= 0 && m_Bottom >= 0 && m_Right >= 0
			&& m_Row <= m_Bottom && m_Col <= m_Right;
	}

	tString tRect::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row << ":" << Base10ToAlpha(m_Right) << m_Bottom;
		return(wStream.str());
	}


    void tTempoRect::Translate(tTempoPoint* sTempoPoint) {
        tTempoPoint::Translate(sTempoPoint);
        m_Bottom+=sTempoPoint->Row();
        m_Right+=sTempoPoint->Col();
    }

    tBool tRect::IsColSelect() {
        return((m_Row==0) && (m_Bottom==Cst_MaxRow));
    }

    tBool tRect::IsRowSelect() {
        return((m_Col==0) && (m_Right==Cst_MaxCol));
    }

    tBool tRect::IsSheetSelect() {
        return((m_Col==0) && (m_Right==Cst_MaxCol) && (m_Row==0) && (m_Bottom==Cst_MaxRow));
    }

	void tRect::RowSelect(tIndex sIndex) {
		m_Row = sIndex;
		m_Bottom = sIndex;
		m_Col = 0;
		m_Right = Cst_MaxCol;
	}

	void tRect::ColSelect(tIndex sIndex) {
		m_Col = sIndex;
		m_Right = sIndex;
		m_Row = 0;
		m_Bottom = Cst_MaxRow;
	}

	void tRect::SheetSelect() {
		m_Col = 0;
		m_Right = Cst_MaxCol;
		m_Row = 0;
		m_Bottom = Cst_MaxRow;
	}

	void tRect::MoveRow(tIndex sSize) {
		m_Row += sSize;
		m_Bottom += sSize;
	};

	void tRect::MoveCol(tIndex sSize) {
		m_Col += sSize;
		m_Right += sSize;
	};

	tBool tRect::operator < (const tRect sRect) {
		if (m_Row == sRect.m_Row) {
			if (m_Col == sRect.m_Col) {
				if (m_Bottom == sRect.m_Bottom) {
					return(m_Right < sRect.m_Right);
				}
				else { return(m_Bottom < sRect.m_Bottom); }
			}
			else { return(m_Col < sRect.m_Col); }
		}
		else {
			return(m_Row < sRect.m_Row);
		}
	}

	tBool tRect::operator == (const tRect sRect) {
		return((m_Row == sRect.m_Row) &&
			(m_Col == sRect.m_Col) &&
			(m_Bottom == sRect.m_Bottom) &&
			(m_Right == sRect.m_Right));
	}

	tRect tRect::operator = (const tRect sRect) {
		m_Row = sRect.m_Row;
		m_Col = sRect.m_Col;
		m_Bottom = sRect.m_Bottom;
		m_Right = sRect.m_Right;
		return(*this);
	}



	//=========================================================================
	//! Sorted range by operator < on SkRect
	class SkComparatorTempoRect {
	public:
		/// @brief      Operator() compare with SkRect < operator.
		/// @param[in]  sE2 SkRect*
		/// @param[in]  sE1 SkRect*
		SkInline bool operator()(tTempoRect* sE1, tTempoRect* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//=========================================================================
	//! Select Range by integer
	tTempoRect::tTempoRect() : tTempoPoint(), m_Bottom(-1), m_Right(-1)  {}
	tTempoRect::tTempoRect(tIndex sTopRow, tIndex sTopCol, tIndex sBottomRow, tIndex sRightCol) : tTempoPoint(sTopRow,sTopCol),m_Bottom(sBottomRow), m_Right(sRightCol){}
	tTempoRect::tTempoRect(const tTempoRect& sRect) : tTempoPoint(sRect), m_Bottom(sRect.m_Bottom), m_Right(sRect.m_Right){}
    tTempoRect::tTempoRect(tRect& sRect) :  tTempoPoint(sRect), m_Bottom(sRect.Bottom()), m_Right(sRect.Right()){} ;

	tTempoRect::tTempoRect(tString sRef) : tTempoPoint(), m_Bottom(-1), m_Right(-1) {
		ParseRef(sRef);
	}

	tTempoRect::~tTempoRect() {	};

	tIndex tTempoRect::Top() { return(m_Row); }
	void tTempoRect::Top(tIndex sValue) { m_Row = sValue; }

	tIndex tTempoRect::Left() { return(m_Col); }
	void tTempoRect::Left(tIndex sValue) { m_Col = sValue; }

	tIndex tTempoRect::Bottom() { return(m_Bottom); }
	void tTempoRect::Bottom(tIndex sValue) { m_Bottom = sValue; }

	tIndex tTempoRect::Right() { return(m_Right); }
	void tTempoRect::Right(tIndex sValue) { m_Right = sValue; }

    tInt tTempoRect::Width() { return(m_Right-m_Col+1); }
    tInt tTempoRect::Height() { return(m_Bottom-m_Row+1); }

	tBool tTempoRect::IsValid() const {
		return m_Row >= 0 && m_Col >= 0 && m_Bottom >= 0 && m_Right >= 0
			&& m_Row <= m_Bottom && m_Col <= m_Right;
	}

	tString tTempoRect::StrRef() {
		tStringStream wStream;
		wStream << Base10ToAlpha(m_Col) << m_Row << ":" << Base10ToAlpha(m_Right) << m_Bottom;
		return(wStream.str());
	}

	tBool tTempoRect::ParseRef(tString sRef) {
		// Find the colon separator for range format "A1:B10"
		size_t wColonPos = sRef.find(':');
		if (wColonPos == tString::npos) {
			return(false); // No colon found, not a valid range
		}

		// Split the reference into two parts: top-left and bottom-right
		tString wTopLeft = sRef.substr(0, wColonPos);
		tString wBottomRight = sRef.substr(wColonPos + 1);

		// Parse top-left cell
		tString wTopCol, wTopRow;
		bool wFoundDigit = false;
		
		// Loop through each character in the top-left reference
		for (tChar c : wTopLeft) {
			// Check if character is a valid column letter (A-Z)
			if (c >= '@' && c <= 'Z') {
				if (!wFoundDigit) {
					wTopCol += c;
				} else {
					// If a letter appears after digits, it's invalid
					return(false);
				}
			} else if (std::isdigit(c)) {
				// If it's a digit, add to row part
				wFoundDigit = true;
				wTopRow += c;
			} else {
				// Ignore any other character (optional: handle error)
				break;
			}
		}

		// Parse bottom-right cell
		tString wBottomCol, wBottomRow;
		wFoundDigit = false;
		
		// Loop through each character in the bottom-right reference
		for (tChar c : wBottomRight) {
			// Check if character is a valid column letter (A-Z)
			if (c >= '@' && c <= 'Z') {
				if (!wFoundDigit) {
					wBottomCol += c;
				} else {
					// If a letter appears after digits, it's invalid
					return(false);
				}
			} else if (std::isdigit(c)) {
				// If it's a digit, add to row part
				wFoundDigit = true;
				wBottomRow += c;
			} else {
				// Ignore any other character (optional: handle error)
				break;
			}
		}

		// Set the rectangle coordinates if all parts are valid
		if (!wTopCol.empty() && !wTopRow.empty() && !wBottomCol.empty() && !wBottomRow.empty()) {
			m_Col = AlphaToBase10(wTopCol);
			m_Row = std::stoi(wTopRow);
			m_Right = AlphaToBase10(wBottomCol);
			m_Bottom = std::stoi(wBottomRow);
			return(true);
		}

		return(false);
	}
  
    tBool tTempoRect::IsPoint() {
        return((m_Row==m_Bottom) && (m_Col==m_Right));
    }
    tBool tTempoRect::IsColSelect() {
        return((m_Row==0) && (m_Bottom==Cst_MaxRow));
    }

    tBool tTempoRect::IsRowSelect() {
        return((m_Col==0) && (m_Right==Cst_MaxCol));
    }

   
    tBool tTempoRect::IsSheetSelect() {
        return((m_Col==0)  &&
               (m_Right==Cst_MaxCol) &&
               (m_Row==0) &&
               (m_Bottom==Cst_MaxRow));
    }

	void tTempoRect::MoveRow(tIndex sSize) {
		m_Row += sSize;
		m_Bottom += sSize;
	};

	void tTempoRect::MoveCol(tIndex sSize) {
		m_Col += sSize;
		m_Right += sSize;
	};


    tBool tTempoRect::CallBack(tUndoRedoJsonCallBack* sUndoRedoJson) {
        return(sUndoRedoJson->CallBackRange(this));
    }

	// Json ===============================================================
	void tTempoRect::Json(Writer<StringBuffer>* sWriter) {
		sWriter->StartObject();
		sWriter->Key("r");
		sWriter->StartArray();
		sWriter->Int(m_Row);
		sWriter->Int(m_Col);
		sWriter->Int(m_Bottom);
		sWriter->Int(m_Right);
		sWriter->EndArray();
		sWriter->EndObject();
	}

	void tTempoRect::Json(const Value& sValue) {
		// Extract rectangle coordinates from the JSON array
		const Value& wArray = sValue["r"];
		if (wArray.IsArray() && wArray.Size() >= 4) {
			m_Row = wArray[0].GetInt();
			m_Col = wArray[1].GetInt();
			m_Bottom = wArray[2].GetInt();
			m_Right = wArray[3].GetInt();
		}
	}

	// Operator new and delete ==============================================
	void* tTempoRect::operator new(tSize sz) noexcept {
		return(tApplication::Instance()->AllocTemporary(sz));
	}

	void tTempoRect::operator delete(void* ptr) noexcept {
        // do nothing
	}

	// Operator==============================================================
	tBool tTempoRect::operator < (const tTempoRect sRect) {
		if (m_Row == sRect.m_Row) {
			if (m_Col == sRect.m_Col) {
				if (m_Bottom == sRect.m_Bottom) {
					return(m_Right < sRect.m_Right);
				}
				else { return(m_Bottom < sRect.m_Bottom); }
			}
			else { return(m_Col < sRect.m_Col); }
		}
		else {
			return(m_Row < sRect.m_Row);
		}
	}

	tBool tTempoRect::operator == (const tTempoRect sRect) {
		return((m_Row == sRect.m_Row) &&
			(m_Col == sRect.m_Col) &&
			(m_Bottom == sRect.m_Bottom) &&
			(m_Right == sRect.m_Right));
	}

	tTempoRect tTempoRect::operator = (const tTempoRect sRect) {
		m_Row = sRect.m_Row;
		m_Col = sRect.m_Col;
		m_Bottom = sRect.m_Bottom;
		m_Right = sRect.m_Right;
		return(*this);
	}

} //  NameSpace end
