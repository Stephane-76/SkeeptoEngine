//=============================================================================
// SkSpreadSheet Container of WorkBook
//=============================================================================
#ifndef SkSpreadSheet_hpp
#define SkSpreadSheet_hpp

#include <vector>
#include <unordered_map>
#include "SkWorkBook.hpp"
#include "SkFormatApi.hpp"
#include "SkUndoRedo.hpp"
#include "SkJsonKey.hpp"
#include "SkTools.hpp"

namespace SkSpreadSheet {

	// Forward declarations
	class tWorkBook;
	typedef std::vector<tWorkBook*> tVectorWorkBookClass;


    //! CallBack CellChange ======================================================
    typedef void tOnCellChange(tCell*,tVariant&);
	

    
    //! Class container ========================================================
	class tSpreadSheetContainer : tClass {
	public:
		typedef tAllocator<tWorkBook, tAllocatorRef, 1> tAllocatorWorkBook;
		typedef vector<tAllocatorRef> tVectorWorkBook;
	private:
		//! Allocator for workBook
		tAllocatorWorkBook		m_AllocatorWorkBook;

		
		//! Active WorkBook Pointer
		tWorkBook*				m_ActiveWorkBook;
        
		//! Vector of index of WorkBook on allocator
		tVectorWorkBook 	    m_VectorWorkBook;
        
        //! Format Api
        tFormatApi*             m_FormatApi;

		//! Function dictionary
		tFunctionDictionary	    m_FunctionDictionary;

		//! Used to compile a cell
		tLemonInterface		    m_LemonInterface;

		//! Shared Formula container
		tSharedFormulaPool  m_SharedFormulaPool;
  
        // Shared Element in Json File
        tJsonSharedString  m_JsonSharedString;
        tJsonSharedString  m_JsonSharedFormula;

        
        //! CallBack CellChange ======================================================
        static tOnCellChange*  m_OnCellChange;
        
		/// @brief		Find iterrator by uri.
		/// @param[in]	sUri tString
		/// @return		tVectorInt::iterator 
		tVectorWorkBook::iterator FindWorkBookByIterator(tString sUri);
        
        // Json ==============================================================
        typedef  std::deque<tCell*> tContainerCell;
        tContainerCell m_ContainerJsonCell;
        //! Persisted v/t from .sker for formula cells (ReadJson defers EndCalculate when set).
        std::unordered_map<tCell*, tVariant> m_JsonCachedFormulaValues;
        tBool m_JsonRestoreCachedFormulaValues;
        //! When true, JsonEnd() prints split compile/calculate timings (Pressure benchmark only).
        tBool m_JsonEndProfile = false;

        //! Get Cell active  while Json operation
        tCell* m_CurrentJsonCell;

        void CompileQueuedJsonCells(tVectorCell* sVectorCellCalculate, tSize* sTotalFormulaCells = nullptr);
	public:
		/// @brief		Constructor.
		tSpreadSheetContainer();

		/// @brief		Destructor.
		~tSpreadSheetContainer();

		/// @brief		Clear.
		void Clear();
        
        
        /// @brief      Clear All Undo Redo of AllSheet
        void ClearUndoRedo();
        
		/// @brief		Active WorkBook.
		/// @param[in]	sUri tString
		tBool ActiveWorkBook(tString sUri);

		/// @brief		Active WorkBook.
		/// @param[in]	s>WorkBook SkWorBook*
		void ActiveWorkBook(tWorkBook* sWorkBook);
        
        //! Return FormatApi in ActiveWorkBook
        tFormatApi* FormatApi();

        //! Return FormatApi in ActiveWorkBook
        void FormatApi(tFormatApi* sFormatApi);

		/// @brief		Get active WorkBook.
		/// @return		tWorkBook* 
		tWorkBook* ActiveWorkBook();

		/// @brief		Get WorkBook by Ref.
		/// @return		tWorkBook* 
		tWorkBook* WorkBook(tAllocatorRef sRef);

		/// @brief      Add WorkBook.
		/// @param[in]	sUri tString
        /// @return     tWorkBook*
		tWorkBook* AddWorkBook(tString sUri);

		/// @brief      Find WorkBook by Uri.
		/// @param[in]	sUri tString
        /// @return     tWorkBook*
		tWorkBook* FindWorkBook(tString sUri);
        
        /// @brief      Delete WorkBook by Uri.
        /// @param[in]  sUri tString
        /// @return     tBool
        tBool DeleteWorkBook(tString sUri);
        
