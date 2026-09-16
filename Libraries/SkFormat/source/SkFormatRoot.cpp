//=============================================================================
// SkFormat (Sub system of Css )
//=============================================================================
#include "../include/SkFormatRoot.hpp"

#define _DEBUGSKformat

namespace SkFormat {
	// SkFormatRoot ===========================================================
	// One class For all application 
	tFormatRoot* tFormatRoot::m_StaticFormatRoot = nullptr;

	tFormatRoot::tFormatRoot() : tClass(), m_OldKeyPool(""), m_CurrentFormat(),m_ModifyFormat(), m_ModifyRef(0) {
        for(tSize wInd=0; wInd < sizeof(CstRecColor)/sizeof(tRecColor); wInd++ ) {
            m_MapNameColor[CstRecColor[wInd].m_Key]=&CstRecColor[wInd];
            m_MapColor[tUInt(CstRecColor[wInd].m_Color)]=&CstRecColor[wInd];
        }
    }

	void tFormatRoot::Clear() {
        m_FormatMerge.Clear();
        tVectorString wVectorName;
        tMapKey::iterator wIterator;
        for(wIterator=m_MapFormat.begin();wIterator!=m_MapFormat.end(); wIterator++) {
            wVectorName.push_back((*wIterator).first);
        }
        for(auto wName : wVectorName) {
            DeleteFormat(wName);
        }
        
        m_AllocatorMargin.Clear();
		m_AllocatorPadding.Clear();
		m_AllocatorShadow.Clear();
		m_AllocatorFont.Clear();
		m_AllocatorText.Clear();
		m_AllocatorBorderRect.Clear();
		m_AllocatorFormat.Clear();
		m_MapFormatPool.clear();
        m_MapFormat.clear();
        m_CurrentFormat.Clear();
        m_ModifyFormat.Clear();
	};

	void tFormatRoot::Reset() {
		Clear();
		m_AllocatorMargin.Init();
		m_AllocatorPadding.Init();
		m_AllocatorShadow.Init();
		m_AllocatorFont.Init();
		m_AllocatorText.Init();
		m_AllocatorBorderRect.Init();
		m_AllocatorFormat.Init();
	};


    tVectorString tFormatRoot::ListOfCssName() {
        tVectorString wResult;
        tMapKey::iterator wIterator;
        for(wIterator=m_MapFormat.begin();wIterator!=m_MapFormat.end(); wIterator++) {
            wResult.push_back((*wIterator).first);
        }
        sort(wResult.begin(),wResult.end(),tComparatorClass<tString>());
        return(wResult);
    }

	// Format root Instance ===================================================
	tFormatRoot* tFormatRoot::Instance() {
		if (m_StaticFormatRoot == nullptr) {
			m_StaticFormatRoot = new  tFormatRoot();
		}
		return(m_StaticFormatRoot);
	}


	void tFormatRoot::IncComponent(tFormatCss* sFormat) {
		if (sFormat->Margin() != 0) Margin(sFormat->Margin())->Inc();
		if (sFormat->Padding() != 0) Padding(sFormat->Padding())->Inc();
		if (sFormat->Shadow() != 0) Shadow(sFormat->Shadow())->Inc();
		if (sFormat->Font() != 0) Font(sFormat->Font())->Inc();
		if (sFormat->Text() != 0) Text(sFormat->Text())->Inc();
		if (sFormat->BorderRect() != 0) BorderRect(sFormat->BorderRect())->Inc();
	}

	void tFormatRoot::DeleteComponent(tFormatCss* sFormat) {
        DeleteMargin(sFormat->Margin());
		DeletePadding(sFormat->Padding());
		DeleteShadow(sFormat->Shadow()); 
		DeleteFont(sFormat->Font()); 
		DeleteText(sFormat->Text()); 
		DeleteBorderRect(sFormat->BorderRect()); 
        sFormat->ClearComponent();
	}
    
	// Interface By Ref =======================================================
	tFormatCss* tFormatRoot::AllocFormatRef() {
#ifdef debugformat
        cout << "tFormatRoot::AllocFormatRef()" << endl;
#endif
		m_OldKeyPool = "";
		m_CurrentKey = "";
		m_ModifyRef = 0;
		m_CurrentFormat.Clear();
		return(&m_CurrentFormat);
	}
    
	tFormatCss* tFormatRoot::FormatRef(tFormatRef sFormatRef) {
        if (sFormatRef==0) return(nullptr);
		tFormatCss* wFormat = m_AllocatorFormat(sFormatRef);
		assert(wFormat != nullptr);
		return(wFormat);
	}

