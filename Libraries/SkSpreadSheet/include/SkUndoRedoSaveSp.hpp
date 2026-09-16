//=============================================================================
// SkSpreadSheet save element for Undo Redo
// Used for supress row col or sheet (with or without undo).
//=============================================================================
#ifndef SkUndoRedoSave_hpp
#define SkUndoRedoSave_hpp

#include <SkUndoRedo.hpp>
#include <SkVariant.hpp>

#include "SkTools.hpp"
#include "SkColRowCellRange.hpp"
#include "SkSelect.hpp"
#include "SkJsonKey.hpp"

#include "SkCalculationPath.hpp"
#include "SkRangeNamed.hpp"
#include "SkRangeData.hpp"
#include "SkConditionalFormat.hpp"

using namespace SkRoot;
using namespace rapidjson;

namespace SkSpreadSheet {

struct tRebasePlan;


//! end define constants for json keys ============================================
	class tSaveSelect;
    class tUndoSpreadSheet;

    //========================================================================
    class tSave : tClass {
    protected:
        //! Index on Table ColRowCellRange  for find Sheet
        tAllocatorRef    m_ColRowCellRangeRef;
    public:
        /// @brief Constructor tSave
        tSave();
        /// @brief Constructor tSave
        /// @param[in]  sColRowCellRangeRef  tAllocatorRef
        tSave(tAllocatorRef sColRowCellRangeRef);
        /// @brief Constructor of Copy
        tSave(const tSave& sSave);
        
        /// @brief        Return IndexOfAllocator cellRange
        /// @return        tIndex
        tAllocatorRef ColRowCellRangeRef();
        
        /// @brief        Return ColRowCellRange
        /// @return        tColRowCellRange*
        tColRowCellRange* ColRowCellRange();

        /// @brief        Return Sheet
        /// @return        tSheet*
        tSheet* Sheet();

        /// @brief Return Rebase Plan (the same or another Sheet
        /// @param[in] sRebasePlan tRebasePlan
        /// @return tRebasePlan
        tRebasePlan IsRebaseAnotherSheet(tRebasePlan sRebasePlan);

        
        /// @brief Return Rebase Plan (the same or another Sheet
        /// @param[in] sRebasePlan tRebasePlan
        /// @param[in] sSheetRef tAllocatorRef
        /// @return tRebasePlan
        tRebasePlan IsRebaseAnotherSheet(tRebasePlan sRebasePlan,tAllocatorRef sSheetRef);
    };

	//========================================================================
	//! Undo Save Formula of Cell
	class tSaveFormulaCell : tSave {
	private:
        //! Type
		tTypeItem		m_Type;
		//! Row 
		tIndex			m_Row;
		//! Col
		tIndex			m_Col;
		//! Cell Index  in vector formula
		tIndex			m_Index;

		//! Attribute 
		tSharedString	m_Attribute;

        //! Shared formula (formulas with the same content but different references are shared in a single SkFormulaItem)
        tSharedFormula m_SharedFormula;
	public:
        /// @brief Constructor tSaveFormulaCell.
       tSaveFormulaCell();
        /// @brief Constructor tSaveFormulaCell.
		/// @param[in]  sSheetAllocator  tAllocatorRef
		/// @param[in]  sType tTypeItem
		/// @param[in]  sRow tIndex
		/// @param[in]  sCol tIndex
		/// @param[in]  sAttribute tString
		/// @param[in]  sIndex tIndex Formula position
		tSaveFormulaCell(tAllocatorRef sSheetAllocator,tTypeItem sType, tIndex sRow, tIndex sCol,tString sAttribute, tIndex sIndex);
		/// @brief constructor of copy tSaveFormulaCell.
		/// @param[in]  sFormulaCell const tSaveFormulaCell&
		tSaveFormulaCell(const tSaveFormulaCell& sFormulaCell);

		/// @brief destructor of tSaveFormulaCell.
		virtual ~tSaveFormulaCell();

		/// @brief		Return Type
		/// @return		tTypeItem
		tTypeItem Type();

		/// @brief		Return Row
		/// @return		tIndex
		tIndex Row();

		/// @brief		Return Col
		/// @return		tIndex
		tIndex Col();

		/// @brief		Return Attribute name
		/// @return		tString
		tString Attribute();

		/// @brief		Return Index
		/// @return		tIndex
		tIndex Index();

		/// @brief		Return tCell (Warning use in do not in undo)
		/// @return		tCell*
		tCell* Cell();
        
        // Formula
        /// @brief      Set formula
        /// @param[in]  sFormula tFormula*
        void Formula(const tFormula* sFormula);

        /// @brief        Return formula
        /// @return        tFormula*
        tFormula* Formula();

        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        virtual tString Debug();
#endif

        // Rebase ===============================================================
        /// @brief      Rebase cell coordinates using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if cell was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan);
	};

	typedef std::vector<tSaveFormulaCell> tVectorSaveFormulaCell;
	
	//=========================================================================
	//! Undo save cell (Value, formula, css , dependent)
	class tSaveCell : public tSave {
	protected:
		//! Sorted by row col
		typedef std::vector<tSaveCell*> tVectorSaveCell;
		tTypeItem				m_Type;
		//! Row 
		tIndex					m_Row;
		//! Col
		tIndex					m_Col;
        
        //! Value or formula
        tVariant                m_Variant;

		//! Extension (for matrix)
		tExtension              m_Extension;
        
        //! Format
        tFormatRef              m_Css;
        
        // Key Format for Json
        tSharedString           m_FormatStr;

		//! Vector contains depend (or is tRange vector contains cell(s) with formula on this range)
		tVectorSaveCell			m_VectorDepend;
		
