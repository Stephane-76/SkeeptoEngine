//=============================================================================
// Skeema Application
/*!
 * @mainpage
##	Application support
###		Allows to hang all the elements necessary for the execution.
@par
-	Types (See SkTypes.hpp).
-	Globals Variables in Dictionary (see SkTokens.hpp).
-	Shared string (see tSharedString.hpp).
-	Boxing simple type. (for attach method to type like tString.Ltrim(); see SkTypesClass.hpp)
-	Circular memory (SkApplication.hpp).
-	Sparse array (template see SkSparseArray.hpp).
-	Allocator of class (template see SkAllocator.hpp).
-	File management (see SkFile.hpp).
-	Undo Redo (see SkUndoRedo.hpp).
.......
@par	One Class of SkApplication for one executable or dynamic library

###	This is the root library for all skeema applications.
*/
//=============================================================================

#ifndef SkApplication_hpp
#define SkApplication_hpp

#include "SkClass.hpp"
#include "SkUtf.hpp"
#include "SkTypesClass.hpp"
#include "SkTokens.hpp"
#include "SkSharedString.hpp"
#include "SkVariant.hpp"
#include "SkException.hpp"
#include "SkFile.hpp"
#include "SkClipboard.hpp"
#include "SkSparseArray.hpp"
#include "SkAllocator.hpp"
#include "SkUndoRedo.hpp"
#include "SkModelClass.hpp"
#include "SkGenericClass.hpp"
#include "SkFormatApi.hpp"
#include "SkFormatString.hpp"
#include "SkLocale.hpp"
#include "SkFormatDate.hpp"
#include "SkFormatNumber.hpp"

namespace SkRoot {
	//==========================================================================
	//! For circular memory. Extremely fast circular memory blocks are never removed (Warning, lethal overflow ..)
	class tCircularMemory {
	private:
		tSize m_Pos;
		tChar* m_Buffer;  
	public:
		tCircularMemory();
		~tCircularMemory();
		void Clear();

		void* Alloc(tSize sSize);
	};

	//==========================================================================
	//! Application Root : One class for all instance of application
	class tApplication : public tVirtualClass {
		private:
			//! Elements attached to the application (Dictionary String tVirtualClass)
			SkRoot::tTokens* m_Global;

			//! Class Factory for virtual class
			tClassFactory*  m_ClassFactory;

			//! Shared String 
			tSharedStringContainer* m_SharedStringContainer;
			//! Circular Memory(old memory of Nat System! a very good idea)
			tCircularMemory			m_CircularMemory;
			//! Times Get
			vector<clock_t>			m_VectorTimes;
			//! Undo Redo System
			tUndoRedoContainer		m_UndoRedoContainer;
        
            //! Locale
            tLocale*                m_Locale;
        
            //! Format String
            tFormatStringRoot       m_FormatStringRoot;
        
			//! ClipBoard
			tClipboard				m_Clipboard;

			//! Format Date
            tDateFormatter* 		m_DateFormatter;

			//! Format Number
			tNumberFormatter*		m_NumberFormatter;
	public:
			tApplication();
			virtual ~tApplication();
			void Clear();

			// Global Management ==============================================
			/// @brief      Add tVirtualClass in the Application.
			/// @param[in]  sKey unique for the system
			/// @param[in]  sVirtualClass for add derived sVirtualClass 
			/// @return     tBool false an element already exist
			tBool AddGlobal(tString sKey, tVirtualClass* sVirtualClass);

			/// @brief      Delete tVirtualClass in the Application.
			/// @param[in]  sKey unique for the system
			/// @return     tBool false element don't exist
			tBool DeleteGlobal(tString sKey);

			/// @brief      Get  tVirtualClass in the Application.
			/// @param[in]  sKey unique for the system
			/// @return     tVirtualClass* if nullptr element don't exist
			tVirtualClass* Global(tString sKey);

			// Memory Management =============================================
			/// @brief      Alloc memory for tApplication.
			/// @param[in]  sSize Size of allocated buffer
			/// @return     void* return new pointer
			void* AllocMem(tSize sSize);

