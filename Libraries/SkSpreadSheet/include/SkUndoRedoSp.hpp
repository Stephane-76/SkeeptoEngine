//=============================================================================
// SkSpreadSheet Undo Redo
//=============================================================================
#ifndef tUndoRedoSp_hpp
#define tUndoRedoSp_hpp

#include <SkUndoRedo.hpp>

#include <map>
#include <vector>
#include <SkVariant.hpp>
#include "SkUndoRedoSaveSp.hpp"
#include "SkSpreadSheet.hpp"
#include "SkUser.hpp"
#include "SkUndoRedoRebase.hpp"
using namespace SkRoot;

namespace SkSpreadSheet {
    tString BorderStr(tShort sBorder);
    
    class tSheet;
	enum class tUndoState : tChar { BeforeDo, Do, BeforeUndo, Undo };


    // Interface Mode on Api & tUndoSpreadSheet 
    typedef tBitSet<tByte> tMode;

    const tByte t_IsUndoActif=0;
    const tByte t_IsDo=1;
    const tByte t_IsRedo=2;
    const tByte t_IsUndo=3;
    const tByte t_IsJson=4;
    const tByte t_Client=5;
    const tByte t_Server=6;
    const tByte t_IsError=7;


    //=========================================================================
    //! Undo ancestor (sUndo actif ) for SpreadSheet Server IsUndo=false
    class tUndoSpreadSheet : public tUndo {
    protected:
        // Mode Client server Undo ApplyString ================================
        tMode m_Mode;
        //! Error explanation
        tString m_Error;
        
        // Rebase =============================================================
        tSequenceId m_SequenceId;
        // Unique operation ID to find the corresponding Do operation for an Undo
        std::uint64_t m_OperationId;
        // RebasePlan
        tRebasePlan m_RebasePlan;
        //! Workbook URI pinned for Client Do/Undo/Redo (tInterfaceWeb).
        tSharedString   m_WorkBookTarget;
    public:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        /// @brief constructor of tUndoSpreadSheet.
        tUndoSpreadSheet(tMode sMode);
        
        /// @brief destructor of tUndoSpreadSheet.
        ~tUndoSpreadSheet() override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        virtual void DeleteBeforeDo()=0;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        virtual tSaveSelect* SaveSelect()=0;

        /// @brief      Get RefRebase
        /// @return     tString
        virtual tString RefRebase()=0;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        virtual tBool Rebase()=0;
        
        
        /// @brief Normalize Sheet
        /// @param[out] sSheet tSheet** 
        void NormalizeSheet(tSheet** sSheet);


        /// @brief Set SequenceId.
        /// @param[in] sSequenceId tSequenceId
        void SequenceId(tSequenceId sSequenceId);

        /// @brief Get SequenceId.
        /// @return tSequenceId
        tSequenceId SequenceId();
        
        /// @brief Set OperationId (unique ID to find corresponding Do operation).
        /// @param[in] sOperationId std::uint64_t
        void OperationId(std::uint64_t sOperationId);

        /// @brief Get OperationId.
        /// @return std::uint64_t
        std::uint64_t OperationId();
        
        /// @brief        Get IsUndo.
        /// @return       IsUndo
        tBool IsUndoActif();

        /// @brief        Set IsUndo.
        /// @param[in]    sIsUndo tBool
        void IsUndoActif(tBool sIsUndo);
        
         /// @brief Set IsJson.
        /// @param[in] sJson tBool
        void IsJson(tBool sJson);
        
        /// @brief        Get IsJson.
        /// @return       IsJson
        tBool IsJson();
        /*
        /// @brief        Set Alone.
        /// @param[in]    sAlone tBool
        void Alone(tBool sAlone);

        /// @brief        Get Alone.
        /// @return        tBool
        tBool Alone();
         */
        /// @brief        Set Client.
        /// @param[in]    sClient tBool
        void Client(tBool sClient);

        /// @brief        Get Client.
        /// @return       tBool
        tBool Client();
        
        /// @brief        Set Server.
        /// @param[in]    sServer tBool
        void Server(tBool sServer);

        /// @brief        Get Server.
        /// @return       tBool
        tBool Server();

        /// @brief        Set IsDo.
        /// @param[in]    sIsDo tBool
        void IsDo(tBool sIsDo);

        /// @brief        Get IsDo.
        /// @return       tBool
        tBool IsDo();
        
        /// @brief        Set IsUndo.
        /// @param[in]    sIsUndo tBool
        void IsUndo(tBool sIsUndo);

        /// @brief        Get IsUndo.
        /// @return       tBool
        tBool IsUndo();
        
        /// @brief        Set IsRedo.
        /// @param[in]    sIsRedo tBool
        void IsRedo(tBool sIsRedo);

        /// @brief        Get IsRedo.
        /// @return       tBool
        tBool IsRedo();
     
        
        /// @brief        Set IsError.
        /// @param[in]    sIsError tBool
        void IsError(tBool sIsError);

        /// @brief        Get IsError.
        /// @return       tBool
        tBool IsError();

        /// @brief        Set Error.
        /// @param[in]    sError tString
        void Error(tString sError);

        /// @brief        Get Error.
        /// @return       tString
        tString Error();
        
        
        ///@Brief Sheet
        ///@return tSheet
        virtual tSheet* Sheet();
        ///
        ///@brief       SetRebasePlan
        void SetRebasePlan();
        
        /// @brief      Test if Sheet exist
        /// @param[in]  sWorkBook tWorkBook*
        /// @param[in]  sName tString
        /// @return     tBool
        tBool TestSheetExists(tWorkBook* sWorkBook,tString sName);
        
        /// @brief      Test if Sheet already exist
        /// @param[in]  sWorkBook tWorkBook*
        /// @param[in]  sName tString
        /// @return     tBool
        tBool TestSheetAlreadyExists(tWorkBook* sWorkBook,tString sName);
        
        /// @brief      Return WorkBook
        /// @return     tWorkBook*
        tWorkBook* WorkBook();

        /// @brief       WriteJson.
        /// @return      tString
        tString WriteJson();

        /// @brief       ReadJson.
        /// @param[in]   sJson tString
        void ReadJson(tString sJson);
        
        void DebugFlags(tString sTitle);

        /// @brief Pin target workbook URI for Sheet()/WorkBook() resolution.
        /// @param[in] sWorkBookTarget tString
        void WorkBookTarget(tString sWorkBookTarget);
    };

    /// @brief Execute spreadsheet undo Do through tUndoSpreadSheet (avoids tUndo* vtable drift).
    tBool DispatchSpreadSheetDo(tUndoRedoContainer* sContainer, tUndoSpreadSheet* sUndo);

    /// @brief Execute spreadsheet undo Undo through tUndoSpreadSheet.
    tBool DispatchSpreadSheetUndo(tUndoRedoContainer* sContainer);

    /// @brief Execute spreadsheet redo Do through tUndoSpreadSheet.
    tBool DispatchSpreadSheetRedo(tUndoRedoContainer* sContainer);

    //=========================================================================
    //! Undo to change the size of row or columns
	class tUndoChangeSize : public tUndoSpreadSheet {
	private:
		//! Row or Col
		tBool	m_IsRow;
		//! Index 
		tIndex	m_Begin;
        
        tIndex  m_End;
        
        tIndex  m_BeginRebase;
        
        tIndex  m_EndRebase;
        
		//! Size
		tDouble  m_Size;
        //! Save Value
        stack<tDouble> m_StackSize;
		//! Sheet
		tSheet*        m_Sheet;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
        /// @brief constructor of tUndoChangeSize.
        tUndoChangeSize();

        /// @brief constructor of tUndoChangeSize.
        /// @param[in] sIsRow tBool
        /// @param[in] sBegin  tIndex
        /// @param[in] sEnd  tIndex
        /// @param[in] sSize tDouble
        /// @param[in] sSheet tSheet
		tUndoChangeSize(tBool sIsRow,tIndex sBegin,tIndex sEnd, tDouble sSize,tMode sMode, tSheet* sSheet);
		/// @brief		destructor tUndoChangeSize.
		~tUndoChangeSize() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

           
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        ///@Brief Sheet
        ///@return tSheet
        tSheet* Sheet() override;
        
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
        
        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;
	};

   
	//=========================================================================
	//! Ancesto Undo Redo spreadSheet (callback treats cells and range)
	class tUndoSpreadSheetCallBack : public tUndoSpreadSheet {
    protected:
        // Sheet 
        tAllocatorRef  m_SheetAllocatorRef;
        // Name Sheet
        tSharedString  m_SheetName;
        
        // m_Ref
        tString         m_Ref;
        
