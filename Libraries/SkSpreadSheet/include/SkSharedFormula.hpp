//=============================================================================
// SkRoot SharedFormula
// Manage shared string  (Mutualized Formula)
// Author : Stephane Allez 
//=============================================================================
#ifndef tSharedFormula_hpp
#define tSharedFormula_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkFormula.hpp"

namespace SkSpreadSheet {

	//==========================================================================
	//! For unique formula
	class tSharedFormulaItem : public tClass {
	protected:
		//! Number of instances of formula.
		tInt        m_Count;  
	protected:
		//! Allocator on element m_MapFormulaItem for delete in allocator. 
		tAllocatorRef	m_AllocatorRef;

		//! Formula
		tFormula	m_Formula;
	public:
		/// @brief      constructor.
		tSharedFormulaItem();

		/// @brief      constructor of copy.
		/// @param[in]	sFormula SkFormula&
		tSharedFormulaItem(const tFormula& sFormula);
		
		/// @brief      destructor.
		~tSharedFormulaItem();

		/// @brief      Clear.
		void Clear();

		/// @brief      Set Index on SkAllocatorFormulaItem.
		/// @param[in]	sAllocatorRef tAllocatorRef
		void AllocatorRef(tAllocatorRef sAllocatorRef);

		/// @brief      return Index on SkAllocatorFormulaItem
		/// @return		tAllocatorRef 
		tAllocatorRef AllocatorRef();

		/// @brief      Set formula.
		/// @@param[in]	sFormula SkFormula
		void Formula(const tFormula sFormula);


		/// @brief      Return formula (non-const version).
		/// @return		tFormula*
		tFormula* Formula();
		
		/// @brief      Return formula (const version).
		/// @return		const tFormula*
		const tFormula* Formula() const;


		/// @brief      Increment m_Count.
		void Inc();

		/// @brief      Decrement m_Count.
		void Dec();
		
		/// @brief      m_Count==0.
		/// @return		tBool
		tBool IsEmpty();

		/// @brief      Return formula string.
		/// @return		tString
		tString operator()();
	};

	//==========================================================================
	//! Container of all SharedFormulaItems 
	class tSharedFormulaPool : public tClass {
	private:

		typedef tAllocator<tSharedFormulaItem, tAllocatorRef, 512> tAllocatorFormulaItem;
		typedef std::unordered_map<tString, tAllocatorRef> tMapFormulaItem;
		//! Map of formula pool
		tMapFormulaItem		   m_MapFormulaItem;
		tAllocatorFormulaItem  m_AllocatorFormulaItem;
	public:
		/// @brief      constructor.
		tSharedFormulaPool();
		
		/// @brief      Destructor.
		~tSharedFormulaPool();

		/// @brief      Clear all allocators before destruction.
		/// This should be called before deleting KeyValue to prevent use-after-free.
		void Clear();


		/// @brief      Return tSharedFormulaItem by formula.
		/// @param[in]	sFormula SkFormula&
		/// @return		tSharedFormulaItem*
		tSharedFormulaItem* Get(const tFormula& sFormula);

		/// @brief      Return tSharedFormulaItem by Index.
		/// @param[in]	sIndex tIndex
		/// @return		tSharedFormulaItem*
		tSharedFormulaItem* Get(tIndex sIndex);

		/// @brief      Add tSharedFormulaItem by formula.
		/// @param[in]	sFormula SkFormula&
		/// @return		tSharedFormulaItem*
		tSharedFormulaItem* Add(const tFormula& sFormula);

		/// @brief      Rmove  tSharedFormulaItem.
		/// @param[in]	stSharedFormulaItem tSharedFormulaItem*
		void Remove(tSharedFormulaItem* stSharedFormulaItem);

		/// @brief      Return number of tSharedFormulaItem.
		/// @return		tSize
		tSize Size();


		/// @brief      Return tSharedFormulaPool instance one by application.
		/// @return		tSharedFormulaPool*
		//static tSharedFormulaPool* Instance();
	};

	//==========================================================================
	//! The formulas with the same content but different references are shared in a single SkFormulaItem
	class tSharedFormula : tClass {
	private:
		//! Container of formula pool
		tSharedFormulaItem* m_SharedFormulaItem;

	public:
		/// @brief      Constructor.
		tSharedFormula();

		/// @brief      Constructor of copy.
		/// @param[in]	stSharedFormula tSharedFormula&
		tSharedFormula(const tSharedFormula& stSharedFormula);

		/// @brief      Constructor with formula.
		/// @param[in]	sFormula SkFormula&
		tSharedFormula(const tFormula& sFormula);

		/// @brief		Clear.
		void Clear();

		/// @brief      Return formula (non-const version).
		/// @return		tFormula*
		tFormula* Formula();
		
		/// @brief      Return formula (const version).
		/// @return		const tFormula*
		const tFormula* Formula() const;


		/// @brief      Operator = with another tSharedFormula.
		/// @param[in]	sString const tSharedFormula&
		/// @return		tSharedFormula&
		tSharedFormula& operator =(const tSharedFormula& sSharedFormula);

		/// @brief      Operator == with another tSharedFormula.
		/// @param[in]	sSharedFormula const tSharedFormula&
		/// @return     tBool
		tBool operator ==(const tSharedFormula& sSharedFormula) const;

		/// @brief      operator () return formula string (key).
		/// @return		tString
		tString operator()();
	};

}; // end of namespace ========================================================

#endif