		//! Vector of dependent objects created in JsonDependent (must be deleted in destructor)
		//! Allocated only when needed (lazy allocation) to save memory
		tVectorSaveCell*		m_VectorDependJson;
		
		//! Vector of cell dependent with formula
		tVectorSaveFormulaCell	m_VectorFormulaCellDepend;
       
		//! Class (Just for Create class dynamically
		tSharedString			m_ClassName;
        //! Name in Class
        tSharedString           m_RefName;
        
		//! Attribute 
		tSharedString			m_Attribute;
	public:
		/// @brief constructor of tSaveCell.
		tSaveCell();

		/// @brief constructor of copy.
		/// @param[in]  sSaveCell const tSaveCell& 
		tSaveCell(const tSaveCell& sSaveCell);
		/// @brief constructor of tSaveCell with parameter row & col.
        /// @param[in]  sSheetAllocator  tAllocatorRef
        /// @param[in]  sType tTypeItem
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        /// @param[in]  sCss tFormatRef
        /// @param[in]  sAttribute tString
        /// @param[in]  sExtension tExtension
     	tSaveCell(tAllocatorRef sSheetAllocator,tTypeItem sType,tIndex sRow, tIndex sCol,tFormatRef sCss,tString sAttribute,tExtension sExtension);
		/// @brief destructor of tSaveCell.
		virtual ~tSaveCell();

		/// @brief Set tSaveRange with tItem.
		/// @param[in]  sItem tItem* 
		void Set(tItem* sItem);

		/// @brief Set Dependent cell with tItem.
		/// @param[in]  sSaveSelect tSaveSelect*
		/// @param[in]  sItem tItem* 
		void SetDepend(tSaveSelect* sSaveSelect, tItem* sItem);

        /// @brief Pass Css in SaveCell only if undo mode.
        /// @param[in]  sCell tCell*
        void PassCss(tCell* sCell);
        
		/// @brief		Return VectorOfDepend
		/// @return		tVectorSaveCell*
		tVectorSaveCell* VectorDepend();

	
		/// @brief		Return Sheet 
		/// @return		tSheet*
		tSheet* Sheet();

		/// @brief		Return Row
		/// @return		tIndex
		tIndex Row();

		/// @brief		Return Col
		/// @return		tIndex
		tIndex Col();
        
        /// @brief       Return Attribute
        /// @return      tString
        tString Attribute();

        /// @brief       Return matrix / spill extension bits snapshotted at save time.
        /// @return      tExtension
        tExtension Extension() const;
        
        /// @brief       Return ClassName
        /// @return      tString
        tString ClassName();
        
        /// @brief       Return RefName
        /// @return      tString
        tString RefName();
        
        // Css ============================================================
        /// @brief      Return css
        /// @return     tFormatRef
        tFormatRef  Css();

        /// @brief     Set Css Value
        /// @param[in] sFormatRef tFormatRef
        void Css(tFormatRef sFormatRef);
        

        /// @brief     Get Format string (Json)
        /// @return        tString
        tString FormatStr();

		/// @brief      Return Reference of cell like A1.
		/// @return		tString
		const tString StrRef();
        
        /// @brief      Set Variant
        /// @param[in]  sValue tVariant&
        void Value(tVariant& sValue);

		/// @brief		Return Value
		/// @return		tVariant*
		tVariant* Value();

		/// @brief		Return tCell (Warning use in do not in undo)
		/// @return		tCell*
		tCell* Cell();

		/// @brief		Ensure tCell
		/// @return		tCell*
		tCell* EnsureCell();

		/// @brief      Push FormulaCell (only for delete col row or sheet).
		/// @param[in]  sSaveFormulaCell tSaveFormulaCell&  
		void PushFormulaCell(tSaveFormulaCell& sSaveFormulaCell);

		/// @brief      Push Recover cell formulas.
		/// @param[in]  sIsJsonUndo tBool  true when undo is applied from JSON (GetMessage)
		void RecoverFormulaCell(tBool sIsJsonUndo = false);

		/// @brief		Return tVectorSaveFormulaCell
		/// @return		tVectorSaveFormulaCell*
		tVectorSaveFormulaCell* VectorFormulaCellDepend();
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        /// @param[in]  sIsWriteDependent tBool
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet,tBool sIsWriteDependent);
        
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void JsonDependent(Writer<StringBuffer>* sWriter,tSheet* sSheet);

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void JsonFormulaCell(Writer<StringBuffer>* sWriter,tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
        /// @brief      Write JSON.
        /// @param[in]  ssValue Value&
        //// @param[in]  sSheet tSheet*
        void JsonDependent(const rapidjson::Value& sValue,tSheet* sSheet);

		 /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void JsonFormulaCell(const rapidjson::Value& sValue,tSheet* sSheet);

		/// @brief      Operator < for comparison with tComparatorSaveCell.
		/// @param[in]  sSaveCell tSaveCell& 
		///	@return		tBool
		tBool operator < (tSaveCell& sSaveCell);

		/// @brief      Operator ==.
		/// @param[in]  sSaveCell tSaveCell& 
		///	@return		tBool
		tBool operator == (tSaveCell& sSaveCell);

#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        virtual tString Debug();
#endif


#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif

        // Rebase ===============================================================
        /// @brief      Rebase cell coordinates using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if cell was deleted
        virtual tBool Rebase(const tRebasePlan& sRebasePlan);
	};
	typedef std::vector<tSaveCell*> tVectorSaveCell;

