//=============================================================================
// SkClipboard  tClipboard
/**
 * @page tClipboard
 * @par
 * @par Interface with clipboard 
 */
//==============================================================================
#ifndef SkClipboard_hpp
#define SkClipboard_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
namespace SkRoot {
    class tClipboard : public tClass {
        private:
            tString m_Text;
        public:
            /// @brief Constructor for clipboard
            tClipboard();

            /// @brief Set copy text
            /// @param[in] sText Text to copy
            void Text(tString sText);

            /// @brief Get copy text
            /// @return Copied text
            tString Text();
    };
}
#endif
