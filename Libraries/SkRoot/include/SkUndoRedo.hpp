//=============================================================================
// Sker Undo Redo 
//=============================================================================
#ifndef SkUndoRedo_hpp
#define SkUndoRedo_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkSharedString.hpp"
namespace SkRoot {
	class tUndoList;

	const tInt SkMaxUndoRedo = 100;

 
	//=========================================================================
	//! Ancestor class contains extra information for UndoRedo
	class tUndoExtra : public tVirtualClass {
	private:
	public:
		tUndoExtra();
	};

	//=========================================================================
	//! Ancestor class to undo and redo
	class tUndo : public tVirtualClass {
	private:
		//! Name of Operation
		tSharedString m_OperationName;
		//! Owner Extra Information (position, selection in spreadsheet by example)
		tUndoExtra* m_Extra;
	public:
		/// @brief constructor of SkUndo.
		tUndo();

		/// @brief destructor of SkUndo.
		virtual ~tUndo();

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		virtual tBool Do();

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		virtual tBool Undo();

		/// @brief      Set Operation name.
		/// @param[in]  sValue tString
		void OperationName(tString sValue);

		/// @brief      Get Operation name.
		/// @return		tString
		tString OperationName();

		/// @brief      Set Extra Undo.
		/// @param[in]  sExtra tUndoExtra*
		void Extra(tUndoExtra* sExtra);

		/// @brief      Get Extra Undo.
		/// @return		tUndoExtra*
        tUndoExtra*  Extra();

        virtual tString Debug();
	};

	typedef std::deque<tUndo*> tDequeUndo;
	typedef std::stack<tUndo*> tStackUndo;

    typedef std::vector<tUndo*> tVectorUndo;

    typedef void tOnDeleteUndo(tUndo*);

	//=========================================================================
	//! Class managing undo redos
	class tUndoRedoContainer : public tClass {
	private:
        tSize           m_MaxUndoRedo;
        //! Contains the undos
        tDequeUndo		m_DequeUndo;
        //! Contains the redos
        tStackUndo		m_StackRedo;
        //! CallBack on delete
        tOnDeleteUndo*   m_OnDeleteUndo;
	public:
		/// @brief constructor of SkUndoRedoContainer.
		tUndoRedoContainer();

		/// @brief destructor of SkUndoRedoContainer.
		~tUndoRedoContainer();

		/// @brief cleal all undo redo.
		void Clear();

		/// @brief clear all undo.
		void ClearUndo();

		/// @brief clear all redo.
		void ClearRedo();

		/// @brief      Apply do.
		/// @param[in]  sUndo SkUndo*
		/// @return     tBool false if not Ok
		tBool Do(tUndo* sUndo);

		/// @brief Push undo after Do() was already executed on sUndo.
		tBool CommitDo(tUndo* sUndo);

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo();

        /// @brief Pop last undo onto redo stack without calling Undo() (multi-user relay).
        tBool PopUndoToRedoWithoutExecute();

        /// @brief Pop redo stack head onto undo deque without calling Do() (spreadsheet dispatch).
        tUndo* PopRedoToUndoDeque();

        /// @brief Remove last undo from deque without Undo(); caller owns the pointer.
        tUndo* DetachLastUndo();

		/// @brief      Apply Redo.
		/// @return     tBool false if not Ok
		tBool Redo();
        
        // Max UndoRedo =======================================================
        ///Brief Set  MaxUndo
        ///@param[in] sMaxUndoRedp tSize
        void MaxUndo(tSize sMaxUndoRedo);
        
        ///Brief Set  MaxUndo
        ///@return  tSize
        tSize MaxUndo();
        
        ///Brief Set  OnDeleteUndo
        ///@param[in] sOnDeleteUndo tOnDeleteUndo)
        void OnDeleteUndo(tOnDeleteUndo* sOnDeleteUndo);
        
        /// InterfaceUndoRedo ========================================================
 		/// @brief      Get Last Undo.
		/// @return     SkUndo*
		tUndo* LastUndo();
        
        /// @brief      Get Last Redo.
        /// @return     SkUndo*
        tUndo* LastRedo();

        /// @brief      Get undo vector .
        /// @return     tVectorUndio
        tVectorUndo GetUndoVector();

        /// @brief      Get  redo  vector .
        /// @return     tVectorUndio
        tVectorUndo GetRedoVector();
        
#ifdef _DEBUGSK
        tString DebugUndo();
        
        tString DebugRedo();

#endif
	};
}

#endif
