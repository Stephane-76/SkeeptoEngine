//=============================================================================
// SkSpreadSheet StackElem
//=============================================================================
#include "../include/SkStackElem.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"

namespace SkSpreadSheet {

	// StackElem function =====================================================
    tStackElem::tStackElem() {
        m_Type = tStackType::t_None;
    }
 
	tStackElem::tStackElem(const tVariant& sVariant)  {
		m_Type = tStackType::t_Variant; 
		m_Variant = new tVariant(sVariant);
	}
	tStackElem::tStackElem(tRange* sRange) {
		m_Type = tStackType::t_Range; 
		m_Range = sRange;
	}
    
	tStackElem::tStackElem(tCell* sCell)  {
		m_Type = tStackType::t_Cell; 
		m_Cell = sCell;
	}
	tStackElem::tStackElem(tArrayValue* sArray) {
		m_Type = tStackType::t_Array;
		m_Array = sArray;
	}
	tStackElem::tStackElem(tFormulaNamed* sLambda) {
		m_Type = tStackType::t_Lambda;
		m_Lambda = sLambda;
	}
	tStackElem::tStackElem(tFormulaNamed* sLambda, const std::map<tString, tStackElem>& sCaptured) {
		m_Type = tStackType::t_Lambda;
		m_Lambda = sLambda;
		// Deep-copy the closure environment; owned by this elem (freed on destruction).
		m_CapturedScope = new std::map<tString, tStackElem>(sCaptured);
	}
	tStackElem::tStackElem(const tStackElem& sOther)  {
		m_Type = sOther.m_Type;
		if (m_Type == tStackType::t_Variant) {
			m_Variant = new tVariant(*sOther.m_Variant);
		} else if (m_Type == tStackType::t_Range) {
			m_Range = sOther.m_Range;
		} else if (m_Type == tStackType::t_Cell || m_Type == tStackType::t_Attribute) {
			m_Cell = sOther.m_Cell;
		} else if (m_Type == tStackType::t_Array) {
			// Deep-copy: array values are owned by each stack elem (transient, freed on destruction).
			m_Array = (sOther.m_Array != nullptr) ? new tArrayValue(*sOther.m_Array) : nullptr;
		} else if (m_Type == tStackType::t_Lambda) {
			// Non-owning reference to a named LAMBDA: shallow copy. The closure environment (if any) is owned,
			// so deep-copy it too.
			m_Lambda = sOther.m_Lambda;
			if (sOther.m_CapturedScope != nullptr) {
				m_CapturedScope = new std::map<tString, tStackElem>(*sOther.m_CapturedScope);
			}
		}
	}

	tStackElem::~tStackElem() {
		if (m_Type == tStackType::t_Variant) {
			if (m_Variant != nullptr) {
				delete m_Variant;
				m_Variant = nullptr;
			}
		} else if (m_Type == tStackType::t_Array) {
			if (m_Array != nullptr) {
				delete m_Array;
				m_Array = nullptr;
			}
		}
		// Owned closure environment (t_Lambda only; nullptr otherwise).
		if (m_CapturedScope != nullptr) {
			delete m_CapturedScope;
			m_CapturedScope = nullptr;
		}
	}