        // m_Ref Rebase
        tString         m_RefRebase;
    private:
		//! State of undo
		tUndoState		m_State;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
 	public:
        /// @brief constructor of tUndoSpreadSheetCallBack.
        tUndoSpreadSheetCallBack();

        /// @brief constructor of tUndoSpreadSheetCallBack.
        /// @param[in] sSheet tSheet
        /// @param[in] m_Ref tString
        /// @param[in] sMode tMode
        tUndoSpreadSheetCallBack(tAllocatorRef sSheetAllocator,tString sRef, tMode sMode);

        /// @brief constructor of tUndoSpreadSheetCallBack.
        /// @param[in] sSheetName  tString
        ///  /// @param[in] m_Ref tString
        /// @param[in] sMode tMode
        tUndoSpreadSheetCallBack(tString sSheetName,tString sRef, tMode sMode);
        
		/// @brief		destructor tUndoSpreadSheetCallBack.
		~tUndoSpreadSheetCallBack() override;  

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
		/// @brief		Set UndoState.
		/// @param[in]	sState
		void UndoState(tUndoState sState);

		/// @brief		Get UndoState.
		/// @return		tUndoState
		tUndoState UndoState();

		/// @brief		Call back for cell in tIndex coordinate.
		/// @param[in]	sPoint
		virtual tBool CallBackCell(tTempoPoint* sPoint);

		/// @brief		Call back for cell in tIndex coordinate.
		/// @param[in]	sRect
		virtual tBool CallBackRange(tTempoRect* sRect);
        
        /// @brief      Get Sheet
        /// @return tSheet*:*
        tSheet* Sheet() override;

        /// @brief Rebind m_SheetAllocatorRef from WorkBookTarget / active workbook sheet.
        void RebindActiveSheet();
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

		// Rebase ===============================================================
		/// @brief      Get SaveSelect for rebase (returns nullptr if not available)
		/// @return     tSaveSelect* pointer to SaveSelect or nullptr
		tSaveSelect* SaveSelect() override;
        
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;
        
        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;

        /// @brief Set Ref  
        /// @param[in] sRef tString
        void Ref(tString sRef);

        /// @brief Set Sheet
        /// @param[in] sSheetName tString
        void SheetName(tString sSheetName);

#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        tString Debug() override;
#endif
	};

    //=========================================================================
    //! Undo JSON Payload - manages undo/redo for cell JSON payload
    class tUndoJsonPayload : public tUndoSpreadSheetCallBack {
    private:
        //! Old JSON payload (for undo)
        tString m_JsonPayload;
        
        tCellClassContainer::tMapPointJsonPayLoad m_MapSaveJsonPayload;
        
        //  We only pass once per cell
        tClassVector<tPoint>  m_ContainerCellDo;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        void Json(const rapidjson::Value& sValue) override;
        
    public:
        /// @brief      Constructor tUndoJsonPayload.
        tUndoJsonPayload();

        /// @brief      Constructor tUndoJsonPayload with parameters.
        /// @param[in]  sRef tString
        /// @param[in]  sJsonPayload tString
        /// @param[in]  sMode tMode
        /// @param[in] sSheet tSheet
        tUndoJsonPayload(tString sRef, tString sJsonPayload, tMode sMode, tSheet* sSheet);

        /// @brief      Destructor tUndoJsonPayload.
        ~tUndoJsonPayload() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Apply Do (set new JSON).
        /// @return     tBool false if not Ok
        tBool Do() override;

        /// @brief      Apply Undo (restore old JSON).
        /// @return     tBool false if not Ok
        tBool Undo() override;

        /// @brief      Call back for cell coordinate.
        /// @param[in]  sPoint tTempoPoint*
        /// @return     tBool
        tBool CallBackCell(tTempoPoint* sPoint) override;

               
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        /// @brief      Rebase coordinates after structural operations
        /// @return     tBool false if rebase failed (cells deleted)
        tBool Rebase() override;
    };
    //=========================================================================
    //! Undo Apply merge
    //! Call twice to create and delete the merged range
    class tUndoApplyMerge : public tUndoSpreadSheetCallBack {
    private:
        //! Selection    (parse of each do)
        tSelect*     m_Select;
        
        tBool        m_Erase;

        //! Previously existing merges absorbed by the current merge
        //! operation. When the user merges A1:C2 and a merge B1:C2 already
        //! exists inside, B1:C2 is unmerged and saved here so Undo/Redo
        //! (including network replay on other clients) can restore it.
        tVectorRect  m_AbsorbedMerges;
    protected:
         // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
       /// @brief constructor of tUndoApplyMerge.
        tUndoApplyMerge();

        /// @brief constructor of tUndoApplyMerge.  
        /// @param[in] sRef tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoApplyMerge(tString sRef, tMode sMode, tSheet* sSheet);

        /// @brief        destructor tUndoApplyMerge.
        ~tUndoApplyMerge() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        /// @brief Call back Rect.
        /// @param[in] sRect tRect*
        tBool CallBackRange(tTempoRect* sRect)  override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
    };


	//=========================================================================
	//! Undo to clear cells
	class tUndoRaz : public tUndoSpreadSheetCallBack {
	protected:
        // Temporary (It is very important to reparse it at each Do)
		tSaveSelect m_SaveSelect;
        // When true, only the cell content (formula/variant) is cleared and the
        // format (CSS) is kept. Default false keeps the historical behavior of
        // clearing both content and format. Serialized for undo/redo & collaboration.
        tBool m_KeepFormat;
        // When true, only the cell format (CSS) is cleared: content, formula and
        // class attributes are kept. Mutually exclusive with m_KeepFormat (which
        // stays false in this mode). Serialized for undo/redo & collaboration.
        tBool m_FormatOnly;
        // Sheet-, row- and column-level CSS live on tSheet / tColRow, not on
        // individual cells. A format-only clear (m_FormatOnly) over a whole
        // sheet/row/column must clear and restore that CSS too; these mirror
        // tUndoFormat's ColRow save containers and are only used when m_FormatOnly.
        tSaveSelectColRow m_SaveSelectColRow_Rows;  // rows (m_DoesRow=true)
        tSaveSelectColRow m_SaveSelectColRow_Cols;  // columns (m_DoesRow=false)
        tFormatRef        m_CssSheet;               // saved whole-sheet CSS
        //! Tables fully inside the Raz selection, removed on Do and restored on Undo.
        struct tRazDeletedTable {
            tString m_Name;
            tString m_Ref;
            tRangeData m_RangeData;
        };
        std::vector<tRazDeletedTable> m_DeletedTables;
    protected:
        void CollectFullyContainedTables();
        void DeleteCollectedTables();
        void RestoreDeletedTables();
        void JsonDeletedTables(Writer<StringBuffer>* sWriter) const;
        void JsonDeletedTables(const rapidjson::Value& sValue);
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
        /// @brief constructor of tUndoRaz.
        tUndoRaz();

        /// @brief constructor of tUndoRaz.
        /// @param[in] sRef tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
		tUndoRaz(tString sRef, tMode sMode, tSheet* sSheet);

		/// @brief destructor of tUndoRaz.
		~tUndoRaz() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        ///@ brief Return tSaveSelect
        ///@return tSaveSelect
        tSaveSelect* SaveSelect() override;

        /// @brief      Set keep format flag (clear content only, keep CSS format).
        /// @param[in]  sKeepFormat tBool true to keep format
        void KeepFormat(tBool sKeepFormat);

        /// @brief      Return keep format flag.
        /// @return     tBool
        tBool KeepFormat() const;

        /// @brief      Set format-only flag (clear CSS format only, keep content and class attributes).
        /// @param[in]  sFormatOnly tBool true to clear format only
        void FormatOnly(tBool sFormatOnly);

        /// @brief      Return format-only flag.
        /// @return     tBool
        tBool FormatOnly() const;
        
		/// @brief Call back cell.
		/// @param[in] sPoint tPoint*
		tBool CallBackCell(tTempoPoint* sPoint)  override;

		/// @brief Call back Rect.
		/// @param[in] sRect tRect*
		tBool CallBackRange(tTempoRect* sRect)  override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;

		/// @brief      Calculate for do & undo.
		virtual void CalculateDo();
               
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif

#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        tString Debug() override;
#endif
	};

	//=========================================================================
	//!  Undo to apply values or formulae to cell
	class tUndoCellValue : public tUndoRaz {
    protected:
        // Origin
        tVariant m_Value;
        // Formula (Rebased
        tVariant m_ValueRebased;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        /// @brief constructor of tUndoCellValue.
        tUndoCellValue();

        /// @brief constructor of tUndoCellValue.
        /// @param[in] sRef tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoCellValue(tString sRef, tMode sMode, tSheet* sSheet);

