//=============================================================================
// Collaboration payload limits for undo messages (paste/move).
//=============================================================================
#ifndef SkCollaborationLimits_hpp
#define SkCollaborationLimits_hpp

#include "SkTools.hpp"

namespace SkSpreadSheet {

    //! Max copied JSON payload embedded in a collaboration Do message (paste/move).
    inline constexpr tSize kCollaborationPasteCopyMaxBytes = 384 * 1024;

    inline tBool IsCollaborationPastePayloadTooLarge(tSize sCopyBytes) {
        return sCopyBytes > kCollaborationPasteCopyMaxBytes;
    }

    inline const char* CollaborationPasteTooLargeMessage() {
        return "Copier-coller trop volumineux pour le mode collaboratif (max. 384 Ko). "
               "Reduisez la selection ou effectuez plusieurs operations.";
    }

} // namespace SkSpreadSheet

#endif // SkCollaborationLimits_hpp
