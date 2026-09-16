//=============================================================================
// SkFormatShare (for management of instance)
//=============================================================================
#include "../include/SkFormatShare.hpp"

namespace SkFormat {
	// SkFormat ==============================================================
	tFormatShare::tFormatShare() : tClass(), m_Count(0) 
#ifdef checkfo
		//! for chech instance
		,m_CountCheck(0)
#endif
	{}
	
	tFormatShare::tFormatShare(const tFormatShare& sFormatShare) : tClass(), m_Count(0)
#ifdef checkfo
		//! for chech instance
		, m_CountCheck(0)
#endif
	{};

	/// @brief      Clear
	void tFormatShare::Clear() {
		m_Count = 0;
#ifdef checkfo
		m_CountCheck = 0;
#endif
	}

	tString tFormatShare::Key() { return(""); }

	void tFormatShare::Inc() { m_Count++; }

	void tFormatShare::Dec() { m_Count--; assert(m_Count>=0); }

	tBool tFormatShare::IsNotUse() { return(m_Count == 0); }

	tInt tFormatShare::Count() { return(m_Count); }


#ifdef checkfo
	/// @brief      Method reset check 
	void tFormatShare::ResetCheck() { m_CountCheck = 0;  }

	/// @brief      Method increments check 
	void tFormatShare::IncCheck() { m_CountCheck++; }

	/// @brief      Method decrements check
	void tFormatShare::DecCheck() { m_CountCheck--; }

	/// @brief      check
	void tFormatShare::Check() {
        assert(m_Count >= 0);
		assert(m_Count == m_CountCheck);
	}
#endif

} // end of namspace