        /// @brief constructor of tUndoCell.
        /// @param[in] sRef tString
        /// @param[in] sValue  tVariant
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoCellValue(tString sRef, tVariant& sValue, tMode sMode, tSheet* sSheet);

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        /// @brief Call back cell.
        /// @param[in] sPoint tPoint*
        tBool CallBackCell(tTempoPoint* sPoint) override;
        
        tBool Undo() override;
	};

	//=========================================================================
	//! Undo to apply a distinct value per cell (Excel-like fill-handle series)
	//! as ONE undo. Reuses the tUndoCellValue / tUndoRaz save-restore mechanism;
	//! only the Do pass differs: each cell picks its target value from m_Values
	//! by its row-major index inside the destination rectangle.
	class tUndoFillSeries : public tUndoCellValue {
	private:
		//! Target value strings, row-major over the destination rectangle
		//! (index = (row - m_Row0) * m_Width + (col - m_Col0)).
		tVectorString m_Values;
		//! Destination rectangle origin and width, computed at Do() from m_RefRebase.
		tIndex m_Row0;
		tIndex m_Col0;
		tIndex m_Width;

		//! Parse m_RefRebase into m_Row0 / m_Col0 / m_Width.
		void ComputeRect();

		//! Row-major index of a cell inside the destination rectangle.
		tSize CellIndex(tTempoPoint* sPoint) const;
	protected:
		// Json ===============================================================
		/// @brief      Write JSON.
		/// @param[in]  sWriter Writer<StringBuffer>*
		void Json(Writer<StringBuffer>* sWriter) override;

		/// @brief      Read JSON.
		/// @param[in]  sValue Value&
		void Json(const rapidjson::Value& sValue) override;
	public:
		/// @brief constructor of tUndoFillSeries.
		tUndoFillSeries();

		/// @brief constructor of tUndoFillSeries.
		/// @param[in] sRef tString destination range (single rectangle, e.g. A3:A12)
		/// @param[in] sValues tVectorString row-major target values
		/// @param[in] sMode tMode
		/// @param[in] sSheet tSheet
		tUndoFillSeries(tString sRef, const tVectorString& sValues, tMode sMode, tSheet* sSheet);

		/// @brief      Return ClassName
		/// @return     tString
		tString ClassName() const override;

		/// @brief Call back cell.
		/// @param[in] sPoint tPoint*
		tBool CallBackCell(tTempoPoint* sPoint) override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do() override;
	};

	//=========================================================================
	//! One cell-class attribute name/value pair for batch undo.
	struct tCellAttributeWire {
		tString Name;
		tVariant Value;
	};
	typedef std::vector<tCellAttributeWire> tVectorCellAttributeWire;

	//=========================================================================
	//! Undo for set Attribute Value
	class tUndoCellAttribute : public tUndoRaz {
	private:
		tSharedString   m_Attribute;
		tVariant		m_Variant;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
       /// @brief constructor of tUndoAttribute.
        tUndoCellAttribute();

        /// @brief constructor of tUndoAttribute.
        /// @param[in] sRef tString
        /// @param[in] sAttribute  tString
        /// @param[in] sVariant  tVariant
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoCellAttribute(tString sRef, tString sAttribute, tVariant sVariant, tMode sMode, tSheet* sSheet);

		/// @brief destructor of tUndoAttribute.
		~tUndoCellAttribute() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief Call back cell.
		/// @param[in] sPoint tPoint*
		tBool CallBackCell(tTempoPoint* sPoint) override;

		/// @brief      Calculate for do & undo.
		void CalculateDo() override;

	};

	//=========================================================================
	//! Undo for set multiple attribute values on one cell-class host (atomic).
	class tUndoCellClassAttributes : public tUndoRaz {
	private:
		struct tItem {
			tSharedString m_Name;
			tVariant m_Value;
		};
		std::vector<tItem> m_Attributes;

		void RestoreAttribute(tSheet* sSheet, tIndex sRow, tIndex sCol, const tString& sAttribute);
		void RestoreAllAttributes(tSheet* sSheet, tIndex sRow, tIndex sCol);
	protected:
		void Json(Writer<StringBuffer>* sWriter) override;
		void Json(const rapidjson::Value& sValue) override;
	public:
		tUndoCellClassAttributes();
		tUndoCellClassAttributes(tString sRef, const tVectorCellAttributeWire& sAttributes, tMode sMode, tSheet* sSheet);
		~tUndoCellClassAttributes() override;

		tString ClassName() const override;
		tBool CallBackCell(tTempoPoint* sPoint) override;
		void CalculateDo() override;
	};

    //=========================================================================
    //! Undo for set calculable scalar on tCellClassAttribute (tVariantClass::m_Value)
    class tUndoCellClassCalculable : public tUndoRaz {
    private:
        tVariant m_Variant;
    protected:
        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;
    public:
        tUndoCellClassCalculable();
        tUndoCellClassCalculable(tString sRef, tVariant sVariant, tMode sMode, tSheet* sSheet);
        ~tUndoCellClassCalculable() override;
        tString ClassName() const override;
        tBool CallBackCell(tTempoPoint* sPoint) override;
        void CalculateDo() override;
    };

    //=========================================================================
	//! Undo to place classes in cells
	class tUndoCellClass : public tUndoCellValue {
	private:
		tString      m_ClassName;
        tBool        m_ApplyNumeric;
     protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
        /// @brief constructor of tUndoCellClass.
        tUndoCellClass();

		/// @brief constructor of tUndoCellClass
        /// @param[in] sRef tString
        /// @param[in] sClassName  tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
		tUndoCellClass(tString sRef, tString sClassName, tMode sMode, tSheet* sSheet);
        
        /// @brief constructor of tUndoCellClass
        /// @param[in] sRef tString
        /// @param[in] sVariant tVariant
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoCellClass(tString sRef, tVariant* sVariant, tMode sMode, tSheet* sSheet);

		/// @brief destructor of tUndoCellClass.
		~tUndoCellClass() override;
        

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief Call back cell.
		/// @param[in] sPoint tPoint*
		tBool CallBackCell(tTempoPoint* sPoint) override;
	};

    //=========================================================================
    //!  Undo to apply format on  Celll
    class tUndoFormat : public tUndoSpreadSheetCallBack {
    protected:
        tFormatRef        m_CssSheet;
        tSaveSelectColRow m_SaveSelectColRow_Rows;  // For rows (m_DoesRow=true)
        tSaveSelectColRow m_SaveSelectColRow_Cols;  // For columns (m_DoesRow=false)
        tString            m_Format;
        
        //  We only pass once per cell
        tClassVector<tPoint>  m_ContainerCellDo;
    
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        /// @brief constructor of tUndoFormat.
        tUndoFormat();  

        /// @brief constructor of tUndoFormat.
        /// @param[in] sRef tString
        /// @param[in] sFormat  tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoFormat(tString sRef, tString sFormat, tMode sMode, tSheet* sSheet);
            
        /// @brief destructor of tUndoFormat.
        ~tUndoFormat() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Return SaveSelect for rebase
        /// @return     tSaveSelect* pointer to SaveSelectColRow
        tSaveSelect* SaveSelect() override;

        /// @brief Call back cell.
        /// @param[in] sPoint tPoint*
        tBool CallBackCell(tTempoPoint* sPoint) override;
        
        /// @brief Call back Rect.
        /// @param[in] sRect tRect*
        tBool CallBackRange(tTempoRect* sRect)  override;
        
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;
        
        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;

               
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;
        
        ///@brief Rebase both rows and columns instances
        ///@return  tBool true if rebase successful, false if any cell/range was deleted
        tBool Rebase() override;
        
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif

    };

    //=========================================================================
    //!  Undo inc or dec  format precision on  Celll
    class tUndoPrecision : public tUndoFormat {
    private:
        tBool m_Inc;
        tBool m_LeastOneElement;
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        tUndoPrecision();
        /// @brief constructor of tUndoPrecision.
        /// @param[in] sRef tString
        /// @param[in] sInc tBool
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoPrecision(tString sRef, tBool sInc, tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoPrecision.
        ~tUndoPrecision() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief Call back cell.
        /// @param[in] sPoint tPoint*
        tBool CallBackCell(tTempoPoint* sPoint) override;
        
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;
        
    };

    //=========================================================================
    //!  Undo to apply format border  on  Celll
    class tUndoBorder : public tUndoFormat {
    private:
        tShort m_Border;
        // Merged (cell -> side mask) across all selection areas; avoids double-apply on overlap (e.g. A1:C2;B2:B3).
        std::map<tPoint, tShort> m_CombinedCellMasks;
        tBool m_UseCombinedCellMasks;

