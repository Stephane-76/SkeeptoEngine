//=============================================================================
// SkRoot Application
//=============================================================================
#include "../include/SkApplication.hpp"

#define  _DEBUGSKcircularmemory
namespace SkRoot {
	// Constant circular memory ===============================================
	// Pay attention to alignment
	#if defined(__clang__) || defined(__EMSCRIPTEN__)
	#define SkAlignMemory 8 
	#define SIZE_CIRCULAR_MEMORY 1024 * 512 // 512 Ko
	#else
	#define SIZE_CIRCULAR_MEMORY 1024 * 1000 // 1Go
	#endif

	// Circular Memory ========================================================
	tCircularMemory::tCircularMemory() {
		m_Pos = 0;
		m_Buffer = (tChar*) malloc(SIZE_CIRCULAR_MEMORY);
	};

	tCircularMemory::~tCircularMemory() {
		Clear();
	};

	void tCircularMemory::Clear() {
		if (m_Buffer!=nullptr) free(m_Buffer);
		m_Buffer = nullptr;
	}


	void* tCircularMemory::Alloc(tSize sSize) {
		// Align current write position first
		tSize alignedPos = m_Pos;
	#ifdef SkAlignMemory
		alignedPos = (alignedPos + (SkAlignMemory - 1)) & ~(SkAlignMemory - 1);
	#endif

		// Wrap if not enough space for allocation
		if (alignedPos + sSize >= SIZE_CIRCULAR_MEMORY) {
	#ifdef debugcircularmemory
			cout << "Reset Circular memory" << endl;
	#endif
			alignedPos = 0;
		}

		// Prepare next position (+1 keeps a safety byte as in original code)
		m_Pos = alignedPos + sSize + 1;
	#ifdef SkAlignMemory
		m_Pos = (m_Pos + (SkAlignMemory - 1)) & ~(SkAlignMemory - 1);
	#endif

	#ifdef debugcircularmemory
		cout << "Alloc CM:" << m_Pos << "," << sSize << " aligned at " << alignedPos << endl;
	#endif
		return &m_Buffer[alignedPos];
	}
	// ========================================================================
	// The application is based on this variable ==============================
	static tApplication StaticApplication;
	//=========================================================================

	tApplication::tApplication() : tVirtualClass(), m_Global(nullptr), m_SharedStringContainer(nullptr), m_UndoRedoContainer(),m_Clipboard(),m_DateFormatter(nullptr),m_NumberFormatter(nullptr) {
		m_SharedStringContainer = new tSharedStringContainer();
		m_Global = new tTokens();
		m_ClassFactory = new tClassFactory();
        m_Locale=new tLocale();
        m_Locale->Lang("fr");
	}

	tApplication::~tApplication() {
		Clear();
		delete(m_ClassFactory);
#ifdef _DEBUGLeak
		// Only report once, when destroying the singleton (avoids double report for tApplication-derived instances like tLexerData)
		if (this == Instance() && ReportLeakAtExit()) {
			DebugMemory();
		}
#endif
	};

	void tApplication::Clear() {
        if (m_DateFormatter!=nullptr) {
            delete(m_DateFormatter);
            m_DateFormatter=nullptr;
        }
        if (m_NumberFormatter!=nullptr) {
            delete(m_NumberFormatter);
            m_NumberFormatter=nullptr;
        }
        if (m_Locale!=nullptr) {
            delete(m_Locale);
            m_Locale=nullptr;
        }
       
        m_FormatStringRoot.Clear();
		m_UndoRedoContainer.Clear();
		if (m_SharedStringContainer != nullptr) {
			delete(m_SharedStringContainer);
			m_SharedStringContainer = nullptr;
		}
		if (m_Global != nullptr) {
			delete(m_Global);
			m_Global = nullptr;
		}
		m_CircularMemory.Clear();
        
        if (m_ClassFactory!=nullptr) {
            delete(m_ClassFactory);
            m_ClassFactory=nullptr;
        }
	}

	// Global Management ======================================================
	tBool tApplication::AddGlobal(tString sKey, tVirtualClass* sVirtualClass) {
		return(m_Global->Add(sKey, sVirtualClass));
	}
	
	tBool tApplication::DeleteGlobal(tString sKey) {
		return(m_Global->Delete(sKey));
	}

