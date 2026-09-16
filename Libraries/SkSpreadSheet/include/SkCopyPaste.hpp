//=============================================================================
// SkCopyPaste (Copy and paste)
//=============================================================================
#ifndef SkCopyPaste_hpp
#define SkCopyPaste_hpp

#include "SkColRowCellRange.hpp"
#include <mutex>
using namespace SkRoot;

namespace SkSpreadSheet {

    class tSelect;

    //=========================================================================
    //! tCopy Call Json for each cell
    class tCopy : public tClass {
    private:
        //! Sheet to copy
        tSheet*			m_Sheet;
        //! Writer to write the JSON data
        Writer<StringBuffer>* m_Writer;
        //! Point to the top left cell of the selection
        tPoint			m_Point;
    public:
        /// @brief		Constructor tCopy.
        tCopy();

        /// @brief		Destructor tCopy.
        ~tCopy();

        /// @brief		Call back for cell in tIndex coordinate.
        /// @param[in]	sPoint tTempoPoint*
        /// @return		tBool
        tBool CallBackCell(tTempoPoint* sPoint);

        /// @brief		Call back for cell in tIndex coordinate.
        /// @param[in]	sRect tTempoRect*
        /// @return		tBool
        tBool CallBackRange(tTempoRect* sRect);

        /// @brief		Treat the copy operation.
        /// @param[in]	sRef tString
        /// @param[in]	sSheet tSheet*
        /// @param[out]	oJson optional copy JSON (avoids reading the app clipboard)
        /// @return		tBool
        tBool Treat(tString sRef, tSheet* sSheet, tString* oJson = nullptr);
    };

    /// @brief Build copy JSON for a selection (optionally update the app clipboard).
    tBool CopySelectionToJson(tString sRef, tSheet* sSheet, tString& sOutJson,
                              tBool sUpdateClipboard = false);

    /// @brief True when copy JSON includes at least one cell payload in `cells[]`.
    tBool CopyJsonHasCellPayload(const tString& sCopyJson);

    /// @brief After move Do: erase empty format-only ghost cells left in the former source block.
    void ClearMoveSourceExteriorNeighborGhosts(tSheet* sSheet, const tSelect& sSourceClearSelect,
                                               const tSelect& sMoveDest);

};

#endif // SkCopyPaste_hpp
