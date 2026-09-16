//=============================================================================
// SkLemonReserved ReserverWord for lemon parser
//=============================================================================
#ifndef SkLemonFormatReserved_hpp
#define SkLemonFormatReserved_hpp
#include "SkLexerFormat.hpp"
#include "../include/SkFormatRoot.hpp"

using namespace SkRoot;

namespace SkFormat {

	//=========================================================================
	//! Reference for reserved Id 
	class tLemonFormatIdRef : public tClass {
	private:
		tInt			m_Id;
		tVirtualClass* m_Class;
		tKind			m_Kind;
	public:
		/// @brief		Constructor.
		tLemonFormatIdRef();

		/// @brief		Constructor of copy.
		/// @param[in]	sLemonIdRef const SkLemonIdRef&
		tLemonFormatIdRef(const tLemonFormatIdRef& sLemonIdRef);

		/// @brief		Constructor with id, virtual class kind.
		/// @param[in]	sId tInt Id
		/// @param[in]	sClass SkVirtualClass*
		/// @param[in]	sKind tKind
		tLemonFormatIdRef(tInt sId, tVirtualClass* sClass,tKind sKind);

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
	typedef unordered_map<tString, tLemonFormatIdRef> SkMapIdRef;
  


	//=========================================================================
	//! Container for reference Id and function
	class tLemonFormatReserved : public tClass {
	private:
		//! Map for reserver word
		SkMapIdRef			m_MapIdRef;
        
		//! Empty reserver word
		tLemonFormatIdRef		m_EmptyLemonIdRef;
	public:
		/// @brief		Constructor.
		tLemonFormatReserved();

		/// @brief		Destructor.
		~tLemonFormatReserved();

		/// @brief		Clear m_MapIdRef and m_MapFunctionRef.
		void Clear();

        /// @brief      Init r m_MapIdRef .
        void Init();
        
        // Id =====================================================================
		/// @brief		Add Id.
		/// @param[in]  sName const tString name of Id
		/// @param[in]  sId const tInt Id
		/// @param[in]  sClass SkVirtualClass*
		/// @param[in]  sKind const  tKind
		/// @return		tBool false if already exist
		tBool AddIdRef(const tString sName,const tInt sId, tVirtualClass* sClass,const tKind sKind);

		/// @brief		Add Id.
		/// @param[in]  sName tString name of Id
		/// @return		tBool false if don't exist
		tBool DeleteIdRef(tString sName);

		/// @brief		Return SkLemonIdRef.
		/// @param[in]  sName tString name of Id
		/// @return		SkLemonIdRef&
		tLemonFormatIdRef& IdRef(tString sName);

		/// @brief		Return Id.
		/// @param[in]  sName tString name of Id
		/// @return		tInt
		tInt Id(tString sName);
        
        /// @brief        Return  ttRecColor
        /// @param[in]  sName tString name of Id
        /// @return        tRecColor*
         const tRecColor* Color(tString sName);
        
		/// @brief		Debug.
		void Debug();
	};
}
#endif
