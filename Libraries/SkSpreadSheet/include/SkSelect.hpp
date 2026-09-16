//=============================================================================
// SkSpreadSheet Selection Range and Cell
//=============================================================================
#ifndef SkSelect_hpp
#define SkSelect_hpp

#include <SkUndoRedo.hpp>

#include "SkLemonToken.hpp"
#include "SkLexerSpreadSheet.hpp"
#include "SkUndoRedoRebase.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {
	class tUndoSpreadSheetCallBack;
	class tCopy;
	//=========================================================================
	//! Select for tCell and tRange for Set raz ....
	//! Select contains Tempo Element (Circurlar memory don't Keep after Operation)
	//! Select Parse for rehydatre (See SkUndoRedoSave)
	class tSelect  {
	private:
		//! Vector of Select tPoint and tRange
		tVectorTempoPoint m_VectorSelect;
	public:
		/// @brief		Constructor SkSelect.
		tSelect();

		/// @brief		Destructor SkSelect.
		virtual ~tSelect();

		void Clear();
        
        /// @brief      Return Reference string.
        /// @return        tString
        const tString StrRef() const;

		/// @brief		Parse selection return false is not Ok.
		/// @param[in]	sSelection
		/// @return		tBool
		tBool Parse(tString sSelection);

		/// @brief		Call back for Do & Undo.
		/// @param[in]	sUndoSpreadSheet
		/// @return		tBool
        tBool CallBack(tUndoSpreadSheetCallBack* sUndoSpreadSheet);

		/// @brief		Call back for Do & Undo.
		/// @param[in]	sUndoSpreadSheet
		/// @return		tBool
		tBool CallBack(tCopy* sCopy);

		/// @brief		Return true is Just one Cell.
		/// @return		tBool
		tBool JustOneCell() const;
		
		/// @brief		Return true is Just one Rect.
		/// @return		tBool
		tBool JustOneRect() const;

		/// @brief		Return TempoPoint to first point.
		/// @return		tTempoPoint*
		tTempoPoint*  FirstPoint() const;

		/// @brief		Return TempoPoint to first point.
		/// @return		tTempoRect*
		tTempoRect* FirstRect() const;

		/// @brief		Return VectorPoint or Rect.
		/// @return		tVectorTempoPoint*
		tVectorTempoPoint* VectorSelect() const;

		/// @brief		Return TopLeft point (paste copy).
		/// @return		tBool
		tTempoPoint* ReturnTopLeftPoint();
		
		// Rebase ===============================================================
		/// @brief		Rebase all points and ranges in the selection using rebase plan.
		/// @param[in]	sRebasePlan const tRebasePlan&
		/// @return		tBool true if rebase successful, false if any element was deleted
		tBool Rebase(const tRebasePlan& sRebasePlan);

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const Value& sValue);

#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        virtual tString Debug();
#endif
	};
		
}

#endif
