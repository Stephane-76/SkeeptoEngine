//=============================================================================
// SkLemonReserved ReserverWord for lemon parser
//=============================================================================
#ifndef SkLemonReserved_hpp
#define SkLemonReserved_hpp
#include "SkLexerSpreadSheet.hpp"
#include "SkFunction.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

	//=========================================================================
	//! Reference for reserved Id 
	class tLemonIdRef : public tClass {
	private:
		tInt			m_Id;
		tVirtualClass* m_Class;
		tKind			m_Kind;
	public:
		/// @brief		Constructor.
		tLemonIdRef();

		/// @brief		Constructor of copy.
		/// @param[in]	sLemonIdRef const SkLemonIdRef&
		tLemonIdRef(const tLemonIdRef& sLemonIdRef);

		/// @brief		Constructor with id, virtual class kind.
		/// @param[in]	sId tInt Id
		/// @param[in]	sClass SkVirtualClass*
		/// @param[in]	sKind tKind
		tLemonIdRef(tInt sId, tVirtualClass* sClass,tKind sKind);

		/// @brief		Return Id.
		/// @return		tInt
		tInt Id();

		/// @brief		Set Id.
		/// @param[in]	sId tInt
		void Id(tInt sId);

		/// @brief		Return SkVirtualClass.
		/// @return		SkVirtualClass*
		tVirtualClass* Class();

		/// @brief		Set SkVirtualClass.
		/// @param[in]	sClass SkVirtualClass*
		void Class(tVirtualClass* sClass);

		/// @brief		Return Kind.
		/// @return		tKind
		tKind Kind();
	};
	//! Map fo SkLemonIdRef
	typedef unordered_map<tString, tLemonIdRef> SkMapIdRef;

	//=========================================================================
	//! Container for reference Id and function
	class tLemonReserved : public tClass {
	private:
		//! Map for reserver word
		SkMapIdRef			m_MapIdRef;
		//! Empty reserver word
		tLemonIdRef		m_EmptyLemonIdRef;
	public:
		/// @brief		Constructor.
		tLemonReserved();

		/// @brief		Destructor.
		~tLemonReserved();

		/// @brief		Clear m_MapIdRef and m_MapFunctionRef.
		void Clear();
		// Id =====================================================================
		
		/// @brief		Add Id.
		/// @param[in]  sName tString name of Id
		/// @param[in]  sId tInt Id
		/// @param[in]  sClass SkVirtualClass* 
		/// @param[in]  sKind tKind
		/// @return		tBool false if already exist
		tBool AddIdRef(tString sName, tInt sId, tVirtualClass* sClass, tKind sKind);

		/// @brief		Add Id.
		/// @param[in]  sName tString name of Id
		/// @return		tBool false if don't exist
		tBool DeleteIdRef(tString sName);

		/// @brief		Return SkLemonIdRef.
		/// @param[in]  sName tString name of Id
		/// @return		SkLemonIdRef&
		tLemonIdRef& IdRef(tString sName);

		/// @brief		Return Id.
		/// @param[in]  sName tString name of Id
		/// @return		tInt
		tInt Id(tString sName);

		/// @brief		Debug.
		void Debug();
	};
}
#endif