	//=========================================================================
	//! Sorted tSaveCell by operator < on tSaveCell
	class tComparatorSaveCell {
	public:
		/// @brief      Operator() compare with tSaveCell < operator.
		/// @param[in]  sE2 tSaveCell*
		/// @param[in]  sE1 tSaveCell*
		bool operator()(tSaveCell* sE1, tSaveCell* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//=========================================================================
	//! Undo save value of Range
	class tSaveRange : public tSaveCell {
	private:
		tRange*       m_Range;
		//! Cached allocator ref to avoid UAF on m_Range
		tAllocatorRef m_RangeAllocatorRef;
		//! deleted
		tBool	m_Deleted;
		//! External Recovered 
		tBool   m_ExternalCovered;
		//!	Bottom;
		tIndex  m_Bottom;
		//! Right
		tIndex	m_Right;

		//! Coordinate after delete Col or Row
		tRect   m_CoordAfter;

		//! RangeNamed
		tSharedString m_Name;
  
        //!  Range Data
        tRangeData*   m_RangeData;
        
        //! tExtension (Merged named)
        tExtension    m_Extension;
	public:
		/// @brief constructor of tSaveRange.
		tSaveRange();

		/// @brief constructor of copy.
		/// @param[in]  sSaveRange const tSaveRange&  
		tSaveRange(const tSaveRange& sSaveRange);
		/// @brief constructor of tSaveRange with coordinate.
		/// @param[in]  sIndiceSheet  tIndex
		/// @param[in]  sTop tIndex
		/// @param[in]  sLeft tIndex
		/// @param[in]  sBottom tIndex
		/// @param[in]  sRight tIndex
		tSaveRange(tIndex sIndiceSheet, tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight, tString sName);
		/// @brief constructor of tSaveRange with tRange.
		/// @param[in]  sRange tRange* 
		tSaveRange(tRange* sRange);

		/// @brief destructor of tSaveRange.
		virtual ~tSaveRange();

        /// @brief GetAllocatorRef
        /// @return tAllocatorRef
        tAllocatorRef AllocatorRef();

		/// @brief  Set Extension
		/// @param[in]  sRange tRange*
		void SetExtension(tRange* sRange);
        
		/// @brief      Set value and depend cell with tRange.
		/// @param[in]  sSaveSelect tSaveSelect*
		/// @param[in]  sItem tItem*
		/// @param[in]  sName tString 
		void SetRefAndDepend(tSaveSelect* sSaveSelect, tItem* sItem);

		/// @brief		Set is Deleted
		/// @param[in]  sDeleted tBool
		void Deleted(tBool sDeleted);

		/// @brief		Return is Deleted
		/// @return		tBool
		tBool Deleted();

		/// @brief		Set is External Covered
		/// @param[in]  sExternalCovered tBool
		void ExternalCovered(tBool sExternalCovered);

		/// @brief		Return is External Covered
		/// @return		tBool
		tBool ExternalCovered();

		// Rebase ===============================================================
		/// @brief      Rebase range coordinates using rebase plan
		/// @param[in]  sRebasePlan const tRebasePlan&
		/// @return     tBool true if rebase successful, false if range was deleted
		tBool Rebase(const tRebasePlan& sRebasePlan) override;
        
        /// @brief      Return if range is Named.
        /// @return     tBool;
        tBool IsNamed();
    
		/// @brief      Set Named.
		void SetNamed();
    
        /// @brief      Return if range is Merged.
        /// @return     tBool;
        tBool IsMerged();

		/// @brief      Set Merged.
		void SetMerged();
        
		/// @brief      Return IsData.
		/// @return     tBool;
		tBool IsData();

		/// @brief      Set Data.
		void SetData();

		/// @brief      Set Data.
	
        /// @brief      Return IsConditionalFormat.
        /// @return     tBool;
        tBool IsConditionalFormat();
        
        
        // Conditional Format HighlightCellsRule =====================================
		/// @brief      Return if range is in Conditional Format HighlightCellsRule
		/// @return     tBool;
		tBool IsCFHR();

		/// @brief      Set Conditional Format HighlightCellsRule.
		/// @param[in]  sCFHR tBool
		void SetCFHR();

		// Conditional Format DataBar ======================================	
		/// @brief      Return if range is in Conditional Format DataBar.
		/// @return     tBool;
		tBool IsCFDB();
		/// @brief      Set Conditional Format DataBar.
		void SetCFDB();

		
		// Conditional Format ColorScale ===========================================
		/// @brief      Return if range is in Conditional Format ColorScale.
		/// @return     tBool;
		tBool IsCFCS();

		/// @brief   Set Conditional Format ColorScale.
		void SetCFCS();

		// Conditional Format IconSet ==============================================
		/// @brief      Return if range is in Conditional Format IconSet.
		/// @return     tBool;
		tBool IsCFIS();

		/// @brief      Set Conditional Format IconSet.
		/// @param[in]  sCFIS tBool
		void SetCFIS();

		// Condidtional Format CustomFormat =========================================
		/// @brief      Return if range is in Conditional Format CustomFormat.
		/// @return     tBool;
		tBool IsCFCF();

		/// @brief      Set Conditional Format CustomFormat.
		void SetCFCF();

		/// @brief      Clear CF extension flags; CF state lives in scf, not mr.
		void ClearConditionalFormatExtension();

		// Coordinates ===========================================================
		/// @brief		Return Botttom
		/// @return		tIndex
		tIndex Bottom();

		/// @brief		Return Right
		/// @return		tIndex
		tIndex Right();

		/// @brief		Return Rect
		/// @return		tTempoRect;
		tTempoRect Rect();

		/// @brief      Return Reference of range like A1:A2.
		/// @return		tString
		const tString StrRef();

		/// @brief      Return Reference After like A1:A2.
		/// @return		tString
		const tString StrRefAfter();

		/// @brief		Set coord after Supress
		/// @param[in]  sCoordAfter	SkRect;
		void  CoordAfter(tTempoRect& sCoordAfter);

		/// @brief		Return coord after Supress
		/// @return		SkRect;
		tTempoRect CoordAfter();

		/// @brief		Return tRange (Warning use in do not in undo)
		/// @return		tRange*
		tRange* Range();
        
        /// @brief        Set Range
        /// @param[in]    sRange tRange*
        void Range(tRange* sRange);

		/// @brief		Return Name (Named Range)
		/// @return		tString;
		tString  Name();
  
        ///@briief                  Return RangeData
        ///retrun                   tRangeSata*
        tRangeData* RangeData();
        
        ///@brief       Set RangeData
        ///@param[in]   sRangeData tRangeData
        void RangeData(tRangeData sRangeData);
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        /// @param[in]  sIsWriteDependent tBool
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet,tBool sIsWriteDependent);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);