			/// @brief      Realloc memory for tApplication.
			/// @param[in]  sVoid void* pointer to realloc
			/// @param[in]  sSize Size of allocated buffer
			/// @return     void* return new pointer
			void* ReAllocMem(void* sVoid, tSize sSize);

			/// @brief      Free memory for SkApplication.
			/// @param[in]  sVoid void* pointer to delete
			void  FreeMem(void* sVoid);

			/// @brief      Alloc mem on circular memory. Please note that the life of the pointer may be short.
			/// @param[in]  sSize Size of allocated buffer
			/// @return     void* return new pointer
			void* AllocTemporary(tSize sSize);


			// Undo Redo ======================================================
        /// @brief      Apply do.
        /// @return    tUndoRedoContainer*
            tUndoRedoContainer* UndoRedoContainer();

            /// @brief      Apply do.
			/// @param[in]  sUndo SkUndo*
			/// @return     tBool false if not Ok
			tBool Do(tUndo* sUndo);

			/// @brief      Apply Undo.
			/// @return     tBool false if not Ok
			tBool Undo();

			/// @brief      Apply Redo.
			/// @return     tBool false if not Ok
			tBool Redo();

			/// @brief      Clear UndoRedo.
			void ClearUndoRedo();

			/// @brief      Get LastUIndo.
			/// @return     SkUndo*
			tUndo* LastUndo();
        
            /// @brief      Get Last Redo.
            /// @return     SkUndo*
            tUndo* LastRedo();

            /// @brief      Get undo vector .
            /// @return     tVectorUndo
            tVectorUndo GetUndoVector();

            /// @brief      Get  redo  vector .
            /// @return     tVectorUndo
            tVectorUndo GetRedoVector();
        
        
#ifdef _DEBUGSK
            /// @brief      ÔÔUndo .
            /// @return     tString
            tString DebugUndo();
            
            /// @brief      DebugUndo .
            /// @return     tString
            tString DebugRedo();
#endif


            // Locale ========================================================
            /// @brief      Set locale country
            /// @param[in]  tString String
            void Locale(tString sLang);
        
            /// @brief      Get locale .
            /// @return     tLocale*
            tLocale*  Locale();

            /// @brief      Get  FormatStringRoot.
            /// @return     tFormatStringRoot*
            tFormatStringRoot*  FormatStringRoot();
            
			// Clipboard ======================================================
			/// @brief      Get Clipboard.
			/// @return     tClipboard* 
			tClipboard* Clipboard();

			// Times ==========================================================
			/// @brief      Start the timer.
			void TimerStart();

			/// @brief      Return Elapsed times 
			/// @return     clock_t  
			clock_t TimerElapsed();

			/// @brief      Stop timer
			void TimerStop();

			/// @brief      Reset timer : Stop and Start
			void TimerReset();

			// Shared String Statistics =======================================
			/// @brief      Return Nb of shared string  
			/// @return     tSize 
			tSize NbSharedString();

			// Format Date =====================================================
			/// @brief      Return Format Date Root
			/// @return    tDateFor
            tDateFormatter* DateFormatter();

			// Format Number ===================================================
			/// @brief      Return Format Number Root
			/// @return     tFormatNumberRoot*
			tNumberFormatter* NumberFormatter();

			// Treatment of Console ===========================================
			/// @brief      Print String in console
			/// @param[in]  tString String
			static void ConsoleLog(tString sString);		

			// Treatment of Warning ===========================================
			/// @brief      Print String in console warning
			/// @param[in]  tString String
			static void ConsoleWarning(tString sString);

			// Treatment of Error =============================================
			/// @brief      Print String in console error
			/// @param[in]  tString String
			static void ConsoleError(tString sString);		

			/// @brief      Return ClassFactory
			/// @return     tClassFactory* (pointer of unique tClassFactory)
			tClassFactory* ClassFactory();

			// Treatment of Singleton =========================================
			/// @brief      Return Singleton of tApplication
			/// @return     tApplication* (pointer of unique tApplication) 
			static tApplication* Instance();
	};

}

#endif
