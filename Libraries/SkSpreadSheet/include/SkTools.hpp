//=============================================================================
// SkSpreadSheet Tools
//=============================================================================
#ifndef SkTools_hpp
#define SkTools_hpp
#include <SkTypes.hpp>
#include <SkClass.hpp>
#include <SkVariant.hpp>

#include <cmath>
#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace SkRoot;
using namespace rapidjson;

// Compilation Mode
#ifdef _DEBUGSK
    // checksp: debug-only structural validation (WASM debug / native _DEBUGSK). Never repairs data.
    //
    // Two formula dependency structures must stay symmetric for checksp to pass:
    //   - VectorRef on the formula cell: "what this formula reads" (source of truth for calculation)
    //   - ContainerCellDepend on each source item: "which formula cells depend on me" (recalc propagation)
    //
    // Check() methods (tCell, tRange, tColRowCellRange, …) only read and throw; they must NOT rebuild
    // links. Ephemeral counters (e.g. m_NbCellsCheck) are reset during validation only.
    //
    // Dependency repair belongs on operation paths, not inside Check():
    //   - UndoCell: RecoverFormulaCell → AddDependant → PruneStaleInverseDependents
    //   - UndoCells / UndoDeleteRowByRect: RewireOutgoingFormulaDependents (saved cells only)
    //   - CF JSON load: AddRangeInMapRange (register ranges before a later Check)
    //
    // PruneStaleInverseDependents removes inverse links only when the dependent formula's VectorRef
    // no longer references this cell (direct item or range covering it). It does not touch VectorRef.
#ifndef checksp
    #define checksp
#endif
#endif


namespace SkSpreadSheet {
	
    class tUndoRedoJsonCallBack;
    const tIndex Cst_MaxRow = 1048576;
    const tIndex Cst_MaxCol = 16384;

	/// @brief		For column, convert number to alpha column.
	/// @param[in]	sValue tIndex
	/// @return		tString 
	tString Base10ToAlpha(tIndex sValue);

	/// @brief		For column, convert alpha column to number.
	/// @param[in]	sValue tString
	/// @return		tIndex
	tIndex AlphaToBase10(tString sValue);

	/// @brief		Return sheet name for use in formulas; add single quotes when necessary (Excel-style).
	///				Unquoted names may contain only letters, digits, and underscore (e.g. My_Sheet).
	///				Names with spaces or special characters are wrapped in single quotes (e.g. 'My Sheet').
	///				Any single quote inside the name is escaped by doubling it.
	/// @param[in]	sSheetName raw sheet name (e.g. "My Sheet" or "My_Sheet")
	/// @return		tString name as used in formula: "'My Sheet'" or "My_Sheet"
	tString SheetNameForFormula(tString sSheetName);

    tBool ParseRange(tString sRef, tIndex& sTop, tIndex& sLeft, tIndex& sBottom, tIndex& sRight);

    tBool ParseCell(tString sRef, tIndex& sRow, tIndex& sCol);

    class tTempoPoint;
	//=========================================================================
	//! Point (Cell coordinate)
	class tPoint {
	protected:
		//! Row
		tIndex m_Row;
		//! Col
		tIndex m_Col;
	public:
		/// @brief		Constructor SkPoint.
		tPoint();

		/// @brief		Constructor SkPoint with row & col.
		/// @param[in] sRow
		/// @param[in] sCol
		tPoint(tIndex sRow, tIndex sCol);

		/// @brief		Constructor of copy.
		/// @param[in] sPoint
		tPoint(const tPoint& sPoint);
        
        /// @brief        Constructor of copy.
        /// @param[in] sTempoPoint tTempoPoint
        tPoint(tTempoPoint& sTempoPoint);

		/// @brief		Destructor SkPoint.
		virtual ~tPoint();

		/// @brief		Set row.
		/// @param[in] sRow tIndex
		void Row(tIndex sRow);

		/// @brief		Return row.
		/// @return		tIndex
		tIndex Row() const;

		/// @brief		Set col.
		/// @param[in] sCol tIndex
		void Col(tIndex sCol);

		/// @brief		Return col.
		/// @return		tIndex
		tIndex Col() const;
        
		/// @brief		Return Ref.
		/// @return		tString
		tString StrRef();

		/// @brief		operator <.
		/// @return		tBool
		tBool operator < (const tPoint& sPoint) const;

		/// @brief		operator ==.
		/// @return		tBool
		tBool operator == (const tPoint& sPoint) const;
	};
	//! Vector of tPoint
	typedef std::vector<tPoint> tVectorPoint;

	//=========================================================================
	//! Point (Cell coordinate)  (Use Temporary memory)
	class tTempoPoint {
	protected:
		//! Row
		tIndex m_Row;
		//! Col
		tIndex m_Col;
        //! Maned Range or CellClass
        tString m_Name;
	public:
		/// @brief		Constructor SkPoint.
		tTempoPoint();

		/// @brief		Constructor SkPoint with row & col.
		/// @param[in] sRow tIndex
		/// @param[in] sCol tIndex
		tTempoPoint(tIndex sRow, tIndex sCol);