		/// @brief      Operator < for comparaison with SkComparatorSaveRange.
		/// @param[in]  sSaveRange tSaveRange& 
		///	@return		tBool
		tBool operator < (tSaveRange& sSaveRange);

		/// @brief      Operator ==.
		/// @param[in]  sSaveRange tSaveRange& 
		///	@return		tBool
		tBool operator == (tSaveRange& sSaveRange);

    #ifdef _DEBUGSK
            /// @brief      Debug
            /// @return     tString
        tString Debug() override;
    #endif

	};

	typedef std::vector<tSaveRange*> tVectorSaveRange;

	//=========================================================================
	//! Sorted tSaveRange by operator < on SaveRange
	class tComparatorSaveRange {
	public:
		/// @brief      Operator() compare with tRange < operator.
		/// @param[in]  sE2 tSaveRange*
		/// @param[in]  sE1 tSaveRange*
		inline bool operator()(tSaveRange* sE1, tSaveRange* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//=========================================================================
	//! Sorted tSaveRange by operator < on SaveRange
	class tComparatorSaveRangeCoordAfter {
	public:
		/// @brief      Operator() compare with tRange < operator.
		/// @param[in]  sE2 tSaveRange*
		/// @param[in]  sE1 tSaveRange*
		inline bool operator()(tSaveRange* sE1, tSaveRange* sE2) {
			return(sE1->CoordAfter() < sE2->CoordAfter());
		}
	};

    // ================================================================
    class tSaveConditionalFormat : public tClass {
    private:
		   tString                 m_Ref;
            //! Key
           tString                m_Key;
		   //! Conditional Format Type
		   tConditionalFormatType  m_Type;
		   //! Icon Set Type
		   tIconType               m_IconType;
		   //! Conditional Format pointer
		   tConditionalFormat*     m_ConditionalFormat;
		
		   //! param 1 to 10
		   tSharedString                 m_Param1;
		   tSharedString                 m_Param2;
		   tSharedString                 m_Param3;
		   tSharedString                 m_Param4;
		   tSharedString                 m_Param5;
		   tSharedString                 m_Param6;
		   tSharedString                 m_Param7;
		   tSharedString                 m_Param8;
		   tSharedString                 m_Param9;
		   tSharedString                 m_Param10;
    public:
		/// @brief      Constructor
        tSaveConditionalFormat();
		
        /// @brief      Constructor (Used for undo)
        /// @param[in]  sConditionalFormat tConditionalFormat*
        tSaveConditionalFormat(tConditionalFormat* sConditionalFormat);
        
		~tSaveConditionalFormat();

        /// @brief      Return conditional format
        /// @return     tConditionalFormat*
        tConditionalFormat* ConditionalFormat();

		/// @brief      Return icon type	
		/// @return     tIconType
		tIconType IconType();	

		/// @brief      Get reference
		/// @return     tString
		tString Ref();
        
        ///@breief              GetKey
        // @return     tString
        tString Key();

		/// @brief      Get type
		/// @return     tConditionalFormatType
		tConditionalFormatType Type();

		/// @brief      Return format param 1
		/// @return     tString
		tString FormatParam1();

		/// @brief      Return format param 2
		/// @return     tString
		tString FormatParam2();

		/// @brief      Return format param 3
		/// @return     tString
		tString FormatParam3();

		/// @brief      Return format param 4
		/// @return     tString
		tString FormatParam4();
        
        /// @brief      Return format param 5
        /// @return     tString
        tString FormatParam5();

        /// @brief      Return format param 6
        /// @return     tString
        tString FormatParam6();
        
        /// @brief      Return format param 7
        /// @return     tString
        tString FormatParam7();

        /// @brief      Return format param 8
        /// @return     tString
        tString FormatParam8();
        
        /// @brief      Return format param 9
        /// @return     tString
        tString FormatParam9();

        /// @brief      Return format param 10
        /// @return     tString
        tString FormatParam10();
        
     
		/// @brief      Return vector range
        /// @param[in]  sColRowCellRange tColRowCellRange* 
		/// @return     tVectorRange*
		tConditionalFormat* Undo(tColRowCellRange* sColRowCellRange);

        void Json(Writer<StringBuffer>* sWriter);
        void Json(const rapidjson::Value& sValue);
    };

	typedef std::vector<tSaveConditionalFormat*> tVectorSaveConditionalFormat;
	
    // =========================================================================
    // For selection Col Row or Sheet ==========================================
    class tSaveColRow : public tClass {
    private:
        friend class tSaveSelectColRow;
        //! Index of Col or Row
        tIndex           m_Index;
        
        // Size of Col or Row
        tDouble           m_Size;

        //! For Css    in AllocatorFormat
        tFormatRef       m_Css;
        
        // Key Format for Json
        tSharedString    m_JsonFormat;

        //! Parent ColRow
        tIndex           m_Parent;

        //! Children col row
        vector<tIndex>   m_Children;
    public:
        tSaveColRow();
        tSaveColRow(tIndex sIndex);
        tSaveColRow(tColRow* sColRow);
        ~tSaveColRow();
        
        tIndex Index();
        
        tDouble Size();
      
        void Css(tFormatRef sCss);
        tFormatRef Css();
        
        tString JsonFormat();
        
        void Parent(tIndex sParent);
        tIndex Parent();
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        void Json(const rapidjson::Value& sValue);
        
        /// @brief      Operator < for comparaison with SkComparatorSaveColRow.
        /// @param[in]  sSaveRange tSaveRange&
        ///    @return        tBool
        tBool operator < (tSaveColRow& sSaveColRow);

        /// @brief      Operator ==.
        /// @param[in]  sSaveRange tSaveRange&
        ///    @return        tBool
        tBool operator == (tSaveColRow& sSaveColRow);
        
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
        
    };

    typedef vector<tSaveColRow*> tVectorSaveColRow;

    //=========================================================================
    //! Sorted tSaveColRow  by operator < on tSaveColRow
    class tComparatorSaveColRow {
    public:
        /// @brief      Operator() compare with tSaveCell < operator.
        /// @param[in]  sE2 tSaveCell*
        /// @param[in]  sE1 tSaveCell*
        bool operator()(tSaveColRow* sE1, tSaveColRow* sE2) {
            return(*sE1 < *sE2);
        }
    };

	//=========================================================================
	//! Undo save selection for Set raz copy.... (tTempoSelect temporary class)
	class tSaveSelect : public tClass {
	protected:
        //! /tUndoSpreadSheet (Info for get mode & error)
        tUndoSpreadSheet*       m_UndoSpreadSheet;
        
		//! Cell save vector
		tVectorSaveCell			m_VectorSaveCell;
        
		//! Range save vector
		tVectorSaveRange		m_VectorSaveRange;

        //! Save Selection
        tSharedString           m_SelectStr;
        
		//! Current	ColRowCellRange
		tColRowCellRange*		m_ColRowCellRange;

		// For Raz Cell ======================================================
		//! Vector of cells of other sheets for calculation ===================
		tVectorSaveCell			m_VectorCalculateExternalSaveCell;

		//! To calculate external Cell  (WARNING Direct Cell adress)
		tVectorCell				m_VectorCellCalculate;
        
        //! To save JsonPayLoad
        tCellClassContainer::tMapPointJsonPayLoad m_MapSaveJsonPayload;

        //! When true, UndoCell keeps the live cell CSS (content-only clear). Default false.
        tBool m_KeepFormat = false;
        //! When true, UndoCell restores CSS only: content and class attributes are left
        //! untouched (format-only clear). Default false.
        tBool m_FormatOnly = false;
	public:
		/// @brief constructor of tSaveSelect.
		tSaveSelect();

		/// @brief destructor of tSaveSelect.
		virtual ~tSaveSelect();

		/// @brief Clear.
		void Clear();

        /// @brief Move saved cells/ranges from another payload (used when merging cut into move).
        void AbsorbFrom(tSaveSelect& sOther);
        
        /// @brief Set ColRowCellRange
        /// @param[in]  sColRowCellRange tColRowCellRange*
        void ColRowCellRange(tColRowCellRange* sColRowCellRange);

		/// @brief Return is undo actif or not.
		/// @return tBool
		tBool IsUndoActif();

        /// @brief return UndoSpreadSheet
        /// @return tUndoSpreadSheet*
        tUndoSpreadSheet* UndoSpreadSheet();
        
        /// @brief Set ColRowCellRange
        /// @param[in]  sUndoSpreadSheet  sUndoSpreadSheet*
        void UndoSpreadSheet(tUndoSpreadSheet*  sUndoSpreadSheet);

        /// @brief Set keep format flag (UndoCell keeps live cell CSS).
        /// @param[in]  sKeepFormat tBool
        void KeepFormat(tBool sKeepFormat) { m_KeepFormat = sKeepFormat; }

        /// @brief Return keep format flag.
        /// @return tBool
        tBool KeepFormat() const { return(m_KeepFormat); }

        /// @brief Set format-only flag (UndoCell restores CSS only, keeps content and class).
        /// @param[in]  sFormatOnly tBool
        void FormatOnly(tBool sFormatOnly) { m_FormatOnly = sFormatOnly; }

        /// @brief Return format-only flag.
        /// @return tBool
        tBool FormatOnly() const { return(m_FormatOnly); }
        
        /// @brief Parse Select (Temporary)
        /// @return tBool
        tBool ParseSelect(tString sSelection);
        
		/// @brief return pointer of selection.`
        /// @return tSelect
		tSelect Select();
        
        /// @brief find cell.
        /// @param[in] sRow tIndex
        /// @param[in] sCol  tIndex
        /// @param[in] sSheet  tSheet*
        /// @return  tSaveCell*
        tSaveCell* FindCell(tIndex sRow,tIndex sCol,tSheet* sSheet);

        
        /// @brief find cell.
        /// @param[in]  sCell tCell*
        /// @return  tSaveCell*
        tSaveCell* FindCell(tCell* sCell);

        
		/// @brief add cell.
		/// @param[in]  sCell tCell*
		/// @return  tSaveCell*
		tSaveCell* AddCell(tCell* sCell);

        
		/// @brief add attribute of cel classl.
		/// @param[in]  sCell tCell*
		void AddCellAttributes(tCell* sCell);

		/// @brief Push cell to calculate.
		/// @param[in]  sCell tCell*
		void PushCalculate(tCell* sCell);

		/// @brief Drop pending CalculateDo cell pointers (e.g. after paste Do without CalculateDo).
		void ClearCellCalculate();

        /// @brief Add JsonPayload.
        /// @param[in]  sColRowCellRange tColRowCellRange*
        /// @param[in]  sPoint tPoint
        /// @return tBool
        tBool AddJsonPayload(tColRowCellRange* sColRowCellRange,tTempoPoint* sPoint);

		/// @brief Undo JsonPayload.
		void UndoJsonPayload();

		/// @brief      CalculateDo with External cell dependent Not in RectDeleteArea or Attribute.
		/// @param[in]  sColRowCellRange  tColRowCellRange*
        /// @param[in] 
		void CalculateDo(tColRowCellRange* sColRowCellRange,tVolatile sVolatile);

		/// @brief      Build the calculation graph on sContainerPath (BeginCalculate + Add*; no Reduce).
		void PopulateCalculationGraph(tContainerPath* sContainerPath, tColRowCellRange* sColRowCellRange, tVolatile sVolatile);

		/// @brief      Cooperative variant of CalculateDo: graph on workbook, Reduce via StepRecalculateAllCooperative.
		void BeginCooperativeCalculateDo(tColRowCellRange* sColRowCellRange, tVolatile sVolatile, tWorkBook* sWorkBook);
		/// @brief Find tSaveCell.
		/// @param[in]  sAllocatorColRange tIndex
		/// @param[in]  sType tTypeItem
		/// @param[in]  sRow tIndex
		/// @param[in]  sCol tIndex
		/// @param[in]  sAttribute tAttribute
		/// @return  tSaveCell*
		tSaveCell* FindSaveCell(tAllocatorRef sAllocatorColRange,tTypeItem sType, tIndex sRow, tIndex sCol, tString sAttribute);
		/// @brief add range.
		/// @param[in]  sRange tRange*
		/// @return  tSaveRange*
		tSaveRange* AddRange(tRange* sRange);

		/// @brief      Treats external dependency cells, remove dependency (sEraseSheet=true if DeleteSheet).
		/// @param[in]  sCell tCell*
		/// @param[in]	sEraseSheet tBool
		void TreatsExternalDependencyCells(tCell* sCell, tBool sEraseSheet = false);

        /// @brief      Break external formula refs to a range (e.g. SUM(Sheet1!A1:B2) on DeleteSheet).
        /// @param[in]  sRange tRange*
        /// @param[in]  sEraseSheet tBool
        void TreatsExternalDependencyRange(tRange* sRange, tBool sEraseSheet = false);

        /// @brief      Return true if Cell depend in not in selection
        /// @param[in]  sCell tCell*
        /// @return     tBool
        virtual tBool ExternalCell(tCell* sCell,tBool sEraseSheet=false);
 
		/// @brief      Return true if Cell depend in not in selection
		/// @param[in]  sCell tCell*
		/// @param[in]  sCellDepend tCell*
		/// @param[in]	sEraseSheet tBool
        /// @return     tBool
        virtual tBool ExternalCell(tCell* sCell, tCell* sCellDependent, tBool sEraseSheet = false);

        /// @brief      Undo.
        /// @param[in]  sSaveCell tSaveCell*
        void UndoCell(tSaveCell* sSaveCell);
        
        /// @brief      Undo.
		void UndoCells();

        /// @brief      Undo SaveCells whose sheet coords are outside m_SelectStr.
        ///             Used after Select CallBack restore so spill siblings snapshotted in BeforeDo
        ///             are restored without replacing the class-safe CallBackCell undo path.
        void UndoCellsOutsideSelect();

        /// @brief      Ensure VectorRef sources list this cell in ContainerCellDepend (checksp).
        void RewireOutgoingFormulaDependents();

        /// @brief      Queue formula dependents saved in fcell payloads for CalculateDo (collab JSON undo).
        void PushSavedFormulaDependentsCalculate();

		/// @brief 	Is empty
		/// @return 
		tBool IsEmpty();
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
        /// @brief      Write JSON.
        /// @param[in]  sSheet tSheet*
        /// @return     tString
        tString WriteJson(tSheet* Sheet);

        /// @brief      Read JSON.
        /// @param[in]  sJson tString
        /// @param[in] sColRowCellRange  tColRowCellRange*
        void ReadJson(tString sJson,tColRowCellRange* sColRowCellRange);

#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif

        // Rebase ===============================================================
        /// @brief      Rebase all cell and range coordinates using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if any cell/range was deleted
        virtual tBool Rebase(const tRebasePlan& sRebasePlan);
        
#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        virtual tString Debug();
#endif
	};

    //=========================================================================
    //! Use for Save ColRow 
    class tSaveSelectColRow : public tSaveSelect {
    private:
        //! ColRowSave  vector
        tVectorSaveColRow      m_VectorSaveColRow;
    protected:
        //! Delete Row and Col true for row
        tBool                  m_DoesRow;
    public:
		/// @brief constructor of tSaveSelectColRow.
        tSaveSelectColRow(tBool sDoesRow);
        
        /// @brief destructor of tSaveSelectColRow.
        virtual ~tSaveSelectColRow();
        
        /// @brief Clear.
        void Clear();
        

        /// @brief DeleteCellFormat.
        /// @param[in]  sSheet tSheet
        void DeleteCellFormat(tSheet* sSheet);
            
        /// @brief add ColRow.
        /// @param[in] sColRow  tColRow*
        /// @param[in] sColRowCellRange tColRowCellRange*
        /// @param[in] sIsRow tBool
        /// @return  tSaveColRow*
        tSaveColRow* AddColRow(tColRow* sColRow,tColRowCellRange* sColRowCellRange,tBool sIsRow);
        
        /// @brief find ColRow.
        /// @param[in]  sIndex tIndex
        /// @return  tSaveColRow*
        tSaveColRow* FindColRow(tIndex sIndex);

        /// @brief Restore ColRow from save (index/size/tree links + format).
        /// @param[in]  sColRow  tColRow*
        /// @param[in]  sSaveColRow  tSaveColRow*
        /// @param[in]  sColRowCellRange tColRowCellRange*
        /// @param[in] sIsRow tBool
        /// @param[in] sFormatSaveOwned true = save holds an Inc'd format (delete undo);
        ///            false = save is a non-owned snapshot (tree undo) — do not Dec the
        ///            live Css when it still matches the snapshot.
        void RecupColRow(tColRow* sColRow,tSaveColRow* sSaveColRow,tColRowCellRange* sColRowCellRange,tBool sIsRow,tBool sFormatSaveOwned = true);
        
        
        /// @brief Return tVectorSaveColRow .
        /// @return  tVectorSaveColRow*
        tVectorSaveColRow* VectorColRow();
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter,tSheet* sShee);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
        // Rebase ===============================================================
        /// @brief      Rebase all cell, range and colrow coordinates using rebase plan (rebases both rows and columns)
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if any cell/range/colrow was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan) override;

#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
    };