        void BuildCombinedCellMasks();
        void ApplyCombinedCellMasks();
    protected:
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
       /// @brief constructor of tUndoBorder.
        tUndoBorder();

        /// @brief constructor of tUndoBorder.
        /// @param[in] sRef tString
        /// @param[in] sBorder tShort
        /// @param[in] sFormat  tString if Format="" Raz border
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoBorder(tString sRef, tShort sBorder, tString sFormat, tMode sMode, tSheet* sSheet);
            
        /// @brief destructor of tUndoBorder.
        ~tUndoBorder() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        // Border =============================================================
        /// @brief     delete Border
        /// @param[in] sBorderType tShort
        /// @param[in] sFormat tString
        /// @return tString
        tString FormatBorder(tShort sBorderType,tString sFormat);
            
        /// @brief Call Cell for apply Borderl.
        /// @param[in] sPoint tTempoPoint*
        /// @param[in] sBorder tBorder
        void ApplyBorder(tTempoPoint* sPoint, tShort sBorder);

        /// @brief Fill a per-cell side-mask map from a range and m_Border (cell-local: all sides on the owning cell).
        /// @param[in]  sRow0    first row of the range (inclusive)
        /// @param[in]  sCol0    first col of the range (inclusive)
        /// @param[in]  sRow1    last row of the range (inclusive)
        /// @param[in]  sCol1    last col of the range (inclusive)
        /// @param[out] sMasks   map (point -> combined mask) to populate
        void FillCellMasks(tIndex sRow0, tIndex sCol0,
                           tIndex sRow1, tIndex sCol1,
                           std::map<tPoint, tShort>& sMasks);

        /// @brief Call back cell.
        /// @param[in] sPoint tPoint*
        tBool CallBackCell(tTempoPoint* sPoint) override;
        
        /// @brief Call back Rect.
        /// @param[in] sRect tRect*
        tBool CallBackRange(tTempoRect* sRect)  override;
        
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
    };

	//=========================================================================
	//! Undo for apply Paste
	class tUndoPaste : public tUndoRaz {
	protected:
        tString  m_Copy;
       
        /// @brief  Treat selection
        /// @param sIsDo 
        /// @return 
        tBool TreatSelection(tBool sIsDo);
        
        /// Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
       /// @brief constructor of tUndoPaste.
        tUndoPaste();

        /// @brief constructor of tUndoPaste.
        /// @param[in] sRef tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        /// @param[in] sLoadFromClipboard when true, seed m_Copy from the app clipboard (paste only)
		tUndoPaste(tString sRef, tMode sMode, tSheet* sSheet, tBool sLoadFromClipboard = true);

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;

        /// @brief Copied JSON byte size (collaboration payload estimate).
        /// @return tSize
        tSize CollaborationCopyByteSize() const { return m_Copy.size(); }

        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;


        /// @brief      Rebase coordinates after structural operations
        /// @return     tBool false if rebase failed (cells deleted)
        tBool Rebase() override;
	};

	//=========================================================================
	//! Undo for cut selection (copy to clipboard + clear source, paste later)
	class tUndoCut : public tUndoRaz {
	protected:
		tString m_Copy;

		void ApplyCopyToClipboard();

		void Json(Writer<StringBuffer>* sWriter) override;
		void Json(const rapidjson::Value& sValue) override;

	public:
		tUndoCut();
		tUndoCut(tString sRef, tMode sMode, tSheet* sSheet);

		tString ClassName() const override;

		/// @brief Build m_Copy from the cut selection on the current target sheet.
		tBool EnsureCopyPayload();

		tBool Do() override;

		void DeleteBeforeDo() override;

		tSize CollaborationCopyByteSize() const { return m_Copy.size(); }

        /// @brief Clipboard still carries this cut payload (paste-after-cut detection).
        tBool MatchesCutClipboard() const;

        /// @brief Copy JSON built at cut time.
        const tString& CopyPayload() const { return m_Copy; }
	};

	//=========================================================================
	//! Undo for move selection (copy buffer + raz source + paste dest)
	class tUndoMove : public tUndoPaste {
	protected:
        tString m_SourceRef;
        tString m_SourceRefRebase;
        tBool m_DidGridRelocate;
        tBool m_GridRelocateMovedCells;
        tBool m_SourceAlreadyCleared;

        tBool GetSourceDestRects(tRect& oSource, tRect& oDest) const;
        tBool SelectContainsCell(const tSelect& sSelect, tIndex sRow, tIndex sCol) const;
        void UndoSourceOutsideDest();
        void UndoDestWhenCopyHasNoCells();
        void RemapFormulaReferences(tBool sForward);
        void QueueUndoRestoredCellsForCalculate();
        static tBool RectsOverlap(tRect& sA, tRect& sB);
        tBool DoGridRelocate(tSelect& sSource, tSelect& sDest,
                             tRect& sSourceRect, tRect& sDestRect);
        tBool UndoGridRelocate(tSelect& sDest);

        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;
	public:
        tUndoMove();
        tUndoMove(tString sSourceRef, tString sDestRef, tMode sMode, tSheet* sSheet);

        tString ClassName() const override;

        /// @brief Same rectangle geometry (paste-after-cut merge eligibility).
        tBool MoveSelectSameGeometry(const tSelect& sSource, const tSelect& sDest) const;

        /// @brief Build m_Copy from source selection on the current target sheet.
        tBool EnsureCopyPayload();

        /// @brief Paste leg after tUndoCut Do (source already cleared; save absorbed).
        void AdoptCompletedCut(tUndoCut* sCut);

        tBool Do() override;
        tBool Undo() override;
        tBool Rebase() override;
        void DeleteBeforeDo() override;
	};

	//=========================================================================
	//! Undo for Insert RangeNamed 
    class tUndoAddRangeNamed : public tUndoSpreadSheetCallBack {
	protected:
        //! Name
        tSharedString  m_Name;
        
        //! Name
        tSharedString  m_OldName;
        //! Range Data
        tRangeData     m_RangeData;
      
        //! Old Range Data
        tRangeData     m_OldRangeData;
        
        
        /// Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;  

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
       /// @brief constructor of tUndoAddRangeNamed.
        tUndoAddRangeNamed();

        /// @brief constructor of tUndoAddRangeNamed.
        /// @param[in] sName tString
        /// @param[in] sRef tString
        /// @param[in] sSheet tSheet
		tUndoAddRangeNamed(tString sName,tString sRef,tMode sMode, tSheet* sSheet);

		/// @brief destructor of tUndoAddRangeNamed.
		~tUndoAddRangeNamed() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
		
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;
	};

    //! Undo Add RangeData
    class tUndoAddRangeData :public tUndoAddRangeNamed {
    protected:
        //! Json Data
       tString m_JsonData;
    public:
        tUndoAddRangeData();

        /// @brief Constructor for inserting a named range with optional RangeData JSON.
        /// @param[in] sName Range name
        /// @param[in] sJsonData RangeData payload (JSON string); if empty, descriptors are built from the sheet using @p sRef (see Do).
        /// @param[in] sRef Sheet range reference (e.g. A1:D10), single area only; required when @p sJsonData is empty.
        /// @param[in] sMode Undo mode
        /// @param[in] sSheet Target sheet
        tUndoAddRangeData(tString sName,tString sJsonData, tString sRef,tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoAddRangeNamed.
        ~tUndoAddRangeData() override;
        
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        /// @brief Load RangeData from JSON or, when JSON is empty, from the first row of the stored range ref via FillByRect.
        /// @return false if sheet/ref invalid, FillByRect fails, or resulting RangeData is empty
        tBool Do()  override;
    };
    
    //! Undo Apply RangeData
    class tUndoApplyRangeData :public tUndoAddRangeNamed {
    protected:
        tString m_JsonData;
    public:
        tUndoApplyRangeData();

        /// @brief constructor of tUndoApplyRangeData.
        /// @param[in] sName tString Name of the range
        /// @param[in] sJsonData tString JSON data for RangeData (must be non-empty and parse to column metadata)
        /// @param[in] sMode tMode Undo mode
        /// @param[in] sSheet tSheet* Sheet (if nullptr, uses active sheet)
        tUndoApplyRangeData(tString sName,tString sJsonData,tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoApplyRangeData.
        ~tUndoApplyRangeData() override;
        
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;
        
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;
        
        /// @brief      Apply UnDo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
    };


	//! Undo for Delete RangeNamed 
	class tUndoDeleteRangeNamed : public tUndoAddRangeData {
    protected:
        tSaveRangeNamed   m_SaveRangeNamed;
	public:
        /// @brief constructor of tUndoDeleteRangeNamed.
        tUndoDeleteRangeNamed();