	tStackElem& tStackElem::operator=(const tStackElem& sOther) {
		if (this != &sOther) {
			// Free any owned payload of the current type before reassigning.
			if (m_Type == tStackType::t_Variant && m_Variant != nullptr) {
				delete m_Variant;
			} else if (m_Type == tStackType::t_Array && m_Array != nullptr) {
				delete m_Array;
			}
			// Free the previous closure environment (if any) before reassigning.
			if (m_CapturedScope != nullptr) {
				delete m_CapturedScope;
				m_CapturedScope = nullptr;
			}
			
			m_Type = sOther.m_Type;
			if (m_Type == tStackType::t_Variant) {
				m_Variant = new tVariant(*sOther.m_Variant);
			} else if (m_Type == tStackType::t_Range) {
				m_Range = sOther.m_Range;
			} else if (m_Type == tStackType::t_Cell || m_Type == tStackType::t_Attribute) {
				m_Cell = sOther.m_Cell;
			} else if (m_Type == tStackType::t_Array) {
				m_Array = (sOther.m_Array != nullptr) ? new tArrayValue(*sOther.m_Array) : nullptr;
			} else if (m_Type == tStackType::t_Lambda) {
				m_Lambda = sOther.m_Lambda;
				if (sOther.m_CapturedScope != nullptr) {
					m_CapturedScope = new std::map<tString, tStackElem>(*sOther.m_CapturedScope);
				}
			}
		}
		return *this;
	}

    tStackElem::operator const tVariant() {
        return(Value());
    }


	tStackType tStackElem::Type() const { return(m_Type); }
	tVariant tStackElem::Variant() const {
		if (m_Type == tStackType::t_Variant && m_Variant != nullptr) {
			return *m_Variant;
		}
		return tVariant();
	}
    
	tRange* tStackElem::Range() const {
		if (m_Type == tStackType::t_Range) {
			return m_Range;
		}
		return nullptr;
	}
	tCell* tStackElem::Cell() const {
		if (m_Type == tStackType::t_Cell || m_Type == tStackType::t_Attribute) {
			return m_Cell;
		}
        if (m_Type == tStackType::t_Range) {
            if (m_Range->IsCell()) {
                return(m_Range->Cell());
            }
        }
		return nullptr;
	}

	tArrayValue* tStackElem::Array() const {
		if (m_Type == tStackType::t_Array) {
			return m_Array;
		}
		return nullptr;
	}

	tFormulaNamed* tStackElem::Lambda() const {
		if (m_Type == tStackType::t_Lambda) {
			return m_Lambda;
		}
		return nullptr;
	}

	const std::map<tString, tStackElem>* tStackElem::CapturedScope() const {
		if (m_Type == tStackType::t_Lambda) {
			return m_CapturedScope;
		}
		return nullptr;
	}

    const tVariant tStackElem::Value() const {
         tVariant wValue;
        switch (m_Type) {
            case tStackType::t_Variant : {
                if (m_Variant != nullptr) {
                    wValue = *m_Variant;
                }
                break;
            }
            case tStackType::t_Attribute:
            case tStackType::t_Cell : {
                tCell* wCell=m_Cell;
                if (wCell!=nullptr) {
                    wValue=wCell->CalculableValue();
                }
                break;
            }
            case tStackType::t_Range : {
                if (m_Range!=nullptr) {
                    tCell* wCell=m_Range->Cell();
                    if (wCell!=nullptr) {
                        wValue=wCell->CalculableValue();
                    }
                }
                break;
            }
            case tStackType::t_Array : {
                // Implicit-intersection fallback: an array used where a scalar is
                // expected resolves to its top-left element (Excel @ behaviour).
                if (m_Array != nullptr && m_Array->Count() > 0) {
                    wValue = m_Array->At(0, 0);
                }
                break;
            }
            default : break;
        }
        return(wValue);
    }

#ifdef _DEBUGSK
    tString tStackElem::Debug() const {
        tStringStream wStream;
        switch (m_Type) {
        case tStackType::t_Variant: wStream << "V:" << *m_Variant; break;
        case tStackType::t_Range: wStream << "R:" << m_Range->StrRef(); break;
        case tStackType::t_Cell: wStream << "C:" << m_Cell->StrRef() << "=" << m_Cell->CalculableValue(); break;
        case tStackType::t_Array:
            wStream << "A:" << (m_Array != nullptr ? m_Array->m_Rows : 0) << "x"
                    << (m_Array != nullptr ? m_Array->m_Cols : 0);
            break;
        default: wStream << "Unknown"; break;
        }
        return(wStream.str());
    }
#endif

}