        /// @brief        Constructor SkPoint with sRef like "A1".
        /// @param[in] sRef tString
        tTempoPoint(tString sRef);
        
		/// @brief		Constructor of copy.
		/// @param[in] sPoint
		tTempoPoint(const tTempoPoint& sPoint);

		/// @brief		Constructor of copy.
		/// @param[in] sPoint tPoint&
		tTempoPoint(tPoint& sPoint);

		/// @brief		Set row.
		/// @param[in] sRow tIndex
		void Row(tIndex sRow);

		/// @brief		Return row.
		/// @return		tIndex
		tIndex Row();

		/// @brief		Set col.
		/// @param[in] sCol tIndex
		void Col(tIndex sCol);

		/// @brief		Return col.
		/// @return		tIndex
		tIndex Col();

        /// @brief        SetName.
        /// @param[in] sName tString
        void Name(tString sName);

        /// @brief        Return Name.
        /// @return       tString
        tString Name();
        
		/// @brief		Destructor SkPoint.
		virtual ~tTempoPoint();

		/// @brief		Parse Ref.
		/// @param[in] sRef tString
		/// @return       tBool
		tBool ParseRef(tString sRef);

		/// @brief		Return Ref.
		/// @return		tString
		tString StrRef();
        
        /// @brief      Translate (Paste)
        /// @param[in]  sTempoPoint tTempoPoint*
        virtual void Translate(tTempoPoint* sTempoPoint);
        
        /// @brief        Call back for Do & Undo.
        /// @param[in]    sUndoSpreadSheet
        /// @return        tBool
        tBool CallBack(tUndoRedoJsonCallBack* sUndoRedoJson);

        
        // Json ===============================================================
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void Json(const Value& sValue);

		/// @brief      new for use circular memory
		/// @return     sz tIndex size to alloc
		void* operator new(tSize sz) noexcept;

		/// @brief      delete for use circular memory
		/// @return     sz tIndex size to alloc
		void operator delete(void* ptr) noexcept;

		/// @brief		operator <.
		/// @return		tBool
		tBool operator < (const tTempoPoint sPoint);

		/// @brief		operator ==.
		/// @return		tBool
		tBool operator == (const tTempoPoint sPoint);
	};
	//! Vector of SkPoint
	typedef std::vector<tTempoPoint*> tVectorTempoPoint;

	class tTempoRect;

	//=========================================================================
	//! Range Rect 
	class tRect : public tPoint {
	private:
		//! Bottom row
		tIndex m_Bottom;
		//! Right col
		tIndex m_Right;
	public:
		/// @brief		Constructor SkRect.
		tRect();

		/// @brief		Constructor SkRect with top and bottom row & col.
		/// @param[in] sTop
		/// @param[in] sLeft
		/// @param[in] sBottom
		/// @param[in] sRight
		tRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

		/// @brief		Constructor of copy.
		/// @param[in] sRect
		tRect(const tRect& sRect);
  
        /// @brief		Constructor of copy.
		/// @param[in] sRect tTemporect
        tRect(tTempoRect& sRect);
        
        /// @brief        Constructor with parse
        /// @param[in] sRect
        tRect(const tString sRect);
        
		/// @brief		Destructor SkRect.
		virtual ~tRect();
        
        /// @brief        Constructor SkRect with top and bottom row & col.
        /// @param[in] sTop
        /// @param[in] sLeft
        /// @param[in] sBottom
        /// @param[in] sRight
        void Set(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);
        
		/// @brief		Return Top  : SkPoint ancestor m_row.
		/// @return		tIndex
		tIndex Top();

		/// @brief		Set Top.
		/// @param[in] sValue tIndex
		void Top(tIndex sValue);

		/// @brief		Return Left : SkPoint ancestor m_Col.
		/// @return		tIndex
		tIndex Left();

		/// @brief		Set Left.
		/// @param[in] sValue tIndex
		void Left(tIndex sValue);

		/// @brief		Return bottom row.
		/// @return		tIndex
		tIndex Bottom();

		/// @brief		Set Bottomt.
		/// @param[in] sValue tIndex
		void Bottom(tIndex sValue);

		/// @brief		Return right col.
		/// @return		tIndex
		tIndex Right();

		/// @brief		Set Right.
		/// @param[in] sValue tIndex
		void Right(tIndex sValue);
        
        /// @brief        Return Width.
        /// @return       tInt
        tInt Width();
        
        /// @brief        Return Height.
        /// @return       tInt
        tInt Height();

        /// @brief        True if corners are non-negative and Top<=Bottom, Left<=Right.
        /// @return       tBool
        tBool IsValid() const;

		/// @brief		Return Ref.
		/// @return		tString
		tString StrRef();

        /// #brief Is Col Select
        /// @return tBool
        tBool IsColSelect();
        
        /// #brief Is Col Select
        /// @return tBool
        tBool IsRowSelect();

        /// #brief Is Col Select
        /// @return tBool
        tBool IsSheetSelect();
        