		/// @brief constructor of tUndoDeleteRangeNamed.
        /// @param[in] sName tString
        /// @param[in] sSheet tSheet
		tUndoDeleteRangeNamed(tString sName,tMode sMode, tSheet* sSheet);

		/// @brief destructor of tUndoDeleteRangeNamed.
		~tUndoDeleteRangeNamed() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
  
    /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;


        /// @brief      Rebase coordinates after structural operations
        /// @return     tBool false if rebase failed (cells deleted)
        tBool Rebase() override;

        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;
	};


    //! Undo for Update (rename and/or change selection) of a named range.
    //! Preserves tAllocatorRef of areas kept between old and new selections
    //! (they are fetched via EnsureRange so existing tRange objects are
    //! reused). Formulas that reference the old name keep evaluating — their
    //! VectorRef still points at the same tRange objects — and the display
    //! string automatically picks up the new name through
    //! tFormula::ClassOrRangeStr's m_MapRef-based lookup.
    class tUndoUpdateRangeNamed : public tUndoSpreadSheetCallBack {
    protected:
        //! Target name (what the named range will be called after Do()).
        tSharedString   m_NewName;

        //! Source name (what the named range was called before Do()).
        tSharedString   m_OldName;

        //! Old ref string joined (";"-separated), captured during Do() so
        //! Undo() can restore the original selection. m_RefRebase (base
        //! class) holds the new ref string.
        tString         m_OldRef;

        //! Snapshot of RangeData when the renamed entry was a table (IsData).
        //! Empty when the named range was plain (no table metadata).
        tRangeData      m_RangeData;

        // Json ===============================================================
        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;
    public:
        tUndoUpdateRangeNamed();

        /// @brief constructor.
        /// @param[in] sOldName current name of the range
        /// @param[in] sNewName desired name after update (can equal sOldName
        ///            if only the selection changes)
        /// @param[in] sNewRef new selection string (e.g. "A1:A2;B1:B2")
        /// @param[in] sSheet sheet the range lives on (must stay single-sheet)
        tUndoUpdateRangeNamed(tString sOldName, tString sNewName,
                              tString sNewRef, tMode sMode, tSheet* sSheet);

        ~tUndoUpdateRangeNamed() override;

        tString ClassName() const override;

        tBool Do()  override;
        tBool Undo()  override;
        void DeleteBeforeDo() override;

        //! Override Rebase so that both the new selection (m_RefRebase,
        //! handled by the base) AND the captured old selection (m_OldRef,
        //! used by Undo() to restore the previous state) are rebased when
        //! col/row operations happen after Do().
        tBool Rebase() override;
    };


    //=========================================================================
    //! Undo for Apply Conditional Format
    class tUndoConditionaFormat : public tUndoSpreadSheetCallBack {
    protected:
        // Key of Conditional Format
        tConditionalFormatType m_Type;

        tConditionalFormat* m_ConditionalFormat;
        
        tString m_StringType;
        tString m_Param1;
        tString m_Param2;
        tString m_Param3;
        tString m_Param4;
        tString m_Param5;
        tString m_Param6;
        tString m_Param7;
        tString m_Param8;
        tString m_Param9;
        tString m_Param10;
        tString m_TypeIcon;

        // Snapshot before Do() when updating an existing rule (used by Undo()).
        tBool m_HadExistingBeforeDo;
        tString m_SaveParam1;
        tString m_SaveParam2;
        tString m_SaveParam3;
        tString m_SaveParam4;
        tString m_SaveParam5;
        tString m_SaveParam6;
        tString m_SaveParam7;
        tString m_SaveParam8;
        tString m_SaveParam9;
        tString m_SaveParam10;
        tIconType m_SaveIconType;

        void SaveParamsFromFormat(tConditionalFormat* sConditionalFormat);
        tBool ApplyParamsToFormat(
            tConditionalFormat* sConditionalFormat,
            tConditionalFormatType sType,
            const tString& sParam1,
            const tString& sParam2,
            const tString& sParam3,
            const tString& sParam4,
            const tString& sParam5,
            const tString& sParam6,
            const tString& sParam7,
            const tString& sParam8,
            const tString& sParam9,
            const tString& sParam10,
            tIconType sIconType);
        void RefreshConditionalFormatVisuals(tConditionalFormat* sConditionalFormat);
        /// @return     tString
        tString Key();
        
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        /// @brief constructor of tUndoFormat.
        tUndoConditionaFormat();

        /// @brief constructor of tUndoFormat.
        /// @param[in] sRef tString
        /// @param[in] sType tConditionalFormatType
        /// @param[in] sFormula tString
        /// @param[in] sParam1 tString
        /// @param[in] sParam2 tString
        /// @param[in] sParam3 tString
        /// @param[in] sParam4 tString
        /// @param[in] sParam5 tString
        /// @param[in] sParam6 tString
        /// @param[in] sParam7  tString
        /// @param[in] sParam8 tString
        /// @param[in] sParam9 tString
        /// @param[in] sParam10 tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoConditionaFormat(tString sRef, tString sType, tString sParam1, tString sParam2, tString sParam3, tString sParam4, tString sParam5, tString sParam6, tString sParam7, tString sParam8, tString sParam9, tString sParam10, tMode sMode, tSheet* sSheet);
        
        /// @brief destructor of tUndoFormat.
        ~tUndoConditionaFormat() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

    
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;
        
        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;

        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;
        
        /// @brief      Rebase conditional format
        /// @return     tBool false if not Ok
        tBool Rebase() override;
        
    #ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
    #endif
    };

    //=========================================================================
    //! Undo for Delete Conditional Format
    class tUndoDeleteConditionaFormat : public tUndoConditionaFormat {
    protected:
    private:
     
    public:
        /// @brief constructor of tUndoDeleteConditionaFormat.
        tUndoDeleteConditionaFormat();

        /// @brief constructor of tUndoDeleteConditionaFormat.
        /// @param[in]  sType tString
        /// @param[in] sRef tString
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
        tUndoDeleteConditionaFormat(tString sRef,tString sType,  tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoDeleteConditionaFormat.
        ~tUndoDeleteConditionaFormat() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
    };

    //=========================================================================
    // Ancestor tUndo
    class tUndoAncestorDeleteInsert : public tUndoSpreadSheetCallBack {
    protected:
        tBool        m_DoesRow;
        tIndex       m_Position;
        tIndex       m_Size;
        tRect        m_Rect;
        
        tIndex       m_PositionRebase;
        tRect        m_RectRebase;
        
        tSaveSelectErase m_SaveSelectErase;
        
        /// @brief Rebase Col
        /// @return tBool true if rebase successful, false if any cell/range was deleted
        tBool RebaseCol();
        
        /// @briief Rebase Row
        /// @return tBool true if rebase successful, false if any cell/range was deleted
        tBool RebaseRow();
  
        /// Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
        tUndoAncestorDeleteInsert(tBool sDoesRow, tIndex sPosition, tIndex sSize, tMode sMode, tSheet* sSheet);
        
        tUndoAncestorDeleteInsert(tBool sDoesRow, tRect sRect,tMode sMode, tSheet* sSheet);
        
        ///@brief DoesRow
        ///@return tBool
        tBool DoesRow();
        
        // Interface Rebase
        /// @brief      GetPosition
        /// @return     ttIndex
        tIndex Position();
        
        /// @brief      GetPositionRebase
        /// @return     ttIndex
        tIndex PositionRebase();
        
        /// @brief      GetSize
        /// @return     ttIndex
        tIndex Size();
        
        /// @brief      GetRect
        /// @return     ttRect
        tRect Rect();
        
        /// @brief      GetRectRebase
        /// @return     ttRect
        tRect RectRebase();
        
               
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;
        
        ///@brief Rebase (override to handle rect operations)
        ///@return  tBool true if rebase successful, false if any cell/range was deleted
        tBool Rebase() override;
    };

    //=========================================================================
	//! Undo for Insert Col
	class tUndoInsertCol : public tUndoAncestorDeleteInsert {
	public:
        /// @brief constructor of tUndoInsertCol.
        tUndoInsertCol();

        /// @brief constructor of tUndoInsertCol.
        /// @param[in] sPosition tIndex
        /// @param[in] sSize  tIndex
        /// @param[in] sSheet tSheet
		tUndoInsertCol(tIndex sPosition,tIndex sSize,tMode sMode, tSheet* sSheet);

        /// @brief constructor of tUndoInsertCol with Rect.
        /// @param[in] sRect tRect
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
		tUndoInsertCol(tRect sRect,tMode sMode, tSheet* sSheet);

