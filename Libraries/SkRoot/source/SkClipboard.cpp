//=============================================================================
// SkClipboard  tClipboard
//==============================================================================
#include "../include/SkClipboard.hpp"

namespace SkRoot {
		
	tClipboard::tClipboard() : tClass(),m_Text() {}

	void tClipboard::Text(tString sText) { m_Text = sText; }
	tString tClipboard::Text() { return(m_Text); };
}