	void tFormatRoot::DeleteFormatRef(tFormatRef sFormatRef) {
        tFormatCss* wFormat = m_AllocatorFormat(sFormatRef);
        assert(wFormat != nullptr);
#ifdef debugformat
        cout << "tFormatRoot::DeleteFormatRef("<< sFormatRef <<")" << wFormat->CssStr(this) << " count("<< wFormat->CountCell() << ")" << endl;
#endif
        tString wKeyPool = wFormat->RootKey(this);
        tStringStream wStreamKeyRef;
        wStreamKeyRef << sFormatRef;
       
        // Delete or Dec count of element
        DeleteComponent(wFormat);
        
        // Erase in MapFormat ================================================
        wFormat->DecCell();
        if (wFormat->CountCell()==0) {
            tMapKey::iterator wIterator = m_MapFormat.find(wStreamKeyRef.str());
            // If Exist ===========================================================
            if (wIterator != m_MapFormat.end()) {
                m_MapFormat.erase(wKeyPool);
            }
            else {
                tStringStream wStream;
                wStream << "On delete  format count Cell  Css :" << wStreamKeyRef.str() << " not find on Map !";
                //throw(new tExceptionInternalError(wStream.str()));
            }
        }
        wFormat->Dec();
        if (wFormat->Count() == 0) {
            tMapKey::iterator wIterator = m_MapFormatPool.find(wKeyPool);
            // If Exist ===========================================================
            if (wIterator != m_MapFormatPool.end()) {
                m_MapFormatPool.erase(wKeyPool);
            }
            else {
                tStringStream wStream;
                wStream << "On delete format  Css :" << wKeyPool << " not find on Map !";
                //throw(new tExceptionInternalError(wStream.str()));
            }
            m_AllocatorFormat.Delete(sFormatRef);
        }
	}

	tFormatCss* tFormatRoot::AllocFormat(tString sKey) {
#ifdef debugformat
        cout << "tFormatRoot::AllocFormat(" << sKey << ")" << endl;
#endif
		m_OldKeyPool = "";
		m_CurrentKey = sKey;
		m_ModifyRef = 0;
		m_CurrentFormat.Clear();
        return(&m_CurrentFormat);
	}