	//=========================================================================
	//! Collect all recover range....
	//! After delete col or row 
	class tSaveCoveredRange : public tClass {
	private:
		// ! Vector of all cell range in same position afert delete
		tVectorSaveRange	m_VectorSaveRange;
		tSaveRange*			m_ExternalCovered;
	public:
		/// @brief constructor of SkSaveCoveredRange.
		tSaveCoveredRange();

		/// @brief destructor of SkSaveCoveredRange.
		virtual ~tSaveCoveredRange();

		/// @brief get vector save range.
		/// @return  tVectorSaveRange*
		tVectorSaveRange* VectorSaveRange();

		/// @brief Push range.
		/// @param[in]  sSaveRange tSaveRange* 
		void PushSaveRange(tSaveRange* sSaveRange);

		/// @brief get last save range.
		/// @return  tSaveRange*
		tSaveRange* LastSaveRange();

		/// @brief get external range covered.
		/// @return  tSaveRange*
		tSaveRange* ExternalCovered();

		/// @brief set external range covered.
		/// @param[in]  sSaveRange tSaveRange* 
		void  ExternalCovered(tSaveRange* sSaveRange);
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
        // Rebase ===============================================================
        /// @brief      Rebase all range coordinates using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if any range was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan);

#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        virtual tString Debug();
#endif
	};