        /// @brief destruxtor  of tUndoInsertCol.
        ~tUndoInsertCol() override;
		        
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;
	
		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
        

		// Rebase ===============================================================
		/// @brief      Get SaveSelect for rebase
		/// @return     tSaveSelect* pointer to SaveSelectErase
		tSaveSelect* SaveSelect() override;
        
        ///@brief       Rebase
        tBool Rebase() override;

#ifdef checkfo
        /// @brief Count formats held in SaveSelectErase (undo/redo stack) for CheckFormat.
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
	};

	//=========================================================================
	//! Undo for Insert Row
	class tUndoInsertRow : public tUndoAncestorDeleteInsert {
	public:
       /// @brief constructor of tUndoInsertRow.
        tUndoInsertRow();

        /// @brief constructor of tUndoInsertRow .
        /// @param[in] sPosition tIndex
        /// @param[in] sSize  tIndex
        /// @param[in] sSheet tSheet
		tUndoInsertRow(tIndex sPosition, tIndex sSize,tMode sMode, tSheet* sSheet);
        
        /// @brief constructor of tUndoInsertRow by Rect .
        /// @param[in] sRect  tRect
        /// @param[in] sSheet tSheet
        tUndoInsertRow(tRect sRect,tMode sMode, tSheet* sSheet);
        
		~tUndoInsertRow() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
        
		// Rebase ===============================================================
		/// @brief      Get SaveSelect for rebase
		/// @return     tSaveSelect* pointer to SaveSelectErase
		tSaveSelect* SaveSelect() override;
        
        ///@brief       Rebase
        tBool Rebase() override;

#ifdef checkfo
        /// @brief Count formats held in SaveSelectErase (undo/redo stack) for CheckFormat.
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
	};

	//=========================================================================
	//! Insert row by rect and write a label cell in the same undo (table totals row).
	class tUndoInsertRowWithLabel : public tUndoInsertRow {
	protected:
		tSharedString m_LabelRef;
		tSharedString m_LabelValue;

		void Json(Writer<StringBuffer>* sWriter) override;
		void Json(const rapidjson::Value& sValue) override;

		/// Write m_LabelValue into m_LabelRef after the row insert (Do / Redo).
		tBool WriteLabel();
	public:
		tUndoInsertRowWithLabel();
		tUndoInsertRowWithLabel(tRect sRect, tString sLabelRef, tString sLabelValue,
								tMode sMode, tSheet* sSheet);
		~tUndoInsertRowWithLabel() override;

		tString ClassName() const override;
		tBool Do() override;
	};

	//=========================================================================
	//! Undo for Delete Col
	class tUndoDeleteCol : public tUndoAncestorDeleteInsert {
	public:
       /// @brief constructor of tUndoDeleteCol.
        tUndoDeleteCol();

        /// @brief constructor of tUndoDeleteCol.
        /// @param[in] sPosition tIndex
        /// @param[in] sSize  tIndex
        /// @param[in] sSheet tSheet
		tUndoDeleteCol(tIndex sPosition, tIndex sSize,tMode sMode, tSheet* sSheet);

        /// @brief constructor of tUndoDeleteCol.
        /// @param[in] sRect tRect
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet
		tUndoDeleteCol(tRect sRect,tMode sMode, tSheet* sSheet);

		~tUndoDeleteCol() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
    

		// Rebase ===============================================================
		/// @brief      Get SaveSelect for rebase
		/// @return     tSaveSelect* pointer to SaveSelectErase
		tSaveSelect* SaveSelect() override;
        
        ///@brief       Rebase
        tBool Rebase() override;
        
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        tString Debug() override;
#endif
	};

	//=========================================================================
	//! Undo for Delete Row
	class tUndoDeleteRow : public tUndoAncestorDeleteInsert {
	public:
       /// @brief constructor of tUndoDeleteRow.
        tUndoDeleteRow();

        /// @brief constructor of tUndoDeleteRow.
        /// @param[in] sPosition tIndex
        /// @param[in] sSize  tIndex
        /// @param[in] sSheet tSheet
		tUndoDeleteRow(tIndex sPosition, tIndex sSize,tMode sMode, tSheet* sSheet);
        
        
        /// @brief constructor of tUndoInsertRow by Rect .
        /// @param[in] sRect  tRect
        /// @param[in] sSheet tSheet
        tUndoDeleteRow(tRect sRect,tMode sMode, tSheet* sSheet);
        
		~tUndoDeleteRow() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
        
        // Interface Rebase
		// Rebase ===============================================================
		/// @brief      Get SaveSelect for rebase
		/// @return     tSaveSelect* pointer to SaveSelectErase
		tSaveSelect* SaveSelect() override;
        
        ///@brief       Rebase
        tBool Rebase() override;
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
#ifdef _DEBUGSK
        /// @brief      Debug
        /// @return     tString
        tString Debug() override;
#endif
	};

    //=========================================================================
    //! Undo for Add Sheet
    class tUndoAddSheet : public tUndoSpreadSheet {
    protected:
        ///! Name  of Sheet
        tSharedString   m_Name;
        
        /// Insert at Left Position
        tSharedString   m_SheetLeft;
     
        /// Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;  

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
       /// @brief constructor of tUndoAddSheet.
        tUndoAddSheet();

        /// @brief constructor of tUndoAddSheet.
        /// @param[in] sName tString
        /// @param[in] sSheetLeft tString
        tUndoAddSheet(tString sName,tString sSheetLeft,tMode sMode);
        ~tUndoAddSheet() override;
        
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;

    };

    //=========================================================================
    //! Undo for Rename  Sheet
    class tUndoRenameSheet : public tUndoSpreadSheet {
    protected:
        //! Name of Sheet
        tSharedString      m_Name;
        
        //! New Name of Sheet
        tSharedString      m_NewName;
         /// Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;  

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
       /// @brief constructor of tUndoRenameSheet.
        tUndoRenameSheet();

        /// @brief constructor of tUndoRenameSheet.
        /// @param[in] sName tString
        /// @param[in] sNewName  tString
        tUndoRenameSheet(tString sName,tString sNewName,tMode sMode);
        ~tUndoRenameSheet() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;
        
#ifdef TestMultiUser
        ///@brief   Set WorkBook Target
        /// @param[in] sWorkBookTarger tString
        void WorkBookTarget(tString sWorkBookTarget);
#endif
    };

    //=========================================================================
    //! Undo for Rename  Sheet
    class tUndoSwapSheet : public tUndoSpreadSheet {
    protected:
        ///! From Position
        tSharedString      m_Name1;
        
        ///! To Position
        tSharedString      m_Name2;

        //! true: move m_Name1 after m_Name2; false: swap m_Name1 and m_Name2
        tBool m_InsertAfter;

        //! Sheet left of m_Name1 before Do (insert-after undo restore)
        tSharedString m_OldAfter;
       
        // Json ===============================================================
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
    public:
       /// @brief constructor of tUndoSwapSheet.
        tUndoSwapSheet();

        /// @brief constructor of tUndoSwapSheet.
        /// @param[in] sName1 tString
        /// @param[in] sName2  tString
        /// @param[in] sInsertAfter tBool move sName1 after sName2 when true
        tUndoSwapSheet(tString sName1,tString sName2,tMode sMode,tBool sInsertAfter = false);
        ~tUndoSwapSheet() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;   

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

       
        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;
    };

    
	//=========================================================================
	//! Undo for Delete Sheet
	class tUndoDeleteSheet : public tUndoSpreadSheetCallBack {
	protected:
        //! Index  Sheet
        tIndex           m_Index;
        
        ///! Is Redo
        tBool            m_Redo;
        
        ///! Save
		tSaveSelectErase m_SaveSelectErase;
        
        ///! Insert at Left Position Undo
        tSharedString    m_SheetLeft;
    
        ///!  in mode Client or Alone Keep Sheet
        tAllocatorRef    m_JsonSheetAllocator;
        
        rapidjson::Document m_JsonSheet;
        rapidjson::Document m_JsonErase;
        
         //! @brief Insert Sheet
        //! @return tSheet*
        tSheet* InsertUndoSheet();

        /// Json ===============================================================
        /// @brief Write Sheet before delete
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void WriteJsonSheet(Writer<StringBuffer>* sWriter,tSheet* sSheet);

        
        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in] sSheet tSheet*
        void Json(Writer<StringBuffer>* sWriter) override;  

        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        /// @param[in] sSheet tSheet*
        void Json(const rapidjson::Value& sValue) override;
	public:
       /// @brief constructor of tUndoDeleteSheet.
        tUndoDeleteSheet();

