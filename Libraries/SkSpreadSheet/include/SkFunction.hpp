//=============================================================================
// SkSpreadSheet Function 
//=============================================================================
#ifndef SkFunction_hpp
#define SkFunction_hpp
#include <SkApplication.hpp>
#include "SkColRow.hpp"
#include "SkColRowCellRange.hpp"
using namespace SkRoot;

namespace SkSpreadSheet {

	//=========================================================================
	//! How a function combines with range arguments (Excel dynamic-array semantics).
	//! Used by the evaluator to choose aggregation vs per-cell spill when ranges are passed.
	//=========================================================================
	enum class tFunctionSpillKind : tByte {
		//! Single scalar result; ranges are aggregated or reduced (SUM, MIN, COUNT, ...).
		Aggregate = 0,
		//! Scalar op applied to each cell; range inputs produce a spilled matrix (SIN, ABS, UPPER, ...).
		ElementWise = 1,
	};

	//=========================================================================
	//! Ancestor of all function (virtual)
	class tFunction : public tClass {
		protected:
        /// @brief		Pop N arguments from stack into a temporary vector (for processing).
        /// @param[in]  sStackElems tStackElems* stack containing arguments
        /// @param[in]  sNbArg tShort number of arguments to extract
        /// @return		std::vector<tStackElem> vector containing copies of arguments (last argument first, as popped from stack)
        static std::vector<tStackElem> PopArgs(tStackElems* sStackElems, tShort sNbArg);

        /// @brief		Resolve stack elem (Variant, Cell, Range, or Array) to a single value for use as function argument.
        /// @param[in]  sArg tStackElem from PopArgs (Variant, Cell, Range, or Array)
        /// @param[out] sOut resolved value (from Variant, Cell/Range CalculableValue(), or Array top-left)
        /// @return		true if resolved, false otherwise
        static tBool StackElemToVariant(const tStackElem& sArg, tVariant& sOut);

        /// @brief		Resolve stack elem to 1-based row/column index (INDEX, etc.).
        /// @param[in]  sArg tStackElem (Variant or Cell)
        /// @param[out] sOut index value
        /// @return		true if resolved to Int or Double
        static tBool StackElemToIndex(const tStackElem& sArg, tIndex& sOut);

        /// @brief		Resolve stack elem to signed integer (ADDRESS row/col, etc.).
        /// @param[in]  sArg tStackElem (Variant or Cell)
        /// @param[out] sOut integer value
        /// @return		true if resolved to Int or Double
        static tBool StackElemToInt(const tStackElem& sArg, tInt& sOut);

        /// @brief		Resolve stack elem to boolean (0/false = false, else true).
        /// @param[in]  sArg tStackElem (Variant or Cell)
        /// @param[out] sOut boolean value
        /// @return		true if resolved
        static tBool StackElemToBool(const tStackElem& sArg, tBool& sOut);

    public:
        /// @brief		Materialize a stack elem into an in-memory array.
        ///             t_Array is copied; t_Range is read cell-by-cell; scalar becomes 1x1.
        /// @param[in]  sArg tStackElem
        /// @param[out] sOut tArrayValue
        /// @return		true if materialized
        static tBool StackElemToArray(const tStackElem& sArg, tArrayValue& sOut);

		/// @brief		Constructor SkFunction.
		tFunction();
		
		/// @brief		Virtual destructor.
		virtual ~tFunction();

		/// @brief		Call function with arguments.
		/// @param[in]  sStackElems tStackElems* stack containing arguments
		/// @param[in]  sNbArg tShort number of arguments to extract
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
        
        /// @brief Store the calling cell/range (ROW/CELL without optional ref, INDIRECT, …).
        virtual void PassByRef(tItem* sItem);

        /// @brief If true, Lemon PushRef(..., sDependent=false): VectorRef kept for eval,
        ///        but AddDependent is skipped (no calc graph edge). See tCell::m_VectorRefAddDependent.
        virtual tBool ByRef();

		/// @brief Range combination style for future spill / aggregation (see tFunctionSpillKind).
		/// @return Default ElementWise (Excel 365-style unary math/text on ranges).
		virtual tFunctionSpillKind SpillKind() const;

#ifdef _DEBUGSK
        /// @brief		Return debug string (stack taken by value so it is not modified for Call()).
        /// @return		tString
        virtual tString Debug(tString sFunctionName, tStackElems sStackElemsCopy, tShort sNbArg);
#endif	
        
    
	};