	typedef vector<tSaveCoveredRange*> tVectorSaveCoveredRange;

	//=========================================================================
	//! Undo save selection for Erase col row or sheet....
	class tSaveSelectErase : public tSaveSelectColRow  {
	private:
        tBool                               m_IsRect;

		//! Covered range vector ==============================================
		tVectorSaveCoveredRange				m_VectorSaveCoveredRange;
        
        typedef tClassUnorderedContainer<tRange>   tVectorUniqueRange;
		//!  Unique range vector ==============================================
        //!   WARNING direct Pointer
        tVectorUniqueRange  m_VectorUniqueRange;

		//! Range coordinate after deletion ===================================
		tVectorSaveRange m_VectorSaveRangeCoordAfter;

		//! Named range vector ================================================
        tVectorSaveRange m_VectorNamedMergedDataConditionalRange;

		//!Rectangle of the deletion area
		tRect              m_RectDeleteArea;

		//! Container of Conditional Format
		tVectorSaveConditionalFormat m_VectorSaveConditionalFormat;

        void DeleteRemovedRangeClean(tAllocatorRef sAllocatorRef);
        void SaveConditionalFormatsForRangeUndo(tSaveRange* sSaveRange, tRange* sRange);
        void EraseConditionalFormatRangeInDeleteArea(tSaveRange* sSaveRange);
        void ProcessDeletedRangeRemoval(tSaveRange* sSaveRange);
        void ProcessConditionalFormatsInDeleteArea();
        tBool ComputeRangeFullyInsideDeleteArea(tBool sIsRect, tRange* sRange,
            tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);
        void ApplyRangeCoordAfterDelete(tIndex sPosition, tIndex sSize,
            tIndex& sTop, tIndex& sLeft, tIndex& sBottom, tIndex& sRight);
        tBool IsRangeCoordInvalidAfterDelete(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);
        // Peer GetMessage: svse JSON may have sel but no rc — rebuild sheet extent for calc.
        void EnsureRectDeleteAreaFromSheet();
    public:
        /// @brief      Remove Range of colrow .
        void DetachRangeAfterFromColRow();
        