        /// @brief constructor of tUndoDeleteSheet.
        /// @param[in] sName tString
        /// @param[in] sMode
        tUndoDeleteSheet(tString sName,tMode sMode);
		~tUndoDeleteSheet() override;
        
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

		/// @brief      Apply Do.
		/// @return     tBool false if not Ok
		tBool Do()  override;

		/// @brief      Apply Undo.
		/// @return     tBool false if not Ok
		tBool Undo()  override;
#ifdef checkfo
        /// @brief        In check format
        /// @param[in] sFormatApi tFormatApi
        void IncCheckfo(tFormatApi* sFormatApi);
#endif
	};

//! Undo for Insert Formula Named
class tUndoInsertFormulaNamed : public tUndoSpreadSheet {
    private:
        tWorkBook*    m_WorkBook;
        //! Name
        tSharedString  m_Name;
        //! Formula
        tSharedString  m_Formula;
        //! Old Formula
        tSharedString  m_OldFormula;
        
        //! Row
        tIndex         m_Row;
    public:
        /// @brief constructor of tUndoInsertFormulaNamed.
        tUndoInsertFormulaNamed();

        /// @brief constructor of tUndoInsertFormulaNamed.
        /// @param[in] sName tString
        /// @param[in] sFormula tString
        /// @param[in] sMode tMode
        tUndoInsertFormulaNamed(tString sName,tString sFormula,tMode sMode,tSheet* sSheet);

        /// @brief destructor of tUndoInsertFormulaNamed.
        ~tUndoInsertFormulaNamed() override;


        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;

        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;

        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Rebase
        /// @return     tBool true if rebase successful, false if not
        tBool Rebase() override;

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.  
        /// @param[in]  sValue const rapidjson::Value&
        void Json(const rapidjson::Value& sValue) override;
};

//! Undo for Delete Formula Named
class tUndoDeleteFormulaNamed : public tUndoSpreadSheet {
    private:
        tWorkBook* m_WorkBook;
        tSharedString m_Name;
        tSharedString m_Formula;
        tIndex m_Row;
        tSaveSelect m_SaveSelect;
        //! Cells that referenced this named formula (Sheet!A1), for dependency restore on Undo.
        tVectorString m_DependentRefs;
    public:
        tUndoDeleteFormulaNamed();
        tUndoDeleteFormulaNamed(tString sName, tMode sMode, tSheet* sSheet);
        ~tUndoDeleteFormulaNamed() override;

        tString ClassName() const override;
        tSheet* Sheet() override;
        tBool Do() override;
        tBool Undo() override;
        void DeleteBeforeDo() override;
        tSaveSelect* SaveSelect() override;
        tString RefRebase() override;
        tBool Rebase() override;
        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;
};

//! Undo for insert / update floating object registry + host cell class on _$$A
class tUndoInsertFloatingObject : public tUndoSpreadSheet {
    private:
        tWorkBook* m_WorkBook;
        //! Floating object name
        tSharedString m_Name;
        //! Cell class applied on the host cell (tCellClassAttribute)
        tSharedString m_ClassName;
        //! Sheet where the object is displayed
        tSharedString m_TargetSheetName;
        //! Host row on CstSheetClassAnchor (_$$A)
        tIndex m_HostRow;
        //! True when Do() updates an existing entry instead of creating one
        tBool m_ExistedBefore;
        //! Previous class name (undo restore)
        tSharedString m_OldClassName;
        //! Previous target sheet (undo restore)
        tSharedString m_OldTargetSheetName;
        //! Previous host row (undo restore)
        tIndex m_OldHostRow;
        //! Previous layout (undo restore)
        tFloatingObjectLayout m_OldLayout;
        //! Optional layout applied atomically with insert (Apply float)
        tBool m_UseInitialLayout;
        tString m_InitialAnchorRef;
        tFloatingObjectLayout m_InitialLayout;
        //! Host cell snapshot for undo/redo of tCellClassAttribute
        tSaveSelect m_SaveSelect;
    public:
        /// @brief constructor of tUndoInsertFloatingObject.
        tUndoInsertFloatingObject();

        /// @brief constructor of tUndoInsertFloatingObject.
        /// @param[in] sName tString floating object name
        /// @param[in] sClassName tString cell class on host cell
        /// @param[in] sTargetSheetName tString display sheet
        /// @param[in] sHostRow tIndex row on _$$A (0 = next free row)
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet* rebase context (defaults to _$$A)
        tUndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow, tMode sMode, tSheet* sSheet);

        /// @brief constructor with optional initial layout (single undo for Apply float).
        tUndoInsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow,
                                  tBool sUseInitialLayout, tString sInitialAnchorRef,
                                  const tFloatingObjectLayout& sInitialLayout, tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoInsertFloatingObject.
        ~tUndoInsertFloatingObject() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Rebase context sheet (_$$A for insert/delete)
        /// @return     tSheet*
        tSheet* Sheet() override;

        /// @brief      Apply Do: registry entry + EnsureCellClass on host cell.
        /// @return     tBool false if not Ok
        tBool Do() override;

        /// @brief      Apply Undo: restore previous entry or delete.
        /// @return     tBool false if not Ok
        tBool Undo() override;

        /// @brief      Delete Before Do.
        void DeleteBeforeDo() override;

        /// @brief      Get SaveSelect for host cell class rebase
        /// @return     tSaveSelect*
        tSaveSelect* SaveSelect() override;

        /// @brief      Get RefRebase for host row on _$$A
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Rebase host row (m_HostRow) and SaveSelect on _$$A only.
        /// @return     tBool true if rebase successful, false if not
        tBool Rebase() override;

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue const rapidjson::Value&
        void Json(const rapidjson::Value& sValue) override;
};

//! Undo for delete floating object
class tUndoDeleteFloatingObject : public tUndoSpreadSheet {
    private:
        tWorkBook* m_WorkBook;
        //! Floating object name
        tSharedString m_Name;
        //! Cell class captured before delete
        tSharedString m_ClassName;
        //! Target sheet captured before delete
        tSharedString m_TargetSheetName;
        //! Host row captured before delete
        tIndex m_HostRow;
        //! Layout captured before delete
        tFloatingObjectLayout m_Layout;
        //! Host cell snapshot for undo restore
        tSaveSelect m_SaveSelect;
    public:
        /// @brief constructor of tUndoDeleteFloatingObject.
        tUndoDeleteFloatingObject();

        /// @brief constructor of tUndoDeleteFloatingObject.
        /// @param[in] sName tString floating object name
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet* rebase context (defaults to _$$A)
        tUndoDeleteFloatingObject(tString sName, tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoDeleteFloatingObject.
        ~tUndoDeleteFloatingObject() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Rebase context sheet (_$$A for insert/delete)
        /// @return     tSheet*
        tSheet* Sheet() override;

        /// @brief      Apply Do: remove registry entry (optional host class clear).
        /// @return     tBool false if not Ok
        tBool Do() override;

        /// @brief      Apply Undo: restore registry + host cell class.
        /// @return     tBool false if not Ok
        tBool Undo() override;

        /// @brief      Delete Before Do.
        void DeleteBeforeDo() override;

        /// @brief      Get SaveSelect for host cell class rebase
        /// @return     tSaveSelect*
        tSaveSelect* SaveSelect() override;

        /// @brief      Get RefRebase for host row on _$$A
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Rebase host row (m_HostRow) and SaveSelect on _$$A only.
        ///             Does not rebase layout anchor refs (see tUndoFloatingObjectLayout).
        /// @return     tBool true if rebase successful, false if not
        tBool Rebase() override;

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue const rapidjson::Value&
        void Json(const rapidjson::Value& sValue) override;
};

//! Undo for floating object layout (anchor + diff/size/opacity).
//! JSON wire: full ol/nl objects plus compact dx/dy/w/h/op (alias ud: tUndoFloatChangeSizePos).
class tUndoFloatingObjectLayout : public tUndoSpreadSheet {
    private:
        tWorkBook* m_WorkBook;
        //! Floating object name
        tSharedString m_Name;
        //! Target sheet (Sheet() rebase context for layout anchor ops)
        tSharedString m_TargetSheetName;
        //! Layout before change (m_AnchorRowRef/m_AnchorColRef rebased in Rebase())
        tFloatingObjectLayout m_OldLayout;
        //! Layout after change (m_AnchorRowRef/m_AnchorColRef rebased in Rebase())
        tFloatingObjectLayout m_NewLayout;
        //! True when anchor cell ref changed (not only diff/size/opacity)
        tBool m_ChangeAnchor;
    public:
        /// @brief constructor of tUndoFloatingObjectLayout.
        tUndoFloatingObjectLayout();