        /// @brief 	Set Row Select.
		/// @param[in] sIndex tIndex
		void RowSelect(tIndex sIndex);

        /// @brief 	Set Col Select.
		/// @param[in] sIndex tIndex
		void ColSelect(tIndex sIndex);

        /// @brief 	Set Sheet Select.
		void SheetSelect();

		/// @brief		Move row with sSize .
		/// @param[in] sSize tIndex
		void MoveRow(tIndex sSize);

		/// @brief		Move col with sSize .
		/// @param[in] sSize tIndex
		void MoveCol(tIndex sSize);

		/// @brief		operator < use for sort.
		/// @return		tBool
		tBool operator < (const tRect sRect);

		/// @brief		operator == use for find recover.
		/// @return		tBool
		tBool operator == (const tRect sRect);

		tRect operator = (const tRect sRect);
	};

    typedef vector<tRect> tVectorRect;
    typedef stack<tRect> tStackRect;
	//=========================================================================
	//! Range Rect (Use Temporary memory)
	class tTempoRect : public tTempoPoint {
	private:
		//! Bottom row
		tIndex m_Bottom;
		//! Right col
		tIndex m_Right;
	public:
		/// @brief		Constructor SkRect.
		tTempoRect();

		/// @brief		Constructor SkRect with top and bottom row & col.
		/// @param[in] sTop
		/// @param[in] sLeft
		/// @param[in] sBottom
		/// @param[in] sRight
		tTempoRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);

		/// @brief		Constructor of copy.
		/// @param[in] sRect
		tTempoRect(const tTempoRect& sRect);

        /// @brief        Constructor with tRect.
        /// @param[in] sRect tRect
        tTempoRect(tRect& sRect);

        /// @brief        Constructor with tString.
        /// @param[in] sRef tString
        tTempoRect(tString sRef);

		/// @brief		Destructor SkRect.
		virtual ~tTempoRect();

		/// @brief		Return Top  : SkPoint ancestor m_row.
		/// @return		tIndex
		tIndex Top();

		/// @brief		Set Top.
		/// @param[in] sValue tIndex
		void Top(tIndex sValue);

		/// @brief		Return Left : SkPoint ancestor m_Col.
		/// @return		tIndex
		tIndex Left();

		/// @brief		Set Left.
		/// @param[in] sValue tIndex
		void Left(tIndex sValue);

		/// @brief		Return bottom row.
		/// @return		tIndex
		tIndex Bottom();

		/// @brief		Set Bottomt.
		/// @param[in] sValue tIndex
		void Bottom(tIndex sValue);

		/// @brief		Return right col.
		/// @return		tIndex
		tIndex Right();

		/// @brief		Set Right.
		/// @param[in] sValue tIndex
		void Right(tIndex sValue);
        
        /// @brief        Return Width.
        /// @return       tInt
        tInt Width();
        
        /// @brief        Return Height.
        /// @return       tInt
        tInt Height();

        /// @brief        True if corners are non-negative and Top<=Bottom, Left<=Right.
        /// @return       tBool
        tBool IsValid() const;

		/// @brief        Parse Ref.
		/// @param[in]    sRef tString
		/// @return       tBool
		tBool ParseRef(tString sRef);

		/// @brief		Return Ref.
		/// @return		tString
		tString StrRef();
        
        /// @brief      Translate (Paste)
        /// @param[in]  sTempoPoint tTempoPoint*
        void Translate(tTempoPoint* sTempoPoint) override;

         /// @brief     Return true if tPoint (m_Row==m_Bottom & m_Col==m_Right
        /// @Return tBool
        tBool IsPoint();
    
        /// #brief Is Col Select
        /// @return tBool
        tBool IsColSelect();
        
        /// #brief Is Col Select
        /// @return tBool
        tBool IsRowSelect();

        /// #brief Is Col Select
        /// @return tBool
        tBool IsSheetSelect();
        
		/// @brief		Move row with sSize .
		/// @param[in] sSize tIndex
		void MoveRow(tIndex sSize);

		/// @brief		Move col with sSize .
		/// @param[in] sSize tIndex
		void MoveCol(tIndex sSize);
        
        /// @brief        Call back for Do & Undo.
        /// @param[in]    sUndoSpreadSheet
        /// @return        tBool
        tBool CallBack(tUndoRedoJsonCallBack* sUndoRedoJson);
        
		// Json ===============================================================
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void Json(const Value& sValue);

		/// @brief      new for use circular memory
		/// @param{in] sz tSize
		void* operator new(tSize sz) noexcept;

		/// @brief      delete for use circular memory (We do nothing)
		/// @param{in] sz tSize
		void operator delete(void* ptr) noexcept;
	
		/// @brief		operator < use for sort.
		/// @return		tBool
		tBool operator < (const tTempoRect sRect);

		/// @brief		operator == use for find recover.
		/// @return		tBool
		tBool operator == (const tTempoRect sRect);

		tTempoRect operator = (const tTempoRect sRect);
	};

} // end of namespace

#endif