	//=========================================================================
	//! Used for callback of range 
	class tCallBackRangeFunction : public tSparseArrayCallBack<tAllocatorRef> {
	protected:
		//! Result value. 
		tVariant		  m_Value;
		//! owner table cell range
		tColRowCellRange* m_ColRowCellRange;
	public:
		/// @brief		Constructor SkCallBackRangeFunction with owner tColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackRangeFunction(tColRowCellRange* sColRowCellRange);

		/// @brief		Return m_Value.
		/// @return		SkVariant
		tVariant Value();

		/// @brief		Set m_Value.
		/// @param[in]	sValuet tVariant
		void Value(tVariant sValue);

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tInt Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef); 
	};

	//=========================================================================
	//! Reference of function
	class tFunctionRef : public tClass {
	private:
        //! Name
        tSharedString         m_Name;
        //! Label of function
        tSharedString         m_Label;
        //! Family of function
        tSharedString         m_Family;
        
		//! Number of argument -1 for undefined like SUM().
		tInt			m_NbArg;
        
    protected:
        tVolatile   m_Volatile;
    public:
		//! Pointer of model function
		tFunction*		m_Function;
	public:
		/// @brief		Constructor SkFunctionRef.
		tFunctionRef();
		
		/// @brief		Constructor of copy.
		/// @param[in]  sFunctionRef SkFunctionRef&
		tFunctionRef(const tFunctionRef& sFunctionRef);

		/// @brief		Constructor with nb of argument of ponter of model function.
		//// @param[in]  sName tString name of function
        /// @param[in]  sLabel  tString label of function
        /// @param[in]  sFamily  tString family  of function
        /// @param[in]  sNbArg tInt number of argument
        /// @param[in]  sFunction SkFunction* pointer of model function
        /// @param[in]  sVolatile tVolatile (All , col  or row);
        tFunctionRef(tString sName,tString sLabel,tString sFamily,tInt sNbArg, tFunction* sFunction, tVolatile sVolatile=tVolatile::t_None);
        
        /// @brief      Set Name
        /// @param[in]  sName  tString
        void   Name(tString sName);
        
        /// @brief      Get  Name
        /// @return     tString
        tString Name();
        
        /// @brief      Set Label
        /// @param[in]  sName  tString
        void   Label(tString sLabel);
        
        /// @brief      Get Label
        /// @return     tString
        tString Label();

        /// @brief      Set family
        /// @param[in]  sFamily  tString
        void   Family(tString sFamily);
        
        /// @brief      Get Family
        /// @return     tString
        tString Family();
        
		/// @brief		Return nb argument.
		/// @return		tInt
		tInt NbArg();
        
        
        /// @brief        Return volatile.
        /// @return        tVolatile
        tVolatile Volatile() { return(m_Volatile); }

		/// @brief		Return model of function.
		/// @return		SkFunction* 
		tFunction* Function();
	};
	//! Map of SkFunctionRef
	typedef unordered_map<tString, tFunctionRef> SkMapFunctionRef;

	//=========================================================================
	//! Dictionary of function 
	class tFunctionDictionary : public tClass {
	private:
		//! map of function reference
		SkMapFunctionRef	m_MapFunctionRef;
		//! Empty function reference
		tFunctionRef		m_EmptyFunctionRef;
	public:
        /// @brief       constcutor
        tFunctionDictionary();
        
		/// @brief		Destructor for clear m_MapFunctionRef.
		~tFunctionDictionary();

		// Function ===============================================================
		/// @brief		Add function reference return false if function exist.
		/// @param[in]  sName tString name of function
        /// @param[in]  sLabel  tString label of function
        /// @param[in]  sFamily  tString family  of function
		/// @param[in]  sNbArg tInt number of argument
		/// @param[in]  sFunction tFunction* pointer of model function
        /// @param[in]  sVolatile tVolatile (Recalculate All, Row or Cole
		/// @return		tBool
		tBool AddFunctionRef(tString sName,tString sLabel,tString sFamily, tInt sNbArg, tFunction* sFunction, tVolatile sVolatile=tVolatile::t_None);

		/// @brief		Delete function reference return false if function don't exist.
		/// @param[in]  sName tString name of function
		/// @return		tBool 
		tBool DeleteFunctionRef(tString sName);

		/// @brief		Return SkFunctionRef.
		/// @param[in]  sName tString name of function
		/// @return		SkFunctionRef& 
		tFunctionRef& FunctionRef(tString sName);

		/// @brief		Return number of argument.
		/// @param[in]  sName tString name of function
		/// @return		tInt 
		tInt NbArg(tString sName);

		/// @brief		Return true if function exist.
		/// @param[in]  sName tString name of function
		tBool Exist(tString sName);

	};


}; // end of namespace

#endif
