//=============================================================================
// SkUtf
/**
 * @page SkUtf
 * @par
 * @par Utf 8 16 Managment
 */
 //=============================================================================
#include "../include/SkUtf.hpp"


namespace SkRoot {
	/////////////////////////////////////////////////////////
	// Utf16ToUtf8 -   Convert a character from UTF-16 
	//                 encoding to UTF-8.
	//                 NB: Does not handle Surrogate pairs.
	//                     Does not test for badly formed 
	//                     UTF-16
	// Parameters:
	//   chUtf16 (in): Input char
	// Returns:        UTF-8 version as a string
	/////////////////////////////////////////////////////////
	tString Utf16ToUtf8(t16Char sUtf16) {
		// From RFC 3629
		// 0000 0000-0000 007F   0xxxxxxx
		// 0000 0080-0000 07FF   110xxxxx 10xxxxxx
		// 0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx

		// max output length is 3 bytes (plus one for Nul)
		unsigned char wUtf8[4] = "";

		if (sUtf16 < 0x80)
		{
			wUtf8[0] = static_cast<unsigned char>(sUtf16);
		}
		else if (sUtf16 < 0x7FF)
		{
			wUtf8[0] = static_cast<unsigned char>(0xC0 | ((sUtf16 >> 6) & 0x1F));
			wUtf8[1] = static_cast<unsigned char>(0x80 | (sUtf16 & 0x3F));
		}
		else
		{
			wUtf8[0] = static_cast<unsigned char>(0xE0 | ((sUtf16 >> 12) & 0xF));
			wUtf8[1] = static_cast<unsigned char>(0x80 | ((sUtf16 >> 6) & 0x3F));
			wUtf8[2] = static_cast<unsigned char>(0x80 | (sUtf16 & 0x3F));
		}

		return reinterpret_cast<char*>(wUtf8);
	}


	/////////////////////////////////////////////////////////
	// Utf16ToUtf8 -   Convert a string from UTF-16 encoding
	//                 to UTF-8
	// Parameters:
	//   sNative (in): Input String
	// Returns:        Converted string
	/////////////////////////////////////////////////////////
	tString Utf16ToUtf8(const t16String sUtf16)
	{
		tString wUtf8;
		t16String::const_iterator wIterator;
		for (wIterator = sUtf16.begin(); wIterator != sUtf16.end(); ++wIterator) wUtf8 += Utf16ToUtf8(*wIterator);
		return(wUtf8);
	}

	t16String Utf8ToUtf16(const tString sUtf8) {
		t16String wUtf16 = u"";
		wUtf16.reserve(sUtf8.size());

		for (size_t i = 0; i < sUtf8.size(); ++i) {
			unsigned char ch0 = sUtf8[i];
			if ((ch0 & 0x80) == 0x00) {
				wUtf16 += ((ch0 & 0x7f));
			}
			else {
				if ((ch0 & 0xe0) == 0xc0) {
					unsigned char ch1 = sUtf8[++i];
					wUtf16 += ((ch0 & 0x3f) << 6) | ((ch1 & 0x3f));
				}
				else {
					unsigned char ch1 = '\x0';
					if (i < sUtf8.size()) ch1 = sUtf8[++i];
					unsigned char ch2 = '\x0';
					if (i < sUtf8.size()) ch2 = sUtf8[++i];
					wUtf16 += ((ch0 & 0x0f) << 12) | ((ch1 & 0x3f) << 6) | ((ch2 & 0x3f));
				}
			}
		}
		return(wUtf16);
	}

	t16String Utf8ToUtf16(const tChar* sUtf8) {
		tString wUf8(sUtf8);
		return(Utf8ToUtf16(wUf8));
	}

	/////////////////////////////////////////////////////////
	// CountUtf8Characters - Count UTF-8 characters in a string
	//                       (not bytes, but actual characters)
	// Parameters:
	//   sText (in): UTF-8 encoded string
	// Returns:      Number of UTF-8 characters
	/////////////////////////////////////////////////////////
	tSize CountUtf8Characters(const tString& sText) {
		tSize wCount = 0;
		const tChar* wPtr = sText.c_str();
		const tChar* wEnd = wPtr + sText.length();
		
		while (wPtr < wEnd) {
			tChar wByte = *wPtr;
			// Check if this is a start byte (not a continuation byte)
			// Continuation bytes start with 10xxxxxx (0x80-0xBF)
			if ((wByte & 0xC0) != 0x80) {
				wCount++;
			}
			wPtr++;
		}
		
		return wCount;
	}

	/////////////////////////////////////////////////////////
	// CountUtf16Characters - Count UTF-16 characters in a string
	//                        (not code units, but actual characters)
	//                        Surrogate pairs count as 1 character
	// Parameters:
	//   sText (in): UTF-16 encoded string
	// Returns:      Number of UTF-16 characters
	/////////////////////////////////////////////////////////
	tSize CountUtf16Characters(const t16String& sText) {
		tSize wCount = 0;
		t16String::const_iterator wIterator = sText.begin();
		t16String::const_iterator wEnd = sText.end();
		
		while (wIterator != wEnd) {
			t16Char wCodeUnit = *wIterator;
			
			// Check if this is a high surrogate (0xD800-0xDBFF)
			if (wCodeUnit >= 0xD800 && wCodeUnit <= 0xDBFF) {
				// Check if next code unit is a low surrogate (0xDC00-0xDFFF)
				t16String::const_iterator wNext = wIterator;
				++wNext;
				if (wNext != wEnd) {
					t16Char wNextCodeUnit = *wNext;
					if (wNextCodeUnit >= 0xDC00 && wNextCodeUnit <= 0xDFFF) {
						// This is a surrogate pair = 1 character
						wCount++;
						++wIterator; // Skip the low surrogate
					} else {
						// Invalid: high surrogate without low surrogate, count as 1 character anyway
						wCount++;
					}
				} else {
					// High surrogate at end of string, count as 1 character
					wCount++;
				}
			} else {
				// Normal character (not a surrogate) = 1 character
				wCount++;
			}
			++wIterator;
		}
		
		return wCount;
	}
}