        /// @brief      Replace Range inf colrow .
        void AttachRangeToColRow();
	public:
		/// @brief constructor of tSaveSelect.
		tSaveSelectErase();

		/// @brief destructor of tSaveSelect.
		~tSaveSelectErase() override;

		/// @brief Clear.
		void Clear();
        
        /// @brief Clear after do 
        void ClearAfterDo();

		/// @brief Set Rect For Calculate
		/// @param[in]  sRect SkRect.
		void RectDeleteArea(tRect sRect);

		/// @brief Return Rect for Calculate.
		/// @return tRect
		tRect RectDeleteArea();

		/// @brief Set Rect For Calculate
		/// @param[in]  sRect SkRect.
		tTempoRect* TempoRectDeleteArea();

		/// @brief      Add unique range.
		/// @param[in]  sRange tRange*
        /// @param[in]  tBool 
		tBool AddUniqueRange(tRange* sRange);
        
		/// @brief		Removes (deleted) ranges and processes overlay after Delete Col or Row
		/// @param[in]  sIndiceAllocatorCellRange tIndex
		void RemovesRangesAndProcessesOverlays(tIndex sIndiceAllocatorCellRange);

        /// @brief      Delete sSize Row or Col at position sPosition.
        /// @param[in]  sDoesRow tBool true for row, false for col
        /// @param[in]  sIndiceAllocatorCellRange tIndex
        /// @param[in]  sPosition tIndex
        /// @param[in]  sSize tIndex
        /// @param[in]  sIsRect  ttBoll
		void DeleteColRow(tBool sDoesRow, tIndex sIndiceAllocatorCellRange, tIndex sPosition, tIndex sSize,tBool sIsRect);

