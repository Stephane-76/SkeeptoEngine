//=============================================================================
// SkFormatShare (for management of instance)
//=============================================================================
#ifndef SkFormatShare_hpp
#define SkFormatShare_hpp

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <SkTypes.hpp>
#include <SkClass.hpp>
#include <SkSharedString.hpp>
#include <SkFormatApi.hpp>

using namespace SkRoot;

namespace SkFormat {

	// SkFormatShare  =========================================================
    // Ancestor for all elem Format (Counter of instance).
	class  alignas(SkAlign) tFormatShare : public tClass {
	protected:
		//! for count instance
		tInt	m_Count;
#ifdef checkfo
		//! for chech instance
		tInt m_CountCheck; 
#endif
	public:
		/// @brief		Constructor
		tFormatShare();
		
		/// @brief		Copy constructor 
		/// @param[in]	sFormatShare SkFormatShare&
		tFormatShare(const tFormatShare& sFormatShare);

		/// @brief      Clear
		void Clear();

		/// @brief      Get key of element
		/// @return     tString
		virtual tString Key();

		/// @brief      Method increments identical elem
		void Inc();

		/// @brief      Method decrements identical elem
		void Dec();

		/// @brief      get is shared string is no used
		/// @return     tBool return true if m_Count==0
		tBool IsNotUse();

		/// @brief      Return number of instance
		/// @return     tInt 
		tInt Count();

#ifdef checkfo
		/// @brief      Method reset check 
		void ResetCheck();

		/// @brief      Method increments check 
		void IncCheck();

		/// @brief      Method decrements check
		void DecCheck();

		/// @brief      check
		void Check();
#endif
	};

}; // end of namespace

#endif