        /// @brief constructor of tUndoFloatingObjectLayout.
        /// @param[in] sName tString floating object name
        /// @param[in] sNewLayout tFloatingObjectLayout new geometry
        /// @param[in] sChangeAnchor tBool true if anchor cell changed
        /// @param[in] sMode tMode
        /// @param[in] sSheet tSheet* rebase context (defaults to target sheet)
        tUndoFloatingObjectLayout(tString sName, const tFloatingObjectLayout& sNewLayout, tBool sChangeAnchor, tMode sMode, tSheet* sSheet);

        /// @brief destructor of tUndoFloatingObjectLayout.
        ~tUndoFloatingObjectLayout() override;

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief      Rebase context sheet (target display sheet)
        /// @return     tSheet*
        tSheet* Sheet() override;

        /// @brief      Apply Do: update layout on registry entry only.
        /// @return     tBool false if not Ok
        tBool Do() override;

        /// @brief      Apply Undo: restore previous layout.
        /// @return     tBool false if not Ok
        tBool Undo() override;

        /// @brief      Delete Before Do.
        void DeleteBeforeDo() override;

        /// @brief      Layout undo does not capture host cell class
        /// @return     tSaveSelect* nullptr
        tSaveSelect* SaveSelect() override;

        /// @brief      Get RefRebase for layout anchor cell (m_AnchorRowRef/m_AnchorColRef)
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Rebase m_AnchorRowRef/m_AnchorColRef on the layout anchor sheet.
        ///             Only undo that adjusts tFloatingObjectLayout anchor allocator refs.
        /// @return     tBool true if rebase successful, false if not
        tBool Rebase() override;

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        /// @param[in]  sValue const rapidjson::Value&
        void Json(const rapidjson::Value& sValue) override;
};

 //=========================================================================
    //! Undo to change the open or close node
    class tUndoOpenCloseTree : public tUndoSpreadSheet {
    private:
        //! Row or Col
        tBool m_IsRow;
      
        //! Position
        tIndex m_Position;
        //! Sheet
        tSheet* m_Sheet;
        //! Absolute open state after Do (avoids toggle double-apply via collab echo).
        tBool m_TargetOpen;
        //! True once m_TargetOpen is known (local Do or Json).
        tBool m_HasTargetOpen;
    public:
       /// @brief constructor of tUndoOpenCloseTree.
        tUndoOpenCloseTree();

        /// @brief constructor of tUndoOpenCloseTree.
        /// @param[in] sIsRow tBool
        /// @param[in] sPosition  tIndex
        /// @param[in] sSheet tSheet
        tUndoOpenCloseTree(tBool sIsRow,tIndex sPosition,tMode sMode,tSheet* sSheet);
        

        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief Target sheet for Do/Undo/Rebase (required by SetRebasePlan).
        tSheet* Sheet() override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;

        /// @brief      Write JSON (collab PostMessage).
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        void Json(const rapidjson::Value& sValue) override;
    };

    //=========================================================================
    //! Undo to change tree
    class tUndoChangeTree : public tUndoSpreadSheet {
    private:
        //! Row or Col
        tBool m_IsRow;
        //! Right, Left  or  Down,  Up
        tBool m_Right;
        //! Position
        tIndex m_Position;
        //! Size
        tIndex m_Size;
        //! Sheet tree
        tSheet* m_Sheet;
        //! Save ColRow 
        tSaveSelectColRow m_SaveSelectColRow;
    public:
       /// @brief constructor of tUndoChangeTree.
        tUndoChangeTree();

        /// @brief constructor of tUndoChangeTree.
        /// @param[in] sIsRow tBool
        /// @param[in] sRight tBool
        /// @param[in] sPosition  tIndex
        /// @param[in] sSize  tIndex
        /// @param[in] sSheet tSheet
        tUndoChangeTree(tBool sIsRow,tBool sRight,tIndex sPosition,tIndex sSize,tMode sMode, tSheet* sSheet);
    
        /// @brief      Return ClassName
        /// @return     tString
        tString ClassName() const override;

        /// @brief Target sheet for Do/Undo/Rebase.
        tSheet* Sheet() override;

        // Rebase ===============================================================
        /// @brief      Get SaveSelect for rebase (returns nullptr if not available)
        /// @return     tSaveSelect* pointer to SaveSelect or nullptr
        tSaveSelect* SaveSelect() override;
        
        /// @brief      Get RefRebase
        /// @return     tString
        tString RefRebase() override;

        /// @brief      Apply Do.
        /// @return     tBool false if not Ok
        tBool Do()  override;

        /// @brief      Apply Undo.
        /// @return     tBool false if not Ok
        tBool Undo()  override;
        
        /// @brief      Delete Before Do.
        /// @return     tBool true if delete successful, false if not
        void DeleteBeforeDo() override;

        ///@brief Rebase
        ///@return  tBool true if rebase successful, false if any cell/range was deleted, tString new reference if rebase successful
        tBool Rebase() override;

        /// @brief      Write JSON (collab PostMessage).
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief      Read JSON.
        void Json(const rapidjson::Value& sValue) override;
    };

    //=========================================================================
    //! Undo for freeze-pane / split view (sheet SplitV / SplitH / SplitClear).
    //! m_Cde: 1 = set vertical splitter (frozen columns), 2 = horizontal, 3 = clear both.
    class tUndoSplitView : public tUndoSpreadSheet {
    private:
        //! 1 Vertical, 2 Horizontal, 3 Clear
        tByte m_Cde {};
        //! Column (cde 1) or row (cde 2) anchor; unused for Clear
        tIndex m_Position {0};
        //! Target sheet (Do / Undo execute here)
        tSheet* m_Sheet {nullptr};
        //! Sheet label on the wire (same field semantics as tUndoSpreadSheetCallBack::m_SheetName).
        tSharedString m_SheetName;
        //! Saved SplitV/SplitH before Do (Redo clears capture via DeleteBeforeDo, then Do re-snapshots)
        tIndex m_PrevSplitV {-1};
        tIndex m_PrevSplitH {-1};
        tBool m_PrevCaptured {false};

        static void RestoreSplitPan(tSheet* p, tIndex sColSplit, tIndex sRowSplit);

    protected:
        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;

    public:
        tUndoSplitView();

        /// @brief constructor
        /// @param[in] sCde 1 vertical (SplitV), 2 horizontal (SplitH), 3 clear
        /// @param[in] sPosition freeze line index for cde 1 or 2
        /// @param[in] sMode undo mode flags
        /// @param[in] sSheet sheet (nullptr = normalized active sheet when Do runs via API)
        tUndoSplitView(tByte sCde, tIndex sPosition, tMode sMode, tSheet* sSheet);

        tString ClassName() const override;

        tSheet* Sheet() override;

        tSaveSelect* SaveSelect() override;

        tString RefRebase() override;

        tBool Do() override;

        tBool Undo() override;

        void DeleteBeforeDo() override;

        tBool Rebase() override;
    };
    
    //! Undo replacing print layout (`tPrintParameters` merged snapshot: workbook fields + sheet orientation / FitToPage).
    class tUndoPrintParameters : public tUndoSpreadSheet {
    private:
        tSheet* m_Sheet {nullptr};
        //! Sheet label on the wire (same field semantics as tUndoSpreadSheetCallBack::m_SheetName).
        tSharedString m_SheetName;
        //! Parameters applied on Do().
        tPrintParameters m_PrintParameters;
        //! Snapshot taken at Do() entry (restored by Undo).
        tPrintParameters m_SnapshotBefore;
        tBool m_SnapshotCaptured {false};

    protected:
        void Json(Writer<StringBuffer>* sWriter) override;
        void Json(const rapidjson::Value& sValue) override;

    public:
        tUndoPrintParameters();
        tUndoPrintParameters(tString sJsonPrintParameters, tMode sMode, tSheet* sSheet);

        tString ClassName() const override;
        tSheet* Sheet() override;
        tSaveSelect* SaveSelect() override;
        tString RefRebase() override;

        tBool Do() override;
        tBool Undo() override;

        void DeleteBeforeDo() override;
        tBool Rebase() override;
    };

    //! Reject paste/move when collaboration payload exceeds @ref kCollaborationPasteCopyMaxBytes.
    tBool RejectCollaborationPasteIfTooLarge(tUndoSpreadSheet* sUndo, tBool sCollaborationContext);

    //! After tUndoCut, paste may merge into one tUndoMove (Excel-like). Returns nullptr for plain paste.
    tUndoSpreadSheet* CreateUndoPasteOrMoveAfterCut(tString sDestRef, tMode sMode, tSheet* sSheet,
                                                    tUndoRedoContainer* sContainer);

}

#endif