	// Interface By Key =======================================================
	tFormatCss* tFormatRoot::Format(tString sKey) {
		tMapKey::iterator wIterator = m_MapFormat.find(sKey);
		if (wIterator != m_MapFormat.end()) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormat = m_AllocatorFormat(wRef);
			assert(wFormat != nullptr);
			return(wFormat);
		}
		return(nullptr);
	}


	void tFormatRoot::DeleteFormat(tString sKey) {
		tMapKey::iterator wIterator = m_MapFormat.find(sKey);
		if (wIterator != m_MapFormat.end()) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormat = m_AllocatorFormat(wRef);
			assert(wFormat != nullptr);
#ifdef debugformat
            cout << "tFormatRoot::DeleteFormat("<< sKey <<")" << wFormat->CssStr(this) << " count("<< wFormat->CountCell() << ")" << endl;
#endif

			tString wKeyPool = wFormat->RootKey(this);

            
			// Element delete or decrement count
            DeleteComponent(wFormat);
	        
            wFormat->Dec();
            // Is Last Format Pool ============================================
			if (wFormat->Count() == 0) {
				tMapKey::iterator wIterator = m_MapFormatPool.find(wKeyPool);
				// If Exist ===================================================
				if (wIterator != m_MapFormatPool.end()) {
					m_MapFormatPool.erase(wKeyPool);
				} else {
					tStringStream wStream;
					wStream << "On Delete Format Css :"  << sKey << " Pool:" << wKeyPool << " not find on Map pool !";
					//throw(new tExceptionInternalError(wStream.str()));
				}
				m_AllocatorFormat.Delete(wRef);
			}
			m_MapFormat.erase(sKey);
        } else {
            tStringStream wStream;
            wStream << "On Delete Format Css :" << sKey << " not find on Map !";
            //throw(new tExceptionInternalError(wStream.str()));
        }
	}

	tFormatCss*  tFormatRoot::ModifyFormat(tString sKey) {
		m_CurrentKey = sKey; 
		tMapKey::iterator wIterator = m_MapFormat.find(sKey);
		if (wIterator != m_MapFormat.end()) {
			tFormatRef wRef = (*wIterator).second;
			m_CurrentFormat = (*m_AllocatorFormat(wRef));
			m_OldKeyPool = m_CurrentFormat.RootKey(this);
			m_ModifyRef = wRef;
            m_ModifyFormat=m_CurrentFormat;
			return(&m_CurrentFormat);
		} 
		return(nullptr);
	};

	tFormatRef tFormatRoot::Validate() {
		tString wKeyPool = m_CurrentFormat.RootKey(this);
#ifdef debugformat
        cout << "tFormatRoot::Validate()" << m_CurrentFormat.CssStr(this) << endl;
#endif

		//cout << "Key=" << wKeyPool << endl;
		// Is Empty Key
		if (wKeyPool == "") return(0);
		
        // Format Pool exist ? (same key)
		tMapKey::iterator wIterator = m_MapFormatPool.find(wKeyPool);
		tFormatRef wFormatRef = 0;
        tFormatCss* wFormat=nullptr;
		// If Exist ===========================================================
		if (wIterator != m_MapFormatPool.end()) {
			wFormatRef = (*wIterator).second;
			assert(wFormatRef != 0);
			wFormat = m_AllocatorFormat(wFormatRef);
		}
		else {
			tie(wFormatRef, wFormat) = m_AllocatorFormat.Alloc();
            // Set Format value in memory
			(*wFormat)=m_CurrentFormat;
		}
       
        m_MapFormatPool[wKeyPool]=wFormatRef;
        //wFormat->IncCell();
        wFormat->Inc();
        IncComponent(FormatRef(wFormatRef));
    
		m_MapFormat[m_CurrentKey] = wFormatRef;

        if (m_ModifyRef != 0) {
            DeleteComponent(&m_ModifyFormat);
        }
      
#ifdef checkfo
		Check();
#endif
		m_CurrentFormat.Clear();
		return(wFormatRef);
	}

    tFormatRef tFormatRoot::ApplyCell() {
        tString wKeyPool = m_CurrentFormat.RootKey(this);
        // Is Empty Key
        if (wKeyPool == "") return(0);
             
        // Format Pool exist ? (same key)
        tMapKey::iterator wIterator = m_MapFormatPool.find(wKeyPool);
        tFormatRef wFormatRef = 0;
        tFormatCss* wFormat=nullptr;
        
        // If Exist ===========================================================
        if (wIterator != m_MapFormatPool.end()) {
            wFormatRef = (*wIterator).second;
            assert(wFormatRef != 0);
            wFormat = m_AllocatorFormat(wFormatRef);
            // Inc Cell
            wFormat->IncCell();
#ifdef checkfo
            // Pool entries created via non-cell paths must still participate in CheckCell.
            wFormat->FormatCell(true);
#endif
#ifdef debugformat
            cout << "tFormatRoot::ApplyCell() exist (" << wFormatRef << ") " << wFormat->CssStr(this) << " count("<< wFormat->CountCell() << ")" << endl;
#endif
            return(wFormatRef);
        }
        else {
            tie(wFormatRef, wFormat) = m_AllocatorFormat.Alloc();
            (*wFormat)=m_CurrentFormat;
#ifdef checkfo
            wFormat->FormatCell(true);
#endif
        }
        // FormatRef -> tString (Key)...
        // Key is allocator ref (string)
        tStringStream wStream;
        wStream << wFormatRef;
        
        m_MapFormatPool[wKeyPool]=wFormatRef;
        // Increment counter
        wFormat->Inc();
        // Inc Cell
        wFormat->IncCell();

        IncComponent(FormatRef(wFormatRef));
        // FormatRef
        m_MapFormat[wStream.str()] = wFormatRef;
#ifdef checkfo
        Check();
#endif
#ifdef debugformat
        cout << "tFormatRoot::ApplyCell() create (" << wFormatRef << ") "<< m_CurrentFormat.CssStr(this) << " count("<< wFormat->CountCell() << ")" << endl;
#endif
        m_CurrentFormat.Clear();
        return(wFormatRef);
    }

    void tFormatRoot::DeleteCell(tFormatRef sCellInstance) {
        tStringStream wStream;
        wStream << sCellInstance;
        tString wKey=wStream.str();
        // Format Pool exist ? (same key)
        tMapKey::iterator wIterator = m_MapFormat.find(wKey);
        tFormatRef wFormatRef = 0;
        tFormatCss* wFormat=nullptr;
        // If Exist ===========================================================
        if (wIterator != m_MapFormat.end()) {
            wFormatRef = (*wIterator).second;
            assert(wFormatRef != 0);
            wFormat = m_AllocatorFormat(wFormatRef);
#ifdef debugformat
            cout << "tFormatRoot::DeleteCell():" << wFormat->CssStr(this) << " count("<< wFormat->CountCell() << ")" << endl;
#endif
            wFormat->DecCell();
            if (wFormat->Cell()==0) {
                DeleteFormat(wKey);
            }
        }
        else {
            // Error
            tStringStream wStream;
            wStream << "On Delete Cell  Css :" << wKey << " not find on Map !";
            cerr << wStream.str() << endl;
            //throw(new tExceptionInternalError(wStream.str()));
        }
    }

    void tFormatRoot::IncCell(tFormatRef sCellInstance) {
        tStringStream wStream;
        wStream << sCellInstance;
        tString wKey=wStream.str();
        // Format Pool exist ? (same key)
        tMapKey::iterator wIterator = m_MapFormat.find(wKey);
        tFormatRef wFormatRef = 0;
        tFormatCss* wFormat=nullptr;
        // If Exist ===========================================================
        if (wIterator != m_MapFormat.end()) {
            wFormatRef = (*wIterator).second;
            assert(wFormatRef != 0);
            wFormat = m_AllocatorFormat(wFormatRef);
            wFormat->IncCell();
        }
        else {
            tStringStream wStream;
            wStream << "On increment Cell  Css :" << wKey << " not find on Map !";
            cerr << wStream.str() << endl;
            //throw(new tExceptionInternalError(wStream.str()));
        }
    }


	tFormatCss* tFormatRoot::MergeInPlace(tString sPivotKey, tString sKey) {
		// Already exist
		tString wNewKey = "@" + sPivotKey + "@" + sKey;
		tFormatCss* wNewFormat = Format(wNewKey);
		if (wNewFormat != nullptr) {
			return(wNewFormat);
		}

		// Take format Pivot and Format Copy
		tFormatCss* wFormatPivot = Format(sPivotKey);
		if (wFormatPivot == nullptr) return(nullptr);
		tFormatCss* wFormatCopy = Format(sKey);
		if (wFormatCopy == nullptr) return(nullptr);

        AllocFormat(wNewKey);
		m_CurrentFormat = *wFormatPivot;

		// Place Element
		if (!wFormatCopy->Width().Empty()) m_CurrentFormat.Width(wFormatCopy->Width());
		if (!wFormatCopy->Height().Empty()) m_CurrentFormat.Height(wFormatCopy->Height());

		if (!wFormatCopy->Color().NotUse()) m_CurrentFormat.Color() = wFormatCopy->Color();
		if (!wFormatCopy->BackgroundColor().NotUse()) m_CurrentFormat.BackgroundColor() = wFormatCopy->BackgroundColor();

		if (wFormatCopy->Margin() != 0) {
            if (CurrentMargin()!=nullptr) {
                tUnitRectCss wRectCss=*CurrentMargin();
                wRectCss.Merge(Margin(wFormatCopy->Margin()));
                Margin(wRectCss);
            } else {
                Margin(*Margin(wFormatCopy->Margin()));
            }
		}
		if (wFormatCopy->Padding() != 0) {
            if (CurrentPadding()!=nullptr) {
                tUnitRectCss wRectCss=*CurrentPadding();
                wRectCss.Merge(Padding(wFormatCopy->Padding()));
                Padding(wRectCss);
            } else {
                Padding(*Padding(wFormatCopy->Padding()));
            }
		}
		if (wFormatCopy->Shadow() != 0) {
			Shadow(*Shadow(wFormatCopy->Shadow()));
		}
		if (wFormatCopy->Font() != 0) {
            if (CurrentFont()!=nullptr) {
                tFontCss wFontCss=*CurrentFont();
                wFontCss.Merge(Font(wFormatCopy->Font()));
                Font(wFontCss);
            } else {
                Font(*Font(wFormatCopy->Font()));
            }
		}
		if (wFormatCopy->Text() != 0) {
            if (CurrentText()!=nullptr) {
                tTextCss wTextCss=*CurrentText();
                wTextCss.Merge(Text(wFormatCopy->Text()));
                Text(wTextCss);
            } else {
                Text(*Text(wFormatCopy->Text()));
            }
		}
		if (wFormatCopy->BorderRect() != 0) {
            if (CurrentBorderRect()!=nullptr) {
                tBorderRectCss wBorderRectCss=*CurrentBorderRect();
                wBorderRectCss.Merge(BorderRect(wFormatCopy->BorderRect()));
                BorderRect(wBorderRectCss);
            } else {
                BorderRect(*BorderRect(wFormatCopy->BorderRect()));
            }
        }
       
        Validate();
		return(Format(wNewKey));
	}

    void tFormatRoot::BeginMerge() {
#ifdef debugformat
        cout << "tFormatRoot::BeginMerge()"<< endl;
#endif
        m_FormatMerge.Clear();
#ifdef debugformat
        cout << " After Clear m_FormatMerge=" << m_FormatMerge.CssStr(this) << ";" << endl;
#endif
    }

    void tFormatRoot::Merge(tFormatRef sInstance) {
        tFormatCss* wFormatCopy = FormatRef(sInstance);
        if (wFormatCopy == nullptr) return;
#ifdef debugformat
        cout << "tFormatRoot::Merge() Copy "<< wFormatCopy->CssStr(this) << endl;
        cout << "                     Merge:" << m_FormatMerge.CssStr(this) << endl;
#endif
        // Place Element
        if (!wFormatCopy->Width().Empty()) m_FormatMerge.m_Width=wFormatCopy->Width();
        if (!wFormatCopy->Height().Empty()) m_FormatMerge.m_Height=wFormatCopy->Height();

        if (!wFormatCopy->Color().NotUse()) m_FormatMerge.m_Color = wFormatCopy->Color();
        if (!wFormatCopy->BackgroundColor().NotUse()) m_FormatMerge.m_BackgroundColor = wFormatCopy->BackgroundColor();

        if (wFormatCopy->Margin() != 0) {
            m_FormatMerge.m_Margin.Merge(Margin(wFormatCopy->Margin()));
        }
        if (wFormatCopy->Padding() != 0) {
            m_FormatMerge.m_Padding.Merge(Padding(wFormatCopy->Padding()));
        }
        if (wFormatCopy->Shadow() != 0) {
            // Futur
            //m_FormatMerge.m_Shadow.Merge(Shadow(wFormatCopy->Shadow()));
        }
        if (wFormatCopy->Font() != 0) {
            m_FormatMerge.m_Font.Merge(Font(wFormatCopy->Font()));
        }
        if (wFormatCopy->Text() != 0) {
            m_FormatMerge.m_Text.Merge(Text(wFormatCopy->Text()));
        }
        if (wFormatCopy->BorderRect() != 0) {
            m_FormatMerge.m_BorderRect.Merge(BorderRect(wFormatCopy->BorderRect()));
        }
        
    }
    
    tFormatRef tFormatRoot::ApplyMerge() {
#ifdef debugformat
        cout << "tFormatRoot::ApplyMerge()" << endl;
        cout <<  "     m_CurrentFormat=" <<  m_CurrentFormat.CssStr(this) << endl;
        cout <<  "     m_FormatMerge=" << m_FormatMerge.CssStr(this) << endl;
#endif
        m_CurrentFormat.ClearComponent();
        // Place Element
        if (!m_FormatMerge.m_Width.Empty()) m_CurrentFormat.Width(m_FormatMerge.m_Width);
        if (!m_FormatMerge.m_Height.Empty()) m_CurrentFormat.Height(m_FormatMerge.m_Height);

        if (!m_FormatMerge.m_Color.NotUse()) m_CurrentFormat.Color() = m_FormatMerge.m_Color;
        if (!m_FormatMerge.m_BackgroundColor.NotUse()) m_CurrentFormat.BackgroundColor() = m_FormatMerge.m_BackgroundColor;

        if (!m_FormatMerge.m_Margin.Empty()) {
            Margin(m_FormatMerge.m_Margin);
        }
        if (!m_FormatMerge.m_Padding.Empty()) {
            Padding(m_FormatMerge.m_Padding);
        }
        if (!m_FormatMerge.m_Shadow.Empty()) {
            Shadow(m_FormatMerge.m_Shadow);
        }
        if (!m_FormatMerge.m_Font.Empty()) {
            Font(m_FormatMerge.m_Font);
        }
        if (!m_FormatMerge.m_Text.Empty()) {
            Text(m_FormatMerge.m_Text);
            
        }
        if (!m_FormatMerge.m_BorderRect.Empty()) {
            BorderRect(m_FormatMerge.m_BorderRect);
        }
        return(ApplyCell());
    }

    tFormatMergeCss* tFormatRoot::FormatMerge() { return(&m_FormatMerge); };

    tFormatRef tFormatRoot::DeleteBorder(tFormatRef sInstance,tShort sBorderMask) {
        tFormatCss* wFormat = FormatRef(sInstance);
        if (wFormat == nullptr) return(0);
        
#ifdef debugformat
        cout << "tFormatRoot::DeleteBorder("<< sInstance <<") mask :(" << sBorderMask << ")=" << wFormat->CssStr(this) << endl;
#endif

        tBorderRectCss* wBorderRectCssRef=wFormat->BorderRect(this);
        // if not border don't change
        if (wBorderRectCssRef==nullptr) return(sInstance);
        
        tBorderRectCss wBorderRectCss=tBorderRectCss(*wBorderRectCssRef);
        
        if (wBorderRectCss.DeleteBorder(sBorderMask)) {
            m_CurrentFormat.Clear();
            m_FormatMerge.Clear();
            Merge(sInstance);
            m_FormatMerge.m_BorderRect=wBorderRectCss;
            tFormatRef wResult=ApplyMerge();
#ifdef debugformat
            tFormatCss* wFormat=FormatRef(wResult);
            cout << "tFormatRoot::============>("<< wResult <<") mask :(" << sBorderMask << ")=";
            if (wFormat!=nullptr) cout << wFormat->CssStr(this);
            cout << endl;
#endif
            return(wResult);
        }
        // Else don't change
        return(sInstance);
    }
    
    
    tShort tFormatRoot::BorderMask(tFormatRef sInstance) {
        tFormatCss* wFormat = FormatRef(sInstance);
        if (wFormat != nullptr) {
            tBorderRectCss* wBorderRectCss=wFormat->BorderRect(this);
            if (wBorderRectCss!=nullptr) {
                return(wBorderRectCss->BorderMask());
            }
        }
        return(0);
    }

	tString tFormatRoot::Str(tString sKey) {
		tStringStream wStream;
		m_CurrentKey = sKey;
		tMapKey::iterator wIterator = m_MapFormat.find(sKey);
		if (wIterator != m_MapFormat.end()) {
			wStream << (*wIterator).first << ": {" << endl;
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormat = m_AllocatorFormat(wRef);
			wStream << wFormat->Str(this);
			wStream << "}" << endl;
			return(wStream.str());
		}
		return("");
	}
	
	tString tFormatRoot::Str() {
		tStringStream wStream;
		tMapKey::iterator wIterator;
		for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
			wStream << (*wIterator).first << ": {";
			wStream << wFormatCss->Str(this);
            wStream << "}";
            wStream << " Cpt:" << wFormatCss->CountCell();
            wStream << endl;
		}
		return(wStream.str());
	}

	void tFormatRoot::Width(tUnitCss sWidth) { m_CurrentFormat.Width(sWidth); }
	tUnitCss tFormatRoot::Width() { return(m_CurrentFormat.Width()); }

	void tFormatRoot::Height(tUnitCss sHeight) { m_CurrentFormat.Height(sHeight); }
	tUnitCss tFormatRoot::Height() { return(m_CurrentFormat.Height()); }

	tColorCss& tFormatRoot::Color() { return(m_CurrentFormat.Color()); };

	tColorCss& tFormatRoot::BackgroundColor() { return(m_CurrentFormat.BackgroundColor()); }

	// Margin =================================================================
	void  tFormatRoot::Margin(tUnitRectCss& sMargin) {
		tUnitRectCss* wMargin;
		tFormatRef wMarginRef;
		SkIteratorItem wIterator = m_AllocatorMargin.FindEqual(&sMargin);
		if (wIterator!= m_AllocatorMargin.Vector()->end()) {
			wMarginRef = (*wIterator);
		} else {
			tie(wMarginRef, wMargin) = m_AllocatorMargin.Alloc(&sMargin);
		}
		m_CurrentFormat.Margin(wMarginRef);
	}

	tUnitRectCss* tFormatRoot::Margin(tFormatRef sMarginRef) {
		return(m_AllocatorMargin(sMarginRef));
	}

	tUnitRectCss* tFormatRoot::CurrentMargin() {
		tFormatRef wMarginRef = m_CurrentFormat.Margin();
		if (wMarginRef != 0) {
			return(m_AllocatorMargin(wMarginRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeleteMargin(tFormatRef sMarginRef) {
		if (sMarginRef != 0) {
            tUnitRectCss* wMargin=Margin(sMarginRef);
            wMargin->Dec();
            if (wMargin->Empty()) {
                m_AllocatorMargin.Delete(sMarginRef);
            }
			return(true);
		}
		return(false);
	}

	// Padding ================================================================
	void  tFormatRoot::Padding(tUnitRectCss& sPadding) {
		tUnitRectCss* wPadding;
		tFormatRef wPaddingRef;
		SkIteratorItem wIterator = m_AllocatorPadding.FindEqual(&sPadding);
		if (wIterator != m_AllocatorPadding.Vector()->end()) {
			wPaddingRef = (*wIterator);
		}
		else {
			tie(wPaddingRef, wPadding) = m_AllocatorPadding.Alloc(&sPadding);
		}
		m_CurrentFormat.Padding(wPaddingRef);
	}

	tUnitRectCss* tFormatRoot::Padding(tFormatRef sPaddingRef) {
		return(m_AllocatorPadding(sPaddingRef));
	}

	tUnitRectCss* tFormatRoot::CurrentPadding() {
		tFormatRef wPaddingRef = m_CurrentFormat.Padding();
		if (wPaddingRef != 0) {
			return(m_AllocatorPadding(wPaddingRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeletePadding(tFormatRef sPaddingRef) {
		if (sPaddingRef != 0) {
            tUnitRectCss* wPadding=Padding(sPaddingRef);
            wPadding->Dec();
            if (wPadding->Empty()) {
                m_AllocatorPadding.Delete(sPaddingRef);
            }
			return(true);
		}
		return(false);
	}

	// Shadow ================================================================
	void  tFormatRoot::Shadow(tShadowCss& sShadow) {
		tShadowCss* wShadow;
		tFormatRef wShadowRef;
		SkIteratorItem wIterator = m_AllocatorShadow.FindEqual(&sShadow);
		if (wIterator != m_AllocatorShadow.Vector()->end()) {
			wShadowRef = (*wIterator);
		}
		else {
			tie(wShadowRef, wShadow) = m_AllocatorShadow.Alloc(&sShadow);
		}
		m_CurrentFormat.Shadow(wShadowRef);
	}

	tShadowCss* tFormatRoot::Shadow(tFormatRef sShadowRef) {
		return(m_AllocatorShadow(sShadowRef));
	}

	tShadowCss* tFormatRoot::CurrentShadow() {
		tFormatRef wShadowRef = m_CurrentFormat.Shadow();
		if (wShadowRef != 0) {
			return(m_AllocatorShadow(wShadowRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeleteShadow(tFormatRef sShadowRef) {
		if (sShadowRef != 0) {
            tShadowCss* wShadow=Shadow(sShadowRef);
            wShadow->Dec();
            if (wShadow->Empty()) {
                m_AllocatorShadow.Delete(sShadowRef);
            }
			return(true);
		}
		return(false);
	}

	// Font ===================================================================
	void  tFormatRoot::Font(tFontCss& sFont) {
		tFontCss* wFont;
		tFormatRef wFontRef;
		SkIteratorItem wIterator = m_AllocatorFont.FindEqual(&sFont);
		if (wIterator != m_AllocatorFont.Vector()->end()) {
			wFontRef = (*wIterator);
		}
		else {
			tie(wFontRef, wFont) = m_AllocatorFont.Alloc(&sFont);
		}
		m_CurrentFormat.Font(wFontRef);
	}

	tFontCss* tFormatRoot::Font(tFormatRef sFontRef) {
		return(m_AllocatorFont(sFontRef));
	}

	tFontCss* tFormatRoot::CurrentFont() {
		tFormatRef wFontRef = m_CurrentFormat.Font();
		if (wFontRef != 0) {
			return(m_AllocatorFont(wFontRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeleteFont(tFormatRef sFontRef) {
		if (sFontRef != 0) {
            tFontCss* wFont=Font(sFontRef);
            wFont->Dec();
            if (wFont->Empty()) {
                m_AllocatorFont.Delete(sFontRef);
            }
			return(true);
		}
		return(false);
	}

	// Text ===================================================================
	void  tFormatRoot::Text(tTextCss& sText) {
		tTextCss* wText;
		tFormatRef wTextRef;
		SkIteratorItem wIterator = m_AllocatorText.FindEqual(&sText);
		if (wIterator != m_AllocatorText.Vector()->end()) {
			wTextRef = (*wIterator);
		}
		else {
			tie(wTextRef, wText) = m_AllocatorText.Alloc(&sText);
		}
		m_CurrentFormat.Text(wTextRef);
	}

	tTextCss* tFormatRoot::Text(tFormatRef sTextRef) {
		return(m_AllocatorText(sTextRef));
	}

	tTextCss* tFormatRoot::CurrentText() {
		tFormatRef wTextRef = m_CurrentFormat.Text();
		if (wTextRef != 0) {
			return(m_AllocatorText(wTextRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeleteText(tFormatRef sTextRef) {
        if (sTextRef != 0) {
            tTextCss* wText=Text(sTextRef);
            wText->Dec();
            if (wText->Empty()) {
                m_AllocatorText.Delete(sTextRef);
            }
        }
		return(false);
	}


	// BorderRect ==============================================================
	void  tFormatRoot::BorderRect(tBorderRectCss& sBorderRect) {
		if (sBorderRect.Empty()) {
			// Set 0 Erase with Key After
			if (m_CurrentFormat.BorderRect()!= 0) {
				 m_CurrentFormat.BorderRect((tFormatRef)0);
			}
			return;
		}

		tBorderRectCss* wBorderRect;
		tFormatRef wBorderRectRef;
		SkIteratorItem wIterator = m_AllocatorBorderRect.FindEqual(&sBorderRect);
		if (wIterator != m_AllocatorBorderRect.Vector()->end()) {
			wBorderRectRef = (*wIterator);
		}
		else {
			tie(wBorderRectRef, wBorderRect) = m_AllocatorBorderRect.Alloc(&sBorderRect);
		}
		m_CurrentFormat.BorderRect(wBorderRectRef);
	}

	tBorderRectCss* tFormatRoot::BorderRect(tFormatRef sBorderRectRef) {
		return(m_AllocatorBorderRect(sBorderRectRef));
	}

	tBorderRectCss* tFormatRoot::CurrentBorderRect() {
		tFormatRef wBorderRectRef = m_CurrentFormat.BorderRect();
		if (wBorderRectRef != 0) {
			return(m_AllocatorBorderRect(wBorderRectRef));
		};
		return(nullptr);
	};

	tBool tFormatRoot::DeleteBorderRect(tFormatRef sBorderRectRef) {
        if (sBorderRectRef != 0) {
            tBorderRectCss* wBorderRect=BorderRect(sBorderRectRef);
            wBorderRect->Dec();
            if (wBorderRect->Empty()) {
                m_AllocatorBorderRect.Delete(sBorderRectRef);
            }
        }
		return(false);
	}

    void tFormatRoot::BeginWriteJson() {
        m_MapJsonCell.clear();
        m_VectorFormatCell.clear();
    }
    tSize tFormatRoot::WriteJsonAddFormat(tFormatRef sFormatRef) {
        if (sFormatRef == 0) {
            return 0;
        }
        tMapJsonCell::iterator wIterator;
        
        wIterator=m_MapJsonCell.find(sFormatRef);
        if (wIterator==m_MapJsonCell.end()) {
            tFormatCss* wFormatCss= m_AllocatorFormat(sFormatRef);
            if (wFormatCss == nullptr) {
                cerr << "WriteJsonAddFormat: dangling FormatRef " << sFormatRef
                     << " (format purged while cell still holds Css)" << endl;
                return 0;
            }
            m_VectorFormatCell.push_back(wFormatCss->Str(this));
            m_MapJsonCell[sFormatRef]=m_VectorFormatCell.size()-1;
            return(m_VectorFormatCell.size()-1);
        }
        
        return((*wIterator).second);
    }

    tString tFormatRoot::ReadJsonGetFormat(tSize sIndex) {
        if (sIndex >= m_VectorFormatCell.size()) {
            return("");
        }
        return(m_VectorFormatCell[sIndex]);
    }


    void tFormatRoot::JsonSpreadSheet(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("formats");
        sWriter->StartArray();
        tVectorString::iterator wIterator;
       
        for (wIterator = m_VectorFormatCell.begin(); wIterator != m_VectorFormatCell.end(); wIterator++) {
            sWriter->StartObject();

            sWriter->Key("f");
            sWriter->String((*wIterator).c_str());
            //tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
            //wFormatCss->Json(sWriter, this);
            sWriter->EndObject();
        }
        sWriter->EndArray();
        sWriter->EndObject();

    }

    void tFormatRoot::JsonSpreadSheet(const rapidjson::Value& sValue) {
        const Value& wFormats = sValue["formats"];
        assert(wFormats.IsArray());
        m_MapJsonCell.clear();
        m_VectorFormatCell.clear();
        //cout << "wFormat size " << wFormats.Size() << endl;
        for (SizeType wIndex = 0; wIndex < wFormats.Size(); wIndex++) {
            const Value& wValue = wFormats[wIndex];
            if (wValue.HasMember("f")) {
                tString wFormatString = wValue["f"].GetString();
                m_VectorFormatCell.push_back(wFormatString);
            }
        }
    }

    void tFormatRoot::Json(Writer<StringBuffer>* sWriter) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("formats");
        wWriter.StartArray();
        tMapKey::iterator wIterator;
        for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
            wWriter.StartObject();
            wWriter.Key("k");
            tString wKey = (*wIterator).first;
            wWriter.String(wKey.c_str());
            
            wWriter.Key("f");
            tFormatRef wRef = (*wIterator).second;
            tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
            wFormatCss->Json(&wWriter, this);
            wWriter.EndObject();
        }
        wWriter.EndArray();
        wWriter.EndObject();
    }

	void tFormatRoot::Json(const rapidjson::Value& sValue) {
        const Value& wFormats = sValue["formats"];
        assert(wFormats.IsArray());
        
        for (SizeType wIndex = 0; wIndex < wFormats.Size(); wIndex++) {
            const Value& wValue = wFormats[wIndex];
			tString wKey="";
			if (wValue.HasMember("k")) {
				wKey=wValue["k"].GetString();
			}
            if (wValue.HasMember("f")) {
                AllocFormat(wKey);
				m_CurrentFormat.Json(wValue["f"], this);
				Validate();
            }
        }
    }

	tString tFormatRoot::WriteJson() {
		StringBuffer wStringBuffer;
		Writer<StringBuffer> wWriter(wStringBuffer);
        JsonSpreadSheet(&wWriter);
		return(wStringBuffer.GetString());
	}

	void tFormatRoot::ReadJson(tString sJson) {
        Reset();
		Document wDocument;
		wDocument.Parse(sJson.c_str());
		Json(wDocument);
	}

    const tRecColor* tFormatRoot::FindColorByName(tString sName) {
        tMapNameColor::iterator wIterator;
        wIterator = m_MapNameColor.find(sName);
        if (wIterator!=m_MapNameColor.end()) {
            return((*wIterator).second);
        }
        return(nullptr);
    }

    const tRecColor* tFormatRoot::FindColorByColor(tUInt sColor) {
        tMapColor::iterator wIterator;
        // Add Opacity 1
        // If no alpha specified, force full opacity (0xFF)
        if ((sColor & 0xFF000000) == 0) {
            sColor |= 0xFF000000;
        }
        wIterator = m_MapColor.find(sColor);
        if (wIterator!=m_MapColor.end()) {
            return((*wIterator).second);
        }
        return(nullptr);
    }

    tFormatRef tFormatRoot::Count() {
        return(tFormatRef(m_MapFormat.size()));
    };

	// Debug ==================================================================
#ifdef _DEBUGSK
	tString tFormatRoot::DebugFormat() {
		tStringStream wStream;

		tMapKey::iterator wIterator;
		for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
			tString wKey = (*wIterator).first;

			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
			if (wFormatCss != nullptr) {
				wStream << wKey << "[" << wFormatCss->RootKey(this) << "]." << wFormatCss->Debug(this) << " Nb:" << wFormatCss->CountCell() << " ref:" << wRef << endl;
			}
		}

		return(wStream.str());
	}

	tString tFormatRoot::DebugMargin() { return(m_AllocatorMargin.Debug()); }
	tString tFormatRoot::DebugPadding() { return(m_AllocatorPadding.Debug()); }
	tString tFormatRoot::DebugShadow() { return(m_AllocatorShadow.Debug()); }
	tString tFormatRoot::DebugFont() { return(m_AllocatorFont.Debug()); };
	tString tFormatRoot::DebugText() { return(m_AllocatorText.Debug()); }
	tString tFormatRoot::DebugBorderRect() { return(m_AllocatorBorderRect.Debug()); }

#endif // _DEBUGSK

#ifdef checkfo

    void tFormatRoot::ResetCheckCell() {
        tMapKey::iterator wIterator;
        // Loop and reset =====================================================
        for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
            tFormatRef wRef = (*wIterator).second;
            tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
            if (wFormatCss->FormatCell()) {
                wFormatCss->ResetCellCheck();
            }
        }
    };

    void tFormatRoot::CheckCell() {
        tMapKey::iterator wIterator;
        // Loop and reset =====================================================
        for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
            tFormatRef wRef = (*wIterator).second;
            tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
            if (wFormatCss->FormatCell()) {
                wFormatCss->CheckCell(this);
            }
        }
    };

    void tFormatRoot::Check() {
		tMapKey::iterator wIterator;
		// Loop and reset =====================================================
		for (wIterator = m_MapFormatPool.begin(); wIterator != m_MapFormatPool.end(); wIterator++) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
			wFormatCss->ResetCheck(this);
		}
		// Loop and check =====================================================
        // Usee m_MapFormat because MapFormat folow nb on instance
		for (wIterator = m_MapFormat.begin(); wIterator != m_MapFormat.end(); wIterator++) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
            wFormatCss->IncCheck(this);
    	}
		for (wIterator = m_MapFormatPool.begin(); wIterator != m_MapFormatPool.end(); wIterator++) {
			tFormatRef wRef = (*wIterator).second;
			tFormatCss* wFormatCss = m_AllocatorFormat(wRef);
			wFormatCss->Check(this);
		}
	}
#endif

	void DoneFormatRoot() {
		if (tFormatRoot::m_StaticFormatRoot != nullptr) {
			tFormatRoot::m_StaticFormatRoot->Clear();
			delete(tFormatRoot::m_StaticFormatRoot);
			tFormatRoot::m_StaticFormatRoot = nullptr;
		}
	}

} // end of namespace