        /// @brief      Undo all Named Merged Ranges
        void UndoNamedMergedDataConditionalRange();
        
		/// @brief      UndoDelete Row or Col.
		void UndoDeleteColRow();

		/// @brief      Push cells that depend on ranges referencing the given rectangle to calculation list.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		/// @param[in]  sRect tRect - Rectangle containing rows/cols to check dependencies for
		void PushRangeDependCalculate(tColRowCellRange* sColRowCellRange, tRect sRect);

		/// @brief      Erase all ranges in SkColRow Container.
		void DeleteRangeInColRowContainer();

		/// @brief      Apply all ranges in SkColRow Container.
		void ApplyColRow();
        
        /// @brief      Apply all ranges afterundo Insert r.
        void ApplyColRowUndoInsertRect();
        
        /// @brief      Return true if Cell depend in not in selection
        /// @param[in]  sCell tCell*
        /// @return     tBool
        tBool ExternalCell(tCell* sCell,tBool sEraseSheet=false) override;
        
		/// @brief      Return true if Cell depend in not in selection
		/// @param[in]  sCell tCell*
		/// @param[in]  sCellDepend tCell*
		/// @param[in]	sEraseSheet tBool
        /// @return     tBool
        tBool ExternalCell(tCell* sCell, tCell* sCellDependent, tBool sEraseSheet = false) override;

		/// @brief      Delete sheet.
		/// @param[in]  sIndiceAllocatorCellRange tIndex
		void DeleteSheet(tIndex sIndiceAllocatorCellRange);

		/// @brief      Null formula refs on other sheets that point into the sheet being deleted.
		void InvalidateRefsToDeletedSheet();

		/// @brief      Save then remove named ranges and conditional formats of the sheet being deleted.
		void SaveAndRemoveSheetNamedRangesAndConditionalFormats();

		/// @brief      undo delete sheet.
		void UndoDeleteSheet();

		/// @brief      Return or Create  save conditional format.
		/// @param[in]  sConditionalFormat tConditionalFormat*
		/// @param[in]  sSaveRange tSaveRange*
		/// @return     tSaveConditionalFormat*
		tSaveConditionalFormat* GetOrCreateSaveConditionalFormat(tConditionalFormat* sConditionalFormat, tSaveRange* sSaveRange);

		/// @brief 	Is empty
		/// @return 
		tBool IsEmpty();
		
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter,tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  sSheet tSheet*
        void Json(const rapidjson::Value& sValue,tSheet* sSheet);
        
        /// @brief      Write JSON.
        /// @param[in]  sSheet tSheet*
        /// @return     tString
        tString WriteJson(tSheet* sSheet);

        /// @brief      Read JSON.
        /// @param[in]  sJson tString
        /// @param[in] sColRowCellRange  tColRowCellRange*
        void ReadJson(tString sJson,tColRowCellRange* sColRowCellRange);
        
        // Rebase ===============================================================
        /// @brief      Rebase all cell, range, colrow and additional ranges coordinates using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool true if rebase successful, false if any cell/range/colrow was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan) override;
		
#ifdef _DEBUGSK
		/// @brief		Debug
        /// @return tString
		tString Debug() override;
#endif
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
	};

	//===================================================================
	//! Undo save for Add RangeNamed or delete RangeNamed....
	class tSaveRangeNamed : public tClass {
	private:

		//! Vector contains Not used before SetName =================================
		tVectorSaveCell	m_VectorCellDepend;
	public:
		/// @brief constructor tSaveRangeNamed.
		tSaveRangeNamed();

		/// @brief destructor  tSaveRangeNamed.
		~tSaveRangeNamed();

		
		/// @brief Add Cell dependent 
		/// @param[in]  sCell tCell*
		/// @return tSaveCell*
		tSaveCell* AddCell(tCell* sCell);
		
		/// @brief Save and modify formula on cell dependent
		/// @param[in]  sRange tRange*
		void SaveAndModifyDependent(tRange* sRange);

		/// @brief Save and modify formula on cell dependent
		/// @param[in]  sRange tRange*
		void RecupAndModifyDependent(tRange* sRange);

        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  tSheet* sSheet
        void Json(Writer<StringBuffer>* sWriter, tSheet* sSheet);
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in]  tSheet* sSheet
        void Json(const rapidjson::Value& sValue, tSheet* sSheet);
        
        // Rebase ===============================================================
        /// @brief      Rebase range coordinates and dependent cells using rebase plan
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @param[in]  tSheet* sSheet
        /// @return     tBool true if rebase successful, false if range was deleted
        tBool Rebase(const tRebasePlan& sRebasePlan,tSheet* sSheet);
	};

  
} // end of namespace
#endif // End of #ifndef SkUndoRedoSave_hpp