	tVirtualClass* tApplication::Global(tString sKey) {
		return(m_Global->Token(sKey));
	}
	
	// Methods for heap ========================================================
	void* tApplication::AllocMem(tSize sSize) {
		return(malloc(sSize));
	}
	void* tApplication::ReAllocMem(void* sVoid, tSize sSize) {
		return(realloc(sVoid, sSize));
	}
	void  tApplication::FreeMem(void* sVoid) {
		free(sVoid);
	}

	void* tApplication::AllocTemporary(tSize sSize) {
		return(m_CircularMemory.Alloc(sSize));
	}

	// Undo Redo ======================================================
    tUndoRedoContainer* tApplication::UndoRedoContainer() {
        return(&m_UndoRedoContainer);
    }
	tBool tApplication::Do(tUndo* sUndo) {
		return(m_UndoRedoContainer.Do(sUndo));
	}
	tBool tApplication::Undo() {
		return(m_UndoRedoContainer.Undo());
	}
	tBool tApplication::Redo() {
		return(m_UndoRedoContainer.Redo());
	}

	void tApplication::ClearUndoRedo() {
		m_UndoRedoContainer.Clear();
	}


	tUndo* tApplication::LastUndo() {
		return(m_UndoRedoContainer.LastUndo());
	}

    tUndo* tApplication::LastRedo() {
        return(m_UndoRedoContainer.LastRedo());
    }

    tVectorUndo tApplication::GetUndoVector() {
        return(m_UndoRedoContainer.GetUndoVector());
    }

    tVectorUndo tApplication::GetRedoVector() {
        return(m_UndoRedoContainer.GetRedoVector());
    }

#ifdef _DEBUGSK
    tString  tApplication::DebugUndo() { return(m_UndoRedoContainer.DebugUndo()); }
    tString  tApplication::DebugRedo() { return(m_UndoRedoContainer.DebugRedo()); }
#endif


    void tApplication::Locale(tString sLang) {
        m_Locale->Lang(sLang);
        // Locale affects excelnumber output separators (SkFormatNumber); drop cached parsers.
        if (m_NumberFormatter != nullptr) {
            m_NumberFormatter->ClearPool();
        }
    }

    tLocale*  tApplication::Locale() { return(m_Locale); }

    tFormatStringRoot* tApplication::FormatStringRoot() {
        return(&m_FormatStringRoot);
    }

    tDateFormatter*  tApplication::DateFormatter() {
        if (m_DateFormatter==nullptr) m_DateFormatter=new tDateFormatter();
        return(m_DateFormatter);
    }

    tNumberFormatter*  tApplication::NumberFormatter() {
        if (m_NumberFormatter==nullptr) m_NumberFormatter=new tNumberFormatter();
        return(m_NumberFormatter);
    }

	tClipboard* tApplication::Clipboard() {
		return(&m_Clipboard);
	}

	void tApplication::TimerStart() {
		m_VectorTimes.push_back(std::clock());
	};
	
	clock_t tApplication::TimerElapsed() {
		return(std::clock()- m_VectorTimes.back());
	}

	void tApplication::TimerStop() {
		m_VectorTimes.pop_back();
	};

	void tApplication::TimerReset() {
		m_VectorTimes.clear();
		TimerStart();
	}

	
	tSize tApplication::NbSharedString() {
		return(m_SharedStringContainer->NbSharedString());
	}

	void tApplication::ConsoleLog(tString sString) {
	#ifndef __EMSCRIPTEN__
		printf("%s",sString.c_str()); 
	#else
		printf("%s",sString.c_str());
	#endif	
	}		

	void tApplication::ConsoleWarning(tString sString) {
#ifndef __EMSCRIPTEN__
		printf("Warning %s", sString.c_str());
#else
		printf("Warning %s", sString.c_str());
#endif	
	}

	void tApplication::ConsoleError(tString sString) {
	#ifndef __EMSCRIPTEN__
		printf("Error %s",sString.c_str()); 
	#else
		printf("Error %s",sString.c_str());
	#endif	
	}		

	tClassFactory* tApplication::ClassFactory() {
		return(m_ClassFactory);
	}

	tApplication* tApplication::Instance() {
		return (&StaticApplication);
	}
	

}; // end of namespace