        /// @brief      Delete WorkBook by Uri.
        /// @param[in]  sUri tString
        /// @param[in]  sUriTo tString
        /// @return     tBool
        tBool RenameWorkBook(tString sUri,tString sToUri);
        
        /// @brief      Get list of WorkBook
        /// @param[out] sVectorWorkBookClass tVectorWorkBookClass&
        void WorkBooksList(tVectorWorkBookClass& sVectorWorkBookClass);
        
        /// @brief      Json Get llis URI t of WorkBooks
        /// @param[in]  sWriter Writer<StringBuffer>* 
        void JsonWorkBooksList(Writer<StringBuffer>* sWriter);

		/// @brief      Return dictionary of function (see SkFunction.hpp).
		/// @return		tVectorSheet*
		tFunctionDictionary* FunctionDictionary();

		/// @brief		Return Lemon interface (For test). 
		/// @return		SkLemonInterface*
		tLemonInterface* LemonInterface();

		//! Return Shared Formula container
		/// @return		tSharedFormulaPool*
		tSharedFormulaPool* SharedFormulaPool();

		/// @brief      Return number of shared formula.
		/// @return		tIndex
		tIndex NbSharedFormula();
        
        // Check
#ifdef checksp
        /// @brief Check.
        void Check();
#endif
#ifdef checkfo
        /// @brief        Operator for sort.
        /// @param[in]    sUndo tUndo*
        void CheckFormatUndo(tUndo* sUndo);
        
        /// @brief Check format
        void CheckFormat();
#endif
#ifdef _DEBUGSK
        /// @brief      Debug
        tString Debug();
#endif
        // Extra =============================================================
        static tBool IsOnCellChange();
        static void SetOnCellChange(tOnCellChange* sOnCellChange);
                                                    
        static void OnCellChange(tCell* Cell,tVariant& sValue);

        // Json ===============================================================
        /// @brief      Intialiize compil and calculate
        void JsonBegin();
        /// @brief      Push sCell for future compil and calculate
        /// @param[in]  sCell tCell*
        /// @param[in]  sCachedValue optional persisted display value (ReadJson defer-recalc)
        void PushJsonCell(tCell* sCell, const tVariant* sCachedValue = nullptr);
        /// @brief      ReadJson: restore persisted v/t on formula cells instead of EndCalculate.
        void SetJsonRestoreCachedFormulaValues(tBool sValue);
        /// @brief      Compil  and calculate
        void JsonEnd();
        /// @brief      Enable split compile/calculate timing logs inside JsonEnd (benchmark only).
        /// @param[in]  sValue tBool
        void JsonEndProfile(tBool sValue) { m_JsonEndProfile = sValue; }
        /// @brief      Close JsonShared pools only (copy/export); no JsonCompil or recalc.
        void JsonEndShared();
        /// @brief      Compil/calculate PushJsonCell queue only (paste); skip JsonCompil named ranges.
        void JsonEndCellsOnly();

        /// @brief      Set Json Cell .. (SkCellClass.
        /// @param[in]  sCell tCell*
        void CurrentJsonCell(tCell* sCell);
        
        /// @brief    Return  json Cell (SkCellClass)
        /// @return   tCell*
        tCell* CurrentJsonCell();
        
           ///@ brief Return JsonSharedString
        ///@return tJsonSharedString
        tJsonSharedString*  JsonSharedString();

        ///@ brief Return JsonSharedString
        ///@return tJsonSharedString
        tJsonSharedString*  JsonSharedFormula();
        
          // Json ==============================================================
        /// @brief		Writer Json. 
        /// @param[in]	sWriter Writer<StringBuffer>*
        void JsonShared(Writer<StringBuffer>* sWriter) const;


        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        void JsonShared(const rapidjson::Value& sValue);
        
        
        
        /// @brief        Get list of sheets.
        /// @param[in]    sWriter Writer<StringBuffer>* 
        void JsonSheets(Writer<StringBuffer>* sWriter);

        /// @brief        Get list of sheets.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void JsonInfo(Writer<StringBuffer>* sWriter);
        
        
		/// @brief      Return Instance of SkSpreadSheet.
		/// @return		SkSpreadSheet*
		static tSpreadSheetContainer* Instance();
	};

	typedef tCell* tCellStableRef;
	inline tCell* CellFromStableRef(tCell* p) { return p; }
	inline tCellStableRef StableRefFromCell(tCell* c) { return c; }

	/// @brief      Terminate application.
	void DoneSpreadSheet();
}
#endif
